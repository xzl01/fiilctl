#include "protocol.h"
#include "rfcomm.h"

#include <QDateTime>
#include <cstdio>

namespace fiil {

// 帧： [seq][cmd][type][0x00][len][payload…]（RFCOMM 的 FCS 由内核处理）
static constexpr int kHeaderLen = 5;

QString FiilClient::hexDump(const QByteArray &d)
{
    QString out;
    for (int i = 0; i < d.size(); ++i) {
        if (i)
            out += QLatin1Char(' ');
        out += QString("%1").arg(static_cast<quint8>(d.at(i)), 2, 16, QLatin1Char('0'));
    }
    return out;
}

FiilClient::FiilClient(QObject *parent) : QObject(parent)
{
    m_rf = new Rfcomm(this);
    connect(m_rf, &Rfcomm::connected, this, &FiilClient::onConnected);
    connect(m_rf, &Rfcomm::disconnected, this, &FiilClient::onDisconnected);
    connect(m_rf, &Rfcomm::failed, this, &FiilClient::onFailed);
    connect(m_rf, &Rfcomm::dataReceived, this, &FiilClient::onData);

    m_timeout.setSingleShot(true);
    m_timeout.setInterval(1500);
    connect(&m_timeout, &QTimer::timeout, this, &FiilClient::onTimeout);
}

void FiilClient::start(const QString &mac, quint8 channel)
{
    m_state = DeviceState{};
    m_rx.clear();
    m_queue.clear();
    m_pollQueue.clear();
    m_inFlight = false;
    m_ready = false;
    emit protocolLog(tr("连接 %1 · RFCOMM 通道 %2").arg(mac).arg(channel));
    m_rf->connectTo(mac, channel);
}

void FiilClient::stop()
{
    m_rf->disconnectFromDevice();
}

void FiilClient::onConnected()
{
    emit connectionChanged(true, tr("已连接，握手中…"));
    emit protocolLog(tr("握手 setUnlock {02 34}"));
    // 握手：DeviceInfo + commandId 0xff + buffer {2, 0x34}
    sendRequest(Cmd::DeviceInfo, QByteArray::fromHex("ff020234"));
}

void FiilClient::onDisconnected()
{
    m_ready = false;
    m_inFlight = false;
    m_timeout.stop();
    emit connectionChanged(false, tr("已断开"));
}

void FiilClient::onFailed(const QString &reason)
{
    emit errorOccurred(reason);
    emit connectionChanged(false, reason);
}

void FiilClient::onData(const QByteArray &data)
{
    m_rx.append(data);
    parseStream();
}

void FiilClient::parseStream()
{
    while (m_rx.size() >= kHeaderLen) {
        const quint8 seq = static_cast<quint8>(m_rx.at(0));
        const quint8 cmd = static_cast<quint8>(m_rx.at(1));
        const quint8 type = static_cast<quint8>(m_rx.at(2));
        const int len = static_cast<quint8>(m_rx.at(4));
        if (m_rx.size() < kHeaderLen + len)
            return;
        const QByteArray payload = m_rx.mid(kHeaderLen, len);
        m_rx.remove(0, kHeaderLen + len);
        handleFrame(seq, cmd, type, payload);
    }
}

void FiilClient::handleFrame(quint8 seq, quint8 cmd, quint8 type, const QByteArray &payload)
{
    Q_UNUSED(seq);
    QByteArray frame = buildFrame(cmd, type, payload);
    appendLog(type == Type::Notify ? QStringLiteral("RX 通知") : QStringLiteral("RX"), frame);

    if (type == Type::Notify) {
        if (cmd == Cmd::Notify && payload.size() >= 2) {
            const quint8 id = static_cast<quint8>(payload.at(0));
            const int dlen = static_cast<quint8>(payload.at(1));
            applyInfo(id, payload.mid(2, dlen));
            emit stateChanged();
        }
        return;
    }

    if (type != Type::Response)
        return;

    if (!m_inFlight)
        return;

    // 读响应：payload = [infoId][dlen][data…]；空 payload = 不支持
    if (cmd == Cmd::DeviceInfo && m_current.cmd == Cmd::DeviceInfo) {
        const quint8 wantId = static_cast<quint8>(m_current.payload.at(0));
        if (payload.isEmpty()) {
            m_state.unsupported.insert(wantId);
            if (wantId == 0xff)
                emit errorOccurred(tr("握手无响应"));
            emit protocolLog(tr("   INFO 0x%1 不支持（空响应）").arg(wantId, 2, 16, QLatin1Char('0')));
            finishRequest(false);
            return;
        }
        const quint8 id = static_cast<quint8>(payload.at(0));
        const int dlen = payload.size() >= 2 ? static_cast<quint8>(payload.at(1)) : 0;
        applyInfo(id, payload.mid(2, dlen));
        if (id == 0xff)
            startHandshakeDone();
        emit stateChanged();
        finishRequest(true);
        return;
    }

    // 写响应：payload 最后一字节是状态，0 = 成功
    finishRequest(true);
}

void FiilClient::startHandshakeDone()
{
    if (m_ready)
        return;
    m_ready = true;
    emit connectionChanged(true, tr("已连接"));
    emit protocolLog(tr("握手完成，开始读取参数"));
    refreshAll();
}

void FiilClient::applyInfo(quint8 id, const QByteArray &data)
{
    auto asInt = [&data](int idx) { return idx < data.size() ? static_cast<quint8>(data.at(idx)) : -1; };

    switch (id) {
    case Info::Power: {
        auto fill = [&data](int idx) {
            BatteryLevel b;
            if (idx < data.size()) {
                const quint8 v = static_cast<quint8>(data.at(idx));
                b.charging = (v & 0x80) != 0;
                b.level = v & 0x7f;
            }
            return b;
        };
        m_state.left = fill(0);
        m_state.right = fill(1);
        m_state.box = fill(2);
        break;
    }
    case Info::WorkMode:
        m_state.workMode = asInt(0);
        break;
    case Info::SingleMaf:
        m_state.singleNoise = asInt(0);
        break;
    case Info::Ldac:
        m_state.ldac = asInt(0);
        break;
    case Info::ChangLan:
        m_state.multipoint = asInt(0);
        break;
    case Info::Tone:
        m_state.tone = asInt(0);
        break;
    case Info::MafMode:
        m_state.mafMode = asInt(0);
        break;
    case Info::PlayState:
        m_state.playState = asInt(0);
        break;
    case Info::Language:
        m_state.language = asInt(0);
        break;
    case Info::Tuning:
        m_state.tuning = asInt(0);
        break;
    case Info::EqAdaptive:
        m_state.eqAdaptive = asInt(0);
        break;
    case Info::EarAdaptive:
        m_state.earAdaptive = asInt(0);
        break;
    case Info::NoiseAdaptive:
        m_state.noiseAdaptive = asInt(0);
        break;
    case Info::WindAdaptive:
        m_state.windAdaptive = asInt(0);
        break;
    case Info::Firmware: {
        QString v;
        for (int i = 0; i < data.size(); ++i)
            v += QString("%1%2").arg(static_cast<quint8>(data.at(i))).arg(i + 1 < data.size() ? "." : "");
        m_state.firmware = v;
        break;
    }
    case Info::Pid:
        if (data.size() >= 2)
            m_state.pid = static_cast<quint16>((static_cast<quint8>(data.at(0)) << 8) | static_cast<quint8>(data.at(1)));
        break;
    case Info::Eq:
        applyEq(data);
        break;
    case Info::Key:
        applyKeys(data);
        break;
    case Info::PairList:
        applyPairList(data);
        break;
    default:
        break;
    }
}

// EQ：data = [段数][模式][段数 * 增益(-10..10)]
void FiilClient::applyEq(const QByteArray &data)
{
    if (data.size() < 2)
        return;
    const int seg = static_cast<quint8>(data.at(0));
    if (seg <= 0 || data.size() < 2 + seg)
        return;
    m_state.eqBuffer = data.left(2 + seg);
}

// 按键：data = 若干组 [键位][值个数][值…]
void FiilClient::applyKeys(const QByteArray &data)
{
    QVector<KeyEntry> keys;
    int i = 0;
    while (i + 2 <= data.size()) {
        const int idx = static_cast<quint8>(data.at(i));
        const int count = static_cast<quint8>(data.at(i + 1));
        if (count < 1 || i + 2 + count > data.size())
            break;
        KeyEntry e;
        e.index = idx;
        e.action = static_cast<quint8>(data.at(i + 2));
        keys.append(e);
        i += 2 + count;
    }
    if (!keys.isEmpty())
        m_state.keys = keys;
}

// 配对列表：data = [条数] + 条数 * ([标志][MAC 6][名称 23])
void FiilClient::applyPairList(const QByteArray &data)
{
    if (data.size() < 1)
        return;
    const int count = static_cast<quint8>(data.at(0));
    const int recSize = count > 0 ? (data.size() - 1) / count : 0;
    if (recSize < 7)
        return;
    QVector<PairRecord> list;
    for (int i = 0; i < count; ++i) {
        const QByteArray rec = data.mid(1 + i * recSize, recSize);
        if (rec.size() < 7)
            break;
        PairRecord p;
        p.flags = static_cast<quint8>(rec.at(0));
        p.mac = rec.mid(1, 6);
        QByteArray nameBytes = rec.mid(7);
        const int nul = nameBytes.indexOf('\0');
        if (nul >= 0)
            nameBytes = nameBytes.left(nul);
        p.name = QString::fromUtf8(nameBytes);
        list.append(p);
    }
    m_state.pairList = list;
}

QString FiilClient::keyActionName(int action)
{
    switch (action) {
    case KeyAction::VoiceAssistant:
        return QObject::tr("语音助手");
    case KeyAction::Previous:
        return QObject::tr("上一首");
    case KeyAction::Next:
        return QObject::tr("下一首");
    case KeyAction::VolumeUp:
        return QObject::tr("音量加");
    case KeyAction::VolumeDown:
        return QObject::tr("音量减");
    case KeyAction::PlayPause:
        return QObject::tr("播放/暂停");
    case KeyAction::LowLatency:
        return QObject::tr("低延时");
    case KeyAction::NoiseControl:
        return QObject::tr("降噪设置");
    default:
        return QObject::tr("无操作");
    }
}

QString FiilClient::presetName(int mode)
{
    // 与 App 资源 preset_sound_name 一致，mode = 3 + 序号
    static const char *names[] = {"怀旧", "爵士", "轻柔", "剧院", "金属", "流行",
                                  "R&B", "舞曲", "摇滚", "电子", "重低", "律动"};
    const int idx = mode - 3;
    if (idx >= 0 && idx < static_cast<int>(sizeof(names) / sizeof(names[0])))
        return QString::fromUtf8(names[idx]);
    return {};
}

QByteArray FiilClient::buildFrame(quint8 cmd, quint8 type, const QByteArray &payload)
{
    QByteArray f;
    f.append(static_cast<char>(0)); // 请求方 seq 固定 0
    f.append(static_cast<char>(cmd));
    f.append(static_cast<char>(type));
    f.append(static_cast<char>(0));
    f.append(static_cast<char>(payload.size()));
    f.append(payload);
    return f;
}

void FiilClient::sendRequest(quint8 cmd, const QByteArray &payload)
{
    Request r;
    r.cmd = cmd;
    r.payload = payload;
    m_queue.enqueue(r);
    pump();
}

void FiilClient::pump()
{
    if (m_inFlight || m_queue.isEmpty() || !m_rf->isConnected())
        return;
    m_current = m_queue.dequeue();
    m_inFlight = true;
    const QByteArray frame = buildFrame(m_current.cmd, Type::Request, m_current.payload);
    appendLog(QStringLiteral("TX"), frame);
    m_rf->send(frame);
    m_timeout.start();
}

void FiilClient::finishRequest(bool ok)
{
    m_timeout.stop();
    m_inFlight = false;
    Q_UNUSED(ok);
    QTimer::singleShot(0, this, [this] { pump(); });
}

void FiilClient::onTimeout()
{
    if (!m_inFlight)
        return;
    if (m_current.cmd == Cmd::DeviceInfo && !m_current.payload.isEmpty()) {
        const quint8 id = static_cast<quint8>(m_current.payload.at(0));
        if (id != 0xff) {
            m_state.unsupported.insert(id);
            emit protocolLog(tr("   INFO 0x%1 超时，标记为不支持").arg(id, 2, 16, QLatin1Char('0')));
            emit stateChanged();
        }
    }
    finishRequest(false);
}

void FiilClient::appendLog(const QString &dir, const QByteArray &frame)
{
    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    emit protocolLog(QString("%1 %2 %3").arg(ts, dir, hexDump(frame)));
}

void FiilClient::refreshAll()
{
    m_pollQueue.clear();
    const quint8 ids[] = {
        Info::Power,    Info::Firmware, Info::Pid,      Info::WorkMode, Info::SingleMaf,
        Info::Ldac,     Info::ChangLan, Info::Tone,     Info::MafMode,  Info::PlayState,
        Info::Language, Info::Tuning,   Info::EqAdaptive, Info::EarAdaptive,
        Info::NoiseAdaptive, Info::WindAdaptive, Info::Eq, Info::Key, Info::PairList,
    };
    for (quint8 id : ids)
        m_pollQueue.enqueue(id);
    // 依次读取（每个请求一次，队列里会串行执行）
    while (!m_pollQueue.isEmpty() && m_queue.size() < 64) {
        const quint8 id = m_pollQueue.dequeue();
        if (m_state.unsupported.contains(id))
            continue;
        QByteArray p;
        p.append(static_cast<char>(id));
        p.append(static_cast<char>(0));
        sendRequest(Cmd::DeviceInfo, p);
    }
}

void FiilClient::requestInfo(quint8 id)
{
    QByteArray p;
    p.append(static_cast<char>(id));
    p.append(static_cast<char>(0));
    sendRequest(Cmd::DeviceInfo, p);
}

void FiilClient::setLowLatency(bool on)
{
    QByteArray p;
    p.append(static_cast<char>(on ? 1 : 0));
    sendRequest(Cmd::WorkMode, p);
    m_state.workMode = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setSingleNoise(bool on)
{
    QByteArray p;
    p.append(static_cast<char>(Setting::SingleNoise));
    p.append(static_cast<char>(1));
    p.append(static_cast<char>(on ? 1 : 0));
    sendRequest(Cmd::AppSetting, p);
    m_state.singleNoise = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setLdac(bool on)
{
    QByteArray p;
    p.append(static_cast<char>(Setting::Ldac));
    p.append(static_cast<char>(1));
    p.append(static_cast<char>(on ? 1 : 0));
    sendRequest(Cmd::AppSetting, p);
    m_state.ldac = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setMultipoint(bool on)
{
    QByteArray p;
    p.append(static_cast<char>(Setting::Multipoint));
    p.append(static_cast<char>(1));
    p.append(static_cast<char>(on ? 1 : 0));
    sendRequest(Cmd::AppSetting, p);
    m_state.multipoint = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setTone(int value)
{
    QByteArray p;
    p.append(static_cast<char>(Setting::PromptTone));
    p.append(static_cast<char>(1));
    p.append(static_cast<char>(value));
    sendRequest(Cmd::AppSetting, p);
    m_state.tone = value;
    emit stateChanged();
}

void FiilClient::setMafMode(int mode)
{
    QByteArray p;
    p.append(static_cast<char>(mode));
    sendRequest(Cmd::AncMode, p);
    m_state.mafMode = mode;
    emit stateChanged();
}

// APP_SETTING 通用写法：payload = [controlType][1][value]
static QByteArray appSettingPayload(int controlType, int value)
{
    QByteArray p;
    p.append(static_cast<char>(controlType));
    p.append(static_cast<char>(1));
    p.append(static_cast<char>(value));
    return p;
}

void FiilClient::setEqAdaptive(bool on)
{
    sendRequest(Cmd::AppSetting, appSettingPayload(Setting::EqAdaptive, on ? 1 : 0));
    m_state.eqAdaptive = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setEarAdaptive(bool on)
{
    sendRequest(Cmd::AppSetting, appSettingPayload(Setting::EarAdaptive, on ? 1 : 0));
    m_state.earAdaptive = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setNoiseAdaptive(bool on)
{
    sendRequest(Cmd::AppSetting, appSettingPayload(Setting::NoiseAdaptive, on ? 1 : 0));
    m_state.noiseAdaptive = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setWindAdaptive(bool on)
{
    sendRequest(Cmd::AppSetting, appSettingPayload(Setting::WindAdaptive, on ? 1 : 0));
    m_state.windAdaptive = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setTuning(bool on)
{
    sendRequest(Cmd::AppSetting, appSettingPayload(Setting::Tuning, on ? 1 : 0));
    m_state.tuning = on ? 1 : 0;
    emit stateChanged();
}

void FiilClient::setLanguage(int type)
{
    QByteArray p;
    p.append(static_cast<char>(type));
    sendRequest(Cmd::Language, p);
    m_state.language = type;
    emit stateChanged();
}

void FiilClient::factoryReset()
{
    sendRequest(Cmd::FactoryReset, QByteArray());
}

// 音乐控制：payload = [commandId][bufLen][buffer]，commandId 1=音量(值 0/1)/2=播放/3=暂停/4=上一首/5=下一首
void FiilClient::musicControl(int action)
{
    QByteArray p;
    switch (action) {
    case 0: // 音量减
    case 1: // 音量加
        p.append(static_cast<char>(1));
        p.append(static_cast<char>(1));
        p.append(static_cast<char>(action));
        break;
    case 2: // 播放
        p.append(static_cast<char>(2));
        p.append(static_cast<char>(0));
        break;
    case 3: // 暂停
        p.append(static_cast<char>(3));
        p.append(static_cast<char>(0));
        break;
    case 4: // 上一首
        p.append(static_cast<char>(4));
        p.append(static_cast<char>(0));
        break;
    case 5: // 下一首
        p.append(static_cast<char>(5));
        p.append(static_cast<char>(0));
        break;
    default:
        return;
    }
    sendRequest(Cmd::MusicControl, p);
}

// ==== 调音 ====
void FiilClient::setEqPreset(int mode)
{
    if (m_state.eqBuffer.size() < 2) {
        emit errorOccurred(tr("还没读到 EQ 数据，先点「读取」"));
        return;
    }
    QByteArray buff = m_state.eqBuffer;
    buff[1] = static_cast<char>(mode);
    sendRequest(Cmd::Eq, buff);
    m_state.eqBuffer = buff;
    emit stateChanged();
}

void FiilClient::setEqCustom(const QVector<int> &gains)
{
    const int seg = 10;
    QByteArray buff;
    buff.append(static_cast<char>(seg));
    buff.append(static_cast<char>(15)); // 自定义模式
    for (int i = 0; i < seg; ++i)
        buff.append(static_cast<char>(i < gains.size() ? gains.at(i) : 0));
    sendRequest(Cmd::Eq, buff);
    m_state.eqBuffer = buff;
    emit stateChanged();
}

// ==== 按键：payload = [键位][1][动作] ====
void FiilClient::setKeyAction(int index, int action)
{
    QByteArray p;
    p.append(static_cast<char>(index));
    p.append(static_cast<char>(1));
    p.append(static_cast<char>(action));
    sendRequest(Cmd::Key, p);
}

// ==== 配对列表管理：AppSetting type 6，buffer = [动作][MAC…] ====
void FiilClient::pairAction(int action, const QByteArray &mac)
{
    QByteArray p;
    p.append(static_cast<char>(Setting::PairList));
    p.append(static_cast<char>(mac.size() + 1)); // 值个数
    p.append(static_cast<char>(action));
    p.append(mac);
    sendRequest(Cmd::AppSetting, p);
}

void FiilClient::startPairing()
{
    pairAction(1, QByteArray()); // 1 = 进入配对模式
}

} // namespace fiil

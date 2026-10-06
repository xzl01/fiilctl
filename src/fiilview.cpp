#include "fiilview.h"
#include "bluetooth_devices.h"
#include "protocol.h"

#include <QVariantMap>
#include <cstdio>

namespace fiil {

FiilView::FiilView(QObject *parent) : QObject(parent)
{
    m_client = new FiilClient(this);
    connect(m_client, &FiilClient::stateChanged, this, &FiilView::stateChanged);
    connect(m_client, &FiilClient::protocolLog, this, &FiilView::logLine);
    connect(m_client, &FiilClient::connectionChanged, this, [this](bool, const QString &desc) {
        syncStatus(desc);
    });
    connect(m_client, &FiilClient::errorOccurred, this, [this](const QString &msg) {
        emit logLine(QStringLiteral("!! %1").arg(msg));
        syncStatus(msg);
    });
}

void FiilView::syncStatus(const QString &text)
{
    if (m_status == text)
        return;
    m_status = text;
    emit statusChanged();
}

bool FiilView::ready() const
{
    return m_client->isReady();
}

bool FiilView::connected() const
{
    return m_client->isReady();
}


int FiilView::leftLevel() const { return m_client->state().left.level; }
int FiilView::rightLevel() const { return m_client->state().right.level; }
int FiilView::boxLevel() const { return m_client->state().box.level; }
bool FiilView::leftCharging() const { return m_client->state().left.charging; }
bool FiilView::rightCharging() const { return m_client->state().right.charging; }
bool FiilView::boxCharging() const { return m_client->state().box.charging; }
QString FiilView::firmware() const { return m_client->state().firmware; }
QString FiilView::unsupported() const
{
    const auto &set = m_client->state().unsupported;
    if (set.isEmpty())
        return {};
    QStringList ids;
    for (quint8 id : set)
        ids << QString("0x%1").arg(id, 2, 16, QLatin1Char('0'));
    ids.sort();
    return ids.join(' ');
}
QString FiilView::pid() const
{
    const quint16 p = m_client->state().pid;
    return p ? QString("0x%1").arg(p, 4, 16, QLatin1Char('0')) : QString();
}

int FiilView::lowLatency() const { return m_client->state().workMode; }
int FiilView::singleNoise() const { return m_client->state().singleNoise; }
int FiilView::ldac() const { return m_client->state().ldac; }
int FiilView::multipoint() const { return m_client->state().multipoint; }
int FiilView::tone() const { return m_client->state().tone; }
int FiilView::mafMode() const { return m_client->state().mafMode; }
int FiilView::eqAdaptive() const { return m_client->state().eqAdaptive; }
int FiilView::earAdaptive() const { return m_client->state().earAdaptive; }
int FiilView::noiseAdaptive() const { return m_client->state().noiseAdaptive; }
int FiilView::windAdaptive() const { return m_client->state().windAdaptive; }
int FiilView::tuning() const { return m_client->state().tuning; }
int FiilView::language() const { return m_client->state().language; }
int FiilView::playState() const { return m_client->state().playState; }

int FiilView::eqMode() const
{
    const QByteArray &eq = m_client->state().eqBuffer;
    return eq.size() >= 2 ? static_cast<quint8>(eq.at(1)) : -1;
}

QVariantList FiilView::eqGains() const
{
    QVariantList out;
    const QByteArray &eq = m_client->state().eqBuffer;
    if (eq.size() < 2)
        return out;
    const int seg = static_cast<quint8>(eq.at(0));
    for (int i = 0; i < seg && i + 2 < eq.size(); ++i)
        out.append(static_cast<int>(static_cast<signed char>(eq.at(i + 2))));
    return out;
}

bool FiilView::eqLoaded() const
{
    return m_client->state().eqBuffer.size() >= 2;
}

QVariantList FiilView::keys() const
{
    QVariantList out;
    for (const auto &k : m_client->state().keys) {
        QVariantMap m;
        m.insert("index", k.index);
        m.insert("action", k.action);
        out.append(m);
    }
    return out;
}

QVariantList FiilView::pairList() const
{
    QVariantList out;
    for (const auto &p : m_client->state().pairList) {
        QVariantMap m;
        m.insert("name", p.name.isEmpty() ? tr("(无名)") : p.name);
        m.insert("mac", p.macString());
        m.insert("connected", p.flags == 1);
        out.append(m);
    }
    return out;
}

void FiilView::refreshDevices()
{
    m_devices.clear();
    for (const auto &d : BluetoothDevices::list()) {
        QVariantMap m;
        m.insert("name", d.name);
        m.insert("address", d.address);
        m.insert("connected", d.connected);
        m_devices.append(m);
    }
    emit devicesChanged();
}

void FiilView::connectDevice(const QString &mac, int channel)
{
    m_deviceName = mac;
    m_status = tr("连接中…");
    emit statusChanged();
    emit stateChanged();
    m_client->start(mac, static_cast<quint8>(channel));
}

void FiilView::disconnectDevice()
{
    m_client->stop();
    m_status = tr("已断开");
    emit statusChanged();
    emit stateChanged();
}

void FiilView::refreshAll() { m_client->refreshAll(); }

void FiilView::setLowLatency(bool on) { m_client->setLowLatency(on); }
void FiilView::setSingleNoise(bool on) { m_client->setSingleNoise(on); }
void FiilView::setLdac(bool on) { m_client->setLdac(on); }
void FiilView::setMultipoint(bool on) { m_client->setMultipoint(on); }
void FiilView::setTone(int value) { m_client->setTone(value); }
void FiilView::setMafMode(int mode) { m_client->setMafMode(mode); }
void FiilView::setEqAdaptive(bool on) { m_client->setEqAdaptive(on); }
void FiilView::setEarAdaptive(bool on) { m_client->setEarAdaptive(on); }
void FiilView::setNoiseAdaptive(bool on) { m_client->setNoiseAdaptive(on); }
void FiilView::setWindAdaptive(bool on) { m_client->setWindAdaptive(on); }
void FiilView::setTuning(bool on) { m_client->setTuning(on); }
void FiilView::setLanguage(int type) { m_client->setLanguage(type); }
void FiilView::factoryReset() { m_client->factoryReset(); }
void FiilView::musicControl(int action) { m_client->musicControl(action); }

void FiilView::readEq() { m_client->readEq(); }

void FiilView::setEqPreset(int mode) { m_client->setEqPreset(mode); }

void FiilView::setEqCustom(const QVariantList &gains)
{
    QVector<int> g;
    g.reserve(gains.size());
    for (const QVariant &v : gains)
        g.append(v.toInt());
    m_client->setEqCustom(g);
}

void FiilView::readKeys() { m_client->readKeys(); }
void FiilView::setKeyAction(int index, int action) { m_client->setKeyAction(index, action); }
void FiilView::readPairList() { m_client->readPairList(); }

void FiilView::pairAction(int action, const QString &mac)
{
    QByteArray raw;
    for (const QString &part : mac.split(':'))
        raw.append(static_cast<char>(part.toUInt(nullptr, 16)));
    m_client->pairAction(action, raw);
}

void FiilView::startPairing() { m_client->startPairing(); }

QString FiilView::keyActionName(int action) { return FiilClient::keyActionName(action); }
QString FiilView::presetName(int mode) { return FiilClient::presetName(mode); }

} // namespace fiil

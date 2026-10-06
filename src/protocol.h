// FIIL（中科蓝讯 Bluetrum）私有协议：帧编解码 + 命令队列 + 状态缓存
#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVector>

namespace fiil {

class Rfcomm;

namespace Cmd {
enum : quint8 {
    AppSetting = 0x01,
    Eq = 0x20,
    MusicControl = 0x21,
    Key = 0x22,
    AutoShutdown = 0x23,
    FactoryReset = 0x24,
    WorkMode = 0x25, // 低延时模式：payload {0/1}
    InEarDetect = 0x26,
    DeviceInfo = 0x27, // 读参数：payload {infoId, 0}
    Notify = 0x28,     // 耳机主动上报：payload {infoId, len, data…}
    FindDevice = 0x2A,
    Language = 0x29,
    AncMode = 0x2C, // 降噪模式：payload {0=关,1=开,2=通透}
};
}

namespace Type {
enum : quint8 { Request = 1, Response = 2, Notify = 3 };
}

// INFO id（按 FIILF049Command = FIIL Key Pro2；其它型号是子集）
namespace Info {
enum : quint8 {
    Power = 0x01,         // 电量 [左,右,盒]，每字节 bit7=充电中，bit0-6=0-100
    Firmware = 0x02,      // 固件版本
    Eq = 0x04,
    Key = 0x05,
    PlayState = 0x07,
    WorkMode = 0x08,      // 低延时
    Language = 0x0A,
    MafMode = 0x0C,       // 降噪模式
    ChangLan = 0x80,      // 多设备/畅连
    Tone = 0x82,          // 提示音音量
    Pid = 0x84,
    Tuning = 0x87,
    PairList = 0x88,      // 蓝牙配对列表
    EqAdaptive = 0x89,
    EarAdaptive = 0x8A,
    NoiseAdaptive = 0x8B,
    WindAdaptive = 0x8C,
    Ldac = 0x8F,
    SingleMaf = 0x90,     // 单耳降噪
};
}

// APP_SETTING 的 controlType
namespace Setting {
enum : quint8 {
    Multipoint = 1,     // 蓝牙畅连
    PromptTone = 3,     // 提示音音量
    PairList = 6,       // 配对列表管理
    Tuning = 8,
    Ldac = 13,
    EqAdaptive = 14,
    EarAdaptive = 15,
    NoiseAdaptive = 16,
    WindAdaptive = 17,
    SingleNoise = 18,   // 单耳降噪
};
}

struct BatteryLevel {
    int level = -1; // -1 = 未知
    bool charging = false;
    bool valid() const { return level >= 0; }
};

struct PairRecord {
    quint8 flags = 0; // 0=历史配对, 1=当前连接
    QByteArray mac;   // 6 字节
    QString name;
    QString macString() const
    {
        QStringList parts;
        for (char c : mac)
            parts << QString("%1").arg(static_cast<quint8>(c), 2, 16, QLatin1Char('0')).toUpper();
        return parts.join(':');
    }
};

struct KeyEntry {
    int index = 0;
    int action = 0;
};

// 按键动作码（来自 F049SetFragment 的 switch）
namespace KeyAction {
enum : int {
    None = 0,
    VoiceAssistant = 2,
    Previous = 3,
    Next = 4,
    VolumeUp = 5,
    VolumeDown = 6,
    PlayPause = 7,
    LowLatency = 8,
    NoiseControl = 9,
};
}

struct DeviceState {
    BatteryLevel left, right, box;
    int workMode = -1;      // 0 普通 / 1 低延时
    int singleNoise = -1;   // 单耳降噪
    int ldac = -1;          // LDAC 开关
    int multipoint = -1;    // 畅连
    int tone = -1;          // 提示音音量 0静音/1中/2低/3高
    int mafMode = -1;       // 降噪模式 0关/1开/2通透
    int playState = -1;
    int language = -1;
    int eqAdaptive = -1;    // EQ 自适应
    int earAdaptive = -1;   // 入耳自适应
    int noiseAdaptive = -1; // 降噪自适应
    int windAdaptive = -1;  // 风噪自适应
    int tuning = -1;        // 云感调音
    QString firmware;
    quint16 pid = 0;
    QByteArray eqBuffer;              // [段数=10][模式][10 段增益(-10..10)]
    QVector<KeyEntry> keys;           // 按键映射
    QVector<PairRecord> pairList;     // 配对列表
    QSet<quint8> unsupported;         // 探测到「不支持」的 INFO id
};

class FiilClient : public QObject {
    Q_OBJECT
public:
    explicit FiilClient(QObject *parent = nullptr);

    void start(const QString &mac, quint8 channel);
    void stop();
    bool isReady() const { return m_ready; }
    const DeviceState &state() const { return m_state; }

    // 控制
    void setLowLatency(bool on);
    void setSingleNoise(bool on);
    void setLdac(bool on);
    void setMultipoint(bool on);
    void setTone(int value);   // 0静音 1中 2低 3高
    void setMafMode(int mode); // 0关 1开 2通透
    void setEqAdaptive(bool on);
    void setEarAdaptive(bool on);
    void setNoiseAdaptive(bool on);
    void setWindAdaptive(bool on);
    void setTuning(bool on);
    void setLanguage(int type);
    void factoryReset();
    // 音乐控制：0=音量减 1=音量加 2=播放 3=暂停 4=上一首 5=下一首
    void musicControl(int action);
    void refreshAll();
    void requestInfo(quint8 id);

    // 调音 / 按键 / 配对列表
    void setEqPreset(int mode);                 // 预设：mode = 3 + 预设序号
    void setEqCustom(const QVector<int> &gains); // 自定义：mode = 15
    void readEq() { requestInfo(Info::Eq); }
    void setKeyAction(int index, int action);
    void readKeys() { requestInfo(Info::Key); }
    void readPairList() { requestInfo(Info::PairList); }
    void pairAction(int action, const QByteArray &mac); // 4=断开 5=连接 6=删除
    void startPairing();                               // 进入配对模式

    static QString keyActionName(int action);
    static QString presetName(int mode); // mode 3..14

    static QString hexDump(const QByteArray &d);

signals:
    void connectionChanged(bool connected, const QString &description);
    void stateChanged();
    void protocolLog(const QString &line);
    void errorOccurred(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onFailed(const QString &reason);
    void onData(const QByteArray &data);
    void onTimeout();

private:
    struct Request {
        quint8 cmd = 0;
        QByteArray payload;
    };

    static QByteArray buildFrame(quint8 cmd, quint8 type, const QByteArray &payload);
    void parseStream();
    void handleFrame(quint8 seq, quint8 cmd, quint8 type, const QByteArray &payload);
    void applyInfo(quint8 id, const QByteArray &data);
    void applyEq(const QByteArray &data);
    void applyKeys(const QByteArray &data);
    void applyPairList(const QByteArray &data);
    void sendRequest(quint8 cmd, const QByteArray &payload);
    void pump();
    void startHandshakeDone();
    void finishRequest(bool ok);
    void appendLog(const QString &dir, const QByteArray &frame);

    Rfcomm *m_rf = nullptr;
    QTimer m_timeout;
    QByteArray m_rx;
    QQueue<Request> m_queue;
    Request m_current;
    bool m_inFlight = false;
    bool m_ready = false;
    DeviceState m_state;
    QQueue<quint8> m_pollQueue;
};

} // namespace fiil

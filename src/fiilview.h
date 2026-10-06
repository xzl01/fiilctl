// 把协议层包装成 QML 友好的后端
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>

namespace fiil {

class FiilClient;

class FiilView : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool ready READ ready NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY stateChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)

    Q_PROPERTY(int leftLevel READ leftLevel NOTIFY stateChanged)
    Q_PROPERTY(int rightLevel READ rightLevel NOTIFY stateChanged)
    Q_PROPERTY(int boxLevel READ boxLevel NOTIFY stateChanged)
    Q_PROPERTY(bool leftCharging READ leftCharging NOTIFY stateChanged)
    Q_PROPERTY(bool rightCharging READ rightCharging NOTIFY stateChanged)
    Q_PROPERTY(bool boxCharging READ boxCharging NOTIFY stateChanged)
    Q_PROPERTY(QString firmware READ firmware NOTIFY stateChanged)
    Q_PROPERTY(QString pid READ pid NOTIFY stateChanged)
    Q_PROPERTY(QString unsupported READ unsupported NOTIFY stateChanged)

    Q_PROPERTY(int lowLatency READ lowLatency NOTIFY stateChanged)
    Q_PROPERTY(int singleNoise READ singleNoise NOTIFY stateChanged)
    Q_PROPERTY(int ldac READ ldac NOTIFY stateChanged)
    Q_PROPERTY(int multipoint READ multipoint NOTIFY stateChanged)
    Q_PROPERTY(int tone READ tone NOTIFY stateChanged)
    Q_PROPERTY(int mafMode READ mafMode NOTIFY stateChanged)
    Q_PROPERTY(int eqAdaptive READ eqAdaptive NOTIFY stateChanged)
    Q_PROPERTY(int earAdaptive READ earAdaptive NOTIFY stateChanged)
    Q_PROPERTY(int noiseAdaptive READ noiseAdaptive NOTIFY stateChanged)
    Q_PROPERTY(int windAdaptive READ windAdaptive NOTIFY stateChanged)
    Q_PROPERTY(int tuning READ tuning NOTIFY stateChanged)
    Q_PROPERTY(int language READ language NOTIFY stateChanged)
    Q_PROPERTY(int playState READ playState NOTIFY stateChanged)

    Q_PROPERTY(int eqMode READ eqMode NOTIFY stateChanged)
    Q_PROPERTY(QVariantList eqGains READ eqGains NOTIFY stateChanged)
    Q_PROPERTY(bool eqLoaded READ eqLoaded NOTIFY stateChanged)

    Q_PROPERTY(QVariantList keys READ keys NOTIFY stateChanged)
    Q_PROPERTY(QVariantList pairList READ pairList NOTIFY stateChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)

public:
    explicit FiilView(QObject *parent = nullptr);

    bool ready() const;
    QString status() const { return m_status; }
    QString deviceName() const { return m_deviceName; }
    bool connected() const;

    int leftLevel() const;
    int rightLevel() const;
    int boxLevel() const;
    bool leftCharging() const;
    bool rightCharging() const;
    bool boxCharging() const;
    QString firmware() const;
    QString pid() const;
    QString unsupported() const;

    int lowLatency() const;
    int singleNoise() const;
    int ldac() const;
    int multipoint() const;
    int tone() const;
    int mafMode() const;
    int eqAdaptive() const;
    int earAdaptive() const;
    int noiseAdaptive() const;
    int windAdaptive() const;
    int tuning() const;
    int language() const;
    int playState() const;

    int eqMode() const;
    QVariantList eqGains() const;
    bool eqLoaded() const;

    QVariantList keys() const;
    QVariantList pairList() const;
    QVariantList devices() const { return m_devices; }

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void connectDevice(const QString &mac, int channel);
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void refreshAll();

    Q_INVOKABLE void setLowLatency(bool on);
    Q_INVOKABLE void setSingleNoise(bool on);
    Q_INVOKABLE void setLdac(bool on);
    Q_INVOKABLE void setMultipoint(bool on);
    Q_INVOKABLE void setTone(int value);
    Q_INVOKABLE void setMafMode(int mode);
    Q_INVOKABLE void setEqAdaptive(bool on);
    Q_INVOKABLE void setEarAdaptive(bool on);
    Q_INVOKABLE void setNoiseAdaptive(bool on);
    Q_INVOKABLE void setWindAdaptive(bool on);
    Q_INVOKABLE void setTuning(bool on);
    Q_INVOKABLE void setLanguage(int type);
    Q_INVOKABLE void factoryReset();
    Q_INVOKABLE void musicControl(int action);

    Q_INVOKABLE void readEq();
    Q_INVOKABLE void setEqPreset(int mode);
    Q_INVOKABLE void setEqCustom(const QVariantList &gains);

    Q_INVOKABLE void readKeys();
    Q_INVOKABLE void setKeyAction(int index, int action);

    Q_INVOKABLE void readPairList();
    Q_INVOKABLE void pairAction(int action, const QString &mac);
    Q_INVOKABLE void startPairing();

    Q_INVOKABLE static QString keyActionName(int action);
    Q_INVOKABLE static QString presetName(int mode);

signals:
    void stateChanged();
    void statusChanged();
    void devicesChanged();
    void logLine(const QString &line);

private:
    void syncStatus(const QString &text);

    FiilClient *m_client = nullptr;
    QString m_status = QObject::tr("未连接");
    QString m_deviceName;
    QVariantList m_devices;
};

} // namespace fiil

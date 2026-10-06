// 通过 BlueZ D-Bus 列出已配对/已知的蓝牙设备（用于设备下拉框）
#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace fiil {

struct PairedDevice {
    QString address;
    QString name;
    bool connected = false;
    bool paired = false;
};

class BluetoothDevices : public QObject {
    Q_OBJECT
public:
    static QVector<PairedDevice> list();
    static QString localAdapterAddress();
};

} // namespace fiil

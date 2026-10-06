#include "bluetooth_devices.h"

#include <QProcess>
#include <QRegularExpression>
#include <QStringList>

namespace fiil {

// 直接问 bluetoothctl：比手写蓝护 D-Bus 的 a{oa{sa{sv}}} 解析稳得多，且 bluez-utils 本来就是蓝牙栈必备
static QStringList runBluetoothctl(const QStringList &args)
{
    QProcess p;
    p.start(QStringLiteral("bluetoothctl"), args);
    if (!p.waitForStarted(3000))
        return {};
    if (!p.waitForFinished(5000)) {
        p.kill();
        p.waitForFinished(1000);
    }
    const QString text = QString::fromUtf8(p.readAllStandardOutput());
    return text.split('\n', Qt::SkipEmptyParts);
}

QVector<PairedDevice> BluetoothDevices::list()
{
    QVector<PairedDevice> out;
    QHash<QString, int> index;

    const QRegularExpression re(QStringLiteral("^Device\\s+([0-9A-Fa-f:]{17})\\s*(.*)$"));

    // 1) 已配对/已知设备（名字来自 BlueZ 缓存）
    for (const QString &line : runBluetoothctl({QStringLiteral("devices")})) {
        const auto m = re.match(line.trimmed());
        if (!m.hasMatch())
            continue;
        PairedDevice d;
        d.address = m.captured(1).toUpper();
        d.name = m.captured(2).trimmed();
        if (d.name.isEmpty())
            d.name = d.address;
        d.paired = true;
        if (!index.contains(d.address)) {
            index.insert(d.address, out.size());
            out.append(d);
        }
    }

    // 2) 标记已连接
    for (const QString &line : runBluetoothctl({QStringLiteral("devices"), QStringLiteral("Connected")})) {
        const auto m = re.match(line.trimmed());
        if (!m.hasMatch())
            continue;
        const QString addr = m.captured(1).toUpper();
        if (index.contains(addr))
            out[index.value(addr)].connected = true;
    }

    std::sort(out.begin(), out.end(), [](const PairedDevice &a, const PairedDevice &b) {
        const bool aFiil = a.name.contains(QStringLiteral("FIIL"), Qt::CaseInsensitive);
        const bool bFiil = b.name.contains(QStringLiteral("FIIL"), Qt::CaseInsensitive);
        if (aFiil != bFiil)
            return aFiil;
        if (a.connected != b.connected)
            return a.connected;
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });
    return out;
}

QString BluetoothDevices::localAdapterAddress()
{
    const QRegularExpression re(QStringLiteral("^Controller\\s+([0-9A-Fa-f:]{17})"));
    for (const QString &line : runBluetoothctl({QStringLiteral("list")})) {
        const auto m = re.match(line.trimmed());
        if (m.hasMatch())
            return m.captured(1).toUpper();
    }
    return {};
}

} // namespace fiil

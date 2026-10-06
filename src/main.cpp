#include "bluetooth_devices.h"
#include "fiilview.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTextStream>
#include <qqml.h>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>
#include <QQmlError>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("fiilctl"));
    app.setApplicationDisplayName(QStringLiteral("FIIL 耳机控制台"));
    app.setDesktopFileName(QStringLiteral("fiilctl"));

    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--list")) {
        QTextStream out(stdout);
        out << "adapter: " << fiil::BluetoothDevices::localAdapterAddress() << "\n";
        for (const auto &d : fiil::BluetoothDevices::list())
            out << d.address << "  " << (d.paired ? "paired" : "-") << (d.connected ? " connected" : "") << "  " << d.name << "\n";
        return 0;
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    qmlRegisterType<fiil::FiilView>("Fiilctl.Backend", 1, 0, "FiilView");

    // 离屏截图模式：--shot <页面序号> <输出.png>（用于不打扰桌面的自查/出图）
    int shotPage = -1;
    QString shotPath;
    if (argc > 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--shot")) {
        shotPage = QString::fromLocal8Bit(argv[2]).toInt();
        shotPath = QString::fromLocal8Bit(argv[3]);
    }

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlEngine::warnings, [](const QList<QQmlError> &errs) {
        for (const QQmlError &e : errs)
            std::fprintf(stderr, "[qml] %s\n", qPrintable(e.toString()));
    });
    engine.loadFromModule("Fiilctl.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "[qml] 加载失败：没有 root object\n");
        return 1;
    }

    if (shotPage >= 0) {
        auto *win = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QTimer::singleShot(2500, [win, shotPage, shotPath] {
            if (!win) {
                qApp->quit();
                return;
            }
            win->resize(1020, 1130);
            win->setProperty("navIndex", shotPage);
            QTimer::singleShot(5000, [win, shotPath] {
                const QImage img = win->grabWindow();
                if (img.isNull())
                    std::fprintf(stderr, "[shot] grabWindow 返回空图\n");
                else
                    std::fprintf(stderr, "[shot] 已保存 %s (%dx%d)\n", qPrintable(shotPath), img.width(), img.height());
                img.save(shotPath);
                qApp->quit();
            });
        });
    }
    return app.exec();
}

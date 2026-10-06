#include "bluetooth_devices.h"
#include "fiilview.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>
#include <QTextStream>
#include <qqml.h>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>
#include <QQmlError>

#ifndef FIILCTL_VERSION
#define FIILCTL_VERSION "unknown"
#endif

int main(int argc, char *argv[])
{
    // 纯命令行模式（不需要图形环境，容器/脚本里也能跑）
    const QString firstArg = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    if (firstArg == QStringLiteral("--list") || firstArg == QStringLiteral("--version")
        || firstArg == QStringLiteral("-h") || firstArg == QStringLiteral("--help")) {
        QCoreApplication app(argc, argv);
        app.setApplicationName(QStringLiteral("fiilctl"));
        QTextStream out(stdout);
        if (firstArg == QStringLiteral("--version")) {
            out << "fiilctl " << FIILCTL_VERSION << "\n";
        } else if (firstArg == QStringLiteral("--list")) {
            out << "adapter: " << fiil::BluetoothDevices::localAdapterAddress() << "\n";
            for (const auto &d : fiil::BluetoothDevices::list())
                out << d.address << "  " << (d.paired ? "paired" : "-") << (d.connected ? " connected" : "") << "  " << d.name << "\n";
        } else {
            out << "fiilctl " << FIILCTL_VERSION << "\n\n"
                << "用法：\n"
                << "  fiilctl                 启动图形界面\n"
                << "  fiilctl --list          列出已知蓝牙设备（含是否已配对/已连接）\n"
                << "  fiilctl --version       版本\n";
        }
        return 0;
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("fiilctl"));
    app.setApplicationDisplayName(QStringLiteral("FIIL 耳机控制台"));
    app.setDesktopFileName(QStringLiteral("fiilctl"));

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
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Fiilctl/Ui/Main.qml")));
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

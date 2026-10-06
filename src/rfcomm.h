// 原生 AF_BLUETOOTH / RFCOMM 客户端（不依赖 QtBluetooth）
//
// 注意：Linux 的 RFCOMM 不支持非阻塞 connect（O_NONBLOCK 下 connect() 会直接返回 EBUSY），
// 所以 connect() 放在后台线程里用阻塞方式做，连上之后再把 fd 交回主线程并切成非阻塞 + QSocketNotifier。
#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

#include <atomic>
#include <thread>

class QSocketNotifier;

namespace fiil {

class Rfcomm : public QObject {
    Q_OBJECT
public:
    explicit Rfcomm(QObject *parent = nullptr);
    ~Rfcomm() override;

    bool connectTo(const QString &mac, quint8 channel);
    void disconnectFromDevice();
    bool isConnected() const { return m_connected; }

    qint64 send(const QByteArray &data);

signals:
    void connected();
    void disconnected();
    void failed(const QString &reason);
    void dataReceived(const QByteArray &data);

private slots:
    void onReadable();
    void onWritable();

private:
    void adoptFd(int fd, int generation);
    void closeFd();
    void flushPending();

    int m_fd = -1;
    bool m_connected = false;
    int m_generation = 0;
    QSocketNotifier *m_rd = nullptr;
    QSocketNotifier *m_wr = nullptr;
    QByteArray m_out;
    std::thread m_worker;
};

} // namespace fiil

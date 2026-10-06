#include "rfcomm.h"

#include <QMetaObject>
#include <QSocketNotifier>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fiil {

static bool setNonBlocking(int fd)
{
    const int flags = ::fcntl(fd, F_GETFL, 0);
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

static bool parseAddress(const QString &mac, bdaddr_t *addr)
{
    unsigned int b[6]{};
    const QByteArray ascii = mac.trimmed().toLatin1();
    if (::sscanf(ascii.constData(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6)
        return false;
    for (int i = 0; i < 6; ++i)
        addr->b[i] = static_cast<quint8>(b[5 - i]); // bdaddr_t 逆序存储
    return true;
}

Rfcomm::Rfcomm(QObject *parent) : QObject(parent) {}

Rfcomm::~Rfcomm()
{
    closeFd();
    if (m_worker.joinable())
        m_worker.join();
}

bool Rfcomm::connectTo(const QString &mac, quint8 channel)
{
    // 结束上一次连接（线程若还在阻塞 connect 中，就让它自然结束，用 generation 丢弃结果）
    closeFd();
    if (m_worker.joinable())
        m_worker.join();

    bdaddr_t addr{};
    if (!parseAddress(mac, &addr)) {
        emit failed(tr("蓝牙地址格式不对：%1").arg(mac));
        return false;
    }

    const int generation = ++m_generation;

    m_worker = std::thread([this, addr, channel, generation] {
        const int fd = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        if (fd < 0) {
            const QString err = QString::fromLocal8Bit(::strerror(errno));
            QMetaObject::invokeMethod(this, [this, generation, err] {
                if (generation == m_generation)
                    emit failed(tr("创建 RFCOMM socket 失败：%1").arg(err));
            }, Qt::QueuedConnection);
            return;
        }

        sockaddr_rc remote{};
        remote.rc_family = AF_BLUETOOTH;
        remote.rc_bdaddr = addr;
        remote.rc_channel = channel;

        // 阻塞式 connect（非阻塞在 RFCOMM 上会直接 EBUSY）
        const int rc = ::connect(fd, reinterpret_cast<sockaddr *>(&remote), sizeof(remote));
        if (rc < 0) {
            const int err = errno;
            ::close(fd);
            const QString msg = QString::fromLocal8Bit(::strerror(err));
            QMetaObject::invokeMethod(this, [this, generation, msg] {
                if (generation == m_generation)
                    emit failed(tr("连接失败：%1（耳机这条 SPP 一次只接受一个客户端，手机 App 是否在连着？）").arg(msg));
            }, Qt::QueuedConnection);
            return;
        }

        QMetaObject::invokeMethod(this, [this, fd, generation] { adoptFd(fd, generation); }, Qt::QueuedConnection);
    });

    return true;
}

void Rfcomm::adoptFd(int fd, int generation)
{
    if (generation != m_generation) { // 已被取消
        ::close(fd);
        return;
    }
    setNonBlocking(fd);
    m_fd = fd;
    m_connected = true;
    m_rd = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_rd, &QSocketNotifier::activated, this, &Rfcomm::onReadable);
    emit connected();
    flushPending();
}

void Rfcomm::onWritable()
{
    if (m_fd >= 0)
        flushPending();
}

void Rfcomm::onReadable()
{
    if (m_fd < 0)
        return;

    char buf[1024];
    while (true) {
        const ssize_t n = ::recv(m_fd, buf, sizeof(buf), 0);
        if (n > 0) {
            emit dataReceived(QByteArray(buf, static_cast<int>(n)));
            continue;
        }
        if (n == 0) {
            closeFd();
            emit disconnected();
            return;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        const QString reason = QString::fromLocal8Bit(::strerror(errno));
        closeFd();
        emit failed(tr("读取失败：%1").arg(reason));
        emit disconnected();
        return;
    }
}

qint64 Rfcomm::send(const QByteArray &data)
{
    if (m_fd < 0 || !m_connected)
        return -1;
    m_out.append(data);
    flushPending();
    return data.size();
}

void Rfcomm::flushPending()
{
    while (m_fd >= 0 && !m_out.isEmpty()) {
        const ssize_t n = ::send(m_fd, m_out.constData(), static_cast<size_t>(m_out.size()), MSG_NOSIGNAL);
        if (n > 0) {
            m_out.remove(0, static_cast<int>(n));
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!m_wr) {
                m_wr = new QSocketNotifier(m_fd, QSocketNotifier::Write, this);
                connect(m_wr, &QSocketNotifier::activated, this, &Rfcomm::onWritable);
            }
            return;
        }
        emit failed(tr("发送失败：%1").arg(QString::fromLocal8Bit(::strerror(errno))));
        return;
    }
}

void Rfcomm::disconnectFromDevice()
{
    ++m_generation; // 取消进行中的连接
    const bool wasConnected = m_connected;
    closeFd();
    if (m_worker.joinable() && m_worker.get_id() != std::this_thread::get_id())
        m_worker.join();
    if (wasConnected)
        emit disconnected();
}

void Rfcomm::closeFd()
{
    if (m_rd) {
        m_rd->setEnabled(false);
        delete m_rd;
        m_rd = nullptr;
    }
    if (m_wr) {
        m_wr->setEnabled(false);
        delete m_wr;
        m_wr = nullptr;
    }
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_connected = false;
    m_out.clear();
}

} // namespace fiil

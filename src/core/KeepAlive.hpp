// src/core/KeepAlive.hpp
// 自动保活 / Auto keep-alive.
// 随机间隔探测；仅掉线时触发登录，不做高频轮询，避免 Nginx 403 限流。
// Probes on a random interval; logs in only when offline — no tight polling.
#ifndef GDEI_KEEPALIVE_HPP
#define GDEI_KEEPALIVE_HPP

#include <QObject>
#include <QTimer>

namespace gdei {

class PortalClient;
class ConfigStore;
class Logger;

class KeepAlive : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
public:
    KeepAlive(PortalClient* portal, ConfigStore* config, Logger* logger,
              QObject* parent = nullptr);

    bool running() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void runningChanged();

private slots:
    void onTick();
    void onOnlineChanged(bool online);

private:
    PortalClient* m_portal;
    ConfigStore* m_cfg;
    Logger* m_log;
    QTimer* m_timer;
    bool m_running = false;

    void scheduleNext();
    int randomIntervalSec() const;
    void setRunning(bool v);
};

} // namespace gdei

#endif // GDEI_KEEPALIVE_HPP

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
    // 结构化事件：供 EventBridge 消费，替代英文日志前缀解析。
    // Structured signals consumed by EventBridge instead of parsing log prefixes.
    void probeFailed(int failed, int threshold);
    void reconnectScheduled(int attempt, int delaySec);
    void reconnectSucceeded(int attempts);
    void keepAliveError(const QString& reasonKey);

private slots:
    void onTick();
    void onProbeResult(bool online, QString reason);
    void onLoginFinished(bool ok, QString message);

private:
    PortalClient* m_portal;
    ConfigStore* m_cfg;
    Logger* m_log;
    QTimer* m_timer;
    // 重登退避定时器：与探测定时器分离，掉线后按指数退避重试。
    // Retry timer kept separate from the probe timer for exponential backoff.
    QTimer* m_retryTimer = nullptr;
    bool m_running = false;
    int m_failCount = 0;
    bool m_reconnectInFlight = false;
    int m_reconnectAttempts = 0;
    int m_backoffSec = 0;

    static constexpr int BASE_BACKOFF_SEC = 30;
    static constexpr int MAX_BACKOFF_SEC = 600;
    static constexpr int MAX_ATTEMPTS_WARN = 6;

    void scheduleNext();
    int randomIntervalSec() const;
    void setRunning(bool v);
    void requestReconnect();
    void scheduleReconnectRetry();
    void resetReconnectState();
};

} // namespace gdei

#endif // GDEI_KEEPALIVE_HPP

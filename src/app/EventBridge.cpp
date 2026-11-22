#include "app/EventBridge.hpp"
#include "app/AuthController.hpp"
#include "app/DeviceController.hpp"
#include "app/EventBus.hpp"
#include "app/LocalDeviceController.hpp"
#include "app/NetworkInfoController.hpp"
#include "app/SessionController.hpp"
#include "core/ConfigStore.hpp"
#include "core/Logger.hpp"
#include "core/Types.hpp"

namespace gdei {

EventBridge::EventBridge(EventBus* events, AuthController* auth, SessionController* session,
                         LocalDeviceController* localDevices, DeviceController* portalDevices,
                         NetworkInfoController* network, ConfigStore* config, Logger* logger,
                         QObject* parent)
    : QObject(parent), m_events(events), m_auth(auth), m_session(session), m_network(network) {
    connect(session, &SessionController::eventRequested, this,
            [this](int level, const QString& title, const QString& message,
                   const QString& reason, const QVariantMap& args) {
                forward(level, QStringLiteral("auth"), title, message, reason, args);
            });
    connect(localDevices, &LocalDeviceController::eventRequested, this,
            [this](int level, const QString& title, const QString& message,
                   const QString& reason, const QVariantMap& args) {
                forward(level, QStringLiteral("device"), title, message, reason, args);
            });
    connect(portalDevices, &DeviceController::loginFinished,
            this, &EventBridge::onPortalDevicesFinished);

    connect(auth, &AuthController::keepAliveRunningChanged, this, [this] {
        const bool running = m_auth->keepAliveRunning();
        publish(static_cast<int>(LogLevel::Info), QStringLiteral("keepalive"),
                running ? QStringLiteral("keepalive.started")
                        : QStringLiteral("keepalive.stopped"),
                running ? QStringLiteral("keepalive.startedMessage")
                        : QStringLiteral("keepalive.stoppedMessage"));
    });
    connect(auth, &AuthController::onlineChanged, this, [this] {
        if (m_session->operation() == SessionController::Logout) return;
        const bool online = m_auth->online();
        const QString reason = online ? QString()
            : (!m_network->available() ? QStringLiteral("network.reasonLocalUnavailable")
                                       : QStringLiteral("network.reasonUnknown"));
        publish(static_cast<int>(online ? LogLevel::Info : LogLevel::Warning),
                QStringLiteral("network"),
                online ? QStringLiteral("network.recovered")
                       : QStringLiteral("network.offline"),
                online ? QStringLiteral("network.recoveredMessage")
                       : QStringLiteral("network.offlineMessage"), reason);
    });
    connect(network, &NetworkInfoController::availableChanged, this, [this] {
        const bool available = m_network->available();
        publish(static_cast<int>(available ? LogLevel::Info : LogLevel::Warning),
                QStringLiteral("network"),
                available ? QStringLiteral("network.localRecovered")
                          : QStringLiteral("network.localUnavailable"),
                available ? QStringLiteral("network.localRecoveredMessage")
                          : QStringLiteral("network.localUnavailableMessage"),
                available ? QString() : QStringLiteral("network.reasonLocalUnavailable"));
    });
    connect(config, &ConfigStore::allowInsecureSslChanged, this, [this, config] {
        if (!config->allowInsecureSsl()) return;
        publish(static_cast<int>(LogLevel::Warning), QStringLiteral("config"),
                QStringLiteral("config.certificateWarning"),
                QStringLiteral("config.certificateWarningMessage"),
                QStringLiteral("config.reasonInsecureEnabled"));
    });
    connect(logger, &Logger::logAdded, this, &EventBridge::onLogAdded);
}

void EventBridge::publish(int level, const QString& source, const QString& titleKey,
                          const QString& messageKey, const QString& reasonKey,
                          const QVariantMap& arguments) {
    m_events->publish(level, source, titleKey, messageKey, reasonKey, arguments);
}

void EventBridge::forward(int level, const QString& source, const QString& titleKey,
                          const QString& messageKey, const QString& reasonKey,
                          const QVariantMap& arguments) {
    publish(level, source, titleKey, messageKey, reasonKey, arguments);
}

void EventBridge::onPortalDevicesFinished(bool ok, const QString& message) {
    // 会话过期现在以 i18n key 传递（key|arg 协议的 key 段），不再依赖英文原文匹配。
    // Session expiry now arrives as an i18n key; no English text matching.
    const bool expired = message == QLatin1String("portalDevices.sessionExpired");
    const bool encoding = message == QLatin1String("encoding.nonAscii") ||
                          message == QLatin1String("encoding.unmappable") ||
                          message == QLatin1String("encoding.invalidForm");
    publish(static_cast<int>(ok ? LogLevel::Info : LogLevel::Warning),
            QStringLiteral("device"),
            expired ? QStringLiteral("portalDevices.sessionExpired")
                    : (ok ? QStringLiteral("portalDevices.loginSuccess")
                          : QStringLiteral("portalDevices.operationFailed")),
            expired ? QStringLiteral("portalDevices.sessionExpiredMessage")
                    : (ok ? QStringLiteral("portalDevices.loginSuccessMessage")
                          : QStringLiteral("portalDevices.operationFailedMessage")),
            expired ? QStringLiteral("portalDevices.reasonSessionExpired")
                    : (encoding ? message : (ok ? QString()
                                                : QStringLiteral("portalDevices.reasonUnknown"))));
}

void EventBridge::onLogAdded(int level, const QString& message) {
    Q_UNUSED(level)
    if (message.startsWith(QLatin1String("Offline probe failed"))) {
        publish(static_cast<int>(LogLevel::Warning), QStringLiteral("keepalive"),
                QStringLiteral("keepalive.probeFailed"),
                QStringLiteral("keepalive.probeFailedMessage"),
                QStringLiteral("keepalive.reasonProbeFailed"));
    } else if (message.startsWith(QLatin1String("Offline after"))) {
        publish(static_cast<int>(LogLevel::Warning), QStringLiteral("keepalive"),
                QStringLiteral("keepalive.retry"),
                QStringLiteral("keepalive.retryMessage"),
                QStringLiteral("keepalive.reasonProbeFailed"));
    } else if (message.startsWith(QLatin1String("SSL verification skipped by config"))) {
        publish(static_cast<int>(LogLevel::Warning), QStringLiteral("config"),
                QStringLiteral("config.certificateWarning"),
                QStringLiteral("config.certificateWarningMessage"),
                QStringLiteral("config.reasonInsecureEnabled"));
    }
}

} // namespace gdei

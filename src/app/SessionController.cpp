#include "app/SessionController.hpp"
#include "app/AuthController.hpp"
#include "core/Types.hpp"

namespace gdei {

SessionController::SessionController(AuthController* auth, QObject* parent)
    : QObject(parent), m_auth(auth) {
    connect(m_auth, &AuthController::stateChanged, this, [this] {
        if (m_auth->state() == 2) emit busyChanged();
        else if (!m_busy) emit busyChanged();
    });
    connect(m_auth, &AuthController::loginFinished, this,
            [this](bool ok, const QString& message) {
                const Operation completed = m_operation;
                if (completed == Logout) {
                    finish(Logout, ok, message);
                    return;
                }
                if (completed == ManualLogin || completed == AutomaticLogin) {
                    finish(completed, ok, message);
                    return;
                }
                // KeepAlive 内部仍直接调用原 PortalClient::login()；这里只观察结果。
                if (m_loggedIn) {
                    if (!ok) setLastError(message);
                    emit eventRequested(static_cast<int>(ok ? LogLevel::Info : LogLevel::Warning),
                                        ok ? QStringLiteral("auth.reconnectSuccess")
                                           : QStringLiteral("auth.reconnectFailed"),
                                        ok ? QStringLiteral("auth.reconnectSuccessMessage")
                                           : QStringLiteral("auth.reconnectFailedMessage"),
                                        ok ? QString() : reasonForMessage(message), QVariantMap{});
                }
            });
}

bool SessionController::loggedIn() const { return m_loggedIn; }
bool SessionController::busy() const { return m_busy || m_auth->state() == 2; }
bool SessionController::loginPromptOpen() const { return m_loginPromptOpen; }
QString SessionController::route() const { return m_loggedIn ? QStringLiteral("main")
                                                              : QStringLiteral("login"); }
QString SessionController::lastError() const { return m_lastError; }
int SessionController::operation() const { return static_cast<int>(m_operation); }

void SessionController::login() { begin(ManualLogin); }
void SessionController::loginAutomatically() { begin(AutomaticLogin); }

void SessionController::promptLogin() { setLoginPromptOpen(true); }

void SessionController::dismissLoginPrompt() { setLoginPromptOpen(false); }

void SessionController::logout() {
    if (m_busy) return;
    // 停止保活后再走原下线接口，防止原重连逻辑在退出后再次认证。
    m_auth->stopKeepAlive();
    setLoggedIn(false);
    begin(Logout);
}

void SessionController::begin(Operation operation) {
    if (m_busy || m_auth->state() == 2) return;
    setLastError(QString());
    setOperation(operation);
    setBusy(true);
    if (operation == Logout) m_auth->logout();
    else m_auth->login();
}

void SessionController::finish(Operation operation, bool ok, const QString& message) {
    setBusy(false);
    setOperation(None);

    if (operation == Logout) {
        setLoggedIn(false);
        setLoginPromptOpen(false);
        if (!ok) setLastError(message);
        emit eventRequested(static_cast<int>(ok ? LogLevel::Info : LogLevel::Warning),
                            ok ? QStringLiteral("auth.logoutSuccess")
                               : QStringLiteral("auth.logoutFailed"),
                            ok ? QStringLiteral("auth.logoutSuccessMessage")
                               : QStringLiteral("auth.logoutFailedMessage"),
                            ok ? QString() : reasonForMessage(message), QVariantMap{});
        return;
    }

    if (ok) {
        setLoggedIn(true);
        setLoginPromptOpen(false);
        setLastError(QString());
    } else {
        setLastError(message);
    }
    const bool automatic = operation == AutomaticLogin;
    emit eventRequested(static_cast<int>(ok ? LogLevel::Info : LogLevel::Warning),
                        ok ? QStringLiteral("auth.loginSuccess")
                           : QStringLiteral("auth.loginFailed"),
                        ok ? QStringLiteral("auth.loginSuccessMessage")
                           : QStringLiteral("auth.loginFailedMessage"),
                        ok ? QString() : reasonForMessage(message),
                        QVariantMap{{QStringLiteral("automatic"), automatic}});
}

void SessionController::setLoggedIn(bool value) {
    if (m_loggedIn == value) return;
    m_loggedIn = value;
    emit loggedInChanged();
}

void SessionController::setLoginPromptOpen(bool value) {
    if (m_loginPromptOpen == value) return;
    m_loginPromptOpen = value;
    emit loginPromptOpenChanged();
}

void SessionController::setBusy(bool value) {
    if (m_busy == value) return;
    m_busy = value;
    emit busyChanged();
}

void SessionController::setOperation(Operation operation) {
    if (m_operation == operation) return;
    m_operation = operation;
    emit operationChanged();
}

void SessionController::setLastError(const QString& value) {
    if (m_lastError == value) return;
    m_lastError = value;
    emit lastErrorChanged();
}

QString SessionController::reasonForMessage(const QString& message) {
    if (message == QLatin1String("encoding.nonAscii") ||
        message == QLatin1String("encoding.unmappable") ||
        message == QLatin1String("encoding.invalidForm")) {
        return message;
    }
    if (message.contains(QLatin1String("Missing username or password"))) {
        return QStringLiteral("auth.reasonMissingCredentials");
    }
    if (message.contains(QLatin1String("403"))) {
        return QStringLiteral("auth.reasonServerRejected");
    }
    return QStringLiteral("auth.reasonUnknown");
}

} // namespace gdei

#include "app/SessionController.hpp"
#include "app/AuthController.hpp"
#include "core/Types.hpp"

#include <QLatin1String>

namespace gdei {

SessionController::SessionController(AuthController* auth, QObject* parent)
    : QObject(parent), m_auth(auth) {
    // 登录态由真变假（会话失效）时自动弹登录层，符合"登录失效切回登录"约束。
    // Auto prompt when the session becomes invalid, without touching explicit logout.
    connect(this, &SessionController::loggedInChanged, this, [this] {
        if (!m_loggedIn && m_operation != Logout && !m_loginPromptOpen) {
            promptLogin();
        }
    });
    connect(m_auth, &AuthController::stateChanged, this, [this] {
        if (m_auth->state() == static_cast<int>(AuthState::Authenticating)) emit busyChanged();
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
bool SessionController::busy() const {
    return m_busy || m_auth->state() == static_cast<int>(AuthState::Authenticating);
}
bool SessionController::loginPromptOpen() const { return m_loginPromptOpen; }
QString SessionController::route() const { return m_loggedIn ? QStringLiteral("main")
                                                              : QStringLiteral("login"); }
QString SessionController::lastError() const { return m_lastError; }
int SessionController::operation() const { return static_cast<int>(m_operation); }

void SessionController::login() { begin(ManualLogin); }
void SessionController::loginAutomatically() { begin(AutomaticLogin); }

void SessionController::promptLogin() { setLoginPromptOpen(true); }

void SessionController::dismissLoginPrompt() {
    m_pendingAction.clear();
    setLoginPromptOpen(false);
}

bool SessionController::requireLogin(const QString& actionKey) {
    if (m_loggedIn) return true;
    // 未登录：记录待续做动作并弹层；登录成功后由 finish() 发 pendingActionReady 续做。
    m_pendingAction = actionKey;
    setLastError(QString());
    promptLogin();
    return false;
}

void SessionController::logout() {
    if (m_busy) return;
    // 停止保活后再走原下线接口，防止原重连逻辑在退出后再次认证。
    m_auth->stopKeepAlive();
    setOperation(Logout);   // 先标记，避免 setLoggedIn(false) 触发"登录失效自动弹层"
    setLoggedIn(false);
    begin(Logout);
}

void SessionController::begin(Operation operation) {
    if (m_busy || m_auth->state() == static_cast<int>(AuthState::Authenticating)) return;
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
        setLoginPromptOpen(false);
        m_pendingAction.clear();
        if (!ok) {
            // 服务端下线失败：回滚登录态，避免本地显示已登出但会话仍有效。
            // Roll back on server-side logout failure to avoid a false "signed out" state.
            setLoggedIn(true);
            setLastError(message);
        } else {
            setLoggedIn(false);
            // 仅在真正登出成功后清理门户/设备侧会话与页面栈，防止状态残留。
            emit sessionCleared();
        }
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
        // 通过门禁触发的登录：成功后通知 QML 续做被延迟的动作（编辑/导出/设置等）。
        if (!m_pendingAction.isEmpty()) {
            const QString action = m_pendingAction;
            m_pendingAction.clear();
            emit pendingActionReady(action);
        }
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
    // 已是 i18n key（含点号、无空格）的消息直接透传，避免二次包裹。
    // Messages already carrying an i18n key pass through unchanged.
    if (message.contains(QLatin1Char('.')) && !message.contains(QLatin1Char(' '))) {
        return message;
    }
    // 精确短语匹配，避免 contains("403") 把任意含 403 的文本误判为服务端拒绝。
    // Exact phrase matching avoids misclassifying any text that merely contains "403".
    if (message.contains(QLatin1String("Missing username or password"), Qt::CaseInsensitive) ||
        message.contains(QStringLiteral("未填写账号或密码")) ||
        message.contains(QStringLiteral("未配置账号或密码"))) {
        return QStringLiteral("auth.reasonMissingCredentials");
    }
    if (message.contains(QLatin1String("403")) &&
        (message.contains(QLatin1String("Blocked"), Qt::CaseInsensitive) ||
         message.contains(QLatin1String("rate-limited"), Qt::CaseInsensitive) ||
         message.contains(QStringLiteral("限流")))) {
        return QStringLiteral("auth.reasonServerRejected");
    }
    if (message.contains(QStringLiteral("空")) &&
        message.contains(QStringLiteral("门户"))) {
        return QStringLiteral("auth.reasonUnknown");
    }
    return QStringLiteral("auth.reasonUnknown");
}

} // namespace gdei

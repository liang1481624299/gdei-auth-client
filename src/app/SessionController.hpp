// src/app/SessionController.hpp
// 应用层登录会话；不复用网络在线状态作为页面登录态。
#ifndef GDEI_SESSION_CONTROLLER_HPP
#define GDEI_SESSION_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace gdei {

class AuthController;
class ConfigStore;
class I18n;

class SessionController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    // 登录弹层是否打开：主框架常驻，受保护操作按需弹出，不做全局登录门控。
    // Whether the login sheet is open: the shell stays mounted; login is prompted on demand.
    Q_PROPERTY(bool loginPromptOpen READ loginPromptOpen NOTIFY loginPromptOpenChanged)

public:
    enum Operation {
        None = 0,
        ManualLogin,
        AutomaticLogin,
        Logout,
        Reconnect
    };
    Q_ENUM(Operation)

    explicit SessionController(AuthController* auth, ConfigStore* config,
                               I18n* i18n = nullptr, QObject* parent = nullptr);

    bool loggedIn() const;
    bool busy() const;
    QString lastError() const;
    bool loginPromptOpen() const;
    // 供内部桥接（EventBridge）读取当前操作类型，不作为 QML 属性暴露。
    int operation() const;

    Q_INVOKABLE void login();
    Q_INVOKABLE void loginAutomatically();
    Q_INVOKABLE void logout();
    // 纯本地注销：停止保活、退出本机登录态并清空身份凭据；不调用 captive portal
    // 踢线接口，现有在线会话不主动释放。8080/设备本地会话随 sessionCleared 联动清除。
    // Local sign-out: stop keep-alive, leave the local session and clear stored
    // credentials; the captive portal logout endpoint is NOT called, so the online
    // session is released by the server on timeout. Account/device local sessions
    // are cleared through the sessionCleared signal.
    Q_INVOKABLE void signOutLocal();
    // 打开登录弹层（不立即发起认证，由表单提交后再登录）/ Open the login sheet without authenticating yet.
    Q_INVOKABLE void promptLogin();
    // 关闭登录弹层，返回公开浏览 / Dismiss the sheet and return to public browsing.
    Q_INVOKABLE void dismissLoginPrompt();
    // 受保护操作门禁：已登录返回 true 直接执行；未登录弹层并记录动作，登录成功后发信号续做。
    // Guard for protected actions: true means proceed; otherwise prompt login and emit
    // pendingActionReady(actionKey) after a successful sign-in so the caller can resume.
    Q_INVOKABLE bool requireLogin(const QString& actionKey);

signals:
    void loggedInChanged();
    void pendingActionReady(QString actionKey);
    void sessionCleared();
    void busyChanged();
    void lastErrorChanged();
    void operationChanged();
    void loginPromptOpenChanged();
    void eventRequested(int level, QString titleKey, QString messageKey,
                        QString reasonKey, QVariantMap arguments);

private:
    enum class Completion { Login, Logout };

    void begin(Operation operation);
    void finish(Operation operation, bool ok, const QString& message);
    void setLoggedIn(bool value);
    void setLoginPromptOpen(bool value);
    void setBusy(bool value);
    void setOperation(Operation operation);
    void setLastError(const QString& value);
    static QString reasonForMessage(const QString& message);

    AuthController* m_auth;
    ConfigStore* m_cfg;
    I18n* m_i18n;
    bool m_loggedIn = false;
    bool m_busy = false;
    bool m_loginPromptOpen = false;
    QString m_lastError;
    QString m_pendingAction;
    Operation m_operation = None;
};

} // namespace gdei

#endif // GDEI_SESSION_CONTROLLER_HPP

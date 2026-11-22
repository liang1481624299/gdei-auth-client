// src/app/SessionController.hpp
// 应用层登录会话；不复用网络在线状态作为页面登录态。
#ifndef GDEI_SESSION_CONTROLLER_HPP
#define GDEI_SESSION_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace gdei {

class AuthController;

class SessionController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString route READ route NOTIFY loggedInChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int operation READ operation NOTIFY operationChanged)
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

    explicit SessionController(AuthController* auth, QObject* parent = nullptr);

    bool loggedIn() const;
    bool busy() const;
    QString route() const;
    QString lastError() const;
    int operation() const;
    bool loginPromptOpen() const;

    Q_INVOKABLE void login();
    Q_INVOKABLE void loginAutomatically();
    Q_INVOKABLE void logout();
    // 打开登录弹层（不立即发起认证，由表单提交后再登录）/ Open the login sheet without authenticating yet.
    Q_INVOKABLE void promptLogin();
    // 关闭登录弹层，返回公开浏览 / Dismiss the sheet and return to public browsing.
    Q_INVOKABLE void dismissLoginPrompt();

signals:
    void loggedInChanged();
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
    bool m_loggedIn = false;
    bool m_busy = false;
    bool m_loginPromptOpen = false;
    QString m_lastError;
    Operation m_operation = None;
};

} // namespace gdei

#endif // GDEI_SESSION_CONTROLLER_HPP

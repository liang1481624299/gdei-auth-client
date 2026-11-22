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

    Q_INVOKABLE void login();
    Q_INVOKABLE void loginAutomatically();
    Q_INVOKABLE void logout();

signals:
    void loggedInChanged();
    void busyChanged();
    void lastErrorChanged();
    void operationChanged();
    void eventRequested(int level, QString titleKey, QString messageKey,
                        QString reasonKey, QVariantMap arguments);

private:
    enum class Completion { Login, Logout };

    void begin(Operation operation);
    void finish(Operation operation, bool ok, const QString& message);
    void setLoggedIn(bool value);
    void setBusy(bool value);
    void setOperation(Operation operation);
    void setLastError(const QString& value);
    static QString reasonForMessage(const QString& message);

    AuthController* m_auth;
    bool m_loggedIn = false;
    bool m_busy = false;
    QString m_lastError;
    Operation m_operation = None;
};

} // namespace gdei

#endif // GDEI_SESSION_CONTROLLER_HPP

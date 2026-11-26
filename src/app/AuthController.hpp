// src/app/AuthController.hpp
// 认证控制器 / Authentication controller.
// 组合门户客户端与保活模块，向 QML 暴露状态与指令。
// Combines portal client + keep-alive; exposes state and commands to QML.
#ifndef GDEI_AUTH_CONTROLLER_HPP
#define GDEI_AUTH_CONTROLLER_HPP

#include <QObject>
#include <QString>

namespace gdei {

class ConfigStore;
class Logger;
class I18n;
class PortalClient;
class KeepAlive;

class AuthController : public QObject {
    Q_OBJECT
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)
    Q_PROPERTY(bool keepAliveRunning READ keepAliveRunning NOTIFY keepAliveRunningChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    AuthController(ConfigStore* config, Logger* logger, I18n* i18n,
                   QObject* parent = nullptr);

    int state() const;
    bool online() const;
    bool keepAliveRunning() const;
    QString lastMessage() const;

    Q_INVOKABLE void login();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void startKeepAlive();
    Q_INVOKABLE void stopKeepAlive();

    // 内部桥接用只读访问器（不作为 QML 属性暴露）。
    // Read-only accessors for the internal event bridge; not exposed to QML.
    PortalClient* portal() const { return m_portal; }
    KeepAlive* keepAlive() const { return m_keep; }

signals:
    void stateChanged();
    void onlineChanged();
    void keepAliveRunningChanged();
    void lastMessageChanged();
    void loginFinished(bool ok, QString message);

private:
    ConfigStore* m_cfg;
    Logger* m_log;
    I18n* m_i18n;
    PortalClient* m_portal;
    KeepAlive* m_keep;
    QString m_lastMessage;

    void setMessage(const QString& message);
};

} // namespace gdei

#endif // GDEI_AUTH_CONTROLLER_HPP

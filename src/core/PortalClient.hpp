// src/core/PortalClient.hpp
// 门户认证客户端 / Portal authentication client.
// 基于 QNetworkAccessManager；GET 登录页提取 CSRF，再带 UA/Referer/Cookie 提交表单。
// Uses QNAM: GET login page to extract CSRF, then POST form with full headers.
#ifndef GDEI_PORTAL_CLIENT_HPP
#define GDEI_PORTAL_CLIENT_HPP

#include "core/Types.hpp"

#include <QObject>
#include <QByteArray>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

namespace gdei {

class ConfigStore;
class Logger;
class SessionCookieJar;

class PortalClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)

public:
    PortalClient(ConfigStore* config, Logger* logger, QObject* parent = nullptr);

    int state() const;
    bool online() const;

    Q_INVOKABLE void login();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void refreshStatus();

signals:
    void stateChanged(int state);
    void onlineChanged(bool online);
    void loginFinished(bool ok, QString message);
    void portalError(QString message);

private slots:
    void onPageReply();
    void onLoginReply();
    void onLogoutReply();
    void onProbeReply();

private:
    struct ReplyContext {
        QString action; // page / login / logout / probe
    };

    ConfigStore* m_cfg;
    Logger* m_log;
    QNetworkAccessManager* m_nam;
    SessionCookieJar* m_jar;

    AuthState m_state = AuthState::Unknown;
    bool m_online = false;
    QString m_csrf;
    bool m_busy = false;

    QUrl urlFor(const QString& path) const;
    void applyPeerVerify(QNetworkRequest& req) const;
    QNetworkReply* sendRequest(const QString& method, const QString& path,
                               const QString& body = QString());
    QNetworkReply* postConfiguredForm(const QString& path, const QString& body);
    QString extractCsrf(const QString& html) const;
    QString buildForm(const QString& user, const QString& pass) const;
    QString detectCharset(const QByteArray& contentType, const QByteArray& body) const;
    QString decodeBody(const QByteArray& raw, const QByteArray& contentType) const;
    void parsePayload(const QString& text, bool& ok, QString& message) const;
    void setState(AuthState s);
    void setOnline(bool v);
    void log(LogLevel level, const QString& message);
};

} // namespace gdei

#endif // GDEI_PORTAL_CLIENT_HPP

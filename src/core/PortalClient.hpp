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
    // 每次探测（无论在线态是否翻转）的结果，供保活状态机累计连续失败次数。
    // Per-probe result (regardless of state flip) for the keep-alive failure counter.
    void probeFinished(bool online, QString reason);

private slots:
    void onEntryReply();
    void onPageReply();
    void onLoginReply();
    void onLogoutReply();
    void onProbeReply();
    // 空响应旁路旁证：登录 POST 返回 2xx 空体且无会话 Cookie 时，补发一次外网探测再定在线态。
    // External reachability probe after an empty login body, when no session cookie is held.
    void onEmptyProbeReply();

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
    bool m_probeBusy = false;
    // 最近一次外网探测结果与时间戳，供空响应分支做新鲜旁证。
    // Last external probe result and timestamp, used as a fresh hint for empty-body replies.
    bool m_lastProbeOnline = false;
    qint64 m_lastProbeMs = 0;

    QUrl urlFor(const QString& path) const;
    void applyPeerVerify(QNetworkRequest& req, const QString& urlHost) const;
    QString hostHeader() const;
    QNetworkReply* sendRequest(const QString& method, const QString& path,
                               const QString& body = QString());
    QNetworkReply* postConfiguredForm(const QString& path, const QString& body);
    QString extractCsrf(const QString& html) const;
    QString buildForm(const QString& user, const QString& pass) const;
    QString detectCharset(const QByteArray& contentType, const QByteArray& body) const;
    QString decodeBody(const QByteArray& raw, const QByteArray& contentType) const;
    void parsePayload(const QString& text, bool& ok, QString& message,
                      bool& alreadyOnline, bool& offline) const;
    // 外网探测通用判定：2xx 且响应体非空、含 NCSI 特征串即视为外网可达。
    // Shared external-reachability check: 2xx, non-empty body carrying the NCSI marker.
    bool externalProbeOnline(int status, const QByteArray& raw, const QString& body) const;
    void setState(AuthState s);
    void setOnline(bool v);
    void log(LogLevel level, const QString& message);
};

} // namespace gdei

#endif // GDEI_PORTAL_CLIENT_HPP

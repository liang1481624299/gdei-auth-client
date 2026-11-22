// src/core/AccountPortalClient.hpp
// PC 自助门户账户客户端 / PC self-service portal account client.
// 独立于 captive portal 会话；对接 PrimeFaces 单视图 main.jsf，只读拉取账户资料。
// Separate session from the captive portal; talks to the PrimeFaces single-view
// main.jsf, read-only profile fetch.
//
// 现状 / Status：
//  - 登录页抓取、会话保持、口令编码、登录 POST 链路已落地；
//  - main.jsf 资料区 DOM 解析与 PrimeFaces 部分提交（切换模块）待主页 HTML
//    样本复核后补全，见 .cpp 中 TODO 标记。
#ifndef GDEI_ACCOUNT_PORTAL_CLIENT_HPP
#define GDEI_ACCOUNT_PORTAL_CLIENT_HPP

#include "core/Types.hpp"

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariantMap>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;

namespace gdei {

class ConfigStore;
class Logger;
class SessionCookieJar;

class AccountPortalClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY profileChanged)
    Q_PROPERTY(QVariantList services READ services NOTIFY servicesChanged)
    Q_PROPERTY(QString captchaImage READ captchaImage NOTIFY captchaImageChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    AccountPortalClient(ConfigStore* config, Logger* logger, QObject* parent = nullptr);

    bool authenticated() const;
    bool busy() const;
    QVariantMap profile() const;
    QVariantList services() const;
    QString captchaImage() const;
    QString lastMessage() const;

    // 拉取 PC 登录页并刷新图形验证码 / Open PC login page and refresh captcha.
    Q_INVOKABLE void fetchCaptcha();
    // 输入验证码提交 PC 门户登录 / Submit PC portal login with captcha.
    Q_INVOKABLE void loginWithCode(const QString& code);
    // 重新拉取账户资料与已申请服务 / Refresh profile and granted services.
    Q_INVOKABLE void refreshProfile();
    // 注销门户会话并清空本地状态 / Log out and clear local session.
    Q_INVOKABLE void logout();
    // 清空本地会话与资料（不调用门户注销）/ Clear local session and profile.
    Q_INVOKABLE void clearSession();

signals:
    void authenticatedChanged();
    void busyChanged();
    void profileChanged();
    void servicesChanged();
    void captchaImageChanged();
    void lastMessageChanged();
    void loginFinished(bool ok, QString message);

private slots:
    void onCaptchaPageReply();
    void onCaptchaImageReply();
    void onLoginReply();
    void onHomeReply();
    void onLogoutReply();

private:
    enum class Stage {
        Idle,
        CaptchaPage,
        CaptchaImage,
        Login,
        Home,
        Logout
    };

    ConfigStore* m_cfg;
    Logger* m_log;
    QNetworkAccessManager* m_nam;
    SessionCookieJar* m_jar;

    bool m_authenticated = false;
    bool m_busy = false;
    QVariantMap m_profile;
    QVariantList m_services;
    QString m_captchaImage;
    QString m_lastMessage;

    QString m_jsessionid;
    QString m_viewState;
    QString m_captchaPath;
    QString m_homeHtml;

    QUrl selfUrl(const QString& path) const;
    void setBusy(bool v);
    void setAuthenticated(bool v);
    void setMessage(const QString& message);
    void log(LogLevel level, const QString& message);
    QString decodeBody(const QByteArray& raw, const QByteArray& contentType) const;

    // TODO(profile-parse): 待 main.jsf 主页 HTML 样本复核后实现资料/服务解析。
    // Parse profile & services once the main.jsf DOM sample is captured.
    void parseProfile(const QString& html);
};

} // namespace gdei

#endif // GDEI_ACCOUNT_PORTAL_CLIENT_HPP

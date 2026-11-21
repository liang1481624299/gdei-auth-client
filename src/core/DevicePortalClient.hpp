// src/core/DevicePortalClient.hpp
// 自助服务门户设备客户端 / Self-service portal device client.
// 独立于 captive portal 会话；只读拉取在线 / 历史 / 绑定终端列表。
// Separate session from the captive portal; read-only fetch of online /
// history / bound terminal lists.
//
// 写操作（删除绑定、启用/禁用无感知、置为丢失、增加终端）走 ICEfaces
// 部分提交协议，本期不实现，界面侧给出不可用提示。
// Write actions ride the ICEfaces partial-submit protocol and are out of scope.
#ifndef GDEI_DEVICE_PORTAL_CLIENT_HPP
#define GDEI_DEVICE_PORTAL_CLIENT_HPP

#include "core/Types.hpp"

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;

namespace gdei {

class ConfigStore;
class Logger;
class SessionCookieJar;

class DevicePortalClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QString captchaImage READ captchaImage NOTIFY captchaImageChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    DevicePortalClient(ConfigStore* config, Logger* logger, QObject* parent = nullptr);

    bool authenticated() const;
    bool busy() const;
    QVariantList devices() const;
    QString captchaImage() const;
    QString lastMessage() const;

    // 拉取登录页并刷新图形验证码 / Open login page and refresh captcha.
    Q_INVOKABLE void fetchCaptcha();
    // 输入验证码提交自助门户登录 / Submit self-service login with captcha.
    Q_INVOKABLE void loginWithCode(const QString& code);
    // 重新拉取在线 / 历史 / 绑定终端列表 / Refresh device lists.
    Q_INVOKABLE void refreshDevices();
    // 清空本地会话与列表（不调用门户注销）/ Clear local session and lists.
    Q_INVOKABLE void clearSession();

signals:
    void authenticatedChanged();
    void busyChanged();
    void devicesChanged();
    void captchaImageChanged();
    void lastMessageChanged();
    void loginFinished(bool ok, QString message);

private slots:
    void onCaptchaPageReply();
    void onCaptchaImageReply();
    void onLoginReply();
    void onHomeReply();
    void onMacListReply();

private:
    enum class Stage {
        Idle,
        CaptchaPage,
        CaptchaImage,
        Login,
        Home,
        MacList
    };

    ConfigStore* m_cfg;
    Logger* m_log;
    QNetworkAccessManager* m_nam;
    SessionCookieJar* m_jar;

    bool m_authenticated = false;
    bool m_busy = false;
    QVariantList m_devices;
    QString m_captchaImage;
    QString m_lastMessage;

    QString m_jsessionid;
    QString m_viewState;
    QString m_iceWindow;
    QString m_iceView;
    QString m_homeHtml;

    QUrl selfUrl(const QString& path) const;
    void setBusy(bool v);
    void setAuthenticated(bool v);
    void setMessage(const QString& message);
    void log(LogLevel level, const QString& message);
    QString decodeBody(const QByteArray& raw, const QByteArray& contentType) const;

    static QString fieldValue(const QString& item, const QString& label);
    static QString spanByClass(const QString& item, const QString& cls);
    static QString blockById(const QString& html, const QString& id);
    static QStringList splitItems(const QString& block);
    QVariantList parseOnline(const QString& html, const QString& category) const;
    QVariantList parseBound(const QString& html) const;
};

} // namespace gdei

#endif // GDEI_DEVICE_PORTAL_CLIENT_HPP

// src/app/AccountController.hpp
// 账户控制器 / Account controller: exposes the PC self-service account client to QML.
// 独立于 captive 与会话控制器；包装 AccountPortalClient，仅向 UI 暴露状态与指令。
#ifndef GDEI_ACCOUNT_CONTROLLER_HPP
#define GDEI_ACCOUNT_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace gdei {

class ConfigStore;
class Logger;
class I18n;
class AccountPortalClient;

class AccountController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY profileChanged)
    Q_PROPERTY(QVariantList services READ services NOTIFY servicesChanged)
    Q_PROPERTY(QString captchaImage READ captchaImage NOTIFY captchaImageChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    AccountController(ConfigStore* config, Logger* logger, I18n* i18n,
                      QObject* parent = nullptr);

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

private:
    AccountPortalClient* m_account;
    I18n* m_i18n;
};

} // namespace gdei

#endif // GDEI_ACCOUNT_CONTROLLER_HPP

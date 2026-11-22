// src/app/DeviceController.hpp
// 设备控制器 / Device controller: exposes self-service device client to QML.
#ifndef GDEI_DEVICE_CONTROLLER_HPP
#define GDEI_DEVICE_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>

namespace gdei {

class ConfigStore;
class Logger;
class I18n;
class DevicePortalClient;

class DeviceController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QString captchaImage READ captchaImage NOTIFY captchaImageChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    DeviceController(ConfigStore* config, Logger* logger, I18n* i18n,
                     QObject* parent = nullptr);

    bool authenticated() const;
    bool busy() const;
    QVariantList devices() const;
    QString captchaImage() const;
    QString lastMessage() const;

    Q_INVOKABLE void fetchCaptcha();
    Q_INVOKABLE void loginWithCode(const QString& code);
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void clearSession();

signals:
    void authenticatedChanged();
    void busyChanged();
    void devicesChanged();
    void captchaImageChanged();
    void lastMessageChanged();
    void loginFinished(bool ok, QString message);

private:
    DevicePortalClient* m_dev;
    I18n* m_i18n;
};

} // namespace gdei

#endif // GDEI_DEVICE_CONTROLLER_HPP

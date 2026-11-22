// src/app/LocalDeviceController.hpp
// 本地连接档案指令与活动配置同步。
#ifndef GDEI_LOCAL_DEVICE_CONTROLLER_HPP
#define GDEI_LOCAL_DEVICE_CONTROLLER_HPP

#include <QObject>
#include <QVariantMap>

namespace gdei {

class AuthController;
class ConfigStore;
class I18n;
class LocalDeviceStore;
struct LocalProfile;

class LocalDeviceController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* model READ model CONSTANT)
    Q_PROPERTY(QString selectedId READ selectedId NOTIFY selectedIdChanged)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectedNameChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    LocalDeviceController(ConfigStore* config, AuthController* auth, I18n* i18n,
                          QObject* parent = nullptr);

    QObject* model() const;
    QString selectedId() const;
    QString selectedName() const;
    int count() const;

    Q_INVOKABLE bool selectProfile(const QString& id);
    Q_INVOKABLE QString addProfile(const QString& name, const QString& username,
                                   const QString& password, const QString& host,
                                   const QString& ip, const QString& nasip,
                                   bool rememberPassword);
    Q_INVOKABLE bool updateProfile(const QString& id, const QString& name,
                                   const QString& username, const QString& password,
                                   const QString& host, const QString& ip,
                                   const QString& nasip, bool rememberPassword);
    Q_INVOKABLE bool removeProfile(const QString& id);
    Q_INVOKABLE QVariantMap profile(const QString& id) const;

signals:
    void selectedIdChanged();
    void selectedNameChanged();
    void countChanged();
    void eventRequested(int level, QString titleKey, QString messageKey,
                        QString reasonKey, QVariantMap arguments);

private:
    LocalProfile makeDefaultProfile(bool fromConfig) const;
    bool addStoredProfile(const LocalProfile& profile, const QString& password, bool announce);
    bool syncSelectedProfile();
    bool switchingBlocked() const;
    void emitSwitchBlocked(const QString& id);
    void emitSaveFailed(const QString& reasonKey, const QVariantMap& arguments);
    void emitMissingProfile(const QString& operationKey, const QString& id);

    ConfigStore* m_config;
    AuthController* m_auth;
    I18n* m_i18n;
    LocalDeviceStore* m_store;
};

} // namespace gdei

#endif // GDEI_LOCAL_DEVICE_CONTROLLER_HPP

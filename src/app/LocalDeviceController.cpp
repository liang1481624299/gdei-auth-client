// src/app/LocalDeviceController.cpp
#include "app/LocalDeviceController.hpp"
#include "app/AuthController.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/I18n.hpp"
#include "core/LocalDeviceStore.hpp"
#include "core/SecretStore.hpp"
#include "core/Types.hpp"

#include <QSettings>
#include <QTimer>
#include <QUuid>

namespace gdei {
namespace {

QString passwordKey(const QString& id) {
    return QStringLiteral("local-profile/") + id + QStringLiteral("/password");
}

QVariantMap argumentsFor(const LocalProfile& profile) {
    return {
        {QStringLiteral("id"), profile.id},
        {QStringLiteral("name"), profile.name}
    };
}

} // namespace

LocalDeviceController::LocalDeviceController(ConfigStore* config, AuthController* auth,
                                             I18n* i18n, QObject* parent)
    : QObject(parent), m_config(config), m_auth(auth), m_i18n(i18n),
      m_store(new LocalDeviceStore(this)) {
    connect(m_store, &LocalDeviceStore::selectedIdChanged,
            this, &LocalDeviceController::selectedIdChanged);
    connect(m_store, &LocalDeviceStore::selectedIdChanged,
            this, &LocalDeviceController::selectedNameChanged);
    connect(m_store, &LocalDeviceStore::dataChanged, this, [this] {
        emit selectedNameChanged();
    });
    connect(m_store, &LocalDeviceStore::rowsInserted, this, [this] { emit countChanged(); });
    connect(m_store, &LocalDeviceStore::rowsRemoved, this, [this] { emit countChanged(); });
    connect(m_store, &LocalDeviceStore::modelReset, this, [this] {
        emit countChanged();
        emit selectedNameChanged();
    });

    // 延迟到事件桥接完成后加载，确保迁移/读取失败不会丢失事件。
    QTimer::singleShot(0, this, [this] {
        if (!m_store->load()) {
            emit eventRequested(static_cast<int>(LogLevel::Error),
                                QStringLiteral("profiles.loadFailed"),
                                QStringLiteral("profiles.loadFailedMessage"),
                                m_store->lastErrorKey(), QVariantMap{});
        }
        if (m_store->count() == 0) {
            addStoredProfile(makeDefaultProfile(true), m_config->password(), false);
        }
    });
}

QObject* LocalDeviceController::model() const { return m_store; }
QString LocalDeviceController::selectedId() const { return m_store->selectedId(); }

QString LocalDeviceController::selectedName() const {
    return m_store->profile(m_store->selectedId()).name;
}

int LocalDeviceController::count() const { return m_store->count(); }

bool LocalDeviceController::selectProfile(const QString& id) {
    if (!m_store->contains(id)) {
        emitMissingProfile(QStringLiteral("profiles.selectFailed"), id);
        return false;
    }
    if (id != m_store->selectedId() && switchingBlocked()) {
        emitSwitchBlocked(id);
        return false;
    }
    if (!m_store->selectProfile(id)) {
        emitSaveFailed(m_store->lastErrorKey(), {{QStringLiteral("id"), id}});
        return false;
    }
    if (!syncSelectedProfile()) return false;
    emit eventRequested(static_cast<int>(LogLevel::Info),
                        QStringLiteral("profiles.selected"),
                        QStringLiteral("profiles.selectedMessage"), QString(),
                        argumentsFor(m_store->profile(id)));
    return true;
}

QString LocalDeviceController::addProfile(const QString& name, const QString& username,
                                          const QString& password, const QString& host,
                                          const QString& ip, const QString& nasip,
                                          bool rememberPassword) {
    LocalProfile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = name;
    profile.username = username;
    profile.host = ConfigStore::normalizeHost(host);
    if (profile.host.isEmpty()) profile.host = constants::DEFAULT_HOST;
    profile.ip = ip;
    profile.nasip = nasip;
    profile.rememberPassword = rememberPassword;
    profile.createdAt = QDateTime::currentDateTimeUtc();
    profile.updatedAt = profile.createdAt;
    return addStoredProfile(profile, password, true) ? profile.id : QString();
}

bool LocalDeviceController::updateProfile(const QString& id, const QString& name,
                                          const QString& username, const QString& password,
                                          const QString& host, const QString& ip,
                                          const QString& nasip, bool rememberPassword) {
    LocalProfile updated = m_store->profile(id);
    if (updated.id.isEmpty()) {
        emitMissingProfile(QStringLiteral("profiles.updateFailed"), id);
        return false;
    }
    if (id == m_store->selectedId() && switchingBlocked()) {
        emitSwitchBlocked(id);
        return false;
    }
    updated.name = name;
    updated.username = username;
    updated.host = ConfigStore::normalizeHost(host);
    if (updated.host.isEmpty()) updated.host = constants::DEFAULT_HOST;
    updated.ip = ip;
    updated.nasip = nasip;
    updated.rememberPassword = rememberPassword;
    updated.updatedAt = QDateTime::currentDateTimeUtc();

    const QString key = passwordKey(id);
    const bool changesSecret = !rememberPassword || !password.isEmpty();
    const QString previousPassword = changesSecret ? SecretStore::instance().read(key) : QString();
    bool secretSaved = true;
    if (!rememberPassword) secretSaved = SecretStore::instance().clear(key);
    else if (!password.isEmpty()) secretSaved = SecretStore::instance().write(key, password);
    if (!secretSaved) {
        emitSaveFailed(QStringLiteral("profiles.secretStoreFailed"), argumentsFor(updated));
        return false;
    }

    if (!m_store->updateProfile(updated)) {
        if (changesSecret) {
            if (previousPassword.isEmpty()) SecretStore::instance().clear(key);
            else SecretStore::instance().write(key, previousPassword);
        }
        emitSaveFailed(m_store->lastErrorKey(), argumentsFor(updated));
        return false;
    }
    if (id == m_store->selectedId() && !syncSelectedProfile()) return false;

    emit eventRequested(static_cast<int>(LogLevel::Info),
                        QStringLiteral("profiles.updated"),
                        QStringLiteral("profiles.updatedMessage"), QString(),
                        argumentsFor(updated));
    return true;
}

bool LocalDeviceController::removeProfile(const QString& id) {
    const LocalProfile removed = m_store->profile(id);
    if (removed.id.isEmpty()) {
        emitMissingProfile(QStringLiteral("profiles.deleteFailed"), id);
        return false;
    }
    const bool wasSelected = id == m_store->selectedId();
    if (wasSelected && switchingBlocked()) {
        emitSwitchBlocked(id);
        return false;
    }
    if (!m_store->removeProfile(id)) {
        emitSaveFailed(m_store->lastErrorKey(), argumentsFor(removed));
        return false;
    }
    if (!SecretStore::instance().clear(passwordKey(id))) {
        emitSaveFailed(QStringLiteral("profiles.secretStoreFailed"), argumentsFor(removed));
        return false;
    }

    if (wasSelected) {
        if (m_store->count() == 0) {
            if (!addStoredProfile(makeDefaultProfile(false), QString(), false)) return false;
        } else if (!syncSelectedProfile()) {
            return false;
        }
    }

    emit eventRequested(static_cast<int>(LogLevel::Info),
                        QStringLiteral("profiles.deleted"),
                        QStringLiteral("profiles.deletedMessage"), QString(),
                        argumentsFor(removed));
    return true;
}

QVariantMap LocalDeviceController::profile(const QString& id) const {
    return m_store->profileMap(id);
}

LocalProfile LocalDeviceController::makeDefaultProfile(bool fromConfig) const {
    LocalProfile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = m_i18n ? m_i18n->text(QStringLiteral("profiles.defaultName"))
                          : QStringLiteral("profiles.defaultName");
    profile.username = fromConfig ? m_config->username() : QString();
    profile.host = fromConfig ? m_config->host() : constants::DEFAULT_HOST;
    profile.ip = fromConfig ? m_config->ip() : constants::DEFAULT_IP;
    profile.nasip = fromConfig ? m_config->nasip() : constants::DEFAULT_NASIP;
    profile.rememberPassword = fromConfig && m_config->rememberPassword();
    profile.createdAt = QDateTime::currentDateTimeUtc();
    profile.updatedAt = profile.createdAt;
    return profile;
}

bool LocalDeviceController::addStoredProfile(const LocalProfile& profile,
                                             const QString& password, bool announce) {
    const QString key = passwordKey(profile.id);
    bool secretSaved = true;
    if (!profile.rememberPassword) secretSaved = SecretStore::instance().clear(key);
    else if (!password.isEmpty()) secretSaved = SecretStore::instance().write(key, password);
    if (!secretSaved) {
        emitSaveFailed(QStringLiteral("profiles.secretStoreFailed"), argumentsFor(profile));
        return false;
    }
    if (!m_store->addProfile(profile)) {
        SecretStore::instance().clear(key);
        emitSaveFailed(m_store->lastErrorKey(), argumentsFor(profile));
        return false;
    }
    if (m_store->selectedId() == profile.id && !syncSelectedProfile()) return false;

    if (announce) {
        emit eventRequested(static_cast<int>(LogLevel::Info),
                            QStringLiteral("profiles.added"),
                            QStringLiteral("profiles.addedMessage"), QString(),
                            argumentsFor(profile));
    }
    return true;
}

bool LocalDeviceController::syncSelectedProfile() {
    const LocalProfile selected = m_store->profile(m_store->selectedId());
    if (selected.id.isEmpty()) return false;
    const QString password = selected.rememberPassword
        ? SecretStore::instance().read(passwordKey(selected.id)) : QString();
    m_config->setUsername(selected.username);
    m_config->setPassword(password);
    m_config->setHost(selected.host);
    m_config->setIp(selected.ip);
    m_config->setNasip(selected.nasip);
    m_config->setRememberPassword(selected.rememberPassword);
    m_config->save();

    QSettings settings(constants::ORG_NAME, constants::APP_NAME);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        emitSaveFailed(QStringLiteral("profiles.configSaveFailed"), argumentsFor(selected));
        return false;
    }
    return true;
}

bool LocalDeviceController::switchingBlocked() const {
    return m_auth && (m_auth->state() == static_cast<int>(AuthState::Authenticating) ||
                      m_auth->online() || m_auth->keepAliveRunning());
}

void LocalDeviceController::emitSwitchBlocked(const QString& id) {
    emit eventRequested(static_cast<int>(LogLevel::Warning),
                        QStringLiteral("profiles.switchBlocked"),
                        QStringLiteral("profiles.switchBlockedMessage"),
                        QStringLiteral("profiles.stopFirst"),
                        {{QStringLiteral("id"), id}});
}

void LocalDeviceController::emitSaveFailed(const QString& reasonKey,
                                           const QVariantMap& arguments) {
    emit eventRequested(static_cast<int>(LogLevel::Error),
                        QStringLiteral("profiles.saveFailed"),
                        QStringLiteral("profiles.saveFailedMessage"),
                        reasonKey, arguments);
}

void LocalDeviceController::emitMissingProfile(const QString& operationKey, const QString& id) {
    emit eventRequested(static_cast<int>(LogLevel::Warning), operationKey,
                        operationKey + QStringLiteral("Message"),
                        QStringLiteral("profiles.profileNotFound"),
                        {{QStringLiteral("id"), id}});
}

} // namespace gdei

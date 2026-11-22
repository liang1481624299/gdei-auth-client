// src/core/LocalDeviceStore.cpp
#include "core/LocalDeviceStore.hpp"
#include "core/Constants.hpp"

#include <QSettings>
#include <QStringList>
#include <algorithm>

namespace gdei {
namespace {

QString utcString(const QDateTime& value) {
    return value.toUTC().toString(Qt::ISODateWithMs);
}

QVariantMap toMap(const LocalProfile& profile) {
    return {
        {QStringLiteral("id"), profile.id},
        {QStringLiteral("name"), profile.name},
        {QStringLiteral("username"), profile.username},
        {QStringLiteral("host"), profile.host},
        {QStringLiteral("ip"), profile.ip},
        {QStringLiteral("nasip"), profile.nasip},
        {QStringLiteral("rememberPassword"), profile.rememberPassword},
        {QStringLiteral("createdAt"), utcString(profile.createdAt)},
        {QStringLiteral("updatedAt"), utcString(profile.updatedAt)}
    };
}

} // namespace

LocalDeviceStore::LocalDeviceStore(QObject* parent) : QAbstractListModel(parent) {}

int LocalDeviceStore::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_profiles.size());
}

QVariant LocalDeviceStore::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_profiles.size()) return {};
    const LocalProfile& profile = m_profiles.at(index.row());
    if (role == Qt::DisplayRole || role == NameRole) return profile.name;
    switch (role) {
        case IdRole: return profile.id;
        case UsernameRole: return profile.username;
        case HostRole: return profile.host;
        case IpRole: return profile.ip;
        case NasIpRole: return profile.nasip;
        case RememberPasswordRole: return profile.rememberPassword;
        case CreatedAtRole: return utcString(profile.createdAt);
        case UpdatedAtRole: return utcString(profile.updatedAt);
        default: return {};
    }
}

QHash<int, QByteArray> LocalDeviceStore::roleNames() const {
    return {
        {IdRole, QByteArrayLiteral("id")},
        {NameRole, QByteArrayLiteral("name")},
        {UsernameRole, QByteArrayLiteral("username")},
        {HostRole, QByteArrayLiteral("host")},
        {IpRole, QByteArrayLiteral("ip")},
        {NasIpRole, QByteArrayLiteral("nasip")},
        {RememberPasswordRole, QByteArrayLiteral("rememberPassword")},
        {CreatedAtRole, QByteArrayLiteral("createdAt")},
        {UpdatedAtRole, QByteArrayLiteral("updatedAt")}
    };
}

bool LocalDeviceStore::load() {
    m_lastErrorKey.clear();
    QList<LocalProfile> loaded;
    QSettings settings(constants::ORG_NAME, constants::APP_NAME);
    settings.beginGroup(QStringLiteral("localProfiles"));
    QString selectedId = settings.value(QStringLiteral("selectedId")).toString();
    settings.beginGroup(QStringLiteral("profiles"));
    const QStringList ids = settings.childGroups();
    for (const QString& id : ids) {
        settings.beginGroup(id);
        LocalProfile profile;
        profile.id = id;
        profile.name = settings.value(QStringLiteral("name")).toString();
        profile.username = settings.value(QStringLiteral("username")).toString();
        profile.host = settings.value(QStringLiteral("host")).toString();
        profile.ip = settings.value(QStringLiteral("ip")).toString();
        profile.nasip = settings.value(QStringLiteral("nasip")).toString();
        profile.rememberPassword = settings.value(QStringLiteral("rememberPassword"), false).toBool();
        profile.createdAt = QDateTime::fromString(
            settings.value(QStringLiteral("createdAt")).toString(), Qt::ISODateWithMs).toUTC();
        profile.updatedAt = QDateTime::fromString(
            settings.value(QStringLiteral("updatedAt")).toString(), Qt::ISODateWithMs).toUTC();
        if (!profile.createdAt.isValid()) profile.createdAt = QDateTime::currentDateTimeUtc();
        if (!profile.updatedAt.isValid()) profile.updatedAt = profile.createdAt;
        loaded.append(profile);
        settings.endGroup();
    }
    settings.endGroup();
    settings.endGroup();

    std::sort(loaded.begin(), loaded.end(), [](const LocalProfile& a, const LocalProfile& b) {
        if (a.createdAt != b.createdAt) return a.createdAt < b.createdAt;
        return a.id < b.id;
    });

    const QString oldSelectedId = m_selectedId;
    auto selectedExists = [&loaded](const QString& id) {
        return std::any_of(loaded.cbegin(), loaded.cend(), [&id](const LocalProfile& profile) {
            return profile.id == id;
        });
    };
    if (!selectedExists(selectedId)) selectedId = loaded.isEmpty() ? QString() : loaded.first().id;

    beginResetModel();
    m_profiles = loaded;
    m_selectedId = selectedId;
    endResetModel();
    if (oldSelectedId != m_selectedId) emit selectedIdChanged();

    if (settings.status() != QSettings::NoError) {
        setError(QStringLiteral("profiles.settingsReadFailed"));
        return false;
    }

    const QString persistedSelectedId = settings.value(QStringLiteral("localProfiles/selectedId")).toString();
    if (persistedSelectedId != m_selectedId) return persist(m_profiles, m_selectedId);
    return true;
}

bool LocalDeviceStore::addProfile(const LocalProfile& profile) {
    m_lastErrorKey.clear();
    if (profile.id.isEmpty() || contains(profile.id)) {
        setError(QStringLiteral("profiles.invalidProfile"));
        return false;
    }
    QList<LocalProfile> updated = m_profiles;
    updated.append(profile);
    const QString selectedId = m_selectedId.isEmpty() ? profile.id : m_selectedId;
    if (!persist(updated, selectedId)) return false;

    const bool selectionChanged = selectedId != m_selectedId;
    const int newRow = static_cast<int>(m_profiles.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_profiles.append(profile);
    m_selectedId = selectedId;
    endInsertRows();
    if (selectionChanged) emit selectedIdChanged();
    return true;
}

bool LocalDeviceStore::updateProfile(const LocalProfile& profile) {
    m_lastErrorKey.clear();
    const int row = indexOf(profile.id);
    if (row < 0) {
        setError(QStringLiteral("profiles.profileNotFound"));
        return false;
    }
    LocalProfile replacement = profile;
    replacement.createdAt = m_profiles.at(row).createdAt;
    if (!replacement.updatedAt.isValid()) replacement.updatedAt = QDateTime::currentDateTimeUtc();
    QList<LocalProfile> updated = m_profiles;
    updated[row] = replacement;
    if (!persist(updated, m_selectedId)) return false;

    m_profiles[row] = replacement;
    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed);
    return true;
}

bool LocalDeviceStore::removeProfile(const QString& id) {
    m_lastErrorKey.clear();
    const int row = indexOf(id);
    if (row < 0) {
        setError(QStringLiteral("profiles.profileNotFound"));
        return false;
    }
    QList<LocalProfile> updated = m_profiles;
    updated.removeAt(row);
    QString selectedId = m_selectedId;
    if (selectedId == id) {
        if (updated.isEmpty()) selectedId.clear();
        else selectedId = updated.at(qMin(row, static_cast<int>(updated.size()) - 1)).id;
    }
    if (!persist(updated, selectedId)) return false;

    const bool selectionChanged = selectedId != m_selectedId;
    beginRemoveRows(QModelIndex(), row, row);
    m_profiles.removeAt(row);
    m_selectedId = selectedId;
    endRemoveRows();
    if (selectionChanged) emit selectedIdChanged();
    return true;
}

bool LocalDeviceStore::selectProfile(const QString& id) {
    m_lastErrorKey.clear();
    if (!contains(id)) {
        setError(QStringLiteral("profiles.profileNotFound"));
        return false;
    }
    if (id == m_selectedId) return true;
    if (!persist(m_profiles, id)) return false;
    m_selectedId = id;
    emit selectedIdChanged();
    return true;
}

int LocalDeviceStore::count() const { return static_cast<int>(m_profiles.size()); }
bool LocalDeviceStore::contains(const QString& id) const { return indexOf(id) >= 0; }

LocalProfile LocalDeviceStore::profile(const QString& id) const {
    const int row = indexOf(id);
    return row < 0 ? LocalProfile{} : m_profiles.at(row);
}

QVariantMap LocalDeviceStore::profileMap(const QString& id) const {
    const int row = indexOf(id);
    return row < 0 ? QVariantMap{} : toMap(m_profiles.at(row));
}

QString LocalDeviceStore::selectedId() const { return m_selectedId; }
QString LocalDeviceStore::lastErrorKey() const { return m_lastErrorKey; }

int LocalDeviceStore::indexOf(const QString& id) const {
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles.at(i).id == id) return i;
    }
    return -1;
}

bool LocalDeviceStore::persist(const QList<LocalProfile>& profiles, const QString& selectedId) {
    QSettings settings(constants::ORG_NAME, constants::APP_NAME);
    settings.beginGroup(QStringLiteral("localProfiles"));
    settings.remove(QStringLiteral("profiles"));
    settings.setValue(QStringLiteral("selectedId"), selectedId);
    settings.beginGroup(QStringLiteral("profiles"));
    for (const LocalProfile& profile : profiles) {
        settings.beginGroup(profile.id);
        settings.setValue(QStringLiteral("id"), profile.id);
        settings.setValue(QStringLiteral("name"), profile.name);
        settings.setValue(QStringLiteral("username"), profile.username);
        settings.setValue(QStringLiteral("host"), profile.host);
        settings.setValue(QStringLiteral("ip"), profile.ip);
        settings.setValue(QStringLiteral("nasip"), profile.nasip);
        settings.setValue(QStringLiteral("rememberPassword"), profile.rememberPassword);
        settings.setValue(QStringLiteral("createdAt"), utcString(profile.createdAt));
        settings.setValue(QStringLiteral("updatedAt"), utcString(profile.updatedAt));
        settings.endGroup();
    }
    settings.endGroup();
    settings.endGroup();
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        setError(QStringLiteral("profiles.settingsWriteFailed"));
        return false;
    }
    m_lastErrorKey.clear();
    return true;
}

void LocalDeviceStore::setError(const QString& key) { m_lastErrorKey = key; }

} // namespace gdei

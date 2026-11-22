// src/core/LocalDeviceStore.hpp
// 本地连接档案模型与独立命名空间持久化。
#ifndef GDEI_LOCAL_DEVICE_STORE_HPP
#define GDEI_LOCAL_DEVICE_STORE_HPP

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QVariantMap>

namespace gdei {

struct LocalProfile {
    QString id;
    QString name;
    QString username;
    QString host;
    QString ip;
    QString nasip;
    bool rememberPassword = false;
    QDateTime createdAt;
    QDateTime updatedAt;
};

class LocalDeviceStore : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        UsernameRole,
        HostRole,
        IpRole,
        NasIpRole,
        RememberPasswordRole,
        CreatedAtRole,
        UpdatedAtRole
    };

    explicit LocalDeviceStore(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool load();
    bool addProfile(const LocalProfile& profile);
    bool updateProfile(const LocalProfile& profile);
    bool removeProfile(const QString& id);
    bool selectProfile(const QString& id);

    int count() const;
    bool contains(const QString& id) const;
    LocalProfile profile(const QString& id) const;
    QVariantMap profileMap(const QString& id) const;
    QString selectedId() const;
    QString lastErrorKey() const;

signals:
    void selectedIdChanged();

private:
    int indexOf(const QString& id) const;
    bool persist(const QList<LocalProfile>& profiles, const QString& selectedId);
    void setError(const QString& key);

    QList<LocalProfile> m_profiles;
    QString m_selectedId;
    QString m_lastErrorKey;
};

} // namespace gdei

#endif // GDEI_LOCAL_DEVICE_STORE_HPP

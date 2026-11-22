// src/app/EventModel.hpp
// 有界事件历史 / Bounded, newest-first event history.
#ifndef GDEI_EVENT_MODEL_HPP
#define GDEI_EVENT_MODEL_HPP

#include "app/AppEvent.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QVariant>

namespace gdei {

class EventModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TimestampRole,
        LevelRole,
        TitleKeyRole,
        MessageKeyRole,
        ReasonKeyRole,
        SourceRole,
        ArgumentsRole,
        UnreadRole
    };

    explicit EventModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    int unreadCount() const;
    int filterLevel() const;

    void append(AppEvent event);
    void clear();
    void markAllRead();
    void setFilterLevel(int level);

signals:
    void countChanged();
    void unreadCountChanged();
    void filterLevelChanged();

private:
    static constexpr int MaxHistory = 200;

    bool matchesFilter(const AppEvent& event) const;
    int historyRowForVisibleRow(int visibleRow) const;
    int visibleRowForHistoryRow(int historyRow) const;
    void removeOldest();

    QList<AppEvent> m_events;
    int m_unreadCount = 0;
    int m_filterLevel = -1;
};

} // namespace gdei

#endif // GDEI_EVENT_MODEL_HPP

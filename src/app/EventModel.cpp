#include "app/EventModel.hpp"
#include "core/Types.hpp"

namespace gdei {

EventModel::EventModel(QObject* parent) : QAbstractListModel(parent) {}

int EventModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    if (m_filterLevel == -1) return static_cast<int>(m_events.size());

    int result = 0;
    for (const AppEvent& event : m_events) {
        if (matchesFilter(event)) ++result;
    }
    return result;
}

QVariant EventModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.column() != 0) return {};

    const int historyRow = historyRowForVisibleRow(index.row());
    if (historyRow < 0) return {};

    const AppEvent& event = m_events.at(historyRow);
    switch (role) {
        case IdRole:         return QVariant::fromValue(event.id);
        case TimestampRole:  return event.timestampUtc;
        case LevelRole:      return event.level;
        case TitleKeyRole:   return event.titleKey;
        case MessageKeyRole: return event.messageKey;
        case ReasonKeyRole:  return event.reasonKey;
        case SourceRole:     return event.source;
        case ArgumentsRole:  return event.arguments;
        case UnreadRole:     return event.unread;
        default:             return {};
    }
}

QHash<int, QByteArray> EventModel::roleNames() const {
    return {
        {IdRole, "id"},
        {TimestampRole, "timestamp"},
        {LevelRole, "level"},
        {TitleKeyRole, "titleKey"},
        {MessageKeyRole, "messageKey"},
        {ReasonKeyRole, "reasonKey"},
        {SourceRole, "source"},
        {ArgumentsRole, "arguments"},
        {UnreadRole, "unread"}
    };
}

int EventModel::count() const { return static_cast<int>(m_events.size()); }
int EventModel::unreadCount() const { return m_unreadCount; }
int EventModel::filterLevel() const { return m_filterLevel; }

void EventModel::append(AppEvent event) {
    const int oldCount = count();
    const int oldUnreadCount = m_unreadCount;
    const bool alertLevel = event.level == static_cast<int>(LogLevel::Warning) ||
                            event.level == static_cast<int>(LogLevel::Error);
    event.unread = event.unread && alertLevel;

    if (count() >= MaxHistory) removeOldest();

    const bool visible = matchesFilter(event);
    if (visible) beginInsertRows(QModelIndex(), 0, 0);
    m_events.prepend(event);
    if (event.unread) ++m_unreadCount;
    if (visible) endInsertRows();

    if (count() != oldCount) emit countChanged();
    if (m_unreadCount != oldUnreadCount) emit unreadCountChanged();
}

void EventModel::clear() {
    if (m_events.isEmpty()) return;

    const bool hadUnread = m_unreadCount != 0;
    beginResetModel();
    m_events.clear();
    m_unreadCount = 0;
    endResetModel();

    emit countChanged();
    if (hadUnread) emit unreadCountChanged();
}

void EventModel::markAllRead() {
    if (m_unreadCount == 0) return;

    const int size = static_cast<int>(m_events.size());
    for (int historyRow = 0; historyRow < size; ++historyRow) {
        AppEvent& event = m_events[historyRow];
        if (!event.unread) continue;

        event.unread = false;
        const int visibleRow = visibleRowForHistoryRow(historyRow);
        if (visibleRow >= 0) {
            const QModelIndex changed = createIndex(visibleRow, 0);
            emit dataChanged(changed, changed, {UnreadRole});
        }
    }

    m_unreadCount = 0;
    emit unreadCountChanged();
}

void EventModel::setFilterLevel(int level) {
    if (m_filterLevel == level) return;

    beginResetModel();
    m_filterLevel = level;
    endResetModel();
    emit filterLevelChanged();
}

bool EventModel::matchesFilter(const AppEvent& event) const {
    return m_filterLevel == -1 || event.level == m_filterLevel;
}

int EventModel::historyRowForVisibleRow(int visibleRow) const {
    if (visibleRow < 0) return -1;

    const int size = static_cast<int>(m_events.size());
    for (int historyRow = 0; historyRow < size; ++historyRow) {
        if (!matchesFilter(m_events.at(historyRow))) continue;
        if (visibleRow-- == 0) return historyRow;
    }
    return -1;
}

int EventModel::visibleRowForHistoryRow(int historyRow) const {
    if (historyRow < 0 || historyRow >= static_cast<int>(m_events.size()) ||
        !matchesFilter(m_events.at(historyRow))) {
        return -1;
    }

    int visibleRow = 0;
    for (int row = 0; row < historyRow; ++row) {
        if (matchesFilter(m_events.at(row))) ++visibleRow;
    }
    return visibleRow;
}

void EventModel::removeOldest() {
    const int historyRow = static_cast<int>(m_events.size()) - 1;
    const int visibleRow = visibleRowForHistoryRow(historyRow);
    if (visibleRow >= 0) beginRemoveRows(QModelIndex(), visibleRow, visibleRow);

    if (m_events.last().unread) --m_unreadCount;
    m_events.removeLast();

    if (visibleRow >= 0) endRemoveRows();
}

} // namespace gdei

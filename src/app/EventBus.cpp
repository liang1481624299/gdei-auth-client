#include "app/EventBus.hpp"
#include "app/EventModel.hpp"
#include "core/Types.hpp"

#include <QDateTime>
#include <QMutexLocker>

#include <utility>

namespace gdei {
namespace {

QVariantMap eventMap(const AppEvent& event) {
    return {
        {"id", QVariant::fromValue(event.id)},
        {"timestamp", event.timestampUtc},
        {"level", event.level},
        {"titleKey", event.titleKey},
        {"messageKey", event.messageKey},
        {"reasonKey", event.reasonKey},
        {"source", event.source},
        {"arguments", event.arguments},
        {"unread", event.unread}
    };
}

bool isAlertLevel(int level) {
    return level == static_cast<int>(LogLevel::Warning) ||
           level == static_cast<int>(LogLevel::Error);
}

} // namespace

EventBus::EventBus(QObject* parent)
    : QObject(parent), m_model(new EventModel(this)) {
    connect(m_model, &EventModel::unreadCountChanged,
            this, &EventBus::unreadCountChanged);
    connect(m_model, &EventModel::filterLevelChanged,
            this, &EventBus::filterLevelChanged);
    connect(this, &EventBus::pendingEventsAvailable,
            this, &EventBus::deliverPending, Qt::QueuedConnection);
}

QAbstractItemModel* EventBus::model() const { return m_model; }
int EventBus::unreadCount() const { return m_model->unreadCount(); }
bool EventBus::infoToasts() const { return m_infoToasts; }

void EventBus::setInfoToasts(bool enabled) {
    if (m_infoToasts == enabled) return;
    m_infoToasts = enabled;
    emit infoToastsChanged();
}

int EventBus::filterLevel() const { return m_model->filterLevel(); }
void EventBus::setFilterLevel(int level) { m_model->setFilterLevel(level); }

void EventBus::publish(int level, QString source, QString titleKey,
                       QString messageKey, QString reasonKey,
                       QVariantMap arguments) {
    AppEvent event;
    event.timestampUtc = QDateTime::currentDateTimeUtc();
    event.level = level;
    event.titleKey = std::move(titleKey);
    event.messageKey = std::move(messageKey);
    event.reasonKey = std::move(reasonKey);
    event.source = std::move(source);
    event.arguments = std::move(arguments);

    bool notify = false;
    {
        QMutexLocker<QMutex> locker(&m_mutex);
        event.id = ++m_nextId;
        m_pendingEvents.enqueue(std::move(event));
        if (!m_deliveryScheduled) {
            m_deliveryScheduled = true;
            notify = true;
        }
    }

    if (notify) emit pendingEventsAvailable();
}

void EventBus::clear() {
    {
        QMutexLocker<QMutex> locker(&m_mutex);
        m_pendingEvents.clear();
    }
    m_model->clear();
}

void EventBus::markAllRead() { m_model->markAllRead(); }

void EventBus::setEventPageVisible(bool visible) {
    if (m_eventPageVisible == visible) return;
    m_eventPageVisible = visible;
    if (visible) markAllRead();
}

void EventBus::deliverPending() {
    for (;;) {
        AppEvent event;
        {
            QMutexLocker<QMutex> locker(&m_mutex);
            if (m_pendingEvents.isEmpty()) {
                m_deliveryScheduled = false;
                return;
            }
            event = m_pendingEvents.dequeue();
        }

        event.unread = !m_eventPageVisible && isAlertLevel(event.level);
        const bool toast = isAlertLevel(event.level) ||
                           (m_infoToasts && event.level == static_cast<int>(LogLevel::Info));
        const QVariantMap notification = toast ? eventMap(event) : QVariantMap();
        m_model->append(event);
        if (toast) emit toastRequested(notification);
    }
}

} // namespace gdei

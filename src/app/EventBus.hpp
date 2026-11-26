// src/app/EventBus.hpp
// 跨线程事件入口 / Thread-safe event ingress with UI-thread delivery.
#ifndef GDEI_EVENT_BUS_HPP
#define GDEI_EVENT_BUS_HPP

#include "app/AppEvent.hpp"

#include <QAbstractItemModel>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QVariantMap>

namespace gdei {

class EventModel;

class EventBus : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(bool infoToasts READ infoToasts WRITE setInfoToasts NOTIFY infoToastsChanged)
    Q_PROPERTY(int filterLevel READ filterLevel WRITE setFilterLevel NOTIFY filterLevelChanged)
public:
    explicit EventBus(QObject* parent = nullptr);

    QAbstractItemModel* model() const;
    int unreadCount() const;
    bool infoToasts() const;
    void setInfoToasts(bool enabled);
    int filterLevel() const;
    void setFilterLevel(int level);

    // 仅允许 C++ 业务层发布事件；QML 不得直接调用（避免伪造网络/保活事件）。
    // C++-only ingress: QML must not fabricate business events.
    void publish(int level, QString source, QString titleKey,
                 QString messageKey, QString reasonKey = QString(),
                 QVariantMap arguments = QVariantMap());
    Q_INVOKABLE void clear();
    Q_INVOKABLE void markAllRead();
    Q_INVOKABLE void setEventPageVisible(bool visible);

signals:
    void unreadCountChanged();
    void infoToastsChanged();
    void filterLevelChanged();
    void toastRequested(QVariantMap event);

private slots:
    void deliverPending();

private:
    Q_SIGNAL void pendingEventsAvailable();

    EventModel* m_model;
    bool m_infoToasts = false;
    bool m_eventPageVisible = false;

    QMutex m_mutex;
    qint64 m_nextId = 0;
    QQueue<AppEvent> m_pendingEvents;
    bool m_deliveryScheduled = false;
};

} // namespace gdei

#endif // GDEI_EVENT_BUS_HPP

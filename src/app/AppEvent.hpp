// src/app/AppEvent.hpp
// 全局事件值类型 / Global event value type.
#ifndef GDEI_APP_EVENT_HPP
#define GDEI_APP_EVENT_HPP

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace gdei {

struct AppEvent {
    qint64 id = 0;
    QDateTime timestampUtc;
    int level = 0;
    QString titleKey;
    QString messageKey;
    QString reasonKey;
    QString source;
    // 仅传递非敏感 i18n 参数 / Non-sensitive i18n arguments only.
    QVariantMap arguments;
    bool unread = false;
};

} // namespace gdei

Q_DECLARE_METATYPE(gdei::AppEvent)

#endif // GDEI_APP_EVENT_HPP

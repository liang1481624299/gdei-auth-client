// src/core/SessionCookieJar.cpp
#include "core/SessionCookieJar.hpp"

namespace gdei {

SessionCookieJar::SessionCookieJar(QObject* parent)
    : QNetworkCookieJar(parent) {}

QList<QNetworkCookie> SessionCookieJar::list() const {
    return allCookies();
}

void SessionCookieJar::clear() {
    setCookies(QList<QNetworkCookie>());
}

} // namespace gdei

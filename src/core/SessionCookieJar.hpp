// src/core/SessionCookieJar.hpp
// 会话 Cookie 管理 / Session cookie jar.
// 跟随重定向与 Nginx 会话校验、IP 绑定机制。
#ifndef GDEI_SESSION_COOKIE_JAR_HPP
#define GDEI_SESSION_COOKIE_JAR_HPP

#include <QNetworkCookieJar>
#include <QList>
#include <QNetworkCookie>

namespace gdei {

class SessionCookieJar : public QNetworkCookieJar {
    Q_OBJECT
public:
    explicit SessionCookieJar(QObject* parent = nullptr);

    // 当前保存的全部 Cookie（调试/日志用）/ all cookies for debugging.
    QList<QNetworkCookie> list() const;

    // 清空全部 Cookie / drop all stored cookies.
    void clear();
};

} // namespace gdei

#endif // GDEI_SESSION_COOKIE_JAR_HPP

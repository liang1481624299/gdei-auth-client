// src/core/PortalClient.cpp
#include "core/PortalClient.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/SessionCookieJar.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QRegularExpression>
#include <QUrl>

namespace gdei {

PortalClient::PortalClient(ConfigStore* config, Logger* logger, QObject* parent)
    : QObject(parent), m_cfg(config), m_log(logger) {
    m_nam = new QNetworkAccessManager(this);
    m_jar = new SessionCookieJar(this);
    m_nam->setCookieJar(m_jar);

    // 默认严格校验证书；仅当用户在设置中显式开启“跳过证书校验”时才放行，
    // 并写入警告日志留痕。未开启时不忽略，握手错误经 reply->error() 上报。
    // Strict certificate verification by default; ignore SSL errors only when
    // the user explicitly enables the insecure toggle, and log a warning.
    connect(m_nam, &QNetworkAccessManager::sslErrors, this,
            [this](QNetworkReply* reply, const QList<QSslError>& errors) {
                Q_UNUSED(errors);
                if (!reply) return;
                if (m_cfg->allowInsecureSsl()) {
                    reply->ignoreSslErrors();
                    if (m_log) {
                        m_log->warning(tr("SSL verification skipped by config / 按配置跳过证书校验"));
                    }
                }
            });
}

int PortalClient::state() const { return static_cast<int>(m_state); }
bool PortalClient::online() const { return m_online; }

void PortalClient::setState(AuthState s) {
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(static_cast<int>(s));
}

void PortalClient::setOnline(bool v) {
    if (m_online == v) return;
    m_online = v;
    emit onlineChanged(v);
}

void PortalClient::log(LogLevel level, const QString& message) {
    if (m_log) m_log->log(level, message);
}

QUrl PortalClient::urlFor(const QString& path) const {
    const QString host = m_cfg->ip().isEmpty() ? m_cfg->host() : m_cfg->ip();
    QUrl url;
    url.setScheme(constants::DEFAULT_SCHEME);
    url.setHost(host);
    url.setPort(constants::DEFAULT_PORT);
    url.setPath(path);
    return url;
}

QNetworkReply* PortalClient::sendRequest(const QString& method,
                                         const QString& path,
                                         const QString& body) {
    QNetworkRequest req(urlFor(path));
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setRawHeader("Host", m_cfg->host().toUtf8());
    req.setRawHeader("Referer",
        QString("%1://%2%3").arg(constants::DEFAULT_SCHEME, m_cfg->host(),
            constants::LOGIN_PAGE_PATH + m_cfg->nasip()).toUtf8());
    req.setTransferTimeout(constants::CONNECT_TIMEOUT_MS);

    if (!body.isEmpty()) {
        req.setHeader(QNetworkRequest::ContentTypeHeader,
            "application/x-www-form-urlencoded; charset=UTF-8");
        req.setRawHeader("X-Requested-With", "XMLHttpRequest");
    }

    QNetworkReply* reply = nullptr;
    if (method == "POST") {
        reply = m_nam->post(req, body.toUtf8());
    } else {
        reply = m_nam->get(req);
    }
    return reply;
}

QString PortalClient::extractCsrf(const QString& html) const {
    // 兼容常见隐藏域命名 / common hidden-input names.
    QRegularExpression re(
        "name=[\"'](?:csrf|_csrf|_token|token|authenticity_token)[\"']"
        "[^>]*value=[\"']([^\"']*)[\"']",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(html);
    if (m.hasMatch()) return m.captured(1);

    QRegularExpression re2(
        "value=[\"']([^\"']*)[\"'][^>]*"
        "name=[\"'](?:csrf|_csrf|_token|token|authenticity_token)[\"']",
        QRegularExpression::CaseInsensitiveOption);
    m = re2.match(html);
    return m.hasMatch() ? m.captured(1) : QString();
}

QString PortalClient::buildForm(const QString& user, const QString& pass) const {
    const QString pwdB64 = QString::fromLatin1(pass.toUtf8().toBase64());

    QList<QPair<QString, QString>> fields;
    fields << qMakePair(QString::fromLatin1(constants::FIELD_FREE_OTP), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERNAME), user);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_PASSWORD), pwdB64);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERURL), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERIP), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_BASIP), m_cfg->nasip());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_LANGUAGE), constants::DEFAULT_LANGUAGE);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERS_MAC), constants::DEFAULT_NULL);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_WLAN_NASID), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_WLAN_SSID), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_ENTRANCE), constants::DEFAULT_NULL);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_VERIFY_CODE), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_DYNAMIC_PWD), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_CUSTOM_PAGE), constants::DEFAULT_CUSTOM_PAGE);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_DYN_PWD_TYPE), constants::DEFAULT_DYN_PWD_TYPE);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_PWD_MODE), constants::DEFAULT_PWD_MODE);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_PROXY_IP), m_cfg->ip());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_PROXY_PORT), constants::DEFAULT_PROXY_PORT);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_PWD_ENCRYPT), constants::DEFAULT_PWD_ENCRYPT);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_ASSIGN_IP), constants::DEFAULT_ASSIGN_IP);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_APP_ROOT_URL),
        QString("%1://%2/portal/").arg(constants::DEFAULT_SCHEME, m_cfg->host()));
    fields << qMakePair(QString::fromLatin1(constants::FIELD_MANUAL_URL), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_MANUAL_KEY), QString());

    if (!m_csrf.isEmpty()) {
        fields << qMakePair("csrf_token", m_csrf);
    }

    QStringList parts;
    for (const auto& f : fields) {
        parts << QUrl::toPercentEncoding(f.first) + "=" +
                 QUrl::toPercentEncoding(f.second);
    }
    return parts.join("&");
}

void PortalClient::parsePayload(const QString& text, bool& ok, QString& message) const {
    ok = false;
    message = text.trimmed();
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return;

    QByteArray raw = QByteArray::fromBase64(trimmed.toLatin1());
    QString decoded = QUrl::fromPercentEncoding(raw);

    QJsonDocument doc = QJsonDocument::fromJson(decoded.toUtf8());
    if (!doc.isObject()) {
        message = decoded;
        return;
    }
    QJsonObject obj = doc.object();
    ok = obj.value("errorNumber").toString() == "1";

    const QString ed = obj.value("e_d").toString();
    if (!ed.isEmpty() && obj.contains(ed)) {
        message = obj.value(ed).toString();
    } else if (obj.contains("errorMessage")) {
        message = obj.value("errorMessage").toString();
    } else {
        message = decoded;
    }
}

void PortalClient::login() {
    const QString user = m_cfg->username();
    const QString pass = m_cfg->password();
    if (user.isEmpty() || pass.isEmpty()) {
        emit loginFinished(false, tr("Missing username or password"));
        return;
    }
    if (m_busy) return;
    m_busy = true;
    setState(AuthState::Authenticating);
    m_csrf.clear();

    log(LogLevel::Info, tr("Opening login page / 正在打开登录页"));
    QNetworkReply* reply = sendRequest("GET",
        constants::LOGIN_PAGE_PATH + m_cfg->nasip());
    reply->setProperty("action", "page");
    connect(reply, &QNetworkReply::finished, this, &PortalClient::onPageReply);
}

void PortalClient::logout() {
    if (m_busy) return;
    m_busy = true;
    const QString body = buildForm(m_cfg->username(), m_cfg->password());
    QNetworkReply* reply = sendRequest("POST", constants::LOGOUT_ACTION_PATH, body);
    reply->setProperty("action", "logout");
    connect(reply, &QNetworkReply::finished, this, &PortalClient::onLogoutReply);
}

void PortalClient::refreshStatus() {
    // NCSI 是外网地址，不走门户直连 IP，单独用普通 GET。
    // External NCSI probe: plain GET, not through the portal direct IP.
    QNetworkReply* reply = m_nam->get(QNetworkRequest(QUrl(constants::NCSI_URL)));
    reply->setProperty("action", "probe");
    connect(reply, &QNetworkReply::finished, this, &PortalClient::onProbeReply);
}

void PortalClient::onPageReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QString html = QString::fromUtf8(reply->readAll());
    m_csrf = extractCsrf(html);
    if (!m_csrf.isEmpty()) log(LogLevel::Debug, tr("CSRF token extracted"));

    const QString body = buildForm(m_cfg->username(), m_cfg->password());
    QNetworkReply* next = sendRequest("POST", constants::LOGIN_ACTION_PATH, body);
    next->setProperty("action", "login");
    connect(next, &QNetworkReply::finished, this, &PortalClient::onLoginReply);
}

void PortalClient::onLoginReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    m_busy = false;

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString body = QString::fromUtf8(reply->readAll());

    if (reply->error() != QNetworkReply::NoError) {
        setState(AuthState::Error);
        QString msg = tr("Request error / 请求异常: ") + reply->errorString();
        if (status == 403) msg = tr("Blocked or rate-limited (403) / 被限流或拒绝访问 (403)");
        log(LogLevel::Error, msg);
        emit loginFinished(false, msg);
        return;
    }

    bool ok = false;
    QString message;
    parsePayload(body, ok, message);
    if (ok) {
        setState(AuthState::Online);
        setOnline(true);
        log(LogLevel::Info, tr("Authenticated, online / 认证成功，已上线"));
    } else {
        setState(AuthState::Offline);
        log(LogLevel::Warning, tr("Login failed / 认证失败: ") + message);
    }
    emit loginFinished(ok, message);
}

void PortalClient::onLogoutReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    m_busy = false;

    bool ok = false;
    QString message;
    parsePayload(QString::fromUtf8(reply->readAll()), ok, message);
    setState(AuthState::Offline);
    setOnline(false);
    log(LogLevel::Info, ok ? tr("Logged out / 已下线") : tr("Logout result / 注销结果: ") + message);
    emit loginFinished(ok, message);
}

void PortalClient::onProbeReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setOnline(false);
        setState(AuthState::Offline);
        return;
    }
    const QString body = QString::fromUtf8(reply->readAll());
    const bool online = body.contains("Microsoft", Qt::CaseInsensitive);
    setOnline(online);
    setState(online ? AuthState::Online : AuthState::Offline);
}

} // namespace gdei

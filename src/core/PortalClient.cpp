// src/core/PortalClient.cpp
#include "core/PortalClient.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/FormEncoding.hpp"
#include "core/Logger.hpp"
#include "core/SessionCookieJar.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QRegularExpression>
#include <QStringDecoder>
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
    // 域名为主、备用直连 IP 兜底。门户证书签给域名（通配符 *.gdei.edu.cn），
    // 若用 IP 建连，Qt 会按 IP 校验主机名而证书不含 IP SAN，导致 HostNameMismatch。
    // Domain first, fallback to the direct IP only when host is unset. The portal
    // certificate is issued for the domain; connecting by IP makes Qt verify the
    // certificate against the IP, which has no IP SAN and fails hostname matching.
    const QString host = m_cfg->host().isEmpty() ? m_cfg->ip() : m_cfg->host();
    QUrl url;
    url.setScheme(constants::DEFAULT_SCHEME);
    url.setHost(host);
    url.setPort(constants::DEFAULT_PORT);
    url.setPath(path);
    return url;
}

void PortalClient::applyPeerVerify(QNetworkRequest& req, const QString& urlHost) const {
    // 仅当用直连 IP 建连时才需要把证书主机名校验名对齐到域名；URL 已是域名时
    // Qt 会自然按域名校验，重复 setPeerVerifyName 属于冗余。
    // Only override the peer name when connecting by IP; when the URL host is already
    // the domain, Qt verifies it naturally and an override is redundant.
    if (urlHost != constants::DEFAULT_IP && urlHost != m_cfg->ip()) return;
    const QString verifyName = m_cfg->host().isEmpty() ? constants::DEFAULT_HOST : m_cfg->host();
    if (!verifyName.isEmpty()) req.setPeerVerifyName(verifyName);
}

QString PortalClient::hostHeader() const {
    // IP 兜底时 Host 头不能为空；优先用配置域名，其次用兜底常量域名。
    // In IP fallback the Host header must still carry a hostname.
    if (!m_cfg->host().isEmpty()) return m_cfg->host();
    return constants::DEFAULT_HOST;
}

QNetworkReply* PortalClient::sendRequest(const QString& method,
                                         const QString& path,
                                         const QString& body) {
    const QUrl url = urlFor(path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setRawHeader("Host", hostHeader().toUtf8());
    req.setRawHeader("Referer",
        QString("%1://%2%3").arg(constants::DEFAULT_SCHEME, hostHeader(),
            constants::LOGIN_PAGE_REFERRER).toUtf8());
    req.setTransferTimeout(m_cfg->connectTimeoutMs());
    applyPeerVerify(req, url.host());

    if (!body.isEmpty()) {
        req.setHeader(QNetworkRequest::ContentTypeHeader,
            "application/x-www-form-urlencoded; charset=UTF-8");
        req.setRawHeader("X-Requested-With", "XMLHttpRequest");
        // 同源 jQuery ajax 浏览器会自动带 Origin；补齐以贴近真实请求头。
        // Same-origin jQuery ajax sends Origin automatically; mirror it.
        req.setRawHeader("Origin",
            QString("%1://%2").arg(constants::DEFAULT_SCHEME, hostHeader()).toUtf8());
    }

    QNetworkReply* reply = nullptr;
    if (method == "POST") {
        reply = m_nam->post(req, body.toUtf8());
    } else {
        reply = m_nam->get(req);
    }
    return reply;
}

QNetworkReply* PortalClient::postConfiguredForm(const QString& path, const QString& body) {
    const FormEncoding::Result encoded = FormEncoding::encode(body, m_cfg->requestEncoding());
    if (encoded.error != FormEncoding::Error::None) {
        m_busy = false;
        setState(AuthState::Error);
        emit loginFinished(false, encoded.errorKey);
        return nullptr;
    }

    if (encoded.charset == "UTF-8") {
        return sendRequest("POST", path, body);
    }

    const QUrl url = urlFor(path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setRawHeader("Host", hostHeader().toUtf8());
    req.setRawHeader("Referer",
        QString("%1://%2%3").arg(constants::DEFAULT_SCHEME, hostHeader(),
            constants::LOGIN_PAGE_REFERRER).toUtf8());
    req.setTransferTimeout(m_cfg->connectTimeoutMs());
    applyPeerVerify(req, url.host());
    req.setHeader(QNetworkRequest::ContentTypeHeader,
        QString("application/x-www-form-urlencoded; charset=%1").arg(encoded.charset));
    req.setRawHeader("X-Requested-With", "XMLHttpRequest");
    req.setRawHeader("Origin",
        QString("%1://%2").arg(constants::DEFAULT_SCHEME, hostHeader()).toUtf8());
    return m_nam->post(req, encoded.body);
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

namespace {
// 归一化编码名；不在允许集合内返回空串交由调用方回退 UTF-8。
// Normalize a charset name; return empty when outside the allowed set.
QString normalizeCharset(const QString& raw) {
    QString s = raw.trimmed().toLower();
    s.remove('"').remove('\'');
    if (s == "utf8" || s == "utf-8") return "utf-8";
    if (s == "gbk" || s == "gb2312" || s == "gb18030" || s == "cp936") return "gbk";
    if (s == "windows-1252" || s == "cp1252") return "windows-1252";
    if (s == "iso-8859-1" || s == "latin1" || s == "latin-1") return "iso-8859-1";
    return {};
}
} // namespace

QString PortalClient::detectCharset(const QByteArray& contentType, const QByteArray& body) const {
    // 优先级：HTTP Content-Type charset → HTML meta charset → 默认 UTF-8。
    // Priority: Content-Type charset → HTML meta charset → default UTF-8.
    QRegularExpression reHdr("charset=([^\\s;]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch mh = reHdr.match(QString::fromLatin1(contentType));
    if (mh.hasMatch()) {
        const QString cs = normalizeCharset(mh.captured(1));
        if (!cs.isEmpty()) return cs;
    }

    QRegularExpression reMeta(
        "<meta[^>]+charset=[\"']?([^\\s\"'>/]+)",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch mm = reMeta.match(QString::fromUtf8(body));
    if (mm.hasMatch()) {
        const QString cs = normalizeCharset(mm.captured(1));
        if (!cs.isEmpty()) return cs;
    }

    return "utf-8";
}

QString PortalClient::decodeBody(const QByteArray& raw, const QByteArray& contentType) const {
    const QString cs = detectCharset(contentType, raw);

    if (cs == "windows-1252" || cs == "iso-8859-1") {
        const char* name = cs == "windows-1252" ? "Windows-1252" : "ISO-8859-1";
        QStringDecoder decoder(name);
        return decoder(raw);
    }
    if (cs == "gbk") {
        // TODO(P1): 接入 Qt5Compat 的 QTextCodec 以支持 GBK/GB2312；当前回退 UTF-8。
        // TODO(P1): link Qt5Compat QTextCodec for GBK/GB2312; fall back to UTF-8 meanwhile.
        if (m_log) {
            m_log->warning(tr("GBK/GB2312 codec pending, falling back to UTF-8 / GBK 编码待接入，暂按 UTF-8 解码"));
        }
    }
    return QString::fromUtf8(raw);
}

QString PortalClient::buildForm(const QString& user, const QString& pass) const {
    const QString pwdB64 = QString::fromLatin1(pass.toUtf8().toBase64());

    QList<QPair<QString, QString>> fields;
    fields << qMakePair(QString::fromLatin1(constants::FIELD_FREE_OTP), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERNAME), user);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_PASSWORD), pwdB64);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERURL), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERIP), QString());
    // basip 隐藏域在登录页 HTML 中 value 即为空；服务端按来源 IP 鉴权，不填 NAS 地址。
    // The basip hidden field renders empty; the server authenticates by source IP.
    fields << qMakePair(QString::fromLatin1(constants::FIELD_BASIP), QString());
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

void PortalClient::parsePayload(const QString& text, bool& ok, QString& message,
                                bool& alreadyOnline, bool& offline) const {
    ok = false;
    alreadyOnline = false;
    offline = false;
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
    const QString errNo = obj.value("errorNumber").toString();
    ok = errNo == "1";
    // errorNumber=7：当前账号已在线，重复登录返回 "User is already online."；
    // 视为已达成在线态，而非普通认证失败。
    // errorNumber=8：已离线，下线 t=lo 成功后回此码；对下线动作即成功。
    // 7 = already online (repeat login); 8 = offline (logout success).
    alreadyOnline = errNo == "7";
    offline = errNo == "8";

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

    log(LogLevel::Info, tr("Opening portal entry / 正在打开门户入口"));
    QNetworkReply* entry = sendRequest("GET",
        constants::ENTRY_PAGE_PATH + m_cfg->nasip());
    if (!entry) { setState(AuthState::Error); emit loginFinished(false, QStringLiteral("auth.reasonNetworkError")); return; }
    entry->setProperty("action", "entry");
    connect(entry, &QNetworkReply::finished, this, &PortalClient::onEntryReply);
}

void PortalClient::onEntryReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    // 入口步仅用于跟随 302、建立会话 Cookie；响应体本身在非浏览器下可能为空，属正常。
    // The entry step only follows the 302 and establishes the session cookie; its body
    // may be empty outside a browser, which is expected.
    reply->readAll();
    reply->deleteLater();

    log(LogLevel::Info, tr("Opening login page / 正在打开登录页"));
    QNetworkReply* next = sendRequest("GET",
        constants::LOGIN_PAGE_PATH + m_cfg->nasip());
    if (!next) { setState(AuthState::Error); emit loginFinished(false, QStringLiteral("auth.reasonNetworkError")); return; }
    next->setProperty("action", "page");
    connect(next, &QNetworkReply::finished, this, &PortalClient::onPageReply);
}

void PortalClient::logout() {
    if (m_busy) return;
    m_busy = true;
    const QString body = buildForm(m_cfg->username(), m_cfg->password());
    QNetworkReply* reply = postConfiguredForm(constants::LOGOUT_ACTION_PATH, body);
    if (!reply) return;
    reply->setProperty("action", "logout");
    connect(reply, &QNetworkReply::finished, this, &PortalClient::onLogoutReply);
}

void PortalClient::refreshStatus() {
    // 并发去重：保活定时器与手动刷新可能同时触发，丢弃在途的重复探测。
    // De-duplicate concurrent probes from the timer and manual refresh.
    if (m_probeBusy) return;
    QNetworkRequest probeReq(QUrl(m_cfg->probeUrl()));
    probeReq.setTransferTimeout(m_cfg->probeTimeoutMs());
    QNetworkReply* reply = m_nam->get(probeReq);
    if (!reply) {
        emit probeFinished(false, QStringLiteral("auth.reasonNetworkError"));
        return;
    }
    m_probeBusy = true;
    reply->setProperty("action", "probe");
    connect(reply, &QNetworkReply::finished, this, &PortalClient::onProbeReply);
}

void PortalClient::onPageReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray rawPage = reply->readAll();
    const QString html = decodeBody(rawPage,
        reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
    m_csrf = extractCsrf(html);
    if (!m_csrf.isEmpty()) log(LogLevel::Debug, tr("CSRF token extracted"));

    const QString body = buildForm(m_cfg->username(), m_cfg->password());
    QNetworkReply* next = postConfiguredForm(constants::LOGIN_ACTION_PATH, body);
    if (!next) return;
    next->setProperty("action", "login");
    connect(next, &QNetworkReply::finished, this, &PortalClient::onLoginReply);
}

void PortalClient::onLoginReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    m_busy = false;

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray raw = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        setState(AuthState::Error);
        setOnline(false);
        QString msg = tr("Request error / 请求异常: ") + reply->errorString();
        if (status == 403) msg = tr("Blocked or rate-limited (403) / 被限流或拒绝访问 (403)");
        log(LogLevel::Error, msg);
        emit loginFinished(false, msg);
        return;
    }

    // 成功判定叠加 HTTP 状态码：非 2xx 一律视为失败，不能仅凭返回 200 放行。
    // Multi-condition success: a non-2xx status is a failure; 200 alone is not enough.
    if (status > 0 && (status < 200 || status >= 300)) {
        setState(AuthState::Offline);
        setOnline(false);
        const QString msg = tr("Unexpected HTTP status / 异常 HTTP 状态: %1").arg(status);
        log(LogLevel::Warning, msg);
        emit loginFinished(false, msg);
        return;
    }

    // 空响应：门户对"已在线重复登录"常返回空体（见 CAPTURE-GUIDE）。结合 2xx
    // 与入口步已建立的会话 Cookie 判定为已在线；否则仍按失败处理，避免误判。
    // Empty body: the portal often returns it for "already online" re-login. Treat
    // 2xx plus an established session cookie as online; otherwise keep it a failure.
    if (raw.trimmed().isEmpty()) {
        const bool establishedSession = !m_jar->list().isEmpty();
        const bool successStatus = status > 0 && status >= 200 && status < 300;
        if (successStatus && establishedSession) {
            setState(AuthState::Online);
            setOnline(true);
            log(LogLevel::Info, tr("Already online (empty response) / 账号已在线（空响应）"));
            emit loginFinished(true, QStringLiteral("auth.reconnectSuccessMessage"));
            return;
        }
        setState(AuthState::Offline);
        setOnline(false);
        const QString msg = tr("Empty response from portal / 门户返回为空");
        log(LogLevel::Warning, msg);
        emit loginFinished(false, msg);
        return;
    }

    bool ok = false;
    bool already = false;
    bool offline = false;
    QString message;
    parsePayload(decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray()),
                 ok, message, already, offline);
    if (ok) {
        setState(AuthState::Online);
        setOnline(true);
        log(LogLevel::Info, tr("Authenticated, online / 认证成功，已上线"));
    } else if (already) {
        // 服务端告知已在线：在线态已达成，仅记录提示，不按失败报错。
        // Server reports already online: the online state holds; log a hint, not a failure.
        setState(AuthState::Online);
        setOnline(true);
        log(LogLevel::Info, tr("Already online / 账号已在线: ") + message);
    } else {
        setState(AuthState::Offline);
        setOnline(false);
        log(LogLevel::Warning, tr("Login failed / 认证失败: ") + message);
    }
    emit loginFinished(ok || already, message);
}

void PortalClient::onLogoutReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    m_busy = false;

    bool ok = false;
    bool already = false;
    bool offline = false;
    QString message;
    const QByteArray rawLogout = reply->readAll();
    parsePayload(decodeBody(rawLogout, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray()),
                 ok, message, already, offline);
    // 下线成功回 errorNumber=8（User is offline）；与 1 一样视为会话已释放。
    // Logout success returns errorNumber=8 (User is offline); treat it like 1 as released.
    const bool loggedOut = ok || offline;
    setState(AuthState::Offline);
    setOnline(false);
    log(LogLevel::Info, loggedOut ? tr("Logged out / 已下线") : tr("Logout result / 注销结果: ") + message);
    emit loginFinished(loggedOut, message);
}

void PortalClient::onProbeReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    m_probeBusy = false;
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setOnline(false);
        setState(AuthState::Offline);
        emit portalError(QStringLiteral("auth.reasonNetworkError"));
        emit probeFinished(false, QStringLiteral("auth.reasonNetworkError"));
        return;
    }

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray raw = reply->readAll();
    // 探测同样要求 2xx 且响应体含 NCSI 特征串；状态码异常或空体均判离线。
    // Probe requires a 2xx status and the NCSI marker; bad status or empty body = offline.
    const bool statusOk = (status == 0) || (status >= 200 && status < 300);
    const QString body = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
    const bool online = statusOk && !raw.trimmed().isEmpty() &&
                        body.contains("Microsoft", Qt::CaseInsensitive);
    setOnline(online);
    setState(online ? AuthState::Online : AuthState::Offline);
    // 每次探测都发结果，保活据此累计连续失败；onlineChanged 仅在翻转时发，不能用于计数。
    // Emit on every probe; onlineChanged only fires on flips and cannot drive the counter.
    emit probeFinished(online, online ? QString() : QStringLiteral("auth.reasonServerRejected"));
}

} // namespace gdei

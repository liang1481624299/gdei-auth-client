// src/core/PortalClient.cpp
#include "core/PortalClient.hpp"
#include "core/BodyDecoder.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/FormEncoding.hpp"
#include "core/Logger.hpp"
#include "core/SessionCookieJar.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QLatin1Char>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QSslConfiguration>
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
                    emit insecureSslSkipped(reply->url().host());
                }
            });
}

int PortalClient::state() const { return static_cast<int>(m_state); }
bool PortalClient::online() const { return m_online; }
bool PortalClient::busy() const { return m_busy; }

void PortalClient::setCaCertificates(const QList<QSslCertificate>& certificates) {
    m_caCertificates = certificates;
}

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
    const QString host = !m_cfg->host().isEmpty() ? m_cfg->host()
                        : !m_cfg->ip().isEmpty()  ? m_cfg->ip()
                        : constants::DEFAULT_HOST;
    QUrl url;
    url.setScheme(constants::DEFAULT_SCHEME);
    url.setHost(host);
    url.setPort(constants::DEFAULT_PORT);
    // setPath 会把查询串里的 '?' 编码成 %3F，导致路径变成 /portal/pws%3Ft=... 而 404；
    // 需先拆分 path 与 query，分别交给 setPath / setQuery。
    // setPath would percent-encode the '?' of a query string into the path
    // (e.g. /portal/pws%3Ft=...) and yield 404; split path/query first.
    const int q = path.indexOf(QLatin1Char('?'));
    if (q >= 0) {
        url.setPath(path.left(q));
        url.setQuery(path.mid(q + 1));
    } else {
        url.setPath(path);
    }
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

QNetworkRequest PortalClient::makeRequest(const QString& path,
                                          const QString& accept) const {
    // 统一的请求基线：UA / Host / Referer / Accept / Accept-Language / 超时 / 证书主机名对齐 / 用户 CA。
    // Shared request baseline so every portal call keeps identical TLS behaviour.
    const QUrl url = urlFor(path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setRawHeader("Host", hostHeader().toUtf8());
    req.setRawHeader("Referer",
        QString("%1://%2%3").arg(constants::DEFAULT_SCHEME, hostHeader(),
            constants::LOGIN_PAGE_REFERRER).toUtf8());
    // 实测门户过滤器要求 Accept 与 Accept-Language 同时存在，缺任一回 200 空体。
    // The portal filter requires both Accept and Accept-Language; either
    // missing yields a 200 empty body.
    req.setRawHeader("Accept",
        accept.isEmpty() ? constants::ACCEPT_HTML.toUtf8() : accept.toUtf8());
    req.setRawHeader("Accept-Language", constants::ACCEPT_LANGUAGE.toUtf8());
    req.setTransferTimeout(m_cfg->connectTimeoutMs());
    applyPeerVerify(req, url.host());

    if (!m_caCertificates.isEmpty() && url.scheme() == QLatin1String("https")) {
        QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
        ssl.addCaCertificates(m_caCertificates);
        req.setSslConfiguration(ssl);
    }
    return req;
}

QNetworkReply* PortalClient::sendRequest(const QString& method,
                                         const QString& path,
                                         const QString& body) {
    const bool isPost = !body.isEmpty();
    QNetworkRequest req = makeRequest(
        path, isPost ? constants::ACCEPT_AJAX : constants::ACCEPT_HTML);

    if (isPost) {
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

    QNetworkRequest req = makeRequest(path, constants::ACCEPT_AJAX);
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
    if (cs == "gbk" || cs == "gb2312" || cs == "gb18030") {
        return BodyDecoder::fromLegacy(raw);
    }
    // 未声明或声明为 UTF-8 时同样走自适应探测：部分门户的 meta charset 与实际字节不一致。
    return BodyDecoder::fromLegacy(raw);
}

bool PortalClient::externalProbeOnline(int status, const QByteArray& raw,
                                       const QString& body) const {
    const bool statusOk = (status == 0) || (status >= 200 && status < 300);
    // generate_204 类探测在线时回 204 No Content（空体），不能仅凭空体判离线；
    // connecttest.txt 类探测仍要求响应体含 Microsoft 特征串。
    // generate_204 probes answer 204 No Content when online; an empty body
    // must not count as offline. Text-body probes still match "Microsoft".
    const bool noContentOnline = (status == 204);
    const bool bodyOnline = !raw.trimmed().isEmpty() &&
        body.contains(QStringLiteral("Microsoft"), Qt::CaseInsensitive);
    return statusOk && (noContentOnline || bodyOnline);
}

QString PortalClient::buildForm(const QString& user, const QString& pass) const {
    const QString pwdB64 = QString::fromLatin1(pass.toUtf8().toBase64());

    QList<QPair<QString, QString>> fields;
    fields << qMakePair(QString::fromLatin1(constants::FIELD_FREE_OTP), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERNAME), user);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_PASSWORD), pwdB64);
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERURL), QString());
    fields << qMakePair(QString::fromLatin1(constants::FIELD_USERIP), QString());
    // basip 隐藏域取值为 NAS（AC）地址，与登录页渲染值一致；服务端仍按来源 IP 鉴权。
    // The basip hidden field carries the NAS (AC) address, matching the rendered
    // login page; the server still authenticates by source IP.
    const QString basip = m_cfg->nasip().isEmpty()
        ? constants::DEFAULT_NASIP : m_cfg->nasip();
    fields << qMakePair(QString::fromLatin1(constants::FIELD_BASIP), basip);
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
    // errorNumber=8：本就离线 / 会话已释放（重复下线场景），作为下线成功的兼容分支保留。
    // 7 = already online (repeat login); 8 = already offline / session released (compat).
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
        emit loginFinished(false, QStringLiteral("auth.reasonMissingCredentials"));
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
    if (m_busy) {
        // 在途认证时拒绝下线，但必须回一个失败结果，否则上层会话卡在 busy 等待态。
        // Reject logout while busy, but report failure so the caller never hangs.
        emit loginFinished(false, QStringLiteral("auth.logoutFailedMessage"));
        return;
    }
    m_busy = true;
    // 下线 t=lo 实测表单 userName/userPwd 均为空：在线会话由服务端按来源 IP / 会话释放，
    // 不提交账号口令。其余隐藏域与上线一致。
    // The logout form carries empty userName/userPwd: the session is released by
    // source IP / online session, no credentials are submitted.
    const QString body = buildForm(QString(), QString());
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
        QString logMsg = tr("Request error / 请求异常: ") + reply->errorString();
        if (status == 403) logMsg = tr("Blocked or rate-limited (403) / 被限流或拒绝访问 (403)");
        log(LogLevel::Error, logMsg);
        // UI 错误原因走 key|arg 协议；403 用专用 reason，其余带 errorString 参数。
        // UI reasons use the key|arg protocol; 403 has a dedicated reason.
        const QString reason = status == 403
            ? QStringLiteral("auth.reasonServerRejected")
            : QStringLiteral("auth.reasonRequestError|") + reply->errorString();
        emit loginFinished(false, reason);
        return;
    }

    // 成功判定叠加 HTTP 状态码：非 2xx 一律视为失败，不能仅凭返回 200 放行。
    // Multi-condition success: a non-2xx status is a failure; 200 alone is not enough.
    if (status > 0 && (status < 200 || status >= 300)) {
        setState(AuthState::Offline);
        setOnline(false);
        log(LogLevel::Warning, tr("Unexpected HTTP status / 异常 HTTP 状态: %1").arg(status));
        emit loginFinished(false, QStringLiteral("auth.reasonHttpStatus|%1").arg(status));
        return;
    }

    // 空响应：已在线会话旁路时，门户 POST 常返回 2xx 空体。在线旁证有两类：
    //  1) 会话 Cookie 非空（入口步建立）；
    //  2) 近期外网探测可达（NCSI 含 Microsoft 特征串）。
    // captive 无状态、不下发 Cookie，故不能只认 Cookie；两类旁证都没有时，补发一次
    // 外网旁证探测，由 onEmptyProbeReply 聚合，避免把已在线旁路误判为认证失败。
    // Empty body: an already-online bypass often returns a 2xx empty body. Hints are a
    // session cookie OR a recent successful external probe. The captive is stateless and
    // sets no cookie, so cookie alone is insufficient; if neither hint exists, issue a
    // follow-up external probe (onEmptyProbeReply) instead of failing outright.
    if (raw.trimmed().isEmpty()) {
        const bool establishedSession = !m_jar->list().isEmpty();
        const bool successStatus = status > 0 && status >= 200 && status < 300;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const bool freshExternal = m_lastProbeOnline &&
            (now - m_lastProbeMs) < constants::EMPTY_HINT_FRESH_MS;
        if (successStatus && (establishedSession || freshExternal)) {
            setState(AuthState::Online);
            setOnline(true);
            log(LogLevel::Info, tr("Already online (empty response) / 账号已在线（空响应）"));
            emit loginFinished(true, QStringLiteral("auth.reconnectSuccessMessage"));
            return;
        }
        if (successStatus) {
            // 无现成旁证：补发一次外网探测再定，等待 onEmptyProbeReply 回调。
            // No hint on hand: probe external reachability and decide in the callback.
            QNetworkRequest probeReq(QUrl(m_cfg->probeUrl()));
            probeReq.setTransferTimeout(m_cfg->probeTimeoutMs());
            QNetworkReply* probe = m_nam->get(probeReq);
            probe->setProperty("action", "emptyProbe");
            connect(probe, &QNetworkReply::finished, this, &PortalClient::onEmptyProbeReply);
            return;
        }
        setState(AuthState::Offline);
        setOnline(false);
        log(LogLevel::Warning, tr("Empty response from portal / 门户返回为空"));
        emit loginFinished(false, QStringLiteral("auth.reasonEmptyResponse"));
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
    // 服务端原文可能是英文，不直接暴露给 UI；包装为 i18n key|原文，由 app 层翻译。
    // Server text may be English; wrap as key|raw so the app layer localizes the prefix.
    const QString uiMessage = (ok || already)
        ? QStringLiteral("auth.reconnectSuccessMessage")
        : (message.isEmpty() ? QStringLiteral("auth.reasonUnknown")
                             : (message.contains(QLatin1Char('.')) && !message.contains(QLatin1Char(' '))
                                    ? message
                                    : QStringLiteral("auth.reasonRequestError|") + message));
    emit loginFinished(ok || already, uiMessage);
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
    // 在线状态下手动下线成功实测回 errorNumber=1；8 为本就离线/会话已释放的兼容码，
    // 两者都视为会话已释放。
    // A manual logout from online returns errorNumber=1; 8 is the already-offline
    // compat code. Both count as the session being released.
    const bool loggedOut = ok || offline;
    setState(AuthState::Offline);
    setOnline(false);
    log(LogLevel::Info, loggedOut ? tr("Logged out / 已下线") : tr("Logout result / 注销结果: ") + message);
    const QString uiMessage = loggedOut
        ? QStringLiteral("auth.logoutSuccessMessage")
        : QStringLiteral("auth.logoutFailedMessage");
    emit loginFinished(loggedOut, uiMessage);
}

void PortalClient::onProbeReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    m_probeBusy = false;
    if (!reply) return;
    reply->deleteLater();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray raw = reply->readAll();
    const QString body = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
    const bool online = reply->error() == QNetworkReply::NoError &&
                        externalProbeOnline(status, raw, body);

    m_lastProbeOnline = online;
    m_lastProbeMs = QDateTime::currentMSecsSinceEpoch();

    if (reply->error() != QNetworkReply::NoError) {
        setOnline(false);
        setState(AuthState::Offline);
        emit portalError(QStringLiteral("auth.reasonNetworkError"));
        emit probeFinished(false, QStringLiteral("auth.reasonNetworkError"));
        return;
    }

    setOnline(online);
    setState(online ? AuthState::Online : AuthState::Offline);
    // 每次探测都发结果，保活据此累计连续失败；onlineChanged 仅在翻转时发，不能用于计数。
    // Emit on every probe; onlineChanged only fires on flips and cannot drive the counter.
    emit probeFinished(online, online ? QString() : QStringLiteral("auth.reasonServerRejected"));
}

void PortalClient::onEmptyProbeReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray raw = reply->readAll();
    const QString body = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
    const bool online = reply->error() == QNetworkReply::NoError &&
                        externalProbeOnline(status, raw, body);

    m_lastProbeOnline = online;
    m_lastProbeMs = QDateTime::currentMSecsSinceEpoch();

    if (online) {
        setState(AuthState::Online);
        setOnline(true);
        log(LogLevel::Info,
            tr("Already online (empty body, external reachable) / 账号已在线（空响应，外网可达）"));
        emit loginFinished(true, QStringLiteral("auth.reconnectSuccessMessage"));
    } else {
        setState(AuthState::Offline);
        setOnline(false);
        log(LogLevel::Warning, tr("Empty response from portal / 门户返回为空"));
        emit loginFinished(false, QStringLiteral("auth.reasonEmptyResponse"));
    }
}

} // namespace gdei

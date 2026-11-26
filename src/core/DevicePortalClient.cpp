// src/core/DevicePortalClient.cpp
#include "core/DevicePortalClient.hpp"
#include "core/BodyDecoder.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/FormEncoding.hpp"
#include "core/Logger.hpp"
#include "core/SessionCookieJar.hpp"

#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

namespace gdei {

DevicePortalClient::DevicePortalClient(ConfigStore* config, Logger* logger,
                                       QObject* parent)
    : QObject(parent), m_cfg(config), m_log(logger) {
    m_nam = new QNetworkAccessManager(this);
    m_jar = new SessionCookieJar(this);
    m_nam->setCookieJar(m_jar);
}

bool DevicePortalClient::authenticated() const { return m_authenticated; }
bool DevicePortalClient::busy() const { return m_busy; }
QVariantList DevicePortalClient::devices() const { return m_devices; }
QString DevicePortalClient::captchaImage() const { return m_captchaImage; }
QString DevicePortalClient::lastMessage() const { return m_lastMessage; }

void DevicePortalClient::setBusy(bool v) {
    if (m_busy == v) return;
    m_busy = v;
    emit busyChanged();
}

void DevicePortalClient::setAuthenticated(bool v) {
    if (m_authenticated == v) return;
    m_authenticated = v;
    emit authenticatedChanged();
}

void DevicePortalClient::setMessage(const QString& message) {
    if (m_lastMessage == message) return;
    m_lastMessage = message;
    emit lastMessageChanged();
}

void DevicePortalClient::log(LogLevel level, const QString& message) {
    if (m_log) m_log->log(level, message);
}

QUrl DevicePortalClient::selfUrl(const QString& path) const {
    QUrl url;
    url.setScheme(constants::SELF_SERVICE_SCHEME);
    url.setHost(m_cfg->host().isEmpty() ? m_cfg->ip() : m_cfg->host());
    url.setPort(constants::SELF_SERVICE_PORT);
    url.setPath(path);
    return url;
}

QString DevicePortalClient::decodeBody(const QByteArray& raw,
                                        const QByteArray& contentType) const {
    QRegularExpression reHdr("charset=([^\\s;]+)",
                             QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch mh = reHdr.match(QString::fromLatin1(contentType));
    QString cs;
    if (mh.hasMatch()) cs = mh.captured(1).toLower().trimmed();

    if (cs == "windows-1252" || cs == "cp1252") {
        QStringDecoder decoder("Windows-1252");
        return decoder(raw);
    }
    if (cs == "iso-8859-1" || cs == "latin1") {
        QStringDecoder decoder("ISO-8859-1");
        return decoder(raw);
    }
    // 声明缺失或声明为 UTF-8 时按字节自适应：优先严格 UTF-8，失败再按 GB18030。
    return BodyDecoder::fromLegacy(raw);
}

QString DevicePortalClient::fieldValue(const QString& item, const QString& label) {
    const QString esc = QRegularExpression::escape(label);
    QRegularExpression re(QStringLiteral(
        "(?:label|span)[^>]*>\\s*%1\\s*[:：]?\\s*</(?:label|span)>"
        "\\s*</td>\\s*<td[^>]*>\\s*<span[^>]*>([^<]*)</span>").arg(esc));
    QRegularExpressionMatch m = re.match(item);
    return m.hasMatch() ? m.captured(1).trimmed() : QString();
}

QString DevicePortalClient::spanByClass(const QString& item, const QString& cls) {
    const QString esc = QRegularExpression::escape(cls);
    QRegularExpression re(QStringLiteral("%1\"[^>]*>([^<]*)<").arg(esc));
    QRegularExpressionMatch m = re.match(item);
    return m.hasMatch() ? m.captured(1).trimmed() : QString();
}

QString DevicePortalClient::blockById(const QString& html, const QString& id) {
    const QRegularExpression start(QStringLiteral("id=\"%1\"").arg(id));
    QRegularExpressionMatch ms = start.match(html);
    if (!ms.hasMatch()) return {};
    const int from = ms.capturedStart();
    const int end = html.indexOf(QStringLiteral("</ul>"), from);
    if (end < 0) return html.mid(from);
    return html.mid(from, end - from);
}

QStringList DevicePortalClient::splitItems(const QString& block) {
    QStringList out;
    const QStringList parts = block.split(QStringLiteral("<li"), Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        if (p.contains(QStringLiteral("<li")) || p.contains(QStringLiteral("macTop")) ||
            p.contains(QStringLiteral("ms_online"))) {
            out.append(QStringLiteral("<li") + p);
        }
    }
    return out;
}

QVariantList DevicePortalClient::parseOnline(const QString& html,
                                             const QString& category) const {
    QVariantList out;
    const QString ulId = (category == QLatin1String("history")) ? "historyOnline"
                                                                : "currentOnline";
    const QString block = blockById(html, ulId);
    if (block.isEmpty()) return out;

    const QStringList items = splitItems(block);
    for (const QString& item : items) {
        QVariantMap m;
        m["category"] = category;
        m["deviceType"] = spanByClass(item, "ms_online_item_left_text");
        m["mac"] = fieldValue(item, QStringLiteral("MAC地址"));
        m["ip"] = fieldValue(item, QStringLiteral("ip"));
        m["duration"] = fieldValue(item, QStringLiteral("上网时长"));
        m["os"] = QString();
        m["service"] = QString();
        m["authStatus"] = QString();
        m["lostStatus"] = QString();
        m["description"] = QString();

        // 首行：在线为单个登录时间；历史为 开始 / 结束 两个时间。
        const int firstDiv = item.indexOf(QStringLiteral("ms_online_item_first"));
        if (firstDiv >= 0) {
            const int divEnd = item.indexOf(QStringLiteral("</div>"), firstDiv);
            const QString div = divEnd > firstDiv ? item.mid(firstDiv, divEnd - firstDiv)
                                                  : item.mid(firstDiv);
            QRegularExpression spanRe(QStringLiteral("<span[^>]*>([^<]+)</span>"));
            QRegularExpressionMatchIterator it = spanRe.globalMatch(div);
            QStringList times;
            while (it.hasNext()) {
                const QString t = it.next().captured(1).trimmed();
                if (t != QLatin1String("/") && !t.isEmpty()) times.append(t);
            }
            m["startTime"] = times.value(0);
            m["endTime"] = (category == QLatin1String("history")) ? times.value(1) : QString();
        } else {
            m["startTime"] = QString();
            m["endTime"] = QString();
        }

        if (!m["mac"].toString().isEmpty()) out.append(m);
    }
    return out;
}

QVariantList DevicePortalClient::parseBound(const QString& html) const {
    QVariantList out;
    const QString block = blockById(html, "listIterator1");
    if (block.isEmpty()) return out;

    const QStringList items = splitItems(block);
    for (const QString& item : items) {
        QRegularExpression macRe(QStringLiteral("<span id=\"macTop\"[^>]*>([^<]+)</span>"));
        QRegularExpressionMatch mm = macRe.match(item);
        if (!mm.hasMatch()) continue;

        QVariantMap m;
        m["category"] = QStringLiteral("bound");
        m["mac"] = mm.captured(1).trimmed();
        m["deviceType"] = spanByClass(item, "mssv-endpoint-type");
        m["os"] = fieldValue(item, QStringLiteral("操作系统"));
        m["service"] = fieldValue(item, QStringLiteral("服务"));
        m["startTime"] = fieldValue(item, QStringLiteral("绑定时间"));
        m["endTime"] = QString();
        m["ip"] = QString();
        m["duration"] = QString();
        m["authStatus"] = fieldValue(item, QStringLiteral("无感知认证"));
        m["lostStatus"] = fieldValue(item, QStringLiteral("终端状态"));
        m["description"] = QString();
        out.append(m);
    }
    return out;
}

void DevicePortalClient::fetchCaptcha() {
    if (m_busy) return;
    setBusy(true);
    setMessage(QString());
    m_captchaImage.clear();
    emit captchaImageChanged();

    QNetworkRequest req(selfUrl(constants::SELF_LOGIN_PATH));
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* reply = m_nam->get(req);
    reply->setProperty("stage", static_cast<int>(Stage::CaptchaPage));
    connect(reply, &QNetworkReply::finished, this, &DevicePortalClient::onCaptchaPageReply);
}

void DevicePortalClient::onCaptchaPageReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    const QString html = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    if (reply->error() != QNetworkReply::NoError || html.isEmpty()) {
        setBusy(false);
        setMessage(QStringLiteral("portalDevices.unreachable"));
        log(LogLevel::Warning, tr("Self-service captcha page error / 验证码页面请求异常"));
        emit loginFinished(false, m_lastMessage);
        return;
    }

    const QByteArray setCookie = reply->rawHeader("Set-Cookie");
    QRegularExpression jsRe("JSESSIONID=([^;]+)");
    QRegularExpressionMatch jm = jsRe.match(QString::fromLatin1(setCookie));
    m_jsessionid = jm.hasMatch() ? jm.captured(1) : QString();

    QRegularExpression vsRe("name=\"javax.faces.ViewState\"[^>]*value=\"([^\"]*)\"");
    QRegularExpressionMatch vm = vsRe.match(html);
    m_viewState = vm.hasMatch() ? vm.captured(1) : QString();

    QRegularExpression winRe("name=\"ice\\.window\"[^>]*value=\"([^\"]*)\"");
    QRegularExpressionMatch wm = winRe.match(html);
    m_iceWindow = wm.hasMatch() ? wm.captured(1) : QString();

    QRegularExpression vwrRe("name=\"ice\\.view\"[^>]*value=\"([^\"]*)\"");
    QRegularExpressionMatch vrm = vwrRe.match(html);
    m_iceView = vrm.hasMatch() ? vrm.captured(1) : QString();

    QNetworkRequest imgReq(selfUrl(constants::SELF_CAPTCHA_PATH));
    imgReq.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    imgReq.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* img = m_nam->get(imgReq);
    img->setProperty("stage", static_cast<int>(Stage::CaptchaImage));
    connect(img, &QNetworkReply::finished, this, &DevicePortalClient::onCaptchaImageReply);
}

void DevicePortalClient::onCaptchaImageReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray bytes = reply->readAll();
    if (reply->error() != QNetworkReply::NoError || bytes.isEmpty()) {
        setBusy(false);
        setMessage(QStringLiteral("portalDevices.captchaUnavailable"));
        emit loginFinished(false, m_lastMessage);
        return;
    }
    m_captchaImage = QStringLiteral("data:image/jpeg;base64,") + bytes.toBase64();
    emit captchaImageChanged();
    setBusy(false);
}

void DevicePortalClient::loginWithCode(const QString& code) {
    const QString user = m_cfg->username();
    const QString pass = m_cfg->password();
    if (user.isEmpty() || pass.isEmpty()) {
        setMessage(QStringLiteral("auth.reasonMissingCredentials"));
        return;
    }
    if (code.trimmed().isEmpty()) {
        setMessage(QStringLiteral("portalDevices.enterCaptcha"));
        return;
    }
    if (m_busy) return;
    setBusy(true);
    setMessage(QString());

    auto enc = [](const QString& v) { return QString::fromUtf8(QUrl::toPercentEncoding(v)); };

    // 设备自助门户（ICEfaces）直接提交明文口令，且门户仅在 8080 HTTP 提供。
    // 与账户门户的「前端可逆编码」不同，这里不做任何混淆——协议本身没有该要求，
    // 客户端也不应伪造不被服务端识别的编码。传输层风险已在 UI 明确警示。
    // The ICEfaces device portal expects the raw password over plain HTTP; no client-side
    // obfuscation is applied because the server does not expect any.
    QStringList fields;
    fields << "ice.window=" + enc(m_iceWindow);
    fields << "ice.view=" + enc(m_iceView);
    fields << "textInput=" + enc(user);
    fields << "loginCodeInput=" + enc(pass);
    fields << "code=" + enc(code.trimmed());
    fields << "submitBtn=" + enc(QStringLiteral("登录"));
    fields << "j_id_h_SUBMIT=1";
    fields << "javax.faces.ViewState=" + enc(m_viewState);
    const QString body = fields.join("&");
    const FormEncoding::Result encoded = FormEncoding::encode(body, m_cfg->requestEncoding());
    if (encoded.error != FormEncoding::Error::None) {
        setBusy(false);
        setMessage(encoded.errorKey);
        emit loginFinished(false, encoded.errorKey);
        return;
    }

    QUrl url = selfUrl(constants::SELF_LOGIN_PATH);
    if (!m_jsessionid.isEmpty()) {
        url.setPath(constants::SELF_LOGIN_PATH + ";jsessionid=" + m_jsessionid);
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QString("application/x-www-form-urlencoded; charset=%1").arg(encoded.charset));
    req.setRawHeader("Referer", selfUrl(constants::SELF_LOGIN_PATH).toString().toUtf8());
    req.setTransferTimeout(m_cfg->connectTimeoutMs());

    QNetworkReply* reply = m_nam->post(req, encoded.body);
    reply->setProperty("stage", static_cast<int>(Stage::Login));
    connect(reply, &QNetworkReply::finished, this, &DevicePortalClient::onLoginReply);
}

void DevicePortalClient::onLoginReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    const QString html = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    // 验证码错误 / 凭据错误时服务端重渲染登录页，含 loginCodeInput 字段。
    const bool stillOnLogin = html.contains(QStringLiteral("loginCodeInput"));
    const bool ok = reply->error() == QNetworkReply::NoError && !stillOnLogin &&
                    (html.contains(QStringLiteral("currentOnline")) ||
                     html.contains(QStringLiteral("historyOnline")) ||
                     html.contains(QStringLiteral("listIterator1")));

    if (!ok) {
        setAuthenticated(false);
        setBusy(false);
        setMessage(QStringLiteral("portalDevices.loginFailed"));
        log(LogLevel::Warning, tr("Self-service login rejected / 自助门户登录被拒绝"));
        emit loginFinished(false, m_lastMessage);
        fetchCaptcha(); // 刷新验证码供重试 / Refresh captcha for retry.
        return;
    }

    setAuthenticated(true);
    m_homeHtml = html;
    log(LogLevel::Info, tr("Self-service authenticated / 自助门户登录成功"));

    QNetworkRequest homeReq(selfUrl(constants::SELF_HOME_PATH));
    homeReq.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    homeReq.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* home = m_nam->get(homeReq);
    home->setProperty("stage", static_cast<int>(Stage::Home));
    connect(home, &QNetworkReply::finished, this, &DevicePortalClient::onHomeReply);
}

void DevicePortalClient::refreshDevices() {
    if (!m_authenticated) {
        setMessage(QStringLiteral("portalDevices.loginRequired"));
        return;
    }
    if (m_busy) return;
    setBusy(true);
    setMessage(QString());

    QNetworkRequest homeReq(selfUrl(constants::SELF_HOME_PATH));
    homeReq.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    homeReq.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* home = m_nam->get(homeReq);
    home->setProperty("stage", static_cast<int>(Stage::Home));
    connect(home, &QNetworkReply::finished, this, &DevicePortalClient::onHomeReply);
}

void DevicePortalClient::onHomeReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    m_homeHtml = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    if (reply->error() != QNetworkReply::NoError ||
        m_homeHtml.contains(QStringLiteral("loginCodeInput"))) {
        setBusy(false);
        setAuthenticated(false);
        setMessage(QStringLiteral("portalDevices.sessionExpired"));
        emit loginFinished(false, m_lastMessage);
        fetchCaptcha();
        return;
    }

    QNetworkRequest macReq(selfUrl(constants::SELF_MAC_LIST_PATH));
    macReq.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    macReq.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* mac = m_nam->get(macReq);
    mac->setProperty("stage", static_cast<int>(Stage::MacList));
    connect(mac, &QNetworkReply::finished, this, &DevicePortalClient::onMacListReply);
}

void DevicePortalClient::onMacListReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    const QString macHtml = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    QVariantList devices;
    devices += parseOnline(m_homeHtml, QStringLiteral("online"));
    devices += parseOnline(m_homeHtml, QStringLiteral("history"));
    devices += parseBound(macHtml);

    m_devices = devices;
    emit devicesChanged();
    setBusy(false);

    if (devices.isEmpty()) {
        setMessage(QStringLiteral("portalDevices.noDevices"));
    } else {
        setMessage(QStringLiteral("portalDevices.loaded|%1").arg(devices.size()));
    }
    emit loginFinished(true, m_lastMessage);
}

void DevicePortalClient::clearSession() {
    setAuthenticated(false);
    m_devices.clear();
    emit devicesChanged();
    m_captchaImage.clear();
    emit captchaImageChanged();
    m_homeHtml.clear();
    m_jsessionid.clear();
    m_viewState.clear();
    m_iceWindow.clear();
    m_iceView.clear();
    // 基类未暴露清空接口；直接更换全新 Jar 丢弃旧会话 Cookie。
    // Base exposes no clear API; swap in a fresh jar to drop old cookies.
    SessionCookieJar* oldJar = m_jar;
    m_jar = new SessionCookieJar(this);
    m_nam->setCookieJar(m_jar);
    if (oldJar) oldJar->deleteLater();
    setMessage(QString());
}

} // namespace gdei

// src/core/AccountPortalClient.cpp
#include "core/AccountPortalClient.hpp"
#include "core/BodyDecoder.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/FormEncoding.hpp"
#include "core/Logger.hpp"
#include "core/SessionCookieJar.hpp"
#include "core/SelfServiceCodec.hpp"

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

AccountPortalClient::AccountPortalClient(ConfigStore* config, Logger* logger,
                                         QObject* parent)
    : QObject(parent), m_cfg(config), m_log(logger) {
    m_nam = new QNetworkAccessManager(this);
    m_jar = new SessionCookieJar(this);
    m_nam->setCookieJar(m_jar);
}

bool AccountPortalClient::authenticated() const { return m_authenticated; }
bool AccountPortalClient::busy() const { return m_busy; }
QVariantMap AccountPortalClient::profile() const { return m_profile; }
QVariantList AccountPortalClient::services() const { return m_services; }
QString AccountPortalClient::captchaImage() const { return m_captchaImage; }
QString AccountPortalClient::lastMessage() const { return m_lastMessage; }

void AccountPortalClient::setBusy(bool v) {
    if (m_busy == v) return;
    m_busy = v;
    emit busyChanged();
}

void AccountPortalClient::setAuthenticated(bool v) {
    if (m_authenticated == v) return;
    m_authenticated = v;
    emit authenticatedChanged();
}

void AccountPortalClient::setMessage(const QString& message) {
    if (m_lastMessage == message) return;
    m_lastMessage = message;
    emit lastMessageChanged();
}

void AccountPortalClient::log(LogLevel level, const QString& message) {
    if (m_log) m_log->log(level, message);
}

QUrl AccountPortalClient::selfUrl(const QString& path) const {
    QUrl url;
    url.setScheme(constants::SELF_SERVICE_SCHEME);
    url.setHost(m_cfg->host().isEmpty() ? m_cfg->ip() : m_cfg->host());
    url.setPort(constants::SELF_SERVICE_PORT);
    url.setPath(path);
    return url;
}

QString AccountPortalClient::decodeBody(const QByteArray& raw,
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
    if (cs == "gbk" || cs == "gb2312" || cs == "gb18030") {
        return BodyDecoder::fromLegacy(raw);
    }
    return BodyDecoder::fromLegacy(raw);
}

void AccountPortalClient::fetchCaptcha() {
    if (m_busy) return;
    setBusy(true);
    setMessage(QString());
    m_captchaImage.clear();
    emit captchaImageChanged();

    QNetworkRequest req(selfUrl(constants::SELF_PC_LOGIN_PATH));
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* reply = m_nam->get(req);
    reply->setProperty("stage", static_cast<int>(Stage::CaptchaPage));
    connect(reply, &QNetworkReply::finished, this, &AccountPortalClient::onCaptchaPageReply);
}

void AccountPortalClient::onCaptchaPageReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    const QString html = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    if (reply->error() != QNetworkReply::NoError || html.isEmpty()) {
        setBusy(false);
        setMessage(QStringLiteral("portalDevices.unreachable"));
        log(LogLevel::Warning, tr("PC captcha page error / PC 验证码页面请求异常"));
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

    QRegularExpression capRe("codeImages/(checkCode[^']+\\.jpg)");
    QRegularExpressionMatch cm = capRe.match(html);
    m_captchaPath = cm.hasMatch() ? cm.captured(1) : QString();

    if (m_captchaPath.isEmpty()) {
        setBusy(false);
        setMessage(QStringLiteral("portalDevices.captchaUnavailable"));
        emit loginFinished(false, m_lastMessage);
        return;
    }

    QNetworkRequest imgReq(selfUrl(constants::SELF_PC_CAPTCHA_PATH + m_captchaPath));
    imgReq.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    imgReq.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* img = m_nam->get(imgReq);
    img->setProperty("stage", static_cast<int>(Stage::CaptchaImage));
    connect(img, &QNetworkReply::finished, this, &AccountPortalClient::onCaptchaImageReply);
}

void AccountPortalClient::onCaptchaImageReply() {
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

void AccountPortalClient::loginWithCode(const QString& code) {
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

    // 口令按门户前端可逆编码后再进表单 / Encode password with the portal frontend scheme.
    // 注意：该编码是门户前端约定的可逆混淆（AES 常量密钥），不是加密；配合 8080 HTTP
    // 传输仅能通过 TLS 之外的网络隔离降低风险，客户端无法替服务端升级协议。
    // Note: this is a reversible obfuscation required by the portal frontend, not encryption.
    const QString encodedPass = SelfServiceCodec::encodePassword(pass);

    QStringList fields;
    fields << QString::fromLatin1(constants::PC_FIELD_DETERMINE_SHOW) + "=" + enc(constants::PC_VALUE_DETERMINE_SHOW);
    fields << QString::fromLatin1(constants::PC_FIELD_USERNAME) + "=" + enc(user);
    fields << QString::fromLatin1(constants::PC_FIELD_PASSWORD) + "=" + enc(encodedPass);
    fields << QString::fromLatin1(constants::PC_FIELD_CODE) + "=" + enc(code.trimmed());
    fields << QString::fromLatin1(constants::PC_FIELD_USERTYPE) + "=" + enc(constants::PC_USERTYPE_NORMAL);
    // SelectOneMenu 只读焦点占位字段，提交为空 / Readonly focus placeholder, submitted empty.
    fields << QString::fromLatin1(constants::PC_FIELD_USERTYPE_FOCUS) + "=";
    // 触发动作的按钮标识（submit 按钮无 value）/ CommandButton identifier, empty value.
    fields << QString::fromLatin1(constants::PC_FIELD_LOGIN_CMD) + "=";
    fields << QString::fromLatin1(constants::PC_FIELD_GUEST_CTR) + "=0";
    fields << QString::fromLatin1(constants::PC_FIELD_DESC) + "=" +
                  enc(QStringLiteral("访客管理员管理访客"));
    fields << QString::fromLatin1(constants::PC_FIELD_CHECK_CODE) + "=true";
    fields << QString::fromLatin1(constants::PC_FORM_SUBMIT) + "=1";
    fields << QString::fromLatin1(constants::PF_VIEW_STATE) + "=" + enc(m_viewState);
    const QString body = fields.join("&");

    const FormEncoding::Result encoded = FormEncoding::encode(body, m_cfg->requestEncoding());
    if (encoded.error != FormEncoding::Error::None) {
        setBusy(false);
        setMessage(encoded.errorKey);
        emit loginFinished(false, encoded.errorKey);
        return;
    }

    QUrl url = selfUrl(constants::SELF_PC_LOGIN_PATH);
    if (!m_jsessionid.isEmpty()) {
        url.setPath(constants::SELF_PC_LOGIN_PATH + ";jsessionid=" + m_jsessionid);
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QString("application/x-www-form-urlencoded; charset=%1").arg(encoded.charset));
    req.setRawHeader("Referer", selfUrl(constants::SELF_PC_LOGIN_PATH).toString().toUtf8());
    req.setTransferTimeout(m_cfg->connectTimeoutMs());

    QNetworkReply* reply = m_nam->post(req, encoded.body);
    reply->setProperty("stage", static_cast<int>(Stage::Login));
    connect(reply, &QNetworkReply::finished, this, &AccountPortalClient::onLoginReply);
}

void AccountPortalClient::onLoginReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    const QString html = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    // 凭据 / 验证码错误时服务端重渲染登录页，含 mainForm:username 字段。
    const bool stillOnLogin = html.contains(QStringLiteral("mainForm:username"));
    const bool ok = reply->error() == QNetworkReply::NoError && !stillOnLogin;

    if (!ok) {
        setAuthenticated(false);
        setBusy(false);
        setMessage(QStringLiteral("portalDevices.loginFailed"));
        log(LogLevel::Warning, tr("PC self-service login rejected / PC 自助门户登录被拒绝"));
        emit loginFinished(false, m_lastMessage);
        fetchCaptcha(); // 刷新验证码供重试 / Refresh captcha for retry.
        return;
    }

    setAuthenticated(true);
    m_homeHtml = html;
    log(LogLevel::Info, tr("PC self-service authenticated / PC 自助门户登录成功"));

    QNetworkRequest homeReq(selfUrl(constants::SELF_PC_HOME_PATH));
    homeReq.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    homeReq.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* home = m_nam->get(homeReq);
    home->setProperty("stage", static_cast<int>(Stage::Home));
    connect(home, &QNetworkReply::finished, this, &AccountPortalClient::onHomeReply);
}

void AccountPortalClient::refreshProfile() {
    if (!m_authenticated) {
        setMessage(QStringLiteral("portalDevices.loginRequired"));
        return;
    }
    if (m_busy) return;
    setBusy(true);
    setMessage(QString());

    QNetworkRequest homeReq(selfUrl(constants::SELF_PC_HOME_PATH));
    homeReq.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    homeReq.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* home = m_nam->get(homeReq);
    home->setProperty("stage", static_cast<int>(Stage::Home));
    connect(home, &QNetworkReply::finished, this, &AccountPortalClient::onHomeReply);
}

void AccountPortalClient::onHomeReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    m_homeHtml = decodeBody(raw, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    if (reply->error() != QNetworkReply::NoError ||
        m_homeHtml.contains(QStringLiteral("mainForm:username"))) {
        setBusy(false);
        setAuthenticated(false);
        setMessage(QStringLiteral("portalDevices.sessionExpired"));
        emit loginFinished(false, m_lastMessage);
        fetchCaptcha();
        return;
    }

    parseProfile(m_homeHtml);
    setBusy(false);
    emit loginFinished(true, m_lastMessage);
}

void AccountPortalClient::logout() {
    if (m_busy) return;
    setBusy(true);

    QNetworkRequest req(selfUrl(constants::SELF_PC_LOGOUT_PATH));
    req.setHeader(QNetworkRequest::UserAgentHeader, constants::USER_AGENT);
    req.setTransferTimeout(m_cfg->connectTimeoutMs());
    QNetworkReply* reply = m_nam->get(req);
    reply->setProperty("stage", static_cast<int>(Stage::Logout));
    connect(reply, &QNetworkReply::finished, this, &AccountPortalClient::onLogoutReply);
}

void AccountPortalClient::onLogoutReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    clearSession();
    setBusy(false);
    setMessage(QStringLiteral("auth.signOut"));
}

void AccountPortalClient::parseProfile(const QString& html) {
    // TODO(profile-parse): 依据 main.jsf 主页样本实现字段提取。
    // 已从登录页核对的字段标签见 docs/NAC-SELF-SERVICE.md 第 4 节；
    // 服务端为左右两列“标签 - 值”布局，解析时按标签定位相邻值单元格。
    Q_UNUSED(html);
    m_profile.clear();
    m_services.clear();
    emit profileChanged();
    emit servicesChanged();
}

void AccountPortalClient::clearSession() {
    setAuthenticated(false);
    m_profile.clear();
    m_services.clear();
    emit profileChanged();
    emit servicesChanged();
    m_captchaImage.clear();
    emit captchaImageChanged();
    m_homeHtml.clear();
    m_jsessionid.clear();
    m_viewState.clear();
    m_captchaPath.clear();
    // 更换全新 Jar 丢弃旧会话 Cookie / Swap in a fresh jar to drop old cookies.
    SessionCookieJar* oldJar = m_jar;
    m_jar = new SessionCookieJar(this);
    m_nam->setCookieJar(m_jar);
    if (oldJar) oldJar->deleteLater();
    setMessage(QString());
}

} // namespace gdei

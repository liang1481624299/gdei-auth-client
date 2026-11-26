// src/core/ConfigStore.cpp
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/SecretStore.hpp"

#include <QHostAddress>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <QApplication>
#include <QStyleHints>
#include <utility>

namespace gdei {
namespace {

// 仅允许 http/https 且有主机的探测地址，防止 file: 等 scheme 触发本地资源访问。
// Only http/https URLs with a host are accepted, blocking file: and other schemes.
bool isSafeProbeUrl(const QString& value) {
    const QUrl url(value);
    return url.isValid() && !url.host().isEmpty() &&
           (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
}

// 从用户粘贴的整串 URL / 主机名中提取纯主机名（域名或 IPv4），剥离 scheme、路径、
// 查询串与端口；非法时返回空串。客户端各自按 443/8080 单独设置端口，host 字段只
// 保留主机，不能夹带路径，否则 QUrl 解析后主机为空会报 "Host  not found"。
// Extract a bare hostname/IPv4 from a pasted URL; strip scheme, path, query and port.
// Returns empty when invalid. Clients set 443/8080 ports separately, so the host
// field must never carry a path; an embedded path yields an empty host and a
// "Host  not found" error.
QString normalizeHost(const QString& value) {
    QString s = value.trimmed();
    if (s.isEmpty()) return QString();
    const int schemeIdx = s.indexOf(QLatin1String("://"));
    if (schemeIdx >= 0) s = s.mid(schemeIdx + 3);
    const int slash = s.indexOf(QLatin1Char('/'));
    if (slash >= 0) s = s.left(slash);
    const int qmark = s.indexOf(QLatin1Char('?'));
    if (qmark >= 0) s = s.left(qmark);
    const int colon = s.lastIndexOf(QLatin1Char(':'));
    if (colon >= 0) s = s.left(colon);
    s = s.trimmed();
    if (s.isEmpty()) return QString();
    if (!QHostAddress(s).isNull()) return s;   // 合法 IPv4 / valid IPv4
    static const QRegularExpression hostRe(
        QStringLiteral("^([a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?\\.)*"
                        "[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?$"));
    return hostRe.match(s).hasMatch() ? s : QString();
}

} // namespace

ConfigStore::ConfigStore(QObject* parent) : QObject(parent) {
    m_host = constants::DEFAULT_HOST;
    m_ip = constants::DEFAULT_IP;
    m_nasip = constants::DEFAULT_NASIP;
}

QString ConfigStore::username() const { return m_username; }
QString ConfigStore::password() const { return m_password; }
bool ConfigStore::rememberPassword() const { return m_rememberPassword; }
bool ConfigStore::autoLogin() const { return m_autoLogin; }
int ConfigStore::lang() const { return static_cast<int>(m_lang); }
int ConfigStore::themeMode() const { return m_themeMode; }
QString ConfigStore::host() const { return m_host; }
QString ConfigStore::ip() const { return m_ip; }
QString ConfigStore::nasip() const { return m_nasip; }
bool ConfigStore::allowInsecureSsl() const { return m_allowInsecureSsl; }
int ConfigStore::keepaliveMinSec() const { return m_keepaliveMinSec; }
int ConfigStore::keepaliveMaxSec() const { return m_keepaliveMaxSec; }
int ConfigStore::offlineFailThreshold() const { return m_offlineFailThreshold; }
int ConfigStore::connectTimeoutMs() const { return m_connectTimeoutMs; }
int ConfigStore::probeTimeoutMs() const { return m_probeTimeoutMs; }
QString ConfigStore::probeUrl() const { return m_probeUrl; }
QString ConfigStore::healthUrl() const { return m_healthUrl; }
QString ConfigStore::requestEncoding() const { return m_requestEncoding; }

namespace {
int clampInt(int v, int lo, int hi) { return qBound(lo, v, hi); }

QString normalizeRequestEncoding(const QString& value) {
    QString normalized = value.trimmed().toUpper();
    normalized.remove(QLatin1Char('-'));
    if (normalized == "AUTO" || normalized == "AUTODETECT") return QStringLiteral("AUTO");
    if (normalized == "ASCII") return QStringLiteral("ASCII");
    if (normalized == "GBK") return QStringLiteral("GBK");
    if (normalized == "UTF8") return QStringLiteral("UTF-8");
    return QStringLiteral("UTF-8");
}
} // namespace

void ConfigStore::setUsername(const QString& v) {
    if (m_username == v) return;
    m_username = v; emit usernameChanged();
}
void ConfigStore::setPassword(const QString& v) {
    if (m_password == v) return;
    m_password = v; emit passwordChanged();
}
void ConfigStore::setRememberPassword(bool v) {
    if (m_rememberPassword == v) return;
    m_rememberPassword = v; emit rememberPasswordChanged();
}
void ConfigStore::setAutoLogin(bool v) {
    if (m_autoLogin == v) return;
    m_autoLogin = v; emit autoLoginChanged();
}
void ConfigStore::setLang(int v) {
    // 非法语言值不进入状态机；有效范围 ZhCN(0)..Ja(3)。
    // Reject out-of-range language ids instead of casting arbitrary ints into Lang.
    if (v < static_cast<int>(Lang::ZhCN) || v > static_cast<int>(Lang::Ja)) return;
    Lang l = static_cast<Lang>(v);
    if (m_lang == l) return;
    m_lang = l; emit langChanged();
}
void ConfigStore::setThemeMode(int v) {
    // 主题模式收敛到 0 跟随系统 / 1 浅色 / 2 深色，越界回退跟随系统。
    // Clamp theme mode to system/light/dark; out-of-range falls back to system.
    int c = qBound(0, v, 2);
    if (m_themeMode == c) return;
    m_themeMode = c; emit themeModeChanged();
}
QString ConfigStore::normalizeHost(const QString& v) {
    return gdei::normalizeHost(v);
}

void ConfigStore::setHost(const QString& v) {
    const QString c = normalizeHost(v);
    // 主机域名不得为空（PortalClient 用它构造 Host 头与证书校验名）；非法整串忽略，保留现值。
    // Host must be a bare hostname/IP; an invalid pasted URL is rejected and the
    // current value is kept. It feeds the Host header and certificate name.
    if (c.isEmpty()) return;
    if (m_host == c) return;
    m_host = c; emit hostChanged();
}
void ConfigStore::setIp(const QString& v) {
    const QString c = v.trimmed();
    // IP 兜底地址必须为空或合法 IPv4（允许空表示未配置）。
    // Fallback IP must be empty or a valid IPv4 address.
    if (!c.isEmpty() && QHostAddress(c).isNull()) return;
    if (m_ip == c) return;
    m_ip = c; emit ipChanged();
}
void ConfigStore::setNasip(const QString& v) {
    const QString c = v.trimmed();
    if (!c.isEmpty() && QHostAddress(c).isNull()) return;
    if (m_nasip == c) return;
    m_nasip = c; emit nasipChanged();
}
void ConfigStore::setAllowInsecureSsl(bool v) {
    if (m_allowInsecureSsl == v) return;
    m_allowInsecureSsl = v; emit allowInsecureSslChanged();
}
void ConfigStore::setKeepaliveMinSec(int v) {
    const int c = clampInt(v, constants::KEEPALIVE_MIN_BOUND_SEC, constants::KEEPALIVE_MAX_BOUND_SEC);
    if (m_keepaliveMinSec == c) return;
    m_keepaliveMinSec = c; emit keepaliveMinSecChanged();
}
void ConfigStore::setKeepaliveMaxSec(int v) {
    const int c = clampInt(v, constants::KEEPALIVE_MIN_BOUND_SEC, constants::KEEPALIVE_MAX_BOUND_SEC);
    if (m_keepaliveMaxSec == c) return;
    m_keepaliveMaxSec = c; emit keepaliveMaxSecChanged();
}
void ConfigStore::setOfflineFailThreshold(int v) {
    const int c = clampInt(v, constants::FAIL_THRESHOLD_MIN, constants::FAIL_THRESHOLD_MAX);
    if (m_offlineFailThreshold == c) return;
    m_offlineFailThreshold = c; emit offlineFailThresholdChanged();
}
void ConfigStore::setConnectTimeoutMs(int v) {
    const int c = clampInt(v, constants::TIMEOUT_MIN_MS, constants::TIMEOUT_MAX_MS);
    if (m_connectTimeoutMs == c) return;
    m_connectTimeoutMs = c; emit connectTimeoutMsChanged();
}
void ConfigStore::setProbeTimeoutMs(int v) {
    const int c = clampInt(v, constants::TIMEOUT_MIN_MS, constants::TIMEOUT_MAX_MS);
    if (m_probeTimeoutMs == c) return;
    m_probeTimeoutMs = c; emit probeTimeoutMsChanged();
}
void ConfigStore::setProbeUrl(const QString& v) {
    const QString trimmed = v.trimmed();
    if (!trimmed.isEmpty() && !isSafeProbeUrl(trimmed)) return;  // 拒绝非法 scheme
    const QString c = trimmed.isEmpty() ? constants::NCSI_URL : trimmed;
    if (m_probeUrl == c) return;
    m_probeUrl = c; emit probeUrlChanged();
}
void ConfigStore::setHealthUrl(const QString& v) {
    const QString trimmed = v.trimmed();
    if (!trimmed.isEmpty() && !isSafeProbeUrl(trimmed)) return;
    const QString c = trimmed.isEmpty() ? constants::HEALTH_URL : trimmed;
    if (m_healthUrl == c) return;
    m_healthUrl = c; emit healthUrlChanged();
}
void ConfigStore::setRequestEncoding(const QString& v) {
    const QString c = normalizeRequestEncoding(v);
    if (m_requestEncoding == c) return;
    m_requestEncoding = c; emit requestEncodingChanged();
}

void ConfigStore::load() {
    QSettings s(constants::ORG_NAME, constants::APP_NAME);
    m_username   = s.value("username", "").toString();
    m_rememberPassword = s.value("remember_password", false).toBool();
    m_autoLogin   = s.value("auto_login", false).toBool();
    // 配置文件可能被手改或来自旧版本：越界语言值回退简体中文，避免 i18n 越界取词。
    // Clamp lang on load; a hand-edited or legacy value falls back to ZhCN.
    const int langRaw = s.value("lang", static_cast<int>(Lang::ZhCN)).toInt();
    m_lang = (langRaw >= static_cast<int>(Lang::ZhCN) && langRaw <= static_cast<int>(Lang::Ja))
                 ? static_cast<Lang>(langRaw)
                 : Lang::ZhCN;
    m_themeMode   = qBound(0, s.value("theme_mode", 0).toInt(), 2);
    m_host = normalizeHost(s.value("host", constants::DEFAULT_HOST).toString());
    if (m_host.isEmpty()) m_host = constants::DEFAULT_HOST;  // 旧版本/手改坏的 host 回退默认域
    m_ip   = s.value("ip", constants::DEFAULT_IP).toString();
    m_nasip = s.value("nasip", constants::DEFAULT_NASIP).toString();
    m_allowInsecureSsl = s.value("allow_insecure_ssl", false).toBool();
    m_keepaliveMinSec = clampInt(s.value("keepalive_min_sec", constants::KEEPALIVE_MIN_SEC).toInt(),
                                 constants::KEEPALIVE_MIN_BOUND_SEC, constants::KEEPALIVE_MAX_BOUND_SEC);
    m_keepaliveMaxSec = clampInt(s.value("keepalive_max_sec", constants::KEEPALIVE_MAX_SEC).toInt(),
                                 constants::KEEPALIVE_MIN_BOUND_SEC, constants::KEEPALIVE_MAX_BOUND_SEC);
    if (m_keepaliveMaxSec < m_keepaliveMinSec) std::swap(m_keepaliveMinSec, m_keepaliveMaxSec);
    m_offlineFailThreshold = clampInt(s.value("offline_fail_threshold", constants::OFFLINE_FAIL_THRESHOLD).toInt(),
                                      constants::FAIL_THRESHOLD_MIN, constants::FAIL_THRESHOLD_MAX);
    m_connectTimeoutMs = clampInt(s.value("connect_timeout_ms", constants::CONNECT_TIMEOUT_MS).toInt(),
                                  constants::TIMEOUT_MIN_MS, constants::TIMEOUT_MAX_MS);
    m_probeTimeoutMs = clampInt(s.value("probe_timeout_ms", constants::PROBE_TIMEOUT_MS).toInt(),
                                constants::TIMEOUT_MIN_MS, constants::TIMEOUT_MAX_MS);
    // 配置文件可能被篡改，load 必须复用 setter 的 scheme/host 白名单，
    // 防止 file: 等危险 scheme 注入探测地址。
    // Validate persisted URLs with the same whitelist used by setters.
    const QString loadedProbe = s.value("probe_url", constants::NCSI_URL).toString().trimmed();
    m_probeUrl = (!loadedProbe.isEmpty() && isSafeProbeUrl(loadedProbe))
                     ? loadedProbe : constants::NCSI_URL;
    const QString loadedHealth = s.value("health_url", constants::HEALTH_URL).toString().trimmed();
    m_healthUrl = (!loadedHealth.isEmpty() && isSafeProbeUrl(loadedHealth))
                      ? loadedHealth : constants::HEALTH_URL;
    m_requestEncoding = normalizeRequestEncoding(s.value("request_encoding", "AUTO").toString());

    // 仅在记住密码时从平台安全存储还原口令 / restore from secure store only when remembered.
    m_password = m_rememberPassword ? SecretStore::instance().read(QStringLiteral("password"))
                                    : QString();

    emit usernameChanged(); emit passwordChanged();
    emit rememberPasswordChanged(); emit autoLoginChanged();
    emit langChanged(); emit themeModeChanged(); emit hostChanged(); emit ipChanged(); emit nasipChanged();
    emit allowInsecureSslChanged();
    emit keepaliveMinSecChanged(); emit keepaliveMaxSecChanged(); emit offlineFailThresholdChanged();
    emit connectTimeoutMsChanged(); emit probeTimeoutMsChanged();
    emit probeUrlChanged(); emit healthUrlChanged(); emit requestEncodingChanged();
}

bool ConfigStore::save() {
    QSettings s(constants::ORG_NAME, constants::APP_NAME);
    s.setValue("username", m_username);
    s.setValue("remember_password", m_rememberPassword);
    s.setValue("auto_login", m_autoLogin);
    s.setValue("lang", static_cast<int>(m_lang));
    s.setValue("theme_mode", m_themeMode);
    s.setValue("host", m_host);
    s.setValue("ip", m_ip);
    s.setValue("nasip", m_nasip);
    s.setValue("allow_insecure_ssl", m_allowInsecureSsl);
    s.setValue("keepalive_min_sec", m_keepaliveMinSec);
    s.setValue("keepalive_max_sec", m_keepaliveMaxSec);
    s.setValue("offline_fail_threshold", m_offlineFailThreshold);
    s.setValue("connect_timeout_ms", m_connectTimeoutMs);
    s.setValue("probe_timeout_ms", m_probeTimeoutMs);
    s.setValue("probe_url", m_probeUrl);
    s.setValue("health_url", m_healthUrl);
    s.setValue("request_encoding", m_requestEncoding);
    // 清理旧版本写入的混淆口令遗留键 / drop the legacy obfuscated password key.
    s.remove("password_ob");

    s.sync();
    const bool settingsOk = (s.status() == QSettings::NoError);

    // 口令走平台安全存储；未记住或为空时清除 / password via secure store; clear when not remembered.
    const bool secretOk = (m_rememberPassword && !m_password.isEmpty())
        ? SecretStore::instance().write(QStringLiteral("password"), m_password)
        : SecretStore::instance().clear(QStringLiteral("password"));
    return settingsOk && secretOk;
}

bool ConfigStore::resetToDefaults() {
    // 复位到出厂默认值 / Restore factory defaults.
    m_username.clear();
    m_password.clear();
    m_rememberPassword = false;
    m_autoLogin = false;
    m_lang = Lang::ZhCN;
    m_themeMode = 0;
    m_host = constants::DEFAULT_HOST;
    m_ip = constants::DEFAULT_IP;
    m_nasip = constants::DEFAULT_NASIP;
    m_allowInsecureSsl = false;
    m_keepaliveMinSec = constants::KEEPALIVE_MIN_SEC;
    m_keepaliveMaxSec = constants::KEEPALIVE_MAX_SEC;
    m_offlineFailThreshold = constants::OFFLINE_FAIL_THRESHOLD;
    m_connectTimeoutMs = constants::CONNECT_TIMEOUT_MS;
    m_probeTimeoutMs = constants::PROBE_TIMEOUT_MS;
    m_probeUrl = constants::NCSI_URL;
    m_healthUrl = constants::HEALTH_URL;
    m_requestEncoding = QStringLiteral("AUTO");

    // 仅移除全局设置自身写入的顶层键；localProfiles 设备配置组不在此列，予以保留。
    // Remove only the keys owned by global settings; keep the localProfiles group.
    QSettings s(constants::ORG_NAME, constants::APP_NAME);
    static const QStringList ownedKeys = {
        QStringLiteral("username"),          QStringLiteral("remember_password"),
        QStringLiteral("auto_login"),        QStringLiteral("lang"),
        QStringLiteral("theme_mode"),
        QStringLiteral("host"),              QStringLiteral("ip"),
        QStringLiteral("nasip"),             QStringLiteral("allow_insecure_ssl"),
        QStringLiteral("keepalive_min_sec"), QStringLiteral("keepalive_max_sec"),
        QStringLiteral("offline_fail_threshold"), QStringLiteral("connect_timeout_ms"),
        QStringLiteral("probe_timeout_ms"),  QStringLiteral("probe_url"),
        QStringLiteral("health_url"),        QStringLiteral("request_encoding"),
        QStringLiteral("password_ob")
    };
    for (const QString& key : ownedKeys) s.remove(key);
    s.sync();

    // 清除已保存口令 / drop the stored credential.
    SecretStore::instance().clear(QStringLiteral("password"));

    emit usernameChanged(); emit passwordChanged();
    emit rememberPasswordChanged(); emit autoLoginChanged();
    emit langChanged(); emit themeModeChanged(); emit hostChanged(); emit ipChanged(); emit nasipChanged();
    emit allowInsecureSslChanged();
    emit keepaliveMinSecChanged(); emit keepaliveMaxSecChanged(); emit offlineFailThresholdChanged();
    emit connectTimeoutMsChanged(); emit probeTimeoutMsChanged();
    emit probeUrlChanged(); emit healthUrlChanged(); emit requestEncodingChanged();

    // 落盘默认值并返回存储状态 / persist defaults and report storage status.
    return save();
}

bool ConfigStore::systemDark() const {
#ifdef Q_OS_WIN
    // Basic 样式下 Qt 的 SystemPalette / styleHints 可能拿不到 Windows 深色偏好，
    // 直接读注册表 AppsUseLightTheme（1=浅色，0=深色），最可靠。
    // Under the Basic style Qt may not surface the Windows dark preference; read
    // the AppsUseLightTheme registry value directly (1 light, 0 dark).
    QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
                  QSettings::NativeFormat);
    const QVariant v = reg.value(QStringLiteral("AppsUseLightTheme"), 1);
    const int light = v.toInt();
    return light == 0;
#else
    if (auto* app = qApp) {
        const Qt::ColorScheme cs = app->styleHints()->colorScheme();
        if (cs == Qt::DarkScheme) return true;
        if (cs == Qt::LightScheme) return false;
    }
    return false;
#endif
}

bool ConfigStore::clearCredentials() {
    // 只清身份字段；服务器地址、语言、保活/超时、探测地址等网络与偏好配置保留。
    // Clear identity fields only; keep host, language, keep-alive/timeout and probe prefs.
    m_username.clear();
    m_password.clear();
    m_rememberPassword = false;
    m_autoLogin = false;

    QSettings s(constants::ORG_NAME, constants::APP_NAME);
    s.remove(QStringLiteral("username"));
    s.remove(QStringLiteral("remember_password"));
    s.remove(QStringLiteral("auto_login"));
    s.remove(QStringLiteral("password_ob"));
    s.sync();
    const bool settingsOk = (s.status() == QSettings::NoError);

    // 清除安全存储中的已存口令 / drop the stored password from the secure store.
    const bool secretOk = SecretStore::instance().clear(QStringLiteral("password"));

    emit usernameChanged(); emit passwordChanged();
    emit rememberPasswordChanged(); emit autoLoginChanged();
    return settingsOk && secretOk;
}

} // namespace gdei

// src/core/ConfigStore.cpp
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/SecretStore.hpp"

#include <QSettings>
#include <utility>

namespace gdei {

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

namespace {
int clampInt(int v, int lo, int hi) { return qBound(lo, v, hi); }
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
    Lang l = static_cast<Lang>(v);
    if (m_lang == l) return;
    m_lang = l; emit langChanged();
}
void ConfigStore::setHost(const QString& v) {
    if (m_host == v) return;
    m_host = v; emit hostChanged();
}
void ConfigStore::setIp(const QString& v) {
    if (m_ip == v) return;
    m_ip = v; emit ipChanged();
}
void ConfigStore::setNasip(const QString& v) {
    if (m_nasip == v) return;
    m_nasip = v; emit nasipChanged();
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
    const QString c = v.trimmed().isEmpty() ? constants::NCSI_URL : v.trimmed();
    if (m_probeUrl == c) return;
    m_probeUrl = c; emit probeUrlChanged();
}
void ConfigStore::setHealthUrl(const QString& v) {
    const QString c = v.trimmed().isEmpty() ? constants::HEALTH_URL : v.trimmed();
    if (m_healthUrl == c) return;
    m_healthUrl = c; emit healthUrlChanged();
}

void ConfigStore::load() {
    QSettings s(constants::ORG_NAME, constants::APP_NAME);
    m_username   = s.value("username", "").toString();
    m_rememberPassword = s.value("remember_password", false).toBool();
    m_autoLogin   = s.value("auto_login", false).toBool();
    m_lang        = static_cast<Lang>(s.value("lang", static_cast<int>(Lang::ZhCN)).toInt());
    m_host = s.value("host", constants::DEFAULT_HOST).toString();
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
    m_probeUrl = s.value("probe_url", constants::NCSI_URL).toString().trimmed();
    if (m_probeUrl.isEmpty()) m_probeUrl = constants::NCSI_URL;
    m_healthUrl = s.value("health_url", constants::HEALTH_URL).toString().trimmed();
    if (m_healthUrl.isEmpty()) m_healthUrl = constants::HEALTH_URL;

    // 仅在记住密码时从平台安全存储还原口令 / restore from secure store only when remembered.
    m_password = m_rememberPassword ? SecretStore::instance().read(QStringLiteral("password"))
                                    : QString();

    emit usernameChanged(); emit passwordChanged();
    emit rememberPasswordChanged(); emit autoLoginChanged();
    emit langChanged(); emit hostChanged(); emit ipChanged(); emit nasipChanged();
    emit allowInsecureSslChanged();
    emit keepaliveMinSecChanged(); emit keepaliveMaxSecChanged(); emit offlineFailThresholdChanged();
    emit connectTimeoutMsChanged(); emit probeTimeoutMsChanged();
    emit probeUrlChanged(); emit healthUrlChanged();
}

void ConfigStore::save() {
    QSettings s(constants::ORG_NAME, constants::APP_NAME);
    s.setValue("username", m_username);
    s.setValue("remember_password", m_rememberPassword);
    s.setValue("auto_login", m_autoLogin);
    s.setValue("lang", static_cast<int>(m_lang));
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
    // 清理旧版本写入的混淆口令遗留键 / drop the legacy obfuscated password key.
    s.remove("password_ob");

    // 口令走平台安全存储；未记住或为空时清除 / password via secure store; clear when not remembered.
    if (m_rememberPassword && !m_password.isEmpty()) {
        SecretStore::instance().write(QStringLiteral("password"), m_password);
    } else {
        SecretStore::instance().clear(QStringLiteral("password"));
    }
}

} // namespace gdei

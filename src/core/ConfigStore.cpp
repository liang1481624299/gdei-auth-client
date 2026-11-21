// src/core/ConfigStore.cpp
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/Crypto.hpp"

#include <QSettings>

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

void ConfigStore::load() {
    QSettings s(constants::ORG_NAME, constants::APP_NAME);
    m_username   = s.value("username", "").toString();
    m_passwordOb = s.value("password_ob", "").toString();
    m_rememberPassword = s.value("remember_password", false).toBool();
    m_autoLogin   = s.value("auto_login", false).toBool();
    m_lang        = static_cast<Lang>(s.value("lang", static_cast<int>(Lang::ZhCN)).toInt());
    m_host = s.value("host", constants::DEFAULT_HOST).toString();
    m_ip   = s.value("ip", constants::DEFAULT_IP).toString();
    m_nasip = s.value("nasip", constants::DEFAULT_NASIP).toString();

    // 仅在记住密码时还原口令 / restore password only when remembered.
    m_password = m_rememberPassword ? Crypto::deobfuscate(m_passwordOb) : QString();

    emit usernameChanged(); emit passwordChanged();
    emit rememberPasswordChanged(); emit autoLoginChanged();
    emit langChanged(); emit hostChanged(); emit ipChanged(); emit nasipChanged();
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

    if (m_rememberPassword && !m_password.isEmpty()) {
        m_passwordOb = Crypto::obfuscate(m_password);
    } else {
        m_passwordOb.clear();
    }
    s.setValue("password_ob", m_passwordOb);
}

} // namespace gdei

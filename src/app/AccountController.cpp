// src/app/AccountController.cpp
#include "app/AccountController.hpp"
#include "core/AccountPortalClient.hpp"
#include "core/ConfigStore.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"

namespace gdei {

AccountController::AccountController(ConfigStore* config, Logger* logger, I18n* i18n,
                                     QObject* parent)
    : QObject(parent),
      m_account(new AccountPortalClient(config, logger, this)),
      m_i18n(i18n) {
    connect(m_account, &AccountPortalClient::authenticatedChanged,
            this, &AccountController::authenticatedChanged);
    connect(m_account, &AccountPortalClient::busyChanged,
            this, &AccountController::busyChanged);
    connect(m_account, &AccountPortalClient::profileChanged,
            this, &AccountController::profileChanged);
    connect(m_account, &AccountPortalClient::servicesChanged,
            this, &AccountController::servicesChanged);
    connect(m_account, &AccountPortalClient::captchaImageChanged,
            this, &AccountController::captchaImageChanged);
    connect(m_account, &AccountPortalClient::lastMessageChanged,
            this, &AccountController::lastMessageChanged);
    connect(m_account, &AccountPortalClient::loginFinished,
            this, &AccountController::loginFinished);
    if (m_i18n) {
        connect(m_i18n, &I18n::langChanged, this, &AccountController::lastMessageChanged);
    }
}

bool AccountController::authenticated() const { return m_account->authenticated(); }
bool AccountController::busy() const { return m_account->busy(); }
QVariantMap AccountController::profile() const { return m_account->profile(); }
QVariantList AccountController::services() const { return m_account->services(); }
QString AccountController::captchaImage() const { return m_account->captchaImage(); }
QString AccountController::lastMessage() const {
    // 非已注册消息键原样透传（兼容 core 层双语串与服务端原文）。
    // Unknown message keys pass through unchanged (core bilingual strings / raw text).
    return m_i18n ? m_i18n->localize(m_account->lastMessage()) : m_account->lastMessage();
}

void AccountController::fetchCaptcha() { m_account->fetchCaptcha(); }
void AccountController::loginWithCode(const QString& code) { m_account->loginWithCode(code); }
void AccountController::refreshProfile() { m_account->refreshProfile(); }
void AccountController::logout() { m_account->logout(); }
void AccountController::clearSession() { m_account->clearSession(); }

} // namespace gdei

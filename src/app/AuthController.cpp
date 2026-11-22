// src/app/AuthController.cpp
#include "app/AuthController.hpp"
#include "core/ConfigStore.hpp"
#include "core/I18n.hpp"
#include "core/KeepAlive.hpp"
#include "core/Logger.hpp"
#include "core/PortalClient.hpp"

namespace gdei {
namespace {

QString localizedMessage(const I18n* i18n, const QString& message) {
    if (!i18n) return message;
    if (message == QLatin1String("encoding.nonAscii") ||
        message == QLatin1String("encoding.unmappable") ||
        message == QLatin1String("encoding.invalidForm")) {
        return i18n->text(message);
    }
    return message;
}

} // namespace

AuthController::AuthController(ConfigStore* config, Logger* logger, I18n* i18n,
                               QObject* parent)
    : QObject(parent), m_cfg(config), m_log(logger), m_i18n(i18n) {
    m_portal = new PortalClient(m_cfg, m_log, this);
    m_keep = new KeepAlive(m_portal, m_cfg, m_log, this);

    connect(m_portal, &PortalClient::stateChanged, this, &AuthController::stateChanged);
    connect(m_portal, &PortalClient::onlineChanged, this, &AuthController::onlineChanged);
    connect(m_portal, &PortalClient::loginFinished, this,
            [this](bool ok, const QString& message) {
                setMessage(message);
                if (m_log) m_log->log(ok ? LogLevel::Info : LogLevel::Warning, lastMessage());
                emit loginFinished(ok, message);
            });
    connect(m_keep, &KeepAlive::runningChanged, this, &AuthController::keepAliveRunningChanged);
    if (m_i18n) {
        connect(m_i18n, &I18n::langChanged, this, &AuthController::lastMessageChanged);
    }
}

int AuthController::state() const { return m_portal->state(); }
bool AuthController::online() const { return m_portal->online(); }
bool AuthController::keepAliveRunning() const { return m_keep->running(); }
QString AuthController::lastMessage() const { return localizedMessage(m_i18n, m_lastMessage); }

void AuthController::setMessage(const QString& message) {
    if (m_lastMessage == message) return;
    m_lastMessage = message;
    emit lastMessageChanged();
}

void AuthController::login() { m_portal->login(); }
void AuthController::logout() { m_portal->logout(); }
void AuthController::refreshStatus() { m_portal->refreshStatus(); }
void AuthController::startKeepAlive() { m_keep->start(); }
void AuthController::stopKeepAlive() { m_keep->stop(); }

} // namespace gdei

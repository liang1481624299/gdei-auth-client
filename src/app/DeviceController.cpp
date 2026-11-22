// src/app/DeviceController.cpp
#include "app/DeviceController.hpp"
#include "core/ConfigStore.hpp"
#include "core/DevicePortalClient.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"

namespace gdei {

DeviceController::DeviceController(ConfigStore* config, Logger* logger, I18n* i18n,
                                   QObject* parent)
    : QObject(parent), m_dev(new DevicePortalClient(config, logger, this)), m_i18n(i18n) {
    connect(m_dev, &DevicePortalClient::authenticatedChanged,
            this, &DeviceController::authenticatedChanged);
    connect(m_dev, &DevicePortalClient::busyChanged,
            this, &DeviceController::busyChanged);
    connect(m_dev, &DevicePortalClient::devicesChanged,
            this, &DeviceController::devicesChanged);
    connect(m_dev, &DevicePortalClient::captchaImageChanged,
            this, &DeviceController::captchaImageChanged);
    connect(m_dev, &DevicePortalClient::lastMessageChanged,
            this, &DeviceController::lastMessageChanged);
    connect(m_dev, &DevicePortalClient::loginFinished,
            this, &DeviceController::loginFinished);
    if (m_i18n) {
        connect(m_i18n, &I18n::langChanged, this, &DeviceController::lastMessageChanged);
    }
}

bool DeviceController::authenticated() const { return m_dev->authenticated(); }
bool DeviceController::busy() const { return m_dev->busy(); }
QVariantList DeviceController::devices() const { return m_dev->devices(); }
QString DeviceController::captchaImage() const { return m_dev->captchaImage(); }
QString DeviceController::lastMessage() const {
    return m_i18n ? m_i18n->localize(m_dev->lastMessage()) : m_dev->lastMessage();
}

void DeviceController::fetchCaptcha() { m_dev->fetchCaptcha(); }
void DeviceController::loginWithCode(const QString& code) { m_dev->loginWithCode(code); }
void DeviceController::refreshDevices() { m_dev->refreshDevices(); }
void DeviceController::clearSession() { m_dev->clearSession(); }

} // namespace gdei

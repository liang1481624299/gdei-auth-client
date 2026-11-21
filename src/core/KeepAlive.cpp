// src/core/KeepAlive.cpp
#include "core/KeepAlive.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/PortalClient.hpp"

#include <QRandomGenerator>

namespace gdei {

KeepAlive::KeepAlive(PortalClient* portal, ConfigStore* config, Logger* logger,
                     QObject* parent)
    : QObject(parent), m_portal(portal), m_cfg(config), m_log(logger) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &KeepAlive::onTick);
    connect(m_portal, &PortalClient::onlineChanged,
            this, &KeepAlive::onOnlineChanged);
}

bool KeepAlive::running() const { return m_running; }

void KeepAlive::setRunning(bool v) {
    if (m_running == v) return;
    m_running = v;
    emit runningChanged();
}

void KeepAlive::start() {
    setRunning(true);
    m_portal->refreshStatus();
    scheduleNext();
    if (m_log) m_log->info(tr("Keep-alive started / 保活已启动"));
}

void KeepAlive::stop() {
    m_timer->stop();
    setRunning(false);
    if (m_log) m_log->info(tr("Keep-alive stopped / 保活已停止"));
}

void KeepAlive::scheduleNext() {
    m_timer->start(randomIntervalSec() * 1000);
}

int KeepAlive::randomIntervalSec() const {
    return QRandomGenerator::global()->bounded(
        constants::KEEPALIVE_MIN_SEC, constants::KEEPALIVE_MAX_SEC + 1);
}

void KeepAlive::onTick() {
    // 只做连通探测；是否登录由 onlineChanged 回调决定。
    // Probe only; login decision is handled in onOnlineChanged.
    m_portal->refreshStatus();
    scheduleNext();
}

void KeepAlive::onOnlineChanged(bool online) {
    if (!m_running) return;
    if (online) return;
    if (m_log) m_log->warning(tr("Offline, re-authenticating / 掉线，尝试重新认证"));
    m_portal->login();
}

} // namespace gdei

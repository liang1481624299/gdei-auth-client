// src/core/KeepAlive.cpp
#include "core/KeepAlive.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/PortalClient.hpp"

#include <QRandomGenerator>
#include <utility>

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
    m_failCount = 0;
    m_portal->refreshStatus();
    scheduleNext();
    if (m_log) m_log->info(tr("Keep-alive started / 保活已启动"));
}

void KeepAlive::stop() {
    m_timer->stop();
    setRunning(false);
    m_failCount = 0;
    if (m_log) m_log->info(tr("Keep-alive stopped / 保活已停止"));
}

void KeepAlive::scheduleNext() {
    m_timer->start(randomIntervalSec() * 1000);
}

int KeepAlive::randomIntervalSec() const {
    int lo = m_cfg->keepaliveMinSec();
    int hi = m_cfg->keepaliveMaxSec();
    lo = qBound(constants::KEEPALIVE_MIN_BOUND_SEC, lo, constants::KEEPALIVE_MAX_BOUND_SEC);
    hi = qBound(constants::KEEPALIVE_MIN_BOUND_SEC, hi, constants::KEEPALIVE_MAX_BOUND_SEC);
    if (hi < lo) std::swap(lo, hi);
    return QRandomGenerator::global()->bounded(lo, hi + 1);
}

void KeepAlive::onTick() {
    // 只做连通探测；是否登录由 onlineChanged 回调决定。
    // Probe only; login decision is handled in onOnlineChanged.
    m_portal->refreshStatus();
    scheduleNext();
}

void KeepAlive::onOnlineChanged(bool online) {
    if (!m_running) return;
    if (online) {
        m_failCount = 0;
        return;
    }

    // 连续失败达到阈值才判定掉线并重登；阈值内仅记警告，避免单次抖动触发重登。
    // Only declare offline after consecutive failures reach the threshold;
    // within the threshold we log a warning instead of re-authenticating.
    const int threshold = qBound(constants::FAIL_THRESHOLD_MIN,
                                 m_cfg->offlineFailThreshold(),
                                 constants::FAIL_THRESHOLD_MAX);
    ++m_failCount;
    if (m_failCount < threshold) {
        if (m_log) {
            m_log->warning(tr("Offline probe failed %1/%2, will retry / 掉线探测 %1/%2 次，将重试")
                               .arg(m_failCount).arg(threshold));
        }
        return;
    }
    if (m_log) {
        m_log->warning(tr("Offline after %1 consecutive probes, re-authenticating / 连续 %1 次探测失败，重新认证")
                           .arg(m_failCount));
    }
    m_portal->login();
}

} // namespace gdei

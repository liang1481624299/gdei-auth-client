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
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &KeepAlive::onTick);

    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &KeepAlive::requestReconnect);
    connect(m_portal, &PortalClient::loginFinished, this, &KeepAlive::onLoginFinished);
    // 连接每次探测结果而非 onlineChanged：后者只在状态翻转时发出，
    // 持续掉线时永不重复触发，导致失败计数停在 1、阈值形同虚设。
    // Connect the per-probe result instead of onlineChanged, which fires only on a
    // flip and leaves the failure counter stuck at 1 during a sustained outage.
    connect(m_portal, &PortalClient::probeFinished,
            this, &KeepAlive::onProbeResult);
}

bool KeepAlive::running() const { return m_running; }

void KeepAlive::setRunning(bool v) {
    if (m_running == v) return;
    m_running = v;
    emit runningChanged();
}

void KeepAlive::start() {
    if (m_running) return;   // 幂等：重复调用不重置失败计数、不重复探测
    setRunning(true);
    m_failCount = 0;
    resetReconnectState();
    // 首次探测结果回来后由 onProbeResult 安排下一次；这里只发一次探测。
    // The first probe's callback schedules the next tick; issue only one probe here.
    m_portal->refreshStatus();
    if (m_log) m_log->info(tr("Keep-alive started / 保活已启动"));
}

void KeepAlive::stop() {
    m_timer->stop();
    resetReconnectState();
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
    // 只做连通探测；是否重登由 onProbeResult 依据每次探测结果决定。
    // 下一次调度放在探测回调里，避免探测重叠、保持随机间隔。
    // Probe only; re-login is decided in onProbeResult from every probe result.
    // Next tick is scheduled in the callback to avoid overlapping probes.
    if (!m_running) return;
    m_portal->refreshStatus();
}

void KeepAlive::onProbeResult(bool online, QString /*reason*/) {
    if (!m_running) return;
    // 探测结束即安排下一次，保持原有随机间隔，避免 Nginx 403 限流。
    // Schedule the next probe after each result, preserving the random interval.
    scheduleNext();

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
    emit probeFailed(m_failCount, threshold);
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
    // 达到阈值触发重登后清零，避免重登过程中后续探测重复叠加计数。
    // Reset after triggering re-login so in-flight probes don't keep stacking the count.
    m_failCount = 0;
    requestReconnect();
}

void KeepAlive::requestReconnect() {
    if (!m_running) return;
    // 互斥：探测定时器与退避定时器只允许一次在途重登。
    // Mutex: only one re-login may be in flight across probe/backoff timers.
    if (m_reconnectInFlight) return;
    // 退避窗口内不再触发，避免阈值反复命中后变成高频重登。
    if (m_retryTimer->isActive()) return;

    if (m_portal->busy()) {
        // 手动登录或上一次请求仍占用 NAM：不静默吞掉，改为退避后重试。
        // Portal busy (manual login in flight): defer instead of silently dropping.
        if (m_log) m_log->info(tr("Portal busy, re-login deferred / 门户繁忙，重登顺延"));
        scheduleReconnectRetry();
        return;
    }

    ++m_reconnectAttempts;
    m_reconnectInFlight = true;
    if (m_log) {
        m_log->warning(tr("Re-authenticating (attempt %1) / 重新认证（第 %1 次）")
                           .arg(m_reconnectAttempts));
    }
    m_portal->login();
}

void KeepAlive::onLoginFinished(bool ok, QString message) {
    // 只接管保活自己发起的重登，不干扰手动登录的结果处理。
    // Only own re-login attempts are handled here.
    if (!m_reconnectInFlight) return;
    m_reconnectInFlight = false;

    if (ok) {
        const int attempts = m_reconnectAttempts;
        resetReconnectState();
        emit reconnectSucceeded(attempts);
        if (m_log) m_log->info(tr("Re-authentication succeeded / 重新认证成功"));
        return;
    }

    if (m_log) {
        m_log->warning(tr("Re-authentication failed: %1 / 重新认证失败：%1").arg(message));
    }
    if (m_reconnectAttempts >= MAX_ATTEMPTS_WARN) {
        emit keepAliveError(QStringLiteral("events.keepAliveReconnectExhausted"));
    }
    scheduleReconnectRetry();
}

void KeepAlive::scheduleReconnectRetry() {
    if (!m_running) return;
    // 指数退避：30s、60s、120s… 封顶 600s，避免持续重登触发门户限流。
    // Exponential backoff capped at MAX_BACKOFF_SEC.
    int delay = BASE_BACKOFF_SEC;
    for (int i = 1; i < m_reconnectAttempts && delay < MAX_BACKOFF_SEC; ++i) {
        delay *= 2;
    }
    m_backoffSec = qMin(delay, MAX_BACKOFF_SEC);
    emit reconnectScheduled(m_reconnectAttempts + 1, m_backoffSec);
    m_retryTimer->start(m_backoffSec * 1000);
}

void KeepAlive::resetReconnectState() {
    if (m_retryTimer) m_retryTimer->stop();
    m_reconnectInFlight = false;
    m_reconnectAttempts = 0;
    m_backoffSec = 0;
}

} // namespace gdei

// gdei/keepalive.cpp
#include "gdei/keepalive.hpp"
#include "gdei/portal_client.hpp"

#include <chrono>
#include <thread>

namespace gdei {

KeepAlive::KeepAlive(const Config& cfg,
                     std::function<void(const std::string&)> log)
    : cfg_(cfg), log_(log), stop_(false) {}

void KeepAlive::stop() { stop_ = true; }

void KeepAlive::emit(const std::string& msg) const {
    if (log_) log_(msg);
}

Outcome KeepAlive::runOnce() {
    PortalClient c(cfg_);

    if (c.probeInternet(6)) {
        emit("在线 / online");
        return Outcome::success("在线 / online");
    }

    std::string detail;
    c.probePortal(&detail);
    emit("掉线，尝试重新认证 / offline, re-authenticating (" + detail + ")");

    Outcome r = c.login();
    emit(r.message);
    return r;
}

void KeepAlive::loop() {
    int interval = cfg_.keepaliveIntervalSec > 0 ? cfg_.keepaliveIntervalSec : 60;
    while (!stop_) {
        runOnce();
        // 逐秒睡眠，便于及时响应 stop()。
        for (int i = 0; i < interval && !stop_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace gdei

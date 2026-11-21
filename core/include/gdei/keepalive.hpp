// gdei/keepalive.hpp
// 自动保活 / Auto keep-alive.
// 周期性探测外网连通性；掉线则自动重新发包认证。
// Periodically probes Internet reachability; re-authenticates when offline.
#ifndef GDEI_KEEPALIVE_HPP
#define GDEI_KEEPALIVE_HPP

#include "gdei/config.hpp"
#include "gdei/result.hpp"

#include <functional>
#include <string>

namespace gdei {

class KeepAlive {
public:
    explicit KeepAlive(const Config& cfg,
                       std::function<void(const std::string&)> log = nullptr);

    void stop();   // 退出循环 / request loop exit

    // 执行一轮探测 + 必要时重登。
    Outcome runOnce();

    // 阻塞循环，按配置间隔轮询，直到 stop()。
    void loop();

private:
    Config cfg_;
    std::function<void(const std::string&)> log_;
    volatile bool stop_;

    void emit(const std::string& msg) const;
};

} // namespace gdei

#endif // GDEI_KEEPALIVE_HPP

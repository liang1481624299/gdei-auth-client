// gdei/portal_client.hpp
// H3C iMC Portal 认证客户端核心 / Portal authentication core.
// 协议字段来自 2026-11-21 实测抓包，见 docs/PROTOCOL.md。
#ifndef GDEI_PORTAL_CLIENT_HPP
#define GDEI_PORTAL_CLIENT_HPP

#include "gdei/config.hpp"
#include "gdei/http_client.hpp"
#include "gdei/result.hpp"

#include <map>
#include <memory>
#include <string>

namespace gdei {

// 门户返回报文解析结果 / parsed portal response.
struct PortalReply {
    bool decoded = false;        // 是否成功解出 JSON / JSON decoded
    bool success = false;        // errorNumber == "1"
    std::string message;         // 成功或失败文案 / success or error text
    std::string raw;             // 解码后的原始 JSON / decoded raw JSON
};

class PortalClient {
public:
    explicit PortalClient(const Config& cfg, int timeoutSec = 10);
    ~PortalClient();

    // 登录 / 上线；user/pass 为空时取配置。
    Outcome login(const std::string& user = std::string(),
                  const std::string& pass = std::string());
    // 注销 / 下线。
    Outcome logout(const std::string& user = std::string(),
                   const std::string& pass = std::string());

    // 门户是否可达（能拿到 HTTP 响应即可，不代表已认证）。
    bool probePortal(std::string* detail = 0);
    // 外网是否已通（NCSI 探测未被门户重定向），即已认证上网。
    bool probeInternet(int timeoutSec = 6);

    // 构造登录表单（顺序与浏览器 serializeArray 一致）。
    static std::string buildForm(const Config& cfg,
                                 const std::string& user,
                                 const std::string& pass,
                                 const std::string& userip = std::string(),
                                 const std::string& userurl = std::string());

    // 报文：Base64 -> URL 解码 -> JSON。
    static PortalReply parsePayload(const std::string& text);

private:
    Config cfg_;
    int timeout_;
    std::unique_ptr<HttpClient> http_;
    std::map<std::string, std::string> cookies_;

    std::string authority() const;          // host[:port]
    std::string buildUrl(const std::string& path) const;
    HttpResponse send(const std::string& method, const std::string& path,
                      const std::string& body);
    void openEntry();                       // 先访问入口/登录页，尽量建立会话
};

} // namespace gdei

#endif // GDEI_PORTAL_CLIENT_HPP

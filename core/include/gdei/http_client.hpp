// gdei/http_client.hpp
// HTTP 传输抽象 / Transport abstraction.
// 核心协议只依赖这个接口；Windows 用 WinINet 后端，
// POSIX / macOS / Android 用 libcurl 后端，便于按平台替换。
// Core protocol depends only on this interface; backend is platform-specific.
#ifndef GDEI_HTTP_CLIENT_HPP
#define GDEI_HTTP_CLIENT_HPP

#include <string>
#include <vector>

namespace gdei {

struct HttpHeader {
    std::string name;
    std::string value;
};

struct HttpResponse {
    int status = 0;
    std::string body;
    std::vector<HttpHeader> headers;
    std::string error;   // 传输层错误；为空表示请求已发出 / transport error, empty = sent

    bool ok() const { return error.empty(); }
    const std::string header(const std::string& name) const;
};

class HttpClient {
public:
    virtual ~HttpClient() {}

    // method: GET / POST ...
    // url: 完整地址 full URL scheme://host[:port]/path?query
    // hostHeader: 直连 IP 时仍需带原始域名 Host 头 / original Host header
    // headers: 附加请求头；body: 已编码的表单体（GET 时为空）
    virtual HttpResponse request(const std::string& method,
                                 const std::string& url,
                                 const std::string& hostHeader,
                                 const std::vector<HttpHeader>& headers,
                                 const std::string& body,
                                 int timeoutSec) = 0;
};

// 按平台创建默认后端 / create default backend for the current platform.
HttpClient* createDefaultHttpClient();

} // namespace gdei

#endif // GDEI_HTTP_CLIENT_HPP

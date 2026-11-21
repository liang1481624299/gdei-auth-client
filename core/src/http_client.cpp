// gdei/http_client.cpp
#include "gdei/http_client.hpp"

#include <cctype>

namespace gdei {

const std::string HttpResponse::header(const std::string& name) const {
    for (std::vector<HttpHeader>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string a = it->name, b = name;
        for (size_t k = 0; k < a.size(); ++k) a[k] = static_cast<char>(::tolower(a[k]));
        for (size_t k = 0; k < b.size(); ++k) b[k] = static_cast<char>(::tolower(b[k]));
        if (a == b) return it->value;
    }
    return std::string();
}

namespace internal {
HttpClient* createWinInet();   // http_wininet.cpp (Windows)
HttpClient* createCurl();      // http_curl.cpp (POSIX / Android, with libcurl)
} // namespace internal

namespace {

// 没有可用后端时的占位：返回传输错误，便于上层提示。
class StubClient : public HttpClient {
public:
    HttpResponse request(const std::string&, const std::string&,
                         const std::string&, const std::vector<HttpHeader>&,
                         const std::string&, int) {
        HttpResponse r;
        r.error = "no HTTP backend available / 未编译可用的 HTTP 后端";
        return r;
    }
};

} // namespace

HttpClient* createDefaultHttpClient() {
#if defined(_WIN32)
    return internal::createWinInet();
#elif defined(GDEI_HAVE_CURL)
    return internal::createCurl();
#else
    return new StubClient();
#endif
}

} // namespace gdei

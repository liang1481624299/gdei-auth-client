// gdei/http_wininet.cpp
// Windows 原生 WinINet 后端 / Native WinINet backend (Windows 7 and later).
// 只在 _WIN32 下编译。Compile only on Windows.
#if defined(_WIN32)

#include "gdei/http_client.hpp"

#include <windows.h>
#include <wininet.h>

#include <cstdlib>
#include <sstream>

#pragma comment(lib, "wininet.lib")

namespace gdei {
namespace internal {

namespace {

struct UrlParts {
    bool https = false;
    std::string dialHost;
    int port = 80;
    std::string path;
};

bool parseUrl(const std::string& url, UrlParts& out) {
    std::string scheme, rest;
    size_t p = url.find("://");
    if (p == std::string::npos) return false;
    scheme = url.substr(0, p);
    rest = url.substr(p + 3);
    out.https = (scheme == "https");

    size_t slash = rest.find('/');
    std::string authority = (slash == std::string::npos) ? rest : rest.substr(0, slash);
    out.path = (slash == std::string::npos) ? "/" : rest.substr(slash);

    size_t colon = authority.find(':');
    if (colon == std::string::npos) {
        out.dialHost = authority;
        out.port = out.https ? 443 : 80;
    } else {
        out.dialHost = authority.substr(0, colon);
        out.port = atoi(authority.substr(colon + 1).c_str());
        if (out.port <= 0) out.port = out.https ? 443 : 80;
    }
    return !out.dialHost.empty();
}

std::string lastErrorString(DWORD err) {
    char buf[256];
    buf[0] = '\0';
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, 0, buf, sizeof(buf), NULL);
    std::string s = buf;
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

} // namespace

class WinInetClient : public HttpClient {
public:
    HttpResponse request(const std::string& method,
                         const std::string& url,
                         const std::string& hostHeader,
                         const std::vector<HttpHeader>& headers,
                         const std::string& body,
                         int timeoutSec) {
        HttpResponse resp;
        UrlParts u;
        if (!parseUrl(url, u)) { resp.error = "bad URL"; return resp; }

        HINTERNET hSession = InternetOpenA("GDEI-Auth-Client",
            INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hSession) { resp.error = "InternetOpen failed: " + lastErrorString(GetLastError()); return resp; }

        DWORD timeout = static_cast<DWORD>(timeoutSec * 1000);
        InternetSetOption(hSession, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOption(hSession, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOption(hSession, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        HINTERNET hConnect = InternetConnectA(hSession, u.dialHost.c_str(),
            static_cast<INTERNET_PORT>(u.port), NULL, NULL,
            INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) {
            resp.error = "InternetConnect failed: " + lastErrorString(GetLastError());
            InternetCloseHandle(hSession);
            return resp;
        }

        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
        if (u.https) {
            flags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                     SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                     SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                     SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        }

        HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), u.path.c_str(),
            NULL, NULL, NULL, flags, 0);
        if (!hRequest) {
            resp.error = "HttpOpenRequest failed: " + lastErrorString(GetLastError());
            InternetCloseHandle(hConnect); InternetCloseHandle(hSession);
            return resp;
        }

        // 汇总请求头 / build request header block.
        std::string headerBlock;
        if (!hostHeader.empty()) {
            headerBlock += "Host: "; headerBlock += hostHeader; headerBlock += "\r\n";
        }
        for (std::vector<HttpHeader>::const_iterator it = headers.begin();
             it != headers.end(); ++it) {
            if (it->name == "Host") continue; // Host 已单独处理
            headerBlock += it->name; headerBlock += ": ";
            headerBlock += it->value; headerBlock += "\r\n";
        }
        if (!headerBlock.empty()) {
            HttpAddRequestHeadersA(hRequest, headerBlock.c_str(), -1,
                HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
        }

        BOOL sent = HttpSendRequestA(hRequest, NULL, 0,
            body.empty() ? NULL : (LPVOID)body.data(),
            static_cast<DWORD>(body.size()));
        if (!sent) {
            resp.error = "HttpSendRequest failed: " + lastErrorString(GetLastError());
            InternetCloseHandle(hRequest); InternetCloseHandle(hConnect);
            InternetCloseHandle(hSession);
            return resp;
        }

        // 状态码 / status code.
        DWORD status = 0, sz = sizeof(status);
        HttpQueryInfoA(hRequest, HTTP_STATUS_CODE, &status, &sz, NULL);
        resp.status = static_cast<int>(status);

        // 响应头 / response headers.
        DWORD hdrSize = 0;
        HttpQueryInfoA(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &hdrSize, NULL);
        if (hdrSize > 0) {
            std::string raw(hdrSize, '\0');
            if (HttpQueryInfoA(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF,
                               &raw[0], &hdrSize, NULL)) {
                std::istringstream ss(raw);
                std::string line;
                while (std::getline(ss, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        HttpHeader h;
                        h.name = line.substr(0, colon);
                        h.value = line.substr(colon + 1);
                        size_t f = h.value.find_first_not_of(" \t");
                        if (f != std::string::npos) h.value = h.value.substr(f);
                        resp.headers.push_back(h);
                    }
                }
            }
        }

        // 响应体 / body.
        char buf[4096];
        DWORD read = 0;
        while (InternetReadFile(hRequest, buf, sizeof(buf), &read) && read > 0) {
            resp.body.append(buf, read);
        }

        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return resp;
    }
};

HttpClient* createWinInet() { return new WinInetClient(); }

} // namespace internal
} // namespace gdei

#endif // _WIN32

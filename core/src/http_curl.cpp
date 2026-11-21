// gdei/http_curl.cpp
// libcurl 后端 / libcurl backend (Linux, macOS, Android).
// 仅在定义 GDEI_HAVE_CURL 时编译；CMake 检测到 libcurl 会自动打开。
// Compiled only when GDEI_HAVE_CURL is defined.
#if defined(GDEI_HAVE_CURL)

#include "gdei/http_client.hpp"

#include <curl/curl.h>

#include <cstdlib>

namespace gdei {
namespace internal {

namespace {

size_t writeBody(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::string* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t writeHeaders(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::vector<HttpHeader>* headers =
        static_cast<std::vector<HttpHeader>*>(userdata);
    std::string line(ptr, size * nmemb);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty() && line.back() == '\n') line.pop_back();
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
        HttpHeader h;
        h.name = line.substr(0, colon);
        h.value = line.substr(colon + 1);
        size_t f = h.value.find_first_not_of(" \t");
        if (f != std::string::npos) h.value = h.value.substr(f);
        headers->push_back(h);
    }
    return size * nmemb;
}

class CurlClient : public HttpClient {
public:
    CurlClient() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlClient() { curl_global_cleanup(); }

    HttpResponse request(const std::string& method,
                         const std::string& url,
                         const std::string& hostHeader,
                         const std::vector<HttpHeader>& headers,
                         const std::string& body,
                         int timeoutSec) {
        HttpResponse resp;
        CURL* curl = curl_easy_init();
        if (!curl) { resp.error = "curl_easy_init failed"; return resp; }

        struct curl_slist* slist = NULL;
        if (!hostHeader.empty()) {
            slist = curl_slist_append(slist, ("Host: " + hostHeader).c_str());
        }
        for (std::vector<HttpHeader>::const_iterator it = headers.begin();
             it != headers.end(); ++it) {
            if (it->name == "Host") continue;
            slist = curl_slist_append(slist, (it->name + ": " + it->value).c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBody);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeHeaders);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp.headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        if (!body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
        }

        CURLcode rc = curl_easy_perform(curl);
        if (rc != CURLE_OK) {
            resp.error = curl_easy_strerror(rc);
        } else {
            long code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
            resp.status = static_cast<int>(code);
        }

        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);
        return resp;
    }
};

} // namespace

HttpClient* createCurl() { return new CurlClient(); }

} // namespace internal
} // namespace gdei

#endif // GDEI_HAVE_CURL

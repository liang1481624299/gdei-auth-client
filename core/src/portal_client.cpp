// gdei/portal_client.cpp
#include "gdei/portal_client.hpp"
#include "gdei/encoding.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace gdei {

namespace {

// 表单字段顺序（与浏览器 serializeArray() 一致）/ ordered form fields.
const char* kFieldOrder[] = {
    "freeOTP_secret_hidden", "userName", "userPwd", "userurl", "userip",
    "basip", "language", "usermac", "wlannasid", "wlanssid", "entrance",
    "loginVerifyCode", "userDynamicPwddd", "customPageId",
    "send_dynamic_pwd_type", "pwdMode", "portalProxyIP", "portalProxyPort",
    "dcPwdNeedEncrypt", "assignIpType", "appRootUrl", "manualUrl",
    "manualUrlEncryptKey"
};
const size_t kFieldCount = sizeof(kFieldOrder) / sizeof(kFieldOrder[0]);

const char* kUserAgent =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36";

std::string pathWithNasip(const std::string& tmpl, const std::string& nasip) {
    std::string p = tmpl;
    const std::string key = "{nasip}";
    size_t pos = p.find(key);
    if (pos != std::string::npos) p.replace(pos, key.size(), urlEncode(nasip));
    return p;
}

} // namespace

PortalClient::PortalClient(const Config& cfg, int timeoutSec)
    : cfg_(cfg), timeout_(timeoutSec), http_(createDefaultHttpClient()) {}

PortalClient::~PortalClient() {}

std::string PortalClient::authority() const {
    std::string a = cfg_.ip.empty() ? cfg_.host : cfg_.ip;
    bool defaultPort = (cfg_.scheme == "https" && cfg_.port == 443) ||
                       (cfg_.scheme == "http" && cfg_.port == 80);
    if (!defaultPort) {
        a += ":";
        a += std::to_string(cfg_.port);
    }
    return a;
}

std::string PortalClient::buildUrl(const std::string& path) const {
    return cfg_.scheme + "://" + authority() + path;
}

HttpResponse PortalClient::send(const std::string& method,
                                const std::string& path,
                                const std::string& body) {
    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader{"Host", cfg_.host});
    headers.push_back(HttpHeader{"User-Agent", kUserAgent});
    headers.push_back(HttpHeader{"Accept", body.empty() ? "*/*" : "text/plain, */*; q=0.01"});
    headers.push_back(HttpHeader{"Referer",
        cfg_.scheme + "://" + cfg_.host + "/portal/2021/login.jsp"});

    if (!cookies_.empty()) {
        std::string cookie;
        for (std::map<std::string, std::string>::const_iterator it = cookies_.begin();
             it != cookies_.end(); ++it) {
            if (!cookie.empty()) cookie += "; ";
            cookie += it->first;
            cookie += "=";
            cookie += it->second;
        }
        headers.push_back(HttpHeader{"Cookie", cookie});
    }
    if (!body.empty()) {
        headers.push_back(HttpHeader{"Content-Type",
            "application/x-www-form-urlencoded; charset=UTF-8"});
        headers.push_back(HttpHeader{"X-Requested-With", "XMLHttpRequest"});
    }

    HttpResponse resp = http_->request(method, buildUrl(path), cfg_.host,
                                       headers, body, timeout_);

    // 保存 Set-Cookie（只取第一段 name=value）。
    for (std::vector<HttpHeader>::const_iterator it = resp.headers.begin();
         it != resp.headers.end(); ++it) {
        std::string name = it->name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name != "set-cookie") continue;
        std::string first = it->value.substr(0, it->value.find(';'));
        size_t eq = first.find('=');
        if (eq != std::string::npos) {
            std::string ck = first.substr(0, eq);
            std::string cv = first.substr(eq + 1);
            cookies_[ck] = cv;
        }
    }
    return resp;
}

void PortalClient::openEntry() {
    // 入口与登录页在非浏览器下可能返回空 200，属正常；失败不阻断后续 POST。
    send("GET", pathWithNasip("/portal?nasip={nasip}", cfg_.nasip), "");
    send("GET", pathWithNasip("/portal/2021/login.jsp?nasip={nasip}", cfg_.nasip), "");
}

std::string PortalClient::buildForm(const Config& cfg,
                                    const std::string& user,
                                    const std::string& pass,
                                    const std::string& userip,
                                    const std::string& userurl) {
    std::map<std::string, std::string> f;
    f["freeOTP_secret_hidden"] = "";
    f["userName"] = ensureUtf8(user);
    f["userPwd"] = base64Encode(ensureUtf8(pass));
    f["userurl"] = userurl;
    f["userip"] = userip;
    f["basip"] = cfg.nasip;
    f["language"] = "English";
    f["usermac"] = "null";
    f["wlannasid"] = "";
    f["wlanssid"] = "";
    f["entrance"] = "null";
    f["loginVerifyCode"] = "";
    f["userDynamicPwddd"] = "";
    f["customPageId"] = "105";
    f["send_dynamic_pwd_type"] = "0";
    f["pwdMode"] = "0";
    f["portalProxyIP"] = cfg.ip;
    f["portalProxyPort"] = "50200";
    f["dcPwdNeedEncrypt"] = "1";
    f["assignIpType"] = "0";
    f["appRootUrl"] = cfg.scheme + "://" + cfg.host + "/portal/";
    f["manualUrl"] = "";
    f["manualUrlEncryptKey"] = "";

    std::string out;
    for (size_t k = 0; k < kFieldCount; ++k) {
        if (k) out += "&";
        out += urlEncode(kFieldOrder[k]);
        out += "=";
        out += urlEncode(f[kFieldOrder[k]]);
    }
    return out;
}

PortalReply PortalClient::parsePayload(const std::string& text) {
    PortalReply r;
    std::string t = text;
    // 去空白 / trim
    size_t a = t.find_first_not_of(" \t\r\n");
    size_t b = t.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return r;
    t = t.substr(a, b - a + 1);

    bool ok = false;
    std::string raw = base64Decode(t, &ok);
    if (!ok) { r.raw = t; return r; }
    raw = urlDecode(raw);

    JsonValue root;
    std::string jerr;
    if (!parseJson(raw, root, &jerr) || !root.isObject()) {
        r.raw = raw;
        return r;
    }
    r.decoded = true;
    r.raw = raw;

    const JsonValue* num = root.find("errorNumber");
    r.success = (num && num->asString() == "1");

    const JsonValue* ed = root.find("e_d");
    if (ed && !ed->asString().empty()) {
        const JsonValue* msg = root.find(ed->asString());
        if (msg) r.message = msg->asString();
    }
    if (r.message.empty()) {
        if (const JsonValue* m = root.find("errorMessage")) r.message = m->asString();
    }
    if (r.message.empty()) r.message = raw;
    return r;
}

Outcome PortalClient::login(const std::string& user, const std::string& pass) {
    std::string u = user.empty() ? cfg_.username : user;
    std::string p = pass.empty() ? cfg_.plainPassword() : pass;
    if (u.empty() || p.empty()) {
        return Outcome::failure("未配置账号或密码 / missing username or password");
    }

    openEntry();
    std::string body = buildForm(cfg_, u, p);
    HttpResponse resp = send("POST", "/portal/pws?t=li&ifEmailAuth=false", body);
    if (!resp.ok()) return Outcome::failure("请求异常 / request error: " + resp.error, 2);

    PortalReply reply = parsePayload(resp.body);
    if (reply.success) return Outcome::success("认证成功，已上线 / logged in");
    return Outcome::failure("认证失败 / login failed: " + reply.message, 1);
}

Outcome PortalClient::logout(const std::string& user, const std::string& pass) {
    std::string u = user.empty() ? cfg_.username : user;
    std::string p = pass.empty() ? cfg_.plainPassword() : pass;
    openEntry();
    std::string body = buildForm(cfg_, u, p);
    HttpResponse resp = send("POST", "/portal/pws?t=lo", body);
    if (!resp.ok()) return Outcome::failure("请求异常 / request error: " + resp.error, 2);

    PortalReply reply = parsePayload(resp.body);
    if (reply.success) return Outcome::success("已下线 / logged out");
    return Outcome::failure("注销结果 / logout: " + reply.message, 1);
}

bool PortalClient::probePortal(std::string* detail) {
    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader{"User-Agent", kUserAgent});
    HttpResponse resp = http_->request("GET", cfg_.healthUrl, cfg_.host,
                                       headers, "", timeout_);
    if (detail) {
        if (!resp.ok()) *detail = resp.error;
        else {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "HTTP %d", resp.status);
            *detail = buf;
        }
    }
    return resp.ok() && resp.status > 0;
}

bool PortalClient::probeInternet(int timeoutSec) {
    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader{"User-Agent", "Microsoft NCSI"});
    HttpResponse resp = http_->request("GET",
        "http://www.msftconnecttest.com/connecttest.txt", "", headers, "", timeoutSec);
    if (!resp.ok()) return false;
    return resp.body.find("Microsoft") != std::string::npos;
}

} // namespace gdei

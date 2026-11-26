// src/core/Constants.hpp
// 全局常量：门户地址、表单字段、默认值、超时与退避间隔。
// Central constants: portal URLs, form fields, defaults, timeouts, backoff.
// 不在业务代码里写魔法字符串 / No magic strings in business code.
#ifndef GDEI_CONSTANTS_HPP
#define GDEI_CONSTANTS_HPP

#include <QString>
#include <QStringList>

namespace gdei::constants {

// ---- 应用信息 / App info ----
inline const QString APP_NAME = "gdei-connect";
inline const QString ORG_NAME = "GDEI";

// ---- 门户地址 / Portal endpoints ----
inline const QString DEFAULT_HOST = "nac.gdei.edu.cn";
inline const QString DEFAULT_IP   = "210.38.64.39";
inline const QString DEFAULT_NASIP = "192.168.100.8";
inline const QString DEFAULT_SCHEME = "https";
inline constexpr int DEFAULT_PORT = 443;

inline const QString LOGIN_PAGE_PATH  = "/portal/2021/login.jsp?nasip=";
// Referer 头只引用登录页本身，不带 nasip 查询串（与浏览器实测一致）。
// Referer header references the login page without the nasip query string.
inline const QString LOGIN_PAGE_REFERRER = "/portal/2021/login.jsp";
inline const QString ENTRY_PAGE_PATH  = "/portal?nasip=";
inline const QString LOGIN_ACTION_PATH  = "/portal/pws?t=li&ifEmailAuth=false";
inline const QString LOGOUT_ACTION_PATH = "/portal/pws?t=lo";

// 门户可达性探测 / portal reachability probe.
inline const QString HEALTH_URL =
    "http://nac.gdei.edu.cn:8080/mselfservice/snac/macList.xhtml";
// 外网连通性探测 / external connectivity (NCSI).
inline const QString NCSI_URL =
    "http://www.msftconnecttest.com/connecttest.txt";
// 标准 generate_204 探测：在线回 204 No Content，被 captive 劫持时回 302/200 门户页。
// Standard generate_204 probe: 204 when online, 302/200 portal body when hijacked.
inline const QString NCSI_204_URL =
    "http://edge-http.microsoft.com/captiveportal/generate_204";

// ---- 诊断页探测兜底 / Diagnostics probe fallbacks ----
// 仅用于诊断工具页的公开探测，不参与登录认证 / 报文提交链路。
// Diagnostics-only public probes; never part of the authentication flow.
// HTTP 短探测兜底地址（用户未自定义 probeUrl 时使用）。
inline const QString DIAG_HTTP_FALLBACK_URL = "https://www.baidu.com";
// 编码嗅探兜底门户基址（带自助服务端口）；config.host 为空时使用。
inline const QString DIAG_PORTAL_HOST = "nac.gdei.edu.cn:8080";
// 公共 DoH 对比端点（只读解析 example.com，仅测延迟与可达性）。
inline const QString DOH_ALI_ENDPOINT =
    "https://dns.alidns.com/resolve?name=example.com&type=A";
inline const QString DOH_CF_ENDPOINT =
    "https://cloudflare-dns.com/dns-query?name=example.com&type=A";

// ---- 自助服务门户 / Self-service portal (:8080, 独立会话) ----
inline const QString SELF_SERVICE_SCHEME = "http";
inline constexpr int SELF_SERVICE_PORT = 8080;
inline const QString SELF_LOGIN_PATH    = "/mselfservice/login.jsf";
inline const QString SELF_HOME_PATH     = "/mselfservice/user/index.jsf";
inline const QString SELF_MAC_LIST_PATH = "/mselfservice/snac/macList.xhtml";
inline const QString SELF_CAPTCHA_PATH  = "/mselfservice/Kaptcha.jpg";

// ---- PC 自助门户（PrimeFaces 单视图，:8080）/ PC self-service (PrimeFaces) ----
// 登录后所有模块切换均为 POST 同一 main.jsf 的部分提交，无独立 .jsf 页面。
inline const QString SELF_PC_LOGIN_PATH    = "/selfservice/login.jsf";
inline const QString SELF_PC_HOME_PATH     = "/selfservice/main/main.jsf";
inline const QString SELF_PC_LOGOUT_PATH   = "/selfservice/logout.xml";
inline const QString SELF_PC_CAPTCHA_PATH  = "/selfservice/codeImages/";

// PC 登录表单字段 / PC login form fields（mainForm 前缀）。
inline constexpr const char* PC_FIELD_USERNAME   = "mainForm:username";
inline constexpr const char* PC_FIELD_PASSWORD   = "mainForm:password";
inline constexpr const char* PC_FIELD_CODE       = "mainForm:code";
inline constexpr const char* PC_FIELD_USERTYPE   = "mainForm:usertype_input";
inline constexpr const char* PC_FIELD_USERTYPE_FOCUS = "mainForm:usertype_focus";
inline constexpr const char* PC_FIELD_LOGIN_CMD  = "mainForm:loginCmd";
inline constexpr const char* PC_FIELD_GUEST_CTR  = "mainForm:ifFromGuestCenter";
inline constexpr const char* PC_FIELD_DESC       = "mainForm:descContent";
inline constexpr const char* PC_FIELD_CHECK_CODE = "mainForm:ifCheckCode";
inline constexpr const char* PC_FIELD_DETERMINE_SHOW = "detemineIsShow";
inline constexpr const char* PC_FORM_SUBMIT      = "mainForm_SUBMIT";

// detemineIsShow 标识当前中文登录图 / Hidden flag for the Chinese login image.
inline const QString PC_VALUE_DETERMINE_SHOW = "login_zh.gif";

// PrimeFaces 部分提交参数 / PrimeFaces partial-submit parameters.
inline constexpr const char* PF_PARTIAL_AJAX = "javax.faces.partial.ajax";
inline constexpr const char* PF_SOURCE       = "javax.faces.source";
inline constexpr const char* PF_EXECUTE      = "javax.faces.execute";
inline constexpr const char* PF_VIEW_STATE   = "javax.faces.ViewState";

// 用户类型：1 普通用户 / 2 设备管理用户。
inline const QString PC_USERTYPE_NORMAL = "1";
inline const QString PC_USERTYPE_DEVICE = "2";

inline const QString USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36";

// 浏览器风格 Accept / Accept-Language 头；实测门户过滤器要求两者同时存在，
// 缺任一时回 200 空体。AJAX 提交与页面浏览分别对齐浏览器取值。
// Browser-style headers: the portal filter requires both Accept and
// Accept-Language present, otherwise it answers 200 with an empty body.
inline const QString ACCEPT_HTML =
    "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
inline const QString ACCEPT_AJAX =
    "application/json, text/javascript, */*; q=0.01";
inline const QString ACCEPT_LANGUAGE = "en-US,en;q=0.9";

// ---- 表单字段名 / Form field names ----
inline constexpr const char* FIELD_FREE_OTP      = "freeOTP_secret_hidden";
inline constexpr const char* FIELD_USERNAME      = "userName";
inline constexpr const char* FIELD_PASSWORD      = "userPwd";
inline constexpr const char* FIELD_USERURL       = "userurl";
inline constexpr const char* FIELD_USERIP        = "userip";
inline constexpr const char* FIELD_BASIP         = "basip";
inline constexpr const char* FIELD_LANGUAGE      = "language";
inline constexpr const char* FIELD_USERS_MAC     = "usermac";
inline constexpr const char* FIELD_WLAN_NASID    = "wlannasid";
inline constexpr const char* FIELD_WLAN_SSID     = "wlanssid";
inline constexpr const char* FIELD_ENTRANCE      = "entrance";
inline constexpr const char* FIELD_VERIFY_CODE   = "loginVerifyCode";
inline constexpr const char* FIELD_DYNAMIC_PWD   = "userDynamicPwddd";
inline constexpr const char* FIELD_CUSTOM_PAGE   = "customPageId";
inline constexpr const char* FIELD_DYN_PWD_TYPE  = "send_dynamic_pwd_type";
inline constexpr const char* FIELD_PWD_MODE      = "pwdMode";
inline constexpr const char* FIELD_PROXY_IP      = "portalProxyIP";
inline constexpr const char* FIELD_PROXY_PORT    = "portalProxyPort";
inline constexpr const char* FIELD_PWD_ENCRYPT   = "dcPwdNeedEncrypt";
inline constexpr const char* FIELD_ASSIGN_IP     = "assignIpType";
inline constexpr const char* FIELD_APP_ROOT_URL  = "appRootUrl";
inline constexpr const char* FIELD_MANUAL_URL    = "manualUrl";
inline constexpr const char* FIELD_MANUAL_KEY    = "manualUrlEncryptKey";

// ---- 表单默认值 / Form defaults (captured from live portal) ----
inline const QString DEFAULT_LANGUAGE      = "English";
inline const QString DEFAULT_NULL          = "null";
inline const QString DEFAULT_CUSTOM_PAGE   = "105";
inline const QString DEFAULT_DYN_PWD_TYPE  = "0";
inline const QString DEFAULT_PWD_MODE      = "0";
inline const QString DEFAULT_PROXY_PORT    = "50200";
inline const QString DEFAULT_PWD_ENCRYPT   = "1";
inline const QString DEFAULT_ASSIGN_IP     = "0";

// ---- 超时与保活退避 / Timeouts & keep-alive backoff ----
inline constexpr int CONNECT_TIMEOUT_MS   = 10000;
inline constexpr int PROBE_TIMEOUT_MS     = 6000;
// 空响应分支采信"近期外网探测可达"旁证的新鲜度窗口（毫秒）。
// Freshness window for trusting a recent external probe as an empty-body online hint.
inline constexpr qint64 EMPTY_HINT_FRESH_MS = 120000;
// 超时可配置边界（毫秒）/ Bounds for configurable timeouts (ms).
inline constexpr int TIMEOUT_MIN_MS       = 1000;
inline constexpr int TIMEOUT_MAX_MS       = 60000;
// 随机间隔区间（秒）：基线默认 3 分钟 ±30s（150–210 秒），避免固定频率被 Nginx 限流 403。
// Random jitter range: baseline 3 min ±30s (150–210s) to avoid Nginx rate-limit 403.
inline constexpr int KEEPALIVE_MIN_SEC    = 150;
inline constexpr int KEEPALIVE_MAX_SEC    = 210;
// 连续探测失败达到该次数才判定掉线并重登；单次失败只记警告。
// Consecutive failed probes before declaring offline and re-authenticating.
inline constexpr int OFFLINE_FAIL_THRESHOLD = 3;
// 可配置区间的硬边界 / Hard bounds for configurable keep-alive interval.
inline constexpr int KEEPALIVE_MIN_BOUND_SEC = 30;
inline constexpr int KEEPALIVE_MAX_BOUND_SEC = 600;
inline constexpr int FAIL_THRESHOLD_MIN = 1;
inline constexpr int FAIL_THRESHOLD_MAX = 10;

} // namespace gdei::constants

#endif // GDEI_CONSTANTS_HPP

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
inline const QString ENTRY_PAGE_PATH  = "/portal?nasip=";
inline const QString LOGIN_ACTION_PATH  = "/portal/pws?t=li&ifEmailAuth=false";
inline const QString LOGOUT_ACTION_PATH = "/portal/pws?t=lo";

// 门户可达性探测 / portal reachability probe.
inline const QString HEALTH_URL =
    "http://nac.gdei.edu.cn:8080/mselfservice/snac/macList.xhtml";
// 外网连通性探测 / external connectivity (NCSI).
inline const QString NCSI_URL =
    "http://www.msftconnecttest.com/connecttest.txt";

inline const QString USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36";

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

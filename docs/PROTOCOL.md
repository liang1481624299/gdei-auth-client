# 门户协议逆向说明 / Portal Protocol Notes

> 来源：2026-11-21 对 `nac.gdei.edu.cn` 真实浏览器抓包（H3C iMC Portal 体系）。
> Captured from a real browser session against the live portal.

## 1. 地址 / Endpoints

| 用途 | 地址 |
|---|---|
| 门户域名 / portal host | `https://nac.gdei.edu.cn` |
| 备用直连 IP / fallback IP | `210.38.64.39` |
| NAS (AC) 地址 | `192.168.100.8` |
| 登录页 / login page | `/portal/2021/login.jsp?nasip=192.168.100.8` |
| 登录接口 / login | `POST /portal/pws?t=li&ifEmailAuth=false` |
| 注销接口 / logout | `POST /portal/pws?t=lo` |
| 门户可达性探测 / probe | `http://nac.gdei.edu.cn:8080/mselfservice/snac/macList.xhtml` |
| 外网连通性探测 / NCSI | `http://www.msftconnecttest.com/connecttest.txt`（body 含 `Microsoft` 视为已认证上网） |

## 2. 请求要点 / Request

**登录时序 / Flow**：先 `GET` 登录页 → 解析 HTML 提取 CSRF token（隐藏域）→ 携带完整请求头（UA / Referer / Cookie）`POST` 表单提交。完整 CookieJar 管理会话，匹配 Nginx 会话校验与 IP 绑定。

- 方法：`POST`，`Content-Type: application/x-www-form-urlencoded; charset=UTF-8`
- 带 `X-Requested-With: XMLHttpRequest`，`Referer: https://nac.gdei.edu.cn/portal/2021/login.jsp`
- 直连 IP 时仍需在 `Host` 头带原始域名。
- 用户名不带 `@域名` 后缀；`userip` 留空，由服务端按来源 IP 识别。
- 实现位置：`src/core/PortalClient.cpp`（`sendRequest` / `extractCsrf` / `buildForm` / `parsePayload`）。

### 表单字段（顺序与浏览器 `serializeArray()` 一致）

| 字段 | 默认值 / 说明 |
|---|---|
| `freeOTP_secret_hidden` | 空 |
| `userName` | 用户名（UTF-8） |
| `userPwd` | **Base64(UTF-8 明文口令)**，标准 Base64 |
| `userurl` | 空 |
| `userip` | 空 |
| `basip` | NAS 地址，默认 `192.168.100.8` |
| `language` | `English` |
| `usermac` | `null` |
| `wlannasid` | 空 |
| `wlanssid` | 空 |
| `entrance` | `null` |
| `loginVerifyCode` | 验证码（见第 5 节） |
| `userDynamicPwddd` | 空 |
| `customPageId` | `105` |
| `send_dynamic_pwd_type` | `0` |
| `pwdMode` | `0` |
| `portalProxyIP` | `210.38.64.39` |
| `portalProxyPort` | `50200` |
| `dcPwdNeedEncrypt` | `1` |
| `assignIpType` | `0` |
| `appRootUrl` | `https://nac.gdei.edu.cn/portal/` |
| `manualUrl` | 空 |
| `manualUrlEncryptKey` | 空 |

### 实测登录 Body（示例）

```
freeOTP_secret_hidden=&userName=<username>&userPwd=<base64(password)>&userurl=&userip=&basip=192.168.100.8&language=English&usermac=null&wlannasid=&wlanssid=&entrance=null&loginVerifyCode=&userDynamicPwddd=&customPageId=105&send_dynamic_pwd_type=0&pwdMode=0&portalProxyIP=210.38.64.39&portalProxyPort=50200&dcPwdNeedEncrypt=1&assignIpType=0&appRootUrl=https%3A%2F%2Fnac.gdei.edu.cn%2Fportal%2F&manualUrl=&manualUrlEncryptKey=
```

> 示例中的账号与口令已脱敏；`userPwd` 为口令 UTF-8 字节的标准 Base64。
> Credentials redacted; `userPwd` is standard Base64 of the UTF-8 password bytes.

## 3. 返回报文 / Response

1. Body 先做 **Base64 解码**；
2. 再做 **URL 解码**；
3. 得到 JSON。

- `errorNumber == "1"`：成功。
- 失败文案：取 `e_d` 字段指向的键对应的值；其次取 `errorMessage`。
- 已在线会话下重复登录，接口可能返回**空响应**，属正常现象。

## 4. 字符集 / Encoding

- 内部一律按 **UTF-8** 处理用户名、口令与报文（QString 内部 UTF-16，跨边界按 UTF-8 取字节）。
- 口令在 Base64 之前先按 UTF-8 取字节，避免非 ASCII 口令乱码。
- 网络报文经 `QNetworkAccessManager` 发送，表单体按 UTF-8 编码后 `application/x-www-form-urlencoded`。

## 5. 已知限制 / Notes

- 连续输错密码后，门户会弹出**图形验证码**（`loginVerifyCode` / `id_verifyCode`），当前客户端不处理该分支；遇到时先用浏览器手动登录一次恢复正常。
- 本部署未发现 `id_serviceType` 域名下拉，`inner_str` / `outer_str` 均为 `null`。

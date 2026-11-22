# 门户协议逆向说明 / Portal Protocol Notes

> 来源：2026-11-21 对 `nac.gdei.edu.cn` 真实浏览器抓包（H3C iMC Portal 体系）。
> Captured from a real browser session against the live portal.
> 最后核对 / Last reviewed：本地 CST(UTC+8) `2026-11-22 19:35:32` ｜ UTC/GMT `2026-11-22 11:35:32Z` ｜ UNIX `1795347332`。

## 1. 地址 / Endpoints

| 用途 | 地址 |
|---|---|
| 门户域名 / portal host | `https://nac.gdei.edu.cn` |
| 备用直连 IP / fallback IP | `210.38.64.39` |
| NAS (AC) 地址 | `192.168.100.8` |
| 登录页 / login page | `/portal/2021/login.jsp?nasip=192.168.100.8` |
| 登录接口 / login | `POST /portal/pws?t=li&ifEmailAuth=false` |
| 注销接口 / logout | `POST /portal/pws?t=lo` |
| 门户可达性探测 / probe | `http://nac.gdei.edu.cn:8080/mselfservice/snac/macList.xhtml`（匿名 302 跳登录，认证后 200） |
| 8080 根入口 / 8080 root | `http://nac.gdei.edu.cn:8080/`（匿名 200，126 B，页面内 JS `window.location.href='/selfservice/index.xhtml'`） |
| 8080 captive 入口 / 8080 portal entry | `http://nac.gdei.edu.cn:8080/portal?nasip=192.168.100.8`（302 → `/portal/?nasip=192.168.100.8`；已在线会话再访问返回 200 空体） |
| 外网连通性探测 / NCSI | `http://www.msftconnecttest.com/connecttest.txt`（body 含 `Microsoft` 视为已认证上网） |
| 自助门户登录页 / self-service login | `http://nac.gdei.edu.cn:8080/mselfservice/login.jsf`（详见第 6 节） |
| 自助门户首页 / self-service home | `/mselfservice/user/index.jsf` |
| 绑定终端列表 / bound terminal list | `/mselfservice/snac/macList.xhtml` |
| 增加终端 / add terminal | `/mselfservice/snac/macAdd.xhtml` |

## 2. 请求要点 / Request

**登录时序 / Flow**：`GET` 入口 `/portal?nasip=<nasip>`（302 → `/portal/?nasip=<nasip>`，200 空体，**响应头无 `Set-Cookie`**）→ `GET` 登录页 `/portal/2021/login.jsp?nasip=<nasip>`（HTML 隐藏域；页面**无 CSRF/token 隐藏域**）→ `POST` 表单提交。captive 认证为**无状态**：服务端按来源 IP 鉴权，表单 `basip` / `userip` 隐藏域均为空，不依赖 Cookie 会话；前两步 GET 主要是对齐浏览器时序与渲染页面。

- 方法：`POST`，`Content-Type: application/x-www-form-urlencoded; charset=UTF-8`
- 带 `X-Requested-With: XMLHttpRequest`，`Origin: https://nac.gdei.edu.cn`，`Referer: https://nac.gdei.edu.cn/portal/2021/login.jsp`（Referer 不带 `nasip` 查询串）。
- 直连 IP 时仍需在 `Host` 头带原始域名。

**`hello1/2/3/4/5` Cookie 是前端"记住用户名密码"记忆，不是服务端会话标识**（2026-11-22 阅 `common_head_service_code.js` / `common.js` 确认）：由页面 JS `document.cookie` 写入，`Path=/portal/`，有效期一年；`hello1`=用户名，`hello2`=是否勾选记住密码，`hello3`=口令经每字符 `XOR 0xff` 本地混淆后的串（固定混淆，可逆，**与上线提交的 `userPwd` 无关**），`hello4`=服务类型值，`hello5`=服务类型文本。服务端 captive 认证不读这些 Cookie；客户端无需复刻该记忆逻辑。

- 表单额外字段 `isSavePwd=on`：仅"记住密码"勾选时由浏览器带上，位于 `userPwd` 之后、`userurl` 之前；基线与客户端当前未携带，预计不参与服务端认证校验，记住密码开启后可按需补齐。
- 建连**域名优先**，备用 IP 仅在域名未配置时兜底；门户证书为通配符 `*.gdei.edu.cn`（SAN 不含 IP），用 IP 建连会触发证书主机名不匹配（`HostNameMismatch` / `SEC_E_WRONG_PRINCIPAL`）。IP 兜底时通过 `QNetworkRequest::setPeerVerifyName(host)` 把证书主机名校验名对齐到配置域名；证书链/有效期/签发者仍严格校验，不默认关闭校验。仅在高级设置显式开启 `allowInsecureSsl` 后才调用 `ignoreSslErrors()` 放行并写警告日志。
- 用户名默认不带 `@域名` 后缀；若页面选择了服务类型，前端会把用户名拼成 `用户名@服务类型`（当前部署服务类型为空，提交为纯用户名）。`userip` 留空，由服务端按来源 IP 识别。
- 实现位置：`src/core/PortalClient.cpp`（`sendRequest` / `extractCsrf` / `buildForm` / `parsePayload`）。

### 表单字段（顺序与浏览器 `serializeArray()` 一致）

| 字段 | 默认值 / 说明 |
|---|---|
| `freeOTP_secret_hidden` | 空 |
| `userName` | 用户名（UTF-8） |
| `userPwd` | **Base64(UTF-8 明文口令)**，标准 Base64 |
| `userurl` | 空 |
| `userip` | 空 |
| `basip` | 空（登录页隐藏域 `value=""`；服务端按来源 IP 鉴权） |
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
freeOTP_secret_hidden=&userName=<username>&userPwd=<base64(password)>&userurl=&userip=&basip=&language=English&usermac=null&wlannasid=&wlanssid=&entrance=null&loginVerifyCode=&userDynamicPwddd=&customPageId=105&send_dynamic_pwd_type=0&pwdMode=0&portalProxyIP=210.38.64.39&portalProxyPort=50200&dcPwdNeedEncrypt=1&assignIpType=0&appRootUrl=https%3A%2F%2Fnac.gdei.edu.cn%2Fportal%2F&manualUrl=&manualUrlEncryptKey=
```

> 示例中的账号与口令已脱敏；`userPwd` 为口令 UTF-8 字节的标准 Base64。
> Credentials redacted; `userPwd` is standard Base64 of the UTF-8 password bytes.

## 3. 返回报文 / Response

1. Body 先做 **Base64 解码**；
2. 再做 **URL 解码**；
3. 得到 JSON。

- `errorNumber == "1"`：上线成功。
- `errorNumber == "7"`：**已在线重复登录**（`portServErrorCode="2"`，`portServErrorCodeDesc="User is already online."`）。上线分支应识别为"当前已在线"，而非普通认证失败。
- `errorNumber == "8"`：**已离线**（`portServErrorCode="2"`，`portServErrorCodeDesc="User is offline."`）。下线 `t=lo` 成功后回此码；对下线动作即视为成功。
- 失败文案：取 `e_d` 字段指向的键对应的值；其次取 `errorMessage`。
- **空响应分支**：已在线会话下重复登录，或当前网络已旁路认证时，上线 / 保活三步（入口 GET → 登录页 GET → `POST pws`）均可能返回 200 空体、无 Set-Cookie；这属于"当前已在线 / 旁路"环境的正常现象，不等同于 app 请求构造错误。

### 3.0 已在线 / 已离线报文样本（2026-11-22 实测，浏览器 200，`text/plain;charset=utf-8`）

已在线（重复上线，`Content-Length: 290`）：

```json
{
  "portServErrorCode": "2",
  "portServErrorCodeDesc": "User is already online.",
  "e_c": "portServErrorCode",
  "e_d": "portServErrorCodeDesc",
  "errorNumber": "7"
}
```

已离线（下线 `t=lo` 后，`Content-Length: 44`）：

```json
{
  "portServErrorCode": "2",
  "portServErrorCodeDesc": "User is offline.",
  "e_c": "portServErrorCode",
  "e_d": "portServErrorCodeDesc",
  "errorNumber": "8"
}
```

> 下线 `POST /portal/pws?t=lo` 与上线提交**同一套完整表单**（含 `userName` / `userPwd` / `basip` 等），且 **Request cookies 为空**（实测 "No request cookies were sent"），再次印证 captive 认证无状态、不依赖 `hello*` 记忆 Cookie。

### 3.1 成功报文字段（`pl` 参数 / 响应体 JSON，2026-11-22 实测）

成功后前端 `location.replace("/portal/page/afterLogin.jsp?pl=" + json.portalLink + "&userName=…&loginType=1|2&customCfg=…")`；`pl` 取自成功响应 JSON 的 `portalLink` 字段，其解码流程与第 3 节一致。`portalLink` 是第二重 Base64/URL 编码 JSON，解码后与外层报文同构（仅去掉 `portalLink` 自身）。实测成功报文（`Content-Length: 1538`，取值已脱敏）：

```json
{
  "errorNumber": "1",
  "heartBeatCyc": 0,
  "heartBeatTimeoutMaxTime": 0,
  "userDevPort": "<接入端口/会话标识，脱敏>",
  "userStatus": 99,
  "serialNo": "<会话序列号，脱敏>",
  "ifNeedModifyPwd": false,
  "browserUrl": "",
  "clientPrivateIp": "",
  "userurl": "",
  "usermac": null,
  "nasIp": "",
  "clientLanguage": "English",
  "ifTryUsePopupWindow": false,
  "triggerRedirectUrl": "",
  "portalLink": "<第二重 Base64/URL 编码 JSON，内嵌与外层同构报文>"
}
```

| 字段 | 类型 | 实测值 / 含义 |
|---|---|---|
| `errorNumber` | str | 成功标记，`"1"` = 成功 |
| `heartBeatCyc` | int | 服务端心跳周期（秒）；实测 `0`，未指定，沿用客户端本地保活间隔 |
| `heartBeatTimeoutMaxTime` | int | 心跳超时上限（秒）；实测 `0`，未指定 |
| `userDevPort` | str | 用户接入端口 / 会话标识（vlan 端口串，敏感，不落盘） |
| `userStatus` | int | 用户状态码；实测成功为 `99`（完整枚举待核对） |
| `serialNo` | int | 本次会话序列号（负数，敏感，不落盘） |
| `ifNeedModifyPwd` | bool | 是否需要下次登录改密；实测 `false` |
| `browserUrl` | str | 登录后浏览器跳转 URL，实测空 |
| `clientPrivateIp` | str | 客户端私网 IP；实测空，由服务端按来源地址识别 |
| `userurl` | str | 用户原始访问 URL，实测空 |
| `usermac` | null | 用户 MAC；`null` 表示未上报 |
| `nasIp` | str | NAS / AC 地址；实测空串（与表单 `basip` 无关，后者亦为空） |
| `clientLanguage` | str | 客户端语言，实测 `English` |
| `ifTryUsePopupWindow` | bool | 是否尝试弹窗；实测 `false` |
| `triggerRedirectUrl` | str | 触发重定向 URL，实测空 |
| `portalLink` | str | 成功后跳转 `afterLogin.jsp` 的 `pl` 参数值（第二重 Base64/URL 编码 JSON） |

`loginSucc.jsp` 查询串附加参数：`customCfg=105`、`uamInitLogo=H3C`、`loginType=3`、`v_is_selfLogin=0`、`byodserverip=<BYOD 服务器 IP>`、`byodserverhttpport=8080`；`userName` / `userPwd` 在成功页为 `null`，不落凭据。

> 当前客户端成功判定只消费 `errorNumber` 与失败文案；心跳周期、会话序列号、接入端口等字段暂未纳入模型。`heartBeatCyc=0` 时沿用本地 150–210s 随机保活间隔；若后续服务端下发非 0 周期，再接入 `KeepAlive`。

## 4. 字符集 / Encoding

- 内部一律按 **UTF-8** 处理用户名、口令与报文（QString 内部 UTF-16，跨边界按 UTF-8 取字节）。
- 口令在 Base64 之前先按 UTF-8 取字节，避免非 ASCII 口令乱码。
- 网络报文经 `QNetworkAccessManager` 发送，表单体按 UTF-8 编码后 `application/x-www-form-urlencoded`。

## 5. 已知限制 / Notes

- 连续输错密码后，门户会弹出**图形验证码**（`loginVerifyCode` / `id_verifyCode`），当前客户端不处理该分支；遇到时先用浏览器手动登录一次恢复正常。
- 本部署未发现 `id_serviceType` 域名下拉，`inner_str` / `outer_str` 均为 `null`。

## 6. 自助服务门户 / Self-service portal（`:8080`）

> 2026-11-21 认证后实测。技术栈：JSF + ICEfaces 3.3 + PrimeFaces 3.5（H3C iMC）；移动端路径 `/mselfservice/*`，另有 PC 版 `/selfservice/*`。
> 与 captive portal（`/portal/*`）是**两套独立会话**，不共享登录态。

### 6.1 会话机制 / Session

- 会话标识为 `JSESSIONID`（`Set-Cookie`，`Path=/mselfservice`），同时表单 `action` URL 自带 `;jsessionid=<SID>`；另有 flash cookie `oam.Flash.RENDERMAP.TOKEN`、`oam.Flash.REDIRECT`。后续请求携带 `Cookie: JSESSIONID=<SID>` 即可；匿名访问受保护页面一律 `302 → /mselfservice/login.jsf`。
- 每个 JSF 视图带动态隐藏域：`ice.window`、`ice.view`、`javax.faces.ViewState`，每次渲染都会变化；AJAX 回发需携带，并随响应里的新 ViewState 更新。
- 组件 id（形如 `__n:0:__3k`、`__3a:0:__3m`）由 JSF 运行时生成，**不同渲染会变**；解析应按中文标签文本（`MAC地址`、`ip`、`上网时长`、`绑定时间`…）定位，不要写死 id。

### 6.2 自助门户登录 / Self-service login

1. `GET /mselfservice/login.jsf` → 200，拿到 `JSESSIONID`、`ViewState`、`ice.window`、`ice.view`。
2. 图形验证码：`GET /mselfservice/Kaptcha.jpg`，必须用同一 `JSESSIONID` Cookie 下载，图片与会话绑定。
3. `POST /mselfservice/login.jsf;jsessionid=<SID>`，`application/x-www-form-urlencoded`，字段：

| 字段 | 说明 |
|---|---|
| `textInput` | 账号 |
| `loginCodeInput` | 口令（明文，未做 Base64） |
| `code` | 图形验证码 |
| `submitBtn` | `登录` |
| `j_id_h_SUBMIT` | `1` |
| `ice.window` / `ice.view` / `javax.faces.ViewState` | 视图隐藏域 |

- 成功：`302 Found → /mselfservice/user/index.jsf`，响应体为空。
- 限制：必须人工识别图形验证码；客户端自动化登录此门户需先解决验证码，本期不实现。

### 6.3 认证后页面与数据 / Authenticated pages

**首页 `/mselfservice/user/index.jsf`**（服务端直出，无需二次 AJAX）：

- 在线用户：`ul#currentOnline`，每条含 登录时间、终端类型、MAC 地址、IP、上网时长。
- 历史记录：`ul#historyOnline`，每条含 开始时间 / 结束时间、终端类型、MAC、IP、上网时长。
- 两个列表均为**只读**，页面上没有下线/踢线按钮。

**绑定终端 `/mselfservice/snac/macList.xhtml`**（我的终端，标题 `终端(N)`，容器 `ul#listIterator1`）：

| 字段 | 标签 / 标识 |
|---|---|
| MAC | `span#macTop` |
| 终端类型 | `terminalType`（如 `PC`） |
| 操作系统 | `osType`（可选，部分行缺失） |
| 服务 | `serviceTemplateName`（可选，如 `学生WIFI`） |
| 绑定时间 | `boundTime` |
| 无感知认证 | `status`（`启用` / `禁用`） |
| 终端状态 | `lostStatus`（`正常` / `丢失`） |

行内操作：启用/禁用无感知认证（按钮 id 随当前状态切换）、置为丢失、删除（弹二次确认「确认删除吗？」，`panelConfirmation`）；头部为「返回」「增加」。

**增加终端 `/mselfservice/snac/macAdd.xhtml`**（表单 `mainForm`）：

| 字段 | 说明 |
|---|---|
| `mainForm:original` | MAC 地址（text，maxlength 17，必填） |
| `mainForm:new` | 描述（text，maxlength 64，必填） |
| `mainForm:switch_hidden` | 无感知认证开关，`0` / `1` |
| `mainForm:onAdd` | 提交按钮「完成」（前端先过 `checkValue()` 校验） |
| `mainForm:cancel` | 返回 |

MAC 前端正则接受 `XX-XX-XX-XX-XX-XX`、`XX:XX:XX:XX:XX:XX`、`XXXX-XXXX-XXXX` 三种写法；拒绝全 `00` 与全 `FF` 广播地址。

### 6.4 写操作机制与限制 / Write-action constraints

- 按钮走 ICEfaces bridge：`ice.mobi.button.select(...)` 或 `jsf.ajax.request(source, event, {execute:'@all', render:'@all', mac:'<MAC>', 'javax.faces.behavior.event':'action'})`，回发到当前 `.xhtml` / `.jsf`，返回 JSF partial-response XML（`<partial-response><changes><update id="javax.faces.ViewState">…</update></changes></partial-response>`）。
- 实测：手写标准 `javax.faces.partial.ajax` 回发只能刷新 ViewState，**不能**触发 ICEfaces 的导航或业务动作；增加 / 删除 / 开关 / 置为丢失等写操作需复刻 ICEfaces 增量提交协议，本期不实现。
- **未发现在线会话踢线（强制下线）接口**：自助门户只能查看在线/历史、管理绑定终端；主动下线本机仍走 captive `POST /portal/pws?t=lo`。

### 6.5 对客户端的落地建议 / Implementation notes（P1-7）

- 只读列表可直接 `GET`：`macList.xhtml`（绑定终端）、`index.jsf`（在线 / 历史）；按标签文本解析，不依赖动态组件 id。
- 写操作暂缓；若后续需要，先用真实浏览器录制 ICEfaces 回发报文（`bridge.js` 序列化格式），再设计 `DevicePortalClient` 与 `DeviceInfo` 模型。
- 设备门户与 captive portal 会话独立：设备管理需单独登录（账号 + 口令 + 验证码）；任何会话令牌、口令均不得明文写入仓库。

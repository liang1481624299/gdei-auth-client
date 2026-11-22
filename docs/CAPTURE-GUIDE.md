# 校园网门户抓包指南（另一台电脑 / Capture Guide）

> 用途：在**未认证口**的另一台电脑上，用浏览器 F12 采集 captive 上线 / 下线 / 失败报文与自助门户登录链路，供客户端对齐请求构造与返回判定。
> Purpose: capture the captive online / offline / failure traffic and self-service login flow from an unauthenticated machine via browser DevTools, so the client can align request construction and response verdicts.
>
> 最后核对 / Last reviewed：本地 CST(UTC+8) `2026-11-22 18:03:22` ｜ UTC/GMT `2026-11-22 10:03:22Z` ｜ UNIX `1795341802`。
>
> **合规红线 / Compliance**：回贴前把口令、验证码、手机号、证件号、邮箱、MAC、在线记录等个人敏感值打码；**字段名、URL、路径、状态码、报文结构必须保留**。账号名可保留，口令对应的 Base64 值用 `<redacted>` 代替并注明长度。

## 0. 环境准备 / Environment

1. 用**另一台电脑**，网线**直连校园网**，中间不要经过路由器 / 交换机 NAT（与本机之前的接法一致：直连网卡）。
2. 确认处于**未认证**状态：浏览器随便访问一个 `http://` 站点会被弹回门户，或系统网络提示"无 Internet"。若这台机器之前登录过，先在门户里点"下线 / 注销"，再清浏览器 Cookie。
3. 浏览器用 **Edge 或 Chrome**（不要用手机），版本尽量新。

## 1. F12 准备 / DevTools Setup

1. 按 `F12` 打开开发者工具，切到 **Network（网络）** 面板。
2. 勾选：
   - **Preserve log（保留日志）** —— 跳转 / 刷新后不清空；
   - **Disable cache（禁用缓存）**；
   - 日志级别保留 `All`，过滤框先留空。
3. 面板上方确认抓到的是真实请求；若页面用了 JS 渲染，勾选 **Fetch/XHR** 之外，**Doc / XHR / Fetch / WS** 都要保留。
4. 建议整段结束后右键请求列表 → **Save all as HAR** 导出 `.har` 文件，连同下面的 cURL 一起回贴。

## 2. 抓包 A：一次成功上线 / Successful Online

> 目标：拿到从未认证到上线成功的完整请求链。

1. F12 Network 清空列表，确认 Preserve log 已开。
2. 地址栏访问：`https://nac.gdei.edu.cn/portal/2021/login.jsp?nasip=192.168.100.8`
   - 若被自动跳转到别的登录页，以跳转后的页面为准，把第一跳的 URL 也记下。
3. 在登录页填入账号、口令、图形验证码，点登录。
4. 在 Network 列表里依次找下面这些请求（按时间顺序），逐条点开：

| 顺序 | 预期请求 / Expected request | 关注点 |
|---|---|---|
| 1 | `GET /portal?nasip=192.168.100.8`（可能 302 到 `/portal/?nasip=...`） | 响应头 `Set-Cookie`、`Location`；是否下发会话标识 Cookie |
| 2 | `GET /portal/2021/login.jsp?nasip=...` | 响应头 `Set-Cookie`；HTML 里的隐藏域（CSRF / token / 验证码字段名） |
| 3 | `POST /portal/pws?t=li&ifEmailAuth=false` | **上线提交**，见下方字段清单 |
| 4 | 成功后跳转的 `GET /portal/2021/loginSucc.jsp?pl=...` | `pl` 参数原文；跳转链 |

5. 对第 1、2、3、4 条，每条记录：
   - **请求行**：方法、完整 URL（含 query）、HTTP 状态码、耗时；
   - **请求头**：`Host`、`Referer`、`User-Agent`、`Cookie`、`Content-Type`、`X-Requested-With`、`Accept`；
   - **请求体 / Form Data**：逐字段的 `字段名=值` 原始字符串（口令值打码，保留字段名与顺序）；
   - **响应头**：`Set-Cookie`、`Location`、`Content-Type`、`Content-Encoding`、`Content-Length`；
   - **响应体**：Raw 原文（Base64 / JSON 原样贴，不要格式化）。
6. 最省事的方式：每条请求右键 → **Copy → Copy as cURL (bash)**，存成文本；cURL 里含完整请求头与 body，响应体再单独从 Response 标签复制。

## 3. 抓包 B：一次失败登录 / Failed Login

> 目标：拿到服务端对错误凭据 / 缺验证码的失败报文，对照成功报文区分 `errorNumber` 与失败文案键。

1. 上线成功后，先在门户点"下线 / 注销"回到未认证；或另开无痕窗口。
2. F12 清空，在登录页**故意填错口令**（验证码填正确）提交；若门户强制验证码，也记录一次留空验证码的提交。
3. 抓 `POST /portal/pws?t=li&ifEmailAuth=false` 的：状态码、请求体、响应头、响应体原文。
4. 记录页面弹出的失败提示文案原文（如"用户名或密码错误""验证码错误"）。

## 4. 抓包 C：下线链路 / Offline Logout

1. 保持已在线，F12 清空。
2. 在门户点"下线 / 注销 / 退出登录"。
3. 抓 `POST /portal/pws?t=lo`（以及下线前后的跳转 GET）：请求头、Cookie、请求体、响应体原文。

## 5. 抓包 D：自助门户（阶段 B，可选但推荐）/ Self-service Portal

> 这部分对应 app 的"账户中心 / 设备管理"，当前 PC 自助客户端仍是骨架，需要登录后样本才能接线。

### 5.1 PC 自助（PrimeFaces，`:8080`）

1. 访问 `http://nac.gdei.edu.cn:8080/selfservice/login.jsf`，登录。
2. 记录：
   - 登录 `POST` 的完整表单（字段名、ViewState、验证码）、成功后的 **302 落点 URL**；
   - 登录后首页文档请求（`main.jsf` 或其他）的 URL 与 HTML；
   - 逐个点开左侧 / 顶部菜单（基本信息、费用信息、绑定信息、已申请服务、上网明细、终端设备），抓每次切换的 **PrimeFaces 部分提交**：`javax.faces.source`、`javax.faces.execute`、`ViewState`、表单参数与响应片段；
   - 各列表 / 详情返回的 HTML 或 JSON 结构（字段列名、分页 / 时间范围参数、流量单位）。

### 5.2 移动自助（ICEfaces，`:8080`）

1. 访问 `http://nac.gdei.edu.cn:8080/mselfservice/login.jsf`，登录。
2. 抓登录 `POST`（`textInput` / `loginCodeInput` / `code` / `submitBtn` / `ice.window` / `ice.view` / ViewState）与 `GET /mselfservice/snac/macList.xhtml` 的响应结构。

## 6. 回贴格式 / What to Send Back

- 优先：导出的 **HAR 文件**（整段 Network 记录）。
- 其次：每条关键请求的 **Copy as cURL** + 对应 **Response 原文**。
- 附一句环境说明：本机网卡 IP、是否经路由器、浏览器与系统版本、抓取时间。

## 7. 我这边重点核对的疑点 / Open Questions

1. **入口 Cookie**：`/portal?nasip=` 这一步是否下发 `Set-Cookie`？后续 `POST pws` 携带了哪些 Cookie？app 此前漏了入口 GET，需确认它是否为必需步骤。
2. **表单字段**：浏览器实际上送的字段名、顺序、空值与默认值（`usermac`、`userip`、`basip`、`portalProxyIP`、`appRootUrl`、`language`、`customPageId` 等）与客户端构造是否一致。
3. **口令编码**：浏览器提交前 `userPwd` 是明文、标准 Base64(UTF-8)，还是走了门户 `ssv.js` 的二次编码？对比明文口令与抓包值即可判定。
4. **验证码**：captive 上线是否强制图形验证码？字段名是什么？在什么条件下可留空？
5. **三分支报文**：成功、失败、已在线重复登录（空体）三种响应的状态码与响应体原文，用于把客户端判定逻辑从"空体一律失败"细化为正确分支。
6. **成功跳转**：上线成功后是 302 跳 `loginSucc.jsp?pl=`，还是 AJAX 直接返回 JSON？`pl` 解码后的字段清单。

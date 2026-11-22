# NAC 网络自助服务平台调研 / NAC Self-Service Portal Notes

> 来源：2026-11-22 对公网可访问登录页、未登录跳转行为与前端脚本的静态分析。
> Source: public-facing login pages, unauthenticated redirect behavior and frontend scripts analyzed from the public network.
>
> **环境限制 / Environment caveat**：本次在**校外公网**进行，未持有登录会话。登录后的功能页、AJAX 提交端点、上网明细查询接口与真实返回 JSON 均**未实测**；未登录访问受保护页面仅返回 `meta refresh` 跳回 `login.jsf`，猜测的常见页面名多为 `404`。需在校内网（gdei-wifi）、已登录 WebVPN 会话，或由用户在已登录浏览器 F12 抓包后补抓，见第 7 节。
>
> **合规红线 / Compliance**：账号余额、证件号码、电话、电子邮件、MAC 地址、在线记录等均为本人敏感信息，由客户端在用户登录后按账号**运行时拉取**，不得硬编码进仓库或文档；口令仅经系统密钥存储读取，不明文落盘、不写日志。本文档只记录**字段名与功能分层**，不记录任何真实取值。

## 1. 系统标识与技术栈 / System Identity & Stack

| 项 / Item | 结论 / Finding |
|---|---|
| 厂商 / Vendor | 新华三技术有限公司（H3C），页脚版权 `©2007-2021 新华三技术有限公司`；门户标题「广东二师网络自助服务平台」，页脚 `©2007-2023 广东第二师范学院` |
| 产品形态 / Product | H3C iMC 准入（EAD/NAC）用户自助服务台，含普通用户、设备管理用户、访客自助三类入口 |
| 服务容器 / Container | Apache-Coyote/1.1（Tomcat），HTTP 明文 `:8080` |
| Web 框架 / Framework | JSF（`.jsf` 后缀）+ PrimeFaces 3.5（PC 端主题 `imc-classic-blue`）；触屏端 ICEfaces 3.3 + PrimeFaces aristo |
| 前端依赖 / Frontend | jQuery、PrimeFaces ajax（`PrimeFaces.ab`）、Zepto（触屏端）；公共框架脚本 `imcCommonCompressed.js`、`imcComponent.js`、`ssv.js` |
| 会话标识 / Session | `JSESSIONID`（`HttpOnly`）、`oam.Flash.RENDERMAP.TOKEN`、JSF `javax.faces.ViewState`；ICEfaces 额外带 `ice.window`、`ice.view` |

## 2. 入口与上下文 / Entries & Contexts

| 用途 / Purpose | 地址 / URL | 未登录可达 / Public |
|---|---|---|
| PC 版登录 / PC login | `http://nac.gdei.edu.cn:8080/selfservice/login.jsf` | 是 |
| 触屏版登录 / mobile login | `http://nac.gdei.edu.cn:8080/mselfservice/login.jsf` | 是 |
| 访客管理自助 / guest self-service | `http://nac.gdei.edu.cn:8080/selfservice/mlogin.jsf` | 是 |
| 触屏版忘记密码 / mobile forget password | `http://nac.gdei.edu.cn:8080/mselfservice/user/forgetPassword.jsf` | 是 |
| 设备用户忘记密码 / device-user forget password | `http://nac.gdei.edu.cn:8080/selfservice/mail/devUserForget.jsf` | 是 |
| 公开 FAQ / public FAQ | `http://nac.gdei.edu.cn:8080/selfservice/preregister/unLoginFaqList.jsf` | 是（当前列表 0 条记录） |
| 登录后首页 / post-login home | `http://nac.gdei.edu.cn:8080/selfservice/index.jsf` | 否，未登录 `meta refresh` 跳 `login.jsf` |

PC 登录页底部导航互链：`PC 版` ↔ `触屏版自助`、`访客管理自助`、`忘记密码`、`FAQ`。页面自述功能特性：**网络自助服务、网络故障报修、上网信息查询、终端设备管理**。

### 2.1 受保护页面前缀探测（未登录 302 跳 `login.jsf`）

未登录访问受保护页面，容器返回 `302 → /<context>/login.jsf`；不存在的页面返回 `404`。据此区分出以下受保护前缀（具体文件名仍需登录后菜单复核，`302` 可能是目录级会话过滤，不代表每个候选文件都真实存在）：

| 前缀 / 候选路径 | 上下文 | 探测结果 / Result |
|---|---|---|
| `/mselfservice/user/userInfo.jsf` | 移动 | 302 跳移动登录（用户信息） |
| `/mselfservice/user/feeInfo.jsf` | 移动 | 302（费用信息） |
| `/mselfservice/user/onlineDetail.jsf` | 移动 | 302（上网明细） |
| `/mselfservice/user/bindInfo.jsf` | 移动 | 302（绑定信息） |
| `/mselfservice/user/serviceInfo.jsf` | 移动 | 302（已申请服务） |
| `/mselfservice/user/modifyPassword.jsf`、`updatePassword.jsf` | 移动 | 302（修改密码） |
| `/selfservice/fee/feeInfo.jsf` | PC | 302 跳 PC 登录（费用信息） |
| `/selfservice/guest/guestList.jsf` | PC | 302（访客列表） |
| `/selfservice/guest/guestPreRegister.jsf` | PC | 302（访客预注册） |

移动版功能页集中在 `/mselfservice/user/` 目录；PC 端模块目录命名与移动版不同，匿名猜测多为 `404`，登录后需从左侧菜单/iframe `src` 取真实路径。

## 3. 登录表单与提交 / Login Form & Submission

### 3.1 PC 版（`/selfservice/login.jsf`，表单 `mainForm`）

| 字段名 / Field name | 类型 / Type | 说明 / Notes |
|---|---|---|
| `mainForm:username` | text | 帐号名，必填 |
| `mainForm:password` | password | 密码，必填；**提交前**经前端函数 `imc_byod_function_base64_with_chinese(...)` 做可逆编码后再放入提交值 |
| `mainForm:code` | text | 验证码，`maxlength=4`，必填（隐藏项 `mainForm:ifCheckCode=true` 控制是否校验） |
| `mainForm:usertype_input` | select | 用户类型：`1` 普通用户（默认）/ `2` 设备管理用户；切换时前端 `differWithType()` 显隐对应入口 |
| `mainForm:ifFromGuestCenter` | hidden | 是否来自访客中心，默认 `0` |
| `mainForm:descContent` | hidden | 访客管理员描述文案 |
| `mainForm_SUBMIT` / `javax.faces.ViewState` | hidden | JSF 提交标记与视图状态，**每次 POST 必须随表单回传**当前页的 ViewState |

- 验证码图片：`/selfservice/codeImages/checkCode<数字>.jpg`；「看不清，换一张」走 PrimeFaces ajax（`mainForm:switchValidCode`）异步刷新。
- 密码编码：函数名表明为「带中文的 Base64」，典型实现是先按 UTF-8/`encodeURIComponent` 转字节再 Base64；**属前端可逆编码，不是服务端强加密**。函数体所在的 `ssv.js` 在匿名请求下返回 `404`，具体编码表与是否含加盐需登录环境复核。
- 浏览器兼容提示：推荐 IE10/IE11、Firefox 30+（页脚 50+）、Chrome 44+。

### 3.2 触屏版（`/mselfservice/login.jsf`，ICEfaces 表单）

| 字段名 / Field name | 类型 / Type | 说明 / Notes |
|---|---|---|
| `textInput` | text | 帐号名，`maxlength=32` |
| `loginCodeInput` | password | 密码，`maxlength=32` |
| `code` | text | 验证码，`maxlength=4` |
| `ice.window` / `ice.view` / `javax.faces.ViewState` | hidden | ICEfaces + JSF 视图状态，POST 须回传 |

- 验证码图片：`/mselfservice/Kaptcha.jpg?<随机数>`，点击/触摸刷新。
- 登录前前端校验：空帐号、空密码、空验证码分别弹提示框（`show_warning_panel`）。

### 3.3 找回密码 / Password Recovery

- 触屏版：帐号名 `textInput` + 验证码 `code`，提交按钮 `sendMail`（按帐号向预留邮箱/手机发送重置链路）。
- PC 设备用户：帐号名 `mainForm:accountName` + 验证码 `mainForm:code`。
- 具体发送渠道（邮件/短信）、重置链接有效期与后续步骤需登录/收信后复核。

## 4. 登录后功能模块（按页面文案梳理，字段名脱敏）/ Post-Login Modules

> 以下模块与字段名来自用户提供的已登录页面文案；**字段取值一律不落档**。页面真实 `.jsf` 路径、表单 `name` 与 AJAX 端点待登录态抓包确认。

### 4.1 基本信息 / Basic Profile

| 字段 / Field | 可编辑 / Editable |
|---|---|
| 用户姓名 | 是（更新资料页，必填标记 `*`） |
| 证件号码 | 是（必填标记 `*`） |
| 通讯地址 | 是 |
| 电话 | 是 |
| 电子邮件 | 是 |

### 4.2 用户接入信息 / Account & Access Info

| 字段 / Field | 说明 / Notes |
|---|---|
| 帐号名 | 登录帐号，只读 |
| 帐号状态 | 如「正常」等状态枚举 |
| 允许用户修改密码 | 是/否 |
| 启用密码控制策略 | 是/否 |
| 开户日期 | 日期 |
| 下次登录需修改密码 | 是/否 |
| 帐号类型 | 如「预付费」等计费类型 |
| 帐号余额（元） | 金额，敏感，运行时读取 |
| 帐号生效时间 | 日期时间，可空 |
| 帐号失效时间 | 日期时间 |
| 最大闲置时长（分钟） | 数值，可空 |
| 在线数量限制 | 并发在线上限 |
| 在线状态 | 在线/离线 |
| 当前在线数 | 数值 |
| 最后一次下线时间 | 日期时间 |
| 登录提示信息 | 文本，可空 |
| LDAP 服务器认证 | 是/否 |
| 创建者 | 可空 |

### 4.3 绑定信息 / Binding Info

| 字段 / Field | 说明 / Notes |
|---|---|
| 端口号 | 接入端口，可空 |
| 无线 SSID | 关联热点，可空 |
| 计算机名称 | 终端主机名，可空 |
| Windows 域 | 域信息，可空 |
| MAC 地址 | 绑定终端网卡；更新资料页中可编辑 |

### 4.4 已申请的服务信息 / Granted Services

列表字段：服务名、服务描述、服务后缀、计费周期开始时间、计费周期结束时间。
已观测到的服务后缀示例（属账号开通项，非个人敏感标识）：`gdeivpn`（VPN 接入）、`gdeiwifi-s`（学生 WIFI）、学生接入。

### 4.5 用户费用信息 / Fee Info

| 字段 / Field |
|---|
| 帐号名 |
| 帐号状态 |
| 帐号类型 |
| 帐号余额（元） |
| 应缴费用（元） |

### 4.6 上网明细查询 / Online History

页面文案提及「查询上网明细」，对应历史在线记录/流量明细列表；**查询条件、分页参数、时间范围与返回列待登录态抓包**。预期字段方向：上线时间、下线时间、在线时长、接入方式（有线/无线）、SSID/端口、MAC、上下行流量、在线状态等，以实际页面为准。

### 4.7 其他功能 / Other

- 更新用户资料：提交 4.1 可编辑项 + 绑定 MAC 地址。
- 网络故障报修：页面自述功能之一，报修表单字段与提交端点待抓包。
- 终端设备管理：绑定/解绑终端 MAC、查看在线设备。
- 修改密码：登录后自助改密入口（与 3.3 忘记密码流程不同），表单与策略校验待抓包。

## 5. 对接要点 / Integration Notes

1. **会话保持**：JSF/ICEfaces 为有状态服务，POST 必须携带 `JSESSIONID` Cookie 与当前页 `javax.faces.ViewState`（触屏端另需 `ice.window`/`ice.view`）；ViewState 与会话绑定，刷新页面后须重新取页。
2. **提交方式**：列表查询、按钮操作多经 PrimeFaces ajax 或 ICEfaces 增量提交，字段 `name` 带 `mainForm:` 前缀；不能按普通 REST 表单臆造端点，需以登录后实际请求为准。
3. **密码编码**：PC 端提交前对密码做前端可逆编码；客户端若自行登录，需先在浏览器里确认 `imc_byod_function_base64_with_chinese` 的完整实现（UTF-8 处理 + Base64 字母表），不要直接发送明文，也不要误用标准 Base64 导致校验失败。
4. **验证码**：登录与找回密码均带 4 位图形验证码，存在会话绑定；自动化对接需处理验证码识别或改为引导用户在浏览器/系统密钥库侧完成登录。
5. **敏感数据最小化**：余额、证件号、电话、邮箱、MAC、明细记录仅在用户本人登录后的界面/本地缓存中使用；日志、崩溃上报、仓库文案一律脱敏。

## 6. 与现有客户端的关系 / Relation to the Client

- 本自助平台面向**已入网后的账户管理**（资料、费用、绑定、明细、报修），与仓库现有「校园网认证上线/保活」链路（captive portal 拨号登录）属于不同环节：前者是 NAC 自助 Web 台，后者是接入认证。
- 客户端可在后续版本中以「账户中心」入口对接：登录态复用或独立跳转，先只读展示基本信息/余额/在线状态，再视需要开放改资料、改密、绑定 MAC 等写操作。

## 7. 登录后待补抓清单 / Post-Login TODO

- [ ] 登录后首页 `index.jsf` 左侧/顶部菜单的真实 `.jsf` 路径与页面标题映射；重点核对移动 `/mselfservice/user/` 与 PC `/selfservice/fee/`、`/selfservice/guest/` 下的实际文件名。
- [ ] 各模块列表与详情的 AJAX 端点、请求参数、分页/时间范围参数与返回 JSON 结构。
- [ ] 上网明细查询的条件项、返回列与流量单位。
- [ ] 修改密码、更新资料、报修、绑定/解绑 MAC 的提交字段名、校验规则与成功/失败响应。
- [ ] `imc_byod_function_base64_with_chinese` 完整函数体（定位 `ssv.js` 或内联脚本），确认编码细节。
- [ ] 验证码生成/校验会话绑定方式，以及连续失败锁定策略。
- [ ] 设备管理用户（`usertype=2`）与访客自助（`mlogin.jsf`）的专属功能与字段。

> 采集方式三选一：① 用户在已登录浏览器 F12 复制各接口请求/响应 JSON 回贴；② 本机接入 gdei-wifi 或登录 WebVPN 后继续抓取；③ 先按第 4 节字段名搭个人信息模型与 UI 骨架，接口留待对接。

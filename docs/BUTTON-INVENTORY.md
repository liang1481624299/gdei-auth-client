# 可接入按钮 / 功能入口清单 / Available Button & Entry Inventory

> 生成日期：2026-11-25（CST, UTC+8）。
> 基线：`docs/REQUIREMENTS.md`、`docs/GAP-REPORT.md`、`docs/UI-SPEC-GAPS.md`、`docs/NAC-SELF-SERVICE.md`、`docs/PROTOCOL.md`、`docs/CAS-PORTAL.md`。
> 方法：逐条对照文档记录的门户接口 / 页面，核对当前后端控制器（`src/app/*Controller.*`、`src/core/*PortalClient.*`）与 QML 落点（`qml/pages`、`qml/components`），区分「已接 / 后端占位待解析 / 未实现」。
> **口径前提**：8080 自助门户除已接入的触屏在线/历史/绑定列表外，其余登录后页面、AJAX 端点、写报文均**未持登录会话实测**；本清单只列「可接方向 + 前置依赖」，不臆造 DOM 选择器、AJAX 端点与回发报文。

## 0. 分级与合规口径

### 0.1 优先级分级

| 级别 | 含义 | 落地前提 |
|---|---|---|
| **P1 近期** | 只读；后端登录链路 / 解析雏形已有，或仅需补本地字段；无需录制写报文，风险低 | 补解析、补模型字段、补本地系统枚举即可 |
| **P2 中期** | 只读但需登录态样本补解析，或需新增异步只读后端任务；仍低风险 | 需已登录浏览器 F12 抓回页面 HTML / 返回 JSON（见 `NAC-SELF-SERVICE.md` §7 待抓清单） |
| **P3 远期 / 写操作** | 需复刻 ICEfaces bridge / PrimeFaces partial-submit 回发协议，或属独立 CAS 体系；**本期不实现**，UI 先留占位或不可用提示 | 必须先用真实浏览器录制完整回发报文（source、参数、ViewState、成功判定），经合规评审后再落代码 |

### 0.2 合规红线（与 `REQUIREMENTS.md` §9、`UI-SPEC-GAPS.md` §三一致，不变更）

- 禁止批量踢设备、破解或规避并发设备上限、查询 / 展示他人设备、调用后台管理接口。
- 不做端口 / ARP / 子网扫描；不引入代理、翻墙功能；不打包浏览器内核 / CEF / WebView。
- 破坏性操作（删除绑定、下线、置丢失、静态 IP 解绑等）必须二次确认，默认不误踢本机。
- 余额、证件号、电话、邮箱、MAC、在线明细、会话序列号 / 接入端口等敏感信息仅登录后运行时拉取，不落仓库、不写日志（日志经 Logger 统一脱敏）。
- **任务 #85（满员释放名额）属合规敏感项，本清单不推进其实现**；在线踢线仅在用户手动确认单台本人设备后讨论，且门户是否提供踢线接口待抓包确认。

### 0.3 现状基线：当前已挂出的按钮 / 动作（避免重复造）

| 端 | 已接动作 | 后端门面 |
|---|---|---|
| captive 443 | 上线、下线、保活开关（start/stop）、刷新在线状态、刷新网卡、连通测试、登录、本地注销 | `app.auth`、`app.session` |
| 8080 触屏设备 | 验证码拉取、验证码登录、刷新在线/历史/绑定列表、清空本地会话；条目点击进设备详情页（骨架）；详情页分享 / 设置为置灰占位入口，未绑定动作 | `app.portalDevices` |
| 8080 PC 账户 | 验证码拉取、验证码登录、刷新资料（`parseProfile` 空占位）、清空本地会话；余额 / 套餐 / 时长仅留 UI 槽位 | `app.account` |
| 本地档案 | 增 / 删 / 选本地认证配置 | `app.localDevices` |
| 证书 | 导入 PEM/DER、列表（主题/颁发者/有效期/过期/指纹）、按指纹删除、注入 CA 信任锚 | `app.certificates` |
| 诊断 / 其他 | 日志面板、日志导出、事件分级筛选、清空事件、主题 / 语言切换、高级设置保存、重置默认配置、证书跳过校验开关 | `app.logs`、`app.events`、`app.i18n`、`app.config`、`app.certificates` |

> 备注：`src/core/CertificateStore.*` 已完整落地（导入 / 列表 / 删除 / 持久化 / 注入 `PortalClient`）。早期审计草稿中「证书模块缺失」的结论已过时，以本节现状为准；HTML 审计报告对应条目需后续更正。

---

## A. captive 443 认证链路（核心上线 / 保活）

> 协议依据：`PROTOCOL.md` §1–§5。后端：`src/core/PortalClient.*`、`src/app/AuthController.*`、`src/core/KeepAlive.*`。

| # | 可接按钮 / 展示位 | 来源接口 / 字段 | 后端现状 | 前置依赖 | 分级 |
|---|---|---|---|---|---|
| A1 | 成功报文扩展字段只读展示：接入端口 `userDevPort`、会话序列号 `serialNo`、服务端心跳周期 `heartBeatCyc`、用户状态码 `userStatus`、是否需改密 `ifNeedModifyPwd`（`PROTOCOL.md` §3.1） | 上线成功 JSON（`parsePayload` 已解码，字段未纳入模型） | 报文已 Base64→URL→JSON 解码，仅消费 `errorNumber` 与失败文案；上述字段未进模型 | 在会话 / 网络详情卡补只读模型字段；端口、序列号运行时展示、日志脱敏 | **P1**（纯展示增强） |
| A2 | `ifNeedModifyPwd=true` 时在账号 / 概览卡引导「前往修改密码」 | 成功报文布尔位 | 未建模 | 依赖 A1 字段落模型；改密动作本身见 B12（P3） | **P2** |
| A3 | 服务端心跳周期接入保活：`heartBeatCyc` 非 0 时按服务端周期调度，`=0` 沿用本地 150–210s | 成功报文 `heartBeatCyc` / `heartBeatTimeoutMaxTime` | 当前固定本地随机间隔，未消费心跳字段 | 字段落模型后接 `KeepAlive` 调度；需多窗口实测非 0 样本 | **P2** |
| A4 | 图形验证码分支：连续输错后门户弹码，登录层补验证码图片 + 输入框提交 | captive `loginVerifyCode` / `id_verifyCode`（`PROTOCOL.md` §5） | 当前不处理该分支，遇错只能浏览器手动登录；未实测触发样本 | 需抓取「连续失败 → 弹码」页面结构、验证码图端点与提交字段；自动化识别本期不做，走人工识图 | **P3** |
| A5 | 服务类型下拉（用户名拼 `用户名@服务类型`） | `id_serviceType` / `inner_str` / `outer_str`（`PROTOCOL.md` §2、§5） | 当前部署为空，未发现下拉；提交纯用户名 | 待门户实际下发服务类型枚举再做；本期不臆造选项 | **P3** |

---

## B. 8080 PC 自助门户（PrimeFaces，账户中心）

> 后端：`src/core/AccountPortalClient.*`、`src/app/AccountController.*`；门面 `app.account`。会话独立于 captive，需单独登录（账号 + 口令 + 图形验证码）。
> 路径依据：`NAC-SELF-SERVICE.md` §2.2、§4；登录链路与口令编码已落地（`SelfServiceCodec`），`parseProfile` 为空占位（`AccountPortalClient.hpp` TODO 标注）。

### B.1 只读类（补解析即可，低风险）

| # | 可接按钮 / 展示位 | 来源页面 / 字段 | 后端现状 | 前置依赖 | 分级 |
|---|---|---|---|---|---|
| B1 | 账户资料卡补全：姓名、账号状态、开户日期、账号类型（计费类型）、生效 / 失效时间、在线状态、当前在线数、在线数量限制、最后下线时间、登录提示、是否允许改密、下次登录需改密（§4.1、§4.2） | `/selfservice/maintain/main.jsf` 资料概览 | `profile` map 已建、UI 槽位预留（`DashboardPage` 取 balance/plan/duration）；`parseProfile` 空实现 | 需已登录 `main.jsf` HTML 样本补 DOM 解析；字段按标签文本定位，不写死动态组件 id | **P1**（解析补齐后直接上卡） |
| B2 | 费用信息卡：余额、应缴费用（§4.5） | `/selfservice/fee/userFeeInfo.jsf`（移动对应 `/mselfservice/user/feeInfo.jsf`） | 余额刷新按钮已有，取值槽位在；应缴费用未建模 | 同 B1，需主页 / 费用页样本；余额、应缴为敏感金额，运行时拉取 | **P1** |
| B3 | 「我的服务」列表：服务名、描述、服务后缀、计费周期起止（§4.4） | `/selfservice/fee/preServiceList.jsf`（移动 `/mselfservice/user/serviceInfo.jsf`） | `services` QVariantList 属性已建、控制器已暴露，解析为空 | 需服务列表页样本补解析；账号页新增服务卡片列表 | **P1** |
| B4 | 上网明细子页：上线 / 下线时间、在线时长、接入方式、SSID / 端口、MAC、上下行流量、在线状态（§4.6） | `/selfservice/fee/onlineDetail.jsf`（移动 `/mselfservice/user/onlineDetail.jsf`、`/mselfservice/billdetail/billDetailList.jsf`） | 后端无明细模型与请求阶段 | 需抓查询条件、时间范围 / 分页参数、返回列与流量单位；新增只读拉取阶段 + 子页列表 | **P2** |
| B5 | 账单明细列表 | `/selfservice/fee/billDetail.jsf`（移动 billDetailList） | 未实现 | 同 B4，抓列表列与分页 | **P2** |
| B6 | 缴费记录列表 | `/selfservice/fee/paymentRecord.jsf` | 未实现 | 抓列表列与分页 | **P2** |
| B7 | 报修记录列表（本人历史工单，只读） | `/selfservice/trouble/troubleList.jsf` | 未实现 | 抓工单列表列 | **P2** |
| B8 | 静态 IP 绑定只读列表：IP、MAC、安全组、绑定时间（§4.9） | `/selfservice/boundIp/boundIpList.jsf`（匿名 200 空壳，数据行需登录后填充） | 未实现 | 需登录后数据行 DOM 样本；后端新增只读列表拉取 | **P2** |
| B9 | 常见问题入口：登录后 FAQ；公开 FAQ（匿名可达，当前 0 条记录） | `/selfservice/faq/faqList.jsf`；公开 `/selfservice/preregister/unLoginFaqList.jsf` | 未实现 | 公开页可先接；登录后 FAQ 随账户会话 | **P2** |
| B10 | 在线数 / 剩余可登录设备数统计（需求 5.4 ❌） | 在线计数（触屏 `onlineUserList`）+ 上限（§4.2「在线数量限制」「当前在线数」） | 列表已展示，未单独统计；剩余额度 = 上限 − 当前在线 | 注意跨两套独立会话：上限来自 PC 资料（`app.account`），在线数来自触屏列表（`app.portalDevices`），需两侧会话均就绪再算；**仅展示本人额度，不做满员释放或踢人** | **P1** |

### B.2 写操作类（需录制回发报文，本期不实现）

| # | 可接按钮（远期） | 来源页面 | 现状 / 约束 | 前置依赖 | 分级 |
|---|---|---|---|---|---|
| B11 | 修改资料：姓名、证件号、通讯地址、电话、电子邮件、绑定 MAC（§4.1、§4.3、§4.7） | `/selfservice/maintain/modifyAcct.jsf` | 无写阶段；PrimeFaces partial submit | 录表单字段名、必填校验、提交端点与成功 / 失败响应；提交前二次确认 | **P3** |
| B12 | 修改密码（登录后自助改密，区别于找回密码） | `/mselfservice/user/modifyPassword.jsf`、`updatePassword.jsf`；PC 对应页待复核 | 无写阶段 | 抓旧 / 新密码字段、策略校验（长度 / 复杂度）、失败响应 | **P3** |
| B13 | 故障报修提交 | `/selfservice/trouble/troubleSubmit.jsf` | 无写阶段 | 抓报修表单字段（类型 / 描述 / 联系方式）与提交端点 | **P3** |
| B14 | 静态 IP 申请、解绑、行内修改（§4.9） | `boundIpList.jsf` 的 `mainForm:delete`、申请静态 IP、行内修改 | 无写阶段；PrimeFaces 行选中 + partial submit | 录 source、行选中参数、分页 / 排序报文；解绑二次确认 | **P3** |
| B15 | 找回密码入口（账号 + 验证码 → 预留邮箱 / 手机发重置链路，§3.3） | 触屏 `/mselfservice/user/forgetPassword.jsf`；设备用户 `/selfservice/mail/devUserForget.jsf`（均匿名可达） | 登录页未挂入口 | 复核发送渠道（邮件 / 短信）、链接有效期、后续步骤；验证码人工识别 | **P3** |
| B16 | 访客自助：访客列表、访客预注册 | `/selfservice/mlogin.jsf`、`guestList.jsf`、`guestPreRegister.jsf` | 属设备管理用户 / 访客管理员角色（`usertype=2`），普通学生账号大概率无权限 | 先确认角色可见性；本期不接，远期按角色条件渲染 | **P3** |

---

## C. 8080 触屏自助门户（ICEfaces，设备中心）

> 后端：`src/core/DevicePortalClient.*`、`src/app/DeviceController.*`；门面 `app.portalDevices`。
> 依据：`PROTOCOL.md` §6.3–§6.5、`NAC-SELF-SERVICE.md` §4.8。已只读接入：`index.jsf`（在线 / 历史）、`macList.xhtml`（绑定终端）。

### C.1 只读类

| # | 可接按钮 / 展示位 | 来源字段 | 后端现状 | 前置依赖 | 分级 |
|---|---|---|---|---|---|
| C1 | 在线终端详情补字段：用户姓名、帐号名、用户 SSID、在线项服务名（§4.8 截图 1/2） | `onlineUserList.jsf` 列表 / 详情页 | 详情跳转骨架已落地：条目点击经 `DevicePanel → RemoteDevicesPage → 主栈索引 10` 进 `DeviceDetailPage.qml`；`parseOnline` 已取 deviceType/mac/ip/duration/startTime/endTime；`os`/`service`/`authStatus`/`lostStatus` 在线项置空；姓名、帐号名、SSID、服务名及主机名 / 节点密钥 / 版本等详情字段仍为「不可用」占位，未建模 | 确认列表与详情是同一 DOM 还是二次跳转；抓详情选择器；姓名为实名敏感信息，运行时展示可脱敏；字段产出后逐项接详情页占位槽位 | **P1**（字段补齐）；详情跳转 **✅ 骨架已落地** |
| C2 | 绑定 / 在线终端补绑定信息字段：端口号、无线 SSID、计算机名称、Windows 域（§4.3） | `bindInfo.jsf`、绑定行 | 绑定行已取 deviceType/os/service/startTime(绑定时间)/authStatus(无感知)/lostStatus(终端状态)；端口、SSID、主机名、域未取 | 抓对应标签文本；缺失留空不崩溃（容错已有） | **P2** |
| C3 | 绑定时间展示回归核对 | `macList.xhtml`（§6.3 标签 `boundTime`） | 后端 `parseBound` 已把「绑定时间」取入 `startTime`，卡片第三行已 join 展示；无新增解析工作 | 仅回归确认多语言 / 空态下展示正常 | **P1**（核对项） |

### C.2 写操作类（ICEfaces bridge，本期不实现，UI 给不可用提示）

> 机制约束（`PROTOCOL.md` §6.4）：写按钮走 ICEfaces bridge（`ice.mobi.button.select` / `jsf.ajax.request`），返回 partial-response XML；手写标准 `javax.faces.partial.ajax` 只能刷新 ViewState、不能触发业务动作。以下均需先录制真实浏览器回发报文。

| # | 可接按钮（远期） | 来源页面 / 字段 | 现状 / 约束 | 前置依赖 | 分级 |
|---|---|---|---|---|---|
| C4 | 增加终端：MAC、描述、无感知开关（§6.3 `macAdd.xhtml` 表单） | `/mselfservice/snac/macAdd.xhtml`（`mainForm:original` / `new` / `switch_hidden` / `onAdd`） | 无写阶段；MAC 前端正则三种写法、拒全 00 / 全 FF 广播 | 录 ICEfaces 提交报文；MAC 格式校验落 C++ 层；二次确认 | **P3** |
| C5 | 启用 / 禁用无感知认证 | 绑定行内开关（`status` 启用 / 禁用） | 无写阶段 | 录按钮 source 与状态切换报文 | **P3** |
| C6 | 置为丢失（`lostStatus` 正常 / 丢失） | 绑定行内操作 | 无写阶段；误置丢失影响后续接入，需强二次确认 | 录回发报文 | **P3** |
| C7 | 删除绑定终端 | 绑定行内删除（`panelConfirmation` 二次确认） | 无写阶段 | 录回发报文；删除前二次确认，默认不误删本机 | **P3** |
| C8 | 在线终端「清除在线信息」（远程下线某台本人在线设备） | 在线详情页按钮（§4.8） | **调研阶段禁止对当前在线设备触发**；§6.4 未发现门户提供在线踢线接口，待抓确认 | 先确认接口存在性；即便存在，仅限用户手动确认单台本人设备；与任务 #85 名额决策解耦 | **P3**（合规敏感，本期不推进） |

---

## D. 本机诊断 / 系统能力（非门户接口，对应 UI-SPEC-GAPS G2/G3/G6/G7）

> 后端：`src/app/NetworkInfoController.*`、`src/app/AppController::reportDiagnostic`；诊断页 `qml/pages/DiagnosticsPage.qml`、`qml/components/DiagnosticToolCard.qml`。
> 明细依据：`UI-SPEC-GAPS.md` G3 表。诊断结果只经 `reportDiagnostic` 入全局事件队列，QML 不能伪造业务事件。

| # | 工具 / 按钮 | 现状 | 平台约束 | 分级 |
|---|---|---|---|---|
| D1 | 多维度并行探测：DNS / DoH / DoT / TCP 握手 / HTTP 短请求，识别半连通（G3-1） | 仅 HTTP 在线探测雏形 | 跨平台；禁止只 ping / 只查 DNS | **P2** |
| D2 | 网卡型号 / MAC / 驱动版本，命中 AX201 弹指引（G3-2、G6） | 仅内核接口名 / IP / 掩码 / MAC；无友好名、型号、驱动；AX201 文字指引已有雏形 | 跨平台枚举；Windows 取友好名 / 型号 / 驱动 | **P2** |
| D3 | TCP Offload 检测 + 一键修复（Chimney / RSS / 自动调谐）（G3-3） | 未实现 | **仅 Windows**，`netsh`；非 Windows 隐藏入口 | **P3** |
| D4 | DNS 对比：内网 DNS vs 公共 `223.5.5.5` 解析延迟 / 异常（G3-4） | 未实现 | 跨平台 | **P2** |
| D5 | 路由追踪 tracert / mtr，可视化节点与丢包位置（G3-5） | 未实现 | 跨平台命令封装 | **P2** |
| D6 | LibreSpeed 测速：上下行 / 延迟 / 抖动，结果入日志（G3-6） | 未实现 | 跨平台；需对接自建测速节点 | **P3** |
| D7 | 编码探测 + 转换结果展示（G3-8、G4） | 后端 codec 探测雏形已有；诊断页未展示「探测到的编码 + 转换结果」 | 跨平台；AUTO 真正自动识别前不宣称已嗅探 | **P1**（接 UI 展示） |
| D8 | 默认网关读取并展示（G7） | 后端未读路由表，详情折叠网关暂显示「不可用」 | 跨平台取系统路由表 | **P1**（补本地字段） |
| D9 | 半连通（半残）后端状态判定（G2） | UI 已备琥珀 warning 色；后端无独立半连通状态字段，仅 Online/Offline/Authenticating/Error/Unknown | 探测层补短超时 TCP 应用层探测 + 大包 / 时延判定；UI 不臆造判定 | **P2** |

---

## E. 证书模块增强（`src/core/CertificateStore.*`，已落地基础能力）

> 现状：导入 PEM/DER、列表（subject / issuer / notBefore / notAfter / expired / fingerprint）、按指纹删除、持久化 `.pem`、变更后注入 `PortalClient`。以下为增强项。

| # | 按钮 / 展示位 | 现状 | 分级 |
|---|---|---|---|
| E1 | 证书详情查看：序列号、主题备用名（SAN）、公钥算法 / 密钥长度、有效期完整展示 | 列表项字段有限，无详情抽屉 | **P2** |
| E2 | 导出当前门户对端服务端证书，一键加入信任锚 | 仅支持从本地文件路径导入；未从现有 TLS 连接取对端证书 | **P3**（需在 TLS 握手成功后读 `QSslCertificate` 链） |
| E3 | 过期证书一键清理（按 `expired` 标记批量移除，二次确认） | 仅有逐条删除 | **P2** |

---

## F. CAS 融合门户（独立 SSO，远期方向，不混入本 NAC 客户端）

> 依据：`docs/CAS-PORTAL.md`。属另一套统一认证（`security.gdei.edu.cn/cas`、`iportal.gdei.edu.cn`、`jwxt.gdei.edu.cn`、`dsj.gdei.edu.cn`、`webvpn.gdei.edu.cn`）。2026-11-26 已匿名实测登录表单字段、RSA 加密、无图形验证码、校验端点 404 与子系统回调；同日在本人登录会话复核了票据跳转链、会话 Cookie、门户 `userLogin` 字段，并经 SSO 免密进入 jwxt 教务与 dsj 画像，拿到学院全称/专业方向/年级/班级/学籍状态等学籍字段。身份证/手机/考生号/宿舍等敏感项不纳入客户端采集。

| # | 方向 | 涉及接口（文档记录，门户侧已登录会话实测） | 口径 | 分级 |
|---|---|---|---|---|
| F1 | 当前用户资料 | 主资料 `tryLoginUserInfo`；补充 `api/upp/userControl/getLoginInfo`（返回 `strategy`/`headPortrait`/`guideUsed`/`firLogin`） | 独立 CAS 登录链路；不在本 NAC 客户端内接 | **P3 / 独立产品方向** |
| F2 | 菜单 / 权限 / 部门树 | `/api/authc/users/menus`、`/api/authc/users/<id>/authority`、`/api/authc/users/selector`、`/api/authc/departments*` | 同上；权限数据不与本客户端设备 / 账户状态混用 | **P3** |
| F3 | 消息 / 待办 / 日程 / 邮箱 / 应用商店 / 主题 / 附件等模块 | `api/uppinfo/infoCenter/*`、`api/uppmessage/message/*`、`api/uppflow/process/*`、`api/uppcard/affairCenter/*`、`api/bs-schedule/*`、`api/uppemail/*`、`api/upp/appStore/*`、`api/upp/theme/*`、`api/upp/attachmentManagement` | 远期若做融合门户再评估；本期仅登记，不建入口 | **P3** |
| F4 | 更换头像、新手引导标记 | `api/authc/users/portraitChange`（POST）、`api/upp/userControl/guideHasUsed`（POST） | 个人资料写操作；独立 CAS 会话，本期不接 | **P3** |
| F5 | 教务学籍字段：jwxt `GET /api/studentInfo/selectMyjwByid/<xsid>`（学院 `xymc`/专业方向 `zyfmc`/年级 `nj`/班级 `bjmc`/学籍状态 `xjztmc`/培养层次 `pyccmc`/校区 `xqmc`/学制 `xz`/入学日期 `rxrq` 等，§4.2）；dsj 画像 `queryserver.do`/`showreport.do`（§4.3） | 已登录会话实测字段名；jwxt 网关鉴权头、异动/注册/成绩 Tab 接口未补（`CAS-PORTAL.md` §9-1/2） | 身份展示字段以 jwxt 为准；身份证/手机/宿舍/生源地等高敏感项不采集；门户 `departments` 学生账号无权限 | **P3** |
| F6 | 校外 WebVPN 可达链路（登录、双因素、绑定邮箱 / 手机 / OTP / 微信、改密 / 重置） | `webvpn.gdei.edu.cn` Vue SPA 路由 `/auth/*`；代理 URL 改写规则未抓（`CAS-PORTAL.md` §5、§9-4） | 属网络代理范畴；本客户端合规边界不含代理 / 翻墙，仅作远期可行性记录 | **P3**（边界外，谨慎评估） |

---

## G. 应用自身跨平台能力（非接口）

| # | 按钮 / 能力 | 现状（需求编号） | 分级 |
|---|---|---|---|
| G1 | macOS / Linux 系统托盘菜单（显示 / 上线 / 下线 / 退出 + 在线状态图标） | 仅 WIN32 托盘（4.3.1 🟡） | **P2** |
| G2 | Android 前台服务、常驻通知、引导关闭电池优化 | 未实现（4.3.2 ❌） | **P2** |

---

## H. 汇总与建议落地顺序

### H.1 第一批（P1，后端链路已有，补解析 / 补本地字段即可）

1. **B1/B2/B3** 账户资料、费用、已申请服务解析补齐——登录链路与 UI 槽位已就绪，仅需一份已登录 `main.jsf` 样本补 `parseProfile` 即可上卡（样本取得前不臆造选择器）。
2. **A1** captive 成功报文扩展字段只读展示（端口 / 序列号 / 心跳周期 / 状态码 / 需改密）。
3. **C1/C3** 在线终端详情补姓名 / 帐号名 / SSID，绑定时间展示核对。
4. **B10** 在线数 / 剩余额度统计（只读本人额度）。
5. **D7** 编码探测结果接诊断页 UI；**D8** 默认网关读取。

### H.2 第二批（P2，需登录态抓样本或新增只读后端任务）

- 明细 / 账单 / 缴费记录 / 报修记录 / 静态 IP 只读列表（B4–B8）、公开 FAQ（B9）。
- 诊断工具 D1/D2/D4/D5/D9；本机字段补齐 C2；证书详情 E1、过期清理 E3；跨平台托盘 G1、Android 前台服务 G2。

### H.3 第三批（P3，写操作 / 独立体系，先录制报文或单独立项）

- 所有写操作：改资料 B11、改密 B12、报修提交 B13、静态 IP 申请 / 解绑 / 修改 B14、找回密码 B15、访客自助 B16；增加终端 C4、无感知开关 C5、置丢失 C6、删除绑定 C7、在线踢线 C8。
- captive 验证码分支 A4、服务类型下拉 A5；Offload 修复 D3、测速 D6；证书对端导出 E2。
- CAS 融合门户 F1–F6。

### H.4 待拍板口径（沿用历史审计，本轮不擅自决策）

1. 诊断工具集平台范围：先做 Windows 可用子集（`netsh` / `ping` / `tracert`），还是四平台一次性铺开（`UI-SPEC-GAPS.md` §四-3）。
2. 账户中心 / 自助门户写操作是否本期接入：依赖登录态抓包（真实 DOM、AJAX 端点、ICEfaces / PrimeFaces 回发报文），清单 §7 待补抓项见 `NAC-SELF-SERVICE.md` §7。
3. CAS 融合门户（个人信息 / 教务 / 消息 / 待办）是否纳入本 NAC 客户端，还是保持独立远期方向。
4. 在线踢线 / 名额释放（C8、任务 #85）：需产品规则另行定义，门户不暴露对端是否装本客户端 / 是否保活；合规敏感，不在本清单推进。

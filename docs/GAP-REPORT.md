# 需求差异报告 / Gap Report

核对日期：本地 CST(UTC+8) `2026-11-22 17:40:35` ｜ UTC/GMT `2026-11-22 09:40:35Z` ｜ UNIX `1795340435`
核对基准：上传的《gdei-connect Qt6 全栈开发硬性基线》规格文档
核对对象：仓库 `main`（HEAD `ab7ca36`）源码、CMake、GitHub Actions、README 与 docs，并含当前工作区未提交改动；旧规格基准提交 `ad660a1` 仅作历史对照。

## 状态图例 / Status Legend

- ✅ 符合：代码与规格一致
- 🟡 部分符合：有雏形但不完整，或实现方式与规格要求不同
- ❌ 未实现：代码中不存在对应能力
- ⚠️ 文档超前：README/兼容文档已宣称，但流水线或代码尚未产出

优先级：**P0** 安全红线 / 交付硬性基线，必须先修；**P1** 核心业务与可靠性；**P2** 工程规范与收尾增强。

---

## 一、P0 — 安全红线与交付硬性基线

### P0-1 证书校验被全局无条件关闭 ✅（本轮已修复）
- 规格：默认校验服务器证书；自签证书放行策略做成**可配置项**；禁止全局粗暴关闭校验。
- 现状：`ConfigStore` 新增 `allowInsecureSsl`（默认关，持久化键 `allow_insecure_ssl`）；`PortalClient` 的 `sslErrors` 槽仅在开关开启时 `ignoreSslErrors()` 并写警告日志，默认路径不忽略、握手错误经 `reply->error()` 上报。高级设置卡片已加对应开关。
- 遗留：放行时目前忽略全部 SSL 错误，后续可收窄为仅自签 / 主机名不匹配等可接受类型。

### P0-2 口令未走平台安全存储，密钥硬编码 ✅（Win/mac 已修复，Linux/Android 后备）
- 规格：账号密码经平台安全能力加密后本地持久化；密钥走 Keychain / DPAPI / Secret Service / Keystore；**禁止硬编码固定密钥**。
- 现状：新增 `SecretStore` 抽象与平台后端——Windows DPAPI（`CryptProtectData`/`CryptUnprotectData`，链接 advapi32）、macOS Keychain（Generic Password，链接 Security 框架）；`ConfigStore` 经 `SecretStore` 读写口令，并清理旧版 `password_ob` 遗留键。旧固定 XOR 的 `Crypto.*` 已删除。
- 遗留：Linux 仍为 QSettings Base64 后备（待 Secret Service/libsecret）；Android 待 Keystore；后端名在 `SecretStore::backend()` 显式标注，便于审计识别。

### P0-3 打包产物与规格基线不符 🟡（部分对齐，安装包矩阵待补）
- 规格产物：Windows exe 安装包、macOS dmg、Android apk、Linux deb / rpm / Arch `pkg.tar.zst`；打包脚本统一放 `packaging/`；CMake 输出按 `build/<platform>/<arch>` 组织；应用级运行依赖随包分发，OpenSSL 运行库随 Windows 包提供。
- 现状（本轮已对齐部分）：
  - 已新建 `packaging/` 目录：`windows/deploy.ps1`（windeployqt 收集后 zip）、`macos/deploy.sh`（macdeployqt + hdiutil 出 dmg）、`linux/deploy.sh`（二进制 tar.gz）；`release.yml` 改为调用这些脚本。
  - CI 矩阵扩为 Windows / macOS 原生 + Debian bookworm / Arch 容器；容器使用系统 Qt，Debian 由 CPack 出 `.deb`，Arch 出 tar.gz；Android 为 `continue-on-error` 实验性脚手架。
  - README / COMPATIBILITY 已区分“目标产物”与“当前 CI 产物”，去除超前声明。
- 遗留差距：
  - 构建输出仍固定 `build/`，未按 `build/<platform>/<arch>` 分层。
  - Windows 仍为 zip，未出 NSIS exe；Arch 未出 `pkg.tar.zst`（待 PKGBUILD）；rpm 未纳入容器；Android 仅脚手架，无可安装 apk。
  - 流水线未收集 OpenSSL 运行库；未做干净环境启动验证；macOS 未签名 / 未公证。
- 建议：
  1. release.yml 接入 CPack NSIS；Android 接入 androiddeployqt / Gradle 出正式 apk。
  2. Arch 补 PKGBUILD 出 `pkg.tar.zst`；Linux 包在对应发行版容器内生成，声明系统级依赖；应用级 Qt 库随包或合法分发。
  3. 补 OpenSSL 运行库收集步骤（Windows）与干净环境冒烟验证。

---

## 二、P1 — 核心业务与可靠性

### P1-1 掉线判定无连续失败阈值 ✅
- 规格：连续多次失败才判定掉线，仅掉线时触发登录；禁止单次失败即重登。
- 现状：`KeepAlive` 维护连续失败计数，成功探测清零；阈值（默认 3，可配置 1–10）内仅记警告，达到阈值才调用 `login()`。
- 遗留：暂无指数退避。主动下线与本地注销均先 `stopKeepAlive()`，不再在阈值后自动重登；主动离线与探测离线已区分。

### P1-2 探测间隔与基线不符 ✅
- 规格：检测间隔默认 **3 分钟，±30 秒随机抖动**（即 150–210 秒），防 Nginx 限流 403。
- 现状：常量默认 150–210 秒随机抖动；上下限纳入 `ConfigStore` 持久化，边界 30–600 秒，min/max 倒置时自动交换。

### P1-3 登录成功判定过于单一 🟡
- 规格：结合状态码、跳转目标、页面特征综合判定；不能仅凭 POST 返回 200 判定成功。
- 现状：登录与探测已叠加 2xx 状态码、空响应归类、NCSI 特征串与 JSON `errorNumber=="1"`；非 2xx / 空体单独归为失败。
- 遗留：未专门校验重定向落点目标页特征；JSON 解析失败仍回退展示原始报文。

### P1-4 通信编码未做动态探测 🟡
- 规格：编码探测优先级 HTTP `Content-Type` charset → HTML meta charset → 自动猜测；POST 表单使用探测到的服务端编码，并在 `Content-Type` 带对应 charset；允许集合 UTF-8 / GBK / GB2312 / Windows-1252 / ISO-8859-1。
- 现状：`PortalClient` 实现响应头 charset → `<meta charset>` 探测；UTF-8 / Windows-1252 / ISO-8859-1 经 `QStringDecoder` 解码。
- 遗留：GBK/GB2312 需接入 Qt5Compat `QTextCodec`（当前回退 UTF-8 并告警）；POST 表单仍按 UTF-8 编码，未按探测编码回写。

### P1-5 日志缺少敏感信息统一脱敏层 ✅
- 规格：密码、Cookie、token、MAC、IP 等敏感信息必须脱敏，禁止明文打印。
- 现状：`Logger::sanitize()` 在写入缓冲 / 面板 / 持久文件 / 导出前统一掩码：口令与 token 键值对、Cookie 头、MAC 地址、IPv4 后两段；规则集中在 Logger。
- 遗留：IPv6、Bearer 票据等模式未单独覆盖；脱敏规则未抽为常量配置。

### P1-6 桌面托盘 / 后台保活、Android 前台服务未实现 🟡（Windows 托盘已落地，其余平台待补）
- 规格：Windows/macOS/Linux 系统托盘 + 后台静默保活；Android 前台服务、常驻通知、引导关闭电池优化。
- 现状：Windows 已落地 `src/platform/TrayController.*`（`QSystemTrayIcon` 托盘图标、最小化隐藏、显示 / 上线 / 下线 / 退出菜单及在线状态图标），仅托盘壳引入 QtWidgets，主界面仍为 QML；macOS / Linux 托盘与 Android 服务 / 通知 / 电池优化引导代码尚未实现。
- 遗留与权衡：macOS / Linux 托盘待补；Android 前台服务待移动构建链。

### P1-7 设备管理门户：只读列表已实现，写操作未支持 🟡
- 规格（扩展）：读取本人在线设备列表（设备名 / MAC / IP / 在线时长 / 绑定状态）、当前在线状态与设备数量、剩余可登录设备数；仅在用户手动确认后下线本账号单台设备；禁止批量踢设备 / 绕过多端限制 / 访问他人数据。
- 抓包结论（2026-11-21，详见 `docs/PROTOCOL.md` 第 6 节）：
  - 自助门户与 captive portal 独立会话，登录需账号 + 口令 + 图形验证码 + 动态 ViewState；认证后 `GET /mselfservice/user/index.jsf` 直出在线用户 / 历史记录，`GET /mselfservice/snac/macList.xhtml` 为绑定终端列表，`/mselfservice/snac/macAdd.xhtml` 为增加终端页。
  - 在线 / 历史列表只读，字段含 时间、终端类型、MAC、IP、上网时长；绑定终端含 MAC、类型、操作系统、服务、绑定时间、无感知认证状态、终端状态。
  - **门户未提供在线会话踢线（强制下线）接口**；绑定终端的删除 / 开关 / 置丢失走 ICEfaces bridge 增量提交，手写标准 JSF ajax 无法触发，本期不实现。
- 现状：已新增 `DevicePortalClient` + `DeviceController` + `DeviceItem.qml` / `DevicePanel.qml`：独立会话、验证码登录、拉取在线 / 历史 / 绑定终端并按标签文本解析（不依赖 JSF 动态 id），列表只读展示；设备请求独立于核心登录保活。写操作（删除 / 开关 / 置丢失 / 增加）与在线踢线未实现；门户未提供设备名字段，在线数量统计与剩余可登录设备数未单独展示。
- 建议：在线“单台下线”在本门户无对应接口，回退为 captive `t=lo` 注销本机或在需求侧重新界定；写操作待录制真实浏览器 ICEfaces 回发报文后再定；后续补在线数量与剩余额度展示。

### P1-8 关键配置项未外置 🟡
- 规格：登录地址、门户地址、检测地址、轮询间隔、超时时间、证书放行策略等集中管理、可配置，不散落硬编码。
- 现状：`ConfigStore` 已持久化主机 / IP / NASIP、证书放行、保活上下限、失败阈值、连接 / 探测超时、NCSI 与健康检查地址，带默认值与边界校验。
- 遗留：高级配置暂无 QML 界面入口（仅配置文件 / 属性层可调）；部分系统级地址仍以常量为默认值。

### P1-9 登录门控形态：常驻导航 + 按需登录 🟡
- 规格（产品形态）：对标手机 APP / B 站游客态，导航栏常驻、不强制登录；未登录可浏览公开页，仅上线 / 下线 / 设备管理 / 资料费用明细等受保护操作才要求登录。
- 现状：主框架与底部 TabBar 常驻，不再按 `session.loggedIn` 二选一加载整页；登录改为模态弹层（遮罩 + 登录卡片 + "暂不登录"取消入口）；概览表盘未登录且离线时引导弹出登录层；主页提供独立上线 / 下线主操作，我的页未登录显示"上线"入口、登录后显示"注销"（本地退出本机账号并清除保存凭据，不主动断开 captive 连接）；captive 在线态、页面登录态、移动门户会话三套状态独立。
- 遗留：PC 自助门户资料 / 余额 / 费用 / 明细尚未接线（阶段 B），对应门禁待接入后补齐；远程设备页仍由门户会话内验证码表单门禁。

---

## 三、P2 — 工程规范与收尾增强

### P2-1 i18n 未走 Qt lupdate/lrelease 框架 🟡
- 规格：QML 文本用 `qsTr()`，C++ 文本用 `tr()`；走 lupdate/lrelease 翻译流程；预留简中 / 英文两套。
- 现状：`I18n.cpp` 自定义字典映射；QML 全部经 `app.i18n.text(key)` 取值（`Main.qml` 多处），未使用 `qsTr()`；C++ 中 `tr()` 与硬编码中英双串混用（如 `PortalClient.cpp:177/185/235`）。
- 说明：当前四语言（zh-CN / zh-TW / en-GB / ja）是规格“简中/英文预留”的超集，方向一致，但翻译机制不符合 Qt 规范。
- 建议：迁移到 `.ts/.qm` 资源；QML 改用 `qsTr()`，C++ 统一 `tr()`；字典作为初期文案来源迁入翻译源文件。

### P2-2 网络后端缺少可替换抽象 ❌
- 规格：优先 QNAM（HTTP/1.1、HTTP/2）；预留扩展接口，后续可接入 libcurl 实现 HTTP/3，上层业务不改动。
- 现状：`PortalClient` 直接 `new QNetworkAccessManager`（`PortalClient.cpp:20`），无后端抽象接口；README `:41` 宣称“预留扩展接口”，代码层未体现。
- 建议：定义 `INetworkBackend`（request / reply 信号 / cookieJar 注入），QNAM 为首个实现；后续 libcurl 实现替换后端而不动业务层。

### P2-3 CookieJar 仅内存态 🟡
- 规格：完整 CookieJar 会话管理，匹配 Nginx 会话校验、IP 绑定机制。
- 现状：`SessionCookieJar` 继承 `QNetworkCookieJar`，仅内存保存；重启后会话不恢复；无 IP 绑定 / 会话刷新的专门处理。
- 建议：评估会话持久化与到期刷新策略；IP 绑定变化时主动失效重登。

### P2-4 日志 / 配置文件编码未显式固定 UTF-8 🟡
- 规格：文件读写 QTextStream 强制 UTF-8，不跟随系统 locale。
- 现状：`Logger.cpp:46/66` 创建 QTextStream 但未显式 `setEncoding(QStringConverter::Utf8)`（Qt6 默认 UTF-8，但未按要求显式固定）。
- 建议：所有 QTextStream 显式设置 UTF-8；配置文件读写同样显式声明编码。

### P2-5 干净环境与实机验证缺失 🟡（Windows 已冒烟，其余平台待补）
- 规格：打包后在未安装 Qt 开发环境的 Win/macOS 可启动；Android 干净设备可安装启动；Linux 包依赖声明完整。
- 现状：Windows 已在本地全量编译，windeployqt 收集 Qt/QML/平台插件，自包含目录经 3 秒存活冒烟（进程不崩溃、窗口可打开）；macOS 未签名 / 未公证、未在干净环境启动验证；Android 仅脚手架 / Debug APK，未在干净真机安装启动；Linux 未在对应发行版容器验证依赖声明；Runner 仅出 x64 / arm64 单架构。
- 建议：CI 增加最小依赖安装后的启动冒烟测试；补充各底线系统 / 架构实机清单；记录未打包的系统级依赖。

### P2-6 可复用组件缺口 🟡
- 规格优先产出：卡片、输入框、按钮、开关、下拉选择、设备列表项。
- 现状：已有 `Card`、`TextField`、`PrimaryButton`、`Switch`、`LangSelector`（ComboBox 下拉）、`LogPanel`、`DeviceItem` / `DevicePanel`（随 P1-7 补齐）；设备列表项已具备，剩余为列表项在多选 / 批量场景下的交互增强。

### P2-7 第三方依赖与许可台账缺失 🟡
- 规格：新增依赖记录许可信息；交付前核对 Qt LGPL 及第三方库许可；不引入强传染协议组件。
- 现状：当前无第三方库引入；仓库有 LGPL-3.0 LICENSE；但打包一旦带入 OpenSSL / Qt 动态库 / 其他运行库，缺少许可义务清单（动态链接声明、源码提供方式等）。
- 建议：建 `docs/THIRD-PARTY-NOTICES.md`，登记随包分发的库、版本、许可与义务。

### P2-8 异常分支与验证码覆盖 🟡
- 规格：覆盖网络超时、403、登录失败、证书异常、权限异常、门户接口不可用，并给友好提示。
- 现状：403 有专门文案；超时 / 证书 / 权限 / 门户不可用分支不完整；图形验证码分支未处理（`PROTOCOL.md:84` 已注明）。
- 建议：补全错误码到友好文案的映射表；验证码分支至少给出明确提示与手动恢复路径。

---

## 四、已符合项（保持，勿回退）

- 单一源码仓库、单一 CMake 工程，固定 Qt6 / C++20 / Quick Controls 2，无 Qt5 分支。
- 未引入 Qt Widgets 主界面、CEF / WebView / Node.js / 浏览器内核（源码 grep 无命中）。
- 分层清晰：QML 只经 `app.*` 调用 C++ 接口，QML 内未持有 `QNetworkAccessManager` 或定时器（`Main.qml`）。
- 未使用 `fromLocal8Bit` / `toLocal8Bit` 等依赖系统 locale 的转换（源码 grep 无命中）。
- 表单字段名、门户地址、默认值、超时常量集中在 `Constants.hpp`，业务代码无魔法字符串。
- 代码注释中英双语；敏感词扫描无命中，明文凭据 / token 未入库。
- 网络层基于 QNAM；CookieJar 已接入 NAM；CSRF 提取、Base64+URL 解码+JSON 解析链路已搭起。
- 分级日志面板（DEBUG/INFO/WARN/ERROR）+ 内存缓冲 + 文件持久化 + 导出入口已具备。
- 主框架与底部导航常驻，登录为按需模态弹层；未登录可浏览公开页，受保护操作再引导登录；captive 在线态、页面登录态、门户会话三套状态相互独立。

---

## 五、建议路线 / Recommended Roadmap

**第一批（P0，安全与交付底线）**
1. 证书校验策略改为可配置开关，默认严格校验。
2. 抽象并接入平台密钥存储，移除固定 XOR 密钥。
3. 对齐打包基线：`packaging/` 目录、CPack / 官方部署工具落地、产物类型与 README 声明一致；先修正文档超前表述。

**第二批（P1，核心可用）**
4. 保活连续失败阈值 + 间隔改为 150–210 秒并可配置。
5. 登录成功多条件判定；通信编码动态探测。
6. 日志统一脱敏；配置项外置。
7. 桌面托盘 / 后台驻留；Android 前台服务与电池优化引导。
8. 设备门户读取 + 设备列表模型与组件 + 确认后单台下线。

**第三批（P2，规范收尾）**
9. i18n 迁移 lupdate/lrelease；网络后端抽象；Cookie 持久化。
10. 显式 UTF-8、干净环境冒烟验证、DeviceItem 组件、第三方许可台账、异常分支补全。

# 需求差异报告 / Gap Report

核对日期：2026-11-21
核对基准：上传的《gdei-connect Qt6 全栈开发硬性基线》规格文档
核对对象：仓库 `main`（提交 `ad660a1`）源码、CMake、GitHub Actions、README 与 docs

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
  - README / COMPATIBILITY 已区分“目标产物”与“当前 CI 产物”，去除超前声明。
- 遗留差距：
  - 构建输出仍固定 `build/`，未按 `build/<platform>/<arch>` 分层。
  - Windows 仍为 zip，未出 NSIS exe；Linux 仍为裸二进制 tar.gz，未出 deb / rpm / Arch zst；无 Android APK。
  - 流水线未收集 OpenSSL 运行库；未做干净环境启动验证；macOS 未签名 / 未公证。
- 建议：
  1. release.yml 接入 CPack（NSIS / DragNDrop / DEB;RPM），在带对应工具链的 Runner 上出安装包；Android 接入 androiddeployqt / Gradle。
  2. Linux 包在对应发行版容器内生成，声明系统级依赖；应用级 Qt 库随包或合法分发。
  3. 补 OpenSSL 运行库收集步骤（Windows）与干净环境冒烟验证。

---

## 二、P1 — 核心业务与可靠性

### P1-1 掉线判定无连续失败阈值 ❌
- 规格：连续多次失败才判定掉线，仅掉线时触发登录；禁止单次失败即重登。
- 现状：`src/core/KeepAlive.cpp:58-63`，`online == false` 立即 `m_portal->login()`；无失败计数、无退避。
- 建议：引入连续失败阈值（如 2~3 次，可配置），计数器在成功探测后清零；阈值内只记录警告，达到阈值才进入重登。

### P1-2 探测间隔与基线不符 🟡
- 规格：检测间隔默认 **3 分钟，±30 秒随机抖动**（即 150–210 秒），防 Nginx 限流 403。
- 现状：`Constants.hpp:80-81` `KEEPALIVE_MIN_SEC=45`、`KEEPALIVE_MAX_SEC=90`，45–90 秒，频率高于规格要求。
- 建议：常量改为 150 / 210，并纳入可配置项。

### P1-3 登录成功判定过于单一 🟡
- 规格：结合状态码、跳转目标、页面特征综合判定；不能仅凭 POST 返回 200 判定成功。
- 现状：`PortalClient.cpp:146-171` `parsePayload` 仅 Base64+URL 解码后看 JSON `errorNumber=="1"`；未检查 HTTP 状态码、重定向目标、页面特征串。
- 建议：成功条件叠加：状态码 2xx、返回体特征（JSON 字段 / 页面关键字）、必要时跟随重定向后的落点；空响应与错误文案分别归类。

### P1-4 通信编码未做动态探测 ❌
- 规格：编码探测优先级 HTTP `Content-Type` charset → HTML meta charset → 自动猜测；POST 表单使用探测到的服务端编码，并在 `Content-Type` 带对应 charset；允许集合 UTF-8 / GBK / GB2312 / Windows-1252 / ISO-8859-1。
- 现状：`PortalClient.cpp:75` 固定 `charset=UTF-8`；读取全部 `fromUtf8`，表单 `toUtf8`；无 meta 解析、无编码猜测、无 GBK 等分支。
- 建议：在 PortalClient 增加编码探测环节（响应头 → `<meta charset>` → 猜测），保存当前编码供回写表单与解码使用。

### P1-5 日志缺少敏感信息统一脱敏层 ❌
- 规格：密码、Cookie、token、MAC、IP 等敏感信息必须脱敏，禁止明文打印。
- 现状：`Logger.cpp` 只做级别分级与落盘，无脱敏过滤器；当前消息体未主动带入口令/Cookie，但缺少防护层，后续接口（设备列表等）容易回归出明文。
- 建议：Logger 写入前统一过 `sanitize()`，对密码、Cookie 值、token、MAC、IPv4/IPv6 做掩码；脱敏规则集中在常量配置。

### P1-6 桌面托盘 / 后台保活、Android 前台服务未实现 ❌ / ⚠️
- 规格：Windows/macOS/Linux 系统托盘 + 后台静默保活；Android 前台服务、常驻通知、引导关闭电池优化。
- 现状：源码无 `QSystemTrayIcon`、无托盘菜单、无后台驻留入口；无 Android 服务 / 通知 / 电池优化引导代码。README `:36`、`docs/COMPATIBILITY.md:31/38/45/53` 已宣称上述能力。
- 建议：桌面侧补托盘图标、显示/隐藏主窗口、退出、状态气泡；Android 侧补前台服务与通知渠道、电池优化引导页；文档先降级为规划项。

### P1-7 设备管理门户扩展功能未实现 ❌
- 规格（扩展）：读取本人在线设备列表（设备名 / MAC / IP / 在线时长 / 绑定状态）、当前在线状态与设备数量、剩余可登录设备数；仅在用户手动确认后下线本账号单台设备；禁止批量踢设备 / 绕过多端限制 / 访问他人数据。
- 现状：`HEALTH_URL`（`macList.xhtml`）仅在 `Constants.hpp:30-31` 定义，`PortalClient` 未请求该门户；无设备数据模型、无设备列表 QML 组件、无下线确认流程。
- 建议：新增 `DevicePortalClient`（复用会话 Cookie）+ `DeviceInfo` 模型 + `DeviceItem.qml`；字段缺失 / 页面改版 / 结构变化时降级展示“设备状态暂不可用”，不影响核心登录保活；下线动作必须二次确认。

### P1-8 关键配置项未外置 ❌
- 规格：登录地址、门户地址、检测地址、轮询间隔、超时时间、证书放行策略等集中管理、可配置，不散落硬编码。
- 现状：`ConfigStore` 仅持久化 username / password / remember / autoLogin / lang / host / ip / nasip；超时（`CONNECT_TIMEOUT_MS` 10000、`PROBE_TIMEOUT_MS` 6000）、保活间隔、证书策略、NCSI/健康地址均为编译期常量，界面与配置文件不可调。
- 建议：ConfigStore 增补超时、间隔、抖动范围、证书放行、探测地址等字段，带默认值与边界校验。

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

### P2-5 干净环境与实机验证缺失 ❌
- 规格：打包后在未安装 Qt 开发环境的 Win/macOS 可启动；Android 干净设备可安装启动；Linux 包依赖声明完整。
- 现状：本机无 Qt / 编译器，源码未经本地编译；CI 仅在带 Qt 的 Runner 构建；无干净环境启动验证；macOS 未签名 / 未公证；Runner 仅出 x64 / arm64 单架构。
- 建议：CI 增加最小依赖安装后的启动冒烟测试；补充各底线系统 / 架构实机清单；记录未打包的系统级依赖。

### P2-6 可复用组件缺口 🟡
- 规格优先产出：卡片、输入框、按钮、开关、下拉选择、设备列表项。
- 现状：已有 `Card`、`TextField`、`PrimaryButton`、`Switch`、`LangSelector`（ComboBox 下拉）、`LogPanel`；**缺设备列表项 `DeviceItem.qml`**（随 P1-7 一起补）。

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

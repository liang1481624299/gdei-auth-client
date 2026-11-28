<div align="center">

<img src="assets/app.png" width="96" alt="GDEI Connect 图标" />

# GDEI Connect · 校园内网 P2P 协作客户端

面向校内二层局域网的**纯去中心化、无服务器、无数据库**跨平台校园网工具 + 内网 P2P 协作平台：基于 Qt6 + C++20 + QML，覆盖 NAC 认证保活、内网设备发现、分级可见性与点对点文件协作。

[![CI](https://github.com/liang1481624299/gdei-auth-client/actions/workflows/ci.yml/badge.svg)](https://github.com/liang1481624299/gdei-auth-client/actions/workflows/ci.yml)
![Qt 6 LTS](https://img.shields.io/badge/Qt-6%20LTS-green)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![QML](https://img.shields.io/badge/UI-QML%20·%20Quick%20Controls%202-teal)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Win%20·%20macOS%20·%20Linux%20·%20Android-lightgrey)
![i18n](https://img.shields.io/badge/i18n-4%20langs-yellow)
![Status](https://img.shields.io/badge/Status-Active-green)
![License](https://img.shields.io/badge/License-LGPL--3.0-red)

[README](#readme) · [Contributing](https://github.com/liang1481624299/gdei-auth-client/issues) · [LGPL-3.0 license](LICENSE)

[开发规范](#开发规范) · [本地体验](#快速开始) · [功能介绍](#功能介绍) · [技术架构](#技术架构) · [快速开始](#快速开始) · [配置说明](#配置说明) · [使用流程](#使用流程) · [文档导航](#文档导航) · [许可证](#许可证)

</div>

---

<a id="readme"></a>

## 简介

**一句话定位**：在不依赖任何中心服务器、中心数据库与全校花名册的前提下，把同校已认证设备变成可发现、可分级访问、可点对点协作的内网节点，服务校园网登录保活与本地文件协作场景。

平台由三部分组成：

- **C++ 核心层（`src/core`）**：门户协议客户端、会话 CookieJar、表单/报文库编解码、配置与加密持久化、保活探测、分级日志、四语言字典、验证码 OCR 桥接。
- **应用控制器层（`src/app`）**：认证态 / 会话态 / 设备 / 三源合并 / 内网发现 / 诊断等控制器，经 Qt 信号槽与 `QObject` 接口向 QML 暴露能力。
- **内网与平台层（`src/net`、`src/platform`）**：心跳/组播发现、邻居表与 OUI/SSDP/NetBIOS 名称补全、网卡选择、主机路由绑定与系统托盘。

> 信任根唯一依赖学校官方 NAC 门户签名身份；客户端不自建用户库、不缓存全校学生信息。仅校内二层 LAN 启用完整 P2P 能力，VPN/隧道网段与公网一律隔离。

## 开发规范

- **架构分层**：C++ 负责业务、网络与底层系统能力；QML 只做界面交互与状态展示，不写业务逻辑。平台差异仅用 Qt 内置跨平台宏收敛。
- **语言与注释**：源码注释中英双语，便于学校信息中心团队阅读维护；界面内置简体中文、繁體中文、English (UK)、日本語四语言。
- **构建方式**：单一源码、单一 CMake 工程，固定 Qt6，不维护 Qt5 分支；CLI 优先，GUI 为可选壳层。
- **提交规范**：Conventional Commits 单行中文格式 `type(scope): 中文简述`（`type` 如 `fix/docs/feat/refactor`），不加多行正文与句号。
- **凭据边界**：真实账号口令不得提交仓库；仓库内不保存任何明文凭据、Token 或 API Key；keystore 仅经 CI Secrets 注入。
- **去中心化取舍**：采用 P2P 最终一致性；离线设备走异步吊销 + 凭证 TTL 双兜底；不做公网打洞、UPnP/NAT-PMP/PCP 端口映射与公网中继。

## 功能介绍

### 校园网基础能力

- 上线 / 下线 / 状态查询：GET 登录页提取 CSRF → 携带完整请求头（UA / Referer / Cookie）提交表单 → CookieJar 维持会话。
- **自动保活**：按带随机抖动的间隔探测外网连通性，仅掉线时触发重连，避免固定高频轮询被限流 403。
- **网络环境判定**：本机自动区分「校内真实 LAN / VPN 隧道 / 外网」，仅在合规校内网卡存在时拉起发现引擎；隧道/虚拟/远程共享网卡在取地址、发心跳、收包各环节全部排除。
- **在线设备面板**：门户设备列表 × 内网发现 × 本地台账（按 MAC 记忆）三源合并展示，未装客户端的节点按 OUI / 端口 / NetBIOS 尽力补全，拿不到名字显示「未知」，非本客户端节点统一标「非可信」。
- 分级日志面板（DEBUG / INFO / WARN / ERROR），支持导出日志文件；Windows 系统托盘支持显示主窗口、上线、下线、退出。

### 去中心化内网发现

- mDNS 轻量心跳广播 + 手动 IP 直连兜底（防 VLAN 组播隔离）；心跳 30s、超时 90s 清理，不做全网扫描、不引发 ARP 风暴。
- 仅发现已认证、同 APP 的合法校内节点；探测只针对已发现主机，端口表固定且很小。
- 节点互报昵称、主机名、班级 ID、系统与多网卡 MAC；跨重启记忆只落在本机台账，无服务端存根。

### 验证码 OCR（本地离线）

- 账户自助页 / 设备面板两套验证码链路共用一个 `OcrBridge`：`QProcess` 拉起常驻 CPython 子进程，stdin/stdout 行分隔 JSON 通信，ddddocr 在本机 onnxruntime CPU 推理。
- 识别结果**只回填输入框、不自动提交**；图片不上传、不访问外网；缺运行时或缺模块自动退回手输；Android 编译为不可用桩。
- 高级设置页可开关 OCR（配置键 `ocr_enabled`，默认开）。

### 规划中能力（设计已落档，未完全落地）

- **三级去中心化权限**：仅本人可见 / 同班可见 / 全校内网可见，全部点对点实时比对、无中心查表。
- **P2P 文件传输**：4MB 固定分片、临时文件缓存、断点续传、SHA256 分片与整包校验，受控沙箱目录浏览与主动拉取。
- **异步吊销模型**：在线节点直连下发登出；离线节点靠本机根签名吊销声明 + Gossip 黑名单同步，下次上线握手时强制退号；凭证自带 TTL 到期兜底失效。

## 技术架构

```
gdei-auth-client/
├── CMakeLists.txt              # 单一 CMake 构建；固定 Qt6、C++20
├── src/
│   ├── main.cpp                # 入口：装配核心对象并注入 QML 根上下文
│   ├── core/                   # 业务 / 网络 / 持久化 / 加密 / 系统能力
│   │   ├── Constants.hpp       #   门户地址、表单字段、默认值、超时与退避常量
│   │   ├── PortalClient.*      #   QNAM 门户认证客户端（CSRF / 表单 / 解码）
│   │   ├── DevicePortalClient.*# 移动 ICEfaces 自助门户：验证码会话 + 设备只读列表
│   │   ├── AccountPortalClient.*# PC PrimeFaces 自助门户：验证码会话 + 资料占位
│   │   ├── SessionCookieJar.*  #   完整 Cookie 会话管理
│   │   ├── SecretStore.*       #   平台安全存储：Windows DPAPI / macOS Keychain（Linux/Android 后备）
│   │   ├── ConfigStore.*       #   QSettings 持久化配置
│   │   ├── KeepAlive.*         #   随机间隔探测，掉线才重登
│   │   ├── OcrBridge.*         #   Python 子进程侧车桥接（行 JSON 协议 + 超时/降级）
│   │   ├── LanAssetStore.*     #   按 MAC 的本地设备台账（QSettings，无服务端）
│   │   └── I18n.*              #   四语言字典与切换
│   ├── app/                    # 应用层控制器，向 QML 暴露接口
│   │   ├── AuthController.*    #   captive 在线态：门户客户端 + 保活
│   │   ├── SessionController.* #   页面登录态：手动 / 自动认证 + 按需登录弹层
│   │   ├── LanDiscoveryController.* # 内网发现状态与节点列表门面
│   │   ├── DeviceMergeController.*  # 门户 × 发现 × 台账三源合并
│   │   ├── NetworkInfoController.*  # 本机网卡 / 网络环境判定
│   │   └── AppController.*     # 聚合 config / i18n / auth / session / devices 暴露给 QML
│   ├── net/                    # 内网发现与名称补全
│   │   ├── LanDiscovery.*      #   心跳广播 / 组播提问 / 邻居表维护
│   │   ├── NeighborCache.*    #   被动读取本机邻居表，不做网段 sweep
│   │   ├── OuiDatabase.*       #   MAC 厂商补全
│   │   ├── SsdpProbe.*         #   固定小端口表的单主机探测
│   │   └── NetbiosName.*       #   NetBIOS 名称补全
│   └── platform/               # 平台适配
│       ├── RouteBinder.*        #   门户流量主机路由 / 源地址绑定
│       └── TrayController.*    #   系统托盘（Windows 落地；macOS/Linux 预留）
├── qml/                        # 界面层：页面、主题令牌与可复用组件
├── scripts/                    # 免编译参考脚本 + Python OCR 侧车（不参与 CMake 构建）
├── packaging/                  # 各平台部署打包脚本收口
├── android/                    # Qt for Android 清单、图标与网络安全配置
└── docs/                       # 协议、设计、兼容矩阵与审计文档
```

**分层约定**：C++ 负责业务、网络与底层系统能力；QML 只做 UI 交互。跨平台差异集中在 `src/platform` 与 Qt 宏内收敛，不在界面层散落条件分支。

## 快速开始

### 依赖

- CMake ≥ 3.16
- Qt 6 LTS，组件：`Core` `Quick` `QuickControls2` `Network` `Qml` `Svg` `Core5Compat`；Windows 托盘另需 `Widgets`
- 编译器支持 C++20（MSVC 2019+ / GCC 11+ / Clang 13+ / Xcode 13+）

### 一键构建（推荐，自动部署工具链并收编运行时）

各平台一键脚本会自动准备便携工具链，并把 Qt 运行时依赖收集到产物旁，保证双击即开（修复历史版本「只出裸二进制、干净机器打不开」的问题）。

| 平台 | 脚本 | 产物 |
|---|---|---|
| Windows | `scripts/build-windows.ps1` | `artifacts/windows/gdei-auth-client/`（自包含目录）+ `.zip` |
| macOS | `scripts/build-macos.sh` | `artifacts/macos/gdei-auth-client-macos.dmg` |
| Linux（Debian / Arch） | `scripts/build-linux.sh` | `artifacts/arch/gdei-auth-client-linux-x64/` + `.tar.gz`（目标机无需装 Qt） |
| Android | `scripts/build-android.ps1` | 本地出单 ABI debug APK；CI / Release 出双 ABI 合并包，按签名状态命名 signed / unsigned。需 Qt for Android + SDK/NDK + JDK17 |

Windows 示例（项目根目录，PowerShell）：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

脚本会把便携 Python / CMake / Ninja 装到 `%USERPROFILE%\.cache\gdei-toolchain`，用 aqt 拉取 Qt6 + MinGW，编译后自动运行 `windeployqt`。

### 手动配置与构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

打包脚本收口在 [`packaging/`](packaging/README.md)；CI 在 tag 推送时按平台归档并挂到 GitHub Release。

## 配置说明

配置通过 `QSettings` 持久化（组织名 `GDEI`，应用名 `gdei-connect`），界面「保存」按钮写入磁盘：

| 配置键 | 说明 |
|---|---|
| `username` | 账号 |
| `remember_password` | 记住密码开关；开启时口令经平台安全存储落盘 |
| `auto_login` | 自动登录开关（启动且凭据齐全时自动上线） |
| `lang` | 界面语言：`0` 简体中文 / `1` 繁體中文 / `2` English / `3` 日本語 |
| `theme_mode` | 主题模式（跟随系统 / 浅色 / 深色） |
| `host` | 门户域名（默认 `nac.gdei.edu.cn`） |
| `ip` | 备用直连 IP（默认 `210.38.64.39`） |
| `nasip` | NAS / AC 地址（默认 `192.168.100.8`） |
| `allow_insecure_ssl` | 显式放行自签等异常证书；默认严格校验，开启后写警告日志留痕 |
| `keepalive_min_sec` / `keepalive_max_sec` | 保活探测随机抖动区间 |
| `offline_fail_threshold` | 判定离线前的连续失败次数 |
| `connect_timeout_ms` / `probe_timeout_ms` | 连接 / 探测超时 |
| `probe_url` / `health_url` | 连通性探测地址 |
| `request_encoding` | 认证 POST 表单编码：`AUTO` / `ASCII` / `GBK` / `UTF8`，默认 `AUTO`；界面、翻译、配置与日志始终使用 UTF-8 |
| `discover_nickname` | 发现页对外展示的节点昵称 |
| `discover_auto_start` | 上线后是否自动启动内网发现 |
| `ocr_enabled` | 验证码本地 OCR 回填开关（默认开） |

> 口令只在「记住密码」开启时落盘；Windows 走 DPAPI，macOS 走 Keychain，Linux / Android 为明确标注的后备实现。

## 使用流程

1. **登录**：在登录页输入账号与口令（或开启自动登录）。自助门户图形验证码可由本地 OCR 回填，**人工核对后再手动提交**；OCR 不可用时退回手输。
2. **保活在线**：上线后保活控制器按随机抖动间隔探测，掉线才重连；托盘可快速上线 / 下线 / 退出。
3. **浏览设备**：设备页展示门户、内网发现与本地台账合并后的节点；「发现」页查看同网段在线节点，按全部 / 可信 / 非可信过滤。
4. **内网协作（规划中）**：在受控沙箱目录内浏览对方共享、按分片拉取文件；权限与吊销模型见 [P2P-TRANSFER-PLAN.md](docs/P2P-TRANSFER-PLAN.md)。

## 兼容矩阵

> 硬性基线：固定 Qt6，不维护 Qt5 分支。

| 平台 | 架构 | 版本底线 | 备注 |
|---|---|---|---|
| Windows | x64 / ARM64 | Windows 10 1509 ~ Windows 11 25H2 | zip 自包含包；ARM64 走实验性构建；Qt6 不支持 32 位 x86 |
| macOS | x64（Intel）/ ARM64（Apple Silicon） | macOS 10.15 Catalina 及以上 | dmg 打包，x64 与 ARM64 分别构建 |
| Ubuntu | x64 / ARM64 | 24.04 (noble) 及以上 | 原生 runner 系统 Qt 构建 |
| Android | arm64-v8a / x86_64 | Android 9（API 28）及以上 | CI 双 ABI 合并包；真机安装与交互验证仍待补 |
| Debian | x64 | bookworm 及以上 | 容器内 CPack 出 `.deb` |
| Arch Linux | x64 | 滚动发行，内核 5.0+ | 容器内 tar.gz；`pkg.tar.zst` 待 PKGBUILD |

详细工具链与打包说明见 [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md)。

## 安全与边界

- **无中心依赖**：不建用户数据库、不缓存全校学生信息；所有信任与权限点对点本地校验、点对点同步。
- **VPN 隔离**：VPN 模式只保留网页自助管理；禁用全部 P2P、发现与文件共享，杜绝外网渗透。
- **网络行为合规**：邻居表走被动读取，不做网段 sweep、不全端口扫描；探测只针对已发现主机；ARP 纯被动读取。
- **OCR 本地离线**：识别不出校、不传图；结果只回填不自动提交。
- **证书与凭据**：默认严格校验服务器证书；代码不使用硬编码固定密钥，真实凭据不入库。

## 文档导航

- [PROTOCOL.md](docs/PROTOCOL.md)：抓包协议逆向说明、时序与表单字段。
- [CAS-PORTAL.md](docs/CAS-PORTAL.md)：CAS 外壳 / 登录表单 / RSA 加密 / 子系统回调链路与匿名实测结论。
- [LAN-DISCOVERY.md](docs/LAN-DISCOVERY.md)：内网设备发现设计、心跳 v2 协议、字段清单、探测边界与实测记录。
- [NIC-BINDING-DESIGN.md](docs/NIC-BINDING-DESIGN.md)：多网卡校园网识别标注与门户主机路由 / 源地址绑定设计。
- [P2P-TRANSFER-PLAN.md](docs/P2P-TRANSFER-PLAN.md)：内网文件传输开发计划，以及隧道 / 打洞 / 端口映射必要性评估与边界结论。
- [OCR-CAPTCHA-DESIGN.md](docs/OCR-CAPTCHA-DESIGN.md)：OCR 侧车协议、降级链、打包约束与安全边界。
- [COMPATIBILITY.md](docs/COMPATIBILITY.md)：兼容平台矩阵、Qt6 工具链与打包明细。
- [THIRD-PARTY-NOTICES.md](docs/THIRD-PARTY-NOTICES.md)：应用校徽与各平台品牌图标的来源、授权与商标归属。
- [troubleshoot.md](docs/troubleshoot.md)：校园网常见异常现象、根因与自查排查步骤。
- [AUDIT-2026-11-28.md](docs/AUDIT-2026-11-28.md)：本轮健康度审计、已修复项与剩余扣分项。

## 许可证 / License

**LGPL-3.0**。不引入 GPL 强传染依赖；使用方需遵守 LGPL 关于动态链接与源码提供的要求。应用内平台品牌图标仅作兼容标识，相关商标归各自厂商所有，详见第三方声明。

完整许可证文本见 [LICENSE](LICENSE) 与 <https://www.gnu.org/licenses/lgpl-3.0.html>。

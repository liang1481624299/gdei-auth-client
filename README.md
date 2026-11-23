<div align="center">

<img src="assets/app.png" width="96" alt="GDEI 校徽" />

# gdei-connect · GDEI 校园网自动认证客户端

面向广二师校园网（H3C iMC Portal）的跨平台自动认证客户端 —— **固定 Qt6 + QML + C++20，单一源码、单一 CMake**，内置四语言、掉线自动重连、系统保活与分级日志。

[![CI](https://github.com/liang1481624299/gdei-auth-client/actions/workflows/ci.yml/badge.svg)](https://github.com/liang1481624299/gdei-auth-client/actions/workflows/ci.yml)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![Qt6](https://img.shields.io/badge/Qt-6%20LTS-green)
![QML](https://img.shields.io/badge/UI-QML%20·%20Quick%20Controls%202-teal)
![Platform](https://img.shields.io/badge/Platform-Win%20·%20macOS%20·%20Linux%20·%20Android-lightgrey)
![i18n](https://img.shields.io/badge/i18n-4%20langs-yellow)
![License](https://img.shields.io/badge/License-LGPL--3.0-red)

[简介](#简介) · [功能](#功能) · [技术架构](#技术架构) · [快速开始](#快速开始) · [配置](#配置) · [多语言](#多语言) · [兼容矩阵](#兼容矩阵) · [持续集成与发布](#持续集成与发布--ci--release) · [文档](#文档)

</div>

---

## 简介

**gdei-connect** 是一套桌面 / 移动跨平台的校园网自动认证客户端，覆盖上线、下线、状态查询与掉线自动重连。核心是一套基于 `QNetworkAccessManager` 的 Portal 协议内核：GET 登录页提取 CSRF、携带完整请求头（UA / Referer / Cookie）提交表单、完整 CookieJar 会话管理、返回报文 Base64 + URL 解码 + JSON 解析，全部自主实现，不引入浏览器内核或重型第三方 GUI 框架。

项目结构严格分层：

- **C++ 层（`src/core`、`src/app`）**：业务流程、网络请求、会话与 Cookie、加密持久化、保活探测、日志与国际化，全部在 C++ 完成，经 Qt 信号槽 / `QObject` 接口暴露给 UI。
- **QML 层（`qml/`）**：只负责界面交互与状态展示，不写业务逻辑；可复用组件包括卡片、输入框、主按钮、开关、语言选择器与日志面板。
- **参考脚本（`scripts/`）**：Python / PowerShell / bat / shell 免编译参考实现，不参与 CMake 构建。

## 功能

- 上线 / 下线 / 状态查询，登录流程：GET 登录页 → 解析 HTML 提取 CSRF token → 带 UA / Referer / Cookie POST 表单
- **导航常驻、按需登录**：未登录可浏览概览 / 设备 / 我的等公开页；上线、下线、设备管理等受保护操作才弹出登录层，可取消返回浏览；captive 在线态、页面登录态、门户会话三套状态相互独立、分别登出
- **完整 CookieJar 会话管理**，匹配 Nginx 会话校验与 IP 绑定机制
- **自动保活**：定时探测外网连通性，仅掉线时才触发登录；探测间隔带随机抖动，避免固定高频轮询被 Nginx 限流 403
- **Windows 系统托盘**：最小化到托盘，支持显示主窗口、上线、下线、退出，图标与提示跟随在线状态；保活沿用现有控制器。macOS / Linux 托盘、Android 前台服务与电池优化引导仍为规划项
- 账号密码经平台安全存储（Windows DPAPI / macOS Keychain）持久化，支持记住密码、自动登录开关；Linux / Android 为明确标注的后备实现
- 内置分级日志面板（DEBUG / INFO / WARN / ERROR），支持导出日志文件
- **四语言界面**：简体中文、繁體中文、English (UK)、日本語，顶部下拉即时切换
- 错误处理覆盖网络超时、403 限流、登录失败、证书异常与权限异常，并给出友好提示
- 网络层以 `QNetworkAccessManager` 为主（HTTP/1.1、HTTP/2），预留后续接入 libcurl 实现 HTTP/3 的扩展接口，上层业务不改动

## 技术架构

```
gdei-auth-client/
├── CMakeLists.txt            # 单一 CMake 构建；固定 Qt6、C++20
├── src/
│   ├── main.cpp              # 入口：装配核心对象并注入 QML 根上下文
│   ├── core/                 # 业务 / 网络 / 持久化 / 加密 / 系统能力
│   │   ├── Constants.hpp     #   门户地址、表单字段、默认值、超时与退避常量
│   │   ├── Types.hpp         #   日志级别 / 认证状态 / 语言枚举
│   │   ├── Logger.*          #   分级日志：内存缓冲 + 文件持久化 + 导出
│   │   ├── SecretStore.*     #   平台安全存储：Windows DPAPI / macOS Keychain（Linux/Android 后备）
│   │   ├── ConfigStore.*     #   QSettings 持久化配置
│   │   ├── SessionCookieJar.*#   完整 Cookie 会话管理
│   │   ├── PortalClient.*    #   QNAM 门户认证客户端（CSRF / 表单 / 解码）
│   │   ├── DevicePortalClient.*# 移动 ICEfaces 自助门户：验证码会话 + 设备只读列表
│   │   ├── AccountPortalClient.*# PC PrimeFaces 自助门户：验证码会话 + 资料/服务占位
│   │   ├── SelfServiceCodec.*  #   自助门户口令前端可逆编码
│   │   ├── KeepAlive.*       #   随机间隔探测，掉线才重登
│   │   └── I18n.*            #   四语言字典与切换
│   └── app/                  # 应用层控制器，向 QML 暴露接口
│       ├── AuthController.*  #   captive 在线态：组合门户客户端 + 保活
│       ├── SessionController.*# 页面登录态：手动 / 自动认证 + 按需登录弹层
│       ├── DeviceController.*# 移动自助门户会话与设备列表
│       ├── AccountController.*# PC 自助门户账户会话与资料占位
│       ├── LocalDeviceController.*# 本机本地设备档案
│       ├── NetworkInfoController.*# 本机网卡 / 网络信息
│       ├── LogController.*   #   日志面板适配层
│       └── AppController.*   #   聚合 config / i18n / auth / session / devices / account / logs 暴露给 QML
├── qml/
│   ├── Main.qml              # 主窗口
│   ├── theme/Theme.qml       # 设计令牌（配色 / 圆角 / 阴影）
│   └── components/           # 可复用 QML 组件
│       ├── Card.qml          #   卡片容器
│       ├── TextField.qml     #   输入框
│       ├── PrimaryButton.qml #   主按钮
│       ├── Switch.qml        #   开关
│       ├── LangSelector.qml  #   语言选择器
│       └── LogPanel.qml      #   日志面板
├── scripts/                  # 免编译参考脚本（不参与构建）
└── docs/
    ├── PROTOCOL.md           # 抓包协议逆向说明与表单字段
    └── COMPATIBILITY.md      # 兼容平台矩阵与工具链明细
```

**分层约定**：C++ 负责业务、网络与底层系统能力；QML 只做 UI 交互。平台差异仅用 Qt 内置跨平台宏收敛，不分散编写。代码注释中英双语，便于学校信息中心 Java 团队阅读维护。

## 快速开始

### 依赖

- CMake ≥ 3.16
- Qt 6 LTS，组件：`Core` `Quick` `QuickControls2` `Network` `Qml` `Svg` `Core5Compat`；Windows 托盘另需 `Widgets`
- 编译器支持 C++20（MSVC 2019+ / GCC 11+ / Clang 13+ / Xcode 13+）

### 一键构建（推荐，自动打包全部依赖）

各平台都有对应的一键脚本，会**自动部署工具链 + 自动把 Qt 运行时依赖打包到产物旁**，保证双击即开。这修复了历史版本“Windows 打不开 / Linux 装上也打不开”的问题——根因是之前只产出裸 exe/二进制，没有拷贝 Qt 平台插件与 QML 导入模块。

| 平台 | 脚本 | 产物 |
|---|---|---|
| Windows | `scripts/build-windows.ps1` | `artifacts/windows/gdei-connect/`（整个文件夹自包含）+ `artifacts/windows/gdei-connect-windows-x64.zip` |
| macOS | `scripts/build-macos.sh` | `artifacts/macos/gdei-connect-macos.dmg` |
| Linux（Debian / Arch） | `scripts/build-linux.sh` | `artifacts/arch/gdei-connect-linux-x64/` + `.tar.gz`（自包含，目标机无需装 Qt） |
| Android | `scripts/build-android.ps1` | `artifacts/android/gdei-connect-android-arm64-v8a-debug.apk`（需提供 Qt for Android + SDK/NDK + JDK17 路径） |

Windows 示例（在项目根目录，PowerShell）：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

该脚本会把便携 Python / CMake / Ninja 装到 `%USERPROFILE%\.cache\gdei-toolchain`，用 aqt 拉取 Qt6 + MinGW，编译后自动运行 `windeployqt`。整个 `artifacts/windows/gdei-connect` 文件夹拷到任何 Windows 机器都能直接双击运行，无需预装 Qt 或编译器。

> CMake 现已加入 **POST_BUILD 自动 windeployqt**（Windows）与跨平台 `qt_generate_deploy_qml_app_script`（install/打包路径），每次构建都会把 `platforms/qwindows.dll`、Qt DLL、QML 导入模块等完整依赖闭包拷到产物旁，不再产出“裸 exe”。

### 配置与构建（手动）

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 打包产物

打包脚本收口在 [`packaging/`](packaging/README.md)。下表区分**目标产物**与**当前 CI 实际产出**：

| 平台 | 目标产物 | 当前 CI 产物 |
|---|---|---|
| Windows | NSIS exe 安装包 | `gdei-connect-windows-x64.zip`（windeployqt 收集依赖） |
| macOS | DragNDrop dmg（签名 / 公证待补） | `gdei-connect-macos.dmg`（macdeployqt + hdiutil） |
| Linux | deb / rpm / Arch `pkg.tar.zst` | `gdei-connect-linux-x64.tar.gz`（自包含：Qt 库 + 插件 + QML，见 `scripts/build-linux.sh`） |
| Android | apk（Qt for Android + NDK） | CI 仍为实验性脚手架；本地已产出 `artifacts/android/gdei-connect-android-arm64-v8a-debug.apk` |

```bash
cd build
cpack -G <generator>
```

> Windows 已在便携工具链（aqtinstall + MinGW）下本地编译验证，产物经 windeployqt 自包含打包，可直接双击运行。Linux 自包含归档同样打包了 Qt 运行时；如需 deb / rpm / Arch 原生包，请在对应发行版容器内用 CPack 生成（系统级依赖 glibc / X11 / Wayland / dbus 等仍由目标设备提供）。
>
> Android 本地 Debug 包已使用 Qt 6.8.3、NDK 26.1.10909125、JDK 17 和 targetSdk 35 完成构建，包名 `org.gdei.connect`，minSdk 28，ABI 为 `arm64-v8a`；已通过 aapt 元数据 / 权限检查、包内容检查与 apksigner v2 签名校验。平台权限仅 `INTERNET` 和 `ACCESS_NETWORK_STATE`；Qt6Core 默认合并的旧式 `WRITE_EXTERNAL_STORAGE` 已在 Manifest 中移除。AndroidX 自动合并的 `DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION` 是应用自有的 signature 级内部权限。Debug 签名不用于正式发布；真机安装、旋转、触控及后台保活仍未验证。

## 配置

配置通过 `QSettings` 持久化（组织名 `GDEI`，应用名 `gdei-connect`），界面“保存”按钮写入磁盘：

| 字段 | 说明 |
|---|---|
| `username` | 账号 |
| `password` | 口令（内存明文；仅“记住密码”开启时以混淆值落盘） |
| `rememberPassword` | 记住密码开关 |
| `autoLogin` | 自动登录开关（启动且凭据齐全时自动上线） |
| `lang` | 界面语言枚举：`0` 简体中文 / `1` 繁體中文 / `2` English / `3` 日本語 |
| `host` | 门户域名（默认 `nac.gdei.edu.cn`） |
| `ip` | 备用直连 IP（默认 `210.38.64.39`） |
| `nasip` | NAS / AC 地址（默认 `192.168.100.8`） |
| `requestEncoding` | 认证 POST 表单编码：`ASCII` / `GBK` / `UTF-8`，默认 `UTF-8`；界面、翻译、配置与日志始终使用 UTF-8 |

## 多语言

| 代码 | 切换器显示名 |
|---|---|
| `zh-CN` | 简体中文 |
| `zh-TW` | 繁體中文 |
| `en-GB` | English |
| `ja` | 日本語 |

- 顶部语言下拉即时切换；选择后随配置保存，下次启动沿用。
- 译文集中在 `src/core/I18n.cpp` 对应字典；缺键自动回退 English (UK)，再回退键名。

## 兼容矩阵

> 硬性基线（固定 Qt6，不维护 Qt5 分支）：

| 平台 | 架构 | 版本底线 | 备注 |
|---|---|---|---|
| Windows | x86 / x64 / ARM64 | Windows 10 1509 ~ Windows 11 25H2 | NSIS 安装包；ARM64 需对应工具链交叉编译 |
| macOS | x64 / ARM64 | macOS 10.15 Catalina 及以上 | dmg 打包 |
| Android | ARM64 | Android 9（API 28）及以上 | 当前构建脚本目标为 arm64-v8a；其他 ABI 需另行构建验证，CI 仍为实验性 |
| Debian | x64 | bookworm 及以上 | 容器内 CPack 出 `.deb` |
| Arch Linux | x64 | 滚动发行，内核 5.0+ | 容器内 tar.gz；`pkg.tar.zst` 待 PKGBUILD |

详细工具链与打包说明见 [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md)。

## 持续集成与发布 / CI & Release

流水线定义在 `.github/workflows/`：

- **`ci.yml`**：推送到 `main` 或 Pull Request 时，在 Windows / macOS 原生 Runner 与 Debian / Arch 容器内配置并构建 Qt6/QML 工程；Android 为实验性脚手架（`continue-on-error`）。产物作为 Actions 构件保留 14 天。
- **`release.yml`**：推送 `v*` 标签时，按平台调用 [`packaging/`](packaging/README.md) 脚本打包（Windows zip / macOS dmg / Debian deb / Arch tar.gz），并自动挂到对应 **GitHub Release** 供下载。

发版 / Cut a release：

```bash
git tag v0.2.1
git push origin v0.2.1
```

> 原生 Runner 用缓存 Qt SDK；Debian / Arch 容器使用系统 Qt 以减少重复下载。Android 本地构建链路已验证并产出 Debug APK；CI 正式出包、其他 ABI、真机验证与 Arch `pkg.tar.zst` 仍需对应工具链、设备或扩展 Runner。

## 保活机制 / Keep-alive

1. 按随机抖动间隔访问 NCSI 探测地址，返回体含 `Microsoft` 判定在线（默认 150–210 秒，即 3 分钟 ±30 秒，可配置）；
2. 探测离线时才触发重新认证，不做无脑高频 POST；
3. Windows 最小化到托盘后由现有保活控制器继续运行；macOS / Linux 托盘与 Android 前台服务保活仍为规划中能力。

## 安全说明 / Security

- 本地口令经平台安全存储落盘：Windows 走 DPAPI，macOS 走 Keychain；Linux / Android 暂为明确标注的后备实现（Secret Service / Keystore 待接入）。代码不使用硬编码固定密钥。
- 真实账号口令不得提交仓库；仓库内不保存任何明文凭据或 token。
- 网络层**默认严格校验服务器证书**；仅在设置中显式开启“跳过证书校验”时才放行自签等异常，并写入警告日志留痕。

## 排障 / Troubleshooting

- **登录返回空响应**：本机可能已在线，属正常；断网后再测完整认证流程。
- **提示图形验证码**：连续输错后门户会要求验证码，当前版本不处理；先用浏览器手动登录一次。
- **连不上门户**：检查 `host / ip / nasip`，确认处于校园网可达路径。
- **403 被拒**：多为触发限流，保活随机间隔已降低概率；稍后再试或检查是否多端重复登录。

## 文档

- [PROTOCOL.md](docs/PROTOCOL.md)：抓包协议逆向说明、时序与表单字段。
- [COMPATIBILITY.md](docs/COMPATIBILITY.md)：兼容平台矩阵、Qt6 工具链与打包明细。
- [THIRD-PARTY-NOTICES.md](docs/THIRD-PARTY-NOTICES.md)：应用校徽与各平台品牌图标的来源、授权与商标归属。

## 许可证 / License

**LGPL-3.0**。不引入 GPL 强传染依赖；使用方需遵守 LGPL 关于动态链接与源码提供的要求。应用内平台品牌图标仅作兼容标识，相关商标归各自厂商所有，详见第三方声明。

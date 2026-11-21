<div align="center">

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
- **完整 CookieJar 会话管理**，匹配 Nginx 会话校验与 IP 绑定机制
- **自动保活**：定时探测外网连通性，仅掉线时才触发登录；探测间隔带随机抖动，避免固定高频轮询被 Nginx 限流 403
- 桌面端系统托盘 / 后台静默保活、Android 前台服务与电池优化引导：**规划中，尚未实现**
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
│   │   ├── KeepAlive.*       #   随机间隔探测，掉线才重登
│   │   └── I18n.*            #   四语言字典与切换
│   └── app/                  # 应用层控制器，向 QML 暴露接口
│       ├── AuthController.*  #   组合门户客户端 + 保活
│       ├── LogController.*   #   日志面板适配层
│       └── AppController.*   #   聚合 config / i18n / auth / logs 暴露给 QML
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
- Qt 6 LTS，组件：`Core` `Quick` `QuickControls2` `Network` `Qml` `Svg`
- 编译器支持 C++20（MSVC 2019+ / GCC 11+ / Clang 13+ / Xcode 13+）

### 配置与构建

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
| Linux | deb / rpm / Arch `pkg.tar.zst` | `gdei-connect-linux-x64.tar.gz`（裸二进制归档） |
| Android | apk（Qt for Android + NDK） | 暂无 |

```bash
cd build
cpack -G <generator>
```

> 本仓库在无 Qt / 编译器的 Windows 沙箱中生成源码基准，未在本地编译验证；请在目标工具链上构建。Linux 包需在对应发行版容器内生成，系统级依赖（glibc / X11 / Wayland / dbus 等）由目标设备提供。

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
| Android | ARM / ARM64 / x86 / x64 | Android 8.0（API 26）及以上 | 前台服务 + 电池优化引导；apk 打包（CI 脚手架实验性，待出包） |
| Debian | x64 | bookworm 及以上 | 容器内 CPack 出 `.deb` |
| Arch Linux | x64 | 滚动发行，内核 5.0+ | 容器内 tar.gz；`pkg.tar.zst` 待 PKGBUILD |

详细工具链与打包说明见 [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md)。

## 持续集成与发布 / CI & Release

流水线定义在 `.github/workflows/`：

- **`ci.yml`**：推送到 `main` 或 Pull Request 时，在 Windows / macOS 原生 Runner 与 Debian / Arch 容器内配置并构建 Qt6/QML 工程；Android 为实验性脚手架（`continue-on-error`）。产物作为 Actions 构件保留 14 天。
- **`release.yml`**：推送 `v*` 标签时，按平台调用 [`packaging/`](packaging/README.md) 脚本打包（Windows zip / macOS dmg / Debian deb / Arch tar.gz），并自动挂到对应 **GitHub Release** 供下载。

发版 / Cut a release：

```bash
git tag v0.2.0
git push origin v0.2.0
```

> 原生 Runner 用缓存 Qt SDK；Debian / Arch 容器使用系统 Qt 以减少重复下载。Android apk、ARM64 与 Arch `pkg.tar.zst` 仍需对应工具链或扩展 Runner 出包。

## 保活机制 / Keep-alive

1. 按随机抖动间隔访问 NCSI 探测地址，返回体含 `Microsoft` 判定在线（当前 45–90 秒；基线目标 3 分钟 ±30 秒，待调整）；
2. 探测离线时才触发重新认证，不做无脑高频 POST；
3. 桌面端托盘 / 后台静默运行、Android 前台服务保活为规划中能力，尚未实现。

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

## 许可证 / License

**LGPL-3.0**。不引入 GPL 强传染依赖；使用方需遵守 LGPL 关于动态链接与源码提供的要求。

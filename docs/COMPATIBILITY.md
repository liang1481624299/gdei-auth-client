# 兼容策略 / Compatibility Strategy

核对日期：2026-11-21。目标范围：下列最低版本 → 2026-11-21 仍在维护的新系统。

## 1. 分层原则 / Layering

- **核心库 `gdei_core`**：纯 C++17，不链接 Qt；HTTP 后端 Windows 用 WinINet，其余用 libcurl。CLI 覆盖面最广。
- **CLI `gdei-auth`**：只依赖核心库，无显示器设备可直接运行。
- **GUI `gdei-auth-gui`**：Qt Widgets 封装核心库；能装 Qt 的系统才提供图形界面。

## 2. Qt 版本分档 / Qt Tiering

依据 Qt 官方支持平台文档：

- Qt 6.5 官方底线：Windows 10 **1809+**、macOS **11+**、iOS **14+**、Android **8.0 (API 26)+**。
- Qt 5.15 官方底线：Windows 10、macOS **10.13+**、Android **5.0 (API 21)+**。

| 平台底线 | GUI 选用 | 理由 / Reason |
|---|---|---|
| Windows 10 1703 / 1709 | Qt 5.15 | Qt6 要求 1809+ |
| Windows 10 1809+ / Windows 11 | Qt 6 LTS | Qt6 官方支持 |
| macOS 10.13 High Sierra | Qt 5.15 | Qt6 要求 macOS 11+ |
| macOS 11+ | Qt 6 LTS | Qt6 官方支持 |
| Android 7.1 (API 25) | Qt 5.15 | Qt6.5 从 API 26 起 |
| Android 8.0 (API 26)+ | Qt 6.5+ | Qt6 官方支持 |
| iOS 11 | Qt 5.15（尽力） | 现代 Qt 对 iOS 11 仅旧版可尝试 |
| iOS 10.3 | 不支持 | 低于现代 Qt 实际部署底线 |
| iOS 14+ | Qt 6 | Qt6 官方支持 |

参考：
- Qt 6.5 Supported Platforms: https://doc.qt.io/qt-6.5/supported-platforms.html
- Qt 5.15 Supported Platforms: https://doc.qt.io/qt-5/supported-platforms.html

## 3. 平台明细 / Platform Details

### Windows

- CLI：WinINet 自带，可运行于列出的 Windows 版本；编译器 MSVC 2019+ 或对应 MinGW。
- GUI：
  - 1703 / 1709 → Qt 5.15；
  - 1809+ / 11 → Qt 6 LTS；ARM64 需从 x64 Windows 交叉编译部署。

### Linux（内核 4.10 – 4.14）

- CLI：用发行版自带 GCC/Clang 编译，链接系统 libcurl；注意不要在更新的 glibc 上编译后拷到老系统（符号兼容）。
- GUI：老发行版用 Qt 5.15，并在同代系统上构建。

### macOS

- 10.13：Qt 5.15，`CMAKE_OSX_DEPLOYMENT_TARGET=10.13`；不要把部署目标降到 Qt 默认值以下。
- 11+：Qt 6 LTS。

### Android

- CLI：Termux 环境编译可执行文件。
- GUI：7.1 用 Qt 5.15；8.0+ 用 Qt 6.5+；NDK/JDK/Gradle 版本与所用 Qt 套件官方要求对齐。

### iOS

- 无终端 CLI；仅 GUI，需 Xcode 工程、开发者签名与上架/侧载配置。
- 10.3 不支持；11 用 Qt 5.15 尽力打包并实测；14+ 用 Qt6。

### ChromeOS（Chrome 55 – 62）

- 普通 Chromebook 不能直接安装 Qt 原生程序。
- 可行方向：后续补 **PWA 网页前端**（复用同一套 HTTP/认证逻辑），或在 Linux/Android 容器中跑 CLI。

### Windows 10 Mobile 1709

- 现代 Qt 无对应移植，不提供原生客户端；仅建议网页或脚本方案。

## 4. 待办 / Open Items

- [ ] 增加 Web/PWA 前端，覆盖 ChromeOS 与无 Qt 环境。
- [ ] 在各底线系统上实机编译验证（本仓库当前为源码基准，未本地交叉编译）。
- [ ] iOS 工程与签名配置（Qt 5.15 档）。

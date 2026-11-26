# 兼容策略 / Compatibility Strategy

核对日期：本地 CST(UTC+8) `2026-11-22 17:40:35` ｜ UTC/GMT `2026-11-22 09:40:35Z` ｜ UNIX `1795340435`。本项目**固定 Qt6、C++20、Qt Quick/QML**，单一源码仓库、单一 CMake 构建，不做 Qt 版本分支，不维护两套工程。

## 1. 硬性平台基线 / Hard Baseline

| 平台 | 架构 | 版本底线 | 目标产物 | 当前 CI 产物 |
|---|---|---|---|---|
| Windows | x64 / ARM64 | Windows 10 1509 ~ Windows 11 25H2 | NSIS exe 安装包 | x64：windeployqt 收集后的 zip；ARM64：`windows-11-arm` runner 实验性构建 |
| macOS | x64（Intel）/ ARM64（Apple Silicon） | macOS 10.15 Catalina 及以上 | DragNDrop dmg（签名 / 公证待补） | x64：`macos-15-intel`；ARM64：`macos-latest`，macdeployqt + hdiutil 生成的 dmg |
| Ubuntu | x64 / ARM64 | 24.04 (noble) 及以上 | 自包含 tar.gz（deb 可选） | x64：`ubuntu-24.04`；ARM64：`ubuntu-24.04-arm`，系统 Qt 容器外原生构建 |
| Debian | x64 | bookworm（内核 6.1）及以上 | deb | 容器内 CPack 生成 `.deb` |
| Arch Linux | x64 | 滚动发行，内核 5.0 及以上 | `pkg.tar.zst` | 容器内裸二进制 tar.gz |
| Android | arm64-v8a / x86_64 | Android 9（API 28）及以上 | 双 ABI 合并 apk | CI 为实验性脚手架；arm64-v8a（真机）+ x86_64（模拟器），真机状态待验证 |
| Arch Linux ARM64 (aarch64) | ARM64 | 基线目标，未排期 | — | 无现成 GitHub 托管 ARM 容器/镜像，待自托管 ARM runner 或 QEMU 交叉链路打通 |

> 说明：“目标产物”是产品交付基线；“当前 CI 产物”是托管 Runner 现阶段实际产出，明细见 [`packaging/README.md`](../packaging/README.md)。Qt6 官方桌面端**不提供 Windows 32 位（x86）预编译库**，该架构不在支持范围内；标注为“底线”的系统需在对应工具链上实机构建验证，见第 4 节待办。

## 2. 技术栈基线 / Tech Stack Baseline

- 后端逻辑、网络、持久化、加密、系统能力：**C++20**
- UI：**Qt Quick QML + Qt Quick Controls 2**（禁止用 Qt Widgets 做主界面）
- 构建系统：**CMake**（单一工程）
- Qt 版本：**Qt6 固定**，不允许切 Qt5
- 网络：优先 `QNetworkAccessManager`（HTTP/1.1、HTTP/2）；预留 libcurl 接入 HTTP/3 的扩展接口，上层业务逻辑不改
- 许可证：LGPL；不引入 GPL 强传染依赖，不引入重型第三方 GUI 框架、CEF、WebView、Node.js 或浏览器内核

## 3. 平台明细 / Platform Details

### Windows

- 工具链：MSVC 2022+ 或对应 MinGW，C++20 支持。
- x64（x86_64）为常规构建目标；ARM64 由 `windows-11-arm` 托管 runner 实验性构建。Qt6 官方不提供 Windows 32 位（x86）预编译库，该架构不可行。
- 后台能力：Windows 系统托盘支持最小化、显示主窗口、上线、下线与退出；保活沿用现有控制器。
- 安装包：目标 CPack NSIS；当前 Runner 产出 windeployqt 收集后的 zip。

### macOS

- 工具链：Xcode 13+，`CMAKE_OSX_DEPLOYMENT_TARGET=10.15`。
- 同时支持 Intel x64（`macos-15-intel`，Qt `clang_64`）与 Apple Silicon ARM64（`macos-latest`，Qt `clang_arm64`）；可出通用二进制或分别打包。
- 后台能力：系统托盘 + 静默保活（规划中，尚未实现）。
- 安装包：目标 CPack DragNDrop（dmg）；当前由 macdeployqt + hdiutil 生成 dmg，未签名 / 未公证。

### Android

- 构建：本地已验证 Qt 6.8.3 for Android、NDK 26.1.10909125、JDK 17、Gradle 8.10 / AGP 8.6.0 与 build-tools 35.0.0。
- 最低 SDK：API 28（Android 9），targetSdk / compileSdk 35；构建脚本可选 `arm64-v8a`（真机）或 `x86_64`（模拟器），CI 以 `QT_ANDROID_ABIS="arm64-v8a;x86_64"` 出双 ABI 合并包。
- 后台能力：前台服务保活；首次运行引导用户关闭电池优化（规划中，尚未实现）。
- 产物：CI 实验性产出 `gdei-auth-client-android-arm64-v8a-x86_64-*.apk`，包名 `org.gdei.connect`；x86（32 位）与 armeabi-v7a（ARM32）未纳入本轮。真机安装、旋转与触控验证尚未完成；Release 包未签名，需自签名后才能安装或发布。
- 权限：仅申请 `INTERNET`、`ACCESS_NETWORK_STATE` 两个平台权限；Manifest 移除 Qt6Core 自动加入的旧式 `WRITE_EXTERNAL_STORAGE`。AndroidX 合并的应用自有 `DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION` 为 signature 级内部权限，不是共享存储或敏感运行时权限。

### Linux

- 工具链：GCC 11+ / Clang 13+，内核 5.0+。
- CI 覆盖：Ubuntu 24.04 原生 runner（x64 `ubuntu-24.04`、ARM64 `ubuntu-24.04-arm`，系统 Qt）；`debian:bookworm` 与 `archlinux:latest` 容器内 x64 系统 Qt 构建，不在容器内重复下载整套 Qt SDK。
- 打包：Debian 由 CPack 生成 `.deb`（`dpkg-shlibdeps` 声明动态依赖）；Arch 当前产出裸二进制 tar.gz，`pkg.tar.zst` 待补 PKGBUILD；rpm 暂未纳入目标容器。Ubuntu 保底产物为自包含 tar.gz。
- Arch Linux ARM64 (aarch64) 属基线目标：GitHub 托管环境无现成 Arch ARM 容器/镜像，暂未排期 CI，待自托管 ARM runner 或 QEMU 交叉链路打通。
- 注意不要在更新的 glibc 上编译后拷贝到老系统（符号兼容）。
- 后台能力：系统托盘 + 静默保活（规划中，尚未实现）。

## 4. 待办 / Open Items

- [ ] 在各底线系统 / 架构上实机编译验证（Windows x64 已本机构建；Windows ARM64、macOS Intel、Ubuntu ARM64、Android x86_64 待 CI 首跑与实机验证）。
- [ ] Android 双 ABI 合并包真机安装、旋转与触控验证，x86_64 模拟器回归，前台服务与电池优化引导；CI 脚手架转为正式出包流水线。
- [ ] Debian `.deb`、Arch `pkg.tar.zst`（待 PKGBUILD）在对应发行版上安装验证；Arch Linux ARM64 待自托管 ARM runner 打通。
- [ ] macOS / Linux 系统托盘实现与实机验证；Windows 托盘菜单动作及长时间后台保活仍需完整回归。
- [ ] Release 分发包按架构拆分产物名（当前打包脚本仍输出 x64 命名），ARM64 桌面产物接入签名/公证流程。

## 参考 / References

- Qt 6 Supported Platforms: https://doc.qt.io/qt-6/supported-platforms.html
- Qt for Android: https://doc.qt.io/qt-6/android-getting-started.html

# 兼容策略 / Compatibility Strategy

核对日期：2026-11-22。本项目**固定 Qt6、C++20、Qt Quick/QML**，单一源码仓库、单一 CMake 构建，不做 Qt 版本分支，不维护两套工程。

## 1. 硬性平台基线 / Hard Baseline

| 平台 | 架构 | 版本底线 | 目标产物 | 当前 CI 产物 |
|---|---|---|---|---|
| Windows | x86 / x64 / ARM64 | Windows 10 1509 ~ Windows 11 25H2 | NSIS exe 安装包 | windeployqt 收集后的 zip |
| macOS | x64 / ARM64 | macOS 10.15 Catalina 及以上 | DragNDrop dmg（签名 / 公证待补） | macdeployqt + hdiutil 生成的 dmg |
| Android | ARM64 | Android 9（API 28）及以上 | arm64-v8a apk | CI 为实验性脚手架；本地已产出 Debug APK，其他 ABI 与真机状态待验证 |
| Debian | x64 | bookworm（内核 6.1）及以上 | deb | 容器内 CPack 生成 `.deb` |
| Arch Linux | x64 | 滚动发行，内核 5.0 及以上 | `pkg.tar.zst` | 容器内裸二进制 tar.gz |
| 其他 Linux | x86 / x64 / ARM | 内核 5.0 及以上 | rpm / 通用 tar.gz | 暂未在对应发行版容器出包 |

> 说明：“目标产物”是产品交付基线；“当前 CI 产物”是托管 Runner 现阶段实际产出，明细见 [`packaging/README.md`](../packaging/README.md)。Qt6 官方支持矩阵在个别旧底线上可能更严；标注为“底线”的系统需在对应工具链上实机构建验证，见第 4 节待办。

## 2. 技术栈基线 / Tech Stack Baseline

- 后端逻辑、网络、持久化、加密、系统能力：**C++20**
- UI：**Qt Quick QML + Qt Quick Controls 2**（禁止用 Qt Widgets 做主界面）
- 构建系统：**CMake**（单一工程）
- Qt 版本：**Qt6 固定**，不允许切 Qt5
- 网络：优先 `QNetworkAccessManager`（HTTP/1.1、HTTP/2）；预留 libcurl 接入 HTTP/3 的扩展接口，上层业务逻辑不改
- 许可证：LGPL；不引入 GPL 强传染依赖，不引入重型第三方 GUI 框架、CEF、WebView、Node.js 或浏览器内核

## 3. 平台明细 / Platform Details

### Windows

- 工具链：MSVC 2019+ 或对应 MinGW，C++20 支持。
- x64 为常规构建目标；x86 / ARM64 需对应工具链交叉编译。
- 后台能力：Windows 系统托盘支持最小化、显示主窗口、上线、下线与退出；保活沿用现有控制器。
- 安装包：目标 CPack NSIS；当前 Runner 产出 windeployqt 收集后的 zip。

### macOS

- 工具链：Xcode 13+，`CMAKE_OSX_DEPLOYMENT_TARGET=10.15`。
- 同时支持 Intel x64 与 Apple Silicon ARM64；可出通用二进制或分别打包。
- 后台能力：系统托盘 + 静默保活（规划中，尚未实现）。
- 安装包：目标 CPack DragNDrop（dmg）；当前由 macdeployqt + hdiutil 生成 dmg，未签名 / 未公证。

### Android

- 构建：本地已验证 Qt 6.8.3 for Android、NDK 26.1.10909125、JDK 17、Gradle 8.10 / AGP 8.6.0 与 build-tools 35.0.0。
- 最低 SDK：API 28（Android 9），targetSdk / compileSdk 35；当前构建脚本仅接受 arm64-v8a。
- 后台能力：前台服务保活；首次运行引导用户关闭电池优化（规划中，尚未实现）。
- 产物：本地已产出 `artifacts/android/gdei-connect-android-arm64-v8a-debug.apk`，包名 `org.gdei.connect`，versionName `0.2.0`；已通过 aapt 包信息 / 权限 / 资源依赖检查与 apksigner v2 Debug 签名校验。CI 仍为实验性脚手架（`continue-on-error`），真机安装与运行未验证。
- 权限：仅申请 `INTERNET`、`ACCESS_NETWORK_STATE` 两个平台权限；Manifest 移除 Qt6Core 自动加入的旧式 `WRITE_EXTERNAL_STORAGE`。AndroidX 合并的应用自有 `DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION` 为 signature 级内部权限，不是共享存储或敏感运行时权限。

### Linux

- 工具链：GCC 11+ / Clang 13+，内核 5.0+。
- CI 在 `debian:bookworm` 与 `archlinux:latest` 容器内使用系统 Qt 构建，不在容器内重复下载整套 Qt SDK。
- 打包：Debian 由 CPack 生成 `.deb`（`dpkg-shlibdeps` 声明动态依赖）；Arch 当前产出裸二进制 tar.gz，`pkg.tar.zst` 待补 PKGBUILD；rpm 暂未纳入目标容器。
- 注意不要在更新的 glibc 上编译后拷贝到老系统（符号兼容）。
- 后台能力：系统托盘 + 静默保活（规划中，尚未实现）。

## 4. 待办 / Open Items

- [ ] 在各底线系统 / 架构上实机编译验证（Windows x64 已本机构建，其他底线系统与架构仍待验证）。
- [ ] Android apk 真机安装、旋转与触控验证，前台服务与电池优化引导；CI 脚手架转为正式出包流水线。
- [ ] Debian `.deb`、Arch `pkg.tar.zst`（待 PKGBUILD）在对应发行版上安装验证。
- [ ] macOS / Linux 系统托盘实现与实机验证；Windows 托盘菜单动作及长时间后台保活仍需完整回归。

## 参考 / References

- Qt 6 Supported Platforms: https://doc.qt.io/qt-6/supported-platforms.html
- Qt for Android: https://doc.qt.io/qt-6/android-getting-started.html

# 兼容策略 / Compatibility Strategy

核对日期：2026-11-21。本项目**固定 Qt6、C++20、Qt Quick/QML**，单一源码仓库、单一 CMake 构建，不做 Qt 版本分支，不维护两套工程。

## 1. 硬性平台基线 / Hard Baseline

| 平台 | 架构 | 版本底线 | 打包产物 |
|---|---|---|---|
| Windows | x86 / x64 / ARM64 | Windows 10 1509 ~ Windows 11 25H2 | NSIS exe 安装包 |
| macOS | x64 / ARM64 | macOS 10.15 Catalina 及以上 | DragNDrop dmg |
| Android | ARM / ARM64 / x86 / x64 | Android 8.0（API 26）及以上 | apk（前台服务） |
| Linux | x86 / x64 / ARM | 内核 5.0 及以上 | deb / rpm / Arch `pkg.tar.zst` |

> 说明：上表是产品交付目标。Qt6 官方支持矩阵在个别旧底线上可能更严（例如部分 Qt6 LTS 版本要求更新的 Windows / macOS 版本）；标注为“底线”的系统需在对应工具链上实机构建验证，见第 4 节待办。

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
- 后台能力：系统托盘 + 静默保活。
- 安装包：CPack NSIS。

### macOS

- 工具链：Xcode 13+，`CMAKE_OSX_DEPLOYMENT_TARGET=10.15`。
- 同时支持 Intel x64 与 Apple Silicon ARM64；可出通用二进制或分别打包。
- 后台能力：系统托盘 + 静默保活。
- 安装包：CPack DragNDrop（dmg）。

### Android

- 构建：Qt for Android 套件 + NDK + JDK + Gradle，版本与所用 Qt6 官方要求对齐。
- 最低 SDK：API 26（Android 8.0）。
- 后台能力：前台服务保活；首次运行引导用户关闭电池优化。
- 产物：apk。

### Linux

- 工具链：GCC 11+ / Clang 13+，内核 5.0+。
- 打包：CPack 生成 deb / rpm / tgz；Arch `pkg.tar.zst` 由打包脚本基于 tar 产物压缩生成。
- 注意不要在更新的 glibc 上编译后拷贝到老系统（符号兼容）。
- 后台能力：系统托盘 + 静默保活。

## 4. 待办 / Open Items

- [ ] 在各底线系统 / 架构上实机编译验证（当前仓库为源码基准，本机无 Qt 与编译器，未本地构建）。
- [ ] Android apk 签名、前台服务与电池优化引导的真机验证。
- [ ] Linux deb / rpm / Arch 包在对应发行版上安装验证。
- [ ] 系统托盘与后台保活在桌面三平台的实机行为验证。

## 参考 / References

- Qt 6 Supported Platforms: https://doc.qt.io/qt-6/supported-platforms.html
- Qt for Android: https://doc.qt.io/qt-6/android-getting-started.html

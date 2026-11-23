# 打包脚本 / Packaging

平台部署脚本统一收口在本目录；CI（`.github/workflows/release.yml`）按平台调用对应脚本。

## 目录结构

```
packaging/
├── windows/deploy.ps1   # windeployqt 收集 Qt 运行库 / 插件 / QML，归档 zip
├── macos/deploy.sh      # macdeployqt 收编框架，hdiutil 制作 dmg
└── linux/deploy.sh      # cmake --install 收编 Qt 运行时为自包含目录，归档 tar.gz（Debian / Arch 通用，目标机无需装 Qt）
```

## 产物输出位置

各平台最终分发包统一输出到仓库根 `artifacts/<平台>/`：`windows/`、`macos/`、`debian/`、`arch/`、`android/`；该目录内容被 `.gitignore` 忽略，仅保留骨架与 `artifacts/README.md`。中间编译目录为 `build/`、`build-android/`，不属于产物目录。

## 产物现状与目标 / Current vs Target

| 平台 | 目标产物 | 当前 CI 产物 | 说明 |
|---|---|---|---|
| Windows | NSIS exe 安装包 | `gdei-auth-client-windows-x64.zip` | 已用 windeployqt 收集依赖；NSIS 安装包待 CPack 在带 NSIS 工具链的 Runner 上生成 |
| macOS | dmg | `gdei-auth-client-macos.dmg` | macdeployqt 收编后 hdiutil 打包；未签名 / 未公证，待补 |
| Debian (bookworm) | deb | CPack 生成的 `.deb`（可选） | 通用保底产物为自包含 `gdei-auth-client-linux-x64.tar.gz`（Qt 库 + 插件 + QML 已打包，解压即用）；deb 可在 Debian 容器内用 CPack 生成，`dpkg-shlibdeps` 声明系统动态依赖 |
| Arch Linux | `pkg.tar.zst` | `gdei-auth-client-linux-x64.tar.gz`（自包含） | 自包含 tar.gz 解压即用，目标机无需预装 Qt；`pkg.tar.zst` 原生包待补 PKGBUILD |
| Android | apk | 实验性脚手架，暂无 apk | Qt for Android + NDK / Gradle 工具链与真机验证待补，CI 中 `continue-on-error` 不阻塞主线 |

## 依赖分发原则

- 应用级运行依赖（Qt 库、QML 插件、imageformats 等）随安装包分发，用户无需手动安装 Qt。
- 系统底层依赖（glibc、X11 / Wayland、dbus、Cocoa / Foundation / Security、Android 运行时）由目标设备系统提供，不强行静态打包。
- 无法随包分发的系统级依赖，需在 README 与发布说明中列明，不宣称“完全零依赖”。

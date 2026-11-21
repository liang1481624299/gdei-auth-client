# 打包脚本 / Packaging

平台部署脚本统一收口在本目录；CI（`.github/workflows/release.yml`）按平台调用对应脚本。

## 目录结构

```
packaging/
├── windows/deploy.ps1   # windeployqt 收集 Qt 运行库 / 插件 / QML，归档 zip
├── macos/deploy.sh      # macdeployqt 收编框架，hdiutil 制作 dmg
└── linux/deploy.sh      # 二进制 tar.gz 归档（Arch 用；deb 由 Debian 容器内 CPack 生成）
```

## 产物现状与目标 / Current vs Target

| 平台 | 目标产物 | 当前 CI 产物 | 说明 |
|---|---|---|---|
| Windows | NSIS exe 安装包 | `gdei-connect-windows-x64.zip` | 已用 windeployqt 收集依赖；NSIS 安装包待 CPack 在带 NSIS 工具链的 Runner 上生成 |
| macOS | dmg | `gdei-connect-macos.dmg` | macdeployqt 收编后 hdiutil 打包；未签名 / 未公证，待补 |
| Debian (bookworm) | deb | CPack 生成的 `.deb` | 容器内系统 Qt 构建，`dpkg-shlibdeps` 声明动态依赖；rpm 不在 Debian 目标内 |
| Arch Linux | `pkg.tar.zst` | `gdei-connect-linux-x64.tar.gz` | 容器内系统 Qt 构建；`pkg.tar.zst` 待补 PKGBUILD |
| Android | apk | 实验性脚手架，暂无 apk | Qt for Android + NDK / Gradle 工具链与真机验证待补，CI 中 `continue-on-error` 不阻塞主线 |

## 依赖分发原则

- 应用级运行依赖（Qt 库、QML 插件、imageformats 等）随安装包分发，用户无需手动安装 Qt。
- 系统底层依赖（glibc、X11 / Wayland、dbus、Cocoa / Foundation / Security、Android 运行时）由目标设备系统提供，不强行静态打包。
- 无法随包分发的系统级依赖，需在 README 与发布说明中列明，不宣称“完全零依赖”。

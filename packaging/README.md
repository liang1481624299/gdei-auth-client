# 打包脚本 / Packaging

平台部署脚本统一收口在本目录；CI（`.github/workflows/release.yml`）按平台调用对应脚本。

## 目录结构

```
packaging/
├── windows/deploy.ps1   # windeployqt 收集 Qt 运行库 / 插件 / QML，归档 zip
├── macos/deploy.sh      # macdeployqt 收编框架，hdiutil 制作 dmg
└── linux/deploy.sh      # 二进制 tar.gz 归档（deb / rpm / Arch 在对应容器内生成）
```

## 产物现状与目标 / Current vs Target

| 平台 | 目标产物 | 当前 CI 产物 | 说明 |
|---|---|---|---|
| Windows | NSIS exe 安装包 | `gdei-connect-windows-x64.zip` | 已用 windeployqt 收集依赖；NSIS 安装包待 CPack 在带 NSIS 工具链的 Runner 上生成 |
| macOS | dmg | `gdei-connect-macos.dmg` | macdeployqt 收编后 hdiutil 打包；未签名 / 未公证，待补 |
| Linux | deb / rpm / Arch `pkg.tar.zst` | `gdei-connect-linux-x64.tar.gz` | 需在对应发行版容器内生成；系统级依赖不静态打包 |
| Android | apk | 暂无 | 待 Qt for Android + NDK / Gradle 工具链接入 |

## 依赖分发原则

- 应用级运行依赖（Qt 库、QML 插件、imageformats 等）随安装包分发，用户无需手动安装 Qt。
- 系统底层依赖（glibc、X11 / Wayland、dbus、Cocoa / Foundation / Security、Android 运行时）由目标设备系统提供，不强行静态打包。
- 无法随包分发的系统级依赖，需在 README 与发布说明中列明，不宣称“完全零依赖”。

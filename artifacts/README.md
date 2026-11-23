# 构建产物目录 / Build Artifacts

本目录存放各平台**最终分发包**，按系统分类。所有产物均为本地或 CI 构建生成，**不纳入版本控制**（见根目录 `.gitignore`）；目录骨架与本说明文件除外。

> This folder holds per-platform release packages, grouped by OS. Built binaries/archives are git-ignored; only the skeleton and this README are tracked.

## 目录与产物 / Layout

| 子目录 | 平台 / Target | 产物 / Artifact | 来源 / Producer |
|---|---|---|---|
| `windows/` | Windows x64 | `gdei-auth-client-windows-x64.zip`（解压后为自包含 `gdei-auth-client/` 目录） | `packaging/windows/deploy.ps1` |
| `macos/` | macOS | `gdei-auth-client-macos.dmg` | `packaging/macos/deploy.sh` |
| `debian/` | Debian (bookworm) | `*.deb`（CPack DEB 包） | Release 工作流 `cpack -G DEB` |
| `arch/` | Arch Linux / 通用 x86_64 | `gdei-auth-client-linux-x64.tar.gz`（自包含目录 + 启动脚本）；CPack Arch 包 `*.pkg.tar.zst` | `packaging/linux/deploy.sh` / Release 工作流 `cpack -G` |
| `android/` | Android arm64-v8a | `gdei-auth-client-android-arm64-v8a-<debug|release>.apk`（含 `.sha256`） | `scripts/build-android.ps1` |

## 说明 / Notes

- 中间编译目录 `build/`、`build-android/` 不属于本目录；本目录只放可分发的最终包。
- 历史脚本曾输出到 `dist/`，现已统一迁移到 `artifacts/<平台>/`；`dist/` 仅作兼容保留并被忽略。
- 包内可能包含本机路径、设备标识等信息，分发前按合规要求检查；凭据不得进入产物或仓库。

# gdei-auth-client

广二师（GDEI）校园网（H3C iMC Portal）自动认证客户端。
GDEI campus network (H3C iMC Portal) auto authentication client.

**设计原则 / Design**：核心是命令行（CLI），无显示器的设备也能直接用；
图形界面（Qt GUI）只是桌面端的便利封装，与 CLI 共用同一套核心库。
The CLI is the core; the Qt GUI is an optional desktop wrapper around the same core.

---

## 功能 / Features

- 上线 / 下线 / 状态查询（login / logout / status）
- **自动保活**：周期性探测外网，掉线自动重新发包认证
- 多平台：Windows / Linux / macOS / Android，x86 / x64 / ARM
- 内置 UTF-8 / ASCII 字符集处理，减少非 ASCII 账号口令的乱码问题
- 本地配置保存账号口令（可逆混淆，非明文落盘）
- 另附 Python / PowerShell / bat / shell 参考脚本（无需编译即可用）

## 目录结构 / Layout

```
gdei-auth-client/
├── CMakeLists.txt          # 顶层构建：core 静态库 + cli + 可选 gui
├── config.example.json     # 配置示例 / config template
├── core/                   # 纯 C++11 协议核心（不依赖 Qt）
│   ├── include/gdei/       #   配置 / 编码 / JSON / HTTP / 门户 / 保活
│   └── src/
├── cli/main.cpp            # 命令行前端 / headless CLI
├── gui/qt/                 # 可选 Qt 图形界面 / optional Qt GUI
├── scripts/                # 免编译参考实现
│   ├── python/             #   campus_core.py / campus_gui.py
│   ├── windows/            #   portal.ps1 / login.bat
│   └── unix/               #   login.sh
└── docs/
    ├── PROTOCOL.md         # 抓包协议逆向说明 / captured protocol notes
    └── COMPATIBILITY.md    # 兼容策略与 Qt 分档 / compatibility & Qt tiering
```

## 快速开始 / Quick Start

### 1. 保存账号口令（可选，也可每次用 `--user/--pass` 临时指定）

```bash
# Linux / macOS / Android Termux
./gdei-auth configure --user <账号> --pass '<口令>' --interval 60

# Windows
gdei-auth.exe configure --user <账号> --pass "<口令>" --interval 60
```

### 2. 命令行使用 / CLI

```bash
gdei-auth status       # 查看是否在线 / check online
gdei-auth login        # 上线 / authenticate
gdei-auth logout       # 下线 / log out
gdei-auth keepalive    # 后台保活，掉线自动重登 / auto keep-alive loop
gdei-auth --help       # 帮助 / help
```

通用参数：`--config <路径>` 指定配置文件；`--user` / `--pass` 单次覆盖。

### 3. 图形界面 / GUI（可选）

构建时打开 `-DGDEI_BUILD_GUI=ON`，运行 `gdei-auth-gui`。界面提供：
账号口令填写、状态 / 上线 / 下线 / 保存、保活开关与日志框。

## 版本策略 / Version Strategy

- **核心 CLI**：纯 C++17，不依赖 Qt，可在更老的系统上运行；HTTP 后端 Windows 用 WinINet，其余用 libcurl。
- **GUI**：在系统底线允许时尽量用**最新 Qt 6 LTS**；当 Qt6 官方不再支持该旧系统时，回退 **Qt 5.15 LTS**。
- 分档依据 Qt 官方支持平台文档（2026-11 核对）：
  - Qt 6.5 起：Windows 10 **1809+**、macOS **11+**、iOS **14+**、Android **8.0 (API 26)+**。
  - Qt 5.15：Windows 10、macOS **10.13+**、Android **5.0 (API 21)+**。
- 低于上述 Qt 底线的系统，只能跑 CLI 或走网页方案；现代 Qt 不再提供支持。

## 各平台构建 / Build

### Windows

依赖：CMake、MSVC（或 MinGW）。CLI 使用系统自带 WinINet，无需第三方库。

```bash
cmake -B build -DGDEI_BUILD_GUI=ON
cmake --build build --config Release
```

- 产物：`build/.../gdei-auth.exe`（CLI）、`gdei-auth-gui.exe`（GUI）。
- **Win10 1703 / 1709**：GUI 用 **Qt 5.15**（Qt6 要求 1809+）；**1809 及以上 / Win11** 用最新 Qt 6 LTS。

### Linux

```bash
sudo apt install build-essential cmake libcurl4-openssl-dev   # Debian/Ubuntu
cmake -B build
cmake --build build
```

内核 4.10–4.14 的老发行版建议用发行版自带的 GCC/Clang 编译 CLI；GUI 若需要，用 Qt 5.15 并在同代系统上构建，避免新 glibc 符号。

### macOS

安装 Xcode Command Line Tools 后用上述 CMake 流程；libcurl 随系统提供。

- **macOS 10.13 High Sierra**：GUI 用 **Qt 5.15**（Qt6 要求 macOS 11+）；**macOS 11+** 用最新 Qt 6。

### Android

- **Termux（CLI）**：`pkg install clang cmake libcurl`，再按 CMake 流程编译出可执行文件。
- **Qt for Android（GUI）**：
  - **Android 7.1 (API 25)**：用 **Qt 5.15**；
  - **Android 8.0 (API 26) 及以上**：可用 Qt 6.5+；NDK 版本与官方 Qt 套件保持一致。

### iOS

- iOS 无终端 CLI；只有 GUI 形态，需 Xcode 工程与开发者签名。
- **iOS 11**：尽力用 **Qt 5.15**；**iOS 10.3** 已低于现代 Qt 实际部署底线，不保证可运行；**iOS 14+** 用 Qt 6。

### ChromeOS / Windows Phone

- **ChromeOS（Chrome 55–62）**：普通 Chromebook 不能直接安装 Qt 程序；建议后续补一个 **PWA 网页前端**，或在 Linux/Android 容器里跑 CLI。
- **Windows 10 Mobile 1709**：现代 Qt 已无对应移植，**不提供原生客户端**；仅建议网页或脚本方案。

## 兼容矩阵 / Compatibility

> 范围：列出的最低版本 → 2026-11-21 仍在维护的新系统。CLI 不依赖 Qt，覆盖面比 GUI 更广。

| 平台 | 架构 | CLI | GUI 建议 | 备注 / Notes |
|---|---|---|---|---|
| Windows 10 1703 / 1709 | x64 / ARM64 | ✅ | Qt 5.15 | Qt6 需 1809+ |
| Windows 10 1809–22H2 | x64 / ARM64 | ✅ | Qt 6 LTS | ARM64 交叉编译 |
| Windows 11（–25H2） | x64 / ARM64 | ✅ | Qt 6 LTS | |
| macOS 10.13 High Sierra | x64 | ✅ | Qt 5.15 | Qt6 需 macOS 11+ |
| macOS 11 – 27.x | x64 / ARM64 | ✅ | Qt 6 LTS | |
| Android 7.1 (API 25) | ARM / ARM64 / x86 | ✅（Termux） | Qt 5.15 | |
| Android 8.0 – 15 | ARM / ARM64 / x86 / x64 | ✅（Termux） | Qt 6.5+ | NDK 与 Qt 套件对齐 |
| iOS 10.3 | arm64 | ➖ | ❌ | 低于现代 Qt 部署底线 |
| iOS 11 | arm64 | ➖ | Qt 5.15（尽力） | 需实测签名打包 |
| iOS 14 – 18 | arm64 | ➖ | Qt 6 | |
| Linux 内核 4.10 – 4.14 | x86 / x64 / ARM | ✅ | Qt 5.15（同代构建） | 注意 glibc 符号兼容 |
| Windows 10 Mobile 1709 | ARM | ➖ | ❌ | 现代 Qt 无移植 |
| ChromeOS Chrome 55 – 62 | x86 / x64 | ➖ | ❌ | 建议 PWA 网页前端 |

> 说明：本仓库在 Windows 沙箱中生成源码基准，未在本地交叉编译；请按上表在目标工具链上构建验证。
> Source baseline authored in a Windows sandbox; build on the target toolchain.

## 配置文件 / Config

默认读取当前目录 `config.json`（可用 `--config` 或环境变量 `GDEI_CONFIG` 指定）。字段见 `config.example.json`：

| 字段 | 说明 |
|---|---|
| `username` | 账号 |
| `password_ob` | 混淆后的口令（由 `configure --pass` 生成，不要手填明文） |
| `host` / `ip` | 门户域名 / 备用直连 IP |
| `scheme` / `port` | 协议与端口 |
| `nasip` | NAS (AC) 地址 |
| `healthUrl` | 门户可达性探测地址 |
| `keepaliveIntervalSec` | 保活轮询间隔（秒） |

## 保活机制 / Keep-alive

`keepalive` 命令按间隔执行：

1. 访问 NCSI 探测地址，未被门户重定向且返回 `Microsoft` → 判定已在线；
2. 否则访问 `healthUrl` 确认门户可达，然后自动重新提交认证；
3. 循环直到收到 `Ctrl+C` / 终止信号。

## 字符集 / Encoding

- 核心内部统一 UTF-8；口令先按 UTF-8 取字节再 Base64。
- Windows 命令行参数由 UTF-16 转 UTF-8；POSIX/Android 按 UTF-8 处理。
- 若输入不是合法 UTF-8，按原字节保留，避免二次转码产生乱码。

## 安全说明 / Security

- `password_ob` 是**可逆混淆**（XOR + Base64），仅为避免明文落在配置文件里，**不是强加密**。
- 请勿把 `config.json` 外传或提交到仓库（已在 `.gitignore` 中忽略）。

## 排障 / Troubleshooting

- **登录返回空响应**：本机可能已在线，属正常；断网后再测完整认证流程。
- **提示图形验证码**：连续输错后门户会要求验证码，当前不处理；先用浏览器手动登录一次。
- **连不上门户**：检查 `host/ip/port/nasip`，确认处于校园网可达路径（本机 Realtek 网卡 → Tenda → 校园网）。
- 协议细节与字段含义见 [`docs/PROTOCOL.md`](docs/PROTOCOL.md)。

## 网络测试备注 / Network Notes

- 本机两张可上网网卡：Realtek PCIe GbE 接校园网（上层 Tenda 路由器 NAT）；
  Remote NDIS 用于保持远程会话。断网测试时请勿切断 Remote NDIS，避免远程连接断开。
- 若需要在校园内网（`10.0.0.0/8`）环境下开发，可把网线从学校交换机直插本机，
  获得内网 IP 后再做离线认证实测。

## 许可证 / License

待定（暂未指定 / TBD）。

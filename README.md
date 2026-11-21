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
└── docs/PROTOCOL.md        # 抓包协议逆向说明 / captured protocol notes
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

## 各平台构建 / Build

### Windows

依赖：CMake、MSVC（或 MinGW）。CLI 使用系统自带 WinINet，无需第三方库。

```bash
cmake -B build -DGDEI_BUILD_GUI=ON
cmake --build build --config Release
```

- 产物：`build/.../gdei-auth.exe`（CLI）、`gdei-auth-gui.exe`（GUI）。
- **Windows 7（含无 SP1）**：GUI 请用 **Qt 5.12 LTS**（最后一个官方支持 Win7 的大版本）；
  Qt 6 已放弃 Win7。CLI 用 VS2015 工具集或对应 MinGW 即可。

### Linux

```bash
sudo apt install build-essential cmake libcurl4-openssl-dev   # Debian/Ubuntu
cmake -B build
cmake --build build
```

### macOS

安装 Xcode Command Line Tools 后用上述 CMake 流程；libcurl 随系统提供。

### Android

- **Termux（CLI）**：`pkg install clang cmake libcurl`，再按 CMake 流程编译出可执行文件。
- **Qt for Android（GUI）**：用 Qt 套件部署 `gui/qt`；核心库通过 NDK 编译。

## 兼容矩阵 / Compatibility

| 平台 | 架构 | CLI | GUI | 备注 |
|---|---|---|---|---|
| Windows 7（含无 SP1） | x86 / x64 | ✅ | Qt 5.12 | Qt6 不支持 Win7 |
| Windows 8 / 8.1 | x86 / x64 | ✅ | Qt 5.12 / 5.15 | |
| Windows 10 1509–22H2 | x86 / x64 / ARM64 | ✅ | Qt 5.15 / Qt6 | ARM64 用对应工具链 |
| Windows 11（测试版–25H2） | x86 / x64 / ARM64 | ✅ | Qt6 | |
| Linux（内核 2.7–7 区间） | x86 / x64 / ARM | ✅ | 可选 Qt | 需 libcurl 或自带 HTTP 后端 |
| macOS 10.15–27.1 | x64 / ARM64 | ✅ | 可选 Qt | |
| Android 4–15 | x86 / x64 / ARM / ARM64 | ✅（Termux） | 可选 Qt for Android | 老 Android 注意 NDK 最低版本 |

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

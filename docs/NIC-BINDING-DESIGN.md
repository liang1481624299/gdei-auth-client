# 多网卡识别与门户流量绑定设计 / Multi-NIC Detection & Portal Traffic Binding

## 1. 背景与实测结论

本机同时存在两张活动网卡，且同处 `192.168.0.x` 网段，Windows 源地址/路由选择把门户流量甩到了远程桥接网卡：

| 网卡 | 地址 | 角色 | 绑定该源地址连门户 443 |
|---|---|---|---|
| Ethernet（Realtek） | `192.168.0.100`，DNS `210.38.64.33/45` | 校园网 | **连通** |
| Ethernet 2（Remote NDIS） | `192.168.0.101`，静态 | 云端模型远程桥接 | **超时失败** |

实测事实（2026-11-26）：
- DNS 解析 `nac.gdei.edu.cn -> 210.38.64.39` 正常；外网 NCSI 探测 200 通。
- 系统默认路由下，到门户 `210.38.64.39:443` 的 TCP 被选到 Remote NDIS 网卡，连接失败，客户端表现为 `Operation canceled`。
- 分别绑定两张网卡源地址直连：`192.168.0.100` 连通，`192.168.0.101` 超时。

现有代码现状：
- `NetworkInfoController` 只枚举网卡做展示，结果未回喂给网络请求。
- `PortalClient` / `AccountPortalClient` / `DevicePortalClient` 各自持有 `QNetworkAccessManager`。
- Qt6 的 `QNetworkAccessManager`、`QNetworkRequest`、`QNetworkReply` **均无公开的本地源地址/网卡绑定接口**；底层 `QAbstractSocket::bind()` 存在，但 socket 由 QNAM 内部持有，外部无法在 connect 前调用。

## 2. 设计目标

1. 自动识别哪张活动网卡接着校园网，而不是无脑走 Windows 默认路由。
2. 门户认证/自助/设备三类请求尽量从校园网卡发出。
3. 跨平台（Win/macOS/Linux/Android）；不缓存全校学生信息，不引入服务端。
4. 能力缺失时按降级链退回，UI 显性告知当前走的是哪条路径、是否受限。

## 3. 三层降级链（A → B → C）

传输层按下列顺序尝试，前一层不可用或连不通才退到下一层：

### 层 A：源地址绑定（首选，跨平台，不改系统路由）
- 识别出校园网卡后，门户 TCP socket 在 connect 前 `bind(校园网卡源 IPv4, 0)`，强制从该网卡发出。
- 现状约束：QNAM 不暴露 socket，因此需要在 socket 层实现一套轻量门户 HTTP 传输（TLS、302 跟随、Cookie、超时、编码、请求头基线复用现有 `makeRequest`/`buildForm`/`parsePayload`）。
- 适用：所有平台；不修改本机路由表，不要求管理员权限。
- 状态：**未开工**。QNAM 不暴露 socket，需自建 socket 级轻量门户 HTTP 传输（TLS/302/Cookie/超时/编码/请求头基线复用现有 `makeRequest`/`buildForm`/`parsePayload`），接入三套客户端，QNAM 作回退；属后续主要工作量。识别层已先为其供数。

### 层 B：主机路由强制（A 不可用时的 Windows 兜底）
- 当源地址绑定在当前平台无法实现/失败时，给门户 IP 加一条主机路由（`/32`），显式指定出站网关与网卡，让系统把门户流量送出校园网卡；现有 QNAM 代码基本不改。
- 边界：
  - Windows 需管理员权限；无权限时记日志并退回 C。
  - 路由项为临时项，应用退出时清理；不写持久路由。
  - macOS/Linux 用各自 `route`/`ip route`；Android 不做路由改写。
- 风险：属于本机系统级改动，必须在 UI/日志显性告知，且仅作用于门户主机/IP，不影响其他流量。
- 状态：**已实现，待提权环境实测（2026-11-26）**。新增 `src/platform/RouteBinder.{hpp,cpp}`：`bind(选中网卡, 门户IP)` 先清理上一轮路由；Windows 下先做提权检测（`TokenElevation`），未提权或网关/索引缺失即静默降级 C；提权后经 `route.exe` 加临时 `/32` 主机路由（`mask 255.255.255.255 <校园网关> metric 1 if <索引>`），析构/`release()` 时 `route delete` 清理。控制器暴露 `routeStatus / routeNote / routeBound`，界面徽章标注 Bound / Degraded / Unsupported。非提权冒烟：无 `rootObjects` 报错、未写入任何主机路由。提权下的真实加路由于待管理员会话回归。macOS/Linux 预留，Android 不做路由改写。

### 层 C：识别 + 标注 + 告警（兜底，不动传输层）
- 始终运行网卡识别与可达性探测；在诊断页/日志标明：校园网卡、源 IP、网卡类型、门户是否经该网卡可达。
- 当 A、B 都未生效且门户走错网卡时，UI 给出明确告警（“当前门户流量经远程/虚拟网卡，可能无法认证”），由用户手动切换网卡或选择兜底。
- 状态：**已落地并编译通过（2026-11-26）**。`NetworkInfoController` 暴露 `campusSourceIp/campusInterfaceName/campusReachable/campusMismatch/campusNicList/campusSelected`；`NetworkInfoCard` 新增校园网卡分组（源 IP、可读名、可达性徽章）与不一致告警徽章；四语言（zhCN/zhTW/enGB/ja）文案已补。探测当前在 `refresh()` 主线程同步执行，后续需移到工作线程。

## 4. 网卡识别口径

### 4.1 枚举与过滤
- 仅纳入 `IsUp && IsRunning && !IsLoopback` 的 IPv4 活动地址。
- 排除虚拟/隧道/桥接/远程共享接口，关键词（名称或可读名小写包含）：
  - 现有：`vethernet`, `hyper-v`, `vmware`, `virtualbox`, `vbox`, `wsl`, `tailscale`, `tun`, `tap`, `docker`, `veth`, `vmxnet`, `virtual`, `hyperv`, `warp`, `zerotier`, `openvpn`, `wireguard`
  - 新增：`remote ndis`, `rndis`, `sharing device`, `usb tether`, `bridge`, `wan miniport`, `ppp`, `vpn`

### 4.2 候选打分与可达性探测
对每个未被排除的候选网卡：
1. 记录接口名、可读名、接口索引、IPv4、掩码、MAC、网关（Windows 经 iphlpapi 读取；其他平台留空）、DNS（Windows 读取；其他平台留空）。
2. 绑定该源地址，对门户主机:端口做一次短超时 TCP 连接探测（默认 3s），记录 `portalReachable`。
3. 打分：
   - 探测可达 = 最高分（确认为校园网卡）；
   - 其次为非虚拟、有 MAC 的物理网卡；
   - 链路本地地址、虚拟/隧道/远程共享最低。
4. 选中项 = 最高分候选；同时保留全量网卡列表供 UI 展示。

### 4.3 校园环境判定（与现有网络环境判定对齐）
- 校内真实 LAN：存在探测可达门户的非虚拟网卡。
- VPN 隧道：仅隧道/虚拟网卡可达，或接口命中 VPN 关键词 → 禁用 P2P/共享等高危模块（沿用既有边界）。
- 外网：无任何网卡可达门户。

## 5. 数据模型（NicSelector 输出）

```
NicInfo {
  name            QString   // 接口内部名
  humanName       QString   // 可读名
  index           int       // 接口索引
  ip              QString   // 源 IPv4
  netmask         QString
  mac             QString
  gateway         QString   // Windows 读取，其余平台空
  dns             QStringList
  kind            enum      // Physical / Virtual / Remote / Tunnel / Loopback
  portalReachable bool
  score           int
}

Selection {
  campusNic       NicInfo   // 选中的校园网卡；无则空
  method          enum      // Probe / Score / None
  bindMode        enum      // SourceBind(A) / HostRoute(B) / Advisory(C)
  note            QString   // 人读说明 / 降级原因
}
```

## 6. 代码落点（规划）

- 新增 `src/core/NicSelector.{hpp,cpp}`：枚举、分类、打分、按源地址 TCP 探测、输出 `Selection`。
- `NetworkInfoController`：复用 `NicSelector` 结果，补充 `campusSourceIp`、`campusInterfaceName`、`bindMode`、网卡类型等属性，供诊断页/日志读取。
- 传输层：
  - A：后续新增 socket 级门户传输，接入三套客户端；QNAM 作为回退。
  - B：`src/platform/RouteBinder.{hpp,cpp}`（Windows 优先），负责加/删主机路由，退出清理；无权限降级 C。
- 配置：可选记录“手动指定网卡”覆盖项（默认自动）；不持久化口令。

## 7. 风险与边界

- 双网卡同网段是本机为保留远程调试链路的特殊拓扑；普通单网卡校园环境不应受影响。
- 层 B 改路由表需管理员；失败必须静默降级到 C，不得阻断主流程。
- 探测只做 TCP connect，不发送认证报文、不扫描全网段，符合合规边界。
- 层 A 传输层重写前，现有 QNAM 链路保持可用；识别结果先用于诊断与告警。

## 8. Windows 双网卡同网段：可选解决方案（本机实测 2026-11-26）

### 8.1 本机实测路由事实

`netsh interface ipv4 show interfaces` / `route print -4`：

| 接口索引 | 名称 | 角色 | 接口 metric | 默认路由（0.0.0.0/0）出接口 | 路由 metric |
|---|---|---|---|---|---|
| 18 | Ethernet（Realtek `00 e0 4c …`） | 校园网卡 `192.168.0.100/16` | 25 | `192.168.0.100` | **25** |
| 13 | Ethernet 2（Remote NDIS `34 4b 50 …`） | 云端模型远程桥接 `192.168.0.101/24` | 35 | `192.168.0.101` | 291 |
| 41 | Tailscale | 隧道（应排除） | 5 | — | 隧道不参与校园选路 |
| 20 | vEthernet (Default Switch) | Hyper-V 虚拟交换机（应排除） | 15 | — | 仅本地虚拟网段 |

要点：两张活动网卡同处 `192.168.0.x`，各自带一条经 `192.168.0.1` 的默认路由；Windows 按「最长前缀匹配 → 路由 metric（接口 metric + 路由 metric）」选路。实测校园默认路由 metric 25 已远低于远程 291；若仍走错网卡，通常是远程桥接被配了默认网关/静态路由，或同网段下源地址选择落到了另一张卡。

### 8.2 五种解法（按爆炸半径从小到大）

#### 方案 1：门户 IP 主机路由（对应层 B，最小爆炸半径，推荐自动化）
只给门户目标 IP 加一条 `/32` 主机路由，显式指定校园网关与接口，不影响其他任何流量：

```bat
:: 需管理员；临时项（不加 -p），重启或退出时清理
route add 210.38.64.39 mask 255.255.255.255 192.168.0.1 metric 1 if 18
```

- 优点：仅作用于门户 IP；现有 QNAM 代码基本不改；可在应用退出时 `route delete 210.38.64.39 mask 255.255.255.255` 清理。
- 缺点：需管理员权限；门户用域名时要先 DNS 解析出 IP；网关/接口索引需从适配器配置读取。
- 落地：`src/platform/RouteBinder`，无权限/失败静默降级 C。

#### 方案 2：调低校园网卡接口跃点数 / 调高远程网卡跃点数（影响全部流量）
```bat
:: 需管理员；把校园网卡接口 metric 降到最低，让其默认路由优先
netsh interface ipv4 set interface 18 metric 10
```
- 优点：一条命令即可改变默认选路；无需逐条路由。
- 缺点：影响本机**所有**出站流量，不止门户；若远程桥接还承担特定网段访问，可能被改坏。不适合作为客户端默认行为。

#### 方案 3：Socket 源地址绑定（对应层 A，跨平台，无管理员权限）
门户 TCP socket 在 connect 前 `bind(校园网卡源 IPv4, 0)`，强制从该网卡发出：
- 优点：不改系统路由、不需管理员、跨 Win/macOS/Linux/Android。
- 缺点：Qt6 `QNetworkAccessManager`/`QNetworkRequest`/`QNetworkReply` 无公开源地址绑定接口，socket 由 QNAM 内部持有；需自建 socket 级轻量门户 HTTP 传输。工作量最大。

#### 方案 4：移除远程桥接网卡的默认网关（手动，配置层修复）
远程共享/桥接网卡本就只应服务其本地子网，不应携带默认网关。在「网络连接 → 适配器属性 → IPv4 → 高级」里**去掉远程网卡的默认网关**，或把它配成无网关静态地址；这样非本地流量不再经它发出。
- 优点：治本、不跑命令、不要求路由改写；保留远程调试链路本身。
- 缺点：需用户手动改适配器属性；换环境后可能被桥接软件重新写回。

#### 方案 5：禁用 / 断开远程桥接网卡（最直接的手动兜底）
若当下不需要云端远程调试，直接在「网络连接」禁用 Remote NDIS 网卡，或拔掉对应 USB/共享链路。
- 优点：零配置，杜绝选错。
- 缺点：远程调试链路同时中断；属临时手段。

### 8.3 选型建议

- **客户端自动链路**：优先 A（源绑定，跨平台无权限）→ 退 B（门户 `/32` 主机路由，最小爆炸半径）→ 退 C（识别标注 + 告警）。
- **本机临时绕过**：先试方案 4（去掉远程网卡默认网关）；不行再方案 5（禁用远程网卡）；需要保留远程链路又只修门户时，用方案 1 临时主机路由。
- **不推荐**把方案 2（全局接口 metric 调整）作为应用默认行为，因其影响面超出门户范围。

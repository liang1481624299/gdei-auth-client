# 常见故障排查 / Troubleshooting

> 本页记录校园网接入过程中出现过的真实异常、根因与处置步骤，供客户端排查指引与校内同学自查参考。
> This page records real campus-network incidents, their root causes and fixes, for both client diagnostics and self-service.
>
> **合规红线 / Compliance**：文档只记录机型、网卡型号与通用排查步骤，不记录任何账号、口令、MAC、IP、电话、邮箱等个人敏感取值；这些信息一律由客户端在用户登录后运行时拉取并脱敏。

## 1. Dell 7510（Intel AX201 Wi-Fi 6）校园网半连通异常

### 1.1 现象 / Symptoms

- DNS / DoT / DoH 解析正常。
- HTTP 访问外网（如百度）可以加载，但速度极慢、容易卡死。
- `ping`、`tracert` 第一跳直接超时、全星号 `* * *`。
- `nac.gdei.edu.cn:8080` 认证门户可正常访问。
- 同一 Wi-Fi 环境下其他设备网络正常，仅本机出现该异常。

### 1.2 根因 / Root cause

Intel AX201 的 **Large Send Offload（LSO 大包硬件分片）**、**TCP 校验和卸载**，与校园汇聚交换机 / Portal 网关的报文处理不兼容：

- **小包**（DNS、短小 HTML）：不触发硬件分片，可以正常转发。
- **大 TCP 流**（图片、JS、持续下载）：网卡硬件生成的分片报文被网关丢弃，触发 TCP 反复重传，表现为缓慢、加载不全。
- **ICMP**（ping / tracert）：校园网关本身丢弃 ICMP TTL 回复包，所以路由追踪直接失败（不能据此判断链路不通）。

### 1.3 修复步骤（按优先级）/ Fix steps (by priority)

**第一步：网卡高级属性关闭卸载项**

设备管理器 → Intel Wi-Fi 6 AX201 → 属性 → 高级，全部禁用以下选项：

- Large Send Offload v2 (IPv4)
- Large Send Offload v2 (IPv6)
- TCP Checksum Offload (IPv4 / IPv6)
- ARP Offload、NS Offload

电源管理：取消勾选「允许计算机关闭此设备以节约电源」。

**第二步：管理员 PowerShell 兜底优化**

```powershell
netsh interface tcp set global autotuninglevel=normal
netsh interface tcp set global rss=disabled
```

断开 Wi-Fi 重连，复测网络。

**第三步：仍异常则重装驱动**

重装戴尔官网对应 7510 机型的 AX201 驱动，不要使用 Windows 自动更新推送的通用 Intel 驱动。

### 1.4 验证区分 / Isolation check

- 连接手机热点：热点正常 → 确认是网卡 Offload 与校园网的兼容问题。
- 热点也异常 → 考虑系统 TCP 栈故障或网卡硬件损坏。

### 1.5 对客户端（gdei-connect）的适配要点 / Client implications

- ⚠️ **不要单纯依靠 DNS 解析成功判定网络在线**：存在这种「DNS 通、TCP 吞吐严重受损、半残网络」的异常状态，DNS 正常不代表可正常上网。
- ✅ **连通性检测必须走短超时的 TCP 应用层探测（HTTP 请求），附带超时阈值**；不能只 ping、只查 DNS。ICMP 在本校园网会被网关丢弃，ping 不通不代表离线。
- ✅ **故障提示面板应提供本网卡兼容故障的自查指引**，方便使用 AX201 等网卡的校内同学按 1.3 步骤自行排查。

### 1.6 客户端探针现状对照（2026-11-24 核对）/ Current probe mapping

| 适配要点 | 现有实现 / Current implementation | 结论 |
|---|---|---|
| 在线判定不使用 ping / ICMP | 在线态由 `PortalClient::refreshStatus()` 发起 HTTP GET 探测，代码中无 `ping` / `QHostInfo` / `lookupHost` / DNS 查找判定 | 已符合 |
| 应用层 HTTP 探测 + 超时阈值 | 探测默认目标为 NCSI `connecttest.txt`，`setTransferTimeout(probeTimeoutMs())`（默认 6000 ms）；判定需 `NoError` + 2xx + 响应体非空 + 含 `Microsoft` 特征串 | 已符合 |
| 本地链路与在线态分离 | `NetworkInfoController::available` 仅读取活动网卡标志位（`IsUp`/`IsRunning`），不发请求、不冒充在线；真正在线态仍以 HTTP 探测为准 | 已符合 |
| 半连通大流量受损的识别 | 现有探测目标是极小响应文件；在 LSO 半连通场景下小包可能正常返回，**小探测成功不能排除大 TCP 流被丢包**。吞吐 / 时延受损目前不在在线态判定范围内 | 遗留隐患，见下方待办 |

**遗留待办 / Open items**

- [ ] 评估是否在离线或慢网场景增加一次更大响应体 / 带时延统计的补充探测，用于识别「小包通、大包丢」的半连通状态；该增强会改变在线态判定语义，需产品确认后再实施。
- [x] 故障提示面板接入 1.3 的精简自查指引（四语言文案），定位在主页离线 / 本地不可用警告卡；仅在「有本地链路但校园网离线」时展示，本地网卡未连接时隐藏。

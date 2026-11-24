# UI 规格落地对照与缺口清单 / UI Spec Gap Report

- 规格基线：`docs/UI-DESIGN-SPEC.md`（《gdei-connect 界面设计与开发约束》，新版含问题 P16–P28、页面拆分与高级设置分组校验）
- 落地日期：2026-11-25（CST, UTC+8）
- 落地范围：现有后端字段可支撑的呈现层重构；依赖新增后端能力的工具集与数据解析单列缺口，不在本轮臆造判定。

## 一、本轮已落地（呈现层，2026-11-25）

### 页面拆分与导航（P16、P18、P19、P21）

| 规格条目 | 落地情况 | 落点 |
| ---- | ---- | ---- |
| P16 业务数据与设置分页，移除原「我的」Tab | ✅ Tab 由三改四：概览 / 设备管理 / 仪表盘 / 设置；账号业务数据归仪表盘，偏好与调试项归设置 | `qml/components/AppTabBar.qml`、`qml/Main.qml` |
| P18 设置页分组边界 | ✅ 设置页拆「基础偏好」「系统」两张 Card，各带分组标题，不再平铺一长串 | `qml/pages/ProfilePage.qml` |
| P19 高级设置为设置内子页面，不与语言 / 外观平级 | ✅ 高级设置走跳转子页（栈索引 5），设置页仅列入口行 | `ProfilePage.qml`、`Main.qml` |
| P21 下拉选择与页面跳转控件图标区分 | ✅ 语言 / 外观下拉使用自身指示图标；页面跳转统一用 `chevronRight`，两套不混用 | `ProfilePage.qml` |
| P14 未读红点落点 | ✅ 事件未读角标从原「我的」Tab 移到设置 Tab | `AppTabBar.qml` |

### 仪表盘数据展示（P17、P20）

| 规格条目 | 落地情况 | 落点 |
| ---- | ---- | ---- |
| P17 余额可刷新，失败展示原因与重试，禁止静态「暂无法获取」 | ✅ 余额卡带刷新按钮（busy 转圈）；无值时优先展示 `account.lastMessage` 失败原因，仍无消息再回退不可用文案；不写死静态占位 | `qml/pages/AccountDashboardPage.qml` |
| P20 账号卡片状态标签 | 🟡 已做正常 / 会话失效 / 未知三态标签与对应状态色；「账号受限」「设备超限」标签与本次在线时长待门户会话接口回填（见 G5） | `AccountDashboardPage.qml` |

### 高级设置分组、校验与风险隔离（P22–P28）

| 规格条目 | 落地情况 | 落点 |
| ---- | ---- | ---- |
| P22 四组可折叠面板，默认折叠 | ✅ 编码与安全 / 保活策略 / 超时参数 / 探测地址配置四组 `DisclosureGroup`，默认 `opened:false` | `qml/pages/AdvancedSettingsPage.qml` |
| P23 数值与间隔 / URL 边界校验 | ✅ 保存时统一 `validate()`：所有时间、次数 >0，最小保活间隔 < 最大间隔，URL 匹配 `^https?://.+`；非法输入弹错误提示，不写入配置；数值框配 `IntValidator` 范围 | `AdvancedSettingsPage.qml` |
| P24 高危选项强隔离 + 红色风险提示 | ✅ 「跳过证书校验」归入编码与安全组，开启时下方红色风险文案提示中间人风险 | `AdvancedSettingsPage.qml` |
| P25 探测地址作用说明 + URL 格式校验 | ✅ 外网连通探测地址、账号会话健康检查地址各附小字用途说明；URL 格式纳入保存校验 | `AdvancedSettingsPage.qml` |
| P26 发包编码「自动探测」默认首选 | 🟡 下拉模型 `AUTO/ASCII/GBK/UTF-8`，默认 AUTO 并本地化「自动探测」；后端归一化识别 `AUTO/AUTODETECT`，AUTO 暂按 UTF-8 兜底序列化；真正响应页 charset 嗅探回填未做（见 G4） | `AdvancedSettingsPage.qml`、`src/core/ConfigStore.cpp`、`src/core/FormEncoding.cpp` |
| P27 一键重置默认配置 | ✅ 底部「重置为默认配置」按钮，二次确认后 `resetToDefaults()` 批量复位出厂参数并同步本地暂存 | `AdvancedSettingsPage.qml`、`src/core/ConfigStore.cpp` |
| P28 竖屏折叠适配 | ✅ 四组默认折叠降低单屏密度；数值 / URL 在紧凑断点下列排列，横屏双列铺开 | `AdvancedSettingsPage.qml` |
| 4.10 语言配置与网络编码隔离 | ✅ `requestEncoding` 与 i18n `lang` 为独立配置项，不共用变量；切换语言不影响编码解析 | `src/core/ConfigStore.*`、`src/core/I18n.cpp` |

## 二、前一轮已落地（呈现层，2026-11-24）

| 规格条目 | 落地情况 | 落点 |
| ---- | ---- | ---- |
| 三层状态独立计算、独立渲染，禁止矛盾组合 | ✅ 概览拆为网卡层 / 会话层 / 探测层三行，各自绑定独立字段与状态色，不再混算 | `qml/pages/DashboardPage.qml` |
| 保活总开关只控后台轮询，不发登出请求 | ✅ 保活走 Switch，仅 `startKeepAlive` / `stopKeepAlive`；未登录离线时先弹登录层并复位开关 | `DashboardPage.qml` |
| 登出走按钮样式 + 二次确认，红色仅用于危险操作 | ✅ 登出为 danger 幽灵按钮，沿用二次确认后调 `session.logout()` | `DashboardPage.qml` |
| 网络信息分「本机局域网」「校园认证出口」两组并标注 | ✅ 两组分别附内网 / 出口提示文案；网卡 IP 不再当校园 IP | `qml/components/NetworkInfoCard.qml` |
| 底层调试字段折叠到详情 | ✅ 接口内核名、网关、子网同/不同比对收进「查看详情」折叠，主面只留核心 IP/MAC | `NetworkInfoCard.qml` |
| NAT 下对端 MAC 场景化文案 | ✅ 取不到时显示「NAT 路由环境下无法获取」，不再直出「未提供」 | `NetworkInfoCard.qml` |
| 刷新网卡与连通测试两个按钮分开 | ✅ 「刷新网卡信息」调 `network.refresh()`；「连通测试」调 `auth.refreshStatus()` | `NetworkInfoCard.qml` |
| 四态状态色令牌 | ✅ 绿正常 / 琥珀半残 / 蓝探测中 / 红错误；`info`（探测中蓝）语义别名 | `qml/theme/Theme.qml` |
| 概览诊断入口 | ✅ 「快速连通测试」（原地探测）+「进入网络诊断」（跳转诊断页） | `DashboardPage.qml`、`Main.qml` |
| 概览最近一条告警 + 未读红点 | ✅ 概览展示事件模型最新一条；点击进诊断页 | `DashboardPage.qml` |
| P15 浅色 / 深色预留 | ✅ 三态主题（跟随系统 / 浅色 / 深色）此前已落地 | `Theme.qml` |

## 二、待后端 / 新增能力补齐的缺口

### P1 — 阻断验收清单的核心项

| 编号 | 缺口 | 现状 | 依赖 |
| ---- | ---- | ---- | ---- |
| G1 内置中文字体资源 | ❌ 未打包字体；QML 未显式指定字体族回退链，精简 Linux / Android 系统可能出方框 | 需下载可商用 Noto Sans SC（建议子集化控制体积）入 qrc / CMake 资源，`QGuiApplication` 加载并设置全局字体回退链（内置 → 同语言系统字体 → 通用） |
| G2 半连通（半残）后端判定 | 🟡 UI 已备琥珀 `warning` 色，但后端无独立「半连通」状态字段；现仅有 Online / Offline / Authenticating / Error / Unknown | 需探测层补短超时 TCP 应用层探测 + 大包 / 时延判定，输出半残态；UI 不臆造该判定 |
| G3 诊断页工具集（规格 3.7 八项） | ❌ 诊断页目前仅嵌日志面板；除日志导出、HTTP 探测雏形、编码 codec 雏形外均缺 | 见下表逐项；需新增异步后端任务，结果写入全局日志队列 |
| G4 编码调试入口 UI 与 charset 嗅探回填 | 🟡 后端有编码探测雏形，发包编码已支持 AUTO 归一化（暂 UTF-8 兜底）；诊断页未展示「探测到的门户编码 + 转换结果」，响应页 charset 嗅探回填未做 | 需登录流程回填探测结果并暴露到控制器属性，诊断页展示；AUTO 真正自动识别前不宣称已嗅探 |
| G5 仪表盘账号信息 / 套餐流量 / 在线时长 / 受限状态 | ❌ 仪表盘页已按现有字段呈现并留占位；`AccountPortalClient::parseProfile` 仍为空占位，余额 / 套餐 / 时长 / 账号受限 / 设备超限状态未解析 | 需 8080 主页样本补齐解析；依赖抓包字段 |
| G6 友好网卡名 / 型号 / 驱动版本 | ❌ 后端 `NetworkInfoController` 仅有内核接口名、IP、掩码、MAC；无友好名、型号、驱动字段 | 需平台枚举（Windows 取友好名 / 型号 / 驱动；Linux / macOS 走对应接口）；AX201 命中指引已有雏形 |
| G7 本机网关字段 | ❌ 后端未读取默认网关；详情折叠中网关暂显示「不可用」 | 需读系统路由表取默认网关 |

### G3 诊断工具集明细（规格 3.7）

| # | 工具 | 现状 | 平台约束 |
| ---- | ---- | ---- | ---- |
| 1 | 多维度并行探测：DNS / DoH / DoT / TCP 握手 / HTTP 短请求，识别半连通 | ❌ 仅 HTTP 在线探测雏形 | 跨平台；禁止只 ping / 只查 DNS |
| 2 | 网卡型号 / MAC / 驱动版本，命中 AX201 弹指引 | ❌ 无型号 / 驱动；AX201 文字指引已在概览故障卡 | 跨平台枚举 |
| 3 | TCP Offload 检测 + 一键修复（Chimney / RSS / 自动调谐） | ❌ 未实现 | 仅 Windows，PowerShell `netsh`；非 Windows 隐藏入口 |
| 4 | DNS 对比：内网 DNS vs 公共 `223.5.5.5` 解析延迟 / 异常 | ❌ 未实现 | 跨平台 |
| 5 | 路由追踪 tracert / mtr，可视化节点与丢包位置 | ❌ 未实现 | 跨平台命令封装 |
| 6 | LibreSpeed 测速：上下行 / 延迟 / 抖动，结果入日志 | ❌ 未实现；对接自建测速节点 | 跨平台 |
| 7 | 日志导出 | ✅ 已有 `LogController.exportLog` | — |
| 8 | 编码探测 + 转换结果展示 | 🟡 codec 探测雏形在后端，UI 未接 | 跨平台 |

### P2 — 体验收尾项

| 编号 | 缺口 | 现状 |
| ---- | ---- | ---- |
| G8 骨架屏 / 加载动画系统化 | 🟡 主按钮 busy 有转圈；列表 / 探测等待未统一骨架屏 |
| G9 大屏横屏铺开自适应 | 🟡 现有 `dp` / `sp` / 紧凑断点与内容最大宽度；横屏双列铺开未专门调 |
| G10 图标矢量风格统一 | 🟡 现有 SVG 图标集；未系统对齐 Material 线宽 / 圆角规范 |

## 三、合规边界（本轮与后续均遵守）

- 不做 HTTPS 抓包解密 / 全量流量抓包；仅认证相关调试预览。
- 不做端口 / ARP / 子网扫描；不引入代理、翻墙功能。
- 不新增清单外页面与功能；C++ 核心与 QML 严格分层，UI 改动不破坏认证与探测核心。
- 登出、踢设备等破坏性操作二次确认，默认不误踢本机。

## 四、待拍板口径（沿用历史审计，未在本轮擅自决策）

1. 登录门控：现状为「导航常驻 + 游客可浏览，受保护操作按需弹登录层」；是否改为「未登录强制跳转登录页」待确认。
2. UI 风格：上一轮已按微信风格落地；规格文字另提网页质感，以哪套为准待确认。
3. 诊断工具集平台范围：先做 Windows 可用子集（`netsh` / `ping` / `tracert`），还是四平台一次性铺开，待确认。
4. 字体内嵌体积与来源：Noto Sans SC 全量 / 子集化、许可确认后再入包。

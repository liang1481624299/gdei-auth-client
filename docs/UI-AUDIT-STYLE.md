# 前端体检与参考风格编排对照 / Frontend Audit & Reference Style Mapping

> 生成日期：2026-11-25（CST, UTC+8）。
> 范围：`qml/` 全量静态体检（11 页面 + 29 组件 + 40 图标 + 主题令牌），对照三张附件截图做编排语言拆解与差距分级。
> 附件：图1 为本端 Windows 仪表盘实拍；图2 参考安卓 App 首页；图3 参考安卓 App 个人中心。
> 参考技术栈为安卓 **Kotlin**（Jetpack Compose 或传统 View，未进一步逆向）；本项目技术栈为 **Qt6 + QML + C++**，本文只借鉴视觉编排与信息层级，不照搬其框架、导航结构与业务页面归属。

## 0. 基线约束（不可突破，对照建议均在此范围内）

来源：`docs/UI-DESIGN-SPEC.md`。

- **固定 4 Tab，永久不改**：概览 / 设备管理 / 仪表盘 / 设置；禁止“我的”这类模糊命名；业务数据与设置不得混页。
- **三类状态绝对隔离**：网卡物理层、账号会话态、外网连通态，禁止合并为一个指示灯 / 一条文案。
- **色板固定**：绿 #10B981 正常 / 黄 #F59E0B 半残注意 / 红 #EF4444 断网错误 / 蓝 #3B82F6 加载探测 / 灰 #9CA3AF 次要；状态语义不得互换。
- **图标两套语义**：页面跳转箭头与下拉选择箭头不得混用。
- **高危操作隔离 + 二次确认**；普通用户视图默认干净，技术细节折叠；分组标题强制存在。

## 1. 现状盘点（事实）

### 1.1 规模与目录

| 项 | 现状 |
|---|---|
| QML 文件 | 42 个 / 约 225 KB（`qml/pages` 11、`qml/components` 29、`qml/theme` 1、`qml/icons` 40） |
| 文案字典 | `src/core/I18n.cpp`（1725 行，简体 / 繁體 / English / 日本語四语言内嵌 map） |
| 主题令牌 | `qml/theme/Theme.qml`：深浅色三态、色板、字号阶梯、间距 `sp2…sp12`、圆角、断点 `compactBreakpoint` |
| 资源收录 | 字体 `qml/fonts/NotoSansSC-VF.ttf` 已列入 `CMakeLists.txt:225` QML 模块 RESOURCES；图标在 `CMakeLists.txt:193–233` |
| 页面 | `OverviewPage`、`DashboardPage`、`DevicesPage`、`SettingsPage` 四个主 Tab；子页 `LoginPage`、`AdvancedSettingsPage`、`EventLogPage`、`LogPage`、`AboutPage`、`RemoteDevicesPage`、`DiagnosticsPage` |

### 1.2 已具备的可复用组件

卡片与容器：`BaseCard`、`BaseCollapseGroup`、`PageScaffold`、`AppDialog`、`ToastHost`、`Banner`、`BaseSkeleton`。
按钮与输入：`BaseButton`（primary / secondary / danger / ghost 三态权重）、`BaseSwitch`、`BaseTextField`、`BaseComboBox`。
数据展示：`BaseBadge`、`BaseChart`、`DeviceItem`、`DevicePanel`、`LocalDeviceCard`、`LocalDeviceEditor`、`NetworkInfoCard`、`LogPanel`、`EventListItem`、`UnreadBadge`、`DiagnosticToolCard`。
导航与选择：`AppTabBar`、`AppTabButton`、`SettingRow`、`LangSelector`、`ThemeSelector`、`Icon`、`BrandIcon`。

### 1.3 体检结论速览

- 组件分层清晰、令牌集中、i18n 干净（QML 内未发现绕过字典的硬编码中文 UI 文案）；状态隔离与分组规范在页面层基本落实。
- 主要问题集中在：**跳转行重复内联**、**少量色值绕过令牌**、**参考风格里的几类编排单元尚无对应组件**。

## 2. 体检问题清单（分级 + 绑定位置）

### 2.1 P1 一致性 / 可维护性（纯重构，不改交互）

| 编号 | 现象 | 位置 | 建议 |
|---|---|---|---|
| F1 ✅ | 跳转入口行在两处各内联了一份几乎同构的 `Component`：左图标 + 标题 + 副标题 + 右箭头按钮，hover / 按下态写法重复 | 原 `qml/pages/SettingsPage.qml`（`entryRow`）；原 `qml/pages/DevicesPage.qml`（`navEntryComp`） | **已落地第一批**：抽共享组件 `qml/components/NavEntryRow.qml`（属性：title / subtitle / iconName / badge / onClicked），两页改为引用；组件已登记进 `CMakeLists.txt` |
| F2 ✅ | 色值绕过 `Theme` 令牌硬编码在组件内 | `BaseSwitch.qml` 关闭轨 / 圆点；`BaseButton.qml` 危险按钮白字；`Banner.qml` 背景（`Theme.qml` 已有 `bannerBg` 却未复用） | **已落地第一批**：Theme 新增 `trackOff` / `onSolid` 令牌；`Banner` 复用 `bannerBg`。残留：`BaseChart` 网格线 rgba、`BrandIcon` 品牌色（属例外，见 F3） |
| F3 | 品牌平台图标色硬编码（Windows 蓝 / Android 绿 / Debian 红等） | `qml/components/BrandIcon.qml:41–47` | 属品牌专属色，**保留**，不纳入主题令牌；仅登记说明 |

### 2.2 P2 编排组件缺口（对照参考风格，纯展示组件，不接新业务数据）

| 编号 | 缺口 | 参考出处 | 现状 | 建议新增 / 扩展 |
|---|---|---|---|---|
| G1 🟡 | 轻量文字链（分组头右侧“查看全部 / 查看课表”类纯主色文字 + 可选箭头，无背景无边框） | 图2「查看课表 / 查看全部」 | ✅ 组件 `TextLink.qml` 已建；`BaseCard.actions` 可直接放置；`PageScaffold` 标题区右侧动作槽仍未加 | 后续给 `PageScaffold` 标题区加可选 `actions` 槽；页面接入待第三批 |
| G2 🟡 | 宫格导航（N 列等宽图标方块 + 可选下方文字标签） | 图2「校园服务」四宫格 | ✅ 组件 `IconGrid.qml` 已建（列数随宽度断点自适应、等宽圆角方块 `field` 底 + 浅色 `Icon` + 标签，`activated(index)` 信号）；当前无真实服务入口，未挂页面 | 后续有真实服务入口时传 `model: [{icon, label}]`；不伪造功能 |
| G3 🟡 | 等分统计条（单卡内 N 列均分，数字在上粗体、标签在下灰色，列间竖细分隔线） | 图3「关注 2 / 粉丝 2 / 好友 1」 | ✅ 组件 `StatStrip.qml` 已建（`model: [{value, label, tint}]`，竖分隔线用 `border` 色，空值占位 `-`）；仪表盘余额 / 套餐仍是两张独立 `BaseCard` | 后端资料就绪后，仪表盘余额 / 在线时长 / 在线数可改用本组件 |
| G4 🟡 | 键值行（左灰色标签，右取值 + 可选右箭头，行间分隔线） | 图3「昵称 / 生日 / 院系 / 专业…」资料卡 | ✅ 组件 `KeyValueRow.qml` 已建（label / value / chevron 可选 / clickable 可选，分隔线由调用方放置）；`SettingRow` 仍是图标 + 标题 / 描述范式 | 后续账户资料展示复用（数据仍待后端 `parseProfile` 补齐） |
| G5 🟡 | 功能行 leading 图标底色方块 | 图3 各行左侧圆角深底方块包浅色图标 | `SettingRow` 已加可选 `iconContainer`（第一批落地，默认 false 保持裸图标）；`NavEntryRow` 暂仍裸图标 | `SettingRow` 方块用 `field` 底 + `radiusControl`，图标 `secondaryLabel` 色；`NavEntryRow` 是否同步加方块留待第二批页面编排时定 |
| G6 🟡 | 首字头像（圆形主色底 + 用户名 / 姓名首字，替代固定人形图标） | 图3 圆形「林」字头像 | ✅ 组件 `AvatarBlock.qml` 已建（首字大写、`primaryFill` 底 + `primary` 首字、空名回落 `person` 图标）；`DashboardPage` 仍是 48dp 圆块 + 固定 `person` 图标 | 后端资料就绪后替换现有头像；首字来源与脱敏口径见 §5 待拍板 |
| G7 | 头部多层级 KPI 堆叠（display 粗体主标题 + 主色副标题 + 大字核心数字 + 灰色辅助行） | 图2「早上好… / 绿色日期 / 今日 1 节课 / 校园卡 ¥52.50」 | `PageScaffold` 标题区仅 title + subtitle 两行；无大字号 KPI 位 | 在页面内容区用现有 `Theme` 字号阶梯（`fontTitle1/2/3`、`fontHeadline`）组合头部块；不改动 `PageScaffold` 通用标题 |
| G8 | 信息横卡（左侧固定宽数据列 + 竖色条分隔 + 右侧主副文案） | 图2 课表卡（左 14:00 / 5-8 + 绿竖条 + 右课程名 / 地点老师） | 设备行是「徽章 + 类型 + MAC / 时长」两行堆叠，无左数据列 + 竖条范式 | 可选增强 `DeviceItem` 或新增 `DetailRow`；非必需，优先级低于 G1–G5 |
| G9 🟡 | 图标素材缺口 | 图2 毕业帽 / 日历 / 帮助 / 书本；图3 搜索 / 气泡 / 手机 / 邮件 / 盾牌对勾 / 云下载 / 反馈 / 调色板 / 退出 | 现有 32 枚 SVG 未覆盖上述语义；本轮核对后暂不补（多对应未接入的 CAS 门户 / 未来服务） | 待第三批确定真实入口后，按需补齐线性 SVG（`stroke` 风格与现有 1.75 线宽一致）；未接真实功能前不占位塞入口 |
| G10 | 宽屏 / 横屏铺开策略 | 参考为移动端单列 | `PageScaffold.contentMaxWidth` 720 / 960 居中，超宽屏两侧留白 | 属响应式评估项，先记录；桌面端当前居中单列可接受，不擅自改布局阈值 |

### 2.3 P3 规范级决策（须拍板，本轮不擅自改）

| 编号 | 事项 | 说明 |
|---|---|---|
| D1 | 主色是否向参考图的薄荷绿靠拢 | 参考图主色为薄荷绿系；本项目 `primary` 为 sky 蓝（`Theme.qml:54`），绿色 `success` 专用于“正常 / 在线”。规范色板把绿绑定为正常状态，**不能直接拿绿当主色替换 primary**；是否调整属色板规范级决策 |
| D2 | Tab 数量 / 命名 | 参考为 3 Tab（首页 / 资讯 / 我的）；规范固定 4 Tab 且禁用“我的”，**不可照搬** |
| D3 | 页面分组归属 | 参考图3 把资料、社交、账号功能、更多服务、退出登录混在一页；规范禁止业务数据与设置混页。可借鉴其中的**行 / 卡片单元**，资料归仪表盘、偏好与工具归设置 |

## 3. 参考风格编排拆解（模式 → 现状对照）

| # | 编排模式 | 参考视觉要点 | 我们现状 | 差距 |
|---|---|---|---|---|
| M1 | 身份头部 | 圆形首字头像 + 粗体姓名 + 灰色附属行（用户名 / IP 属地） | `DashboardPage` 有 48dp 圆块 + 固定 `person` 图标 + 账号名 + 状态徽章 | 缺首字头像（G6）；附属行字段待后端资料补齐 |
| M2 | 多层级标题 | 超大粗体主标题 + 主色副标题 + 大字 KPI + 灰辅助行 | 仅标题 + 副标题两行 | 缺 KPI 堆叠位（G7） |
| M3 | 分组头 | 左粗体标题 + 灰色小注，右侧主色文字链 | `BaseCard` 有 title / subtitle，右侧 `actions` 只放按钮 | 缺文字链单元（G1） |
| M4 | 列表卡片（可点行） | 多行行内左文右箭头，行间 1px 分隔线，整行可点 | `SettingsPage` / `DevicesPage` 跳转行 + 分隔线已具备 | 组件重复（F1），抽离后对齐 |
| M5 | 键值行 | 左标签灰字、右值 + 箭头 | 无对应范式 | 缺 `KeyValueRow`（G4） |
| M6 | 功能行（图标方块） | 圆角深底方块包图标 + 标题 + 右箭头 | 裸 `Icon` + 标题 + 箭头 | 缺图标底色方块（G5） |
| M7 | 宫格导航 | N 列等宽图标块 | 无 | 缺 `IconGrid`（G2） |
| M8 | 统计条 | N 列均分 + 竖分隔，数字上标签下 | 无 | 缺 `StatStrip`（G3） |
| M9 | 信息横卡 | 左固定数据列 + 竖色条 + 右主副文案 | 两行堆叠式设备卡 | 可选增强（G8） |
| M10 | 高危独立卡 | 红色图标方块 + 红字，与普通行隔离 | `LocalDeviceCard` 菜单内删除项红字；`AppDialog` 二次确认 | 规范已满足；参考图的整行红色独立卡可作为退出 / 注销行的视觉参照 |
| M11 | 底部导航 | 选中主色填充图标 + 文字，未选中灰 | ✅ `AppTabButton` 选中态已加 `primaryFill` 浅底圆角方块包 primary 图标，未选保持裸 muted 图标；四 Tab 结构与职责不变 | 已落地本轮；选中填充变体受现有线框 SVG 素材限制，未做实心图标变体 |
| M12 | 标题栏右侧动作图标组 | 历史页标题右侧刷新 / 统计 / 垃圾桶三个线框动作图标 | 页面动作走 `BaseCard.actions` 槽位（`BaseButton`），无纯图标动作组 | 可选：后续需要纯图标动作时抽 `IconActionButton`；当前无对应业务，不占位 |
| M13 | 记录横卡（左双行 + 中标签 + 右两列数值） | 测速历史：左日期/时间双行，中网络名，右下载/上传两大数字 | 设备行 `portalRowComp`：badge + 类型 + 本机标记一行，MAC + 时长一行 | 视觉节奏可借鉴（独立卡 + 左右分区）；**速率/日期字段本项目设备列表不存在，不伪造列**；底部「共 N 台」计数可用真实 `devices.length`，待接入时补 i18n |
| M14 | 可排序列头行 | 日期 / 网络 / ↓Mbps / ↑Mbps 列头，选中列主色 | 设备列表无排序需求，后端顺序保证在线置顶 | 不迁移排序交互；不引入列头 |
| M15 | 未登录身份头 | 圆形人形图标 +「立即登录」 | 仪表盘 `person` 圆块 + 账号名 / 未登录文案；`AvatarBlock` 已备待接 | 首字来源与脱敏口径仍挂起（§5 D2）；未登录回落人形图标，已有组件覆盖 |

## 4. 分批落地建议

### 第一批：纯重构 / 令牌收敛（不改交互、不接新数据）— ✅ 已完成并通过 Release 增量构建

1. ✅ 抽 `NavEntryRow.qml`，合并 `SettingsPage.entryRow` 与 `DevicesPage.navEntryComp`（F1）。
2. ✅ 硬编码色值入 `Theme`：开关关闭轨 / 圆点 / 按钮白字 / 横幅背景（F2，复用 `bannerBg`，新增 `trackOff` / `onSolid`）。
3. ✅ `SettingRow` 增加可选图标底色方块属性 `iconContainer`（默认 false，外观不变；G5 前置）。

验证：Ninja Release 增量链接通过，`gdei-auth-client.exe` 重新生成；`qmllint` 改动文件无 error 级告警（`app.*` unqualified 为项目既有单例惯例）。

### 第二批：补展示型编排组件（组件先行，数据仍走现有后端）— 🟡 组件已落地，页面接入待第三批

1. ✅ `TextLink`（G1）、`IconGrid`（G2）、`StatStrip`（G3）、`KeyValueRow`（G4）、`AvatarBlock`（G6）五个组件已新建并登记进 `CMakeLists.txt`，Release 增量构建通过、`qmllint` 无 error。
2. 缺口线性 SVG（G9）本轮核对后**暂不补齐**：所列图标多对应尚未接入的 CAS 门户 / 未来服务，按"未接真实功能前不占位塞入口"处理；待第三批确定真实入口后再按 1.75 线宽风格补。新组件本身不写死中文，文案 key 待页面接入时在 `src/core/I18n.cpp` 四语言字典同批添加。
3. 组件暂未挂到页面：服务宫格 / 统计条 / 资料键值 / 实名头像均依赖后端数据就绪；**无后端数据的位置保持空态 / 占位，不伪造内容**。页面编排对齐列入第三批。
4. ✅ 组件静态自查并修复：`StatStrip` / `IconGrid` 空 model 时整体隐藏（不渲染空框）；`TextLink` 空文本隐藏；`KeyValueRow` 右箭头仅在可点时显示（避免不可点行给跳转暗示）、空值回落占位符；qmllint 无 error（仅项目既有的 delegate 非限定访问惯例警告），Release 增量构建链接通过。

### 第三批：页面编排对齐（依赖后端数据就绪）

1. 仪表盘：身份头部（头像 + 姓名 + 附属行）+ 统计条（余额 / 在线时长 / 在线数）+ 资料键值行；依赖账户资料解析补齐（见 `docs/BUTTON-INVENTORY.md` B1/B2/B3/B10）。
2. 概览：头部 KPI 堆叠（核心状态数字 + 辅助行），三态卡片层级不变。
3. 设备管理：借鉴测速历史记录卡的独立卡 + 左右分区节奏；底部补「共 N 台设备」计数行（真实 `devices.length`，接入时在 `I18n.cpp` 四语言字典补 key）。速率 / 日期时间戳 / 排序列本项目无对应字段，不迁移、不伪造。

### 测速类参考 App 对照结论（第二批补充截图）

- **已落地**：底部导航选中态图标 `primaryFill` 浅底圆角方块（M11）。
- **可借鉴、待数据 / i18n**：记录横卡左右分区节奏、底部计数行（M13）。
- **不迁移**：下载 / 上传速率大数字与日期时间戳列（无字段）；可排序列头（M14，无排序后端）；批量清空 / 垃圾桶（批量踢设备属红线，见任务 #85）；赞赏 / 应用评分 / 观看广告 / 隐私与个人信息清单 / 测速单位分段（业务不符或属独立合规模块）。

### 待排查缺陷（非本轮风格改动引入）

- **宽屏 / 最大化下底部导航被挤出可视区**：默认 400×720 下 `AppTabBar` 正常可见；窗口最大化后概览长内容（最近提醒列表按 `contentHeight` 全量展开）把页面根高度撑过可视区，TabBar 落到可视区外。需在 `Main.qml` / `PageScaffold` / `OverviewPage` 提醒列表处核对 `Layout.fillHeight` 与 Flickable 高度约束；与 G10 宽屏评估一并处理，本轮未擅自改布局阈值。

## 5. 待拍板项

1. **主色取向（D1）**：是否在规范层评估 primary 由 sky 蓝调整为绿色系；若调整需同步重检状态色语义（绿=正常不得与主色冲突）。
2. **头像首字来源（G6）**：取登录账号名首字符，还是门户实名首字；实名属敏感信息，运行时拉取、不落仓库、日志脱敏，且需后端资料就绪。
3. **宫格用途（G2）**：`IconGrid` 预留承载哪些未来服务入口；当前 NAC 客户端无对应业务，先不挂真实按钮。
4. **横屏 / 宽屏阈值（G10）**：是否放宽 `contentMaxWidth` 或做多列排布，需桌面端与移动端一起评估。

## 6. 红线（本次风格对齐中不得违反）

- 不新增绕过 `app.*` 门面的跨层网络请求；诊断 / 探测结果只经 `reportDiagnostic` 入事件队列。
- 不伪造余额、套餐、时长、设备等数据；后端取不到时保持“暂无法获取 / 未统计”空态。
- 高危操作（下线、删除绑定、重置、证书跳过）保留二次确认，红色与普通操作分区。
- 敏感字段（实名、电话、邮箱、MAC、明细、会话序列号）运行时展示，不落仓库、日志脱敏。
- 不照搬参考 App 的社交 / 教务 / 消息模块；那属独立 CAS 融合门户方向（见 `docs/BUTTON-INVENTORY.md` F 节）。

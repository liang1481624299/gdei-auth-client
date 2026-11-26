# QML 前端设计审查报告（对标 Apple HIG）

- 审查日期：2026-11-26
- 项目：gdei-connect（Qt6 + C++ + QML 跨平台原生客户端，Windows / macOS / Linux）
- 审查范围：`qml/pages/DevicesPage.qml`、`qml/pages/DiscoverPage.qml`、`qml/pages/MinePage.qml`，共用组件、`qml/theme/Theme.qml`、底部导航 `AppTabBar`。
- 评价基准：Apple HIG 三原则 —— Clarity（清晰）、Deference（克制）、Depth（层次）。控件形态遵循桌面平台惯例，不硬塞 iOS 专属控件。
- 边界：只评价界面呈现与交互，不评价业务逻辑正确性；对比度按 WCAG 相对亮度公式实测，主观结论均落到可量化依据。

> 说明：代码库中的“发现页”是网络工具**入口列表**，不是内容卡片流，因此“图片比例 / 筛选排序 / 分页加载”等卡片流条款在本项目 N/A。设备页为云端只读列表，本机连接控制在首页 `OverviewPage.qml`。本轮 UI 层未发现阻断主流程的 P0 问题；运行时崩溃 / 无响应属于逻辑与运行时问题，不在本报告结论内。

## 一、全局 / 主题 / 导航

### G1 浅色态主色文字对比度不达 AA 【P1 · 可访问性 · 局部样式调整】
- 位置：`theme/Theme.qml` `primary` 浅色 `#0EA5E9`；用于链接、选中态标签/图标、箭头、主色文字。
- 现象（实测）：`#0EA5E9` on `#FFFFFF` = **2.77:1**，低于正文 AA 4.5:1。主色填充按钮上为白字不受影响，但所有主色文字/图标/链接在浅色态不达标。
- 违反原则：Clarity；WCAG AA 4.5:1。
- 影响：可访问性、浅色态可读性。
- 修改：浅色 `primary` 调深至 `#0369A1`（实测 5.93:1），填充按钮继续用白字；`primaryHover` 同步取更深一档。

### G2 三级文字色在深浅两态都偏浅 【P1 · 可访问性 · 局部样式调整】
- 位置：`Theme.qml` `tertiaryLabel`（浅 `#9CA3AF` / 深 `#6B7280`），用于空值“—”、占位、脚注。
- 现象（实测）：浅色 `#9CA3AF` on 白 = **2.54:1 FAIL**；深色 `#6B7280` on `#111318` = **3.84:1**，仅过大字号 AA，11–12dp 正文不足 4.5:1。
- 违反原则：Clarity；WCAG AA。
- 影响：可访问性、次要信息可读性。
- 修改：浅色 tertiary 抬到 `#6B7280`（4.83:1）；深色 tertiary 抬到 `#9CA3AF`（7.32:1）；旧两档仅留给装饰/禁用。

### G3 浅色态横幅文字与深色底对比度破裂 【P1 · 可访问性 · 局部样式调整】
- 位置：`Theme.qml` `bannerBg=#0F172A`（固定深色）；`components/Banner.qml` 正文用 `th.text`。
- 现象（实测）：浅色态正文 `#374151` on `#0F172A` = **1.73:1 FAIL**；深色态 `#E5E7EB` on `#0F172A` = 14.42:1 正常。
- 违反原则：Clarity；主题派生一致性。
- 影响：浅色态全局错误/警告横幅几乎不可读。
- 修改：`bannerBg` 按 `isDark` 派生（浅色 `#F9FAFB`、深色 `#0F172A`），文字沿用 `th.text`，两态均达标。

### G4 桌面客户端默认开手机竖屏比例 【P2 · 平台惯例 · 局部样式调整（需复检断点）】
- 位置：`Main.qml` 默认 `360×640`、最小 `320×480`（9:16）。
- 现象：跨平台桌面客户端在桌面开窄竖窗，宽屏被强制居中限宽，两列网格被压成单列。
- 违反原则：Deference（控件/窗形遵循所在平台惯例）。
- 影响：美观、桌面可用性、布局密度。
- 修改：桌面默认 ~`960×640` 宽窗、最小宽 ~360；竖屏比例仅保留给移动端；复检 `compactBreakpoint`(600)/`narrowBreakpoint`(320) 列切换。

### G5 选择器绕过间距/尺寸令牌 【P2 · 一致性 · 局部样式调整】
- 位置：`LangSelector.qml`、`ThemeSelector.qml`：`implicitWidth:148`、`leftPadding:12`、`rightPadding:34` 裸像素；`BaseComboBox.qml` 用 `sp4`/`inputHeight` 令牌。
- 违反原则：Depth/规范一致性（8pt 网格、令牌单一来源）。
- 影响：同款下拉在不同页内边距/宽度不齐。
- 修改：内边距改 `th.sp4`/`th.sp6`，宽度交布局，不再写死 148/12/34。

### G6 选中态配色两套语义 【P2 · 一致性 · 局部样式调整】
- 位置：`LangSelector.qml`、`ThemeSelector.qml` 当前行用 `greenFill`/`success` 文字；`BaseComboBox.qml` 用 `primaryFill`/`primary`。
- 现象：同为“当前选中”，一处绿、一处主蓝。
- 违反原则：一致性（同类操作同一视觉编码）。
- 影响：用户误读选中含义。
- 修改：下拉选中态统一 `primaryFill` + `primary`；绿色仅留给“在线/成功”。

### G7 自定义可点项缺键盘焦点与无障碍标注 【P1 · 可访问性 · 局部样式调整】
- 位置：`OverviewPage.qml` 大圆钮 `TapHandler`；`StatusListCard.qml` `TapHandler`；`MinePage.qml` 整卡 `TapHandler`。
- 现象：手写 `TapHandler` 无 `Accessible.role/name`、无 `focusPolicy` 与键盘激活；主操作项不可被读屏/键盘到达。
- 违反原则：Clarity/可访问性（桌面需键盘可达）。
- 影响：可访问性、键盘操作可用性。
- 修改：主操作自定义项补 `Accessible{ role: Accessible.Button; name: ... }`、`focusPolicy: Qt.StrongFocus`，绑定 `ActiveFocusOnTab` + 回车/空格触发同一槽函数。

## 二、设备页 `DevicesPage.qml`

### D1 列表无加载骨架/进行中占位 【P1 · 状态完整性 · 局部样式调整】
- 位置：内嵌 `ListView`；`busy` 仅反映在头部刷新按钮。`BaseSkeleton` 已存在并在 `MinePage.qml` 余额卡使用，设备列表未用。
- 现象：首次已登录拉取时，列表要么空文案、要么旧卡片；刷新期间无骨架/占位，无法区分“加载中”与“空列表”。
- 违反原则：状态完整性（加载/空/错误/成功四态齐全）。
- 影响：可用性、认知负担。
- 修改：`authenticated && busy && count===0` 时显示 2–3 行 `BaseSkeleton`；刷新保留旧卡时整卡降 `disabledOpacity`，避免新旧闪烁。

### D2 失败态无内联重试与引导 【P2 · 状态完整性 · 局部样式调整】
- 位置：`lastMessage` 仅一条 `secondaryLabel` 文案；重试只能找头部刷新按钮。
- 现象：拉取失败只有一句原因，无“重试”按钮、无下一步建议；对照余额卡已有内联重试。
- 违反原则：可用性（错误态可操作）。
- 影响：可用性。
- 修改：失败块对齐余额卡：文案 + 次要按钮“重试”调 `refreshDevices()`，`busy` 时禁用。

### D3 内嵌列表关闭自滚动，依赖外层 Flickable 【P2 · 操作性 · 局部样式调整】
- 位置：`interactive:false`，`implicitHeight: portalList.contentHeight`。
- 现象：内层 ListView 不自行滚动，整列撑开交外层 `Flickable`；模型每次刷新重建 delegate 时，存在滚动位置/选中态跳动风险。
- 违反原则：Depth 稳定、列表不抖动。
- 影响：操作性、视觉稳定。
- 修改：保持外层滚动，但固定 delegate 高度、避免刷新整表重建模型；若长列表，改为内层可滚动并给固定高度。

### D4 只读卡片带 hover 提亮，伪可点 【P2 · 一致性 · 局部样式调整】
- 位置：`DeviceCard.qml` `HoverHandler`；`BaseCard.qml` `hoverable` 默认 `true`；云端设备卡为只读。
- 现象：不可点的只读卡悬停仍提亮，视觉上像可进入。
- 违反原则：Deference（视觉边界与可点范围一致）。
- 影响：可用性误判。
- 修改：`DeviceCard` 增加 `interactive` 属性控制 hover/press 反馈；只读云端卡传 `interactive:false`。

## 三、发现页 `DiscoverPage.qml`

### C1 工具入口单一、缺分组与状态位 【P2 · 信息层级 · 局部样式调整】
- 位置：仅一张卡、一条 `NavEntryRow`（诊断工具）。
- 现象：页面密度很空；无工具不可用/加载失败占位，也无第二组入口。当前后端仅一个真实工具，属功能现状，结构上未为后续工具预留分组。
- 违反原则：信息层级、状态完整性。
- 影响：美观（空旷）、后续扩展一致性。
- 修改：预留“网络工具/链路诊断”分组容器；工具后端不可用时给空态文案 + 重试；不伪造测速/扫描类入口。

## 四、我的页 `MinePage.qml`

### M1 登录按钮文案写成“退出登录”，动作却是登录 【P1 · 操作性 · 局部样式调整】
- 位置：门户验证码登录卡主按钮 `text: profile.logout`，`onClicked: app.account.loginWithCode(...)`。
- 现象：按钮标签为“注销/退出”，实际提交验证码登录；用户可能误点以为在退出。
- 违反原则：文案清晰（动词与动作一致）、避免误操作。
- 影响：操作性、误操作风险。
- 修改：文案改用登录语义键（`devices.login`），退出动作只出现在底部危险区。

### M2 验证码底板硬编码白色，深色态破主题 【P2 · 一致性 · 局部样式调整】
- 位置：`MinePage.qml` 验证码 `Rectangle color:"white"`；同问题 `DevicePanel.qml`。
- 现象：深色模式下出现固定白底，与卡片割裂；颜色绕过 `Theme` 令牌。
- 违反原则：主题派生一致、Depth。
- 影响：美观、深浅一致性。
- 修改：`Theme` 增加 `captchaSurface` 令牌（图片确需白底则令牌内固定白、外围边框/间距走主题），两处引用同一令牌。

### M3 验证码卡片一行三按钮，窄屏拥挤 【P2 · 布局/操作性 · 局部样式调整（该卡片行小重构）】
- 位置：验证码卡内“刷新” + 弹性间隔 + “取消” + 主“登录”挤在同一 `RowLayout`；窗宽 360 时三个按钮标签易被压缩/换行。
- 违反原则：信息层级（主操作独占、次要操作退位）、窄屏适配。
- 影响：操作性、i18n 长文案溢出。
- 修改：主登录按钮整行通栏置底；“刷新验证码/取消”放上方次要行；参照 `LocalProfilesCard.qml` 纵向堆叠。

### M4 破坏性操作视觉权重与普通次要按钮相同 【P1 · 一致性/防误触 · 局部样式调整】
- 位置：`MinePage.qml` 注销本地配置 `type:"secondary"` + trash；`LocalProfilesCard.qml` 删除同样 `secondary`；`DevicePanel.qml` 危险“退出会话”为 `danger` 实心但无二次确认。
- 现象：行内删除/注销与“选择/编辑”同为细轮廓次要按钮，红色只出现在弹窗确认；`DevicePanel` 清会话危险动作直接执行，无 `AppDialog`。
- 违反原则：破坏性操作隔离 + 二次确认；风险语义用颜色/位置显性表达。
- 影响：误操作风险。
- 修改：① 新增 `danger-ghost` 行内样式（红字 + 红描边/红低透填充，非实心红）用于行内删除/注销；② `DevicePanel.clearSession()` 接 `AppDialog` 确认，说明会释放当前自助门户会话；实心 `danger` 仅保留给弹窗确认按钮。

### M5 账号头部双重点击目标，整卡 TapHandler 无无障碍语义 【P2 · 操作性/可访问性 · 局部样式调整】
- 位置：整卡 `TapHandler→goLogin`，右侧箭头；块内又有主按钮 `goLogin`。
- 现象：同一动作两个可点区域；整卡点击未暴露按钮角色/名称；箭头暗示可跳转但 AT 不可达。
- 违反原则：一致性、可访问性。
- 影响：操作性。
- 修改：头部 `TapHandler` 补 `Accessible` 角色与名称；保留单一明确按钮为主触发，整卡仅作视觉引导。

### M6 KPI 大数字未设换行/省略，窄屏可能溢出 【P2 · 可读性 · 局部样式调整】
- 位置：余额、套餐流量 `fontDisplay`(40dp) 加粗，未设 `wrapMode`/`elide`。
- 现象：长额度串（如 “1024.5 GB”）在 320 窄窗可能溢出卡片。
- 违反原则：Clarity、窄屏适配。
- 影响：美观、可读性。
- 修改：数字 `wrapMode: Text.Wrap` 或 `elide: ElideRight`；窄断点下 `fontDisplay` 降一档。

### M7 两类行标题字号/字重不统一 【P2 · 一致性 · 局部样式调整】
- 位置：`SettingRow.qml` 标题 `fontHeadline`(16) `regular`；`NavEntryRow.qml` 标题 `fontBody`(14) `medium`；两者在“系统工具”分组混用。
- 违反原则：同类条目排版一致。
- 影响：视觉层级混乱。
- 修改：统一行标题规范：14dp medium + 脚注描述；16dp 仅留给卡片标题。

## 五、共用组件补充

### S1 非交互状态卡带按压缩放反馈 【P2 · 一致性 · 局部样式调整】
- 位置：`StatusListCard.qml` 始终 `scale` 按压反馈并 `emit clicked()`；`OverviewPage.qml` 三张卡均未接 `onClicked`。
- 现象：不可点的状态卡按压会缩放，像可点条目。
- 违反原则：视觉边界与可点范围一致（Deference）。
- 影响：可用性误判。
- 修改：增加 `interactive` 属性（默认 `false`），仅在外部连接 `clicked` 时启用 TapHandler 与缩放。

### S2 首页主按钮进行中仅降透明度，无明确加载指示 【P1 · 状态完整性 · 局部样式调整】
- 位置：`OverviewPage.qml` `session.busy` 时 `opacity:0.6`，无 spinner/进度文案；下方标签仍只显示“连接/断开”。
- 现象：异步连接/断开期间，用户只看到按钮变暗，看不到“正在连接…”的明确进行态。
- 违反原则：异步操作有明确进行中提示。
- 影响：可用性、重复点击风险。
- 修改：busy 时按钮内切换为进度指示，下方标签显示 `home.connecting`/`home.disconnecting`，并禁用再次触发。

### 弹窗 `AppDialog.qml`（合规项）
模态遮罩、内容可滚动、取消/确认、`destructive` 实心红、ESC 关闭、宽度受限——结构完整，二次确认模式可复用，无需结构性改动。

## 六、页面评分（5 分制）

| 页面 | 清晰度 Clarity | 美观度 Beauty | 可用性 Usability | 一致性 Consistency |
|---|---|---|---|---|
| 设备页 Devices | 3.5 | 3.5 | 3.0 | 4.0 |
| 发现页 Discover | 3.5 | 3.5 | 3.0 | 4.0 |
| 我的页 Mine | 3.5 | 3.5 | 3.0 | 3.5 |

评分依据：主题令牌体系（8pt 间距、字号阶梯、圆角、状态色、44 触控目标）底子完整，组件复用度高，一致性偏上；主要扣分在浅色态对比度、加载/错误/进行态不完整、破坏性操作视觉隔离不足，以及少量令牌外硬编码。

## 七、最优先改的 3 项

1. 修浅色态对比度（G1/G2/G3，P1 可访问性）：主色文字调深、三级文字抬对比度、横幅按主题派生底色/文字。
2. 破坏性操作收口（M4/M1，P1 防误触）：行内删除/注销改 `danger-ghost` 并保留确认；`DevicePanel` 清会话补二次确认；修正“退出”按钮实为登录的文案错配。
3. 补齐进行态与加载态（D1/S2，P1 状态完整性）：设备列表骨架 + 内联重试；首页大圆钮 busy 显示进度指示与进行中文案；自定义主操作项补键盘焦点与 Accessible 标注（G7）。

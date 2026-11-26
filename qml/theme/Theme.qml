import QtQuick

// 全局设计令牌 / Global design tokens —— 现代暗色 SaaS 控制台风格（对标 Cloudflare Dashboard）。
// Modern dark SaaS-console palette. 单一事实来源 / single source of truth.
// 深色模式为规范基准；浅色模式由同色相家族派生，根背景不用纯白。
// Dark is the spec reference; light is derived from the same hue family (never pure white).
QtObject {
    id: theme

    // 外观模式：0 跟随系统 / 1 浅色 / 2 深色，持久化于配置。
    property int mode: app.config.themeMode

    // 系统配色方案：Windows 由 C++ 直接读注册表深色偏好（Basic 样式下 Qt
    // styleHints/SystemPalette 不可靠）；其他平台回退 Qt 配色提示。
    // System scheme: Windows reads the registry dark preference via C++ (Basic
    // style makes Qt styleHints/SystemPalette unreliable); others fall back to Qt hints.
    readonly property bool systemDark: app.config.systemDark()

    // 手动浅色/深色优先；跟随系统时取 systemDark。
    readonly property bool isDark: mode === 2 ? true : mode === 1 ? false : systemDark

    // ====================================================================
    // 背景层级 / Background hierarchy（规范 3.1 固定值）
    // ====================================================================
    readonly property color window:           isDark ? "#000000" : "#F3F4F6"   // 根背景 / page root
    readonly property color bg:               window                          // alias
    readonly property color card:             isDark ? "#111318" : "#FFFFFF"  // 卡片面板
    readonly property color surfaceElevated:  card                            // alias
    readonly property color secondaryBg:      isDark ? "#151820" : "#F9FAFB"  // 次级面板 / hover 提亮
    readonly property color surfaceMuted:     secondaryBg                     // alias
    readonly property color field:            isDark ? "#181B24" : "#F3F4F6"  // 分隔 / 输入框底 / 填充区块
    readonly property color controlHover:     isDark ? "#151820" : "#E5E7EB"
    readonly property color controlPressed:   isDark ? "#1C2029" : "#D1D5DB"
    readonly property color trackOff:         isDark ? "#2A2F3A" : "#D1D5DB"  // 开关关闭轨 / switch off track
    readonly property color separator:        isDark ? "#181B24" : "#E5E7EB"
    readonly property color border:           isDark ? "#272A33" : "#E5E7EB"  // 1px 细边框
    readonly property color overlay:          isDark ? Qt.rgba(0, 0, 0, 0.64) : Qt.rgba(0.07, 0.09, 0.15, 0.45)
    readonly property color bannerBg:         isDark ? "#0F172A" : "#F9FAFB" // 提示横幅（规范 4.9）：按主题派生，避免浅色深底浅字
    readonly property color gridLine:         "#222630"                       // 图表网格线
    readonly property color captchaSurface:  "#FFFFFF"                        // 验证码图片底板（位图多为白底，外围边框走主题）

    // ====================================================================
    // 主色与状态色 / Brand & status（规范 3.1 固定值）
    // ====================================================================
    readonly property color primary:          isDark ? "#38BDF8" : "#0369A1"  // 浅天蓝 / sky；浅色态加深以过 AA 4.5:1
    readonly property color primaryForeground:isDark ? "#071821" : "#FFFFFF"  // 主按钮上的深色文字
    readonly property color onSolid:          "#FFFFFF"                       // 实心主色/危险按钮上的白字、开关圆点
    readonly property color primaryHover:     isDark ? "#0EA5E9" : "#075985"
    readonly property color success:          isDark ? "#22C55E" : "#16A34A"  // 在线 / 正常
    readonly property color warning:          isDark ? "#F59E0B" : "#D97706"  // 探测中 / 半连通
    readonly property color danger:           isDark ? "#EF4444" : "#DC2626"  // 断网 / 错误
    readonly property color info:             primary
    readonly property color disabled:         "#6B7280"

    // 向后兼容别名 / backward-compatible aliases（旧组件引用）
    readonly property color blue:     info
    readonly property color green:    success
    readonly property color orange:   warning
    readonly property color red:      danger
    readonly property color purple:   isDark ? "#A78BFA" : "#8B5CF6"
    readonly property color indigo:   isDark ? "#818CF8" : "#6366F1"
    readonly property color gray:     isDark ? "#9CA3AF" : "#6B7280"

    readonly property color focusRing: Qt.rgba(primary.r, primary.g, primary.b, isDark ? 0.30 : 0.25)

    // 状态色低透明填充 / status fills (low alpha)
    function fill(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }
    readonly property color primaryFill: fill(primary, isDark ? 0.16 : 0.10)
    readonly property color infoFill:    primaryFill
    readonly property color successFill: fill(success, isDark ? 0.16 : 0.10)
    readonly property color greenFill:   successFill
    readonly property color warningFill: fill(warning, isDark ? 0.16 : 0.10)
    readonly property color orangeFill:  warningFill
    readonly property color dangerFill:  fill(danger, isDark ? 0.16 : 0.10)
    readonly property color redFill:     dangerFill
    readonly property color grayFill:    fill(gray, isDark ? 0.14 : 0.08)

    // ====================================================================
    // 文字层级 / Text hierarchy（规范 3.1 固定值）
    // ====================================================================
    readonly property color textPrimary:    isDark ? "#FFFFFF" : "#111827"  // 一级标题 / 核心数字
    readonly property color text:           isDark ? "#E5E7EB" : "#374151"  // 正文
    readonly property color secondaryLabel: isDark ? "#9CA3AF" : "#6B7280"  // 次要说明
    readonly property color muted:          secondaryLabel                   // alias
    readonly property color tertiaryLabel:  isDark ? "#9CA3AF" : "#6B7280"  // 占位 / 禁用：两态均过 AA 正文对比度
    readonly property color quaternaryLabel:isDark ? "#4B5563" : "#D1D5DB"
    readonly property color disabledText:   tertiaryLabel
    readonly property color disabledBackground: isDark ? "#181B24" : "#E5E7EB"

    // ====================================================================
    // DPI / 字号缩放 / DPI & font scaling（不写死 px）
    // ====================================================================
    readonly property real baseFontPixelSize: 13
    readonly property real systemFontPixelSize: (Qt.application && Qt.application.font
                                                 && Qt.application.font.pixelSize > 0)
                                               ? Qt.application.font.pixelSize
                                               : baseFontPixelSize
    readonly property real fontScale: Math.max(0.9, Math.min(1.75,
                                                   systemFontPixelSize / baseFontPixelSize))
    readonly property real screenScale: {
        var w = Screen.desktopAvailableWidth
        return w > 0 ? Math.max(1.0, Math.min(1.35, w / 1600)) : 1.0
    }
    property real userScale: 1.0
    readonly property real uiScale: Math.max(0.8, Math.min(2.2,
                                               fontScale * userScale * screenScale))

    // 设计像素 -> 当前缩放下的逻辑像素。
    function dp(value) { return Math.round(value * uiScale) }
    readonly property int wheelStep: dp(96)
    readonly property int wheelStepSmall: dp(48)

    // 字体回退链 / font fallback chain：内置字体优先 -> 同语言系统字体 -> 通用回退。
    // 内置字体由 main.cpp 的 QFontDatabase::addApplicationFont 注册；
    // Segoe UI 承载拉丁/英文，宋体承载简体中文，Microsoft JhengHei 承载繁体，
    // Meiryo 承载日语，缺失字形自动顺延下一家族不崩溃。
    readonly property string fontFamily: "Segoe UI, " +
        "SimSun, Microsoft JhengHei, Microsoft JhengHei UI, MS UI Gothic, MS Gothic, " +
        "Microsoft YaHei, PingFang SC, PingFang TC, Hiragino Sans GB, " +
        "Noto Sans CJK SC, Noto Sans CJK TC, Noto Sans CJK JP, " +
        "Source Han Sans SC, WenQuanYi Micro Hei, sans-serif"

    readonly property int fontCaption:  dp(11)
    readonly property int fontFootnote: dp(12)
    readonly property int fontSubhead:  dp(13)
    readonly property int fontBody:     dp(14)
    readonly property int fontCallout:  dp(15)
    readonly property int fontHeadline: dp(16)
    readonly property int fontTitle3:   dp(19)
    readonly property int fontTitle2:   dp(22)
    readonly property int fontTitle1:   dp(28)
    readonly property int fontLarge:    dp(34)
    readonly property int fontDisplay:  dp(40)   // KPI 大数字

    readonly property int weightRegular:  400
    readonly property int weightMedium:   500
    readonly property int weightSemibold: 600
    readonly property int weightBold:     700

    // ====================================================================
    // 圆角 / Radius（规范 3.2 固定值）
    // ====================================================================
    readonly property int radiusBadge:   dp(4)   // 徽章角标
    readonly property int radiusControl: dp(10)  // 按钮 / 输入框 / 下拉 / 图标方块
    readonly property int radiusSmall:   dp(10)  // alias
    readonly property int radiusCard:    dp(14)  // 卡片
    readonly property int radius:        dp(14)  // alias
    readonly property int radiusLarge:   dp(16)
    readonly property int radiusPill:    999

    // ====================================================================
    // 间距 / Spacing（8px 基准网格，规范 3.4）
    // ====================================================================
    readonly property int sp1:  dp(2)    // 极紧间距（图标与文字）
    readonly property int sp2:  dp(4)    // 标签 / 小字组件
    readonly property int sp3:  dp(6)    // 紧凑控件内间距
    readonly property int sp4:  dp(8)    // 控件垂直间距
    readonly property int sp5:  dp(10)
    readonly property int sp6:  dp(12)
    readonly property int sp8:  dp(16)   // 卡片之间
    readonly property int sp10: dp(20)   // 卡片内边距
    readonly property int sp12: dp(24)   // 页面外边距
    readonly property int sp16: dp(32)

    readonly property int pageMargin:   sp12
    readonly property int pageSideMargin: sp10   // 页面左右留白（卡片不贴屏幕边）
    readonly property int cardPadding:  sp10
    readonly property int cardSpacing:  sp8
    readonly property int controlGap:   sp4

    // ====================================================================
    // 尺寸 / Sizing
    // ====================================================================
    readonly property int touchTarget:    dp(44)   // ≥44 逻辑像素（规范）
    readonly property int inputHeight:    dp(40)   // 规范：输入框高度 40
    readonly property int buttonHeight:   dp(40)
    readonly property int contentMaxWidth:dp(1080)
    readonly property int compactBreakpoint: dp(600)
    readonly property int wideBreakpoint: dp(960)
    readonly property int narrowBreakpoint: dp(320)  // 超窄屏：字段由两列降为单列

    // ====================================================================
    // 阴影 / Shadow（极轻，规范 3.3）
    // ====================================================================
    readonly property color shadowColor: "#000000"
    readonly property real shadowOpacityCard: isDark ? 0.15 : 0.06
    readonly property real shadowOpacityCtrl: isDark ? 0.12 : 0.04
    readonly property real shadowElevCard: 0.4
    readonly property real disabledOpacity: 0.52

    // ====================================================================
    // 动效 / Motion（200ms ease，规范 7.1）
    // ====================================================================
    readonly property int duration:        200
    readonly property int durationFast:    150
    readonly property int durationNormal:  200
    readonly property int durationSlow:    240
    readonly property int durationEntrance: 200

    function pressed(c) { return Qt.darker(c, isDark ? 1.15 : 1.08) }
    function hovered(c) { return Qt.lighter(c, isDark ? 1.08 : 1.03) }
}

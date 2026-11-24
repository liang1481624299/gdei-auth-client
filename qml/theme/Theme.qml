import QtQuick

QtObject {
    id: theme

    // 外观模式：0 跟随系统 / 1 浅色 / 2 深色，持久化于配置。
    // Appearance mode: 0 follow system, 1 light, 2 dark, persisted in config.
    property int mode: app.config.themeMode

    readonly property SystemPalette sysPalette: SystemPalette { colorGroup: SystemPalette.Active }

    // 系统配色方案（Qt 6.5+）；Unknown 时回退到 SystemPalette 亮度反推。
    // System color scheme; fall back to palette luminance when Unknown.
    readonly property bool systemDark: {
        var hints = Qt.application && Qt.application.styleHints;
        if (hints) {
            var cs = hints.colorScheme;
            if (cs === Qt.DarkScheme) return true;
            if (cs === Qt.LightScheme) return false;
        }
        var w = sysPalette.window;
        var t = sysPalette.text;
        var lw = 0.299 * w.r + 0.587 * w.g + 0.114 * w.b;
        var lt = 0.299 * t.r + 0.587 * t.g + 0.114 * t.b;
        return lt > lw;
    }

    // 手动浅色/深色优先；跟随系统时取 systemDark。
    // Manual light/dark wins; follow-system uses systemDark.
    readonly property bool isDark: mode === 2 ? true : mode === 1 ? false : systemDark

    // ---- 微信风格背景层级 / WeChat background hierarchy ----
    readonly property color window: isDark ? "#000000" : "#EDEDED"
    readonly property color bg: window
    readonly property color card: isDark ? "#1C1C1E" : "#FFFFFF"
    readonly property color surfaceElevated: card
    readonly property color secondaryBg: isDark ? "#1C1C1E" : "#FFFFFF"
    readonly property color surfaceMuted: secondaryBg
    readonly property color field: isDark ? "#2C2C2E" : "#F2F2F7"
    readonly property color controlHover: isDark ? "#3A3A3C" : "#F2F2F7"
    readonly property color controlPressed: isDark ? "#48484A" : "#E5E5E5"
    readonly property color separator: isDark ? "#2C2C2E" : "#E5E5E5"
    readonly property color border: separator
    readonly property color overlay: isDark ? Qt.rgba(0, 0, 0, 0.64) : Qt.rgba(0.05, 0.05, 0.07, 0.36)

    // ---- 微信风格文本层级 / WeChat label hierarchy ----
    readonly property color text: isDark ? "#FFFFFF" : "#000000"
    readonly property color secondaryLabel: isDark ? "#989898" : "#888888"
    readonly property color muted: secondaryLabel
    readonly property color tertiaryLabel: isDark ? "#666666" : "#999999"
    readonly property color quaternaryLabel: isDark ? "#48484A" : "#C7C7CC"
    readonly property color disabledText: isDark ? "#48484A" : "#C7C7CC"
    readonly property color disabledBackground: isDark ? "#2C2C2E" : "#E9E9EB"

    // ---- 规范固定状态色 / Spec-fixed status colors ----
    // 浅色态取规范基准色；深色态取同色相 400 级提亮，保证深色背景对比度。
    // Light uses the spec base hex; dark uses the same hue lifted to level 400 for contrast.
    readonly property color blue: isDark ? "#60A5FA" : "#3B82F6"
    readonly property color green: isDark ? "#34D399" : "#10B981"
    readonly property color orange: isDark ? "#FBBF24" : "#F59E0B"
    readonly property color red: isDark ? "#F87171" : "#EF4444"
    readonly property color purple: isDark ? "#BF5AF2" : "#AF52DE"
    readonly property color indigo: isDark ? "#5E5CE6" : "#5856D6"
    readonly property color gray: isDark ? "#A1A1AA" : "#9CA3AF"

    readonly property color primary: green
    readonly property color primaryHover: Qt.darker(green, isDark ? 0.92 : 1.08)
    readonly property color primaryForeground: "#FFFFFF"
    readonly property color success: green
    readonly property color warning: orange
    readonly property color info: blue
    readonly property color danger: red
    readonly property color dangerForeground: "#FFFFFF"
    readonly property color focusRing: Qt.rgba(green.r, green.g, green.b, isDark ? 0.34 : 0.22)

    function fill(c, a) { return Qt.rgba(c.r, c.g, c.b, a); }
    readonly property color blueFill: fill(blue, isDark ? 0.18 : 0.10)
    readonly property color infoFill: blueFill
    readonly property color greenFill: fill(green, isDark ? 0.18 : 0.10)
    readonly property color orangeFill: fill(orange, isDark ? 0.18 : 0.10)
    readonly property color redFill: fill(red, isDark ? 0.18 : 0.10)
    readonly property color grayFill: fill(gray, isDark ? 0.16 : 0.09)

    // 高分辨率 / 无障碍缩放：以系统应用字体为基准，跟随系统字号放大，范围收敛避免溢出。
    readonly property real baseFontPixelSize: 13
    readonly property real systemFontPixelSize: (Qt.application && Qt.application.font
                                                 && Qt.application.font.pixelSize > 0)
                                               ? Qt.application.font.pixelSize
                                               : baseFontPixelSize
    readonly property real fontScale: Math.max(0.9, Math.min(1.75,
                                                   systemFontPixelSize / baseFontPixelSize))
    // 屏幕逻辑宽度因子：宽屏适度放大以利用空间，上限收敛避免控件过大。
    readonly property real screenScale: {
        var w = Screen.desktopAvailableWidth;
        return w > 0 ? Math.max(1.0, Math.min(1.35, w / 1600)) : 1.0;
    }
    property real userScale: 1.0
    readonly property real uiScale: Math.max(0.8, Math.min(2.2,
                                               fontScale * userScale * screenScale))

    // 设计像素 -> 当前缩放下的逻辑像素；滚轮步长随字号同步，避免高分屏滚动过慢/过快。
    function dp(value) { return Math.round(value * uiScale); }
    readonly property int wheelStep: dp(96)
    readonly property int wheelStepSmall: dp(48)

    readonly property int fontCaption: dp(11)
    readonly property int fontFootnote: dp(12)
    readonly property int fontSubhead: dp(13)
    readonly property int fontBody: dp(14)
    readonly property int fontCallout: dp(15)
    readonly property int fontHeadline: dp(16)
    readonly property int fontTitle3: dp(19)
    readonly property int fontTitle2: dp(22)
    readonly property int fontTitle1: dp(28)
    readonly property int fontLarge: dp(34)

    readonly property int weightRegular: 400
    readonly property int weightMedium: 500
    readonly property int weightSemibold: 600
    readonly property int weightBold: 700

    readonly property int radiusSmall: dp(8)
    readonly property int radiusButton: dp(10)
    readonly property int radius: dp(12)
    readonly property int radiusLarge: dp(16)
    readonly property int radiusPill: 999

    readonly property int sp2: dp(4)
    readonly property int sp3: dp(6)
    readonly property int sp4: dp(8)
    readonly property int sp5: dp(10)
    readonly property int sp6: dp(12)
    readonly property int sp8: dp(16)
    readonly property int sp10: dp(20)
    readonly property int sp12: dp(24)
    readonly property int sp16: dp(32)

    readonly property int touchTarget: dp(48)
    readonly property int inputHeight: dp(48)
    readonly property int buttonHeight: dp(48)
    readonly property int contentMaxWidth: dp(1080)
    readonly property int compactBreakpoint: dp(600)
    readonly property int wideBreakpoint: dp(960)

    readonly property color shadowColor: "#000000"
    readonly property real shadowOpacityCard: isDark ? 0.20 : 0.03
    readonly property real shadowOpacityCtrl: isDark ? 0.18 : 0.04
    readonly property real shadowElevCard: 0.4
    readonly property real disabledOpacity: 0.52

    readonly property int durationFast: 120
    readonly property int durationNormal: 180
    readonly property int durationSlow: 240
    readonly property int durationEntrance: 240

    function pressed(c) { return Qt.darker(c, isDark ? 1.18 : 1.10); }
    function hovered(c) { return Qt.lighter(c, isDark ? 1.10 : 1.035); }
}

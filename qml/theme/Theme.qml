import QtQuick

QtObject {
    id: theme

    readonly property SystemPalette sysPalette: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property bool isDark: {
        var w = sysPalette.window;
        var t = sysPalette.text;
        var lw = 0.299 * w.r + 0.587 * w.g + 0.114 * w.b;
        var lt = 0.299 * t.r + 0.587 * t.g + 0.114 * t.b;
        return lt > lw;
    }

    readonly property color window: isDark ? "#101012" : "#F7F7F8"
    readonly property color bg: window
    readonly property color card: isDark ? "#1B1B1E" : "#FFFFFF"
    readonly property color surfaceElevated: card
    readonly property color secondaryBg: isDark ? "#232326" : "#F4F4F5"
    readonly property color surfaceMuted: secondaryBg
    readonly property color field: isDark ? "#202023" : "#FAFAFA"
    readonly property color controlHover: isDark ? "#2B2B2F" : "#EEEEF0"
    readonly property color controlPressed: isDark ? "#343438" : "#E4E4E7"
    readonly property color separator: isDark ? "#303034" : "#E4E4E7"
    readonly property color border: separator
    readonly property color overlay: isDark ? Qt.rgba(0, 0, 0, 0.64) : Qt.rgba(0.05, 0.05, 0.07, 0.36)

    readonly property color text: isDark ? "#FAFAFA" : "#18181B"
    readonly property color secondaryLabel: isDark ? "#A1A1AA" : "#71717A"
    readonly property color muted: secondaryLabel
    readonly property color tertiaryLabel: isDark ? "#71717A" : "#A1A1AA"
    readonly property color quaternaryLabel: isDark ? "#52525B" : "#D4D4D8"
    readonly property color disabledText: isDark ? "#626269" : "#A1A1AA"
    readonly property color disabledBackground: isDark ? "#29292D" : "#E9E9EB"

    readonly property color blue: isDark ? "#60A5FA" : "#2563EB"
    readonly property color green: isDark ? "#4ADE80" : "#16A34A"
    readonly property color orange: isDark ? "#FBBF24" : "#D97706"
    readonly property color red: isDark ? "#F87171" : "#DC2626"
    readonly property color purple: isDark ? "#C084FC" : "#9333EA"
    readonly property color indigo: isDark ? "#818CF8" : "#4F46E5"
    readonly property color gray: secondaryLabel

    readonly property color primary: blue
    readonly property color primaryHover: Qt.darker(blue, isDark ? 0.92 : 1.08)
    readonly property color primaryForeground: isDark ? "#101018" : "#FFFFFF"
    readonly property color success: green
    readonly property color warning: orange
    readonly property color danger: red
    readonly property color dangerForeground: isDark ? "#18181B" : "#FFFFFF"
    readonly property color focusRing: Qt.rgba(blue.r, blue.g, blue.b, isDark ? 0.34 : 0.22)

    function fill(c, a) { return Qt.rgba(c.r, c.g, c.b, a); }
    readonly property color blueFill: fill(blue, isDark ? 0.18 : 0.10)
    readonly property color greenFill: fill(green, isDark ? 0.18 : 0.10)
    readonly property color orangeFill: fill(orange, isDark ? 0.18 : 0.10)
    readonly property color redFill: fill(red, isDark ? 0.18 : 0.10)
    readonly property color grayFill: fill(gray, isDark ? 0.16 : 0.09)

    readonly property int fontCaption: 11
    readonly property int fontFootnote: 12
    readonly property int fontSubhead: 13
    readonly property int fontBody: 14
    readonly property int fontCallout: 15
    readonly property int fontHeadline: 16
    readonly property int fontTitle3: 19
    readonly property int fontTitle2: 22
    readonly property int fontTitle1: 28
    readonly property int fontLarge: 34

    readonly property int weightRegular: 400
    readonly property int weightMedium: 500
    readonly property int weightSemibold: 600
    readonly property int weightBold: 700

    readonly property int radiusSmall: 8
    readonly property int radius: 12
    readonly property int radiusLarge: 16
    readonly property int radiusPill: 999

    readonly property int sp2: 4
    readonly property int sp3: 6
    readonly property int sp4: 8
    readonly property int sp5: 10
    readonly property int sp6: 12
    readonly property int sp8: 16
    readonly property int sp10: 20
    readonly property int sp12: 24
    readonly property int sp16: 32

    readonly property int touchTarget: 44
    readonly property int inputHeight: 44
    readonly property int buttonHeight: 44
    readonly property int contentMaxWidth: 1080
    readonly property int compactBreakpoint: 600
    readonly property int wideBreakpoint: 960

    readonly property color shadowColor: "#000000"
    readonly property real shadowOpacityCard: isDark ? 0.24 : 0.055
    readonly property real shadowOpacityCtrl: isDark ? 0.20 : 0.06
    readonly property real shadowElevCard: 0.4
    readonly property real disabledOpacity: 0.52

    readonly property int durationFast: 120
    readonly property int durationNormal: 180
    readonly property int durationSlow: 240
    readonly property int durationEntrance: 240

    function pressed(c) { return Qt.darker(c, isDark ? 1.18 : 1.10); }
    function hovered(c) { return Qt.lighter(c, isDark ? 1.10 : 1.035); }
}

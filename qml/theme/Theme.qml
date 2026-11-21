// qml/theme/Theme.qml
// 设计令牌 / Design tokens: clean shadcn-like palette, radii and shadows.
import QtQuick

QtObject {
    // ---- 配色 / Colors ----
    readonly property color bg:       "#f6f8fb"
    readonly property color card:     "#ffffff"
    readonly property color border:   "#e3e8ef"
    readonly property color field:    "#f1f5f9"
    readonly property color text:     "#0f172a"
    readonly property color muted:    "#64748b"
    readonly property color primary:  "#2563eb"
    readonly property color primaryHover: "#1d4ed8"
    readonly property color success:  "#16a34a"
    readonly property color warning:  "#d97706"
    readonly property color danger:   "#dc2626"

    // ---- 圆角 / Radii ----
    readonly property int radius:       14
    readonly property int radiusSmall:  10

    // ---- 阴影 / Shadow ----
    readonly property string shadowColor: "#140b2030"
}

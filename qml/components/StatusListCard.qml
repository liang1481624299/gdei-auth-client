import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// StatusListCard —— 通用状态列表卡片（可复用组件）。
// StatusListCard —— reusable status list card.
//
// 布局 / Layout（纵向列表中的独立大圆角容器，卡片之间靠页面纵向间距分隔，无分割线）：
//   内层 ColumnLayout 填满卡片内边距；横向 RowLayout 排布左侧圆形矢量图标 + 中间三行文本。
//   右侧状态标签作为文本列的直接子项右对齐、垂直居中。
//   Inner ColumnLayout fills the card padding; a RowLayout lays out the leading circular
//   icon and stacked text. The trailing status badge is a direct child of the text column,
//   right aligned and vertically centered.
//
// 传参 / Properties:
//   iconSource  左侧矢量图标名（qml/icons/<name>.svg）
//   title       小标题（次要色，脚注字号）
//   mainText    主状态文本（一级色，加粗标题字号）
//   descText    辅助说明小字（静音色，脚注字号）
//   tagText     右侧状态标签文案；为空则隐藏标签
//   tagTint     标签与图标强调色（可选，默认静音色）
//
// 约束 / Constraints:
//   - 所有尺寸、间距、圆角均经 Theme.dp() 缩放，适配多 DPI / 横竖屏 / 长屏，不写死像素。
//   - 图标为 Qt 自带 SVG 矢量（Icon 组件），随 DPI 矢量渲染。
//   - 点击仅做柔和缩放反馈，不改变配色；不新增背景渐变或额外装饰。
Rectangle {
    id: root
    Theme { id: th }

    property string iconSource: ""
    property string title: ""
    property string mainText: ""
    property string descText: ""
    property string tagText: ""
    property color tagTint: th.muted
    // 仅在外部连接 clicked 时启用按压反馈与可点语义；默认只读，避免伪可点。
    property bool interactive: false

    signal clicked()

    focusPolicy: root.interactive ? Qt.StrongFocus : Qt.NoFocus
    Keys.onReturnPressed: if (root.interactive) root.clicked()
    Keys.onSpacePressed: if (root.interactive) root.clicked()
    Accessible.role: root.interactive ? Accessible.Button : Accessible.StaticText
    Accessible.name: root.mainText.length > 0 ? root.mainText : root.title
    Accessible.focusable: root.interactive

    // 左右内边距与页面标题、底部导航的对齐线一致；高度由内容撑开，最小不低于触控目标。
    // Side padding matches the page title / bottom-nav alignment; height is content-driven,
    // floored at the touch target.
    implicitWidth: Math.max(th.dp(280), bodyCol.implicitWidth + th.cardPadding * 2)
    implicitHeight: Math.max(th.touchTarget, bodyCol.implicitHeight + th.cardPadding * 2)

    color: th.card
    radius: th.radiusLarge
    border.width: th.dp(1)
    border.color: th.border

    // 柔和按压反馈：仅交互态缩放，不切换配色。
    // Soft press feedback: scale only when interactive, colors unchanged.
    transformOrigin: Item.Center
    scale: root.interactive && tap.pressed ? 0.985 : 1.0
    Behavior on scale {
        NumberAnimation { duration: th.durationFast; easing.type: Easing.OutCubic }
    }

    TapHandler {
        id: tap
        enabled: root.interactive
        onTapped: root.clicked()
    }

    ColumnLayout {
        id: bodyCol
        anchors.fill: parent
        anchors.margins: th.cardPadding
        spacing: th.sp6

        RowLayout {
            id: contentRow
            Layout.fillWidth: true
            spacing: th.sp4

            // 左侧图标区：圆形外框，图标居中并预留内边距。
            // Leading icon: circular frame with centered vector icon and inner padding.
            Rectangle {
                Layout.preferredWidth: th.dp(48)
                Layout.preferredHeight: th.dp(48)
                Layout.alignment: Qt.AlignVCenter
                radius: th.radiusControl
                color: th.fill(root.tagTint, th.isDark ? 0.16 : 0.10)

                Icon {
                    anchors.centerIn: parent
                    name: root.iconSource
                    iconSize: th.dp(22)
                    tint: root.tagTint
                }
            }

            // 中间文字区：小标题 / 主状态 / 辅助说明，左对齐，固定行间距。
            // Middle text: title / main status / hint, left aligned with fixed line spacing.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: th.sp2

                QQC.Label {
                    text: root.title
                    visible: text.length > 0
                    color: th.secondaryLabel
                    font.family: th.fontFamily
                    font.pixelSize: th.fontFootnote
                    font.weight: th.weightRegular
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                    Layout.fillWidth: true
                }

                QQC.Label {
                    text: root.mainText
                    visible: text.length > 0
                    color: th.textPrimary
                    font.family: th.fontFamily
                    font.pixelSize: th.fontHeadline
                    font.weight: th.weightSemibold
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                    Layout.fillWidth: true
                }

                QQC.Label {
                    text: root.descText
                    visible: text.length > 0
                    color: th.muted
                    font.family: th.fontFamily
                    font.pixelSize: th.fontFootnote
                    font.weight: th.weightRegular
                    elide: Text.ElideRight
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
            }

            // 右侧状态标签：独立右列，垂直居中；文案为空时隐藏。
            // Trailing badge: independent right column, vertically centered.
            BaseBadge {
                visible: root.tagText.length > 0
                text: root.tagText
                tint: root.tagTint
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            }
        }
    }
}

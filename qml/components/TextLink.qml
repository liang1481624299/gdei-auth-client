import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// TextLink —— 轻量文字链：纯主色文字 + 可选箭头，无背景无边框（分组头右侧“查看全部”类入口）。
// Lightweight text link: primary text + optional chevron, no background/border.
QQC.Button {
    id: root
    Theme { id: th }

    property string text: ""
    property string trailingIcon: ""
    property bool bold: false

    visible: root.text.length > 0
    implicitHeight: th.touchTarget
    padding: 0
    hoverEnabled: true

    contentItem: RowLayout {
        spacing: th.sp1
        Layout.alignment: Qt.AlignVCenter

        QQC.Label {
            text: root.text
            color: root.enabled ? (root.hovered ? th.primaryHover : th.primary) : th.disabledText
            font.pixelSize: th.fontFootnote
            font.weight: root.bold ? th.weightSemibold : th.weightMedium
            verticalAlignment: Text.AlignVCenter
            Layout.alignment: Qt.AlignVCenter
        }
        Icon {
            name: root.trailingIcon
            iconSize: th.dp(14)
            tint: root.enabled ? (root.hovered ? th.primaryHover : th.primary) : th.disabledText
            visible: name.length > 0
            Layout.alignment: Qt.AlignVCenter
        }
    }

    background: Item {}
}

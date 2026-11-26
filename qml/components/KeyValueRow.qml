import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// KeyValueRow —— 键值行：左灰色标签，右取值 + 可选右箭头；可选可点。
// Key-value row: muted label on the left, value + optional chevron on the right; optionally clickable.
// 行间分隔线由调用方放置（与现有页面一致），组件不自绘分隔线。
QQC.Button {
    id: root
    Theme { id: th }

    property string label: ""
    property string value: ""
    property bool chevron: true
    property bool clickable: false

    enabled: clickable
    implicitHeight: th.dp(56)
    padding: 0
    hoverEnabled: clickable

    contentItem: RowLayout {
        spacing: th.sp4

        QQC.Label {
            text: root.label
            color: th.secondaryLabel
            font.pixelSize: th.fontBody
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
        QQC.Label {
            text: root.value.length > 0 ? root.value : "—"
            color: root.value.length > 0
                   ? (root.enabled ? th.text : th.secondaryLabel)
                   : th.tertiaryLabel
            font.pixelSize: th.fontBody
            font.weight: th.weightMedium
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignVCenter
        }
        // 右箭头仅在可点时显示：箭头语义为页面跳转，不可点行不给跳转暗示。
        Icon {
            name: "chevronRight"
            iconSize: th.dp(16)
            tint: th.muted
            visible: root.chevron && root.clickable
            Layout.alignment: Qt.AlignVCenter
        }
    }

    background: Rectangle {
        radius: th.radiusControl
        color: root.clickable
               ? (root.down ? th.controlPressed : root.hovered ? th.controlHover : "transparent")
               : "transparent"
    }
}

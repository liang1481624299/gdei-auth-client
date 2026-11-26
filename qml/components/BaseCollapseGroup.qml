import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// BaseCollapseGroup —— 折叠分组（规范 4.8）。
// 标题行 + 右侧展开/收起箭头；默认收起；展开区与卡片底色一致。
ColumnLayout {
    id: root
    Theme { id: th }

    property string title: ""
    property bool expanded: false
    property Component headerRight: null     // 标题行右侧控件（可选）
    property bool showBorder: true

    default property alias content: body.data

    Layout.fillWidth: true
    spacing: 0

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: th.dp(44)
        color: "transparent"
        border.width: root.showBorder ? 1 : 0
        border.color: th.border
        radius: th.radiusControl

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: th.sp6
            anchors.rightMargin: th.sp6
            spacing: th.sp4

            QQC.Label {
                text: root.title
                color: th.text
                font.pixelSize: th.fontBody
                font.weight: th.weightSemibold
                Layout.fillWidth: true
            }

            Loader {
                sourceComponent: root.headerRight
                Layout.alignment: Qt.AlignVCenter
            }

            Icon {
                name: "chevronDown"
                iconSize: th.dp(16)
                tint: th.secondaryLabel
                rotation: root.expanded ? 180 : 0
                Behavior on rotation { NumberAnimation { duration: th.duration } }
                Layout.alignment: Qt.AlignVCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.expanded = !root.expanded
        }
    }

    Rectangle {
        id: bodyRect
        Layout.fillWidth: true
        clip: true
        color: "transparent"
        border.width: root.showBorder ? 1 : 0
        border.color: th.border
        radius: th.radiusControl
        // 参与父级 ColumnLayout 的高度计算，避免展开后与后续分组重叠。
        Layout.preferredHeight: root.expanded ? Math.min(bodyCol.implicitHeight + th.sp6 * 2, 4096) : 0
        opacity: root.expanded ? 1 : 0
        Behavior on Layout.preferredHeight { NumberAnimation { duration: th.duration; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: th.durationFast } }
        // 内容直接作为本节点的子项，default property alias 才能解析到 body。
        ColumnLayout {
            id: bodyCol
            anchors.fill: parent
            anchors.margins: th.sp6
            spacing: th.sp6
            ColumnLayout { id: body; Layout.fillWidth: true; spacing: th.sp6 }
        }
    }
}

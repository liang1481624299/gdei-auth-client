import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// BaseButton —— 三级权重按钮 / three-weight button（规范 4.4）。
// primary: 填充主色深色字；secondary: 透明 + 细轮廓；danger: 填充红；ghost: 纯文字（弹层取消）。
// hover 仅轻微变暗，禁止缩放 / 发光 / 上浮。
QQC.Button {
    id: root
    Theme { id: th }

    property string type: "primary"
    property string leadingIcon: ""
    property string trailingIcon: ""
    property bool busy: false

    implicitWidth: Math.max(contentRow.implicitWidth + th.sp8 * 2, th.touchTarget)
    implicitHeight: th.buttonHeight
    padding: 0
    hoverEnabled: true
    enabled: !busy

    readonly property bool isPrimary: type === "primary"
    readonly property bool isDanger:  type === "danger"
    readonly property bool isGhost:   type === "ghost"
    readonly property bool isDangerGhost: type === "danger-ghost"
    readonly property bool isSecondary: type === "secondary" || type === "ghost"

    readonly property color fillColor:
        isDanger ? (root.hovered ? th.pressed(th.danger) : th.danger) :
        isPrimary ? (root.hovered ? th.primaryHover : th.primary) :
        "transparent"
    readonly property color textColor:
        isDanger ? th.onSolid :
        isDangerGhost ? th.danger :
        isPrimary ? th.primaryForeground :
        root.hovered ? th.text : th.text

    contentItem: Item {
        anchors.fill: parent

        RowLayout {
            id: contentRow
            anchors.centerIn: parent
            spacing: th.sp3
            Icon {
                name: root.leadingIcon
                iconSize: th.dp(16)
                tint: root.busy ? "transparent" : root.textColor
                visible: name.length > 0
                Layout.alignment: Qt.AlignVCenter
            }
            QQC.Label {
                text: root.busy ? "" : root.text
                color: root.textColor
                font.pixelSize: th.fontBody
                font.weight: th.weightSemibold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.alignment: Qt.AlignVCenter
            }
            Icon {
                name: root.trailingIcon
                iconSize: th.dp(16)
                tint: root.textColor
                visible: name.length > 0
                Layout.alignment: Qt.AlignVCenter
            }
            Rectangle {
                visible: root.busy
                width: th.dp(14); height: width
                radius: width / 2
                color: root.textColor
                opacity: 0.3
                Layout.alignment: Qt.AlignVCenter
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { to: 1.0; duration: 600 }
                    NumberAnimation { to: 0.3; duration: 600 }
                }
            }
        }
    }

    background: Rectangle {
        radius: th.radiusControl
        color: root.isDangerGhost
               ? (root.hovered ? th.dangerFill : "transparent")
               : root.fillColor
        border.width: ((root.isSecondary && !root.isGhost) || root.isDangerGhost) ? 1 : 0
        border.color: root.isDangerGhost ? th.danger
                     : root.hovered ? th.primary : th.border
        Behavior on color { ColorAnimation { duration: th.duration } }
        Behavior on border.color { ColorAnimation { duration: th.duration } }
    }
}

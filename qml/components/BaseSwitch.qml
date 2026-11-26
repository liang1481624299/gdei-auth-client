import QtQuick
import QtQuick.Controls as QQC
import "../theme"

// BaseSwitch —— 圆润轨道开关（规范 4.5）。
// 开启主色；关闭 trackOff；200ms 过渡。
QQC.Switch {
    id: root
    Theme { id: th }

    implicitWidth: th.dp(44)
    implicitHeight: th.dp(26)
    spacing: 0

    indicator: Rectangle {
        implicitWidth: th.dp(44)
        implicitHeight: th.dp(26)
        x: root.leftPadding
        y: parent.height / 2 - height / 2
        radius: height / 2
        color: root.checked ? th.primary : th.trackOff
        border.width: 1
        border.color: root.checked ? th.primary : th.border
        Behavior on color { ColorAnimation { duration: th.duration } }
        Behavior on border.color { ColorAnimation { duration: th.duration } }

        Rectangle {
            width: th.dp(20); height: width
            radius: width / 2
            color: th.onSolid
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? parent.width - width - th.dp(3) : th.dp(3)
            Behavior on x { NumberAnimation { duration: th.duration; easing.type: Easing.OutCubic } }
        }
    }

    background: Item { }
    contentItem: Item { }
}

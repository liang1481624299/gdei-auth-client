import QtQuick
import QtQuick.Controls as QQC
import "../theme"
import "."

QQC.Switch {
    id: control
    Theme { id: th }

    readonly property real trackWidth: 46
    readonly property real trackHeight: 26

    implicitWidth: 54
    implicitHeight: th.touchTarget
    padding: 0
    spacing: 0
    opacity: enabled ? 1 : th.disabledOpacity

    background: Item {
        implicitWidth: control.implicitWidth
        implicitHeight: th.touchTarget

        Rectangle {
            width: control.trackWidth
            height: control.trackHeight
            anchors.centerIn: parent
            radius: height / 2
            color: control.checked ? th.green : th.controlPressed
            border.width: control.activeFocus ? 2 : 0
            border.color: th.focusRing
            Behavior on color { ColorAnimation { duration: th.durationNormal } }
        }
    }

    indicator: Rectangle {
        width: 20
        height: 20
        radius: 10
        color: "#FFFFFF"
        x: (control.width - control.trackWidth) / 2
           + (control.checked ? control.trackWidth - width - 3 : 3)
        y: (control.height - height) / 2
        Behavior on x {
            SpringAnimation { spring: 3.2; damping: 0.68; epsilon: 0.01 }
        }

        SoftShadow {
            z: -1
            anchors.fill: parent
            corner: parent.radius
            strength: 0.16
            blur: 3
            offsetY: 1
        }
    }
}

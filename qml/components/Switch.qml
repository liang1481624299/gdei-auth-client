// qml/components/Switch.qml
// 可复用开关 / Reusable toggle switch.
import QtQuick
import QtQuick.Controls
import "../theme"

Switch {
    id: control
    Theme { id: th }

    implicitHeight: 32
    implicitWidth: 52

    background: Rectangle {
        radius: 16
        color: control.checked ? th.primary : th.border
    }

    indicator: Rectangle {
        x: control.checked ? control.width - width - 3 : 3
        y: (control.height - height) / 2
        width: 26
        height: 26
        radius: 13
        color: "white"
        border.width: 1
        border.color: th.border
    }
}

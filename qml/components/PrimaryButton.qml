// qml/components/PrimaryButton.qml
// 可复用主按钮 / Reusable primary button with press transition.
import QtQuick
import QtQuick.Controls
import "../theme"

Button {
    id: control
    Theme { id: th }

    property color bg: th.primary

    implicitHeight: 42
    padding: 18
    font.pixelSize: 14

    background: Rectangle {
        radius: th.radiusSmall
        color: control.enabled
               ? (control.down ? Qt.darker(control.bg, 1.1) : control.bg)
               : Qt.lighter(control.bg, 1.5)
    }

    contentItem: Label {
        text: control.text
        color: "white"
        font.bold: true
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}

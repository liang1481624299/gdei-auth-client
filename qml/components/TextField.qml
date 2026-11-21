// qml/components/TextField.qml
// 可复用输入框 / Reusable text field with focused border state.
import QtQuick
import QtQuick.Controls
import "../theme"

TextField {
    id: control
    Theme { id: th }

    property bool passwordMode: false

    color: th.text
    placeholderTextColor: th.muted
    echoMode: passwordMode ? TextInput.Password : TextInput.Normal
    implicitHeight: 42
    leftPadding: 12
    rightPadding: 12
    font.pixelSize: 14

    background: Rectangle {
        radius: th.radiusSmall
        color: th.field
        border.width: 1
        border.color: control.activeFocus ? th.primary : th.border
    }
}

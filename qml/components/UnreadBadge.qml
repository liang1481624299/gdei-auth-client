import QtQuick
import "../theme"

Rectangle {
    id: root
    Theme { id: th }

    property int count: 0

    implicitWidth: th.dp(10)
    implicitHeight: th.dp(10)
    width: implicitWidth
    height: implicitHeight
    radius: width / 2
    color: th.danger
    antialiasing: true
}

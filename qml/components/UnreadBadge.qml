import QtQuick
import "../theme"

Rectangle {
    id: root
    Theme { id: th }

    property int count: 0

    implicitWidth: 8
    implicitHeight: 8
    width: implicitWidth
    height: implicitHeight
    radius: width / 2
    color: th.danger
    antialiasing: true
}

import QtQuick
import QtQuick.Controls as QQC
import "../theme"
import "."

Rectangle {
    id: root
    Theme { id: th }

    property string text: ""
    property string iconName: ""
    property color tint: th.muted
    property color fillColor: th.fill(tint, th.isDark ? 0.18 : 0.09)

    implicitWidth: row.implicitWidth + 16
    implicitHeight: 28
    radius: height / 2
    color: fillColor
    border.width: 1
    border.color: th.fill(tint, th.isDark ? 0.26 : 0.12)

    Behavior on color { ColorAnimation { duration: th.durationNormal } }
    Behavior on border.color { ColorAnimation { duration: th.durationNormal } }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 5

        Icon {
            name: root.iconName
            iconSize: 13
            tint: root.tint
            visible: name.length > 0
            anchors.verticalCenter: parent.verticalCenter
        }
        QQC.Label {
            text: root.text
            color: root.tint
            font.pixelSize: th.fontFootnote
            font.weight: th.weightSemibold
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}

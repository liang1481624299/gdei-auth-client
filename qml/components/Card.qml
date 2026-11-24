import QtQuick
import "../theme"
import "."

Rectangle {
    id: root
    default property alias content: column.children

    Theme { id: th }

    property int margins: 16
    property real shadowStrength: cardHover.hovered ? th.shadowOpacityCard * 1.5
                                                     : th.shadowOpacityCard

    color: th.card
    radius: th.radius
    border.width: 0
    border.color: th.separator
    implicitWidth: column.implicitWidth + margins * 2
    implicitHeight: column.implicitHeight + margins * 2

    Behavior on color { ColorAnimation { duration: th.durationNormal } }
    Behavior on shadowStrength { NumberAnimation { duration: th.durationNormal } }

    HoverHandler { id: cardHover }

    SoftShadow {
        z: -1
        anchors.fill: parent
        corner: root.radius
        strength: root.shadowStrength
        blur: 12
        offsetY: 2
        visible: root.shadowStrength > 0
    }

    Column {
        id: column
        x: root.margins
        y: root.margins
        width: Math.max(0, parent.width - root.margins * 2)
        spacing: th.sp6
    }
}

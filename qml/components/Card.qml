// qml/components/Card.qml
// 卡片容器 / Card container: rounded surface, soft shadow, uniform padding.
import QtQuick
import "../theme"

Rectangle {
    id: root
    default property alias content: column.children

    Theme { id: th }

    property int radius: th.radius
    property int margins: 18

    color: th.card
    radius: root.radius
    border.width: 1
    border.color: th.border

    implicitWidth: column.implicitWidth + margins * 2
    implicitHeight: column.implicitHeight + margins * 2

    DropShadow {
        anchors.fill: parent
        radius: 18
        samples: 12
        color: th.shadowColor
        offset.y: 2
        visible: true
    }

    Column {
        id: column
        anchors.fill: parent
        anchors.margins: root.margins
        spacing: 12
    }
}

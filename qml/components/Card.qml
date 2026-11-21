// qml/components/Card.qml
// 卡片容器 / Card container: rounded surface, uniform padding.
import QtQuick
import "../theme"

Rectangle {
    id: root
    default property alias content: column.children

    Theme { id: th }

    property int margins: 18

    color: th.card
    radius: th.radius
    border.width: 1
    border.color: th.border

    implicitWidth: column.implicitWidth + margins * 2
    implicitHeight: column.implicitHeight + margins * 2

    Column {
        id: column
        anchors.fill: parent
        anchors.margins: root.margins
        spacing: 12
    }
}

import QtQuick
import QtQuick.Controls.impl
import "../theme"

Item {
    id: root
    Theme { id: th }

    property string name: ""
    property int iconSize: th.dp(22)
    property color tint: th.text   // 跟随主题，避免暗色模式下深色图标不可见
    property real lineWidth: 1.75

    implicitWidth: iconSize
    implicitHeight: iconSize
    width: iconSize
    height: iconSize

    IconImage {
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        source: root.name.length > 0 ? Qt.resolvedUrl("../icons/" + root.name + ".svg") : ""
        sourceSize: Qt.size(root.iconSize, root.iconSize)
        color: root.tint
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: root.name.length > 0
    }
}

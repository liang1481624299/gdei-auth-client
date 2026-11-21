// qml/components/SoftShadow.qml
// 柔和阴影 / Soft shadow —— 多层圆角矩形叠加近似高斯模糊，不依赖 Qt 特效模块。
// 用法：作为目标元素的兄弟节点，anchors.fill 目标，z 置于其后。
import QtQuick

Item {
    id: root

    property real corner: 12
    property color shadowColor: "#000000"
    property real strength: 0.12   // 整体不透明度 / overall opacity
    property real blur: 10         // 阴影扩散半径 / spread radius
    property real offsetY: 0       // 垂直偏移 / vertical offset

    // 4 层近似 / 4-layer falloff approximation
    Repeater {
        model: [
            { m: 0.00, o: 0.50 },
            { m: 0.30, o: 0.28 },
            { m: 0.62, o: 0.14 },
            { m: 1.00, o: 0.05 }
        ]
        Rectangle {
            radius: root.corner + modelData.m * root.blur
            color: root.shadowColor
            opacity: modelData.o * root.strength
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: -(modelData.m * root.blur)
            anchors.rightMargin: -(modelData.m * root.blur)
            anchors.topMargin: -(modelData.m * root.blur) + root.offsetY
            anchors.bottomMargin: -(modelData.m * root.blur) - root.offsetY
        }
    }
}

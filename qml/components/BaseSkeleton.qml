import QtQuick
import "../theme"

// BaseSkeleton —— 加载骨架占位（规范 7.2）。浅灰脉冲，无空白等待。
Rectangle {
    id: root
    Theme { id: th }

    property color baseColor: th.fill(th.text, th.isDark ? 0.06 : 0.10)

    radius: th.radiusControl
    color: root.baseColor

    SequentialAnimation on opacity {
        loops: Animation.Infinite
        NumberAnimation { to: 0.45; duration: 700; easing.type: Easing.InOutQuad }
        NumberAnimation { to: 1.0;  duration: 700; easing.type: Easing.InOutQuad }
    }
}

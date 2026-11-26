import QtQuick
import QtQuick.Controls as QQC
import "../theme"
import "."

// BaseBadge —— 状态徽章（规范 4.6）。
// 药丸圆角；底色为状态色低透明度版本，文字用对应状态色。
Rectangle {
    id: root
    Theme { id: th }

    property string text: ""
    property string iconName: ""
    property color tint: th.secondaryLabel
    property color fillColor: th.fill(root.tint, th.isDark ? 0.16 : 0.10)
    property int pixelSize: th.fontFootnote

    implicitWidth: row.implicitWidth + th.sp6
    implicitHeight: th.dp(24)
    radius: th.radiusBadge
    color: root.fillColor
    border.width: 1
    border.color: th.fill(root.tint, th.isDark ? 0.28 : 0.14)

    Behavior on color { ColorAnimation { duration: th.duration } }
    Behavior on border.color { ColorAnimation { duration: th.duration } }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: th.sp2
        Icon {
            name: root.iconName
            iconSize: th.dp(12)
            tint: root.tint
            visible: name.length > 0
            anchors.verticalCenter: parent.verticalCenter
        }
        QQC.Label {
            text: root.text
            color: root.tint
            font.pixelSize: root.pixelSize
            font.weight: th.weightSemibold
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}

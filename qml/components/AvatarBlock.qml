import QtQuick
import QtQuick.Controls as QQC
import "../theme"
import "."

// AvatarBlock —— 圆形首字头像：primaryFill 底 + primary 首字；空名回落 person 图标。
// Circular initial avatar: primaryFill background + primary initial; falls back to person icon when name is empty.
// 首字由调用方传入（账号名或实名），组件不自行拉取身份信息；敏感展示口径见 UI-AUDIT-STYLE §5。
Rectangle {
    id: root
    Theme { id: th }

    property string name: ""
    property string iconName: "person"
    property int size: th.dp(48)

    width: size
    height: size
    radius: width / 2
    color: th.primaryFill

    readonly property string initial: name.length > 0 ? name.charAt(0).toUpperCase() : ""

    QQC.Label {
        anchors.centerIn: parent
        visible: root.initial.length > 0
        text: root.initial
        color: th.primary
        font.pixelSize: Math.round(root.size * 0.4)
        font.weight: th.weightSemibold
    }
    Icon {
        anchors.centerIn: parent
        name: root.iconName
        iconSize: Math.round(root.size * 0.45)
        tint: th.primary
        visible: root.initial.length === 0
    }
}

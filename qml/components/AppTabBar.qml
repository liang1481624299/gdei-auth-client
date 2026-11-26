import QtQuick
import QtQuick.Layouts
import "../theme"
import "."

Rectangle {
    id: root
    Theme { id: th }

    property int currentIndex: 0
    signal tabSelected(int index)

    // 紧凑底部导航：图标 + 标签纵向堆叠；高度经 dp 缩放，适配多 DPI / 横竖屏 / 长屏，
    // 不写死像素、不使用硬编码色值。
    // Compact bottom nav: icon above label; height scaled by dp, no hardcoded pixels/colors.
    implicitHeight: th.dp(49)
    color: th.card

    // 顶部分隔细线，与主题分隔色一致；不新增渐变或多余装饰。
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: th.dp(1)
        color: th.separator
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        AppTabButton {
            text: (app.i18n.dictionary["nav.home"])
            iconName: "home"
            selected: root.currentIndex === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            onClicked: root.tabSelected(0)
        }

        AppTabButton {
            text: (app.i18n.dictionary["nav.devices"])
            iconName: "server"
            selected: root.currentIndex === 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            onClicked: root.tabSelected(1)
        }

        AppTabButton {
            text: (app.i18n.dictionary["nav.discover"])
            iconName: "bolt"
            selected: root.currentIndex === 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            onClicked: root.tabSelected(2)
        }

        AppTabButton {
            text: (app.i18n.dictionary["nav.mine"])
            iconName: "person"
            selected: root.currentIndex === 3
            showBadge: app.events.unreadCount > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            onClicked: root.tabSelected(3)
        }
    }
}

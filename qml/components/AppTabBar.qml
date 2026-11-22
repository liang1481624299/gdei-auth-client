import QtQuick
import QtQuick.Layouts
import "../theme"
import "."

Rectangle {
    id: root
    Theme { id: th }

    property int currentIndex: 0
    signal tabSelected(int index)

    implicitHeight: 72
    color: th.card

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: th.separator
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: th.sp3
        spacing: th.sp2

        AppTabButton {
            text: (app.i18n.lang, app.i18n.text("nav.dashboard"))
            iconName: "home"
            selected: root.currentIndex === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            onClicked: root.tabSelected(0)
        }

        AppTabButton {
            text: (app.i18n.lang, app.i18n.text("nav.devices"))
            iconName: "server"
            selected: root.currentIndex === 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            onClicked: root.tabSelected(1)
        }

        AppTabButton {
            text: (app.i18n.lang, app.i18n.text("nav.profile"))
            iconName: "person"
            selected: root.currentIndex === 2
            showBadge: app.events.unreadCount > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            onClicked: root.tabSelected(2)
        }
    }
}

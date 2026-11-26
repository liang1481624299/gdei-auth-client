import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// NavEntryRow —— 可复用跳转入口行：左图标 + 标题/副标题 + 可选未读红点 + 右箭头。
// Shared navigation entry row: leading icon, title/subtitle, optional unread dot, trailing chevron.
// 合并原 SettingsPage.entryRow 与 DevicesPage.navEntryComp 两份重复内联实现。
QQC.Button {
    id: root
    Theme { id: th }

    property string title: ""
    property string subtitle: ""
    property string iconName: ""
    property bool badge: false

    Layout.fillWidth: true
    implicitHeight: Math.max(th.touchTarget, contentRow.implicitHeight + th.sp6)
    padding: 0
    hoverEnabled: true

    contentItem: RowLayout {
        id: contentRow
        spacing: th.sp4

        Rectangle {
            visible: root.iconName.length > 0
            Layout.preferredWidth: th.dp(40)
            Layout.preferredHeight: th.dp(40)
            Layout.alignment: Qt.AlignVCenter
            radius: th.radiusControl
            color: th.field
            Icon {
                anchors.centerIn: parent
                name: root.iconName
                iconSize: th.dp(18)
                tint: th.secondaryLabel
            }
        }

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true

            QQC.Label {
                text: root.title
                color: th.text
                font.pixelSize: th.fontBody
                font.weight: th.weightMedium
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            QQC.Label {
                text: root.subtitle
                color: th.muted
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                visible: text.length > 0
                Layout.fillWidth: true
            }
        }

        UnreadBadge {
            visible: root.badge && app.events.unreadCount > 0
            Layout.alignment: Qt.AlignVCenter
        }

        Icon {
            name: "chevronRight"
            iconSize: th.dp(16)
            tint: th.muted
            Layout.alignment: Qt.AlignVCenter
        }
    }

    background: Rectangle {
        radius: th.radiusControl
        color: root.down ? th.controlPressed : root.hovered ? th.controlHover : "transparent"
        Behavior on color { ColorAnimation { duration: th.durationFast } }
    }
}

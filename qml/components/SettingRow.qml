import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Item {
    id: root
    Theme { id: th }

    property string title: ""
    property string description: ""
    property string leadingIcon: ""
    // 图标是否置于圆角底色方块内（参考功能行样式）；默认 false 保持裸图标。
    // Wrap the leading icon in a rounded tinted block; false keeps the bare icon.
    property bool iconContainer: false
    default property alias content: trailing.children

    implicitHeight: th.dp(56)

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: th.sp5

        Rectangle {
            visible: root.iconContainer && root.leadingIcon.length > 0
            Layout.preferredWidth: th.dp(40)
            Layout.preferredHeight: th.dp(40)
            Layout.alignment: Qt.AlignVCenter
            radius: th.radiusControl
            color: th.field
            Icon {
                anchors.centerIn: parent
                name: root.leadingIcon
                iconSize: th.dp(18)
                tint: th.secondaryLabel
            }
        }

        Icon {
            name: root.leadingIcon
            iconSize: th.dp(22)
            tint: th.muted
            visible: !root.iconContainer && root.leadingIcon.length > 0
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            spacing: th.sp1
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            QQC.Label {
                text: root.title
                color: th.text
                font.pixelSize: th.fontBody
                font.weight: th.weightMedium
                wrapMode: Text.Wrap
                visible: text.length > 0
                Layout.fillWidth: true
            }
            QQC.Label {
                text: root.description
                color: th.muted
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                visible: text.length > 0
                Layout.fillWidth: true
            }
        }

        Item {
            id: trailing
            Layout.preferredWidth: childrenRect.width
            Layout.preferredHeight: Math.max(th.touchTarget, childrenRect.height)
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
    }
}

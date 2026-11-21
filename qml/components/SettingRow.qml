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
    default property alias content: trailing.children

    implicitHeight: Math.max(th.touchTarget, row.implicitHeight + th.sp4)

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: th.sp6

        Icon {
            name: root.leadingIcon
            iconSize: 18
            tint: th.muted
            visible: root.leadingIcon.length > 0
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            spacing: 2
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

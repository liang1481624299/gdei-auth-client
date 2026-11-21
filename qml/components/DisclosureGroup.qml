import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Item {
    id: root
    Theme { id: th }

    property string title: ""
    property string leadingIcon: ""
    property bool opened: true
    default property alias content: body.children

    implicitHeight: header.height + (opened ? body.implicitHeight + th.sp4 : 0)
    Behavior on implicitHeight {
        NumberAnimation { duration: th.durationNormal; easing.type: Easing.InOutCubic }
    }

    Column {
        anchors.fill: parent
        spacing: th.sp4

        QQC.Button {
            id: header
            width: parent.width
            height: th.touchTarget
            padding: 0
            hoverEnabled: true
            onClicked: root.opened = !root.opened

            contentItem: RowLayout {
                spacing: 8

                Icon {
                    name: root.leadingIcon
                    iconSize: 18
                    tint: th.primary
                    visible: root.leadingIcon.length > 0
                    Layout.alignment: Qt.AlignVCenter
                }
                QQC.Label {
                    text: root.title
                    color: th.text
                    font.weight: th.weightSemibold
                    font.pixelSize: th.fontHeadline
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
                Icon {
                    name: "chevronDown"
                    iconSize: 14
                    tint: th.muted
                    rotation: root.opened ? 0 : -90
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: 8
                    Behavior on rotation {
                        NumberAnimation { duration: th.durationNormal; easing.type: Easing.InOutCubic }
                    }
                }
            }

            background: Rectangle {
                radius: th.radiusSmall
                color: header.down ? th.controlPressed : header.hovered ? th.controlHover : "transparent"
                border.width: header.activeFocus ? 1 : 0
                border.color: th.primary
                Behavior on color { ColorAnimation { duration: th.durationFast } }
            }
        }

        Column {
            id: body
            width: parent.width
            spacing: th.sp6
            opacity: root.opened ? 1 : 0
            visible: root.opened || opacity > 0.01
            Behavior on opacity { NumberAnimation { duration: th.durationFast } }
        }
    }
}

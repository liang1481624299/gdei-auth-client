import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

QQC.Button {
    id: control
    Theme { id: th }

    property string iconName: ""
    property bool selected: false
    property bool showBadge: false

    readonly property color foregroundColor: !enabled ? th.disabledText
                                             : selected ? th.primary : th.muted

    implicitWidth: 76
    implicitHeight: 60
    padding: th.sp3
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font.pixelSize: th.fontFootnote
    font.weight: selected ? th.weightSemibold : th.weightMedium
    opacity: enabled ? 1 : th.disabledOpacity
    scale: down ? 0.985 : 1

    Behavior on scale {
        SpringAnimation { spring: 5; damping: 0.78; epsilon: 0.001 }
    }

    contentItem: ColumnLayout {
        spacing: 3

        Item {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 26
            Layout.alignment: Qt.AlignHCenter

            Icon {
                id: icon
                anchors.centerIn: parent
                name: control.iconName
                iconSize: 22
                tint: control.foregroundColor
                Behavior on tint { ColorAnimation { duration: th.durationFast } }
            }

            UnreadBadge {
                visible: control.showBadge
                anchors.left: icon.right
                anchors.leftMargin: -4
                anchors.top: parent.top
                anchors.topMargin: 1
            }
        }

        QQC.Label {
            text: control.text
            color: control.foregroundColor
            font: control.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Behavior on color { ColorAnimation { duration: th.durationFast } }
        }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: th.radius
            color: "transparent"
            border.width: 2
            border.color: th.focusRing
            opacity: control.activeFocus ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: th.durationFast } }
        }

        Rectangle {
            anchors.fill: parent
            radius: th.radiusSmall
            color: control.down ? th.controlPressed
                 : control.hovered ? th.controlHover
                 : control.selected ? th.blueFill : "transparent"
            Behavior on color { ColorAnimation { duration: th.durationFast } }
        }
    }
}

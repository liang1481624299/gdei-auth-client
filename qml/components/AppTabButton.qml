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

    implicitHeight: th.dp(49)
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font.pixelSize: th.fontCaption
    font.weight: selected ? th.weightMedium : th.weightRegular
    opacity: enabled ? 1 : th.disabledOpacity

    contentItem: ColumnLayout {
        spacing: th.dp(2)
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter

        Item {
            Layout.preferredWidth: th.dp(26)
            Layout.preferredHeight: th.dp(25)
            Layout.alignment: Qt.AlignHCenter

            Icon {
                id: icon
                anchors.centerIn: parent
                name: control.iconName
                iconSize: th.dp(25)
                tint: control.foregroundColor
                Behavior on tint { ColorAnimation { duration: th.durationFast } }
            }

            UnreadBadge {
                visible: control.showBadge
                anchors.left: icon.right
                anchors.leftMargin: -th.dp(4)
                anchors.top: parent.top
                anchors.topMargin: th.dp(1)
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
            anchors.margins: -th.dp(2)
            radius: th.radius
            color: "transparent"
            border.width: th.dp(2)
            border.color: th.focusRing
            opacity: control.activeFocus ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: th.durationFast } }
        }
    }
}

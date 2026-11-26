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

    // 选中态用主题主色，未选用静音色；禁用态用禁用文字色，全部走 Theme 令牌。
    // Active uses theme primary, inactive muted; disabled uses disabled text.
    readonly property color foregroundColor: !enabled ? th.disabledText
                                             : selected ? th.primary : th.muted

    implicitHeight: th.dp(46)
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font.pixelSize: th.fontCaption
    font.weight: selected ? th.weightMedium : th.weightRegular
    opacity: enabled ? 1 : th.disabledOpacity

    contentItem: ColumnLayout {
        spacing: th.sp1
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter

        Item {
            id: iconSlot
            Layout.preferredWidth: th.dp(24)
            Layout.preferredHeight: th.dp(24)
            Layout.alignment: Qt.AlignHCenter

            // 选中态：图标置于主色浅底圆角方块内；未选保持裸图标。
            Rectangle {
                anchors.centerIn: parent
                width: th.dp(24)
                height: th.dp(24)
                radius: th.radiusControl
                color: control.selected ? th.primaryFill : "transparent"
                visible: control.selected
                Behavior on color { ColorAnimation { duration: th.durationFast } }

                Icon {
                    anchors.centerIn: parent
                    name: control.iconName
                    iconSize: th.dp(18)
                    tint: control.foregroundColor
                    Behavior on tint { ColorAnimation { duration: th.durationFast } }
                }
            }

            Icon {
                id: icon
                anchors.centerIn: parent
                name: control.iconName
                iconSize: th.dp(20)
                tint: control.foregroundColor
                visible: !control.selected
                Behavior on tint { ColorAnimation { duration: th.durationFast } }
            }

            // 未读角标锚定到图标槽位，避免定位随选中态失效。
            UnreadBadge {
                visible: control.showBadge
                anchors.left: iconSlot.right
                anchors.leftMargin: -th.dp(4)
                anchors.top: iconSlot.top
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

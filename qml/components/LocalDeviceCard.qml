import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Card {
    id: root
    readonly property Theme th: Theme {}

    property string deviceId: ""
    property string name: ""
    property string username: ""
    property string host: ""
    property string ip: ""
    property string nasip: ""
    property bool selected: false
    property bool showSelect: false
    property bool showManage: true

    signal selectRequested()
    signal editRequested()
    signal deleteRequested()
    signal manageRequested()

    margins: th.sp8
    border.color: selected ? th.primary : th.separator

    function localized(key) {
        app.i18n.lang;
        return app.i18n.text(key);
    }

    function displayValue(value) {
        return value.length > 0 ? value : localized("common.unavailable");
    }

    ColumnLayout {
        width: parent.width
        spacing: th.sp5

        RowLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: th.touchTarget
                Layout.preferredHeight: th.touchTarget
                Layout.alignment: Qt.AlignTop
                radius: th.radius
                color: root.selected ? th.blueFill : th.secondaryBg

                Icon {
                    anchors.centerIn: parent
                    name: "server"
                    iconSize: 21
                    tint: root.selected ? th.primary : th.muted
                }
            }

            ColumnLayout {
                spacing: th.sp2
                Layout.fillWidth: true

                QQC.Label {
                    text: root.displayValue(root.name)
                    color: th.text
                    font.pixelSize: th.fontHeadline
                    font.weight: th.weightSemibold
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }

                StatusBadge {
                    text: root.localized("profiles.selected")
                    iconName: "check"
                    tint: th.primary
                    visible: root.selected
                    Layout.alignment: Qt.AlignLeft
                }
            }

            QQC.Button {
                id: menuButton
                implicitWidth: th.touchTarget
                implicitHeight: th.touchTarget
                padding: 0
                hoverEnabled: true
                Accessible.name: root.localized("profiles.manage")
                onClicked: deviceMenu.open()

                contentItem: Icon {
                    name: "ellipsis"
                    iconSize: 21
                    tint: menuButton.hovered || deviceMenu.visible ? th.primary : th.muted
                }

                background: Rectangle {
                    radius: th.radiusSmall
                    color: menuButton.down ? th.controlPressed
                         : menuButton.hovered || deviceMenu.visible ? th.controlHover : "transparent"
                    border.width: menuButton.activeFocus ? 2 : 0
                    border.color: th.focusRing
                    Behavior on color { ColorAnimation { duration: th.durationFast } }
                }

                QQC.Menu {
                    id: deviceMenu
                    y: menuButton.height
                    padding: th.sp2

                    QQC.MenuItem {
                        text: root.localized("profiles.select")
                        visible: root.showSelect
                        enabled: !root.selected
                        implicitHeight: visible ? th.touchTarget : 0
                        onTriggered: root.selectRequested()
                    }
                    QQC.MenuItem {
                        text: root.localized("profiles.edit")
                        implicitHeight: th.touchTarget
                        onTriggered: root.editRequested()
                    }
                    QQC.MenuItem {
                        text: root.localized("profiles.manage")
                        visible: root.showManage
                        implicitHeight: visible ? th.touchTarget : 0
                        onTriggered: root.manageRequested()
                    }
                    QQC.MenuSeparator { visible: root.showSelect || root.showManage }
                    QQC.MenuItem {
                        id: deleteItem
                        text: root.localized("profiles.delete")
                        implicitHeight: th.touchTarget
                        onTriggered: root.deleteRequested()

                        contentItem: QQC.Label {
                            text: deleteItem.text
                            color: deleteItem.enabled ? th.danger : th.disabledText
                            font: deleteItem.font
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 12
                            rightPadding: 12
                        }
                        background: Rectangle {
                            radius: th.radiusSmall
                            color: deleteItem.highlighted || deleteItem.hovered ? th.redFill : "transparent"
                        }
                    }

                    background: Rectangle {
                        implicitWidth: 180
                        radius: th.radius
                        color: th.surfaceElevated
                        border.width: 1
                        border.color: th.separator
                    }
                }
            }
        }

        QQC.Label {
            text: root.localized("label.username") + ": " + root.displayValue(root.username)
            color: th.muted
            font.pixelSize: th.fontSubhead
            wrapMode: Text.WrapAnywhere
            Layout.fillWidth: true
        }

        QQC.Label {
            text: root.localized("label.ip") + ": " + root.displayValue(root.ip)
            color: th.muted
            font.pixelSize: th.fontSubhead
            wrapMode: Text.WrapAnywhere
            Layout.fillWidth: true
        }
    }
}

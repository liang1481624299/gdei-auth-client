import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root
    Theme { id: th }

    signal openRemoteRequested()

    property string pendingDeleteId: ""

    title: "profiles.title"
    subtitle: "profiles.subtitle"
    showBack: false
    contentMaxWidth: 860

    function editProfile(id) {
        profileEditor.edit(id);
    }

    function requestDelete(id) {
        pendingDeleteId = id;
        deleteDialog.open();
    }

    RowLayout {
        Layout.fillWidth: true

        Item { Layout.fillWidth: true }
        PrimaryButton {
            text: (app.i18n.lang, app.i18n.text("profiles.add"))
            leadingIcon: "plus"
            onClicked: profileEditor.create()
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
    }

    Card {
        Layout.fillWidth: true
        visible: app.localDevices.count === 0

        QQC.Label {
            width: parent.width
            text: (app.i18n.lang, app.i18n.text("profiles.empty"))
            color: th.muted
            font.pixelSize: th.fontBody
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }

    Repeater {
        model: app.localDevices.model

        delegate: LocalDeviceCard {
            required property var model

            Layout.fillWidth: true
            deviceId: model.id
            name: model.name
            username: model.username
            host: model.host
            ip: model.ip
            nasip: model.nasip
            selected: app.localDevices.selectedId === model.id
            showSelect: true
            showManage: false
            onSelectRequested: app.localDevices.selectProfile(deviceId)
            onEditRequested: root.editProfile(deviceId)
            onDeleteRequested: root.requestDelete(deviceId)
        }
    }

    Card {
        Layout.fillWidth: true
        margins: th.sp4

        QQC.Button {
            id: remoteButton
            width: parent.width
            implicitHeight: Math.max(th.touchTarget, remoteRow.implicitHeight)
            padding: 0
            hoverEnabled: true
            onClicked: root.openRemoteRequested()

            contentItem: RowLayout {
                id: remoteRow
                spacing: th.sp6

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("profiles.remoteEntry"))
                        color: th.text
                        font.pixelSize: th.fontHeadline
                        font.weight: th.weightSemibold
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("profiles.remoteSubtitle"))
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
                Icon {
                    name: "chevronRight"
                    iconSize: 16
                    tint: th.muted
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            background: Rectangle {
                radius: th.radiusSmall
                color: remoteButton.down ? th.controlPressed
                     : remoteButton.hovered ? th.controlHover : "transparent"
                border.width: remoteButton.activeFocus ? 2 : 0
                border.color: th.focusRing
            }
        }
    }

    LocalDeviceEditor {
        id: profileEditor
    }

    AppDialog {
        id: deleteDialog
        title: (app.i18n.lang, app.i18n.text("profiles.deleteTitle"))
        message: (app.i18n.lang, app.i18n.text("profiles.deleteMessage"))
        confirmText: (app.i18n.lang, app.i18n.text("action.delete"))
        cancelText: (app.i18n.lang, app.i18n.text("action.cancel"))
        showCancel: true
        destructive: true
        onAccepted: {
            if (root.pendingDeleteId.length > 0)
                app.localDevices.removeProfile(root.pendingDeleteId);
            root.pendingDeleteId = "";
        }
        onRejected: root.pendingDeleteId = ""
    }
}

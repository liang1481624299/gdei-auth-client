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
    contentMaxWidth: th.dp(480)

    function editProfile(id) {
        profileEditor.edit(id);
    }

    function requestDelete(id) {
        pendingDeleteId = id;
        deleteDialog.open();
    }

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        // 分组 1：本地认证配置（保活程序使用的本机账号数据）。
        // Group 1: local auth profiles used by the keep-alive client.
        Card {
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: th.sp4

                RowLayout {
                    width: parent.width
                    spacing: th.sp4

                    Icon {
                        name: "person"
                        iconSize: 18
                        tint: th.primary
                        Layout.alignment: Qt.AlignVCenter
                    }
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter

                        QQC.Label {
                            text: root.localized("profiles.localGroup")
                            color: th.text
                            font.pixelSize: th.fontHeadline
                            font.weight: th.weightSemibold
                            Layout.fillWidth: true
                        }
                        QQC.Label {
                            text: root.localized("profiles.localGroupHint")
                            color: th.muted
                            font.pixelSize: th.fontFootnote
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                    PrimaryButton {
                        text: (app.i18n.lang, app.i18n.text("profiles.add"))
                        leadingIcon: "plus"
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        onClicked: profileEditor.create()
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
            }
        }

        // 分组 2：校园网在线设备（学校服务器数据，点击进入门户设备页）。
        // Group 2: online devices reported by the campus portal server.
        Card {
            Layout.fillWidth: true
            margins: th.sp4

            ColumnLayout {
                width: parent.width
                spacing: th.sp4

                RowLayout {
                    width: parent.width
                    spacing: th.sp4

                    Icon {
                        name: "server"
                        iconSize: 18
                        tint: th.primary
                        Layout.alignment: Qt.AlignVCenter
                    }
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter

                        QQC.Label {
                            text: root.localized("profiles.cloudGroup")
                            color: th.text
                            font.pixelSize: th.fontHeadline
                            font.weight: th.weightSemibold
                            Layout.fillWidth: true
                        }
                        QQC.Label {
                            text: root.localized("profiles.cloudGroupHint")
                            color: th.muted
                            font.pixelSize: th.fontFootnote
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                QQC.Button {
                    id: remoteButton
                    width: parent.width
                    implicitHeight: Math.max(th.touchTarget, remoteRow.implicitHeight)
                    padding: 0
                    hoverEnabled: true
                    onClicked: {
                        if (app.session.requireLogin("remote.open"))
                            root.openRemoteRequested();
                    }

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

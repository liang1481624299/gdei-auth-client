import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root
    Theme { id: th }

    signal manageDevicesRequested()

    property string pendingDeleteId: ""
    readonly property var currentProfile: {
        var selectedId = app.localDevices.selectedId;
        app.localDevices.selectedName;
        return selectedId.length > 0 ? app.localDevices.profile(selectedId) : ({});
    }
    readonly property string statusText: {
        app.i18n.lang;
        if (app.auth.state === 2) return app.i18n.text("status.authenticating");
        return app.i18n.text(app.auth.online ? "status.online" : "status.offline");
    }
    readonly property color statusTint: app.auth.state === 2 ? th.warning
                                      : app.auth.online ? th.success : th.muted
    readonly property bool genuinelyOffline: !app.auth.online && app.auth.state !== 2
                                            && !app.session.busy
    readonly property bool showConnectivityWarning: genuinelyOffline || !app.network.available

    title: "dashboard.title"
    subtitle: "dashboard.subtitle"
    showBack: false
    contentMaxWidth: 860

    function requestDelete(id) {
        pendingDeleteId = id;
        deleteDialog.open();
    }

    Component.onCompleted: app.network.refresh()
    onVisibleChanged: if (visible) app.network.refresh()

    Card {
        Layout.fillWidth: true

        RowLayout {
            width: parent.width
            spacing: th.sp6

            QQC.Label {
                text: (app.i18n.lang, app.i18n.text("section.connection"))
                color: th.text
                font.pixelSize: th.fontHeadline
                font.weight: th.weightSemibold
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            StatusBadge {
                text: root.statusText
                tint: root.statusTint
                iconName: app.auth.state === 2 ? "refresh"
                                               : app.auth.online ? "wifi" : "wifiOff"
                Layout.alignment: Qt.AlignVCenter
            }
        }

        Item {
            width: parent.width
            implicitHeight: dial.height + th.sp4

            KeepAliveDial {
                id: dial
                width: Math.min(220, parent.width)
                height: width
                anchors.centerIn: parent
                running: app.auth.keepAliveRunning
                online: app.auth.online
                busy: app.session.busy
                onToggleRequested: {
                    if (app.auth.keepAliveRunning) app.auth.stopKeepAlive();
                    else app.auth.startKeepAlive();
                }
            }
        }
    }

    LocalDeviceCard {
        id: currentDeviceCard
        Layout.fillWidth: true
        visible: app.localDevices.selectedId.length > 0
        deviceId: root.currentProfile.id || ""
        name: root.currentProfile.name || ""
        username: root.currentProfile.username || ""
        host: root.currentProfile.host || ""
        ip: root.currentProfile.ip || ""
        nasip: root.currentProfile.nasip || ""
        selected: true
        showSelect: false
        onManageRequested: root.manageDevicesRequested()
        onEditRequested: profileEditor.edit(deviceId)
        onDeleteRequested: root.requestDelete(deviceId)
    }

    Card {
        Layout.fillWidth: true
        visible: root.showConnectivityWarning
        margins: th.sp6
        color: th.orangeFill
        border.color: th.fill(th.warning, 0.32)

        QQC.Label {
            width: parent.width
            text: !app.network.available
                  ? (app.i18n.lang, app.i18n.text("network.localUnavailableMessage"))
                  : (app.i18n.lang, app.i18n.text("network.offlineMessage"))
            color: th.text
            font.pixelSize: th.fontBody
            wrapMode: Text.Wrap
        }
    }

    NetworkInfoCard {
        Layout.fillWidth: true
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

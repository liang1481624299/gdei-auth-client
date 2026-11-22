import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

QQC.Dialog {
    id: dialog
    Theme { id: th }

    property string profileId: ""

    parent: QQC.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(460, Math.max(0, parent.width - th.sp8))
    height: Math.min(620, Math.max(0, parent.height - th.sp8))
    modal: true
    focus: true
    padding: th.sp8
    closePolicy: QQC.Popup.CloseOnEscape
    title: profileId.length > 0
           ? (app.i18n.lang, app.i18n.text("profiles.edit"))
           : (app.i18n.lang, app.i18n.text("profiles.add"))

    function create() {
        profileId = "";
        nameField.text = "";
        usernameField.text = "";
        passwordField.text = "";
        passwordField.passwordVisible = false;
        hostField.text = "";
        ipField.text = "";
        nasipField.text = "";
        rememberSwitch.checked = false;
        open();
    }

    function edit(id) {
        var profile = app.localDevices.profile(id);
        if (!profile || !profile.id) return;

        profileId = id;
        nameField.text = profile.name || "";
        usernameField.text = profile.username || "";
        passwordField.text = "";
        passwordField.passwordVisible = false;
        hostField.text = profile.host || "";
        ipField.text = profile.ip || "";
        nasipField.text = profile.nasip || "";
        rememberSwitch.checked = profile.rememberPassword === true;
        open();
    }

    function save() {
        var saved;
        if (profileId.length > 0) {
            saved = app.localDevices.updateProfile(
                        profileId, nameField.text, usernameField.text, passwordField.text,
                        hostField.text, ipField.text, nasipField.text, rememberSwitch.checked);
        } else {
            saved = app.localDevices.addProfile(
                        nameField.text, usernameField.text, passwordField.text,
                        hostField.text, ipField.text, nasipField.text,
                        rememberSwitch.checked).length > 0;
        }
        if (saved) accept();
    }

    onOpened: nameField.forceActiveFocus()
    onClosed: {
        passwordField.text = "";
        passwordField.passwordVisible = false;
        profileId = "";
    }

    QQC.Overlay.modal: Rectangle { color: th.overlay }

    header: QQC.Label {
        text: dialog.title
        color: th.text
        font.pixelSize: th.fontTitle3
        font.weight: th.weightSemibold
        wrapMode: Text.Wrap
        leftPadding: th.sp10
        rightPadding: th.sp10
        topPadding: th.sp10
        bottomPadding: th.sp4
    }

    contentItem: QQC.ScrollView {
        id: scroll
        clip: true
        contentWidth: availableWidth
        contentHeight: form.implicitHeight
        QQC.ScrollBar.vertical.policy: QQC.ScrollBar.AsNeeded

        WheelHandler {
            onWheel: function(event) {
                var delta = event.pixelDelta.y;
                if (Math.abs(delta) < 0.01)
                    delta = event.angleDelta.y / 120 * 96;
                if (Math.abs(delta) < 0.01)
                    return;

                var flickable = scroll.contentItem;
                var maximumY = Math.max(0, flickable.contentHeight - flickable.height);
                var nextY = Math.max(0, Math.min(maximumY, flickable.contentY - delta));
                if (Math.abs(nextY - flickable.contentY) < 0.01) {
                    event.accepted = false;
                    return;
                }
                flickable.contentY = nextY;
                event.accepted = true;
            }
        }

        ColumnLayout {
            id: form
            width: Math.max(0, scroll.availableWidth)
            spacing: th.sp5

            ColumnLayout {
                spacing: th.sp3
                Layout.fillWidth: true
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("profiles.name"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }
                TextField {
                    id: nameField
                    leadingIcon: "person"
                    placeholderText: (app.i18n.lang, app.i18n.text("profiles.namePlaceholder"))
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: th.sp3
                Layout.fillWidth: true
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("label.username"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }
                TextField {
                    id: usernameField
                    leadingIcon: "person"
                    placeholderText: (app.i18n.lang, app.i18n.text("placeholder.username"))
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: th.sp3
                Layout.fillWidth: true
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("label.password"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }
                TextField {
                    id: passwordField
                    leadingIcon: "lock"
                    passwordMode: true
                    placeholderText: dialog.profileId.length > 0
                                     ? (app.i18n.lang, app.i18n.text("profiles.passwordKeep"))
                                     : (app.i18n.lang, app.i18n.text("placeholder.password"))
                    showPasswordAccessibleText: (app.i18n.lang, app.i18n.text("action.showPassword"))
                    hidePasswordAccessibleText: (app.i18n.lang, app.i18n.text("action.hidePassword"))
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: th.sp3
                Layout.fillWidth: true
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("label.host"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }
                TextField {
                    id: hostField
                    leadingIcon: "globe"
                    placeholderText: (app.i18n.lang, app.i18n.text("profiles.hostPlaceholder"))
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: th.sp3
                Layout.fillWidth: true
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("label.ip"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }
                TextField {
                    id: ipField
                    leadingIcon: "server"
                    placeholderText: (app.i18n.lang, app.i18n.text("profiles.ipPlaceholder"))
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: th.sp3
                Layout.fillWidth: true
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("label.nasip"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }
                TextField {
                    id: nasipField
                    leadingIcon: "server"
                    placeholderText: (app.i18n.lang, app.i18n.text("profiles.nasipPlaceholder"))
                    Layout.fillWidth: true
                }
            }

            SettingRow {
                title: (app.i18n.lang, app.i18n.text("action.remember"))
                Layout.fillWidth: true
                Switch { id: rememberSwitch }
            }
        }
    }

    footer: RowLayout {
        spacing: th.sp4

        Item { Layout.fillWidth: true }
        PrimaryButton {
            text: (app.i18n.lang, app.i18n.text("action.cancel"))
            variant: "ghost"
            onClicked: dialog.reject()
        }
        PrimaryButton {
            text: (app.i18n.lang, app.i18n.text("btn.save"))
            onClicked: dialog.save()
        }
    }

    background: Rectangle {
        radius: th.radiusLarge
        color: th.surfaceElevated
        border.width: 1
        border.color: th.separator
    }

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: th.durationNormal } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: th.durationFast } }
}

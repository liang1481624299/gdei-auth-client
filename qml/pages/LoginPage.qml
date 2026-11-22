import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: root
    Theme { id: th }

    function submitLogin() {
        if (!app.session.busy) app.session.login();
    }

    onVisibleChanged: if (!visible) passwordField.passwordVisible = false

    QQC.ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        contentHeight: page.height
        QQC.ScrollBar.vertical.policy: QQC.ScrollBar.AsNeeded
        background: Item { }

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

        Item {
            id: page
            width: scroll.availableWidth
            height: Math.max(root.height,
                             loginCard.implicitHeight + th.sp16 * 2)

            Card {
                id: loginCard
                anchors.centerIn: parent
                width: Math.min(440, Math.max(0, parent.width - th.sp12 * 2))
                margins: th.sp10

                ColumnLayout {
                    width: parent.width
                    spacing: th.sp3

                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("app.title"))
                        color: th.text
                        font.pixelSize: th.fontTitle2
                        font.weight: th.weightSemibold
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("app.subtitle"))
                        color: th.muted
                        font.pixelSize: th.fontBody
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    LangSelector {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: th.sp3
                    }
                }

                ColumnLayout {
                    width: parent.width
                    spacing: th.sp3

                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("label.username"))
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    TextField {
                        leadingIcon: "person"
                        placeholderText: (app.i18n.lang, app.i18n.text("placeholder.username"))
                        text: app.config.username
                        enabled: !app.session.busy
                        onTextChanged: app.config.username = text
                        onAccepted: root.submitLogin()
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    width: parent.width
                    spacing: th.sp3

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
                        placeholderText: (app.i18n.lang, app.i18n.text("placeholder.password"))
                        text: app.config.password
                        showPasswordAccessibleText: (app.i18n.lang, app.i18n.text("action.showPassword"))
                        hidePasswordAccessibleText: (app.i18n.lang, app.i18n.text("action.hidePassword"))
                        enabled: !app.session.busy
                        onTextChanged: app.config.password = text
                        onAccepted: root.submitLogin()
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    width: parent.width
                    implicitHeight: settingsColumn.implicitHeight
                    radius: th.radiusSmall
                    color: th.field
                    border.width: 1
                    border.color: th.separator

                    Column {
                        id: settingsColumn
                        anchors.left: parent.left
                        anchors.right: parent.right

                        SettingRow {
                            width: parent.width
                            title: (app.i18n.lang, app.i18n.text("action.remember"))
                            Switch {
                                checked: app.config.rememberPassword
                                onToggled: app.config.rememberPassword = checked
                            }
                        }
                        Rectangle {
                            width: Math.max(0, parent.width - th.sp12)
                            height: 1
                            color: th.separator
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        SettingRow {
                            width: parent.width
                            title: (app.i18n.lang, app.i18n.text("action.autoLogin"))
                            Switch {
                                checked: app.config.autoLogin
                                onToggled: app.config.autoLogin = checked
                            }
                        }
                    }
                }

                PrimaryButton {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("btn.login"))
                    leadingIcon: "power"
                    busy: app.session.busy
                    onClicked: root.submitLogin()
                }
            }
        }
    }

    AppDialog {
        id: loginErrorDialog
        title: (app.i18n.lang, app.i18n.text("login.failed"))
        detail: app.session.lastError
        confirmText: (app.i18n.lang, app.i18n.text("action.ok"))
    }

    Connections {
        target: app.session
        function onLastErrorChanged() {
            if (root.visible && app.session.lastError.length > 0)
                loginErrorDialog.open();
        }
    }
}

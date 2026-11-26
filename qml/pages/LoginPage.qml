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
                    delta = event.angleDelta.y / 120 * th.wheelStep;
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

            BaseCard {
                id: loginCard
                anchors.centerIn: parent
                width: Math.min(th.dp(440), Math.max(0, parent.width - th.sp12 * 2))
                cardPadding: th.sp10
                hoverable: false

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: th.sp3

                    QQC.Label {
                        text: (app.i18n.dictionary["app.title"])
                        color: th.text
                        font.pixelSize: th.fontTitle2
                        font.weight: th.weightSemibold
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (app.i18n.dictionary["app.subtitle"])
                        color: th.muted
                        font.pixelSize: th.fontBody
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (app.i18n.dictionary["login.guestHint"])
                        color: th.muted
                        font.pixelSize: th.fontFootnote
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
                    Layout.fillWidth: true
                    spacing: th.sp3

                    QQC.Label {
                        text: (app.i18n.dictionary["label.username"])
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    BaseTextField {
                        placeholderText: (app.i18n.dictionary["placeholder.username"])
                        text: app.config.username
                        enabled: !app.session.busy
                        onEdited: app.setLoginUsername(text)
                        onAccepted: root.submitLogin()
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: th.sp3

                    QQC.Label {
                        text: (app.i18n.dictionary["label.password"])
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    BaseTextField {
                        id: passwordField
                        password: true
                        placeholderText: (app.i18n.dictionary["placeholder.password"])
                        text: app.config.password
                        enabled: !app.session.busy
                        onEdited: app.setLoginPassword(text)
                        onAccepted: root.submitLogin()
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
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
                            title: (app.i18n.dictionary["action.remember"])
                            BaseSwitch {
                                checked: app.config.rememberPassword
                                onToggled: app.setRememberPassword(checked)
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
                            title: (app.i18n.dictionary["action.autoLogin"])
                            BaseSwitch {
                                checked: app.config.autoLogin
                                onToggled: app.setAutoLogin(checked)
                            }
                        }
                    }
                }

                BaseButton {
                    text: (app.i18n.dictionary["btn.login"])
                    type: "primary"
                    leadingIcon: "power"
                    busy: app.session.busy
                    Layout.fillWidth: true
                    onClicked: root.submitLogin()
                }
                BaseButton {
                    text: (app.i18n.dictionary["login.cancel"])
                    type: "secondary"
                    enabled: !app.session.busy
                    Layout.fillWidth: true
                    onClicked: app.session.dismissLoginPrompt()
                }
            }
        }
    }

    AppDialog {
        id: loginErrorDialog
        title: (app.i18n.dictionary["login.failed"])
        detail: app.session.lastError
        confirmText: (app.i18n.dictionary["action.ok"])
    }

    Connections {
        target: app.session
        function onLastErrorChanged() {
            if (root.visible && app.session.lastError.length > 0)
                loginErrorDialog.open();
        }
    }
}

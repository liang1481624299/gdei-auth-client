// qml/components/DevicePanel.qml
// 设备面板 / Device panel: self-service sign-in with captcha + read-only lists.
import QtQuick
import QtQuick.Controls
import "../theme"
import "."

Card {
    id: root

    property string codeText: ""

    Component.onCompleted: {
        if (!app.devices.authenticated) app.devices.fetchCaptcha();
    }

    Column {
        spacing: 12
        width: parent.width

        // ---- 标题栏 / Header ----
        Row {
            width: parent.width
            spacing: 10
            Label {
                text: app.i18n.lang, app.i18n.text("devices.title")
                color: theme.text
                font.bold: true
                font.pixelSize: 15
                anchors.verticalCenter: parent.verticalCenter
            }
            Item { width: 1; height: 1; anchors.verticalCenter: parent.verticalCenter }
            PrimaryButton {
                text: app.i18n.text("devices.refresh")
                bg: theme.muted
                implicitHeight: 34
                padding: 12
                enabled: app.devices.authenticated && !app.devices.busy
                onClicked: app.devices.refreshDevices()
            }
            PrimaryButton {
                text: app.i18n.text("devices.logout")
                bg: theme.danger
                implicitHeight: 34
                padding: 12
                visible: app.devices.authenticated
                onClicked: app.devices.clearSession()
            }
        }

        // ---- 未登录：验证码 / Signed-out: captcha ----
        Column {
            width: parent.width
            spacing: 8
            visible: !app.devices.authenticated

            Row {
                width: parent.width
                spacing: 12
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    id: captchaImg
                    width: 120
                    height: 44
                    fillMode: Image.PreserveAspectFit
                    source: app.devices.captchaImage
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle {
                        anchors.fill: parent
                        radius: theme.radiusSmall
                        border.width: 1
                        border.color: theme.border
                        visible: captchaImg.status !== Image.Ready
                    }
                }

                TextField {
                    width: parent.width - 132
                    placeholderText: app.i18n.text("devices.code")
                    text: root.codeText
                    onTextChanged: root.codeText = text
                    anchors.verticalCenter: parent.verticalCenter
                    enterPressed: app.devices.loginWithCode(root.codeText)
                }
            }

            PrimaryButton {
                width: parent.width
                text: app.i18n.text("devices.login")
                enabled: !app.devices.busy
                onClicked: {
                    app.devices.loginWithCode(root.codeText);
                    root.codeText = "";
                }
            }
        }

        // ---- 已登录：设备列表 / Signed-in: device list ----
        Column {
            width: parent.width
            spacing: 8
            visible: app.devices.authenticated

            ListView {
                id: deviceList
                width: parent.width
                height: 200
                clip: true
                spacing: 8
                model: app.devices.devices
                interactive: count > 0

                delegate: DeviceItem {
                    width: deviceList.width
                    device: modelData
                }

                Label {
                    anchors.centerIn: parent
                    text: app.i18n.text("devices.empty")
                    color: theme.muted
                    font.pixelSize: 13
                    visible: deviceList.count === 0
                }
            }
        }

        Label {
            width: parent.width
            text: app.devices.lastMessage
            color: theme.muted
            font.pixelSize: 12
            wrapMode: Text.Wrap
            visible: text.length > 0
        }

        Label {
            width: parent.width
            text: app.i18n.text("devices.unsupported")
            color: theme.warning
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }
    }
}

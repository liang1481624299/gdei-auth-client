// qml/Main.qml
// 主界面 / Root window. QML 只负责交互，业务逻辑经 app.* 暴露到 C++。
import QtQuick
import QtQuick.Controls
import "../theme"
import "components"

Window {
    id: root
    Theme { id: th }

    width: 460
    height: 760
    visible: true
    color: th.bg
    title: (app.i18n.lang, app.i18n.text("app.title"))

    // 状态徽标 / Status badge: [color, text]
    property var statusBadge: {
        app.i18n.lang;
        if (app.auth.state === 3)  return [th.success, app.i18n.text("status.online")];
        if (app.auth.state === 2)  return [th.warning, app.i18n.text("btn.status")];
        if (app.auth.state === 4)  return [th.danger,  app.i18n.text("login.failed")];
        return [th.muted, app.i18n.text("status.offline")];
    }

    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // ---- 头部 / Header ----
        Row {
            width: parent.width
            spacing: 12
            anchors.verticalCenter: parent.verticalCenter

            Label {
                text: app.i18n.lang, app.i18n.text("app.title")
                color: th.text
                font.bold: true
                font.pixelSize: 20
                anchors.verticalCenter: parent.verticalCenter
            }
            Item { width: 1; height: 1; anchors.verticalCenter: parent.verticalCenter }
            LangSelector {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // ---- 状态卡片 / Status card ----
        Card {
            width: parent.width
            Row {
                spacing: 14
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: root.statusBadge[0]
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    spacing: 4
                    Label {
                        text: root.statusBadge[1]
                        color: th.text
                        font.bold: true
                        font.pixelSize: 16
                    }
                    Label {
                        text: app.auth.lastMessage
                        color: th.muted
                        font.pixelSize: 13
                        width: parent.width
                        wrapMode: Text.Wrap
                        visible: text.length > 0
                    }
                }
            }
        }

        // ---- 凭据卡片 / Credentials card ----
        Card {
            width: parent.width
            spacing: 12

            Column {
                spacing: 6
                width: parent.width
                Label {
                    text: app.i18n.lang, app.i18n.text("label.username")
                    color: th.muted
                    font.pixelSize: 13
                }
                TextField {
                    width: parent.width
                    text: app.config.username
                    onTextChanged: app.config.username = text
                }
            }

            Column {
                spacing: 6
                width: parent.width
                Label {
                    text: app.i18n.lang, app.i18n.text("label.password")
                    color: th.muted
                    font.pixelSize: 13
                }
                TextField {
                    width: parent.width
                    passwordMode: true
                    text: app.config.password
                    onTextChanged: app.config.password = text
                }
            }

            Row {
                spacing: 20
                Switch {
                    checked: app.config.rememberPassword
                    onCheckedChanged: app.config.rememberPassword = checked
                }
                Label {
                    text: app.i18n.lang, app.i18n.text("action.remember")
                    color: th.text
                    font.pixelSize: 14
                    anchors.verticalCenter: parent.verticalCenter
                }
                Switch {
                    checked: app.config.autoLogin
                    onCheckedChanged: app.config.autoLogin = checked
                }
                Label {
                    text: app.i18n.lang, app.i18n.text("action.autoLogin")
                    color: th.text
                    font.pixelSize: 14
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Row {
                width: parent.width
                spacing: 10
                PrimaryButton {
                    text: app.i18n.lang, app.i18n.text("btn.login")
                    onClicked: app.auth.login()
                }
                PrimaryButton {
                    text: app.i18n.lang, app.i18n.text("btn.logout")
                    bg: th.danger
                    onClicked: app.auth.logout()
                }
                PrimaryButton {
                    text: app.i18n.lang, app.i18n.text("btn.status")
                    bg: th.muted
                    onClicked: app.auth.refreshStatus()
                }
            }

            PrimaryButton {
                width: parent.width
                text: app.auth.keepAliveRunning
                      ? app.i18n.text("btn.keepStop")
                      : app.i18n.text("btn.keepStart")
                bg: th.warning
                onClicked: app.auth.keepAliveRunning
                           ? app.auth.stopKeepAlive()
                           : app.auth.startKeepAlive()
            }
        }

        // ---- 高级设置 / Advanced settings ----
        Card {
            width: parent.width
            spacing: 12

            Row {
                spacing: 10
                Column {
                    spacing: 6
                    width: (parent.width - 10) / 2
                    Label {
                        text: app.i18n.lang, app.i18n.text("label.host")
                        color: th.muted
                        font.pixelSize: 13
                    }
                    TextField {
                        width: parent.width
                        text: app.config.host
                        onTextChanged: app.config.host = text
                    }
                }
                Column {
                    spacing: 6
                    width: (parent.width - 10) / 2
                    Label {
                        text: app.i18n.lang, app.i18n.text("label.ip")
                        color: th.muted
                        font.pixelSize: 13
                    }
                    TextField {
                        width: parent.width
                        text: app.config.ip
                        onTextChanged: app.config.ip = text
                    }
                }
            }

            Column {
                spacing: 6
                width: parent.width
                Label {
                    text: app.i18n.lang, app.i18n.text("label.nasip")
                    color: th.muted
                    font.pixelSize: 13
                }
                TextField {
                    width: parent.width
                    text: app.config.nasip
                    onTextChanged: app.config.nasip = text
                }
            }

            PrimaryButton {
                width: parent.width
                text: app.i18n.lang, app.i18n.text("btn.save")
                bg: th.primaryHover
                onClicked: app.config.save()
            }
        }

        // ---- 日志面板 / Log panel ----
        LogPanel { width: parent.width }
    }
}

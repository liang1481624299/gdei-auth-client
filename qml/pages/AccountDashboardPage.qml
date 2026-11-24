import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 仪表盘：账号业务数据（账号状态、余额、套餐流量、在线时长）。
// Dashboard: account business data (status, balance, plan data, session duration).
PageScaffold {
    id: root

    readonly property Theme th: Theme {}

    // 0 未知/未登录 1 会话正常 2 会话可能失效；受限/设备超限待门户会话接口回填。
    readonly property int accountState: !app.session.loggedIn ? 0
                                        : app.account.authenticated ? 1 : 2
    readonly property string stateKey: accountState === 1 ? "account.statusNormal"
                                      : accountState === 2 ? "account.statusExpired"
                                      : "account.statusUnknown"
    readonly property color stateTint: accountState === 1 ? th.success
                                      : accountState === 2 ? th.warning : th.muted

    readonly property var profile: app.account.profile
    readonly property string balanceValue: {
        var b = profile["balance"] || profile["remaining"] || profile["credit"];
        return (b !== undefined && b !== null && String(b).length > 0) ? String(b) : "";
    }
    readonly property string planValue: {
        var p = profile["plan"] || profile["traffic"] || profile["dataQuota"];
        return (p !== undefined && p !== null && String(p).length > 0) ? String(p) : "";
    }

    title: "nav.profile"

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        // 账号卡片：头像、用户名、会话状态标签。
        Card {
            Layout.fillWidth: true

            RowLayout {
                width: parent.width
                spacing: th.sp6

                Rectangle {
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    Layout.alignment: Qt.AlignVCenter
                    radius: 24
                    color: th.greenFill
                    Icon {
                        anchors.centerIn: parent
                        name: "person"
                        iconSize: 24
                        tint: th.primary
                    }
                }

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true

                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("label.username"))
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: app.config.username.length > 0
                              ? app.config.username
                              : (app.i18n.lang, app.i18n.text("account.notSignedIn"))
                        color: th.text
                        font.pixelSize: th.fontTitle3
                        font.weight: th.weightSemibold
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                    StatusBadge {
                        text: root.localized(root.stateKey)
                        tint: root.stateTint
                        iconName: accountState === 1 ? "check" : accountState === 2 ? "shield" : "person"
                        Layout.alignment: Qt.AlignLeft
                    }
                }
            }
        }

        // 余额模块：带刷新按钮；失败时展示原因，不使用静态“暂无法获取”。
        Card {
            Layout.fillWidth: true

            RowLayout {
                width: parent.width
                spacing: th.sp4

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true

                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("profile.balance"))
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        width: parent.width
                        text: root.balanceValue.length > 0 ? root.balanceValue
                              : app.account.lastMessage.length > 0 ? app.account.lastMessage
                              : (app.i18n.lang, app.i18n.text("profile.balanceUnavailable"))
                        color: root.balanceValue.length > 0 ? th.text : th.secondaryLabel
                        font.pixelSize: th.fontHeadline
                        font.weight: th.weightMedium
                        wrapMode: Text.Wrap
                    }
                }

                PrimaryButton {
                    text: (app.i18n.lang, app.i18n.text("account.refresh"))
                    variant: "secondary"
                    leadingIcon: "refresh"
                    busy: app.account.busy
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: app.account.refreshProfile()
                }
            }
        }

        // 套餐流量与在线时长；后端门户解析未回填前显示占位。
        Card {
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: th.sp5

                RowLayout {
                    width: parent.width
                    spacing: th.sp4
                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("account.planTraffic"))
                        color: th.muted
                        font.pixelSize: th.fontSubhead
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.planValue.length > 0 ? root.planValue
                              : (app.i18n.lang, app.i18n.text("common.unavailable"))
                        color: th.text
                        font.pixelSize: th.fontSubhead
                        Layout.alignment: Qt.AlignRight
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: th.separator
                }

                RowLayout {
                    width: parent.width
                    spacing: th.sp4
                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("account.onlineDuration"))
                        color: th.muted
                        font.pixelSize: th.fontSubhead
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("account.durationUnknown"))
                        color: th.text
                        font.pixelSize: th.fontSubhead
                        Layout.alignment: Qt.AlignRight
                    }
                }
            }
        }

        PrimaryButton {
            text: (app.i18n.lang, app.i18n.text("profile.logout"))
            variant: "danger"
            bg: "transparent"
            leadingIcon: "power"
            Layout.fillWidth: true
            visible: app.session.loggedIn
            onClicked: logoutDialog.open()
        }

        PrimaryButton {
            text: (app.i18n.lang, app.i18n.text("profile.signOut"))
            variant: "secondary"
            leadingIcon: "person"
            Layout.fillWidth: true
            visible: app.session.loggedIn
            onClicked: signOutDialog.open()
        }
    }

    property var logoutDialog: AppDialog {
        title: (app.i18n.lang, app.i18n.text("profile.logoutTitle"))
        message: (app.i18n.lang, app.i18n.text("profile.logoutMessage"))
        confirmText: (app.i18n.lang, app.i18n.text("profile.logout"))
        cancelText: (app.i18n.lang, app.i18n.text("action.cancel"))
        showCancel: true
        destructive: true
        onAccepted: app.session.logout()
    }

    property var signOutDialog: AppDialog {
        title: (app.i18n.lang, app.i18n.text("profile.signOutTitle"))
        message: (app.i18n.lang, app.i18n.text("profile.signOutMessage"))
        confirmText: (app.i18n.lang, app.i18n.text("profile.signOut"))
        cancelText: (app.i18n.lang, app.i18n.text("action.cancel"))
        showCancel: true
        destructive: true
        onAccepted: app.session.signOutLocal()
    }
}

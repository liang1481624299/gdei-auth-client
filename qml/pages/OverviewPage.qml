import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 首页 / Home —— 顶部状态胶囊 + 环形表盘主操作，下方依次为网络明细（默认全展开）、
// 本地认证配置与连通异常提示。大圆钮切换连接 / 断开；在线点按走二次确认。
// Home: status pill + circular dial, then full network details, local profiles and warning.
PageScaffold {
    id: root
    Theme { id: th }

    readonly property bool genuinelyOffline: !app.auth.online && app.auth.state !== 2
                                            && !app.session.busy
    readonly property bool showConnectivityWarning: genuinelyOffline || !app.network.available

    // 大圆钮状态：在线绿、离线主色。
    readonly property bool powerOnline: app.auth.online
    readonly property color powerRing: powerOnline ? th.success : th.primary

    title: "home.title"
    subtitle: "overview.subtitle"
    showBack: false
    contentMaxWidth: th.dp(960)

    Component.onCompleted: app.network.refresh()
    onVisibleChanged: if (visible) app.network.refresh()

    // 大圆钮切换：在线 → 断开（二次确认）；离线 → 未登录先弹登录，已登录直接连接。
    function activatePower() {
        if (app.auth.online) {
            logoutDialog.open();
        } else if (!app.session.loggedIn) {
            app.session.promptLogin();
        } else {
            app.session.login();
        }
    }

    // ── 顶部状态胶囊：在线绿 / 离线红，对标参照图的 Protected 胶囊 ──
    RowLayout {
        Layout.fillWidth: true
        spacing: th.sp4

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitHeight: th.dp(34)
            implicitWidth: pillRow.implicitWidth + th.sp6 * 2
            radius: th.radiusPill
            color: th.fill(root.powerOnline ? th.success : th.danger, th.isDark ? 0.16 : 0.10)
            border.width: 1
            border.color: th.fill(root.powerOnline ? th.success : th.danger, 0.30)

            Row {
                id: pillRow
                anchors.centerIn: parent
                spacing: th.sp3
                Rectangle {
                    width: th.dp(8)
                    height: width
                    radius: width / 2
                    color: root.powerOnline ? th.success : th.danger
                    anchors.verticalCenter: parent.verticalCenter
                }
                QQC.Label {
                    text: root.localized(root.powerOnline ? "home.online" : "home.offline")
                    color: root.powerOnline ? th.success : th.danger
                    font.pixelSize: th.fontBody
                    font.weight: th.weightSemibold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // ── 环形表盘主操作：外环轨道 + 装饰彩点，内圆大圆钮切换连接 / 断开 ──
    ColumnLayout {
        Layout.fillWidth: true
        spacing: th.sp6

        Item {
            id: dial
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: th.dp(216)
            Layout.preferredHeight: th.dp(216)

            // 外环轨道
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "transparent"
                border.width: th.dp(2)
                border.color: th.fill(root.powerRing, 0.35)
            }

            // 轨道装饰彩点（参照图轨道彩珠）
            Rectangle {
                width: th.dp(10); height: width; radius: width / 2
                color: th.success
                anchors.top: parent.top; anchors.topMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Rectangle {
                width: th.dp(10); height: width; radius: width / 2
                color: th.primary
                anchors.right: parent.right; anchors.rightMargin: 0
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                width: th.dp(10); height: width; radius: width / 2
                color: th.warning
                anchors.bottom: parent.bottom; anchors.bottomMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Rectangle {
                width: th.dp(10); height: width; radius: width / 2
                color: th.muted
                anchors.left: parent.left; anchors.leftMargin: 0
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: powerButton
                anchors.centerIn: parent
                width: th.dp(150)
                height: width
                radius: width / 2
                color: th.fill(root.powerRing, th.isDark ? 0.16 : 0.10)
                border.width: th.dp(2)
                border.color: root.powerRing
                opacity: app.session.busy ? 0.6 : 1.0
                scale: powerTap.pressed ? 0.96 : 1.0
                Behavior on scale { NumberAnimation { duration: th.durationFast; easing.type: Easing.OutCubic } }

                focusPolicy: Qt.StrongFocus
                Keys.onReturnPressed: root.activatePower()
                Keys.onSpacePressed: root.activatePower()
                Accessible.role: Accessible.Button
                Accessible.name: root.localized(root.powerOnline ? "home.disconnect" : "home.connect")
                Accessible.focusable: true

                Icon {
                    anchors.centerIn: parent
                    name: "power"
                    iconSize: th.dp(52)
                    tint: root.powerRing
                    visible: !app.session.busy
                }
                QQC.BusyIndicator {
                    anchors.centerIn: parent
                    running: app.session.busy
                    visible: app.session.busy
                    implicitWidth: th.dp(38)
                    implicitHeight: th.dp(38)
                }

                TapHandler {
                    id: powerTap
                    enabled: !app.session.busy
                    onTapped: root.activatePower()
                }
            }
        }

        QQC.Label {
            text: app.session.busy
                  ? root.localized(root.powerOnline ? "home.disconnecting" : "home.connecting")
                  : root.localized(root.powerOnline ? "home.disconnect" : "home.connect")
            color: root.powerRing
            font.pixelSize: th.fontCallout
            font.weight: th.weightSemibold
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
        }
    }

    // 本机链路明细（IP / 掩码 / MAC / 出口 / 诊断字段），默认全展开、不可折叠。
    NetworkInfoCard {
        Layout.fillWidth: true
    }

    // 本机认证配置（添加 / 选择 / 编辑 / 删除），自包含编辑器与删除确认弹窗。
    LocalProfilesCard {
        Layout.fillWidth: true
    }

    // ── 连通异常横幅（橙）──
    BaseCard {
        Layout.fillWidth: true
        visible: root.showConnectivityWarning
        backgroundColor: th.warningFill
        borderColor: th.fill(th.warning, 0.32)
        ColumnLayout {
            spacing: th.sp2; Layout.fillWidth: true
            QQC.Label {
                text: !app.network.available
                      ? root.localized("network.localUnavailableMessage")
                      : root.localized("network.offlineMessage")
                color: th.text; font.pixelSize: th.fontBody; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
            readonly property bool showNicHint: app.network.available && root.genuinelyOffline
            QQC.Label { visible: parent.showNicHint; text: root.localized("troubleshoot.ax201Title")
                        color: th.warning; font.pixelSize: th.fontFootnote
                        font.weight: th.weightSemibold; wrapMode: Text.Wrap; Layout.fillWidth: true }
            QQC.Label { visible: parent.showNicHint; text: root.localized("troubleshoot.ax201Message")
                        color: th.secondaryLabel; font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap; Layout.fillWidth: true }
        }
    }

    // 断开二次确认：退出校园网会话，保留本地配置。
    AppDialog {
        id: logoutDialog
        title: root.localized("home.disconnectTitle")
        message: root.localized("profile.logoutMessage")
        confirmText: root.localized("home.disconnect")
        cancelText: root.localized("action.cancel")
        showCancel: true
        destructive: true
        onAccepted: app.session.logout()
    }
}

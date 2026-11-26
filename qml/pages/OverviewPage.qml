import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 首页 / Home —— 一屏查看网卡、会话、外网探测三态连接情况，大圆钮切换连接 / 断开，
// 下方为链路明细、本机认证配置与连通异常提示。
// 三态绝对隔离（规范）：网卡物理层 / 账号会话态 / 外网连通态，不合并为单一指示灯。
// Home: three isolated layers, a big toggle power button, link details and local profiles.
PageScaffold {
    id: root
    Theme { id: th }

    // ── 三层状态独立计算，互不混算 ──
    readonly property bool nicUp: app.network.available
    readonly property bool signedIn: app.session.loggedIn
    readonly property bool sessionSuspect: signedIn && !app.auth.online && !app.session.busy
    readonly property int probeState: app.auth.state

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

    // ── 三层状态卡片：网卡 / 会话 / 探测（纵向列表，复用通用卡片组件）──
    // 网卡层：只反映本机是否存在可用链路，与认证结果无关。
    StatusListCard {
        id: nicCard
        Layout.fillWidth: true
        readonly property color sTint: root.nicUp ? th.success : th.danger
        readonly property string sKey: root.nicUp ? "layer.nicUp" : "layer.nicDown"
        iconSource: root.nicUp ? "wifi" : "wifiOff"
        title: root.localized("layer.nic")
        mainText: root.localized(sKey)
        descText: root.localized("layer.nicDesc")
        tagTint: sTint
    }

    // 会话层：账号是否在门户侧处于已登录态。
    StatusListCard {
        id: sessionCard
        Layout.fillWidth: true
        readonly property color sTint: root.sessionSuspect ? th.warning : root.signedIn ? th.success : th.muted
        readonly property string sKey: root.sessionSuspect ? "layer.sessionSuspect"
                                     : root.signedIn ? "layer.sessionIn" : "layer.sessionOut"
        iconSource: root.sessionSuspect ? "shield" : root.signedIn ? "check" : "person"
        title: root.localized("layer.session")
        mainText: root.localized(sKey)
        descText: root.localized("layer.sessionDesc")
        tagTint: sTint
    }

    // 探测层：短超时外网探测结果；探测中用主色，在线成功绿，离线/未知灰红。
    StatusListCard {
        id: probeCard
        Layout.fillWidth: true
        readonly property bool pBusy: root.probeState === 2
        readonly property bool pOnline: root.probeState === 3
        readonly property bool pUnknown: root.probeState === 0
        readonly property color pTint: pBusy ? th.info : pOnline ? th.success : pUnknown ? th.muted : th.danger
        readonly property string pKey: pBusy ? "layer.probeBusy" : pOnline ? "layer.probeOnline"
                                    : pUnknown ? "layer.probeUnknown" : "layer.probeOffline"
        iconSource: pBusy ? "refresh" : pOnline ? "wifi" : "wifiOff"
        title: root.localized("layer.probe")
        mainText: root.localized(pKey)
        descText: root.localized("layer.probeDesc")
        tagTint: pTint
    }

    // ── 大圆主操作：切换式，离线点按连接，在线点按断开（断开走二次确认）──
    ColumnLayout {
        Layout.fillWidth: true
        spacing: th.sp4

        Rectangle {
            id: powerButton
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: th.dp(160)
            Layout.preferredHeight: th.dp(160)
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
                iconSize: th.dp(56)
                tint: root.powerRing
                visible: !app.session.busy
            }
            QQC.BusyIndicator {
                anchors.centerIn: parent
                running: app.session.busy
                visible: app.session.busy
                implicitWidth: th.dp(40)
                implicitHeight: th.dp(40)
            }

            TapHandler {
                id: powerTap
                enabled: !app.session.busy
                onTapped: root.activatePower()
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

    // 本机链路明细（IP / 掩码 / MAC / 出口信息），默认折叠；样式与上方三态卡对齐。
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

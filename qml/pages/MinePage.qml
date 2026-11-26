import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 我的 / Mine —— 账号资料 + 偏好 + 系统工具（参考「我」页头像头部与分组列表编排）。
// 业务资料与设置同属账号侧；运行记录与诊断工具在「发现」页，不在此混放。
// Mine: account profile, preferences and system tools; diagnostics and logs live in Discover.
PageScaffold {
    id: root
    Theme { id: th }

    signal openAdvancedRequested()
    signal openAboutRequested()
    signal openEventsRequested()
    signal openLogRequested()

    title: "nav.mine"
    subtitle: "mine.subtitle"
    showBack: false
    contentMaxWidth: th.dp(720)

    function localized(key) { return app.i18n.localize(key); }
    function pickValue(keys) {
        var p = root.profile;
        for (var i = 0; i < keys.length; i++) {
            var v = p[keys[i]];
            if (v !== undefined && v !== null && String(v).length > 0) return String(v);
        }
        return "";
    }

    // ── 账号状态（仅反映自助门户会话，与网卡 / 探测层无关）──
    readonly property int accountState: !app.session.loggedIn ? 0
                                        : app.account.authenticated ? 1 : 2
    readonly property string stateKey: accountState === 1 ? "account.statusNormal"
                                      : accountState === 2 ? "account.statusExpired"
                                      : "account.statusUnknown"
    readonly property color stateTint: accountState === 1 ? th.success
                                      : accountState === 2 ? th.warning : th.muted
    readonly property bool sessionExpired: app.session.loggedIn && !app.account.authenticated

    readonly property var profile: app.account.profile
    readonly property string balanceValue: root.pickValue(["balance", "remaining", "credit"])
    readonly property string planValue: root.pickValue(["plan", "traffic", "dataQuota"])
    readonly property string durationValue: root.pickValue(["onlineDuration", "duration", "sessionTime"])

    readonly property bool balanceFailed: !app.account.busy && root.balanceValue.length === 0
    readonly property bool profileUnavailable: app.account.authenticated
        && !app.account.busy
        && root.balanceValue.length === 0 && root.planValue.length === 0

    property bool showAccountLogin: false
    property string accountCode: ""

    function goLogin() {
        if (!app.session.loggedIn) { app.session.promptLogin(); return; }
        if (!root.showAccountLogin) {
            root.showAccountLogin = true;
            root.accountCode = "";
            app.account.fetchCaptcha();
        }
    }

    Component.onCompleted: {
        if (app.session.loggedIn && app.account.authenticated) app.account.refreshProfile();
    }
    onVisibleChanged: {
        if (visible && app.session.loggedIn && app.account.authenticated && !app.account.busy)
            app.account.refreshProfile();
    }

    Connections {
        target: app.account
        function onAuthenticatedChanged() {
            if (app.account.authenticated) {
                root.showAccountLogin = false;
                root.accountCode = "";
                app.account.refreshProfile();
            }
        }
    }

    // ── 账号头部：头像 + 昵称 + 状态徽章（点击进入门户登录 / 资料）──
    BaseCard {
        Layout.fillWidth: true
        TapHandler { onTapped: root.goLogin() }
        Accessible.role: Accessible.Button
        Accessible.name: app.config.username.length > 0
                          ? app.config.username : root.localized("account.notSignedIn")
        Accessible.focusable: false

        RowLayout {
            spacing: th.sp6
            Layout.fillWidth: true

            AvatarBlock {
                name: app.config.username
                size: th.dp(56)
                Layout.alignment: Qt.AlignVCenter
            }

            ColumnLayout {
                spacing: th.sp2
                Layout.fillWidth: true

                QQC.Label {
                    text: app.config.username.length > 0 ? app.config.username
                          : root.localized("account.notSignedIn")
                    color: th.textPrimary
                    font.pixelSize: th.fontTitle3
                    font.weight: th.weightSemibold
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
                BaseBadge {
                    text: root.localized(root.stateKey)
                    tint: root.stateTint
                    iconName: root.accountState === 1 ? "check" : root.accountState === 2 ? "shield" : "person"
                }
            }

            Icon {
                name: "chevronRight"
                iconSize: th.dp(18)
                tint: th.muted
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // 本次在线时长
        KeyValueRow {
            label: root.localized("account.onlineDuration")
            value: root.durationValue
            chevron: false
            Layout.fillWidth: true
        }

        // 未登录 / 会话失效引导
        ColumnLayout {
            spacing: th.sp4
            Layout.fillWidth: true
            visible: !app.session.loggedIn || root.sessionExpired

            QQC.Label {
                text: root.sessionExpired ? root.localized("account.statusExpired")
                      : root.localized("account.notSignedIn")
                color: root.sessionExpired ? th.warning : th.secondaryLabel
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            BaseButton {
                text: root.localized("account.goLogin")
                type: "primary"
                leadingIcon: "power"
                Layout.fillWidth: true
                onClicked: root.goLogin()
            }
        }
    }

    // ── 门户验证码登录（内联，会话失效时展开）──
    BaseCard {
        Layout.fillWidth: true
        visible: root.showAccountLogin
        title: root.localized("account.portalLoginTitle")

        RowLayout {
            spacing: th.sp2
            Layout.fillWidth: true
            Icon { name: "shield"; iconSize: th.dp(14); tint: th.warning; Layout.alignment: Qt.AlignTop }
            QQC.Label {
                text: root.localized("portal.plaintextWarning")
                color: th.secondaryLabel
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            // 验证码：独占一行宽幅展示，避免与输入框并排被压成窄条；点击图片刷新。
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: th.dp(72)
                radius: th.radiusControl
                color: th.captchaSurface
                border.width: 1
                border.color: th.border
                clip: true

                Image {
                    anchors.centerIn: parent
                    width: parent.width - th.sp6
                    height: parent.height - th.sp6
                    fillMode: Image.PreserveAspectFit
                    source: app.account.captchaImage
                    smooth: true
                    asynchronous: true
                }
                TapHandler { onTapped: app.account.fetchCaptcha() }
            }

            BaseTextField {
                Layout.fillWidth: true
                label: root.localized("devices.code")
                placeholderText: root.localized("devices.code")
                text: root.accountCode
                onEdited: function(t) { root.accountCode = t; }
                onAccepted: if (root.accountCode.trim().length > 0) app.account.loginWithCode(root.accountCode)
            }
        }

        RowLayout {
            spacing: th.sp4
            Layout.fillWidth: true
            BaseButton {
                text: root.localized("account.refresh"); type: "secondary"; leadingIcon: "refresh"
                enabled: !app.account.busy
                onClicked: app.account.fetchCaptcha()
            }
            Item { Layout.fillWidth: true }
            BaseButton {
                text: root.localized("action.cancel"); type: "ghost"
                onClicked: { root.showAccountLogin = false; root.accountCode = ""; }
            }
        }
        // 主操作通栏置底，避免窄屏与次要按钮挤在一行。
        BaseButton {
            text: root.localized("devices.login"); type: "primary"; leadingIcon: "power"
            busy: app.account.busy
            enabled: root.accountCode.trim().length > 0 && !app.account.busy
            Layout.fillWidth: true
            onClicked: { app.account.loginWithCode(root.accountCode); }
        }

        QQC.Label {
            text: app.account.lastMessage
            color: th.muted
            font.pixelSize: th.fontFootnote
            wrapMode: Text.Wrap
            visible: text.length > 0
            Layout.fillWidth: true
        }
    }

    // ── KPI：余额 + 套餐流量 ──
    GridLayout {
        Layout.fillWidth: true
        visible: app.account.authenticated && !root.profileUnavailable
        columns: width < th.compactBreakpoint ? 1 : 2
        columnSpacing: th.sp8
        rowSpacing: th.sp8

        BaseCard {
            Layout.fillWidth: true
            title: root.localized("profile.balance")
            actions: Component {
                BaseButton {
                    text: root.localized("account.refresh")
                    type: "secondary"
                    leadingIcon: "refresh"
                    busy: app.account.busy
                    onClicked: app.account.refreshProfile()
                }
            }

            BaseSkeleton {
                visible: app.account.busy
                Layout.fillWidth: true
                height: th.dp(40)
            }

            QQC.Label {
                visible: !app.account.busy && root.balanceValue.length > 0
                text: root.balanceValue
                color: th.textPrimary
                font.pixelSize: root.width < th.narrowBreakpoint ? th.dp(28) : th.fontDisplay
                font.weight: th.weightBold
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }

            ColumnLayout {
                visible: !app.account.busy && root.balanceFailed
                spacing: th.sp2
                Layout.fillWidth: true
                QQC.Label {
                    text: root.localized("account.balanceFailed")
                    color: th.secondaryLabel
                    font.pixelSize: th.fontBody
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: app.account.lastMessage
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                    visible: text.length > 0
                    Layout.fillWidth: true
                }
                BaseButton {
                    text: root.localized("account.refresh")
                    type: "secondary"
                    leadingIcon: "refresh"
                    busy: app.account.busy
                    Layout.alignment: Qt.AlignLeft
                    onClicked: app.account.refreshProfile()
                }
            }
        }

        BaseCard {
            Layout.fillWidth: true
            title: root.localized("account.planTraffic")

            QQC.Label {
                visible: root.planValue.length > 0
                text: root.planValue
                color: th.textPrimary
                font.pixelSize: root.width < th.narrowBreakpoint ? th.dp(28) : th.fontDisplay
                font.weight: th.weightBold
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }
            ColumnLayout {
                visible: root.planValue.length === 0
                spacing: th.sp2
                Layout.fillWidth: true
                QQC.Label {
                    text: "—"
                    color: th.tertiaryLabel
                    font.pixelSize: th.fontDisplay
                    font.weight: th.weightBold
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: root.localized("account.planHint")
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }
    }

    // ── 门户无资料字段时的中性说明 ──
    BaseCard {
        Layout.fillWidth: true
        visible: root.profileUnavailable

        RowLayout {
            spacing: th.sp4
            Layout.fillWidth: true
            Icon { name: "info"; iconSize: th.dp(20); tint: th.muted; Layout.alignment: Qt.AlignTop }
            ColumnLayout {
                spacing: th.sp2
                Layout.fillWidth: true
                QQC.Label {
                    text: root.localized("account.profileUnavailable")
                    color: th.textPrimary
                    font.pixelSize: th.fontBody
                    font.weight: th.weightMedium
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: root.localized("account.profileUnavailableHint")
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }
    }

    // ── 基础偏好 ──
    BaseCard {
        Layout.fillWidth: true
        title: root.localized("settings.prefGroup")

        SettingRow {
            Layout.fillWidth: true
            title: root.localized("lang.title")
            description: root.localized("lang.hint")
            leadingIcon: "globe"
            LangSelector { }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: th.border }
        SettingRow {
            Layout.fillWidth: true
            title: root.localized("theme.title")
            description: root.localized("theme.hint")
            leadingIcon: "sun"
            ThemeSelector { }
        }
    }

    // ── 系统工具 ──
    BaseCard {
        Layout.fillWidth: true
        title: root.localized("settings.systemGroup")

        SettingRow {
            Layout.fillWidth: true
            title: root.localized("keepalive.switchLabel")
            description: root.localized("keepalive.switchHint")
            leadingIcon: "refresh"
            iconContainer: true
            BaseSwitch {
                checked: app.auth.keepAliveRunning
                onToggled: function(checked) {
                    if (checked) {
                        if (!app.session.loggedIn && !app.auth.online) {
                            app.session.promptLogin(); this.checked = false;
                        } else { app.auth.startKeepAlive(); }
                    } else { app.auth.stopKeepAlive(); }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: th.border }
        NavEntryRow {
            title: root.localized("section.advanced")
            iconName: "slider"
            onClicked: root.openAdvancedRequested()
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: th.border }
        NavEntryRow {
            title: root.localized("about.title")
            iconName: "info"
            onClicked: root.openAboutRequested()
        }
    }

    // ── 运行记录：事件流与系统日志（自发现页迁入）──
    BaseCard {
        Layout.fillWidth: true
        title: root.localized("discover.logGroup")

        NavEntryRow {
            title: root.localized("events.title")
            iconName: "bell"
            badge: true
            onClicked: root.openEventsRequested()
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: th.border }
        NavEntryRow {
            title: root.localized("log.title")
            iconName: "list"
            onClicked: root.openLogRequested()
        }
    }

    // ── 注销本地配置：删除本地账号凭证，次要按钮 + 强确认 ──
    BaseCard {
        Layout.fillWidth: true
        title: root.localized("account.dangerZone")

        ColumnLayout {
            spacing: th.sp2
            Layout.fillWidth: true
            RowLayout {
                spacing: th.sp2
                Layout.fillWidth: true
                Icon { name: "shield"; iconSize: th.dp(14); tint: th.danger }
                QQC.Label {
                    text: root.localized("profile.signOutMessage")
                    color: th.secondaryLabel
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
            BaseButton {
                text: root.localized("profile.signOut")
                type: "danger-ghost"
                leadingIcon: "trash"
                Layout.fillWidth: true
                onClicked: signOutDialog.open()
            }
        }
    }

    AppDialog {
        id: signOutDialog
        title: root.localized("profile.signOutTitle")
        message: root.localized("profile.signOutMessage")
        confirmText: root.localized("profile.signOut")
        cancelText: root.localized("action.cancel")
        showCancel: true
        destructive: true
        onAccepted: app.session.signOutLocal()
    }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root
    Theme { id: th }

    // 跳转到网络诊断页 / Navigate to the diagnostics page.
    signal openDiagnosticsRequested()

    // 三层状态独立计算，互不混算：网卡层（本机链路）/ 会话层（账号登录）/ 探测层（外网可达）。
    // Three independent layers: local adapter, account session, external reachability.
    readonly property bool nicUp: app.network.available
    readonly property bool signedIn: app.session.loggedIn
    readonly property bool sessionSuspect: signedIn && !app.auth.online && !app.session.busy
    readonly property int probeState: app.auth.state

    readonly property bool genuinelyOffline: !app.auth.online && app.auth.state !== 2
                                            && !app.session.busy
    readonly property bool showConnectivityWarning: genuinelyOffline || !app.network.available

    title: "dashboard.title"
    subtitle: "dashboard.subtitle"
    showBack: false
    contentMaxWidth: th.dp(480)

    Component.onCompleted: app.network.refresh()
    onVisibleChanged: if (visible) app.network.refresh()

    // 三层状态卡：网卡 / 会话 / 探测，各自独立渲染状态色与文案。
    Card {
        Layout.fillWidth: true

        ColumnLayout {
            width: parent.width
            spacing: th.sp4

            // 网卡层：只反映本机是否存在可用链路，与认证结果无关。
            RowLayout {
                width: parent.width
                spacing: th.sp4

                Icon {
                    name: root.nicUp ? "wifi" : "wifiOff"
                    iconSize: 18
                    tint: root.nicUp ? th.success : th.danger
                    Layout.alignment: Qt.AlignVCenter
                }
                QQC.Label {
                    text: root.localized("layer.nic")
                    color: th.secondaryLabel
                    font.pixelSize: th.fontSubhead
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
                StatusBadge {
                    text: root.localized(root.nicUp ? "layer.nicUp" : "layer.nicDown")
                    tint: root.nicUp ? th.success : th.danger
                    iconName: root.nicUp ? "wifi" : "wifiOff"
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            // 会话层：账号是否在门户侧处于已登录态。
            RowLayout {
                width: parent.width
                spacing: th.sp4

                Icon {
                    name: "person"
                    iconSize: 18
                    tint: root.sessionSuspect ? th.warning : root.signedIn ? th.success : th.muted
                    Layout.alignment: Qt.AlignVCenter
                }
                QQC.Label {
                    text: root.localized("layer.session")
                    color: th.secondaryLabel
                    font.pixelSize: th.fontSubhead
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
                StatusBadge {
                    text: root.localized(root.sessionSuspect ? "layer.sessionSuspect"
                                         : root.signedIn ? "layer.sessionIn" : "layer.sessionOut")
                    tint: root.sessionSuspect ? th.warning
                          : root.signedIn ? th.success : th.muted
                    iconName: root.sessionSuspect ? "shield" : root.signedIn ? "check" : "person"
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            // 探测层：短超时外网探测结果；探测中用信息蓝，离线/错误用红。
            RowLayout {
                width: parent.width
                spacing: th.sp4

                Icon {
                    name: root.probeState === 2 ? "refresh"
                          : root.probeState === 3 ? "wifi" : "wifiOff"
                    iconSize: 18
                    tint: root.probeState === 2 ? th.info
                          : root.probeState === 3 ? th.success : root.probeState === 0 ? th.muted : th.danger
                    Layout.alignment: Qt.AlignVCenter
                }
                QQC.Label {
                    text: root.localized("layer.probe")
                    color: th.secondaryLabel
                    font.pixelSize: th.fontSubhead
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
                StatusBadge {
                    text: root.localized(root.probeState === 2 ? "layer.probeBusy"
                                         : root.probeState === 3 ? "layer.probeOnline"
                                         : root.probeState === 0 ? "layer.probeUnknown"
                                         : "layer.probeOffline")
                    tint: root.probeState === 2 ? th.info
                          : root.probeState === 3 ? th.success
                          : root.probeState === 0 ? th.muted : th.danger
                    iconName: root.probeState === 2 ? "refresh"
                              : root.probeState === 3 ? "wifi" : "wifiOff"
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }

    // 保活总开关：只控制后台轮询与自动重试，不发送登出请求。
    // Keep-alive switch: polls and retries in the background only; it never signs out.
    Card {
        Layout.fillWidth: true

        RowLayout {
            width: parent.width
            spacing: th.sp4

            ColumnLayout {
                spacing: th.sp2
                Layout.fillWidth: true

                QQC.Label {
                    text: root.localized("keepalive.switchLabel")
                    color: th.text
                    font.pixelSize: th.fontCallout
                    font.weight: th.weightSemibold
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: root.localized("keepalive.switchHint")
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
            }

            Switch {
                checked: app.auth.keepAliveRunning
                Layout.alignment: Qt.AlignVCenter
                onToggled: function(checked) {
                    if (checked) {
                        if (!app.session.loggedIn && !app.auth.online) {
                            // 未登录且离线：先弹登录层，认证成功后再保活；开关复位等待登录。
                            // Offline and signed out: prompt sign-in first; revert the switch.
                            app.session.promptLogin();
                            this.checked = false;
                        } else {
                            app.auth.startKeepAlive();
                        }
                    } else {
                        app.auth.stopKeepAlive();
                    }
                }
            }
        }
    }

    // 上线 / 重连主操作。
    PrimaryButton {
        text: (app.i18n.lang, app.i18n.text("btn.login"))
        leadingIcon: "power"
        Layout.fillWidth: true
        visible: !app.auth.online
        busy: app.session.busy
        onClicked: {
            // 未登录先弹 portal 登录层；已登录但离线则直接重连。
            // Signed out: prompt the portal login sheet; otherwise reconnect directly.
            if (!app.session.loggedIn) app.session.promptLogin();
            else app.session.login();
        }
    }

    // 诊断入口：快速连通测试（不跳页）与进入网络诊断。
    Card {
        Layout.fillWidth: true

        ColumnLayout {
            width: parent.width
            spacing: th.sp4

            PrimaryButton {
                width: parent.width
                text: (app.i18n.lang, app.i18n.text("dashboard.quickTest"))
                variant: "secondary"
                leadingIcon: "refresh"
                onClicked: {
                    app.auth.refreshStatus();
                    app.network.refresh();
                }
            }
            PrimaryButton {
                width: parent.width
                text: (app.i18n.lang, app.i18n.text("dashboard.openDiagnostics"))
                variant: "secondary"
                leadingIcon: "list"
                onClicked: root.openDiagnosticsRequested()
            }
        }
    }

    // 最近一条告警 / 断网事件；未读时显示红点。
    Card {
        Layout.fillWidth: true

        ColumnLayout {
            width: parent.width
            spacing: th.sp4

            RowLayout {
                width: parent.width
                spacing: th.sp4

                QQC.Label {
                    text: root.localized("dashboard.recentAlert")
                    color: th.text
                    font.pixelSize: th.fontHeadline
                    font.weight: th.weightSemibold
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
                UnreadBadge {
                    visible: app.events.unreadCount > 0
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            Item {
                width: parent.width
                implicitHeight: alertList.count > 0 ? alertList.contentHeight : noAlertRow.implicitHeight

                TapHandler {
                    onTapped: root.openDiagnosticsRequested()
                }

                ListView {
                    id: alertList
                    width: parent.width
                    height: Math.min(contentHeight, parent.implicitHeight)
                    clip: true
                    interactive: false
                    boundsBehavior: Flickable.StopAtBounds
                    model: app.eventModel

                    delegate: EventListItem {
                        required property var model
                        width: alertList.width
                        timestamp: model.timestamp
                        level: model.level
                        titleKey: model.titleKey
                        messageKey: model.messageKey
                        reasonKey: model.reasonKey
                        source: model.source
                        arguments: model.arguments
                        unread: model.unread
                    }
                }

                RowLayout {
                    id: noAlertRow
                    width: parent.width
                    visible: alertList.count === 0

                    QQC.Label {
                        text: root.localized("dashboard.noAlert")
                        color: th.muted
                        font.pixelSize: th.fontSubhead
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

    Card {
        id: warnCard
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

        // 有本地链路但校园网离线/半连通时，附网卡 Offload 兼容自查指引；
        // 本地网卡未连接时不展示（关闭卸载项无从谈起）。
        readonly property bool showNicHint: app.network.available && root.genuinelyOffline

        QQC.Label {
            width: parent.width
            visible: warnCard.showNicHint
            text: (app.i18n.lang, app.i18n.text("troubleshoot.ax201Title"))
            color: th.warning
            font.pixelSize: th.fontFootnote
            font.weight: th.weightSemibold
            wrapMode: Text.Wrap
        }
        QQC.Label {
            width: parent.width
            visible: warnCard.showNicHint
            text: (app.i18n.lang, app.i18n.text("troubleshoot.ax201Message"))
            color: th.secondaryLabel
            font.pixelSize: th.fontFootnote
            wrapMode: Text.Wrap
        }
    }

    NetworkInfoCard {
        Layout.fillWidth: true
    }
}

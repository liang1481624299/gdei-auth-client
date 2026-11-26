import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// NetworkInfoCard —— 本机链路明细卡。默认全展开、不提供折叠：
// 本机局域网 / 校园出站网卡 / 认证出口 / 诊断字段（接口、网关、子网比对）全部平铺展示。
// Full-width network details card; always expanded, no collapse toggle.
BaseCard {
    id: root
    readonly property Theme th: Theme {}

    cornerRadius: th.radiusLarge

    readonly property color headTint: app.network.available ? th.primary : th.muted

    readonly property bool compact: width < 520
    readonly property int routeStatus: app.network.routeStatus
    readonly property int subnetStatus: app.network.subnetStatus
    readonly property string subnetKey: subnetStatus === 1 ? "network.subnetSame"
                                        : subnetStatus === 2 ? "network.subnetDifferent"
                                        : "network.subnetUnknown"
    // 跨路由器 / NAT 时本机私网与备用公网 IP 本就不在同一子网，属常态，用中性信息色，
    // 不作橙色告警；该指标仅在直连同二层时为“同一子网”。
    readonly property color subnetColor: subnetStatus === 1 ? th.success : th.muted

    function localized(key) {
        return app.i18n.dictionary[key];
    }

    function valueOrUnavailable(v) {
        if (v !== undefined && v !== null && String(v).length > 0)
            return String(v);
        return localized("common.unavailable");
    }

    // 对端 MAC：NAT 路由环境下取不到，使用场景化文案，不直出“未提供”。
    function targetMacDisplay() {
        if (app.network.targetMac && String(app.network.targetMac).length > 0)
            return String(app.network.targetMac);
        return localized("network.natMacUnavailable");
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: th.sp6

        // 标题头：纯展示，不可折叠、无展开箭头。
        RowLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: th.dp(48)
                Layout.preferredHeight: th.dp(48)
                Layout.alignment: Qt.AlignVCenter
                radius: th.radiusControl
                color: th.fill(root.headTint, th.isDark ? 0.16 : 0.10)

                Icon {
                    anchors.centerIn: parent
                    name: app.network.available ? "wifi" : "wifiOff"
                    iconSize: th.dp(22)
                    tint: root.headTint
                }
            }
            QQC.Label {
                text: root.localized("network.title")
                color: th.textPrimary
                font.pixelSize: th.fontHeadline
                font.weight: th.weightSemibold
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        // 操作按钮：刷新网卡信息与连通测试为两个独立按钮，互不共用。
        RowLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            BaseButton {
                text: root.localized("network.refreshNic")
                type: "secondary"
                leadingIcon: "refresh"
                Layout.fillWidth: true
                onClicked: app.network.refresh()
            }
            BaseButton {
                text: root.localized("network.probeConnectivity")
                type: "secondary"
                leadingIcon: "bolt"
                Layout.fillWidth: true
                onClicked: app.auth.refreshStatus()
            }
        }

        // 本机局域网信息：网卡 IP / 掩码 / 本机 MAC，属内网地址，不代表校园认证地址。
        ColumnLayout {
            spacing: th.sp3
            Layout.fillWidth: true

            QQC.Label {
                text: root.localized("network.lanGroup")
                color: th.text
                font.pixelSize: th.fontCallout
                font.weight: th.weightSemibold
                Layout.fillWidth: true
            }
            QQC.Label {
                text: root.localized("network.lanHint")
                color: th.muted
                font.pixelSize: th.fontCaption
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }

            GridLayout {
                columns: root.compact ? 1 : 2
                columnSpacing: th.sp8
                rowSpacing: th.sp5
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.sourceIp")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.valueOrUnavailable(app.network.sourceIp)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.netmask")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.valueOrUnavailable(app.network.netmask)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.localMac")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.valueOrUnavailable(app.network.localMac)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // 校园网卡：识别层按门户可达性探测选出的出站网卡；与默认出站不一致时告警。
        // Campus NIC: egress adapter picked by portal reachability probe; warn on mismatch.
        ColumnLayout {
            visible: app.network.campusSourceIp
                     && String(app.network.campusSourceIp).length > 0
            spacing: th.sp3
            Layout.fillWidth: true

            QQC.Label {
                text: root.localized("network.campusNic")
                color: th.text
                font.pixelSize: th.fontCallout
                font.weight: th.weightSemibold
                Layout.fillWidth: true
            }
            QQC.Label {
                text: root.localized("network.campusHint")
                color: th.muted
                font.pixelSize: th.fontCaption
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }

            GridLayout {
                columns: root.compact ? 1 : 2
                columnSpacing: th.sp8
                rowSpacing: th.sp5
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.sourceIp")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.valueOrUnavailable(app.network.campusSourceIp)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.interfaceName")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.valueOrUnavailable(
                            app.network.campusSelected.humanName
                            || app.network.campusInterfaceName)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
            }

            RowLayout {
                spacing: th.sp4
                Layout.fillWidth: true

                BaseBadge {
                    text: root.localized(app.network.campusReachable
                        ? "network.campusReachable" : "network.campusUnreachable")
                    iconName: app.network.campusReachable ? "check" : "info"
                    tint: app.network.campusReachable ? th.success : th.danger
                }
            }

            // 层 B：主机路由强制状态；Idle 时不展示。
            RowLayout {
                visible: root.routeStatus !== 0
                spacing: th.sp4
                Layout.fillWidth: true

                BaseBadge {
                    text: root.localized(root.routeStatus === 1
                        ? "network.routeBound"
                        : root.routeStatus === 3
                            ? "network.routeUnsupported"
                            : "network.routeDegraded")
                    iconName: root.routeStatus === 1 ? "check" : "info"
                    tint: root.routeStatus === 1 ? th.success
                        : root.routeStatus === 3 ? th.muted : th.warning
                }
            }

            RowLayout {
                visible: app.network.campusMismatch
                spacing: th.sp4
                Layout.fillWidth: true

                BaseBadge {
                    text: root.localized("network.campusMismatch")
                    iconName: "info"
                    tint: th.warning
                    Layout.fillWidth: true
                }
            }
        }

        // 校园认证出口信息：认证后学校侧出口 IP / 对端 MAC。
        ColumnLayout {
            spacing: th.sp3
            Layout.fillWidth: true

            QQC.Label {
                text: root.localized("network.exitGroup")
                color: th.text
                font.pixelSize: th.fontCallout
                font.weight: th.weightSemibold
                Layout.fillWidth: true
            }
            QQC.Label {
                text: root.localized("network.exitHint")
                color: th.muted
                font.pixelSize: th.fontCaption
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }

            GridLayout {
                columns: root.compact ? 1 : 2
                columnSpacing: th.sp8
                rowSpacing: th.sp5
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.targetIp")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.valueOrUnavailable(app.network.targetIp)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.targetMac")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.targetMacDisplay()
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // 诊断字段：接口内核名、网关、子网比对，平铺展示，不再折叠。
        ColumnLayout {
            spacing: th.sp3
            Layout.fillWidth: true

            QQC.Label {
                text: root.localized("network.details")
                color: th.text
                font.pixelSize: th.fontCallout
                font.weight: th.weightSemibold
                Layout.fillWidth: true
            }

            GridLayout {
                columns: root.compact ? 1 : 2
                columnSpacing: th.sp8
                rowSpacing: th.sp5
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.interfaceName")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.valueOrUnavailable(app.network.interfaceName)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true
                    QQC.Label {
                        text: root.localized("network.gateway")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.localized("common.unavailable")
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
            }

            RowLayout {
                spacing: th.sp4
                Layout.fillWidth: true

                QQC.Label {
                    text: root.localized("network.subnet")
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                BaseBadge {
                    text: root.localized(root.subnetKey)
                    iconName: root.subnetStatus === 1 ? "check" : "info"
                    tint: root.subnetColor
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

BaseCard {
    id: root
    readonly property Theme th: Theme {}

    // 外框圆角与首页三态状态卡（StatusListCard）对齐。
    cornerRadius: th.radiusLarge

    // 网络信息默认折叠；点击标题头展开操作按钮与本机/出口明细。
    // Collapsed by default; tap the header to reveal actions and NIC/exit details.
    property bool expanded: false

    readonly property color headTint: app.network.available ? th.primary : th.muted

    readonly property bool compact: width < 520
    readonly property int subnetStatus: app.network.subnetStatus
    readonly property string subnetKey: subnetStatus === 1 ? "network.subnetSame"
                                        : subnetStatus === 2 ? "network.subnetDifferent"
                                        : "network.subnetUnknown"
    // 跨路由器 / NAT 时本机私网与备用公网 IP 本就不在同一子网，属常态，用中性信息色，
    // 不作橙色告警；该指标仅在直连同二层时为“同一子网”，且只出现在诊断详情中。
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

        // 标题头：整行可点击，切换展开/收起；右侧箭头随状态旋转。
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: th.dp(48)
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: th.sp4

                // 左侧图标方块：与三态状态卡一致的 48×48 圆角方块 + 低透明强调底色。
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
                Icon {
                    name: "chevronDown"
                    iconSize: th.dp(16)
                    tint: th.secondaryLabel
                    rotation: root.expanded ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: th.duration } }
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.expanded = !root.expanded
            }
        }

        // 折叠体：操作按钮与明细字段，收起时高度归零、淡出。
        Rectangle {
            Layout.fillWidth: true
            clip: true
            color: "transparent"
            Layout.preferredHeight: root.expanded
                ? Math.min(bodyCol.implicitHeight + th.sp6 * 2, 4096)
                : 0
            opacity: root.expanded ? 1 : 0
            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: th.duration; easing.type: Easing.OutCubic }
            }
            Behavior on opacity { NumberAnimation { duration: th.durationFast } }

            ColumnLayout {
                id: bodyCol
                anchors.fill: parent
                spacing: th.sp6

                // 标题行：刷新网卡信息与连通测试为两个独立按钮，互不共用。
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

                // 底层调试字段：接口内核名、网关、子网比对，折叠进详情，不上主面、不作告警。
                BaseCollapseGroup {
                    title: root.localized("network.details")
                    expanded: false
                    Layout.fillWidth: true

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
    }
}

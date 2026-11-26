import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// DeviceCard —— 设备卡片（参考 openp2p 设备台风格）。
// 顶部：品牌图标方块 + 设备名 / 副标题 + 右上状态点与徽章；
// 中部：两列键值明细（标签在上、取值在下，空值由调用方过滤）；
// 底部：可选操作槽（default alias）。云端只读设备不填操作槽。
// Device card: brand icon + name/subtitle + status dot, two-column key/value grid, optional action row.
Rectangle {
    id: root
    Theme { id: th }

    property string title: ""
    property string subtitle: ""
    // 传给 BrandIcon 做平台识别的文本（deviceType + os）；识别不到时回退 fallbackIcon。
    property string brandText: ""
    property string fallbackIcon: "server"
    // 右上状态点颜色；statusText 为空时只显示圆点。
    property color statusColor: th.muted
    property string statusText: ""
    property bool statusVisible: true
    property bool selected: false
    // 云端只读卡传 false，关闭悬停/可点反馈；本地配置卡保留交互态。
    property bool interactive: true
    // 明细项：[{ label, value }]，调用方应过滤掉 value 为空的项。
    property var fields: []

    default property alias actionSlot: actionRow.data
    implicitWidth: Math.max(headerRow.implicitWidth, fieldsGrid.implicitWidth,
                            actionRow.implicitWidth) + th.cardPadding * 2
    implicitHeight: bodyCol.implicitHeight + th.cardPadding * 2

    radius: th.radiusCard
    color: root.selected ? th.secondaryBg : th.card
    border.width: 1
    border.color: root.selected ? th.fill(th.primary, 0.30) : th.border

    HoverHandler { id: hoverHandler; enabled: root.interactive }

    ColumnLayout {
        id: bodyCol
        anchors.fill: parent
        anchors.margins: th.cardPadding
        spacing: th.sp6

        // ── 顶部：图标 + 名称 + 状态 ──
        RowLayout {
            id: headerRow
            Layout.fillWidth: true
            spacing: th.sp4

            Rectangle {
                Layout.preferredWidth: th.dp(44)
                Layout.preferredHeight: th.dp(44)
                Layout.alignment: Qt.AlignTop
                radius: th.radiusControl
                color: th.field
                border.width: 1
                border.color: th.border

                BrandIcon {
                    id: brandIcon
                    osText: root.brandText
                    iconSize: th.dp(22)
                    anchors.centerIn: parent
                }
                Icon {
                    anchors.centerIn: parent
                    name: root.fallbackIcon
                    iconSize: th.dp(22)
                    tint: th.secondaryLabel
                    visible: !brandIcon.hasBrand
                }
            }

            ColumnLayout {
                spacing: th.sp2
                Layout.fillWidth: true

                QQC.Label {
                    text: root.title.length > 0 ? root.title : root.localized("common.unavailable")
                    color: th.textPrimary
                    font.pixelSize: th.fontCallout
                    font.weight: th.weightSemibold
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: root.subtitle
                    visible: text.length > 0
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                spacing: th.sp2
                Layout.alignment: Qt.AlignTop
                visible: root.statusVisible

                Rectangle {
                    Layout.preferredWidth: th.dp(8)
                    Layout.preferredHeight: th.dp(8)
                    radius: th.radiusPill
                    color: root.statusColor
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.statusText.length === 0
                }
                BaseBadge {
                    text: root.statusText
                    tint: root.statusColor
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.statusText.length > 0
                }
            }
        }

        // ── 键值明细网格（窄屏单列，宽屏两列）──
        GridLayout {
            id: fieldsGrid
            Layout.fillWidth: true
            columns: root.width < th.narrowBreakpoint ? 1 : 2
            columnSpacing: th.sp6
            rowSpacing: th.sp3
            visible: root.fields.length > 0

            Repeater {
                model: root.fields
                delegate: ColumnLayout {
                    required property var modelData
                    spacing: th.sp1
                    Layout.fillWidth: true

                    QQC.Label {
                        text: modelData.label
                        color: th.secondaryLabel
                        font.pixelSize: th.fontFootnote
                        topPadding: 0
                        bottomPadding: 0
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (modelData.value && modelData.value.length > 0)
                              ? modelData.value : "—"
                        color: (modelData.value && modelData.value.length > 0)
                               ? th.text : th.tertiaryLabel
                        font.pixelSize: th.fontBody
                        font.weight: th.weightMedium
                        wrapMode: Text.WrapAnywhere
                        topPadding: 0
                        bottomPadding: 0
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // ── 底部操作槽（本地配置：选择 / 编辑 / 删除；云端留空）──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: th.sp4
            visible: actionRow.children.length > 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: th.border
            }
            RowLayout {
                id: actionRow
                Layout.fillWidth: true
                spacing: th.sp4
            }
        }
    }

    function localized(key) { return app.i18n.dictionary[key]; }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Card {
    id: root
    readonly property Theme th: Theme {}

    readonly property bool compact: width < 520
    readonly property int subnetStatus: app.network.subnetStatus
    readonly property string subnetKey: subnetStatus === 1 ? "network.subnetSame"
                                        : subnetStatus === 2 ? "network.subnetDifferent"
                                        : "network.subnetUnknown"
    readonly property color subnetColor: subnetStatus === 1 ? th.success
                                         : subnetStatus === 2 ? th.warning : th.muted
    readonly property var fields: [
        { key: "network.sourceIp", value: app.network.sourceIp, notProvided: false },
        { key: "network.targetIp", value: app.network.targetIp, notProvided: false },
        { key: "network.netmask", value: app.network.netmask, notProvided: false },
        { key: "network.localMac", value: app.network.localMac, notProvided: false },
        { key: "network.targetMac", value: app.network.targetMac, notProvided: true },
        { key: "network.interfaceName", value: app.network.interfaceName, notProvided: false }
    ]

    function localized(key) {
        app.i18n.lang;
        return app.i18n.text(key);
    }

    function displayValue(value, notProvided) {
        if (value !== undefined && value !== null && String(value).length > 0)
            return String(value);
        return localized(notProvided ? "common.notProvided" : "common.unavailable");
    }

    ColumnLayout {
        width: parent.width
        spacing: th.sp6

        GridLayout {
            columns: root.compact ? 1 : 2
            columnSpacing: th.sp4
            rowSpacing: th.sp4
            Layout.fillWidth: true

            RowLayout {
                spacing: th.sp4
                Layout.fillWidth: true

                Icon {
                    name: app.network.available ? "wifi" : "wifiOff"
                    iconSize: 19
                    tint: app.network.available ? th.primary : th.muted
                    Layout.alignment: Qt.AlignVCenter
                }
                QQC.Label {
                    text: root.localized("network.title")
                    color: th.text
                    font.pixelSize: th.fontHeadline
                    font.weight: th.weightSemibold
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            PrimaryButton {
                text: root.localized("network.refresh")
                variant: "secondary"
                leadingIcon: "refresh"
                Layout.fillWidth: root.compact
                Layout.alignment: Qt.AlignVCenter | (root.compact ? Qt.AlignLeft : Qt.AlignRight)
                onClicked: app.network.refresh()
            }
        }

        GridLayout {
            id: fieldGrid
            columns: root.compact ? 1 : 2
            columnSpacing: th.sp8
            rowSpacing: th.sp5
            Layout.fillWidth: true

            Repeater {
                model: root.fields

                ColumnLayout {
                    required property var modelData
                    spacing: th.sp2
                    Layout.fillWidth: true

                    QQC.Label {
                        text: root.localized(modelData.key)
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: root.displayValue(modelData.value, modelData.notProvided)
                        color: th.text
                        font.pixelSize: th.fontBody
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
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
            StatusBadge {
                text: root.localized(root.subnetKey)
                iconName: root.subnetStatus === 1 ? "check" : "info"
                tint: root.subnetColor
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }
        }
    }
}

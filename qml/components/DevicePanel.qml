import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

BaseCard {
    id: root
    readonly property Theme th: Theme {}

    property string codeText: ""
    property real availableListHeight: 240
    readonly property bool compact: width < th.compactBreakpoint
    hoverable: false

    ColumnLayout {
        Layout.fillWidth: true
        spacing: th.sp6

        ColumnLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            // 明文门户提示：自助门户为 HTTP，口令仅可逆编码，非加密传输。
            RowLayout {
                spacing: th.sp2
                Layout.fillWidth: true
                Icon { name: "shield"; iconSize: th.dp(14); tint: th.warning; Layout.alignment: Qt.AlignTop }
                QQC.Label {
                    text: (app.i18n.dictionary["portal.plaintextWarning"])
                    color: th.secondaryLabel
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                spacing: th.sp4
                Layout.fillWidth: true
                Icon { name: "server"; iconSize: th.dp(18); tint: th.primary; Layout.alignment: Qt.AlignVCenter }
                QQC.Label {
                    text: (app.i18n.dictionary["devices.title"])
                    color: th.text
                    font.weight: th.weightSemibold
                    font.pixelSize: th.fontHeadline
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            RowLayout {
                spacing: th.sp4
                visible: app.portalDevices.authenticated
                Layout.fillWidth: true

                BaseButton {
                    text: (app.i18n.dictionary["devices.refresh"])
                    type: "secondary"
                    leadingIcon: "refresh"
                    busy: app.portalDevices.busy
                    enabled: app.portalDevices.authenticated && !app.portalDevices.busy
                    Layout.fillWidth: true
                    onClicked: app.portalDevices.refreshDevices()
                }
                BaseButton {
                    text: (app.i18n.dictionary["devices.logout"])
                    type: "danger-ghost"
                    leadingIcon: "power"
                    Layout.fillWidth: true
                    onClicked: logoutDialog.open()
                }
            }
        }

        ColumnLayout {
            spacing: th.sp5
            visible: !app.portalDevices.authenticated
            Layout.fillWidth: true

            GridLayout {
                columns: root.compact ? 1 : 2
                columnSpacing: th.sp5
                rowSpacing: th.sp5
                Layout.fillWidth: true

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: root.compact ? -1 : th.dp(120)
                    Layout.preferredHeight: th.dp(52)
                    Layout.maximumWidth: root.compact ? Infinity : th.dp(140)
                    radius: th.radiusSmall
                    color: th.captchaSurface
                    border.width: 1
                    border.color: th.separator
                    clip: true
                    Accessible.name: (app.i18n.dictionary["devices.code"])

                    Image {
                        anchors.fill: parent
                        anchors.margins: 3
                        fillMode: Image.PreserveAspectFit
                        source: app.portalDevices.captchaImage
                        smooth: true
                        asynchronous: true
                    }
                }

                BaseTextField {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    placeholderText: (app.i18n.dictionary["devices.code"])
                    text: root.codeText
                    onEdited: root.codeText = text
                    onAccepted: app.portalDevices.loginWithCode(root.codeText)
                }
            }

            BaseButton {
                text: (app.i18n.dictionary["devices.login"])
                type: "primary"
                leadingIcon: "power"
                busy: app.portalDevices.busy
                enabled: !app.portalDevices.busy
                Layout.fillWidth: true
                onClicked: {
                    app.portalDevices.loginWithCode(root.codeText);
                    root.codeText = "";
                }
            }
        }

        ListView {
            id: deviceList
            visible: app.portalDevices.authenticated
            clip: true
            spacing: th.sp4
            model: app.portalDevices.devices
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            Layout.fillWidth: true
            Layout.preferredHeight: count === 0 ? 120 : Math.max(88, Math.min(root.availableListHeight, contentHeight))

            WheelHandler {
                onWheel: function(event) {
                    var delta = event.pixelDelta.y;
                    if (Math.abs(delta) < 0.01)
                        delta = event.angleDelta.y / 120 * th.wheelStep;
                    if (Math.abs(delta) < 0.01)
                        return;

                    var maximumY = Math.max(0, deviceList.contentHeight - deviceList.height);
                    var nextY = Math.max(0, Math.min(maximumY, deviceList.contentY - delta));
                    if (Math.abs(nextY - deviceList.contentY) < 0.01) {
                        event.accepted = false;
                        return;
                    }
                    deviceList.contentY = nextY;
                    event.accepted = true;
                }
            }

            delegate: DeviceItem {
                required property var modelData
                width: deviceList.width
                device: modelData
            }

            QQC.Label {
                anchors.centerIn: parent
                width: parent.width - th.sp8
                text: (app.i18n.dictionary["devices.empty"])
                color: th.tertiaryLabel
                font.pixelSize: th.fontSubhead
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: deviceList.count === 0
            }
        }

        QQC.Label {
            text: app.portalDevices.lastMessage
            color: th.muted
            font.pixelSize: th.fontFootnote
            wrapMode: Text.WrapAnywhere
            visible: text.length > 0
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: th.sp4
            visible: app.portalDevices.authenticated
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: th.dp(24)
                Layout.preferredHeight: th.dp(24)
                Layout.alignment: Qt.AlignTop
                radius: th.dp(12)
                color: th.orangeFill
                Icon { name: "info"; iconSize: th.dp(14); tint: th.warning; anchors.centerIn: parent }
            }
            QQC.Label {
                text: (app.i18n.dictionary["devices.unsupported"])
                color: th.muted
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
            }
        }
    }

    AppDialog {
        id: logoutDialog
        title: app.i18n.dictionary["devices.logoutTitle"]
        message: app.i18n.dictionary["devices.logoutMessage"]
        confirmText: app.i18n.dictionary["devices.logout"]
        cancelText: app.i18n.dictionary["action.cancel"]
        showCancel: true
        destructive: true
        onAccepted: app.portalDevices.clearSession()
    }
}

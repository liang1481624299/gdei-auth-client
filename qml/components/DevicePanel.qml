import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Card {
    id: root
    readonly property Theme th: Theme {}

    property string codeText: ""
    property real availableListHeight: 240
    readonly property bool compact: width < th.compactBreakpoint

    ColumnLayout {
        width: parent.width
        spacing: th.sp6

        ColumnLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            RowLayout {
                spacing: th.sp4
                Layout.fillWidth: true
                Icon { name: "server"; iconSize: 18; tint: th.primary; Layout.alignment: Qt.AlignVCenter }
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("devices.title"))
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

                PrimaryButton {
                    text: (app.i18n.lang, app.i18n.text("devices.refresh"))
                    variant: "secondary"
                    leadingIcon: "refresh"
                    busy: app.portalDevices.busy
                    enabled: app.portalDevices.authenticated && !app.portalDevices.busy
                    Layout.fillWidth: true
                    onClicked: app.portalDevices.refreshDevices()
                }
                PrimaryButton {
                    text: (app.i18n.lang, app.i18n.text("devices.logout"))
                    variant: "danger"
                    bg: "transparent"
                    leadingIcon: "power"
                    Layout.fillWidth: true
                    onClicked: app.portalDevices.clearSession()
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
                    Layout.preferredWidth: root.compact ? -1 : 120
                    Layout.preferredHeight: 52
                    Layout.maximumWidth: root.compact ? Infinity : 140
                    radius: th.radiusSmall
                    color: "white"
                    border.width: 1
                    border.color: th.separator
                    clip: true
                    Accessible.name: (app.i18n.lang, app.i18n.text("devices.code"))

                    Image {
                        anchors.fill: parent
                        anchors.margins: 3
                        fillMode: Image.PreserveAspectFit
                        source: app.portalDevices.captchaImage
                        smooth: true
                        asynchronous: true
                    }
                }

                TextField {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    placeholderText: (app.i18n.lang, app.i18n.text("devices.code"))
                    leadingIcon: "shield"
                    text: root.codeText
                    onTextChanged: root.codeText = text
                    onAccepted: app.portalDevices.loginWithCode(root.codeText)
                }
            }

            PrimaryButton {
                text: (app.i18n.lang, app.i18n.text("devices.login"))
                variant: "primary"
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
                text: (app.i18n.lang, app.i18n.text("devices.empty"))
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
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignTop
                radius: 12
                color: th.orangeFill
                Icon { name: "info"; iconSize: 14; tint: th.warning; anchors.centerIn: parent }
            }
            QQC.Label {
                text: (app.i18n.lang, app.i18n.text("devices.unsupported"))
                color: th.muted
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
            }
        }
    }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}

    title: "profiles.remoteEntry"
    showBack: true

    Component.onCompleted: {
        if (!app.portalDevices.authenticated)
            app.portalDevices.fetchCaptcha();
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: th.sp8

        BaseCard {
            Layout.fillWidth: true
            hoverable: false

            QQC.Label {
                Layout.fillWidth: true
                text: (app.i18n.dictionary["profiles.remoteSubtitle"])
                color: th.text
                font.pixelSize: th.fontBody
                wrapMode: Text.Wrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: th.sp4

                Rectangle {
                    Layout.preferredWidth: th.dp(24)
                    Layout.preferredHeight: th.dp(24)
                    Layout.alignment: Qt.AlignTop
                    radius: th.dp(12)
                    color: th.orangeFill
                    Icon {
                        anchors.centerIn: parent
                        name: "info"
                        iconSize: th.dp(14)
                        tint: th.warning
                    }
                }
                QQC.Label {
                    text: (app.i18n.dictionary["profiles.readOnly"])
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
            }
        }

        DevicePanel {
            availableListHeight: Math.max(160, Math.min(420, root.height * 0.45))
            Layout.fillWidth: true
        }
    }
}

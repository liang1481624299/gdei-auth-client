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
        width: parent.width
        spacing: th.sp8

        Card {
            Layout.fillWidth: true

            QQC.Label {
                width: parent.width
                text: (app.i18n.lang, app.i18n.text("profiles.remoteSubtitle"))
                color: th.text
                font.pixelSize: th.fontBody
                wrapMode: Text.Wrap
            }
            RowLayout {
                width: parent.width
                spacing: th.sp4

                Rectangle {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignTop
                    radius: 12
                    color: th.orangeFill
                    Icon {
                        anchors.centerIn: parent
                        name: "info"
                        iconSize: 14
                        tint: th.warning
                    }
                }
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("profiles.readOnly"))
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

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}

    title: "about.title"
    showBack: true

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        Card {
            Layout.fillWidth: true

            RowLayout {
                width: parent.width
                spacing: th.sp6

                Rectangle {
                    Layout.preferredWidth: 52
                    Layout.preferredHeight: 52
                    Layout.alignment: Qt.AlignVCenter
                    radius: th.radius
                    color: th.blueFill
                    Icon {
                        anchors.centerIn: parent
                        name: "power"
                        iconSize: 26
                        tint: th.primary
                    }
                }

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true

                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("app.title"))
                        color: th.text
                        font.pixelSize: th.fontTitle3
                        font.weight: th.weightSemibold
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (app.i18n.lang, app.i18n.text("app.subtitle"))
                        color: th.muted
                        font.pixelSize: th.fontSubhead
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: th.separator
            }

            RowLayout {
                width: parent.width
                spacing: th.sp4

                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("about.version"))
                    color: th.muted
                    font.pixelSize: th.fontBody
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: "0.2.0"
                    color: th.text
                    font.pixelSize: th.fontBody
                    font.weight: th.weightMedium
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            QQC.Label {
                width: parent.width
                text: (app.i18n.lang, app.i18n.text("about.description"))
                color: th.text
                font.pixelSize: th.fontBody
                lineHeight: 1.35
                wrapMode: Text.Wrap
            }
            QQC.Label {
                width: parent.width
                text: (app.i18n.lang, app.i18n.text("about.technology"))
                color: th.muted
                font.pixelSize: th.fontFootnote
                lineHeight: 1.35
                wrapMode: Text.Wrap
            }
        }
    }
}

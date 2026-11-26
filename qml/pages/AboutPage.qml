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
        Layout.fillWidth: true
        spacing: th.sp8

        BaseCard {
            Layout.fillWidth: true
            hoverable: false

            RowLayout {
                Layout.fillWidth: true
                spacing: th.sp6

                Image {
                    Layout.preferredWidth: th.dp(52)
                    Layout.preferredHeight: th.dp(52)
                    Layout.alignment: Qt.AlignVCenter
                    source: "../icons/app.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    sourceSize: Qt.size(th.dp(52), th.dp(52))
                }

                ColumnLayout {
                    spacing: th.sp2
                    Layout.fillWidth: true

                    QQC.Label {
                        text: (app.i18n.dictionary["app.title"])
                        color: th.text
                        font.pixelSize: th.fontTitle3
                        font.weight: th.weightSemibold
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: (app.i18n.dictionary["app.subtitle"])
                        color: th.muted
                        font.pixelSize: th.fontSubhead
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: th.separator
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: th.sp4

                QQC.Label {
                    text: (app.i18n.dictionary["about.version"])
                    color: th.muted
                    font.pixelSize: th.fontBody
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: "0.3.0"
                    color: th.text
                    font.pixelSize: th.fontBody
                    font.weight: th.weightMedium
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            QQC.Label {
                Layout.fillWidth: true
                text: (app.i18n.dictionary["about.description"])
                color: th.text
                font.pixelSize: th.fontBody
                lineHeight: 1.35
                wrapMode: Text.Wrap
            }
            QQC.Label {
                Layout.fillWidth: true
                text: (app.i18n.dictionary["about.technology"])
                color: th.muted
                font.pixelSize: th.fontFootnote
                lineHeight: 1.35
                wrapMode: Text.Wrap
            }
        }
    }
}

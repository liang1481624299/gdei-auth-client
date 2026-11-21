// qml/components/LogPanel.qml
// 日志面板 / Leveled log panel with export action.
import QtQuick
import QtQuick.Controls
import "../theme"
import "."

Card {
    id: root

    property int panelHeight: 220

    Column {
        spacing: 10

        Row {
            width: parent.width
            spacing: 12
            Label {
                text: app.i18n.lang, app.i18n.text("log.title")
                color: theme.text
                font.bold: true
                font.pixelSize: 15
                anchors.verticalCenter: parent.verticalCenter
            }
            Item { width: 1; height: 1; anchors.verticalCenter: parent.verticalCenter }
            PrimaryButton {
                text: app.i18n.lang, app.i18n.text("log.export")
                bg: theme.muted
                implicitHeight: 34
                padding: 12
                onClicked: app.logs.exportLog("")
            }
        }

        ListView {
            id: listView
            width: parent.width
            height: root.panelHeight
            clip: true
            spacing: 6
            model: app.logs.entries

            delegate: Row {
                width: listView.width
                spacing: 8

                Label {
                    text: model.time
                    color: theme.muted
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignTop
                }
                Label {
                    text: model.levelText
                    font.pixelSize: 12
                    font.bold: true
                    verticalAlignment: Text.AlignTop
                    color: {
                        switch (model.level) {
                            case 1: return theme.primary;
                            case 2: return theme.warning;
                            case 3: return theme.danger;
                            default: return theme.muted;
                        }
                    }
                }
                Label {
                    text: model.message
                    color: theme.text
                    font.pixelSize: 12
                    width: listView.width - 150
                    wrapMode: Text.Wrap
                    verticalAlignment: Text.AlignTop
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}
    readonly property var filterModel: [
        { "key": "events.filterAll", "level": -1 },
        { "key": "events.filterInfo", "level": 1 },
        { "key": "events.filterWarning", "level": 2 },
        { "key": "events.filterError", "level": 3 }
    ]

    title: "events.title"
    showBack: true

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        Card {
            Layout.fillWidth: true

            GridLayout {
                width: parent.width
                columns: width >= th.compactBreakpoint ? 4 : 2
                columnSpacing: th.sp4
                rowSpacing: th.sp4

                Repeater {
                    model: root.filterModel

                    PrimaryButton {
                        required property var modelData
                        text: (app.i18n.lang, app.i18n.text(modelData.key))
                        variant: app.events.filterLevel === modelData.level ? "primary" : "secondary"
                        Layout.fillWidth: true
                        onClicked: app.events.filterLevel = modelData.level
                    }
                }
            }

            Rectangle {
                width: parent.width
                implicitHeight: infoToastsRow.implicitHeight
                radius: th.radiusSmall
                color: th.field
                border.width: 1
                border.color: th.separator

                SettingRow {
                    id: infoToastsRow
                    width: parent.width
                    title: (app.i18n.lang, app.i18n.text("events.infoToasts"))
                    Switch {
                        checked: app.events.infoToasts
                        onToggled: app.events.infoToasts = checked
                    }
                }
            }

            PrimaryButton {
                width: parent.width
                text: (app.i18n.lang, app.i18n.text("events.clear"))
                variant: "danger"
                bg: "transparent"
                leadingIcon: "trash"
                onClicked: clearDialog.open()
            }
        }

        Card {
            margins: th.sp3
            Layout.fillWidth: true

            Item {
                width: parent.width
                height: Math.max(200, Math.min(480, root.height * 0.52))

                ListView {
                    id: eventList
                    anchors.fill: parent
                    clip: true
                    spacing: th.sp2
                    model: app.eventModel
                    interactive: contentHeight > height
                    boundsBehavior: Flickable.StopAtBounds
                    QQC.ScrollBar.vertical: QQC.ScrollBar {
                        policy: QQC.ScrollBar.AsNeeded
                    }

                    WheelHandler {
                        onWheel: function(event) {
                            var delta = event.pixelDelta.y;
                            if (Math.abs(delta) < 0.01)
                                delta = event.angleDelta.y / 120 * th.wheelStep;
                            if (Math.abs(delta) < 0.01)
                                return;

                            var maximumY = Math.max(0, eventList.contentHeight - eventList.height);
                            var nextY = Math.max(0, Math.min(maximumY,
                                                              eventList.contentY - delta));
                            if (Math.abs(nextY - eventList.contentY) < 0.01) {
                                event.accepted = false;
                                return;
                            }
                            eventList.contentY = nextY;
                            event.accepted = true;
                        }
                    }

                    delegate: EventListItem {
                        required property var model

                        width: eventList.width
                        timestamp: model.timestamp
                        level: model.level
                        titleKey: model.titleKey
                        messageKey: model.messageKey
                        reasonKey: model.reasonKey
                        source: model.source
                        arguments: model.arguments
                        unread: model.unread
                    }
                }

                QQC.Label {
                    anchors.centerIn: parent
                    width: Math.max(0, parent.width - th.sp12)
                    text: (app.i18n.lang, app.i18n.text("events.empty"))
                    color: th.tertiaryLabel
                    font.pixelSize: th.fontSubhead
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    visible: eventList.count === 0
                }
            }
        }
    }

    property var clearDialog: AppDialog {
        id: clearDialog
        title: (app.i18n.lang, app.i18n.text("events.clearTitle"))
        message: (app.i18n.lang, app.i18n.text("events.clearMessage"))
        confirmText: (app.i18n.lang, app.i18n.text("events.clear"))
        cancelText: (app.i18n.lang, app.i18n.text("action.cancel"))
        showCancel: true
        destructive: true
        onAccepted: app.events.clear()
    }
}

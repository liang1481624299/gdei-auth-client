import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Card {
    id: root
    readonly property Theme th: Theme {}

    property real viewportHeight: 220
    readonly property bool compact: width < th.compactBreakpoint

    function levelColor(level) {
        if (level === 1) return th.primary;
        if (level === 2) return th.warning;
        if (level === 3) return th.danger;
        return th.muted;
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
                Icon { name: "list"; iconSize: 18; tint: th.primary; Layout.alignment: Qt.AlignVCenter }
                QQC.Label {
                    text: (app.i18n.lang, app.i18n.text("log.title"))
                    color: th.text
                    font.weight: th.weightSemibold
                    font.pixelSize: th.fontHeadline
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
            PrimaryButton {
                text: (app.i18n.lang, app.i18n.text("log.export"))
                variant: "secondary"
                leadingIcon: "download"
                Layout.fillWidth: root.compact
                Layout.alignment: Qt.AlignVCenter | (root.compact ? Qt.AlignLeft : Qt.AlignRight)
                onClicked: {
                    var path = app.logs.exportLog("");
                    exportDialog.message = path.length > 0
                            ? (app.i18n.lang, app.i18n.text("log.exported"))
                            : (app.i18n.lang, app.i18n.text("log.exportFailed"));
                    exportDialog.detail = path;
                    exportDialog.open();
                }
            }
        }

        Rectangle {
            radius: th.radiusSmall
            color: th.field
            border.width: 1
            border.color: th.separator
            clip: true
            Layout.fillWidth: true
            Layout.preferredHeight: root.viewportHeight

            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: th.sp2
                clip: true
                spacing: 2
                model: app.logs.entries
                boundsBehavior: Flickable.StopAtBounds
                QQC.ScrollBar.vertical: QQC.ScrollBar {
                    policy: QQC.ScrollBar.AsNeeded
                    interactive: true
                }
                property bool followTail: true

                WheelHandler {
                    onWheel: function(event) {
                        var delta = event.pixelDelta.y;
                        if (Math.abs(delta) < 0.01)
                            delta = event.angleDelta.y / 120 * th.wheelStep;
                        if (Math.abs(delta) < 0.01)
                            return;

                        var maximumY = Math.max(0, listView.contentHeight - listView.height);
                        var nextY = Math.max(0, Math.min(maximumY, listView.contentY - delta));
                        if (Math.abs(nextY - listView.contentY) < 0.01) {
                            event.accepted = false;
                            return;
                        }
                        listView.contentY = nextY;
                        event.accepted = true;
                    }
                }

                onMovementEnded: followTail = atYEnd
                onContentYChanged: {
                    if (!moving && !dragging && !flicking) followTail = atYEnd;
                }
                onCountChanged: if (count === 0) followTail = true

                delegate: Rectangle {
                    required property var modelData
                    width: listView.width
                    height: body.implicitHeight + 12
                    color: "transparent"

                    Column {
                        id: body
                        x: th.sp4
                        y: th.sp3
                        width: Math.max(0, parent.width - th.sp8)
                        spacing: 2

                        RowLayout {
                            width: parent.width
                            spacing: th.sp4

                            Rectangle {
                                Layout.preferredWidth: 7
                                Layout.preferredHeight: 7
                                Layout.alignment: Qt.AlignVCenter
                                radius: 3.5
                                color: root.levelColor(modelData.level)
                            }
                            QQC.Label {
                                text: modelData.time
                                color: th.tertiaryLabel
                                font.pixelSize: th.fontFootnote
                                elide: Text.ElideRight
                                Layout.preferredWidth: 62
                            }
                            QQC.Label {
                                text: modelData.levelText
                                font.pixelSize: th.fontFootnote
                                font.weight: th.weightSemibold
                                color: root.levelColor(modelData.level)
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                        QQC.Label {
                            width: parent.width
                            leftPadding: 15
                            text: modelData.message
                            color: th.text
                            font.pixelSize: th.fontFootnote
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }

                Icon {
                    anchors.centerIn: parent
                    name: "list"
                    iconSize: 26
                    tint: th.quaternaryLabel
                    visible: listView.count === 0
                }
            }

            Connections {
                target: app.logs
                function onEntriesChanged() {
                    if (!listView.followTail || listView.count === 0) return;
                    Qt.callLater(function() {
                        if (listView.followTail) listView.positionViewAtEnd();
                    });
                }
            }
        }
    }

    property var exportDialog: AppDialog {
        title: (app.i18n.lang, app.i18n.text("log.export"))
        confirmText: (app.i18n.lang, app.i18n.text("action.ok"))
        showCancel: false
    }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Item {
    id: root
    Theme { id: th }

    property string title: ""
    property string subtitle: ""
    property bool showBack: false
    property int contentMaxWidth: th.dp(1080)
    signal backRequested()

    default property alias content: contentColumn.data

    readonly property bool compact: width < th.compactBreakpoint
    readonly property int pagePadding: compact ? th.sp6 : th.sp12

    function localized(key) {
        if (key.length === 0) return "";
        app.i18n.lang;
        return app.i18n.text(key);
    }

    QQC.ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        contentHeight: page.height
        QQC.ScrollBar.vertical.policy: QQC.ScrollBar.AsNeeded
        QQC.ScrollBar.horizontal.policy: QQC.ScrollBar.AlwaysOff
        background: Rectangle { color: "transparent" }

        Component.onCompleted: contentItem.boundsBehavior = Flickable.StopAtBounds

        WheelHandler {
            onWheel: function(event) {
                var delta = event.pixelDelta.y;
                if (Math.abs(delta) < 0.01)
                    delta = event.angleDelta.y / 120 * th.wheelStep;
                if (Math.abs(delta) < 0.01)
                    return;

                var flickable = scroll.contentItem;
                var maximumY = Math.max(0, flickable.contentHeight - flickable.height);
                var nextY = Math.max(0, Math.min(maximumY, flickable.contentY - delta));
                if (Math.abs(nextY - flickable.contentY) < 0.01) {
                    event.accepted = false;
                    return;
                }
                flickable.contentY = nextY;
                event.accepted = true;
            }
        }

        Item {
            id: page
            width: scroll.availableWidth
            height: contentColumn.implicitHeight + root.pagePadding * 2

            ColumnLayout {
                id: contentColumn
                anchors.top: parent.top
                anchors.topMargin: root.pagePadding
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(root.contentMaxWidth,
                                Math.max(0, parent.width - root.pagePadding * 2))
                spacing: th.sp8

                RowLayout {
                    visible: root.showBack || root.title.length > 0 || root.subtitle.length > 0
                    spacing: th.sp4
                    Layout.fillWidth: true

                    QQC.Button {
                        id: backButton
                        visible: root.showBack
                        implicitWidth: th.touchTarget
                        implicitHeight: th.touchTarget
                        padding: 0
                        hoverEnabled: true
                        Accessible.name: root.localized("action.back")
                        onClicked: root.backRequested()

                        contentItem: Icon {
                            name: "chevronLeft"
                            iconSize: 21
                            tint: backButton.down || backButton.hovered ? th.primary : th.text
                        }

                        background: Rectangle {
                            radius: th.radiusSmall
                            color: backButton.down ? th.controlPressed
                                 : backButton.hovered ? th.controlHover : "transparent"
                            border.width: backButton.activeFocus ? 2 : 0
                            border.color: th.focusRing
                            Behavior on color { ColorAnimation { duration: th.durationFast } }
                        }
                    }

                    ColumnLayout {
                        spacing: th.sp2
                        Layout.fillWidth: true

                        QQC.Label {
                            text: root.localized(root.title)
                            visible: root.title.length > 0
                            color: th.text
                            font.pixelSize: th.fontTitle1
                            font.weight: th.weightBold
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        QQC.Label {
                            text: root.localized(root.subtitle)
                            visible: root.subtitle.length > 0
                            color: th.muted
                            font.pixelSize: th.fontBody
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}

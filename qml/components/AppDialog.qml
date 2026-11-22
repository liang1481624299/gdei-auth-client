import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

QQC.Dialog {
    id: dialog
    Theme { id: th }

    property string message: ""
    property string detail: ""
    property string confirmText: ""
    property string cancelText: ""
    property bool showCancel: false
    property bool destructive: false

    parent: QQC.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(th.dp(420), Math.max(0, parent.width - th.sp8))
    maximumHeight: Math.max(th.dp(180), parent.height - th.sp16)
    modal: true
    focus: true
    padding: th.sp10
    closePolicy: QQC.Popup.CloseOnEscape

    QQC.Overlay.modal: Rectangle { color: th.overlay }

    contentItem: QQC.ScrollView {
        id: dialogScroll
        clip: true
        contentWidth: availableWidth
        contentHeight: dialogColumn.implicitHeight
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
                var flickable = dialogScroll.contentItem;
                var maximumY = Math.max(0, flickable.contentHeight - flickable.height);
                var nextY = Math.max(0, Math.min(maximumY, flickable.contentY - delta));
                event.accepted = Math.abs(nextY - flickable.contentY) >= 0.01;
                flickable.contentY = nextY;
            }
        }

        ColumnLayout {
            id: dialogColumn
            width: dialogScroll.availableWidth
            spacing: th.sp4

            QQC.Label {
                text: dialog.title
                color: th.text
                font.pixelSize: th.fontTitle3
                font.weight: th.weightSemibold
                wrapMode: Text.Wrap
                visible: text.length > 0
                Layout.fillWidth: true
            }
            QQC.Label {
                text: dialog.message
                color: th.text
                font.pixelSize: th.fontBody
                wrapMode: Text.Wrap
                visible: text.length > 0
                Layout.fillWidth: true
            }
            QQC.Label {
                text: dialog.detail
                color: th.muted
                font.pixelSize: th.fontFootnote
                wrapMode: Text.WrapAnywhere
                visible: text.length > 0
                Layout.fillWidth: true
            }
        }
    }

    footer: RowLayout {
        spacing: th.sp4

        Item { Layout.fillWidth: true }
        PrimaryButton {
            text: dialog.cancelText
            variant: "ghost"
            visible: dialog.showCancel && text.length > 0
            onClicked: dialog.reject()
        }
        PrimaryButton {
            text: dialog.confirmText
            variant: dialog.destructive ? "danger" : "primary"
            visible: text.length > 0
            onClicked: dialog.accept()
        }
    }

    background: Rectangle {
        radius: th.radiusLarge
        color: th.surfaceElevated
        border.width: 1
        border.color: th.separator
    }

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: th.durationNormal } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: th.durationFast } }
}

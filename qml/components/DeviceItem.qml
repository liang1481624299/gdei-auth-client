import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Rectangle {
    id: root
    Theme { id: th }

    property var device

    implicitWidth: 300
    implicitHeight: body.implicitHeight + 20
    height: implicitHeight
    color: th.field
    radius: th.radiusSmall
    border.width: 1
    border.color: th.separator

    function catText(cat) {
        app.i18n.lang;
        if (cat === "history") return app.i18n.text("devices.cat.history");
        if (cat === "bound") return app.i18n.text("devices.cat.bound");
        return app.i18n.text("devices.cat.online");
    }
    function catColor(cat) {
        if (cat === "history") return th.muted;
        if (cat === "bound") return th.primary;
        return th.success;
    }
    function joinNonEmpty(parts) {
        return parts.filter(function(s) { return s && s.length > 0; }).join(" · ");
    }

    Column {
        id: body
        x: 10
        y: 10
        width: Math.max(0, parent.width - 20)
        spacing: th.sp2

        GridLayout {
            width: parent.width
            columns: root.width < 340 ? 1 : 2
            columnSpacing: th.sp4
            rowSpacing: th.sp2

            QQC.Label {
                text: root.device.mac || "-"
                color: th.text
                font.weight: th.weightSemibold
                font.pixelSize: th.fontBody
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }
            StatusBadge {
                text: root.catText(root.device.category)
                tint: root.catColor(root.device.category)
                Layout.alignment: Qt.AlignVCenter | (root.width < 340 ? Qt.AlignLeft : Qt.AlignRight)
            }
        }

        QQC.Label {
            width: parent.width
            text: root.joinNonEmpty([root.device.deviceType, root.device.ip,
                                     root.device.os, root.device.service])
            color: th.muted
            font.pixelSize: th.fontFootnote
            wrapMode: Text.WrapAnywhere
            visible: text.length > 0
        }

        QQC.Label {
            width: parent.width
            text: root.joinNonEmpty([root.device.startTime, root.device.endTime,
                                     root.device.duration, root.device.authStatus,
                                     root.device.lostStatus])
            color: th.tertiaryLabel
            font.pixelSize: th.fontFootnote
            wrapMode: Text.WrapAnywhere
            visible: text.length > 0
        }
    }
}

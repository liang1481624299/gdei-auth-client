// qml/components/DeviceItem.qml
// 单台设备条目 / One device row: MAC + category tag, detail line, status line.
import QtQuick
import QtQuick.Controls
import "../theme"

Rectangle {
    id: root
    Theme { id: th }

    property var device

    width: parent ? parent.width : 300
    height: body.implicitHeight + 16
    color: th.field
    radius: th.radiusSmall

    function catText(cat) {
        if (cat === "history") return app.i18n.text("devices.cat.history");
        if (cat === "bound")   return app.i18n.text("devices.cat.bound");
        return app.i18n.text("devices.cat.online");
    }
    function catColor(cat) {
        if (cat === "history") return th.muted;
        if (cat === "bound")   return th.primary;
        return th.success;
    }
    function joinNonEmpty(parts) {
        return parts.filter(function (s) { return s && s.length > 0; }).join(" · ");
    }

    Column {
        id: body
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        Row {
            width: parent.width
            spacing: 8
            Label {
                text: root.device.mac || "-"
                color: th.text
                font.bold: true
                font.pixelSize: 14
                anchors.verticalCenter: parent.verticalCenter
            }
            Item { width: 1; height: 1; anchors.verticalCenter: parent.verticalCenter }
            Rectangle {
                width: tag.implicitWidth + 14
                height: 22
                radius: 11
                color: root.catColor(root.device.category)
                anchors.verticalCenter: parent.verticalCenter
                Label {
                    id: tag
                    text: root.catText(root.device.category)
                    color: "white"
                    font.pixelSize: 11
                    anchors.centerIn: parent
                }
            }
        }

        Label {
            width: parent.width
            text: root.joinNonEmpty([root.device.deviceType, root.device.ip,
                                     root.device.os, root.device.service])
            color: th.muted
            font.pixelSize: 12
            wrapMode: Text.Wrap
            visible: text.length > 0
        }

        Label {
            width: parent.width
            text: root.joinNonEmpty([root.device.startTime, root.device.endTime,
                                     root.device.duration, root.device.authStatus,
                                     root.device.lostStatus])
            color: th.muted
            font.pixelSize: 12
            wrapMode: Text.Wrap
            visible: text.length > 0
        }
    }
}

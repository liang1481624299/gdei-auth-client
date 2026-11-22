import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Rectangle {
    id: root
    Theme { id: th }

    property var timestamp: null
    property int level: 1
    property string titleKey: ""
    property string messageKey: ""
    property string reasonKey: ""
    property string source: ""
    property var arguments: null
    property bool unread: false

    readonly property string levelKey: level === 2 ? "events.levelWarning"
                                      : level === 3 ? "events.levelError"
                                      : "events.levelInfo"
    readonly property color levelColor: level === 2 ? th.warning
                                       : level === 3 ? th.danger : th.primary

    implicitHeight: body.implicitHeight + th.sp8
    height: implicitHeight
    radius: th.radiusSmall
    color: unread ? th.blueFill : th.card
    border.width: 1
    border.color: unread ? th.fill(th.primary, 0.32) : th.separator

    Behavior on color { ColorAnimation { duration: th.durationNormal } }
    Behavior on border.color { ColorAnimation { duration: th.durationNormal } }

    function localized(key, args) {
        if (key.length === 0) return "";
        app.i18n.lang;
        var value = app.i18n.text(key);
        if (args && args.name !== undefined && args.name !== null && value.indexOf("%1") >= 0)
            value = value.arg(String(args.name));
        return value;
    }

    ColumnLayout {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: th.sp4
        spacing: th.sp3

        RowLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            QQC.Label {
                text: Qt.formatDateTime(root.timestamp, "yyyy-MM-dd HH:mm:ss")
                color: th.tertiaryLabel
                font.pixelSize: th.fontCaption
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }

            StatusBadge {
                text: root.localized(root.levelKey, null)
                tint: root.levelColor
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            UnreadBadge {
                visible: root.unread
                Layout.alignment: Qt.AlignVCenter
            }
        }

        QQC.Label {
            text: root.localized(root.titleKey, root.arguments)
            color: th.text
            font.pixelSize: th.fontCallout
            font.weight: th.weightSemibold
            wrapMode: Text.WrapAnywhere
            visible: root.titleKey.length > 0
            Layout.fillWidth: true
        }

        QQC.Label {
            text: root.localized(root.messageKey, root.arguments)
            color: th.text
            font.pixelSize: th.fontSubhead
            wrapMode: Text.WrapAnywhere
            visible: root.messageKey.length > 0
            Layout.fillWidth: true
        }

        QQC.Label {
            text: root.localized(root.reasonKey, root.arguments)
            color: th.muted
            font.pixelSize: th.fontFootnote
            wrapMode: Text.WrapAnywhere
            visible: root.reasonKey.length > 0
            Layout.fillWidth: true
        }

        QQC.Label {
            text: root.localized("events.source." + root.source, null)
            color: th.tertiaryLabel
            font.pixelSize: th.fontCaption
            wrapMode: Text.WrapAnywhere
            visible: root.source.length > 0
            Layout.fillWidth: true
        }
    }
}

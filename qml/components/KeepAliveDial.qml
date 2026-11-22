import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

QQC.Button {
    id: control
    Theme { id: th }

    property bool running: false
    property bool online: false
    property bool busy: false
    signal toggleRequested()

    readonly property real diameter: Math.max(th.touchTarget, Math.min(240, width))
    readonly property color statusColor: running ? (online ? th.success : th.warning) : th.muted
    readonly property string stateKey: running ? "keepalive.on" : "keepalive.off"

    text: (app.i18n.lang, app.i18n.text(stateKey))
    implicitWidth: 240
    implicitHeight: diameter
    padding: 0
    enabled: !busy
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: busy ? 0.72 : 1
    scale: down && !busy ? 0.985 : 1
    Accessible.name: text
    Accessible.description: (app.i18n.lang, app.i18n.text("keepalive.hint"))
    onClicked: toggleRequested()

    Behavior on scale {
        SpringAnimation { spring: 4.5; damping: 0.78; epsilon: 0.001 }
    }
    Behavior on opacity { NumberAnimation { duration: th.durationFast } }

    contentItem: Item {
        ColumnLayout {
            anchors.centerIn: parent
            width: Math.max(0, Math.min(184, control.diameter - th.sp16))
            spacing: control.diameter < 160 ? th.sp2 : th.sp4

            Icon {
                name: control.running ? "bolt" : "power"
                iconSize: control.diameter < 160 ? 28 : 40
                tint: control.statusColor
                Layout.alignment: Qt.AlignHCenter
                Behavior on tint { ColorAnimation { duration: th.durationNormal } }
            }

            QQC.Label {
                text: control.text
                color: control.statusColor
                font.pixelSize: control.diameter < 160 ? th.fontBody : th.fontTitle3
                font.weight: th.weightSemibold
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Behavior on color { ColorAnimation { duration: th.durationNormal } }
            }

            QQC.Label {
                text: (app.i18n.lang, app.i18n.text("keepalive.hint"))
                color: th.muted
                font.pixelSize: control.diameter < 160 ? th.fontCaption : th.fontFootnote
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                maximumLineCount: 3
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    background: Item {
        Rectangle {
            anchors.centerIn: parent
            width: control.diameter + 6
            height: control.diameter + 6
            radius: width / 2
            color: "transparent"
            border.width: 2
            border.color: th.focusRing
            opacity: control.activeFocus ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: th.durationFast } }
        }

        Rectangle {
            anchors.centerIn: parent
            width: control.diameter
            height: control.diameter
            radius: width / 2
            color: th.fill(control.statusColor, control.running ? (th.isDark ? 0.18 : 0.10)
                                                                : (th.isDark ? 0.12 : 0.07))
            border.width: 2
            border.color: th.fill(control.statusColor, control.hovered ? 0.82 : 0.58)
            antialiasing: true
            Behavior on color { ColorAnimation { duration: th.durationNormal } }
            Behavior on border.color { ColorAnimation { duration: th.durationNormal } }
        }
    }
}

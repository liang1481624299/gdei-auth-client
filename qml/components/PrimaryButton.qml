import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

QQC.Button {
    id: control
    Theme { id: th }

    property string variant: "primary"
    property color bg: variant === "danger" ? th.danger
                     : variant === "primary" ? th.primary
                     : variant === "secondary" ? th.secondaryBg : "transparent"
    property string leadingIcon: ""
    property bool busy: false

    readonly property bool isFilled: bg.a > 0
    readonly property color contentColor: !enabled ? th.disabledText
                                        : variant === "danger" ? (isFilled ? th.dangerForeground : th.danger)
                                        : variant === "primary" ? th.primaryForeground
                                        : variant === "secondary" ? th.text : th.primary

    enabled: !busy
    implicitHeight: th.buttonHeight
    leftPadding: 16
    rightPadding: 16
    topPadding: 0
    bottomPadding: 0
    font.pixelSize: th.fontBody
    font.weight: th.weightSemibold
    hoverEnabled: true
    opacity: busy ? 0.76 : enabled ? 1 : th.disabledOpacity
    scale: down && !busy ? 0.985 : 1

    Behavior on scale {
        SpringAnimation { spring: 5; damping: 0.72; epsilon: 0.001 }
    }

    contentItem: RowLayout {
        spacing: 8

        Icon {
            name: control.busy ? "refresh" : control.leadingIcon
            iconSize: 17
            tint: control.contentColor
            visible: control.busy || name.length > 0
            Layout.alignment: Qt.AlignVCenter
            RotationAnimation on rotation {
                running: control.busy
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 900
            }
        }

        QQC.Label {
            text: control.text
            color: control.contentColor
            font: control.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
    }

    background: Item {
        implicitHeight: th.buttonHeight

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: th.radiusButton
            color: "transparent"
            border.width: 2
            border.color: th.focusRing
            opacity: control.activeFocus ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: th.durationFast } }
        }

        Rectangle {
            anchors.fill: parent
            radius: th.radiusButton
            color: {
                if (!control.enabled)
                    return control.isFilled ? th.disabledBackground : "transparent";
                if (control.variant === "danger" && !control.isFilled)
                    return control.down ? th.redFill : control.hovered ? th.redFill : "transparent";
                if (!control.isFilled)
                    return control.down ? th.controlPressed : control.hovered ? th.controlHover : "transparent";
                if (control.down) return th.pressed(control.bg);
                if (control.hovered) return th.hovered(control.bg);
                return control.bg;
            }
            border.width: control.variant === "secondary" || (control.variant === "danger" && !control.isFilled) ? 1 : 0
            border.color: control.variant === "danger" && !control.isFilled ? th.redFill : th.separator
            Behavior on color { ColorAnimation { duration: th.durationFast } }
            Behavior on border.color { ColorAnimation { duration: th.durationFast } }
        }
    }
}

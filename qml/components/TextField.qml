import QtQuick
import QtQuick.Controls as QQC
import "../theme"
import "."

QQC.TextField {
    id: control
    Theme { id: th }

    property bool passwordMode: false
    property bool passwordVisible: false
    property string leadingIcon: ""
    property string showPasswordAccessibleText: ""
    property string hidePasswordAccessibleText: ""

    color: enabled ? th.text : th.disabledText
    placeholderTextColor: th.tertiaryLabel
    selectedTextColor: th.text
    selectionColor: th.blueFill
    echoMode: passwordMode && !passwordVisible ? TextInput.Password : TextInput.Normal
    font.pixelSize: th.fontBody
    font.weight: th.weightRegular
    implicitHeight: th.inputHeight
    leftPadding: leadingIcon.length > 0 ? 40 : 14
    rightPadding: passwordMode ? th.touchTarget + 8 : 14
    topPadding: 0
    bottomPadding: 0
    selectByMouse: true
    hoverEnabled: true
    opacity: enabled ? 1 : th.disabledOpacity

    onPasswordModeChanged: if (!passwordMode) passwordVisible = false

    cursorDelegate: Rectangle {
        color: th.primary
        width: 2
    }

    background: Item {
        implicitHeight: th.inputHeight

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: th.radius
            color: "transparent"
            border.width: 2
            border.color: th.focusRing
            opacity: control.activeFocus ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: th.durationFast } }
        }

        Rectangle {
            anchors.fill: parent
            radius: th.radiusSmall
            color: !control.enabled ? th.disabledBackground
                 : control.activeFocus ? th.card
                 : control.hovered ? th.controlHover : th.field
            border.width: 1
            border.color: control.activeFocus ? th.primary : th.separator
            Behavior on color { ColorAnimation { duration: th.durationFast } }
            Behavior on border.color { ColorAnimation { duration: th.durationFast } }
        }
    }

    Icon {
        id: leading
        anchors.left: parent.left
        anchors.leftMargin: 13
        anchors.verticalCenter: parent.verticalCenter
        name: control.leadingIcon
        iconSize: 17
        tint: control.activeFocus ? th.primary : th.muted
        visible: name.length > 0
        Behavior on tint { ColorAnimation { duration: th.durationFast } }
    }

    QQC.Button {
        id: passwordButton
        visible: control.passwordMode
        enabled: control.enabled
        width: th.touchTarget
        height: th.touchTarget
        anchors.right: parent.right
        anchors.rightMargin: 1
        anchors.verticalCenter: parent.verticalCenter
        padding: 0
        hoverEnabled: true
        Accessible.name: control.passwordVisible ? control.hidePasswordAccessibleText
                                                 : control.showPasswordAccessibleText
        onClicked: {
            control.passwordVisible = !control.passwordVisible;
            control.forceActiveFocus();
        }

        contentItem: Icon {
            name: control.passwordVisible ? "eyeOff" : "eye"
            iconSize: 18
            tint: passwordButton.activeFocus ? th.primary : th.muted
        }

        background: Rectangle {
            radius: th.radiusSmall
            color: passwordButton.down ? th.controlPressed
                 : passwordButton.hovered ? th.controlHover : "transparent"
            border.width: passwordButton.activeFocus ? 1 : 0
            border.color: th.primary
            Behavior on color { ColorAnimation { duration: th.durationFast } }
        }
    }
}

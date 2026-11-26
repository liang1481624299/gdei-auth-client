import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// BaseTextField —— Outlined 轮廓式输入框（规范 4.3）。
// 透明底、边框与聚焦色取 Theme 令牌、无外发光；提示文字在框下浅灰小字；密码框内置眼睛显隐。
ColumnLayout {
    id: root
    Theme { id: th }

    property alias text: field.text
    property alias placeholderText: field.placeholderText
    property string label: ""
    property string helperText: ""
    property bool password: false
    property bool readOnly: false
    property bool valid: true
    property int inputHeight: th.inputHeight
    property color accent: th.primary
    signal edited(string text)
    signal accepted()

    spacing: th.sp2
    Layout.fillWidth: true

    QQC.Label {
        text: root.label
        visible: text.length > 0
        color: th.secondaryLabel
        font.pixelSize: th.fontFootnote
        Layout.fillWidth: true
    }

    Rectangle {
        Layout.fillWidth: true
        height: root.inputHeight
        radius: th.radiusControl
        color: "transparent"
        border.width: 1
        border.color: !root.valid ? th.danger
                    : field.activeFocus ? root.accent : th.border
        Behavior on border.color { ColorAnimation { duration: th.durationFast } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: th.sp4
            anchors.rightMargin: th.sp4
            spacing: th.sp4

            QQC.TextField {
                id: field
                Layout.fillWidth: true
                Layout.fillHeight: true
                background: Item { }
                color: th.text
                placeholderTextColor: th.tertiaryLabel
                font.pixelSize: th.fontBody
                font.family: th.fontFamily
                selectByMouse: true
                readOnly: root.readOnly
                echoMode: root.password ? (showPwdBtn.checked ? TextInput.Normal : TextInput.Password)
                                         : TextInput.Normal
                onTextEdited: root.edited(text)
                onAccepted: root.accepted()
            }

            Icon {
                id: showPwdBtn
                visible: root.password
                name: parent.visible && field.echoMode === TextInput.Password ? "eyeOff" : "eye"
                iconSize: th.dp(18)
                tint: th.secondaryLabel
                Layout.alignment: Qt.AlignVCenter
                property bool checked: false
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: showPwdBtn.checked = !showPwdBtn.checked
                }
            }
        }
    }

    QQC.Label {
        text: root.helperText
        visible: text.length > 0
        color: root.valid ? th.secondaryLabel : th.danger
        font.pixelSize: th.fontFootnote
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
}

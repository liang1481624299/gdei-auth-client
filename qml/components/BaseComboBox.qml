import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// BaseComboBox —— 下拉选择（规范 4.1/4.3）。
// 与输入框同轮廓样式；下拉箭头用 chevronDown（与页面跳转 chevronRight 区分）。
ColumnLayout {
    id: root
    Theme { id: th }

    property alias model: combo.model
    property alias currentIndex: combo.currentIndex
    property alias currentText: combo.displayText
    property string label: ""
    property string helperText: ""
    property bool enabled: true
    signal activated(int index)

    spacing: th.sp2
    Layout.fillWidth: true

    QQC.Label {
        text: root.label
        visible: text.length > 0
        color: th.secondaryLabel
        font.pixelSize: th.fontFootnote
        Layout.fillWidth: true
    }

    QQC.ComboBox {
        id: combo
        Layout.fillWidth: true
        implicitHeight: th.inputHeight
        enabled: root.enabled
        font.family: th.fontFamily
        font.pixelSize: th.fontBody
        // 内边距属于 Control 自身；contentItem 内的 RowLayout 不接受 padding 属性。
        leftPadding: th.sp4
        rightPadding: th.sp4

        // 自动识别模型文本字段：对象取 name/text，字符串取自身。
        textRole: {
            var m = model
            if (!m || m.length === 0) return ""
            var e = m[0]
            if (typeof e === "object" && e !== null) {
                if (e.name !== undefined) return "name"
                if (e.text !== undefined) return "text"
            }
            return ""
        }

        onActivated: function(index) { root.activated(index) }

        contentItem: RowLayout {
            spacing: th.sp4
            QQC.Label {
                text: combo.displayText
                color: combo.enabled ? th.text : th.tertiaryLabel
                font.pixelSize: th.fontBody
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Icon {
                name: "chevronDown"
                iconSize: th.dp(16)
                tint: combo.enabled ? th.secondaryLabel : th.tertiaryLabel
                rotation: combo.popup.visible ? 180 : 0
                Behavior on rotation { NumberAnimation { duration: th.duration } }
            }
        }

        background: Rectangle {
            radius: th.radiusControl
            color: "transparent"
            border.width: 1
            border.color: combo.pressed ? th.primary : th.border
        }

        indicator: Item { }

        popup: QQC.Popup {
            y: combo.height + th.sp2
            width: combo.width
            implicitHeight: Math.min(contentColumn.implicitHeight, th.dp(260))
            padding: th.sp2
            background: Rectangle {
                radius: th.radiusControl
                color: th.card
                border.width: 1
                border.color: th.border
            }
            contentItem: ColumnLayout {
                id: contentColumn
                spacing: 0
                Repeater {
                    model: combo.popup.visible ? combo.count : 0
                    delegate: Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: th.dp(38)
                        color: combo.currentIndex === index ? th.primaryFill
                              : hoverHandler.hovered ? th.controlHover : "transparent"
                        radius: th.radiusBadge
                        HoverHandler { id: hoverHandler }
                        QQC.Label {
                            anchors.fill: parent
                            anchors.leftMargin: th.sp6
                            verticalAlignment: Text.AlignVCenter
                            text: combo.textAt(index)
                            color: combo.currentIndex === index ? th.primary : th.text
                            font.pixelSize: th.fontBody
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { combo.currentIndex = index; combo.popup.close() }
                        }
                    }
                }
            }
        }
    }

    QQC.Label {
        text: root.helperText
        visible: text.length > 0
        color: th.secondaryLabel
        font.pixelSize: th.fontFootnote
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
}

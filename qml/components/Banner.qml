import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// 全局状态提示横幅（规范 4.9）：背景 bannerBg、8 圆角、左图标、正文白、
// 可点击链接主色、右侧可关闭。用于断网、认证失败等全局提示。
Rectangle {
    id: root
    Theme { id: th }

    property string message: ""
    property color tint: th.primary
    property string iconName: "info"
    property string linkText: ""
    signal linkClicked()
    signal dismissed()

    color: th.bannerBg
    radius: th.radiusCard
    border.width: 1
    border.color: th.border
    Layout.fillWidth: true
    implicitHeight: contentRow.visible ? contentRow.implicitHeight + th.sp4 * 2 : 0
    Behavior on implicitHeight {
        NumberAnimation { duration: th.durationNormal; easing.type: Easing.InOutQuad }
    }

    RowLayout {
        id: contentRow
        anchors.fill: parent
        anchors.margins: th.sp4
        visible: root.message.length > 0
        spacing: th.sp4

        Icon {
            name: root.iconName
            iconSize: th.dp(18)
            tint: root.tint
            Layout.alignment: Qt.AlignTop
        }

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true

            QQC.Label {
                text: root.message
                color: th.text
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            QQC.Label {
                visible: root.linkText.length > 0
                text: root.linkText
                color: th.primary
                font.pixelSize: th.fontFootnote
                Layout.fillWidth: true

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.linkClicked()
                }
            }
        }

        Icon {
            name: "xmark"
            iconSize: th.dp(16)
            tint: th.muted
            Layout.alignment: Qt.AlignTop

            MouseArea {
                anchors.fill: parent
                anchors.margins: -th.sp2
                cursorShape: Qt.PointingHandCursor
                onClicked: root.dismissed()
            }
        }
    }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"

// BaseCard —— 卡片容器 / card container（规范 4.2）。
// 背景 / 边框 / 圆角均取 Theme 令牌；hover 微提亮；无厚重阴影。
Rectangle {
    id: root
    Theme { id: th }

    property string title: ""
    property string subtitle: ""
    property Component actions: null      // 头部右侧操作（可选）
    property bool hoverable: true
    property int cardPadding: th.cardPadding
    property color borderColor: th.border
    property color backgroundColor: th.card
    property int cornerRadius: th.radiusCard

    default property alias content: body.data

    implicitWidth: Math.max(body.implicitWidth, headerRow.implicitWidth) + cardPadding * 2
    implicitHeight: headerRow.implicitHeight + body.implicitHeight
                      + (headerRow.visible && body.children.length > 0 ? th.sp6 : 0)
                      + cardPadding * 2

    color: (hoverable && hoverHandler.hovered) ? th.secondaryBg : root.backgroundColor
    radius: root.cornerRadius
    border.width: 1
    border.color: root.borderColor

    Behavior on color { ColorAnimation { duration: th.duration } }

    HoverHandler { id: hoverHandler; enabled: root.hoverable }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.cardPadding
        spacing: th.sp6

        RowLayout {
            id: headerRow
            Layout.fillWidth: true
            spacing: th.sp4
            visible: root.title.length > 0 || root.subtitle.length > 0 || actionsLoader.item

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                QQC.Label {
                    text: root.title
                    visible: text.length > 0
                    color: th.textPrimary
                    font.pixelSize: th.fontHeadline
                    font.weight: th.weightSemibold
                    Layout.fillWidth: true
                }
                QQC.Label {
                    text: root.subtitle
                    visible: text.length > 0
                    color: th.secondaryLabel
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }
            }

            Loader {
                id: actionsLoader
                sourceComponent: root.actions
                Layout.alignment: Qt.AlignTop
            }
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: th.sp6
        }
    }
}

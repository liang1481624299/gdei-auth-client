import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// PageScaffold —— 页面骨架：左对齐标题区 + 可滚动内容 + 底部粘性区。
// 保留旧 API：title / subtitle / showBack / backText / back() / sticky / content / contentMaxWidth / localized()。
Rectangle {
    id: root
    Theme { id: th }

    property string title: ""
    property string subtitle: ""
    property bool showBack: false
    property string backText: ""
    signal back()
    property Component sticky: null
    property int contentMaxWidth: th.contentMaxWidth
    default property alias content: contentCol.data

    function localized(key) { return app.i18n.localize(key) }

    color: "transparent"

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + (stickyLoader.item ? stickyLoader.item.height + th.sp6 : 0) + th.sp12
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        QQC.ScrollBar.vertical: QQC.ScrollBar { policy: QQC.ScrollBar.AsNeeded }

        Column {
            id: contentColumn
            width: Math.min(flick.width - th.pageSideMargin * 2, root.contentMaxWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 0

            // 顶部安全区
            Item { width: parent.width; height: th.sp6 }

            // 标题区（左对齐）
            Row {
                width: parent.width
                spacing: th.sp4
                visible: root.showBack || root.title.length > 0 || root.subtitle.length > 0

                QQC.Button {
                    visible: root.showBack
                    flat: true
                    hoverEnabled: true
                    padding: 0
                    implicitWidth: Math.max(contentItem.implicitWidth + th.sp4, th.touchTarget)
                    implicitHeight: th.touchTarget
                    onClicked: root.back()
                    contentItem: Row {
                        spacing: th.sp2
                        Icon { name: "chevronLeft"; iconSize: th.dp(18); tint: th.text; anchors.verticalCenter: parent.verticalCenter }
                        QQC.Label { text: root.backText; color: th.text; font.pixelSize: th.fontBody; anchors.verticalCenter: parent.verticalCenter; visible: text.length > 0 }
                    }
                    background: Rectangle { color: parent.hovered ? th.controlHover : "transparent"; radius: th.radiusControl }
                }

                Column {
                    width: parent.width - (root.showBack ? th.touchTarget : 0)
                    spacing: th.sp2
                    QQC.Label {
                        text: root.localized(root.title)
                        visible: text.length > 0
                        color: th.textPrimary
                        font.pixelSize: th.fontTitle3
                        font.weight: th.weightBold
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                    QQC.Label {
                        text: root.localized(root.subtitle)
                        visible: text.length > 0
                        color: th.secondaryLabel
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            Item { width: parent.width; height: th.sp8 }

            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: th.sp8
            }

            Item { width: parent.width; height: th.sp12 }
        }
    }

    // 底部粘性区
    Loader {
        id: stickyLoader
        sourceComponent: root.sticky
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: th.pageSideMargin
        anchors.rightMargin: th.pageSideMargin
    }
}

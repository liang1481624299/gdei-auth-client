import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// IconGrid —— 服务宫格：N 列等宽图标方块 + 可选文字标签；点击回传 index。
// Service icon grid: equal-width icon tiles with optional labels; emits activated(index).
// 数据 model 由调用方传入，每项 { icon: string, label: string }；无真实入口时不传 model，不伪造功能。
Rectangle {
    id: root
    Theme { id: th }

    property var model: []
    property int columns: 4
    property int compactColumns: 2
    signal activated(int index)

    // 无真实入口时整体隐藏，不渲染空宫格。
    visible: root.model && root.model.length > 0

    // 窄屏降列，宽屏按 columns 排布。
    readonly property int effectiveColumns: width < th.compactBreakpoint ? compactColumns : columns
    implicitHeight: grid.implicitHeight

    GridLayout {
        id: grid
        anchors.fill: parent
        columns: root.effectiveColumns
        columnSpacing: th.sp4
        rowSpacing: th.sp6

        Repeater {
            model: root.model
            delegate: QQC.Button {
                id: tileBtn
                required property var modelData
                required property int index
                Layout.fillWidth: true
                implicitHeight: tileCol.implicitHeight
                padding: 0
                hoverEnabled: true
                onClicked: root.activated(index)

                contentItem: ColumnLayout {
                    id: tileCol
                    spacing: th.sp2
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        Layout.preferredWidth: th.dp(44)
                        Layout.preferredHeight: th.dp(44)
                        Layout.alignment: Qt.AlignHCenter
                        radius: th.radiusControl
                        color: tileBtn.hovered ? th.controlHover : th.field
                        Behavior on color { ColorAnimation { duration: th.durationFast } }

                        Icon {
                            anchors.centerIn: parent
                            name: modelData.icon || ""
                            iconSize: th.dp(20)
                            tint: th.secondaryLabel
                            visible: name.length > 0
                        }
                    }
                    QQC.Label {
                        text: modelData.label || ""
                        color: th.secondaryLabel
                        font.pixelSize: th.fontCaption
                        horizontalAlignment: Text.AlignHCenter
                        visible: text.length > 0
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
                background: Item {}
            }
        }
    }
}

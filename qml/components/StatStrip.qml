import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"

// StatStrip —— 等分统计条：单卡内 N 列均分，数字在上粗体、标签在下灰色，列间竖细分隔线。
// Equal-split stat strip: N columns in one card, bold value on top, muted label below, vertical dividers.
// 数据 model 由调用方传入，每项 { value: string, label: string, tint?: color }；取不到数据时显示占位 "-"。
Rectangle {
    id: root
    Theme { id: th }

    property var model: []

    // 无数据时整体隐藏，避免渲染只有边框的空卡片框。
    visible: root.model && root.model.length > 0
    implicitHeight: row.implicitHeight + th.sp6 * 2
    color: th.card
    radius: th.radiusCard
    border.width: 1
    border.color: th.border

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: th.sp6
        spacing: 0

        Repeater {
            model: root.model
            delegate: Item {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                implicitHeight: col.implicitHeight

                // 列间竖分隔线（首列无前置线）。
                Rectangle {
                    visible: index > 0
                    width: 1
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: th.border
                }

                ColumnLayout {
                    id: col
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: th.sp1

                    QQC.Label {
                        text: (modelData.value && String(modelData.value).length > 0) ? modelData.value : "-"
                        color: modelData.tint || th.textPrimary
                        font.pixelSize: th.fontHeadline
                        font.weight: th.weightSemibold
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: modelData.label || ""
                        color: th.secondaryLabel
                        font.pixelSize: th.fontFootnote
                        horizontalAlignment: Text.AlignHCenter
                        visible: text.length > 0
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}

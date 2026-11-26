import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// 诊断工具卡片（诊断页工具集的统一单元，规范 6.6 / FR-8）。
// Diagnostic tool card — supported tools run for real; unsupported ones are disabled
// with an explicit "此平台暂不支持" note (never fake data).
BaseCard {
    id: root
    Theme { id: th }

    property string toolTitle: ""
    property string toolDescription: ""
    property string toolIcon: ""
    property bool supported: true
    property bool running: false
    property string statusText: ""
    property color statusTint: th.muted
    property string resultText: ""
    signal runRequested()

    function localized(key) { return app.i18n.localize(key); }

    Layout.fillWidth: true
    hoverable: false
    cardPadding: th.sp8
    title: toolTitle
    subtitle: toolDescription

    actions: Component {
        BaseButton {
            text: root.supported ? root.localized("diagnostics.run")
                 : root.localized("diagnostics.unsupported")
            type: root.supported ? "secondary" : "ghost"
            leadingIcon: (root.running || !root.supported) ? "" : "refresh"
            busy: root.running
            enabled: root.supported && !root.running
            onClicked: root.runRequested()
        }
    }

    RowLayout {
        visible: root.statusText.length > 0
        spacing: th.sp2
        Layout.fillWidth: true
        BaseBadge {
            text: root.statusText
            tint: root.statusTint
            iconName: root.running ? "refresh" : (root.statusTint === th.success ? "check" : "info")
        }
    }

    // 结果区（等宽字体，可滚动）
    Rectangle {
        visible: root.resultText.length > 0
        Layout.fillWidth: true
        implicitHeight: Math.min(th.dp(180), Math.max(th.dp(48), resultLabel.implicitHeight + th.sp4 * 2))
        radius: th.radiusControl
        color: th.field
        border.width: 1
        border.color: th.border
        clip: true

        QQC.ScrollView {
            anchors.fill: parent
            QQC.Label {
                id: resultLabel
                text: root.resultText
                color: th.text
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                width: parent ? parent.width - th.sp4 : 0
            }
        }
    }

    QQC.Label {
        visible: !root.supported
        text: root.localized("diagnostics.unsupportedHint")
        color: th.muted
        font.pixelSize: th.fontCaption
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
}

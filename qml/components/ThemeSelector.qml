import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// 外观主题选择器：跟随系统 / 浅色 / 深色，三态持久化到配置。
// Appearance selector: follow system / light / dark; persisted to config.
QQC.ComboBox {
    id: control
    Theme { id: th }

    // 0 system, 1 light, 2 dark —— 与 ConfigStore::themeMode 对齐。
    property var modes: [{ "value": 0 }, { "value": 1 }, { "value": 2 }]
    model: modes

    function label(v) {
        if (v === 1) return app.i18n.text("theme.modeLight");
        if (v === 2) return app.i18n.text("theme.modeDark");
        return app.i18n.text("theme.modeSystem");
    }

    currentIndex: {
        for (var i = 0; i < modes.length; ++i) {
            if (modes[i].value === app.config.themeMode) return i;
        }
        return 0;
    }
    displayText: currentIndex >= 0 && currentIndex < modes.length
                 ? label(modes[currentIndex].value) : ""

    implicitHeight: th.touchTarget
    implicitWidth: 148
    leftPadding: 12
    rightPadding: 34
    topPadding: 0
    bottomPadding: 0
    font.pixelSize: th.fontSubhead
    font.weight: th.weightMedium
    hoverEnabled: true
    onActivated: function(index) {
        if (index >= 0 && index < modes.length)
            app.config.themeMode = modes[index].value;
    }

    background: Rectangle {
        radius: th.radiusSmall
        color: !control.enabled ? th.disabledBackground
             : control.pressed ? th.controlPressed
             : control.hovered ? th.controlHover : th.field
        border.width: 1
        border.color: control.activeFocus ? th.primary : th.separator
        Behavior on color { ColorAnimation { duration: th.durationFast } }
    }

    contentItem: RowLayout {
        spacing: 7
        QQC.Label {
            text: control.displayText
            color: control.enabled ? th.text : th.disabledText
            font: control.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
    }

    indicator: Icon {
        name: "chevronDown"
        iconSize: 12
        tint: th.muted
        x: control.width - width - 12
        y: (control.height - height) / 2
        rotation: control.pressed ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: th.durationFast } }
    }

    delegate: QQC.ItemDelegate {
        required property var modelData
        required property int index
        width: ListView.view ? ListView.view.width : control.width
        height: th.touchTarget
        highlighted: control.highlightedIndex === index
        hoverEnabled: true

        background: Rectangle {
            radius: th.radiusSmall
            color: control.highlightedIndex === index || control.currentIndex === index
                   ? th.greenFill : parent.hovered ? th.controlHover : "transparent"
        }
        contentItem: QQC.Label {
            text: control.label(modelData.value)
            color: control.currentIndex === index ? th.primary : th.text
            font.pixelSize: th.fontSubhead
            font.weight: control.currentIndex === index ? th.weightSemibold : th.weightRegular
            leftPadding: 10
            rightPadding: 10
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    popup: QQC.Popup {
        id: popup
        width: Math.max(120, Math.min(Math.max(control.width, 156), control.Window.width - th.sp8))
        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                         Math.max(th.touchTarget * 2, control.Window.height - th.sp8), 300)
        padding: th.sp2
        x: {
            var origin = control.mapToItem(null, 0, 0);
            var desired = origin.x + control.width - width;
            var clamped = Math.max(th.sp2, Math.min(desired, control.Window.width - width - th.sp2));
            return clamped - origin.x;
        }
        y: {
            var origin = control.mapToItem(null, 0, 0);
            var below = origin.y + control.height + th.sp2;
            var above = origin.y - height - th.sp2;
            var desired = control.Window.height - below >= Math.min(height, 176) ? below : above;
            return Math.max(th.sp2, Math.min(desired, control.Window.height - height - th.sp2)) - origin.y;
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            QQC.ScrollIndicator.vertical: QQC.ScrollIndicator {}
        }

        background: Rectangle {
            radius: th.radius
            color: th.surfaceElevated
            border.width: 1
            border.color: th.separator
        }

        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: th.durationFast } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: th.durationFast } }
    }
}

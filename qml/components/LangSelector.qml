import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

QQC.ComboBox {
    id: control
    Theme { id: th }

    property var languages: app.i18n.languages
    model: languages
    currentIndex: {
        for (var i = 0; i < languages.length; ++i) {
            if (languages[i].value === app.i18n.lang) return i;
        }
        return 0;
    }
    displayText: currentIndex >= 0 && currentIndex < languages.length
                 ? languages[currentIndex].name : ""

    implicitHeight: th.touchTarget
    implicitWidth: th.dp(160)
    leftPadding: th.sp4
    rightPadding: th.sp6
    topPadding: 0
    bottomPadding: 0
    font.pixelSize: th.fontSubhead
    font.weight: th.weightMedium
    hoverEnabled: true
    onActivated: function(index) {
        if (index >= 0 && index < languages.length)
            app.i18n.setLang(languages[index].value);
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
        spacing: th.sp2
        Icon {
            name: "globe"
            iconSize: th.dp(16)
            tint: th.primary
            Layout.alignment: Qt.AlignVCenter
        }
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
        iconSize: th.dp(12)
        tint: th.muted
        x: control.width - width - th.sp4
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
                   ? th.primaryFill : parent.hovered ? th.controlHover : "transparent"
        }
        contentItem: QQC.Label {
            text: modelData.name
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
            QQC.ScrollIndicator.vertical: QQC.ScrollIndicator { }
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

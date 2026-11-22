import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}
    readonly property var encodingModel: ["ASCII", "GBK", "UTF-8"]

    title: "section.advanced"
    showBack: true

    component NumericField: ColumnLayout {
        id: numericRoot

        property string label: ""
        property int value: 0
        property int minimum: 0
        property int maximum: 0
        signal submitted(int newValue)

        spacing: th.sp3

        QQC.Label {
            text: numericRoot.label
            color: th.muted
            font.pixelSize: th.fontFootnote
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
        TextField {
            text: numericRoot.value
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: numericRoot.minimum
                top: numericRoot.maximum
            }
            Layout.fillWidth: true
            onEditingFinished: {
                if (acceptableInput)
                    numericRoot.submitted(Number(text));
                else
                    text = numericRoot.value;
            }
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        Card {
            Layout.fillWidth: true

            Column {
                width: parent.width
                spacing: th.sp3

                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("label.requestEncoding"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                }
                QQC.ComboBox {
                    id: encodingCombo
                    width: parent.width
                    implicitHeight: th.inputHeight
                    model: root.encodingModel
                    currentIndex: {
                        var index = root.encodingModel.indexOf(String(app.config.requestEncoding).toUpperCase());
                        return index >= 0 ? index : 2;
                    }
                    displayText: currentIndex >= 0 ? root.encodingModel[currentIndex] : ""
                    leftPadding: 14
                    rightPadding: 40
                    hoverEnabled: true
                    Accessible.name: (app.i18n.lang, app.i18n.text("label.requestEncoding"))
                    onActivated: function(index) {
                        app.config.requestEncoding = root.encodingModel[index];
                    }

                    contentItem: QQC.Label {
                        text: encodingCombo.displayText
                        color: th.text
                        font.pixelSize: th.fontBody
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    indicator: Icon {
                        name: "chevronDown"
                        iconSize: 13
                        tint: th.muted
                        x: encodingCombo.width - width - 14
                        y: (encodingCombo.height - height) / 2
                        rotation: encodingCombo.pressed ? 180 : 0
                        Behavior on rotation { NumberAnimation { duration: th.durationFast } }
                    }
                    background: Rectangle {
                        radius: th.radiusSmall
                        color: encodingCombo.pressed ? th.controlPressed
                             : encodingCombo.hovered ? th.controlHover : th.field
                        border.width: 1
                        border.color: encodingCombo.activeFocus ? th.primary : th.separator
                    }
                    delegate: QQC.ItemDelegate {
                        required property var modelData
                        required property int index
                        width: ListView.view ? ListView.view.width : encodingCombo.width
                        height: th.touchTarget
                        highlighted: encodingCombo.highlightedIndex === index
                        hoverEnabled: true
                        contentItem: QQC.Label {
                            text: modelData
                            color: encodingCombo.currentIndex === index ? th.primary : th.text
                            font.pixelSize: th.fontBody
                            leftPadding: 10
                            rightPadding: 10
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        background: Rectangle {
                            radius: th.radiusSmall
                            color: highlighted || encodingCombo.currentIndex === index
                                   ? th.blueFill : parent.hovered ? th.controlHover : "transparent"
                        }
                    }
                    popup: QQC.Popup {
                        id: encodingPopup
                        width: Math.min(encodingCombo.width,
                                        Math.max(0, encodingCombo.Window.width - th.sp8))
                        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                                         Math.max(th.touchTarget * 2,
                                                  encodingCombo.Window.height - th.sp8))
                        padding: th.sp2
                        x: {
                            var origin = encodingCombo.mapToItem(null, 0, 0);
                            var desired = origin.x;
                            var clamped = Math.max(th.sp2, Math.min(desired,
                                                   encodingCombo.Window.width - width - th.sp2));
                            return clamped - origin.x;
                        }
                        y: {
                            var origin = encodingCombo.mapToItem(null, 0, 0);
                            var below = origin.y + encodingCombo.height + th.sp2;
                            var above = origin.y - height - th.sp2;
                            var desired = encodingCombo.Window.height - below >= height ? below : above;
                            return Math.max(th.sp2, Math.min(desired,
                                            encodingCombo.Window.height - height - th.sp2)) - origin.y;
                        }
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: encodingCombo.delegateModel
                            currentIndex: encodingCombo.highlightedIndex
                            boundsBehavior: Flickable.StopAtBounds
                            QQC.ScrollIndicator.vertical: QQC.ScrollIndicator {}
                        }
                        background: Rectangle {
                            radius: th.radius
                            color: th.surfaceElevated
                            border.width: 1
                            border.color: th.separator
                        }
                        enter: Transition {
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: th.durationFast }
                        }
                        exit: Transition {
                            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: th.durationFast }
                        }
                    }
                }
                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("help.requestEncoding"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                }
            }

            Rectangle {
                width: parent.width
                implicitHeight: insecureRow.implicitHeight
                radius: th.radiusSmall
                color: th.field
                border.width: 1
                border.color: th.separator

                SettingRow {
                    id: insecureRow
                    width: parent.width
                    title: (app.i18n.lang, app.i18n.text("action.insecureSsl"))
                    Switch {
                        checked: app.config.allowInsecureSsl
                        onToggled: app.config.allowInsecureSsl = checked
                    }
                }
            }
        }

        Card {
            Layout.fillWidth: true

            GridLayout {
                width: parent.width
                columns: width >= th.compactBreakpoint ? 2 : 1
                columnSpacing: th.sp6
                rowSpacing: th.sp6

                NumericField {
                    label: (app.i18n.lang, app.i18n.text("advanced.minInterval"))
                    value: app.config.keepaliveMinSec
                    minimum: 30
                    maximum: 600
                    Layout.fillWidth: true
                    onSubmitted: function(newValue) { app.config.keepaliveMinSec = newValue; }
                }
                NumericField {
                    label: (app.i18n.lang, app.i18n.text("advanced.maxInterval"))
                    value: app.config.keepaliveMaxSec
                    minimum: 30
                    maximum: 600
                    Layout.fillWidth: true
                    onSubmitted: function(newValue) { app.config.keepaliveMaxSec = newValue; }
                }
                NumericField {
                    label: (app.i18n.lang, app.i18n.text("advanced.failThreshold"))
                    value: app.config.offlineFailThreshold
                    minimum: 1
                    maximum: 10
                    Layout.fillWidth: true
                    onSubmitted: function(newValue) { app.config.offlineFailThreshold = newValue; }
                }
                NumericField {
                    label: (app.i18n.lang, app.i18n.text("advanced.connectTimeout"))
                    value: app.config.connectTimeoutMs
                    minimum: 1000
                    maximum: 60000
                    Layout.fillWidth: true
                    onSubmitted: function(newValue) { app.config.connectTimeoutMs = newValue; }
                }
                NumericField {
                    label: (app.i18n.lang, app.i18n.text("advanced.probeTimeout"))
                    value: app.config.probeTimeoutMs
                    minimum: 1000
                    maximum: 60000
                    Layout.fillWidth: true
                    onSubmitted: function(newValue) { app.config.probeTimeoutMs = newValue; }
                }
            }

            Column {
                width: parent.width
                spacing: th.sp3
                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("advanced.probeUrl"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                }
                TextField {
                    width: parent.width
                    text: app.config.probeUrl
                    onEditingFinished: app.config.probeUrl = text
                }
            }

            Column {
                width: parent.width
                spacing: th.sp3
                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("advanced.healthUrl"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                }
                TextField {
                    width: parent.width
                    text: app.config.healthUrl
                    onEditingFinished: app.config.healthUrl = text
                }
            }
        }

        GridLayout {
            columns: width >= th.compactBreakpoint ? 2 : 1
            columnSpacing: th.sp4
            rowSpacing: th.sp4
            Layout.fillWidth: true

            PrimaryButton {
                text: (app.i18n.lang, app.i18n.text("btn.save"))
                variant: "primary"
                leadingIcon: "check"
                Layout.fillWidth: true
                onClicked: {
                    if (app.session.requireLogin("settings.save")) {
                        saveFeedback.saved = app.config.save();
                        saveFeedback.visible = true;
                    }
                }
            }
            StatusBadge {
                id: saveFeedback
                property bool saved: true
                text: (app.i18n.lang, app.i18n.text(saved ? "config.saved" : "config.saveFailed"))
                iconName: saved ? "check" : "info"
                tint: saved ? th.success : th.danger
                visible: false
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }
        }

        ColumnLayout {
            spacing: th.sp4
            Layout.fillWidth: true

            PrimaryButton {
                text: (app.i18n.lang, app.i18n.text("action.resetDefaults"))
                variant: "danger"
                bg: "transparent"
                leadingIcon: "refresh"
                Layout.fillWidth: true
                onClicked: resetDialog.open()
            }
            StatusBadge {
                id: resetFeedback
                property bool saved: true
                text: (app.i18n.lang, app.i18n.text(saved ? "config.resetDone" : "config.resetFailed"))
                iconName: saved ? "check" : "info"
                tint: saved ? th.success : th.danger
                visible: false
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }
        }
    }

    property var resetDialog: AppDialog {
        title: (app.i18n.lang, app.i18n.text("config.resetTitle"))
        message: (app.i18n.lang, app.i18n.text("config.resetMessage"))
        confirmText: (app.i18n.lang, app.i18n.text("action.resetDefaults"))
        cancelText: (app.i18n.lang, app.i18n.text("action.cancel"))
        showCancel: true
        destructive: true
        onAccepted: {
            resetFeedback.saved = app.config.resetToDefaults();
            // 复位后界面语言一并回到默认（简体中文）。
            app.i18n.setLang(0);
            resetFeedback.visible = true;
        }
    }
}

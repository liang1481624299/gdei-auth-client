import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}
    readonly property var encodingModel: ["AUTO", "ASCII", "GBK", "UTF-8"]

    // 本地暂存：编辑过程不直接写配置，保存时统一校验通过后再批量落盘。
    // Local draft: edits are validated together on save before persisting.
    property int minSec: app.config.keepaliveMinSec
    property int maxSec: app.config.keepaliveMaxSec
    property int failThreshold: app.config.offlineFailThreshold
    property int connectTimeout: app.config.connectTimeoutMs
    property int probeTimeout: app.config.probeTimeoutMs
    property string probeUrl: app.config.probeUrl
    property string healthUrl: app.config.healthUrl

    title: "section.advanced"
    showBack: true

    function localized(key) {
        app.i18n.lang;
        return app.i18n.text(key);
    }

    function encodingDisplay(index) {
        if (index < 0 || index >= encodingModel.length) return "";
        return encodingModel[index] === "AUTO" ? localized("encoding.auto") : encodingModel[index];
    }

    function isValidUrl(u) {
        var s = String(u).trimmed();
        return /^https?:\/\/.+/i.test(s);
    }

    // 返回空串表示通过；否则返回错误文案键。
    // Returns an empty string when valid, otherwise an error message key.
    function validate(): string {
        if (minSec <= 0 || maxSec <= 0 || failThreshold <= 0 ||
            connectTimeout <= 0 || probeTimeout <= 0)
            return "adv.errNonPositive";
        if (minSec >= maxSec)
            return "adv.errInterval";
        if (!isValidUrl(probeUrl) || !isValidUrl(healthUrl))
            return "adv.errUrl";
        return "";
    }

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

        // 编码与安全
        DisclosureGroup {
            title: root.localized("adv.groupSecurity")
            leadingIcon: "shield"
            opened: false
            Layout.fillWidth: true

            ColumnLayout {
                spacing: th.sp5
                width: parent.width

                ColumnLayout {
                    spacing: th.sp3
                    width: parent.width

                    QQC.Label {
                        width: parent.width
                        text: root.localized("label.requestEncoding")
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
                            return index >= 0 ? index : 0;
                        }
                        displayText: root.encodingDisplay(currentIndex)
                        leftPadding: 14
                        rightPadding: 40
                        hoverEnabled: true
                        Accessible.name: root.localized("label.requestEncoding")
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
                                text: root.encodingDisplay(index)
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
                                       ? th.greenFill : parent.hovered ? th.controlHover : "transparent"
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
                        text: root.localized("help.requestEncoding")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap
                    }
                }

                Rectangle {
                    width: parent.width
                    implicitHeight: insecureRow.implicitHeight + insecureWarn.implicitHeight + th.sp3
                    radius: th.radiusSmall
                    color: th.field
                    border.width: 1
                    border.color: th.separator

                    ColumnLayout {
                        spacing: th.sp3
                        anchors.fill: parent
                        anchors.margins: th.sp4

                        SettingRow {
                            id: insecureRow
                            width: parent.width
                            title: root.localized("action.insecureSsl")
                            Switch {
                                checked: app.config.allowInsecureSsl
                                onToggled: app.config.allowInsecureSsl = checked
                            }
                        }
                        QQC.Label {
                            id: insecureWarn
                            width: parent.width
                            visible: app.config.allowInsecureSsl
                            text: root.localized("adv.insecureWarn")
                            color: th.danger
                            font.pixelSize: th.fontFootnote
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }
        }

        // 保活策略
        DisclosureGroup {
            title: root.localized("adv.groupKeepalive")
            leadingIcon: "bolt"
            opened: false
            Layout.fillWidth: true

            GridLayout {
                width: parent.width
                columns: width >= th.compactBreakpoint ? 2 : 1
                columnSpacing: th.sp6
                rowSpacing: th.sp6

                NumericField {
                    label: root.localized("advanced.minInterval")
                    value: root.minSec
                    minimum: 30
                    maximum: 600
                    Layout.fillWidth: true
                    onSubmitted: function(v) { root.minSec = v; }
                }
                NumericField {
                    label: root.localized("advanced.maxInterval")
                    value: root.maxSec
                    minimum: 30
                    maximum: 600
                    Layout.fillWidth: true
                    onSubmitted: function(v) { root.maxSec = v; }
                }
                NumericField {
                    label: root.localized("advanced.failThreshold")
                    value: root.failThreshold
                    minimum: 1
                    maximum: 10
                    Layout.fillWidth: true
                    onSubmitted: function(v) { root.failThreshold = v; }
                }
            }
        }

        // 超时参数
        DisclosureGroup {
            title: root.localized("adv.groupTimeout")
            leadingIcon: "clock"
            opened: false
            Layout.fillWidth: true

            GridLayout {
                width: parent.width
                columns: width >= th.compactBreakpoint ? 2 : 1
                columnSpacing: th.sp6
                rowSpacing: th.sp6

                NumericField {
                    label: root.localized("advanced.connectTimeout")
                    value: root.connectTimeout
                    minimum: 1000
                    maximum: 60000
                    Layout.fillWidth: true
                    onSubmitted: function(v) { root.connectTimeout = v; }
                }
                NumericField {
                    label: root.localized("advanced.probeTimeout")
                    value: root.probeTimeout
                    minimum: 1000
                    maximum: 60000
                    Layout.fillWidth: true
                    onSubmitted: function(v) { root.probeTimeout = v; }
                }
            }
        }

        // 探测地址配置
        DisclosureGroup {
            title: root.localized("adv.groupProbe")
            leadingIcon: "globe"
            opened: false
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: th.sp5

                ColumnLayout {
                    spacing: th.sp3
                    width: parent.width
                    QQC.Label {
                        width: parent.width
                        text: root.localized("advanced.probeUrl")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap
                    }
                    TextField {
                        width: parent.width
                        text: root.probeUrl
                        onEditingFinished: root.probeUrl = text
                    }
                    QQC.Label {
                        width: parent.width
                        text: root.localized("adv.probeUrlHint")
                        color: th.tertiaryLabel
                        font.pixelSize: th.fontCaption
                        wrapMode: Text.Wrap
                    }
                }

                ColumnLayout {
                    spacing: th.sp3
                    width: parent.width
                    QQC.Label {
                        width: parent.width
                        text: root.localized("advanced.healthUrl")
                        color: th.muted
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.Wrap
                    }
                    TextField {
                        width: parent.width
                        text: root.healthUrl
                        onEditingFinished: root.healthUrl = text
                    }
                    QQC.Label {
                        width: parent.width
                        text: root.localized("adv.healthUrlHint")
                        color: th.tertiaryLabel
                        font.pixelSize: th.fontCaption
                        wrapMode: Text.Wrap
                    }
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
                    var err = root.validate();
                    if (err.length > 0) {
                        errorDialog.message = root.localized(err);
                        errorDialog.open();
                        return;
                    }
                    if (app.session.requireLogin("settings.save")) {
                        app.config.keepaliveMinSec = root.minSec;
                        app.config.keepaliveMaxSec = root.maxSec;
                        app.config.offlineFailThreshold = root.failThreshold;
                        app.config.connectTimeoutMs = root.connectTimeout;
                        app.config.probeTimeoutMs = root.probeTimeout;
                        app.config.probeUrl = root.probeUrl.trimmed();
                        app.config.healthUrl = root.healthUrl.trimmed();
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

    property var errorDialog: AppDialog {
        title: (app.i18n.lang, app.i18n.text("adv.validationTitle"))
        confirmText: (app.i18n.lang, app.i18n.text("action.ok"))
        showCancel: false
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
            // 复位后同步本地暂存与界面语言（简体中文）。
            root.minSec = app.config.keepaliveMinSec;
            root.maxSec = app.config.keepaliveMaxSec;
            root.failThreshold = app.config.offlineFailThreshold;
            root.connectTimeout = app.config.connectTimeoutMs;
            root.probeTimeout = app.config.probeTimeoutMs;
            root.probeUrl = app.config.probeUrl;
            root.healthUrl = app.config.healthUrl;
            app.i18n.setLang(0);
            resetFeedback.visible = true;
        }
    }
}

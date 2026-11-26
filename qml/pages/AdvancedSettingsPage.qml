import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 高级设置子页（规范 6.5）—— 四组默认折叠，编码默认自动探测，高危项红色警示，可重置默认。
// 前端校验：最小 < 最大间隔、时间 / 次数禁止 0 与负数、URL 格式校验。
// All logic preserved; visuals migrated to Base* components for dark-theme correctness.
PageScaffold {
    id: root
    Theme { id: th }

    // 编码选项：对象数组的 name 用于显示（AUTO 走多语言），value 写入配置。
    // 引用 app.i18n.lang 使语言切换时标签自动刷新。
    readonly property var encodingItems: [
        { "name": localized("encoding.auto"), "value": "AUTO" },
        { "name": "ASCII", "value": "ASCII" },
        { "name": "GBK", "value": "GBK" },
        { "name": "UTF-8", "value": "UTF-8" }
    ]

    // 本地暂存：编辑过程不直接写配置，保存时统一校验通过后再批量落盘。
    property int minSec: app.config.keepaliveMinSec
    property int maxSec: app.config.keepaliveMaxSec
    property int failThreshold: app.config.offlineFailThreshold
    property int connectTimeout: app.config.connectTimeoutMs
    property int probeTimeout: app.config.probeTimeoutMs
    property string probeUrl: app.config.probeUrl
    property string healthUrl: app.config.healthUrl
    // 编码选择同样先暂存，保存时经门面统一落盘。
    property string requestEncoding: app.config.requestEncoding
    // 证书导入：路径与结果码（-1 未操作；0 成功；1 读取失败；2 无效证书；3 写入失败）。
    property string certPath: ""
    property int certImportCode: -1

    title: "section.advanced"
    showBack: true
    contentMaxWidth: th.dp(720)

    function localized(key) { return app.i18n.dictionary[key]; }

    function isValidUrl(u) {
        var s = String(u).trimmed();
        return /^https?:\/\/.+/i.test(s);
    }

    // 返回空串表示通过；否则返回错误文案键。
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

    // 数字字段：仅接受数字，失焦时夹到合法区间后回写暂存。
    component NumericField: BaseTextField {
        property int value: 0
        property int minimum: 0
        property int maximum: 0
        signal submitted(int newValue)
        text: String(value)
        onEdited: function(t) {
            var digits = String(t).replace(/[^\d]/g, "");
            var n = digits.length > 0 ? parseInt(digits) : 0;
            if (minimum > 0 && n < minimum) n = minimum;
            if (maximum > 0 && n > maximum) n = maximum;
            submitted(n);
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: th.sp8

        // ── 编码与安全 ──
        BaseCollapseGroup {
            title: root.localized("adv.groupSecurity")
            expanded: false

            BaseComboBox {
                Layout.fillWidth: true
                label: root.localized("label.requestEncoding")
                helperText: root.localized("help.requestEncoding")
                model: root.encodingItems
                currentIndex: {
                    var target = String(root.requestEncoding).toUpperCase();
                    for (var i = 0; i < root.encodingItems.length; i++)
                        if (root.encodingItems[i].value === target) return i;
                    return 0;
                }
                onActivated: function(index) {
                    root.requestEncoding = root.encodingItems[index].value;
                }
            }

            // 跳过证书校验：高危项，红色风险提示 + 开启前二次确认。
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: insecureRow.implicitHeight + (insecureWarn.visible ? insecureWarn.implicitHeight + th.sp3 : 0) + th.sp4 * 2
                radius: th.radiusControl
                color: th.field
                border.width: 1
                border.color: th.border

                ColumnLayout {
                    spacing: th.sp3
                    anchors.fill: parent
                    anchors.margins: th.sp4

                    RowLayout {
                        id: insecureRow
                        Layout.fillWidth: true
                        spacing: th.sp4
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            QQC.Label {
                                text: root.localized("action.insecureSsl")
                                color: th.text
                                font.pixelSize: th.fontBody
                                font.weight: th.weightMedium
                                Layout.fillWidth: true
                            }
                        }
                        BaseSwitch {
                            checked: app.config.allowInsecureSsl
                            onToggled: function(checked) {
                                if (checked) {
                                    // 先复位开关，待弹窗确认后再写入。
                                    this.checked = false;
                                    insecureDialog.open();
                                } else {
                                    app.setSkipCertificateCheck(false);
                                }
                            }
                        }
                    }
                    QQC.Label {
                        id: insecureWarn
                        visible: app.config.allowInsecureSsl
                        text: root.localized("adv.insecureWarn")
                        color: th.danger
                        font.pixelSize: th.fontFootnote
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // ── 证书信任（仅追加信任锚，不关闭校验）──
        BaseCollapseGroup {
            title: root.localized("cert.title")
            expanded: false

            QQC.Label {
                text: root.localized("cert.hint")
                color: th.secondaryLabel
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            // 已导入证书列表：主体 / 有效期 / 指纹 / 删除。
            Repeater {
                model: app.certificates.certificates
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: certRow.implicitHeight + th.sp3 * 2
                    radius: th.radiusControl
                    color: th.field
                    border.width: 1
                    border.color: th.border

                    ColumnLayout {
                        id: certRow
                        anchors.fill: parent
                        anchors.margins: th.sp3
                        spacing: th.sp1

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: th.sp3
                            QQC.Label {
                                text: modelData.subject
                                color: th.text
                                font.pixelSize: th.fontSubhead
                                font.weight: th.weightMedium
                                wrapMode: Text.WrapAnywhere
                                Layout.fillWidth: true
                            }
                            BaseBadge {
                                visible: modelData.expired === true
                                text: root.localized("cert.expired")
                                iconName: "info"
                                tint: th.danger
                            }
                            BaseButton {
                                text: root.localized("cert.delete")
                                type: "danger"
                                onClicked: app.certificates.removeByFingerprint(modelData.fingerprint)
                            }
                        }
                        QQC.Label {
                            text: root.localized("cert.expires").replace("%1", modelData.notAfter)
                            color: modelData.expired === true ? th.danger : th.muted
                            font.pixelSize: th.fontFootnote
                            wrapMode: Text.WrapAnywhere
                            Layout.fillWidth: true
                        }
                        QQC.Label {
                            text: root.localized("cert.fingerprint") + ": " + modelData.fingerprint
                            color: th.muted
                            font.pixelSize: th.fontFootnote
                            font.family: "monospace"
                            wrapMode: Text.WrapAnywhere
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            QQC.Label {
                text: root.localized("cert.empty")
                color: th.tertiaryLabel
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                visible: app.certificates.certificates.length === 0
            }

            // 导入：路径输入 + 导入按钮，结果按返回码反馈。
            RowLayout {
                Layout.fillWidth: true
                spacing: th.sp4
                BaseTextField {
                    Layout.fillWidth: true
                    placeholderText: root.localized("cert.pathPlaceholder")
                    text: root.certPath
                    onEdited: function(t) { root.certPath = t; }
                }
                BaseButton {
                    text: root.localized("cert.import")
                    type: "secondary"
                    leadingIcon: "plus"
                    onClicked: {
                        if (root.certPath.trimmed().length === 0) return;
                        root.certImportCode = app.certificates.importFromPath(root.certPath.trimmed());
                    }
                }
            }
            QQC.Label {
                visible: root.certImportCode >= 0
                text: {
                    switch (root.certImportCode) {
                    case 0: return root.localized("cert.importOk");
                    case 1: return root.localized("cert.importReadFailed");
                    case 2: return root.localized("cert.importInvalid");
                    case 3: return root.localized("cert.importWriteFailed");
                    default: return "";
                    }
                }
                color: root.certImportCode === 0 ? th.success : th.danger
                font.pixelSize: th.fontFootnote
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        // ── 保活策略 ──
        BaseCollapseGroup {
            title: root.localized("adv.groupKeepalive")
            expanded: false

            GridLayout {
                Layout.fillWidth: true
                columns: width >= th.compactBreakpoint ? 2 : 1
                columnSpacing: th.sp6
                rowSpacing: th.sp6

                NumericField {
                    Layout.fillWidth: true
                    label: root.localized("advanced.minInterval")
                    helperText: root.localized("help.minInterval")
                    value: root.minSec
                    minimum: 30
                    maximum: 600
                    onSubmitted: function(v) { root.minSec = v; }
                }
                NumericField {
                    Layout.fillWidth: true
                    label: root.localized("advanced.maxInterval")
                    helperText: root.localized("help.maxInterval")
                    value: root.maxSec
                    minimum: 30
                    maximum: 600
                    onSubmitted: function(v) { root.maxSec = v; }
                }
                NumericField {
                    Layout.fillWidth: true
                    label: root.localized("advanced.failThreshold")
                    helperText: root.localized("help.failThreshold")
                    value: root.failThreshold
                    minimum: 1
                    maximum: 10
                    onSubmitted: function(v) { root.failThreshold = v; }
                }
            }
        }

        // ── 超时参数 ──
        BaseCollapseGroup {
            title: root.localized("adv.groupTimeout")
            expanded: false

            GridLayout {
                Layout.fillWidth: true
                columns: width >= th.compactBreakpoint ? 2 : 1
                columnSpacing: th.sp6
                rowSpacing: th.sp6

                NumericField {
                    Layout.fillWidth: true
                    label: root.localized("advanced.connectTimeout")
                    helperText: root.localized("help.connectTimeout")
                    value: root.connectTimeout
                    minimum: 1000
                    maximum: 60000
                    onSubmitted: function(v) { root.connectTimeout = v; }
                }
                NumericField {
                    Layout.fillWidth: true
                    label: root.localized("advanced.probeTimeout")
                    helperText: root.localized("help.probeTimeout")
                    value: root.probeTimeout
                    minimum: 1000
                    maximum: 60000
                    onSubmitted: function(v) { root.probeTimeout = v; }
                }
            }
        }

        // ── 探测地址配置 ──
        BaseCollapseGroup {
            title: root.localized("adv.groupProbe")
            expanded: false

            BaseTextField {
                Layout.fillWidth: true
                label: root.localized("advanced.probeUrl")
                helperText: root.localized("adv.probeUrlHint")
                text: root.probeUrl
                onEdited: function(t) { root.probeUrl = t; }
            }
            BaseTextField {
                Layout.fillWidth: true
                label: root.localized("advanced.healthUrl")
                helperText: root.localized("adv.healthUrlHint")
                text: root.healthUrl
                onEdited: function(t) { root.healthUrl = t; }
            }
        }

        // ── 保存 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: th.sp4
            BaseButton {
                text: root.localized("btn.save")
                type: "primary"
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
                        saveFeedback.saved = app.saveAdvancedSettings({
                            "keepaliveMinSec": root.minSec,
                            "keepaliveMaxSec": root.maxSec,
                            "offlineFailThreshold": root.failThreshold,
                            "connectTimeoutMs": root.connectTimeout,
                            "probeTimeoutMs": root.probeTimeout,
                            "probeUrl": root.probeUrl.trimmed(),
                            "healthUrl": root.healthUrl.trimmed(),
                            "requestEncoding": root.requestEncoding
                        });
                        saveFeedback.visible = true;
                    }
                }
            }
            BaseBadge {
                id: saveFeedback
                property bool saved: true
                text: root.localized(saved ? "config.saved" : "config.saveFailed")
                iconName: saved ? "check" : "info"
                tint: saved ? th.success : th.danger
                visible: false
            }
        }

        // ── 重置为默认配置（高危，二次确认）──
        BaseButton {
            text: root.localized("action.resetDefaults")
            type: "danger"
            leadingIcon: "refresh"
            Layout.fillWidth: true
            onClicked: resetDialog.open()
        }
        BaseBadge {
            id: resetFeedback
            property bool saved: true
            text: root.localized(saved ? "config.resetDone" : "config.resetFailed")
            iconName: saved ? "check" : "info"
            tint: saved ? th.success : th.danger
            visible: false
        }
    }

    property var errorDialog: AppDialog {
        title: root.localized("adv.validationTitle")
        confirmText: root.localized("action.ok")
        showCancel: false
    }

    property var insecureDialog: AppDialog {
        title: root.localized("adv.insecureConfirmTitle")
        message: root.localized("adv.insecureConfirmMessage")
        confirmText: root.localized("action.ok")
        cancelText: root.localized("action.cancel")
        showCancel: true
        destructive: true
        onAccepted: {
            app.setSkipCertificateCheck(true);
        }
    }

    property var resetDialog: AppDialog {
        title: root.localized("config.resetTitle")
        message: root.localized("config.resetMessage")
        confirmText: root.localized("action.resetDefaults")
        cancelText: root.localized("action.cancel")
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
            root.requestEncoding = app.config.requestEncoding;
            app.i18n.setLang(0);
            resetFeedback.visible = true;
        }
    }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// 本地认证配置 · 添加 / 编辑表单（规范 6.2）。
// Local profile editor — front-end validation, default portal host, encryption notice.
QQC.Dialog {
    id: dialog
    Theme { id: th }

    property string profileId: ""
    // 默认门户主机来自后端配置（ConfigStore 默认常量），前端不写死域名。
    readonly property string defaultHost: app.config.host

    // 前端校验（规范：名称 / 账号非空，门户地址 URL 格式）。非法禁止保存。
    readonly property bool nameOk: nameField.text.trim().length > 0
    readonly property bool userOk: usernameField.text.trim().length > 0
    readonly property bool hostOk: dialog.isHostValid(hostField.text.trim())
    readonly property bool canSave: nameOk && userOk && hostOk

    function normalizeHost(h) {
        if (!h) return "";
        var s = h.trim();
        var si = s.indexOf("://");
        if (si >= 0) s = s.slice(si + 3);
        var sl = s.indexOf("/");  if (sl >= 0) s = s.slice(0, sl);
        var qm = s.indexOf("?");  if (qm >= 0) s = s.slice(0, qm);
        var col = s.lastIndexOf(":"); if (col >= 0) s = s.slice(0, col);
        return s.trim();
    }

    function isHostValid(h) {
        var n = dialog.normalizeHost(h);
        if (!n) return false;
        return /^([a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?)*|\d{1,3}(\.\d{1,3}){3})$/.test(n);
    }

    function localized(key) { return app.i18n.dictionary[key]; }

    parent: QQC.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(th.dp(460), Math.max(0, parent.width - th.sp8))
    height: Math.min(th.dp(640), Math.max(0, parent.height - th.sp8))
    modal: true
    focus: true
    padding: th.sp10
    closePolicy: QQC.Popup.CloseOnEscape
    title: profileId.length > 0 ? localized("profiles.edit") : localized("profiles.add")

    function create() {
        profileId = "";
        nameField.text = "";
        usernameField.text = "";
        passwordField.text = "";
        hostField.text = defaultHost;
        ipField.text = "";
        nasipField.text = "";
        rememberSwitch.checked = false;
        open();
    }

    function edit(id) {
        var profile = app.localDevices.profile(id);
        if (!profile || !profile.id) return;
        profileId = id;
        nameField.text = profile.name || "";
        usernameField.text = profile.username || "";
        passwordField.text = "";
        hostField.text = profile.host || defaultHost;
        ipField.text = profile.ip || "";
        nasipField.text = profile.nasip || "";
        rememberSwitch.checked = profile.rememberPassword === true;
        open();
    }

    function save() {
        if (!canSave) return;
        var saved;
        var hostNorm = dialog.normalizeHost(hostField.text.trim());
        if (profileId.length > 0) {
            saved = app.localDevices.updateProfile(
                        profileId, nameField.text.trim(), usernameField.text.trim(), passwordField.text,
                        hostNorm, ipField.text.trim(), nasipField.text.trim(),
                        rememberSwitch.checked);
        } else {
            saved = app.localDevices.addProfile(
                        nameField.text.trim(), usernameField.text.trim(), passwordField.text,
                        hostNorm, ipField.text.trim(), nasipField.text.trim(),
                        rememberSwitch.checked).length > 0;
        }
        if (saved) accept();
    }

    onOpened: nameField.forceActiveFocus()
    onClosed: { passwordField.text = ""; profileId = ""; }

    QQC.Overlay.modal: Rectangle { color: th.overlay }

    header: QQC.Label {
        text: dialog.title
        color: th.textPrimary
        font.pixelSize: th.fontTitle3
        font.weight: th.weightSemibold
        wrapMode: Text.Wrap
        leftPadding: th.sp2; rightPadding: th.sp2
        topPadding: th.sp2; bottomPadding: th.sp4
    }

    contentItem: QQC.ScrollView {
        id: scroll
        clip: true
        contentWidth: availableWidth
        contentHeight: form.implicitHeight
        QQC.ScrollBar.vertical.policy: QQC.ScrollBar.AsNeeded
        background: Rectangle { color: "transparent" }

        ColumnLayout {
            id: form
            width: Math.max(0, scroll.availableWidth)
            spacing: th.sp5

            BaseTextField {
                id: nameField
                label: localized("profiles.name")
                placeholderText: localized("profiles.namePlaceholder")
                valid: dialog.nameOk
                helperText: dialog.nameOk ? "" : localized("profiles.nameRequired")
                Layout.fillWidth: true
            }
            BaseTextField {
                id: usernameField
                label: localized("label.username")
                placeholderText: localized("placeholder.username")
                valid: dialog.userOk
                helperText: dialog.userOk ? "" : localized("profiles.usernameRequired")
                Layout.fillWidth: true
            }
            BaseTextField {
                id: passwordField
                label: localized("label.password")
                placeholderText: dialog.profileId.length > 0 ? localized("profiles.passwordKeep") : localized("placeholder.password")
                password: true
                Layout.fillWidth: true
            }

            // 加密存储提示（规范：保存前提示密码加密保存在本机）
            RowLayout {
                spacing: th.sp2
                Layout.fillWidth: true
                Icon { name: "lock"; iconSize: th.dp(14); tint: th.secondaryLabel }
                QQC.Label {
                    text: localized("profiles.encryptHint")
                    color: th.secondaryLabel
                    font.pixelSize: th.fontCaption
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            BaseTextField {
                id: hostField
                label: localized("label.host")
                placeholderText: localized("profiles.hostPlaceholder")
                valid: dialog.hostOk
                helperText: dialog.hostOk ? localized("profiles.hostHint") : localized("profiles.hostInvalid")
                Layout.fillWidth: true
            }
            BaseTextField {
                id: ipField
                label: localized("label.ip")
                placeholderText: localized("profiles.ipPlaceholder")
                helperText: localized("profiles.ipHint")
                Layout.fillWidth: true
            }
            BaseTextField {
                id: nasipField
                label: localized("label.nasip")
                placeholderText: localized("profiles.nasipPlaceholder")
                Layout.fillWidth: true
            }

            // 记住密码
            RowLayout {
                Layout.fillWidth: true
                spacing: th.sp4
                QQC.Label {
                    text: localized("action.remember")
                    color: th.text
                    font.pixelSize: th.fontBody
                    Layout.fillWidth: true
                }
                BaseSwitch { id: rememberSwitch }
            }
        }
    }

    footer: RowLayout {
        spacing: th.sp4
        Item { Layout.fillWidth: true }
        BaseButton {
            text: localized("action.cancel"); type: "ghost"
            onClicked: dialog.reject()
        }
        BaseButton {
            text: localized("btn.save"); type: "primary"
            enabled: dialog.canSave
            onClicked: dialog.save()
        }
    }

    background: Rectangle {
        radius: th.radiusCard
        color: th.card
        border.width: 1
        border.color: th.border
    }

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: th.durationNormal } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: th.durationFast } }
}

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

// LocalProfilesCard —— 本机保存的认证配置卡（添加 / 选择 / 编辑 / 删除）。
// Local saved credentials card: add / select / edit / remove local login profiles.
// 自包含：内置配置编辑器与删除二次确认弹窗，直接读写 app.localDevices。
BaseCard {
    id: root
    Theme { id: th }

    property string pendingDeleteId: ""

    function localized(key) { return app.i18n.localize(key); }

    // 组装非空键值明细项。
    function buildFields(entries) {
        var out = [];
        for (var i = 0; i < entries.length; i++) {
            var e = entries[i];
            if (e.value && String(e.value).length > 0)
                out.push({ label: e.label, value: String(e.value) });
        }
        return out;
    }

    function editProfile(id) { profileEditor.edit(id); }
    function requestDelete(id) { pendingDeleteId = id; deleteDialog.open(); }

    title: root.localized("profiles.localGroup")
    subtitle: root.localized("profiles.localGroupHint")
    actions: Component {
        BaseButton {
            text: root.localized("profiles.add")
            type: "primary"
            leadingIcon: "plus"
            onClicked: profileEditor.create()
        }
    }

    QQC.Label {
        visible: app.localDevices.count === 0
        text: root.localized("profiles.empty")
        color: th.muted
        font.pixelSize: th.fontBody
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: th.sp4

        Repeater {
            model: app.localDevices.model
            delegate: DeviceCard {
                id: card
                required property var model
                Layout.fillWidth: true
                title: model.name
                subtitle: root.localized("label.username") + " · " +
                         (model.username && model.username.length > 0 ? model.username
                              : root.localized("common.unavailable"))
                fallbackIcon: "server"
                selected: app.localDevices.selectedId === model.id
                statusText: card.selected ? root.localized("profiles.selected") : ""
                statusColor: th.primary
                fields: root.buildFields([
                    { label: root.localized("label.host"), value: model.host },
                    { label: root.localized("label.ip"), value: model.ip },
                    { label: root.localized("label.nasip"), value: model.nasip }
                ])

                BaseButton {
                    text: root.localized("profiles.select")
                    type: "secondary"
                    leadingIcon: "check"
                    enabled: !card.selected
                    Layout.fillWidth: true
                    onClicked: app.localDevices.selectProfile(model.id)
                }
                BaseButton {
                    text: root.localized("profiles.edit")
                    type: "secondary"
                    leadingIcon: "cog"
                    Layout.fillWidth: true
                    onClicked: root.editProfile(model.id)
                }
                BaseButton {
                    text: root.localized("profiles.delete")
                    type: "danger-ghost"
                    leadingIcon: "trash"
                    Layout.fillWidth: true
                    onClicked: root.requestDelete(model.id)
                }
            }
        }
    }

    LocalDeviceEditor { id: profileEditor }

    AppDialog {
        id: deleteDialog
        title: root.localized("profiles.deleteTitle")
        message: root.localized("profiles.deleteMessage")
        confirmText: root.localized("action.delete")
        cancelText: root.localized("action.cancel")
        showCancel: true
        destructive: true
        onAccepted: {
            if (root.pendingDeleteId.length > 0)
                app.localDevices.removeProfile(root.pendingDeleteId);
            root.pendingDeleteId = "";
        }
        onRejected: root.pendingDeleteId = ""
    }
}

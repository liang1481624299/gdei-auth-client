import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 设备管理页（规范 6.2）—— 仅保留校园网在线设备（只读）；本机认证配置已迁入首页「本地配置卡」。
// 卡片样式参考设备台：品牌图标 + 设备名 + 状态点，两列键值明细；云端只读无操作行。
// Devices page — portal online devices (read-only) only; local profiles live on Home.
PageScaffold {
    id: page
    Theme { id: th }

    signal openRemoteRequested()

    title: "profiles.title"
    subtitle: "profiles.subtitle"
    showBack: false
    contentMaxWidth: th.dp(720)

    function localized(key) { return app.i18n.localize(key); }
    function normalizeMac(m) { return (m || "").toLowerCase().replace(/[^a-f0-9]/g, ""); }
    readonly property string localMacNorm: page.normalizeMac(app.network.localMac)

    // 进入远程设备子页：受保护操作走会话门禁，已登录则直接跳转（门户验证码在子页处理）。
    function openRemote() {
        if (app.session.requireLogin("remote.open"))
            page.openRemoteRequested();
    }

    onVisibleChanged: {
        // 已登录但尚未拉取设备时，进入页面自动刷新一次。
        if (visible && app.portalDevices.authenticated && app.portalDevices.devices.length === 0)
            app.portalDevices.refreshDevices();
    }

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

    // 云端设备状态色：在线绿、绑定蓝、历史灰。
    function cloudStatusColor(cat) {
        if (cat === "history") return th.muted;
        if (cat === "bound") return th.primary;
        return th.success;
    }
    function cloudStatusText(cat) {
        if (cat === "history") return page.localized("devices.cat.history");
        if (cat === "bound") return page.localized("devices.cat.bound");
        return page.localized("devices.cat.online");
    }

    // ── 校园网在线设备（只读）──
    BaseCard {
        Layout.fillWidth: true
        title: page.localized("profiles.cloudGroup")
        subtitle: page.localized("profiles.cloudGroupHint")
        actions: Component {
            BaseButton {
                visible: app.portalDevices.authenticated
                text: page.localized("devices.refresh")
                type: "secondary"
                leadingIcon: "refresh"
                busy: app.portalDevices.busy
                enabled: app.portalDevices.authenticated && !app.portalDevices.busy
                onClicked: app.portalDevices.refreshDevices()
            }
        }

        // 未登录：引导进入门户登录（子页处理验证码）
        NavEntryRow {
            Layout.fillWidth: true
            visible: !app.portalDevices.authenticated
            title: page.localized("profiles.cloudLogin")
            subtitle: page.localized("profiles.cloudLoginHint")
            iconName: "shield"
            onClicked: page.openRemote()
        }

        // 已登录：内嵌只读设备列表（在线置顶由后端解析顺序保证）
        Item {
            visible: app.portalDevices.authenticated
            Layout.fillWidth: true
            implicitHeight: skeletonCol.visible ? skeletonCol.implicitHeight : portalList.contentHeight
            ListView {
                id: portalList
                anchors.fill: parent
                model: app.portalDevices.devices
                interactive: false
                clip: true
                spacing: th.sp4
                boundsBehavior: Flickable.StopAtBounds
                delegate: portalCardComp
            }
            // 加载骨架：首次拉取且暂无条目时占位，避免把“加载中”误读成空列表。
            ColumnLayout {
                id: skeletonCol
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: th.sp4
                visible: app.portalDevices.busy && portalList.count === 0
                BaseSkeleton { Layout.fillWidth: true; height: th.dp(88) }
                BaseSkeleton { Layout.fillWidth: true; height: th.dp(88) }
            }
            QQC.Label {
                anchors.centerIn: parent
                width: parent.width - th.sp8
                text: page.localized("devices.empty")
                color: th.tertiaryLabel
                font.pixelSize: th.fontSubhead
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: app.portalDevices.authenticated && portalList.count === 0 && !app.portalDevices.busy
            }
        }

        // 查看全部线上设备 → 远程设备子页
        NavEntryRow {
            Layout.fillWidth: true
            visible: app.portalDevices.authenticated && app.portalDevices.devices.length > 0
            title: page.localized("profiles.cloudViewAll")
            iconName: "server"
            onClicked: page.openRemoteRequested()
        }

        // 只读说明
        RowLayout {
            spacing: th.sp2
            Layout.fillWidth: true
            Icon { name: "info"; iconSize: th.dp(14); tint: th.muted }
            QQC.Label {
                text: page.localized("profiles.readOnly")
                color: th.muted
                font.pixelSize: th.fontCaption
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        // 拉取失败 / 状态消息（展示原因，无静态「暂无法获取」）
        QQC.Label {
            text: app.portalDevices.lastMessage.length > 0
                  ? page.localized(app.portalDevices.lastMessage) : ""
            color: th.secondaryLabel
            font.pixelSize: th.fontFootnote
            wrapMode: Text.Wrap
            visible: text.length > 0
            Layout.fillWidth: true
        }
        // 失败内联重试：与余额卡一致，给出可操作的下一步。
        BaseButton {
            text: page.localized("action.retry")
            type: "secondary"
            leadingIcon: "refresh"
            busy: app.portalDevices.busy
            enabled: app.portalDevices.authenticated && !app.portalDevices.busy
            visible: app.portalDevices.lastMessage.length > 0
            Layout.alignment: Qt.AlignLeft
            onClicked: app.portalDevices.refreshDevices()
        }
    }

    // 云端只读设备卡片（内嵌 delegate）
    Component {
        id: portalCardComp
        DeviceCard {
            id: card
            required property var modelData
            readonly property bool isLocal: page.normalizeMac(modelData.mac) === page.localMacNorm
            readonly property string cat: modelData.category || "online"

            title: (modelData.deviceType && modelData.deviceType.length > 0)
                   ? modelData.deviceType : page.localized("devices.unknownName")
            subtitle: card.isLocal ? page.localized("devices.thisDevice") : ""
            brandText: (modelData.deviceType || "") + " " + (modelData.os || "")
            fallbackIcon: "server"
            interactive: false
            statusColor: page.cloudStatusColor(card.cat)
            statusText: page.cloudStatusText(card.cat)
            fields: page.buildFields([
                { label: page.localized("label.mac"), value: modelData.mac },
                { label: page.localized("label.ip"), value: modelData.ip },
                { label: page.localized("label.os"), value: modelData.os },
                { label: page.localized("label.duration"), value: modelData.duration },
                { label: page.localized("label.status"), value: modelData.lostStatus }
            ])
        }
    }
}

import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "theme"
import "components"

Window {
    id: root
    Theme { id: th }

    width: Math.min(1040, Screen.desktopAvailableWidth,
                    Math.max(320, Math.round(Screen.desktopAvailableWidth * 0.90)))
    height: Math.min(860, Screen.desktopAvailableHeight,
                     Math.max(400, Math.round(Screen.desktopAvailableHeight * 0.86)))
    minimumWidth: Math.min(320, Screen.desktopAvailableWidth)
    minimumHeight: Math.min(400, Screen.desktopAvailableHeight)
    visible: true
    color: th.window
    title: (app.i18n.lang, app.i18n.text("app.title"))

    readonly property bool compact: width < th.compactBreakpoint
    readonly property bool wide: width >= th.wideBreakpoint
    readonly property int pagePadding: compact ? th.sp6 : th.sp12
    readonly property var encodingModel: ["ASCII", "GBK", "UTF-8"]
    readonly property string statusText: {
        app.i18n.lang;
        if (app.auth.state === 3) return app.i18n.text("status.online");
        if (app.auth.state === 2) return app.i18n.text("status.authenticating");
        if (app.auth.state === 4) return app.i18n.text("login.failed");
        return app.i18n.text("status.offline");
    }
    readonly property color statusTint: app.auth.state === 3 ? th.success
                                      : app.auth.state === 2 ? th.warning
                                      : app.auth.state === 4 ? th.danger : th.muted
    readonly property string statusIconName: app.auth.state === 3 ? "wifi"
                                           : app.auth.state === 2 ? "refresh"
                                           : app.auth.state === 4 ? "xmark" : "wifiOff"

    onActiveChanged: if (!active) passwordField.passwordVisible = false

    QQC.ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        contentHeight: page.height
        QQC.ScrollBar.vertical.policy: QQC.ScrollBar.AsNeeded
        background: Rectangle { color: "transparent" }

        Item {
            id: page
            width: scroll.availableWidth
            height: contentColumn.implicitHeight + root.pagePadding * 2

            ColumnLayout {
                id: contentColumn
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(th.contentMaxWidth, Math.max(0, parent.width - root.pagePadding * 2))
                spacing: th.sp8

                GridLayout {
                    columns: root.compact ? 1 : 2
                    columnSpacing: th.sp8
                    rowSpacing: th.sp6
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: th.sp6
                        Layout.fillWidth: true

                        Rectangle {
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44
                            radius: th.radius
                            color: th.blueFill
                            border.width: 1
                            border.color: th.fill(th.primary, 0.16)
                            Icon {
                                name: "power"
                                iconSize: 23
                                tint: th.primary
                                anchors.centerIn: parent
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Layout.fillWidth: true
                            QQC.Label {
                                text: (app.i18n.lang, app.i18n.text("app.title"))
                                color: th.text
                                font.weight: th.weightSemibold
                                font.pixelSize: th.fontTitle3
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                            QQC.Label {
                                text: (app.i18n.lang, app.i18n.text("app.subtitle"))
                                color: th.muted
                                font.pixelSize: th.fontSubhead
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }

                    LangSelector {
                        Layout.alignment: Qt.AlignVCenter | (root.compact ? Qt.AlignLeft : Qt.AlignRight)
                    }
                }

                Card {
                    margins: th.sp8
                    Layout.fillWidth: true

                    RowLayout {
                        width: parent.width
                        spacing: th.sp6

                        StatusBadge {
                            text: root.statusText
                            tint: root.statusTint
                            iconName: root.statusIconName
                            Layout.alignment: Qt.AlignVCenter
                        }
                        QQC.Label {
                            text: app.auth.lastMessage
                            color: th.muted
                            font.pixelSize: th.fontSubhead
                            wrapMode: Text.Wrap
                            visible: text.length > 0
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }

                GridLayout {
                    id: mainGrid
                    columns: root.wide ? 2 : 1
                    columnSpacing: th.sp8
                    rowSpacing: th.sp8
                    Layout.fillWidth: true

                    Card {
                        id: credentialsCard
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.wide ? (mainGrid.width - mainGrid.columnSpacing) / 2 : mainGrid.width
                        Layout.alignment: Qt.AlignTop

                        RowLayout {
                            width: parent.width
                            spacing: th.sp4
                            Icon { name: "person"; iconSize: 18; tint: th.primary; Layout.alignment: Qt.AlignVCenter }
                            QQC.Label {
                                text: (app.i18n.lang, app.i18n.text("section.credentials"))
                                color: th.text
                                font.weight: th.weightSemibold
                                font.pixelSize: th.fontHeadline
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }

                        Column {
                            width: parent.width
                            spacing: th.sp3
                            QQC.Label {
                                text: (app.i18n.lang, app.i18n.text("label.username"))
                                color: th.muted
                                font.pixelSize: th.fontFootnote
                            }
                            TextField {
                                width: parent.width
                                leadingIcon: "person"
                                placeholderText: (app.i18n.lang, app.i18n.text("placeholder.username"))
                                text: app.config.username
                                onTextChanged: app.config.username = text
                            }
                        }

                        Column {
                            width: parent.width
                            spacing: th.sp3
                            QQC.Label {
                                text: (app.i18n.lang, app.i18n.text("label.password"))
                                color: th.muted
                                font.pixelSize: th.fontFootnote
                            }
                            TextField {
                                id: passwordField
                                width: parent.width
                                leadingIcon: "lock"
                                passwordMode: true
                                placeholderText: (app.i18n.lang, app.i18n.text("placeholder.password"))
                                text: app.config.password
                                showPasswordAccessibleText: (app.i18n.lang, app.i18n.text("action.showPassword"))
                                hidePasswordAccessibleText: (app.i18n.lang, app.i18n.text("action.hidePassword"))
                                onTextChanged: app.config.password = text
                            }
                        }

                        Rectangle {
                            width: parent.width
                            implicitHeight: settingsColumn.implicitHeight
                            radius: th.radiusSmall
                            color: th.field
                            border.width: 1
                            border.color: th.separator

                            Column {
                                id: settingsColumn
                                anchors.left: parent.left
                                anchors.right: parent.right

                                SettingRow {
                                    width: parent.width
                                    title: (app.i18n.lang, app.i18n.text("action.remember"))
                                    Switch {
                                        checked: app.config.rememberPassword
                                        onToggled: app.config.rememberPassword = checked
                                    }
                                }
                                Rectangle { width: parent.width - 24; height: 1; color: th.separator; anchors.horizontalCenter: parent.horizontalCenter }
                                SettingRow {
                                    width: parent.width
                                    title: (app.i18n.lang, app.i18n.text("action.autoLogin"))
                                    Switch {
                                        checked: app.config.autoLogin
                                        onToggled: app.config.autoLogin = checked
                                    }
                                }
                            }
                        }

                        PrimaryButton {
                            width: parent.width
                            text: (app.i18n.lang, app.i18n.text("btn.login"))
                            variant: "primary"
                            leadingIcon: "power"
                            busy: app.auth.state === 2
                            onClicked: app.auth.login()
                        }

                        GridLayout {
                            width: parent.width
                            columns: root.compact ? 1 : 2
                            columnSpacing: th.sp4
                            rowSpacing: th.sp4

                            PrimaryButton {
                                text: (app.i18n.lang, app.i18n.text("btn.status"))
                                variant: "secondary"
                                leadingIcon: "refresh"
                                enabled: app.auth.state !== 2
                                Layout.fillWidth: true
                                onClicked: app.auth.refreshStatus()
                            }
                            PrimaryButton {
                                text: (app.i18n.lang, app.i18n.text("btn.logout"))
                                variant: "danger"
                                bg: "transparent"
                                leadingIcon: "power"
                                enabled: app.auth.state !== 2
                                Layout.fillWidth: true
                                onClicked: app.auth.logout()
                            }
                        }

                        PrimaryButton {
                            width: parent.width
                            text: app.auth.keepAliveRunning
                                  ? (app.i18n.lang, app.i18n.text("btn.keepStop"))
                                  : (app.i18n.lang, app.i18n.text("btn.keepStart"))
                            leadingIcon: "bolt"
                            variant: app.auth.keepAliveRunning ? "secondary" : "ghost"
                            onClicked: app.auth.keepAliveRunning
                                       ? app.auth.stopKeepAlive() : app.auth.startKeepAlive()
                        }
                    }

                    DevicePanel {
                        availableListHeight: Math.max(140, Math.min(280, root.height * 0.32))
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.wide ? (mainGrid.width - mainGrid.columnSpacing) / 2 : mainGrid.width
                        Layout.alignment: Qt.AlignTop
                    }

                    Card {
                        Layout.fillWidth: true
                        Layout.columnSpan: root.wide ? 2 : 1
                        Layout.alignment: Qt.AlignTop

                        DisclosureGroup {
                            width: parent.width
                            title: (app.i18n.lang, app.i18n.text("section.advanced"))
                            leadingIcon: "slider"
                            opened: false

                            GridLayout {
                                width: parent.width
                                columns: root.compact ? 1 : 2
                                columnSpacing: th.sp6
                                rowSpacing: th.sp6

                                ColumnLayout {
                                    spacing: th.sp3
                                    Layout.fillWidth: true
                                    QQC.Label {
                                        text: (app.i18n.lang, app.i18n.text("label.host"))
                                        color: th.muted
                                        font.pixelSize: th.fontFootnote
                                        Layout.fillWidth: true
                                    }
                                    TextField {
                                        leadingIcon: "globe"
                                        text: app.config.host
                                        Layout.fillWidth: true
                                        onTextChanged: app.config.host = text
                                    }
                                }
                                ColumnLayout {
                                    spacing: th.sp3
                                    Layout.fillWidth: true
                                    QQC.Label {
                                        text: (app.i18n.lang, app.i18n.text("label.ip"))
                                        color: th.muted
                                        font.pixelSize: th.fontFootnote
                                        Layout.fillWidth: true
                                    }
                                    TextField {
                                        leadingIcon: "server"
                                        text: app.config.ip
                                        Layout.fillWidth: true
                                        onTextChanged: app.config.ip = text
                                    }
                                }
                            }

                            Column {
                                width: parent.width
                                spacing: th.sp3
                                QQC.Label {
                                    text: (app.i18n.lang, app.i18n.text("label.nasip"))
                                    color: th.muted
                                    font.pixelSize: th.fontFootnote
                                }
                                TextField {
                                    width: parent.width
                                    leadingIcon: "server"
                                    text: app.config.nasip
                                    onTextChanged: app.config.nasip = text
                                }
                            }

                            Column {
                                width: parent.width
                                spacing: th.sp3
                                QQC.Label {
                                    text: (app.i18n.lang, app.i18n.text("label.requestEncoding"))
                                    color: th.muted
                                    font.pixelSize: th.fontFootnote
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
                                    Accessible.name: (app.i18n.lang, app.i18n.text("label.requestEncoding"))
                                    onActivated: function(index) { app.config.requestEncoding = root.encodingModel[index] }

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
                                        contentItem: QQC.Label {
                                            text: modelData
                                            color: encodingCombo.currentIndex === index ? th.primary : th.text
                                            font.pixelSize: th.fontBody
                                            leftPadding: 10
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        background: Rectangle {
                                            radius: th.radiusSmall
                                            color: encodingCombo.highlightedIndex === index ? th.blueFill : "transparent"
                                        }
                                    }
                                    popup: QQC.Popup {
                                        id: encodingPopup
                                        width: encodingCombo.width
                                        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                                                         Math.max(th.touchTarget * 2, encodingCombo.Window.height - th.sp8))
                                        padding: th.sp2
                                        y: {
                                            var origin = encodingCombo.mapToItem(null, 0, 0);
                                            var below = origin.y + encodingCombo.height + th.sp2;
                                            var above = origin.y - height - th.sp2;
                                            var desired = encodingCombo.Window.height - below >= height ? below : above;
                                            return Math.max(th.sp2, Math.min(desired, encodingCombo.Window.height - height - th.sp2)) - origin.y;
                                        }
                                        contentItem: ListView {
                                            clip: true
                                            implicitHeight: contentHeight
                                            model: encodingCombo.delegateModel
                                            currentIndex: encodingCombo.highlightedIndex
                                            boundsBehavior: Flickable.StopAtBounds
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

                            GridLayout {
                                width: parent.width
                                columns: root.compact ? 1 : 2
                                columnSpacing: th.sp6
                                rowSpacing: th.sp6
                                PrimaryButton {
                                    text: (app.i18n.lang, app.i18n.text("btn.save"))
                                    variant: "primary"
                                    leadingIcon: "check"
                                    Layout.fillWidth: true
                                    onClicked: {
                                        app.config.save();
                                        saveFeedback.visible = true;
                                    }
                                }
                                StatusBadge {
                                    id: saveFeedback
                                    text: (app.i18n.lang, app.i18n.text("config.saved"))
                                    iconName: "check"
                                    tint: th.success
                                    visible: false
                                    Layout.alignment: Qt.AlignVCenter | (root.compact ? Qt.AlignLeft : Qt.AlignRight)
                                }
                            }
                        }
                    }

                    LogPanel {
                        viewportHeight: Math.max(160, Math.min(280, root.height * 0.34))
                        Layout.fillWidth: true
                        Layout.columnSpan: root.wide ? 2 : 1
                        Layout.alignment: Qt.AlignTop
                    }
                }
            }
        }
    }
}

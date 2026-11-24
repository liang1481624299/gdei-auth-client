import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}

    signal openAdvancedRequested()
    signal openEventsRequested()
    signal openLogsRequested()
    signal openAboutRequested()

    title: "profile.title"

    component NavigationRow: QQC.ItemDelegate {
        id: control

        property string title: ""
        property string iconName: ""
        property int badgeCount: 0

        implicitHeight: Math.max(th.touchTarget, contentItem.implicitHeight)
        padding: 0
        hoverEnabled: true
        Accessible.name: title

        contentItem: SettingRow {
            title: control.title
            leadingIcon: control.iconName

            RowLayout {
                spacing: th.sp4

                UnreadBadge {
                    count: control.badgeCount
                    visible: control.badgeCount > 0
                    Layout.alignment: Qt.AlignVCenter
                }
                Icon {
                    name: "chevronDown"
                    iconSize: 13
                    rotation: -90
                    tint: th.tertiaryLabel
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }

        background: Rectangle {
            radius: th.radiusSmall
            color: control.down ? th.controlPressed
                 : control.hovered ? th.controlHover : "transparent"
            border.width: control.activeFocus ? 2 : 0
            border.color: th.focusRing
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        GridLayout {
            columns: width >= th.compactBreakpoint ? 2 : 1
            columnSpacing: th.sp8
            rowSpacing: th.sp8
            Layout.fillWidth: true

            Card {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                RowLayout {
                    width: parent.width
                    spacing: th.sp6

                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        Layout.alignment: Qt.AlignVCenter
                        radius: 24
                        color: th.blueFill
                        Icon {
                            anchors.centerIn: parent
                            name: "person"
                            iconSize: 24
                            tint: th.primary
                        }
                    }

                    ColumnLayout {
                        spacing: th.sp2
                        Layout.fillWidth: true

                        QQC.Label {
                            text: (app.i18n.lang, app.i18n.text("label.username"))
                            color: th.muted
                            font.pixelSize: th.fontFootnote
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                        QQC.Label {
                            text: app.config.username.length > 0
                                  ? app.config.username
                                  : (app.i18n.lang, app.i18n.text("common.unavailable"))
                            color: th.text
                            font.pixelSize: th.fontTitle3
                            font.weight: th.weightSemibold
                            wrapMode: Text.WrapAnywhere
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("profile.balance"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    wrapMode: Text.Wrap
                }
                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("profile.balanceUnavailable"))
                    color: th.text
                    font.pixelSize: th.fontHeadline
                    font.weight: th.weightMedium
                    wrapMode: Text.Wrap
                }
            }
        }

        Card {
            margins: th.sp3
            Layout.fillWidth: true

            NavigationRow {
                width: parent.width
                title: (app.i18n.lang, app.i18n.text("section.advanced"))
                iconName: "slider"
                onClicked: root.openAdvancedRequested()
            }
            Rectangle {
                width: parent.width - th.sp12
                height: 1
                anchors.horizontalCenter: parent.horizontalCenter
                color: th.separator
            }
            SettingRow {
                width: parent.width
                title: (app.i18n.lang, app.i18n.text("label.language"))
                leadingIcon: "globe"
                LangSelector {}
            }
            SettingRow {
                width: parent.width
                title: (app.i18n.lang, app.i18n.text("theme.title"))
                leadingIcon: "slider"
                ThemeSelector {}
            }
            Rectangle {
                width: parent.width - th.sp12
                height: 1
                anchors.horizontalCenter: parent.horizontalCenter
                color: th.separator
            }
            NavigationRow {
                width: parent.width
                title: (app.i18n.lang, app.i18n.text("events.title"))
                iconName: "list"
                badgeCount: app.events.unreadCount
                onClicked: root.openEventsRequested()
            }
            Rectangle {
                width: parent.width - th.sp12
                height: 1
                anchors.horizontalCenter: parent.horizontalCenter
                color: th.separator
            }
            NavigationRow {
                width: parent.width
                title: (app.i18n.lang, app.i18n.text("log.title"))
                iconName: "list"
                onClicked: root.openLogsRequested()
            }
            Rectangle {
                width: parent.width - th.sp12
                height: 1
                anchors.horizontalCenter: parent.horizontalCenter
                color: th.separator
            }
            NavigationRow {
                width: parent.width
                title: (app.i18n.lang, app.i18n.text("about.title"))
                iconName: "info"
                onClicked: root.openAboutRequested()
            }
        }

        PrimaryButton {
            text: (app.i18n.lang, app.i18n.text("btn.login"))
            leadingIcon: "power"
            Layout.fillWidth: true
            visible: !app.session.loggedIn
            onClicked: app.session.promptLogin()
        }

        PrimaryButton {
            text: (app.i18n.lang, app.i18n.text("profile.signOut"))
            variant: "danger"
            bg: "transparent"
            leadingIcon: "power"
            Layout.fillWidth: true
            visible: app.session.loggedIn
            onClicked: signOutDialog.open()
        }
    }

    property var signOutDialog: AppDialog {
        id: signOutDialog
        title: (app.i18n.lang, app.i18n.text("profile.signOutTitle"))
        message: (app.i18n.lang, app.i18n.text("profile.signOutMessage"))
        confirmText: (app.i18n.lang, app.i18n.text("profile.signOut"))
        cancelText: (app.i18n.lang, app.i18n.text("action.cancel"))
        showCancel: true
        destructive: true
        onAccepted: app.session.signOutLocal()
    }
}

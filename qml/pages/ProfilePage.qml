import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 设置页：基础偏好（语言、外观）与系统入口（高级设置、事件、日志、关于）。
// Settings: preferences (language, appearance) and system entries.
PageScaffold {
    id: root

    readonly property Theme th: Theme {}

    signal openAdvancedRequested()
    signal openEventsRequested()
    signal openLogsRequested()
    signal openAboutRequested()

    title: "nav.settings"

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
                // 页面跳转统一用右箭头；下拉选择控件使用其自身指示图标，二者不混用。
                Icon {
                    name: "chevronRight"
                    iconSize: 15
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

        // 基础偏好：语言、外观。
        Card {
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: th.sp2

                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("settings.prefGroup"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }

                SettingRow {
                    width: parent.width
                    title: (app.i18n.lang, app.i18n.text("label.language"))
                    leadingIcon: "globe"
                    LangSelector {}
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: th.separator
                }
                SettingRow {
                    width: parent.width
                    title: (app.i18n.lang, app.i18n.text("theme.title"))
                    leadingIcon: "slider"
                    ThemeSelector {}
                }
            }
        }

        // 系统：高级设置为设置内子页面，与基础偏好分组，不与之平级平铺。
        Card {
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: th.sp2

                QQC.Label {
                    width: parent.width
                    text: (app.i18n.lang, app.i18n.text("settings.systemGroup"))
                    color: th.muted
                    font.pixelSize: th.fontFootnote
                    Layout.fillWidth: true
                }

                NavigationRow {
                    width: parent.width
                    title: (app.i18n.lang, app.i18n.text("section.advanced"))
                    iconName: "slider"
                    onClicked: root.openAdvancedRequested()
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
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
                    Layout.fillWidth: true
                    height: 1
                    color: th.separator
                }
                NavigationRow {
                    width: parent.width
                    title: (app.i18n.lang, app.i18n.text("log.title"))
                    iconName: "list"
                    onClicked: root.openLogsRequested()
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: th.separator
                }
                NavigationRow {
                    width: parent.width
                    title: (app.i18n.lang, app.i18n.text("about.title"))
                    iconName: "info"
                    onClicked: root.openAboutRequested()
                }
            }
        }
    }
}

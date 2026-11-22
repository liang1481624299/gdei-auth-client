import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "theme"
import "components"
import "pages"

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

    Loader {
        id: loginLoader
        anchors.fill: parent
        active: !app.session.loggedIn
        sourceComponent: LoginPage {}
    }

    Loader {
        id: mainLoader
        anchors.fill: parent
        active: app.session.loggedIn
        sourceComponent: mainShellComponent
    }

    Component {
        id: mainShellComponent

        Item {
            id: shell
            readonly property int tabHeight: tabBar.height

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                StackLayout {
                    id: stack
                    currentIndex: 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    onCurrentIndexChanged: {
                        app.events.setEventPageVisible(currentIndex === 5);
                    }

                    DashboardPage {
                        onManageDevicesRequested: stack.currentIndex = 1
                    }
                    DevicesPage {
                        onOpenRemoteRequested: stack.currentIndex = 3
                    }
                    ProfilePage {
                        onOpenAdvancedRequested: stack.currentIndex = 4
                        onOpenEventsRequested: stack.currentIndex = 5
                        onOpenLogsRequested: stack.currentIndex = 6
                        onOpenAboutRequested: stack.currentIndex = 7
                    }
                    Loader {
                        active: stack.currentIndex === 3
                        sourceComponent: RemoteDevicesPage {
                            onBackRequested: stack.currentIndex = 1
                        }
                    }
                    AdvancedSettingsPage {
                        onBackRequested: stack.currentIndex = 2
                    }
                    EventLogPage {
                        onBackRequested: stack.currentIndex = 2
                    }
                    DiagnosticsPage {
                        onBackRequested: stack.currentIndex = 2
                    }
                    AboutPage {
                        onBackRequested: stack.currentIndex = 2
                    }
                }

                AppTabBar {
                    id: tabBar
                    currentIndex: stack.currentIndex === 3 ? 1
                                : stack.currentIndex > 3 ? 2 : stack.currentIndex
                    Layout.fillWidth: true
                    onTabSelected: function(index) { stack.currentIndex = index; }
                }
            }

            Component.onCompleted: app.events.setEventPageVisible(stack.currentIndex === 5)
            Component.onDestruction: app.events.setEventPageVisible(false)
        }
    }

    ToastHost {
        bottomOffset: mainLoader.item ? mainLoader.item.tabHeight : 0
    }
}

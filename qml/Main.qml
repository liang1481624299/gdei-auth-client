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

    readonly property int maxWindowWidth: th.dp(1040)
    readonly property int maxWindowHeight: th.dp(860)
    readonly property int minWindowWidth: th.dp(320)
    readonly property int minWindowHeight: th.dp(400)

    width: Math.min(maxWindowWidth, Screen.desktopAvailableWidth,
                    Math.max(minWindowWidth, Math.round(Screen.desktopAvailableWidth * 0.90)))
    height: Math.min(maxWindowHeight, Screen.desktopAvailableHeight,
                     Math.max(minWindowHeight, Math.round(Screen.desktopAvailableHeight * 0.86)))
    minimumWidth: Math.min(minWindowWidth, Screen.desktopAvailableWidth)
    minimumHeight: Math.min(minWindowHeight, Screen.desktopAvailableHeight)
    visible: true
    color: th.window
    title: (app.i18n.lang, app.i18n.text("app.title"))

    // 主框架常驻：未登录也可浏览公开页，导航栏始终可见。
    // The shell stays mounted so public pages and the tab bar are browsable without sign-in.
    Loader {
        id: mainLoader
        anchors.fill: parent
        active: true
        sourceComponent: mainShellComponent
    }

    // 按需登录弹层：受保护操作触发后弹出，可取消返回浏览。
    // On-demand login sheet: shown by protected actions; dismiss to keep browsing.
    Rectangle {
        id: loginMask
        anchors.fill: parent
        visible: app.session.loginPromptOpen
        color: th.overlay
        z: 10

        Loader {
            anchors.fill: parent
            active: loginMask.visible
            sourceComponent: LoginPage {}
        }
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

            // 门禁续做：按需登录后把用户带回被保护的页面继续原操作，不静默执行写操作。
            Connections {
                target: app.session
                function onPendingActionReady(actionKey) {
                    if (actionKey === "remote.open") stack.currentIndex = 3;
                    else if (actionKey === "settings.save") stack.currentIndex = 4;
                }
                // 登出后复位页面栈到首页，避免高级设置/日志等敏感页残留。
                function onSessionCleared() { stack.currentIndex = 0; }
            }

            Component.onCompleted: app.events.setEventPageVisible(stack.currentIndex === 5)
            Component.onDestruction: app.events.setEventPageVisible(false)
        }
    }

    ToastHost {
        bottomOffset: mainLoader.item ? mainLoader.item.tabHeight : 0
    }
}

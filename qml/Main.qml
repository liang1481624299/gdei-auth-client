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

    // 默认按手机竖屏比例开窗，可自由拖拽放大；内容栏在宽屏居中受限。
    // Default phone-portrait size; resizable, content column stays centered on wide screens.
    readonly property int defaultWindowWidth: th.dp(400)
    readonly property int defaultWindowHeight: th.dp(720)
    readonly property int minWindowWidth: th.dp(320)
    readonly property int minWindowHeight: th.dp(480)

    width: defaultWindowWidth
    height: defaultWindowHeight
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
                        app.events.setEventPageVisible(currentIndex === 6);
                    }

                    // 0 主页
                    DashboardPage {
                        onOpenDiagnosticsRequested: stack.currentIndex = 2
                        onOpenEventsRequested: stack.currentIndex = 6
                    }
                    // 1 设备
                    DevicesPage {
                        onOpenRemoteRequested: stack.currentIndex = 4
                    }
                    // 2 发现（网络诊断工具）
                    DiagnosticsPage {}
                    // 3 账号（账号数据 + 并入的偏好与系统入口）
                    AccountDashboardPage {
                        onOpenAdvancedRequested: stack.currentIndex = 5
                        onOpenEventsRequested: stack.currentIndex = 6
                        onOpenAboutRequested: stack.currentIndex = 7
                    }
                    // 4 远程设备
                    Loader {
                        active: stack.currentIndex === 4
                        sourceComponent: RemoteDevicesPage {
                            onBackRequested: stack.currentIndex = 1
                        }
                    }
                    // 5 高级设置
                    AdvancedSettingsPage {
                        onBackRequested: stack.currentIndex = 3
                    }
                    // 6 事件记录
                    EventLogPage {
                        onBackRequested: stack.currentIndex = 3
                    }
                    // 7 关于
                    AboutPage {
                        onBackRequested: stack.currentIndex = 3
                    }
                }

                AppTabBar {
                    id: tabBar
                    currentIndex: stack.currentIndex === 4 ? 1
                                : stack.currentIndex > 3 ? 3 : stack.currentIndex
                    Layout.fillWidth: true
                    onTabSelected: function(index) { stack.currentIndex = index; }
                }
            }

            // 门禁续做：按需登录后把用户带回被保护的页面继续原操作，不静默执行写操作。
            Connections {
                target: app.session
                function onPendingActionReady(actionKey) {
                    if (actionKey === "remote.open") stack.currentIndex = 4;
                    else if (actionKey === "settings.save") stack.currentIndex = 5;
                }
                // 登出后复位页面栈到首页，避免高级设置/日志等敏感页残留。
                function onSessionCleared() { stack.currentIndex = 0; }
            }

            Component.onCompleted: app.events.setEventPageVisible(stack.currentIndex === 6)
            Component.onDestruction: app.events.setEventPageVisible(false)
        }
    }

    ToastHost {
        bottomOffset: mainLoader.item ? mainLoader.item.tabHeight : 0
    }
}

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

    // 桌面默认宽窗开窗，内容栏按各页 contentMaxWidth 居中；最小宽高保留窄屏竖屏可用。
    readonly property int defaultWindowWidth: th.dp(880)
    readonly property int defaultWindowHeight: th.dp(640)
    readonly property int minWindowWidth: th.dp(360)
    readonly property int minWindowHeight: th.dp(520)

    width: defaultWindowWidth
    height: defaultWindowHeight
    minimumWidth: Math.min(minWindowWidth, Screen.desktopAvailableWidth)
    minimumHeight: Math.min(minWindowHeight, Screen.desktopAvailableHeight)
    visible: true
    color: th.window
    title: (app.i18n.dictionary["app.title"])

    // 全局字体：内置字体由 main.cpp 在启动时经 QFontDatabase 注册到应用数据库，
    // Theme.fontFamily 回退链按拉丁 / 简体 / 繁体 / 日语依次兜底，缺失字形顺延不崩溃。
    // Window（非 ApplicationWindow）没有 font 属性；字体由各组件统一使用 Theme.fontFamily。

    // 主框架常驻：未登录也可浏览公开页，导航栏始终可见。
    Loader {
        id: mainLoader
        anchors.fill: parent
        active: true
        sourceComponent: mainShellComponent
    }

    // 按需登录弹层：受保护操作触发后弹出，可取消返回浏览。
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
            anchors.fill: parent
            readonly property int tabHeight: tabBar.height

            // 全局状态横幅（规范 4.9 / 5.2）：断网、认证失败等全局提示，可关闭，
            // 状态变化时重新出现。
            property string bannerText: {
                if (!app.network.available)
                    return app.i18n.dictionary["layer.nicDown"];
                if (app.session.lastError && app.session.lastError.length > 0)
                    return app.i18n.dictionary[app.session.lastError];
                if (app.auth.state === 4 && app.auth.lastMessage && app.auth.lastMessage.length > 0)
                    return app.i18n.dictionary[app.auth.lastMessage];
                return "";
            }
            readonly property color bannerTint: !app.network.available ? th.danger
                                            : (app.session.lastError && app.session.lastError.length > 0) ? th.danger
                                            : th.warning
            readonly property string bannerIcon: !app.network.available ? "wifiOff"
                                          : (app.session.lastError && app.session.lastError.length > 0) ? "xmark"
                                          : "info"
            property bool bannerDismissed: false
            onBannerTextChanged: bannerDismissed = false

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Banner {
                    Layout.fillWidth: true
                    Layout.leftMargin: th.sp6
                    Layout.rightMargin: th.sp6
                    Layout.topMargin: th.sp6
                    message: shell.bannerText
                    tint: shell.bannerTint
                    iconName: shell.bannerIcon
                    visible: shell.bannerText.length > 0 && !shell.bannerDismissed
                    onDismissed: shell.bannerDismissed = true
                }

                StackLayout {
                    id: stack
                    currentIndex: 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 0
                    Layout.minimumHeight: 0

                    onCurrentIndexChanged: {
                        // 事件记录子页可见时清除未读标记。
                        app.events.setEventPageVisible(currentIndex === 7);
                    }

                    // 0 首页（连接状态与上下线）
                    OverviewPage {
                    }
                    // 1 设备
                    DevicesPage {
                        onOpenRemoteRequested: stack.currentIndex = 5
                    }
                    // 2 发现（网络工具与链路信息）
                    DiscoverPage {
                        onOpenDiagnosticsRequested: stack.currentIndex = 4
                    }
                    // 3 我的（账号资料 + 偏好 + 系统工具 + 运行记录）
                    MinePage {
                        onOpenAdvancedRequested: stack.currentIndex = 6
                        onOpenAboutRequested: stack.currentIndex = 9
                        onOpenEventsRequested: stack.currentIndex = 7
                        onOpenLogRequested: stack.currentIndex = 8
                    }

                    // 4 诊断（子页，父 Tab=发现）
                    Loader {
                        active: stack.currentIndex === 4
                        sourceComponent: DiagnosticsPage {
                            onBack: stack.currentIndex = 2
                        }
                    }
                    // 5 远程设备（子页，父 Tab=设备）
                    Loader {
                        active: stack.currentIndex === 5
                        sourceComponent: RemoteDevicesPage {
                            onBack: stack.currentIndex = 1
                        }
                    }
                    // 6 高级设置（子页，父 Tab=我的）
                    Loader {
                        active: stack.currentIndex === 6
                        sourceComponent: AdvancedSettingsPage {
                            onBack: stack.currentIndex = 3
                        }
                    }
                    // 7 事件记录（子页，父 Tab=我的）
                    Loader {
                        active: stack.currentIndex === 7
                        sourceComponent: EventLogPage {
                            onBack: stack.currentIndex = 3
                        }
                    }
                    // 8 系统日志（子页，父 Tab=我的）
                    Loader {
                        active: stack.currentIndex === 8
                        sourceComponent: LogPage {
                            onBack: stack.currentIndex = 3
                        }
                    }
                    // 9 关于（子页，父 Tab=我的）
                    Loader {
                        active: stack.currentIndex === 9
                        sourceComponent: AboutPage {
                            onBack: stack.currentIndex = 3
                        }
                    }
                }

                AppTabBar {
                    id: tabBar
                    Layout.fillWidth: true
                    // 子页面高亮归属到父 Tab：5 设备；4 发现；6/7/8/9 我的。
                    currentIndex: stack.currentIndex === 5 ? 1
                                : stack.currentIndex === 4 ? 2
                                : stack.currentIndex === 6 || stack.currentIndex === 7 || stack.currentIndex === 8 || stack.currentIndex === 9 ? 3
                                : stack.currentIndex
                    onTabSelected: function(index) { stack.currentIndex = index; }
                }
            }

            // 门禁续做：按需登录后把用户带回被保护的页面继续原操作，不静默执行写操作。
            Connections {
                target: app.session
                function onPendingActionReady(actionKey) {
                    if (actionKey === "remote.open") stack.currentIndex = 5;
                    else if (actionKey === "settings.save") stack.currentIndex = 6;
                }
                // 登出后复位页面栈到概览，避免敏感页残留。
                function onSessionCleared() { stack.currentIndex = 0; }
            }

            Component.onCompleted: app.events.setEventPageVisible(stack.currentIndex === 7)
            Component.onDestruction: app.events.setEventPageVisible(false)
        }
    }

    ToastHost {
        bottomOffset: mainLoader.item ? mainLoader.item.tabHeight : 0
    }
}

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

// 系统日志页（StackLayout 索引 8）—— 全屏日志 + 导出。
// System log page — full-height log viewer with export.
PageScaffold {
    id: root
    Theme { id: th }

    title: "log.title"
    showBack: true
    contentMaxWidth: th.dp(960)

    LogPanel {
        Layout.fillWidth: true
        viewportHeight: Math.max(260, root.height - th.dp(200))
    }
}

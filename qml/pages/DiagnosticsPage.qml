import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}

    title: "log.title"
    showBack: true

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        LogPanel {
            viewportHeight: Math.max(180, root.height - 240)
            Layout.fillWidth: true
        }
    }
}

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

PageScaffold {
    id: root

    readonly property Theme th: Theme {}

    title: "nav.discover"
    showBack: false

    ColumnLayout {
        width: parent.width
        spacing: th.sp8

        LogPanel {
            viewportHeight: Math.max(180, root.height - 240)
            Layout.fillWidth: true
        }
    }
}

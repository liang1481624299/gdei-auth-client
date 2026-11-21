// qml/components/LangSelector.qml
// 语言切换器 / Language switcher.
// 显示名：简体中文 / 繁體中文 / 日本語 / English (来自 C++ I18n).
import QtQuick
import QtQuick.Controls
import "../theme"

ComboBox {
    id: control
    Theme { id: th }

    model: app.i18n.languages
    textRole: "name"
    valueRole: "value"
    currentValue: app.i18n.lang

    implicitHeight: 38

    background: Rectangle {
        radius: th.radiusSmall
        color: th.field
        border.width: 1
        border.color: th.border
    }

    delegate: ItemDelegate {
        width: control.width
        Label {
            text: model.name
            color: th.text
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    contentItem: Label {
        text: control.currentText
        color: th.text
        leftPadding: 12
        verticalAlignment: Text.AlignVCenter
    }

    onCurrentValueChanged: {
        if (currentValue !== undefined) app.i18n.setLang(currentValue);
    }
}

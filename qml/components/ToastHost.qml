import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "."

Item {
    id: root
    Theme { id: th }

    property int bottomOffset: 0
    readonly property bool compact: width < th.compactBreakpoint
    readonly property real toastWidth: Math.min(380, Math.max(0, width - th.sp8 * 2))

    anchors.fill: parent
    anchors.bottomMargin: bottomOffset
    z: 1000

    function localized(key, args) {
        if (key === undefined || key === null || String(key).length === 0)
            return "";
        app.i18n.lang;
        var value = app.i18n.text(String(key));
        if (args && args.name !== undefined && args.name !== null && value.indexOf("%1") >= 0)
            value = value.arg(String(args.name));
        return value;
    }

    function levelColor(level) {
        if (level === 3) return th.danger;
        if (level === 2) return th.warning;
        return th.primary;
    }

    ListModel { id: toastModel }

    Connections {
        target: app.events

        function onToastRequested(event) {
            if (toastModel.count >= 3)
                toastModel.remove(0);
            // 存 key/args 而非译文，delegate 显示时才翻译，切语言后自动刷新。
            // Store keys, not rendered text, so labels re-translate on language change.
            toastModel.append({
                titleKey: event.titleKey,
                messageKey: event.messageKey,
                reasonKey: event.reasonKey,
                args: event.arguments,
                level: Number(event.level)
            });
        }
    }

    Column {
        id: toastStack
        x: root.compact ? Math.round((root.width - width) / 2)
                        : root.width - width - th.sp8
        y: th.sp8
        width: root.toastWidth
        spacing: th.sp4

        Repeater {
            model: toastModel

            Item {
                id: toast
                required property int index
                required property string titleKey
                required property string messageKey
                required property string reasonKey
                required property var args
                required property int level

                // 显示时翻译，app.i18n.lang 作为依赖触发切语言刷新。
                readonly property string title: root.localized(titleKey, args)
                readonly property string message: root.localized(messageKey, args)
                readonly property string reason: root.localized(reasonKey, args)

                property bool closing: false
                property real slideY: -th.sp4

                width: toastStack.width
                height: card.implicitHeight
                opacity: 0
                transform: Translate { y: toast.slideY }

                function dismiss() {
                    if (closing) return;
                    closing = true;
                    lifetime.stop();
                    opacity = 0;
                    slideY = -th.sp2;
                    removal.start();
                }

                Component.onCompleted: {
                    opacity = 1;
                    slideY = 0;
                }

                Behavior on opacity { NumberAnimation { duration: th.durationNormal } }
                Behavior on slideY { NumberAnimation { duration: th.durationNormal } }

                Timer {
                    id: lifetime
                    interval: 4500
                    repeat: false
                    running: true
                    onTriggered: toast.dismiss()
                }

                Timer {
                    id: removal
                    interval: th.durationNormal
                    repeat: false
                    onTriggered: {
                        if (toast.index >= 0 && toast.index < toastModel.count)
                            toastModel.remove(toast.index);
                    }
                }

                Card {
                    id: card
                    width: parent.width
                    margins: th.sp5

                    RowLayout {
                        width: parent.width
                        spacing: th.sp4

                        Icon {
                            name: "info"
                            iconSize: 18
                            tint: root.levelColor(toast.level)
                            Layout.alignment: Qt.AlignTop
                        }

                        ColumnLayout {
                            spacing: th.sp2
                            Layout.fillWidth: true

                            QQC.Label {
                                text: toast.title
                                color: th.text
                                font.pixelSize: th.fontBody
                                font.weight: th.weightSemibold
                                wrapMode: Text.WrapAnywhere
                                visible: text.length > 0
                                Layout.fillWidth: true
                            }
                            QQC.Label {
                                text: toast.message
                                color: th.text
                                font.pixelSize: th.fontBody
                                wrapMode: Text.WrapAnywhere
                                visible: text.length > 0
                                Layout.fillWidth: true
                            }
                            QQC.Label {
                                text: toast.reason
                                color: th.muted
                                font.pixelSize: th.fontFootnote
                                wrapMode: Text.WrapAnywhere
                                visible: text.length > 0
                                Layout.fillWidth: true
                            }
                        }

                        QQC.Button {
                            id: closeButton
                            implicitWidth: th.touchTarget
                            implicitHeight: th.touchTarget
                            padding: 0
                            hoverEnabled: true
                            Accessible.name: (app.i18n.lang, app.i18n.text("common.close"))
                            Layout.alignment: Qt.AlignTop
                            onClicked: toast.dismiss()

                            contentItem: Icon {
                                name: "xmark"
                                iconSize: 16
                                tint: closeButton.hovered ? th.text : th.muted
                            }
                            background: Rectangle {
                                radius: th.radiusSmall
                                color: closeButton.down ? th.controlPressed
                                     : closeButton.hovered ? th.controlHover : "transparent"
                                border.width: closeButton.activeFocus ? 2 : 0
                                border.color: th.focusRing
                                Behavior on color { ColorAnimation { duration: th.durationFast } }
                            }
                        }
                    }
                }
            }
        }
    }
}

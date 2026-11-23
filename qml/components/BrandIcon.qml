// qml/components/BrandIcon.qml
// 平台品牌标识：按设备操作系统文本匹配对应品牌单色 SVG，并套品牌色。
// Platform brand glyph: matches the device OS text to a brand mono SVG, tinted per brand.
// 品牌 logo 仅作平台兼容标识，商标归属各厂商，详见 docs/THIRD-PARTY-NOTICES.md。
import QtQuick
import "../theme"
import "."

Item {
    id: root
    Theme { id: th }

    // 门户返回的操作系统 / 终端类型文本；可同时传入 deviceType 辅助匹配。
    property string osText: ""
    property int iconSize: th.dp(18)

    implicitWidth: brandIcon.visible ? iconSize : 0
    implicitHeight: iconSize
    width: implicitWidth
    height: implicitHeight

    // 关键词模糊匹配到品牌 key；发行版优先于通用 Linux。
    function brandKey(text) {
        var s = (text || "").toLowerCase()
        if (s.indexOf("windows") >= 0 || s.indexOf("win32") >= 0 ||
            s.indexOf("win64") >= 0) return "windows"
        if (s.indexOf("macos") >= 0 || s.indexOf("mac os") >= 0 ||
            s.indexOf("darwin") >= 0 || s.indexOf("ios") >= 0 ||
            s.indexOf("iphone") >= 0 || s.indexOf("ipad") >= 0) return "macos"
        if (s.indexOf("android") >= 0 || s.indexOf("安卓") >= 0) return "android"
        if (s.indexOf("debian") >= 0) return "debian"
        if (s.indexOf("arch") >= 0) return "arch"
        if (s.indexOf("ubuntu") >= 0) return "ubuntu"
        if (s.indexOf("linux") >= 0 || s.indexOf("gnu") >= 0 ||
            s.indexOf("centos") >= 0 || s.indexOf("fedora") >= 0) return "linux"
        return ""
    }

    function brandColor(key) {
        switch (key) {
        case "windows": return "#0078D4"
        case "macos":   return th.isDark ? "#AEAEB2" : "#6E6E73"
        case "android": return "#3DDC84"
        case "linux":   return "#FCC624"
        case "debian":  return "#A80030"
        case "arch":    return "#1793D1"
        case "ubuntu":  return "#E95420"
        }
        return th.muted
    }

    readonly property string key: brandKey(osText)

    Icon {
        id: brandIcon
        anchors.centerIn: parent
        name: root.key.length > 0 ? "brand/" + root.key : ""
        iconSize: root.iconSize
        tint: root.brandColor(root.key)
        visible: root.key.length > 0
    }
}

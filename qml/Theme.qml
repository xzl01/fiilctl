pragma Singleton
import QtQuick

QtObject {
    readonly property color bg: "#0d0d10"
    readonly property color bgElev: "#141419"
    readonly property color card: "#17171c"
    readonly property color cardHi: "#1e1e25"
    readonly property color border: "#282830"
    readonly property color borderHi: "#3a3a46"
    readonly property color text: "#ececf1"
    readonly property color textDim: "#8f8f9d"
    readonly property color accent: "#e3c56b"
    readonly property color accentDim: "#3a3218"
    readonly property color good: "#6be3b0"
    readonly property color bad: "#e3806b"

    readonly property int radius: 16
    readonly property int spacing: 14

    readonly property string uiFont: "Noto Sans CJK SC"
    readonly property string monoFont: "JetBrainsMono Nerd Font"
}

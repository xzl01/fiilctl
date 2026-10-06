import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: page
    property var backend

    readonly property var presetNames: ["怀旧", "爵士", "轻柔", "剧院", "金属", "流行", "R&B", "舞曲", "摇滚", "电子", "重低", "律动"]
    readonly property var bands: ["31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"]
    property var localGains: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    property bool dirty: false

    function modeName(m) {
        if (m === 0) return "原声（默认）"
        if (m === 15) return "自定义"
        if (m >= 3 && m <= 14) return presetNames[m - 3]
        return m < 0 ? "—" : "模式 " + m
    }

    Connections {
        target: page.backend
        function onStateChanged() {
            if (page.backend.eqLoaded && !page.dirty) {
                var g = page.backend.eqGains
                var out = []
                for (var i = 0; i < 10; ++i) out.push(i < g.length ? g[i] : 0)
                page.localGains = out
            }
        }
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.height + 24
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            width: parent.width - 8
            x: 4
            spacing: 16

            // ── 当前音效 ──────────────────────────────
            Card {
                width: parent.width
                title: "当前音效"
                subtitle: "读取自耳机 · 10 段增益，单位约 1dB"

                Row {
                    spacing: 12
                    Pill {
                        text: backend.eqLoaded ? page.modeName(backend.eqMode) : "未读取"
                        tone: backend.eqLoaded ? Theme.accent : Theme.textDim
                    }
                    Item { width: Math.max(0, chart.width - 260); height: 1 }
                    ActionButton {
                        small: true
                        text: "读取 EQ"
                        enabled: backend.ready
                        onClicked: {
                            page.dirty = false
                            backend.readEq()
                        }
                    }
                }

                Item {
                    id: chart
                    width: parent.width
                    height: 108

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Repeater {
                            model: 10
                            delegate: Item {
                                required property int index
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                readonly property int gain: page.localGains[index]
                                readonly property real half: 44

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 0
                                    text: gain > 0 ? "+" + gain : gain
                                    color: gain > 0 ? Theme.accent : gain < 0 ? Theme.bad : Theme.textDim
                                    font { family: Theme.monoFont; pixelSize: 11 }
                                }
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: Math.min(parent.width - 14, 30)
                                    radius: 6
                                    color: gain > 0 ? Theme.accent : gain < 0 ? Theme.bad : Theme.cardHi
                                    opacity: 0.9
                                    height: Math.max(3, Math.abs(gain) * 3.6)
                                    y: gain >= 0 ? 16 + half - height : 16 + half
                                }
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 16 + half
                                    width: parent.width - 6
                                    height: 1
                                    color: Theme.border
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 16 + half + 12
                                    text: page.bands[index]
                                    color: Theme.textDim
                                    font { family: Theme.monoFont; pixelSize: 11 }
                                }
                            }
                        }
                    }
                }
            }

            // ── 预设 ─────────────────────────────────
            Card {
                width: parent.width
                title: "预设音效"
                subtitle: "点击即写入耳机（mode = 3 + 序号）"

                Flow {
                    width: parent.width
                    spacing: 10
                    ActionButton {
                        text: "原声（默认）"
                        small: false
                        accent: backend.eqMode === 0
                        enabled: backend.ready
                        onClicked: backend.setEqPreset(0)
                    }
                    Repeater {
                        model: page.presetNames
                        delegate: ActionButton {
                            required property var modelData
                            required property int index
                            text: modelData
                            accent: backend.eqMode === (index + 3)
                            enabled: backend.ready
                            onClicked: backend.setEqPreset(index + 3)
                        }
                    }
                }
            }

            // ── 自定义 ───────────────────────────────
            Card {
                width: parent.width
                title: "自定义 EQ"
                subtitle: "拖动滑杆 → 应用（写入 mode 15）"

                Row {
                    spacing: 12
                    Pill {
                        text: page.dirty ? "有未应用的改动" : "与耳机一致"
                        tone: page.dirty ? Theme.accent : Theme.textDim
                    }
                    Item { width: Math.max(0, chart2.width - 300); height: 1 }
                    ActionButton {
                        small: true
                        text: "重置为 0"
                        enabled: backend.ready
                        onClicked: {
                            var z = []
                            for (var i = 0; i < 10; ++i) z.push(0)
                            page.localGains = z
                            page.dirty = true
                        }
                    }
                    ActionButton {
                        text: "应用"
                        accent: true
                        enabled: backend.ready
                        onClicked: {
                            backend.setEqCustom(page.localGains)
                            page.dirty = false
                        }
                    }
                }

                Item {
                    id: chart2
                    width: parent.width
                    height: 188
                    readonly property real chartTop: 22
                    readonly property real h: 142
                    readonly property real scaleW: 34

                    // dB 刻度
                    Repeater {
                        model: [
                            { db: "+10", f: 0.0 },
                            { db: "+5", f: 0.25 },
                            { db: "0", f: 0.5 },
                            { db: "-5", f: 0.75 },
                            { db: "-10", f: 1.0 }
                        ]
                        delegate: Text {
                            required property var modelData
                            x: 0
                            y: chart2.chartTop + modelData.f * chart2.h - 7
                            width: chart2.scaleW - 6
                            horizontalAlignment: Text.AlignRight
                            text: modelData.db
                            color: modelData.f === 0.5 ? Theme.textDim : "#6a6a76"
                            font { family: Theme.monoFont; pixelSize: 10 }
                        }
                    }

                    // 网格线
                    Repeater {
                        model: [0.0, 0.25, 0.5, 0.75, 1.0]
                        delegate: Rectangle {
                            required property var modelData
                            x: chart2.scaleW
                            y: chart2.chartTop + modelData * chart2.h
                            width: chart2.width - chart2.scaleW
                            height: 1
                            color: modelData === 0.5 ? Theme.borderHi : Theme.border
                            opacity: modelData === 0.5 ? 1.0 : 0.7
                        }
                    }

                    RowLayout {
                        x: chart2.scaleW
                        y: 0
                        width: chart2.width - chart2.scaleW
                        height: chart2.height
                        spacing: 6

                        Repeater {
                            model: 10
                            delegate: Column {
                                required property int index
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 2

                                readonly property int gain: page.localGains[index]

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: gain > 0 ? "+" + gain : "" + gain
                                    color: gain > 0 ? Theme.accent : gain < 0 ? Theme.bad : Theme.textDim
                                    font { family: Theme.monoFont; pixelSize: 12 }
                                }

                                Slider {
                                    id: sl
                                    orientation: Qt.Vertical
                                    from: 10
                                    to: -10
                                    stepSize: 1
                                    value: page.localGains[index]
                                    width: 46
                                    height: chart2.h
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    enabled: backend.ready
                                    onMoved: {
                                        var g = page.localGains.slice()
                                        g[index] = value
                                        page.localGains = g
                                        page.dirty = true
                                    }

                                    background: Rectangle {
                                        readonly property real gp: sl.visualPosition
                                        x: sl.leftPadding + sl.availableWidth / 2 - 4
                                        y: sl.topPadding
                                        width: 8
                                        height: sl.availableHeight
                                        radius: 4
                                        color: Theme.cardHi
                                        border.width: 1
                                        border.color: Theme.border
                                        // 从 0dB 位置向当前值填充
                                        Rectangle {
                                            readonly property real a: Math.min(parent.gp, sl.zeroFrac)
                                            readonly property real b: Math.max(parent.gp, sl.zeroFrac)
                                            x: 0
                                            y: a * parent.height
                                            width: parent.width
                                            height: (b - a) * parent.height
                                            radius: 4
                                            color: sl.value >= 0 ? Theme.accent : Theme.bad
                                            Behavior on y { NumberAnimation { duration: 90 } }
                                        }
                                    }
                                    handle: Rectangle {
                                        readonly property real gp: sl.visualPosition
                                        x: sl.leftPadding + sl.availableWidth / 2 - 11
                                        y: sl.topPadding + gp * (sl.availableHeight - height)
                                        width: 22
                                        height: 22
                                        radius: 11
                                        color: sl.pressed ? Theme.accent : "#e6e6ec"
                                        border.width: 2
                                        border.color: Theme.card
                                        Rectangle { anchors.centerIn: parent; width: 8; height: 2; radius: 1; color: "#6a6a76" }
                                    }
                                }

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: page.bands[index]
                                    color: Theme.textDim
                                    font { family: Theme.monoFont; pixelSize: 11 }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

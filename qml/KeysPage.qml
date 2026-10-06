import QtQuick

Item {
    id: page
    property var backend

    // 设备端 8 个键位的顺序（由 App 左耳默认映射反推，4/4 吻合）：
    //   1 左·点1下  2 右·点1下  3 左·点2下  4 右·点2下
    //   5 左·点3下  6 右·点3下  7 左·按住2秒 8 右·按住2秒
    readonly property var leftKeys: [
        { index: 1, gesture: "点 1 下", icon: "①" },
        { index: 3, gesture: "点 2 下", icon: "②" },
        { index: 5, gesture: "点 3 下", icon: "③" },
        { index: 7, gesture: "按住 2 秒", icon: "◎" }
    ]
    readonly property var rightKeys: [
        { index: 2, gesture: "点 1 下", icon: "①" },
        { index: 4, gesture: "点 2 下", icon: "②" },
        { index: 6, gesture: "点 3 下", icon: "③" },
        { index: 8, gesture: "按住 2 秒", icon: "◎" }
    ]

    readonly property var actionOptions: [
        { group: "媒体控制", icon: "▶", text: "播放 / 暂停", value: 7 },
        { group: "媒体控制", icon: "⏮", text: "上一首", value: 3 },
        { group: "媒体控制", icon: "⏭", text: "下一首", value: 4 },
        { group: "媒体控制", icon: "＋", text: "音量加", value: 5 },
        { group: "媒体控制", icon: "－", text: "音量减", value: 6 },
        { group: "耳机功能", icon: "◐", text: "降噪设置（切换降噪/通透）", value: 9 },
        { group: "耳机功能", icon: "⚡", text: "低延时开关", value: 8 },
        { group: "耳机功能", icon: "◉", text: "语音助手", value: 2 },
        { group: "关闭", icon: "⊘", text: "无操作", value: 0 }
    ]

    function actionOf(index) {
        var ks = page.backend.keys
        for (var i = 0; i < ks.length; ++i)
            if (ks[i].index === index) return ks[i].action
        return -1
    }

    function actionText(action) {
        for (var i = 0; i < actionOptions.length; ++i)
            if (actionOptions[i].value === action) return actionOptions[i].text
        return "—"
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

            Card {
                width: parent.width
                title: "自定义按键"
                subtitle: "改完立即下发到耳机；下拉项按「媒体控制 / 耳机功能 / 关闭」分组，带图标"

                Row {
                    spacing: 12
                    ActionButton {
                        small: true
                        text: "读取按键"
                        enabled: backend.ready
                        onClicked: backend.readKeys()
                    }
                    Pill {
                        text: backend.keys.length > 0 ? "已读取 " + backend.keys.length + " 项" : "未读取"
                        tone: backend.keys.length > 0 ? Theme.accent : Theme.textDim
                    }
                    Pill {
                        text: "左耳点1下=" + page.actionText(page.actionOf(1)) + " · 右耳点1下=" + page.actionText(page.actionOf(2))
                        tone: Theme.textDim
                    }
                }

                Row {
                    spacing: 22

                    Repeater {
                        model: [
                            { title: "左耳", glyph: "L", keys: page.leftKeys, tint: Theme.accent },
                            { title: "右耳", glyph: "R", keys: page.rightKeys, tint: Theme.good }
                        ]
                        delegate: Column {
                            required property var modelData
                            spacing: 10

                            Row {
                                spacing: 8
                                Rectangle {
                                    width: 22; height: 22; radius: 11
                                    color: Qt.rgba(modelData.tint.r, modelData.tint.g, modelData.tint.b, 0.16)
                                    border.width: 1
                                    border.color: Qt.rgba(modelData.tint.r, modelData.tint.g, modelData.tint.b, 0.5)
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.glyph
                                        color: modelData.tint
                                        font { family: Theme.uiFont; pixelSize: 12; weight: Font.DemiBold }
                                    }
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.title
                                    color: Theme.text
                                    font { family: Theme.uiFont; pixelSize: 14; weight: Font.DemiBold }
                                }
                            }

                            Repeater {
                                model: modelData.keys
                                delegate: Row {
                                    required property var modelData
                                    spacing: 10

                                    Text {
                                        width: 20
                                        horizontalAlignment: Text.AlignHCenter
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.icon
                                        color: Theme.textDim
                                        font { family: Theme.uiFont; pixelSize: 13 }
                                    }
                                    Text {
                                        width: 76
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.gesture
                                        color: Theme.textDim
                                        font { family: Theme.uiFont; pixelSize: 13 }
                                    }
                                    Picker {
                                        id: pk
                                        width: 250
                                        enabled: backend.ready
                                        options: page.actionOptions
                                        currentValue: page.actionOf(modelData.index)
                                        onSelected: (v) => backend.setKeyAction(modelData.index, v)
                                        // 调试/出图用：--openpick 时自动展开“左耳 点1下”的下拉
                                        Component.onCompleted: {
                                            if (modelData.index === 1 && Qt.application.arguments.indexOf("--openpick") !== -1)
                                                Qt.callLater(pk.openPopup)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Card {
                width: parent.width
                title: "手势说明"
                subtitle: "不同型号的按键数量可能不同；若某个键位改了没反应，说明该键位在你这副耳机上不存在"

                Text {
                    width: parent.width
                    text: "点 1 下 / 点 2 下 / 点 3 下 = 连续轻触 1～3 次；按住 2 秒 = 长按。\n" +
                          "默认值：左耳点 1 下 音量加、点 2 下 播放暂停、点 3 下 音量减、长按 降噪切换；\n" +
                          "右耳点 1 下 下一首、点 2 下 播放暂停、点 3 下 上一首、长按 降噪切换。"
                    color: Theme.textDim
                    wrapMode: Text.WordWrap
                    font { family: Theme.uiFont; pixelSize: 12 }
                    lineHeight: 1.35
                }
            }
        }
    }
}

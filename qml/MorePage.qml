import QtQuick
import QtQuick.Controls.Basic

Item {
    id: page
    property var backend
    property bool confirmReset: false

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
                title: "播放控制"
                subtitle: "直接控制耳机当前播放（等价于按键操作）"

                Row {
                    spacing: 10
                    ActionButton { text: "音量 −"; enabled: backend.ready; onClicked: backend.musicControl(0) }
                    ActionButton { text: "播放 / 暂停"; enabled: backend.ready; onClicked: backend.musicControl(backend.playState === 1 ? 3 : 2) }
                    ActionButton { text: "音量 +"; enabled: backend.ready; onClicked: backend.musicControl(1) }
                    ActionButton { text: "上一首"; enabled: backend.ready; onClicked: backend.musicControl(4) }
                    ActionButton { text: "下一首"; enabled: backend.ready; onClicked: backend.musicControl(5) }
                }
                Row {
                    spacing: 10
                    Pill { text: "播放状态 " + (backend.playState < 0 ? "—" : backend.playState); tone: Theme.textDim }
                    Pill { text: "语言 " + (backend.language < 0 ? "—" : backend.language); tone: Theme.textDim }
                }
            }

            Card {
                width: parent.width
                title: "语言 / 提示音语言"
                subtitle: "切换耳机内置语音提示的语言"

                Segmented {
                    enabled: backend.ready
                    currentValue: backend.language
                    options: [
                        { text: "中文", value: 0 },
                        { text: "English", value: 1 }
                    ]
                    onSelected: (v) => backend.setLanguage(v)
                }
            }

            Card {
                width: parent.width
                title: "危险操作"
                subtitle: "恢复出厂会清空耳机里的配对记录与自定义设置"

                Row {
                    spacing: 10
                    ActionButton {
                        text: confirmReset ? "确认恢复出厂" : "恢复出厂设置"
                        accent: confirmReset
                        enabled: backend.ready
                        onClicked: {
                            if (page.confirmReset) {
                                backend.factoryReset()
                                page.confirmReset = false
                            } else {
                                page.confirmReset = true
                                resetTimer.restart()
                            }
                        }
                    }
                    ActionButton {
                        text: "取消"
                        visible: page.confirmReset
                        onClicked: {
                            page.confirmReset = false
                            resetTimer.stop()
                        }
                    }
                }
                Timer {
                    id: resetTimer
                    interval: 6000
                    onTriggered: page.confirmReset = false
                }
            }
        }
    }
}

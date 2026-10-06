import QtQuick

Item {
    id: page
    property var backend
    property int selected: -1

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.height + 20
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            width: parent.width - 8
            x: 4
            spacing: 14

            Card {
                width: parent.width
                title: "耳机记录的配对设备"
                subtitle: "「已连接」表示耳机当前正与该设备保持连接；删除记录会让耳机忘掉该设备"

                Row {
                    spacing: 12
                    ActionButton {
                        small: true
                        text: "读取列表"
                        enabled: backend.ready
                        onClicked: backend.readPairList()
                    }
                    ActionButton {
                        small: true
                        text: "进入配对模式"
                        enabled: backend.ready
                        onClicked: backend.startPairing()
                    }
                }

                ListView {
                    id: list
                    width: parent.width
                    height: Math.max(52, Math.min(280, contentHeight))
                    clip: true
                    model: backend.pairList
                    spacing: 6
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: list.width
                        height: 52
                        radius: 10
                        color: page.selected === index ? Theme.accentDim : (hov.containsMouse ? Theme.cardHi : "transparent")
                        border.width: 1
                        border.color: page.selected === index ? Theme.accent : Theme.border

                        Text {
                            anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                            width: 260
                            elide: Text.ElideRight
                            text: modelData.name
                            color: Theme.text
                            font { family: Theme.uiFont; pixelSize: 14 }
                        }
                        Text {
                            anchors { left: parent.left; leftMargin: 290; verticalCenter: parent.verticalCenter }
                            text: modelData.mac
                            color: Theme.textDim
                            font { family: Theme.monoFont; pixelSize: 12 }
                        }
                        Pill {
                            anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                            text: modelData.connected ? "已连接" : "—"
                            tone: modelData.connected ? Theme.good : Theme.textDim
                        }
                        MouseArea {
                            id: hov
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: page.selected = index
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: list.count === 0
                        text: backend.ready ? "点「读取列表」获取" : "未连接"
                        color: Theme.textDim
                        font { family: Theme.uiFont; pixelSize: 13 }
                    }
                }

                Row {
                    spacing: 10
                    ActionButton {
                        text: "连接"
                        small: true
                        enabled: backend.ready && page.selected >= 0
                        onClicked: backend.pairAction(5, backend.pairList[page.selected].mac)
                    }
                    ActionButton {
                        text: "断开"
                        small: true
                        enabled: backend.ready && page.selected >= 0
                        onClicked: backend.pairAction(4, backend.pairList[page.selected].mac)
                    }
                    ActionButton {
                        text: "删除记录"
                        small: true
                        enabled: backend.ready && page.selected >= 0
                        onClicked: backend.pairAction(6, backend.pairList[page.selected].mac)
                    }
                }
            }
        }
    }
}

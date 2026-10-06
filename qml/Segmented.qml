import QtQuick

// 分段选择器（滑块高亮）
Item {
    id: root
    property var options: []        // [{text, value}]
    property int currentValue: -1
    property var currentIndex: {
        for (var i = 0; i < options.length; ++i)
            if (options[i].value === currentValue) return i
        return -1
    }
    signal selected(int value)

    implicitWidth: row.implicitWidth + 8
    implicitHeight: 34
    opacity: enabled ? 1.0 : 0.5

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Theme.bgElev
        border.width: 1
        border.color: Theme.border
    }

    Rectangle {
        id: highlight
        visible: root.currentIndex >= 0
        x: (row.children[root.currentIndex] ? row.children[root.currentIndex].x : 4) + 4
        y: 4
        width: row.children[root.currentIndex] ? row.children[root.currentIndex].width : 0
        height: parent.height - 8
        radius: 7
        color: Theme.accentDim
        border.width: 1
        border.color: Theme.accent
        Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
    }

    Row {
        id: row
        x: 4
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0
        Repeater {
            model: root.options
            delegate: Item {
                required property var modelData
                width: Math.max(58, label.implicitWidth + 24)
                height: root.height
                Text {
                    id: label
                    anchors.centerIn: parent
                    text: modelData.text
                    color: root.currentValue === modelData.value ? Theme.accent : Theme.textDim
                    font { family: Theme.uiFont; pixelSize: 12; weight: root.currentValue === modelData.value ? Font.DemiBold : Font.Normal }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    enabled: root.enabled
                    onClicked: {
                        root.currentValue = modelData.value
                        root.selected(modelData.value)
                    }
                }
            }
        }
    }
}

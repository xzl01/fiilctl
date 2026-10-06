import QtQuick
import QtQuick.Controls.Basic

// 轻量下拉选择器：支持 { icon, text, value, group } 形式的选项，group 会渲染为分组标题
Item {
    id: root
    property var options: []
    property int currentValue: -1
    property int currentIndex: {
        for (var i = 0; i < options.length; ++i)
            if (options[i].value === currentValue) return i
        return -1
    }
    signal selected(int value)

    function openPopup() { popup.open() }

    implicitWidth: Math.max(140, box.implicitWidth + 42)
    implicitHeight: 34
    opacity: enabled ? 1.0 : 0.5

    Rectangle {
        id: box
        anchors.fill: parent
        radius: 10
        color: mouse.containsMouse ? Theme.cardHi : Theme.bgElev
        border.width: 1
        border.color: mouse.containsMouse ? Theme.borderHi : Theme.border
        Behavior on color { ColorAnimation { duration: 120 } }

        Row {
            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
            spacing: 8
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.currentIndex >= 0 && (root.options[root.currentIndex].icon || "") !== ""
                text: root.currentIndex >= 0 ? (root.options[root.currentIndex].icon || "") : ""
                color: Theme.accent
                font { family: Theme.uiFont; pixelSize: 13 }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.currentIndex >= 0 ? root.options[root.currentIndex].text : "—"
                color: Theme.text
                font { family: Theme.uiFont; pixelSize: 13 }
            }
        }
        Text {
            anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
            text: "▾"
            color: Theme.textDim
            font.pixelSize: 12
        }
        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: root.enabled
            onClicked: popup.visible ? popup.close() : popup.open()
        }
    }

    Popup {
        id: popup
        y: root.height + 6
        width: Math.max(root.width, 280)
        padding: 6
        modal: false
        focus: true
        background: Rectangle {
            radius: 12
            color: Theme.cardHi
            border.width: 1
            border.color: Theme.borderHi
        }
        contentItem: ListView {
            implicitHeight: Math.min(340, contentHeight)
            clip: true
            model: root.options
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Column {
                id: item
                required property var modelData
                required property int index
                width: ListView.view.width
                spacing: 0

                readonly property bool showGroup: modelData.group !== undefined
                    && (index === 0 || root.options[index - 1].group !== modelData.group)

                Text {
                    width: parent.width
                    height: item.showGroup ? 24 : 0
                    visible: item.showGroup
                    leftPadding: 12
                    verticalAlignment: Text.AlignVCenter
                    text: modelData.group || ""
                    color: Theme.textDim
                    font { family: Theme.uiFont; pixelSize: 11 }
                }

                Rectangle {
                    width: parent.width
                    height: 34
                    radius: 8
                    color: hov.containsMouse ? Theme.accentDim : "transparent"

                    Row {
                        anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                        spacing: 10
                        Text {
                            width: 18
                            horizontalAlignment: Text.AlignHCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.icon || ""
                            color: root.currentValue === modelData.value ? Theme.accent : Theme.textDim
                            font { family: Theme.uiFont; pixelSize: 13 }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.text
                            color: root.currentValue === modelData.value ? Theme.accent : Theme.text
                            font { family: Theme.uiFont; pixelSize: 13 }
                        }
                    }

                    MouseArea {
                        id: hov
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentValue = modelData.value
                            root.selected(modelData.value)
                            popup.close()
                        }
                    }
                }
            }
        }
    }
}

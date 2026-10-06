import QtQuick

// 现代开关（带滑动动画）
Item {
    id: root
    property bool checked: false
    property string label: ""
    property bool enabledState: true
    signal toggled(bool value)

    implicitWidth: row.implicitWidth
    implicitHeight: 34
    opacity: enabledState ? 1.0 : 0.45

    Row {
        id: row
        spacing: 10
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            id: track
            width: 44
            height: 24
            radius: 12
            anchors.verticalCenter: parent.verticalCenter
            color: root.checked ? Theme.accent : Theme.cardHi
            border.width: 1
            border.color: root.checked ? Theme.accent : Theme.borderHi
            Behavior on color { ColorAnimation { duration: 160 } }

            Rectangle {
                id: knob
                width: 18
                height: 18
                radius: 9
                y: 3
                x: root.checked ? track.width - width - 3 : 3
                color: root.checked ? "#1a1a1a" : "#d8d8e0"
                Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                enabled: root.enabledState
                onClicked: {
                    root.checked = !root.checked
                    root.toggled(root.checked)
                }
            }
        }

        Text {
            text: root.label
            color: Theme.text
            anchors.verticalCenter: parent.verticalCenter
            font { family: Theme.uiFont; pixelSize: 13 }
            visible: root.label.length > 0
        }
    }
}

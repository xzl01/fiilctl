import QtQuick

Rectangle {
    id: card
    default property alias content: body.data
    property string title: ""
    property string subtitle: ""

    color: Theme.card
    radius: Theme.radius
    border.width: 1
    border.color: Theme.border
    implicitHeight: layout.implicitHeight + 32

    Column {
        id: layout
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
        spacing: 12

        Column {
            id: head
            width: parent.width
            spacing: 2
            visible: card.title.length > 0
            Text {
                text: card.title
                color: Theme.text
                font { family: Theme.uiFont; pixelSize: 15; weight: Font.DemiBold }
            }
            Text {
                text: card.subtitle
                visible: card.subtitle.length > 0
                color: Theme.textDim
                font { family: Theme.uiFont; pixelSize: 12 }
                width: parent.width
                wrapMode: Text.WordWrap
            }
        }

        Column {
            id: body
            width: parent.width
            spacing: 10
        }
    }
}

import QtQuick

Rectangle {
    id: pill
    property string text: ""
    property color tone: Theme.textDim
    implicitWidth: label.implicitWidth + 20
    implicitHeight: 24
    radius: 12
    color: Qt.rgba(tone.r, tone.g, tone.b, 0.14)
    border.width: 1
    border.color: Qt.rgba(tone.r, tone.g, tone.b, 0.35)

    Text {
        id: label
        anchors.centerIn: parent
        text: pill.text
        color: pill.tone
        font { family: Theme.uiFont; pixelSize: 11; weight: Font.DemiBold }
    }
}

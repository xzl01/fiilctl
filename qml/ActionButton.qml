import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control
    property bool accent: false
    property bool small: false

    implicitHeight: small ? 30 : 36
    implicitWidth: Math.max(small ? 68 : 88, labelText.implicitWidth + (small ? 22 : 30))
    enabled: true

    background: Rectangle {
        radius: 10
        color: !control.enabled ? "#1a1a1f"
             : control.accent ? (control.down ? Qt.darker(Theme.accent, 1.15) : Theme.accent)
             : (control.hovered ? Theme.cardHi : Theme.card)
        border.width: 1
        border.color: !control.enabled ? Theme.border
                    : control.accent ? Theme.accent
                    : (control.hovered ? Theme.borderHi : Theme.border)
        Behavior on color { ColorAnimation { duration: 110 } }
    }

    contentItem: Text {
        id: labelText
        text: control.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: !control.enabled ? Theme.textDim : (control.accent ? "#161616" : Theme.text)
        font {
            family: Theme.uiFont
            pixelSize: control.small ? 12 : 13
            weight: control.accent ? Font.DemiBold : Font.Normal
        }
    }
}

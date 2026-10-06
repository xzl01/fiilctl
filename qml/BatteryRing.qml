import QtQuick
import QtQuick.Shapes

// 环形电量（270° 弧）
Item {
    id: root
    property int level: -1          // -1 = 未知
    property bool charging: false
    property string caption: ""

    implicitWidth: 118
    implicitHeight: 118

    readonly property real sweep: 270
    readonly property real frac: root.level < 0 ? 0 : Math.max(0, Math.min(100, root.level)) / 100
    readonly property color ringColor: root.level < 0 ? Theme.borderHi
                                      : root.charging ? Theme.good
                                      : root.frac > 0.2 ? Theme.accent : Theme.bad

    Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4
        ShapePath {
            strokeWidth: 9
            strokeColor: Theme.cardHi
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.width / 2 - 7; radiusY: root.height / 2 - 7
                startAngle: 135; sweepAngle: root.sweep
            }
        }
        ShapePath {
            strokeWidth: 9
            strokeColor: root.ringColor
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.width / 2 - 7; radiusY: root.height / 2 - 7
                startAngle: 135; sweepAngle: root.sweep * root.frac
                Behavior on sweepAngle { NumberAnimation { duration: 420; easing.type: Easing.OutCubic } }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.level < 0 ? "--" : root.level + ""
            color: Theme.text
            font { family: Theme.uiFont; pixelSize: 26; weight: Font.DemiBold }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.charging ? "⚡ 充电中" : root.caption
            color: root.charging ? Theme.good : Theme.textDim
            font { family: Theme.uiFont; pixelSize: 11 }
        }
    }
}

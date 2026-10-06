import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Fiilctl.Backend 1.0

ApplicationWindow {
    id: win
    width: 1020
    height: 800
    minimumWidth: 900
    minimumHeight: 640
    visible: true
    color: Theme.bg
    title: "FIIL 耳机控制台"

    property int navIndex: 0
    property int completedCount: 0
    property var logLines: []

    FiilView { id: backend }

    Connections {
        target: backend
        function onLogLine(line) {
            var arr = win.logLines.slice()
            arr.push(line)
            if (arr.length > 800) arr = arr.slice(arr.length - 800)
            win.logLines = arr
            if (logView.autoScroll) logView.positionViewAtEnd()
        }
    }

    // 关闭时主动释放 SPP（耳机这条通道一次只接受一个客户端）
    Component.onDestruction: backend.disconnectDevice()

    Component.onCompleted: {
        console.error("[qml] Main completed #" + (++win.completedCount))
        backend.refreshDevices()
        // 截图/演示模式：自动连第一台 FIIL（仅当传入 --autoconnect）
        if (Qt.application.arguments.indexOf("--autoconnect") !== -1) {
            for (var i = 0; i < backend.devices.length; ++i) {
                if (backend.devices[i].name.indexOf("FIIL") >= 0) {
                    console.error("[qml] autoconnect -> " + backend.devices[i].address)
                    backend.connectDevice(backend.devices[i].address, 6)
                    break
                }
            }
        }
    }

    // ── 顶栏 ─────────────────────────────────────────────
    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 60
        color: Theme.bgElev
        Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 1; color: Theme.border }

        Row {
            anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
            spacing: 10
            Rectangle { width: 9; height: 9; radius: 5; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: "FIIL 控制台"
                color: Theme.text
                font { family: Theme.uiFont; pixelSize: 17; weight: Font.DemiBold }
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Row {
            anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
            spacing: 10

            Picker {
                id: devicePicker
                width: 300
                enabled: !backend.connected
                options: {
                    var out = []
                    var ds = backend.devices
                    for (var i = 0; i < ds.length; ++i)
                        out.push({ text: ds[i].name + (ds[i].connected ? " · 已连接" : "") + "  " + ds[i].address, value: i })
                    return out
                }
                property int picked: 0
                currentValue: picked
                onSelected: (v) => picked = v
                Component.onCompleted: backend.refreshDevices()
            }

            ActionButton {
                small: true
                text: "刷新"
                enabled: !backend.connected
                onClicked: {
                    devicePicker.picked = 0
                    backend.refreshDevices()
                }
            }

            ActionButton {
                id: connectBtn
                text: backend.connected ? "断开" : "连接"
                accent: !backend.connected
                onClicked: {
                    console.error("[qml] connect button clicked")
                    if (backend.connected) {
                        backend.disconnectDevice()
                    } else {
                        var ds = backend.devices
                        if (ds.length === 0) return
                        var idx = Math.min(devicePicker.picked, ds.length - 1)
                        backend.connectDevice(ds[idx].address, 6)
                    }
                }
            }

            Pill {
                anchors.verticalCenter: parent.verticalCenter
                text: backend.status
                tone: backend.connected ? Theme.good : Theme.textDim
            }
        }
    }

    // ── 左侧导航 ─────────────────────────────────────────
    Rectangle {
        id: rail
        anchors { left: parent.left; top: header.bottom; bottom: parent.bottom }
        width: 104
        color: Theme.bgElev
        Rectangle { anchors { right: parent.right; top: parent.top; bottom: parent.bottom } width: 1; color: Theme.border }

        Column {
            anchors { top: parent.top; topMargin: 14; left: parent.left; right: parent.right }
            spacing: 6

            Repeater {
                model: [
                    { glyph: "◍", label: "概览" },
                    { glyph: "♪", label: "调音" },
                    { glyph: "⌨", label: "按键" },
                    { glyph: "⛓", label: "配对" },
                    { glyph: "≡", label: "更多" }
                ]
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    x: 10
                    width: rail.width - 20
                    height: 56
                    radius: 12
                    color: win.navIndex === index ? Theme.accentDim : (hov.containsMouse ? Theme.cardHi : "transparent")
                    Behavior on color { ColorAnimation { duration: 140 } }

                    Rectangle {
                        visible: win.navIndex === index
                        width: 3
                        height: 22
                        radius: 2
                        color: Theme.accent
                        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    }
                    Column {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData.glyph
                            color: win.navIndex === index ? Theme.accent : Theme.textDim
                            font { family: Theme.uiFont; pixelSize: 17 }
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData.label
                            color: win.navIndex === index ? Theme.text : Theme.textDim
                            font { family: Theme.uiFont; pixelSize: 12 }
                        }
                    }
                    MouseArea {
                        id: hov
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: win.navIndex = index
                    }
                }
            }
        }
    }

    // ── 内容区 ───────────────────────────────────────────
    Item {
        id: content
        anchors {
            left: rail.right; leftMargin: 16
            right: parent.right; rightMargin: 16
            top: header.bottom; topMargin: 16
            bottom: logPanel.top; bottomMargin: 12
        }

        StackLayout {
            id: stack
            anchors.fill: parent
            currentIndex: win.navIndex

            OverviewPage { backend: backend }
            EqPage { backend: backend }
            KeysPage { backend: backend }
            PairPage { backend: backend }
            MorePage { backend: backend }
        }
    }

    // ── 日志面板 ─────────────────────────────────────────
    Rectangle {
        id: logPanel
        anchors { left: rail.right; leftMargin: 16; right: parent.right; rightMargin: 16; bottom: parent.bottom; bottomMargin: 12 }
        height: logView.collapsed ? 44 : 200
        radius: Theme.radius
        color: Theme.card
        border.width: 1
        border.color: Theme.border
        Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

        Row {
            id: logHeader
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            height: 20
            spacing: 10
            Text {
                text: "协议日志"
                color: Theme.text
                font { family: Theme.uiFont; pixelSize: 13; weight: Font.DemiBold }
                anchors.verticalCenter: parent.verticalCenter
            }
            Pill { text: win.logLines.length + " 行"; tone: Theme.textDim }
            Item { width: 1; height: 1 }
            ActionButton {
                small: true
                text: "清空"
                onClicked: win.logLines = []
            }
            Text {
                text: "自动滚动"
                color: Theme.textDim
                anchors.verticalCenter: parent.verticalCenter
                font { family: Theme.uiFont; pixelSize: 12 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: logView.autoScroll = !logView.autoScroll
                }
            }
            Rectangle {
                width: 16; height: 16; radius: 8
                color: logView.autoScroll ? Theme.accent : Theme.cardHi
                border.width: 1; border.color: Theme.borderHi
                anchors.verticalCenter: parent.verticalCenter
            }
            Item { width: parent.width - 480; height: 1 }
            ActionButton {
                small: true
                text: logView.collapsed ? "展开" : "收起"
                onClicked: logView.collapsed = !logView.collapsed
            }
        }

        ListView {
            id: logView
            property bool autoScroll: true
            property bool collapsed: true
            anchors { left: parent.left; right: parent.right; top: logHeader.bottom; bottom: parent.bottom; margins: 12 }
            topMargin: 6
            clip: true
            visible: !collapsed
            model: win.logLines
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Text {
                required property var modelData
                width: logView.width
                text: modelData
                color: modelData.indexOf("!! ") === 0 ? Theme.bad
                     : modelData.indexOf("TX") >= 0 ? Theme.accent
                     : modelData.indexOf("通知") >= 0 ? Theme.good
                     : Theme.textDim
                font { family: Theme.monoFont; pixelSize: 11 }
                elide: Text.ElideRight
            }
        }
    }
}

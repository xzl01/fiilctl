import QtQuick
import Fiilctl.Backend 1.0

Item {
    id: page
    property var backend

    Column {
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 4 }
        spacing: 14

        // 电量
        Card {
            width: parent.width
            title: "电量"
            subtitle: backend.deviceName.length > 0 ? backend.deviceName : ""

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 28
                BatteryRing { level: backend.leftLevel; charging: backend.leftCharging; caption: "左耳机" }
                BatteryRing { level: backend.rightLevel; charging: backend.rightCharging; caption: "右耳机" }
                BatteryRing { level: backend.boxLevel; charging: backend.boxCharging; caption: "充电盒" }
            }

            Row {
                spacing: 10
                Pill { text: backend.firmware.length > 0 ? "固件 " + backend.firmware : "固件 —"; tone: Theme.textDim }
                Pill { text: backend.pid.length > 0 ? "PID " + backend.pid : "PID —"; tone: Theme.textDim }
                Pill {
                    visible: backend.unsupported.length > 0
                    text: "不支持 " + backend.unsupported
                    tone: Theme.bad
                }
            }
        }

        // 快捷开关
        Card {
            width: parent.width
            title: "快捷开关"

            Flow {
                width: parent.width
                spacing: 22
                Toggle {
                    label: "低延时模式"
                    enabledState: backend.ready
                    checked: backend.lowLatency === 1
                    onToggled: (v) => backend.setLowLatency(v)
                }
                Toggle {
                    label: "单耳降噪"
                    enabledState: backend.ready
                    checked: backend.singleNoise === 1
                    onToggled: (v) => backend.setSingleNoise(v)
                }
                Toggle {
                    label: "LDAC 高清解码"
                    enabledState: backend.ready
                    checked: backend.ldac === 1
                    onToggled: (v) => backend.setLdac(v)
                }
                Toggle {
                    label: "蓝牙畅连（多设备）"
                    enabledState: backend.ready
                    checked: backend.multipoint === 1
                    onToggled: (v) => backend.setMultipoint(v)
                }
            }
        }

        // 自适应 / 调音
        Card {
            width: parent.width
            title: "自适应与调音"
            subtitle: "耳机根据佩戴/环境自动调整；云感调音需先关闭降噪（MAF）"

            Flow {
                width: parent.width
                spacing: 22
                Toggle {
                    label: "EQ 自适应"
                    enabledState: backend.ready
                    checked: backend.eqAdaptive === 1
                    onToggled: (v) => backend.setEqAdaptive(v)
                }
                Toggle {
                    label: "入耳自适应"
                    enabledState: backend.ready
                    checked: backend.earAdaptive === 1
                    onToggled: (v) => backend.setEarAdaptive(v)
                }
                Toggle {
                    label: "降噪自适应"
                    enabledState: backend.ready
                    checked: backend.noiseAdaptive === 1
                    onToggled: (v) => backend.setNoiseAdaptive(v)
                }
                Toggle {
                    label: "风噪自适应"
                    enabledState: backend.ready
                    checked: backend.windAdaptive === 1
                    onToggled: (v) => backend.setWindAdaptive(v)
                }
                Toggle {
                    label: "云感调音"
                    enabledState: backend.ready && backend.mafMode !== 1
                    checked: backend.tuning === 1
                    onToggled: (v) => backend.setTuning(v)
                }
            }
        }

        // 音效
        Card {
            width: parent.width
            title: "音效"

            Row {
                spacing: 16
                Text {
                    text: "语音提示音量"
                    color: Theme.textDim
                    anchors.verticalCenter: parent.verticalCenter
                    font { family: Theme.uiFont; pixelSize: 13 }
                }
                Segmented {
                    enabled: backend.ready
                    currentValue: backend.tone
                    options: [
                        { text: "静音", value: 0 },
                        { text: "中", value: 1 },
                        { text: "低", value: 2 },
                        { text: "高", value: 3 }
                    ]
                    onSelected: (v) => backend.setTone(v)
                }
            }
            Row {
                spacing: 16
                Text {
                    text: "降噪模式"
                    color: Theme.textDim
                    anchors.verticalCenter: parent.verticalCenter
                    font { family: Theme.uiFont; pixelSize: 13 }
                }
                Segmented {
                    enabled: backend.ready
                    currentValue: backend.mafMode
                    options: [
                        { text: "关", value: 0 },
                        { text: "降噪开", value: 1 },
                        { text: "通透", value: 2 }
                    ]
                    onSelected: (v) => backend.setMafMode(v)
                }
                Item { width: 1; height: 1 }
            }
        }
    }
}

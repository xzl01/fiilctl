# FIIL Key Pro2 · 私有控制协议（从 Android App + HCI 抓包还原）

对象：`FIIL Key Pro2`，BD_ADDR `B0:F1:A3:2B:19:A8`（OUI B0-F1-A3 = Fengfan/峰范 = FIIL）
App：`com.fengeek.f002` v3.5.2（Fengeek 代工，内含 SDK）
芯片方案：**Bluetrum 中科蓝讯**（App 内 `com.bluetrum.devicemanager.*`；UI 实现类 `com.fengeek.main.i.d.e` = `FIILF045Command`）

## 1. 传输层：经典蓝牙 RFCOMM/SPP（**不是 BLE**）

SDP 记录：

```
Service Record Handle: 0x50010003
ServiceClassIDList   : d52b47bc-2dea-42c4-b26d-20252fceeeb3
ProtocolDescriptorList: L2CAP(0x0100) + RFCOMM(0x0003) channel = 6
Bluetooth Profile DescriptorList: Serial Port (0x1101) v0x1102
Attribute 0x0100     : "SPP"
```

- 手机 11 分钟 HCI 日志里 **ATT 包 = 0，LE 连接建立 = 0** → 无 BLE 控制通道
- 一次只接受一个 SPP 客户端（手机 App 占着时 Linux 连不上，反之亦然）

## 2. 帧格式

```
[seq:1][command:1][commandType:1][0x00][len:1][payload:len]  + RFCOMM FCS（内核处理）
```

| command | 名称 | 备注 |
|---|---|---|
| 37 `0x25` | COMMAND_WORK_MODE | payload = `{0/1}`：0=普通，1=游戏/低延时 |
| 39 `0x27` | COMMAND_DEVICE_INFO | 读参数，payload = `{infoId, 0}` |
| 40 `0x28` | COMMAND_NOTIFY | 耳机主动上报，payload = `{infoId, dataLen, data…}` |
| 1 `0x01` | COMMAND_APP_SETTING | payload = `{ctrlType, cnt, value…}` |
| 32 | COMMAND_EQ | `{segmentNum, eqMode, gains…}` |
| 33 | COMMAND_MUSIC_CONTROL | |
| 34 | COMMAND_KEY | `{key, value}` |
| 35 | COMMAND_AUTO_SHUTDOWN | |
| 36 | COMMAND_FACTORY_RESET | |
| 38 | COMMAND_IN_EAR_DETECT | |
| 41 | COMMAND_LANGUAGE | |
| 42 | COMMAND_FIND_DEVICE | |
| 43 | COMMAND_AUTO_ANSWER | |
| 44 `0x2c` | COMMAND_ANC_MODE | payload = `{0=off,1=on,2=transparency}` |
| 45 | COMMAND_BLUETOOTH_NAME | |
| 46 | COMMAND_LED_MODE | |
| 47 | COMMAND_CLEAR_PAIR_RECORD | |
| 48 | COMMAND_ANC_GAIN | |
| 49 | COMMAND_TRANSPARENCY_GAIN | |
| 50 | COMMAND_SOUND_EFFECT_3D | |

commandType：`1` = 请求（主机→耳机），`2` = 响应（耳机→主机），`3` = 通知
seq：请求恒为 `0x00`，响应/通知由耳机递增。

响应格式：`<seq> <cmd> 02 00 <len> <原 payload + 状态/数据>`
例：`00 27 01 00 02 01 00`（读电量）→ `02 27 02 00 05 01 03 64 5a 52`（infoId=1, len=3, 数据 100/90/82）

## 3. 已实测验证的操作（HCI 抓包 ↔ 点击一一对应）

| 手机操作 | 发送帧 | 耳机响应 |
|---|---|---|
| 低延时模式 **开** | `00 25 01 00 01 01` | `04 25 02 00 01 00` |
| 低延时模式 **关** | `00 25 01 00 01 00` | `05 25 02 00 01 00` |
| 单耳降噪 **开** | `00 01 01 00 03 12 01 01` | `0d 01 02 00 03 12 01 00` |
| 单耳降噪 **关** | `00 01 01 00 03 12 01 00` | `0f 01 02 00 03 12 01 00` |
| 提示音音量 = 低 | `00 01 01 00 03 03 01 02` | `00 01 02 00 03 03 01 00` |
| 提示音音量 = 中 | `00 01 01 00 03 03 01 01` | `03 01 02 00 03 03 01 00` |
| （ANC 开，对应 0x2c） | `00 2c 01 00 01 01` | `0b 2c 02 00 01 00` |

## 4. INFO id（`0x27` 读 / `0x28` 通知 的子参数）

从 `com.bluetrum.devicemanager.cmd.Command` + `f39.F39Command`：

```
1  INFO_DEVICE_POWER (电量, 3 字节 L/R/盒)      2  INFO_FIRMWARE_VERSION
3  INFO_BLUETOOTH_NAME                          4  INFO_EQ_SETTING
5  INFO_KEY_SETTINGS                            6  INFO_DEVICE_VOLUME
7  INFO_PLAY_STATE                              8  INFO_WORK_MODE
9  INFO_IN_EAR_STATUS                          10  INFO_LANGUAGE_SETTING
11 INFO_AUTO_ANSWER                            12  INFO_ANC_MODE (f39: DENOISE_MODE)
13 INFO_IS_TWS                                 14  INFO_TWS_CONNECTED
15 INFO_LED_SWITCH                             16  INFO_FW_CHECKSUM
17 INFO_ANC_GAIN                               18  INFO_TRANSPARENCY_GAIN
19 INFO_ANC_GAIN_NUM                           20  INFO_TRANSPARENCY_GAIN_NUM
21 INFO_ALL_EQ_SETTINGS                        22  INFO_MAIN_SIDE
23 INFO_PRODUCT_COLOR                          24  INFO_SOUND_EFFECT_3D
-1 INFO_MAX_PACKET_SIZE       -2 INFO_DEVICE_CAPABILITIES
-112 ?  -113 INFO_LDAC  -114 INFO_LDAC_STATE  -115/-116/-117/-118 LDAC UUID(L/R + 通知)
-119 INFO_ONLINE_SLEEP  -120 INFO_EAR_REACTION  -121 INFO_HALF_IN_EAR_MODE
-122 INFO_MAF_KEY_LONG_PRESS  -123 INFO_NIGHT_RUNNING  -124 INFO_PID
-125 INFO_MULTIPLE_PAIR  -126 INFO_PROMPT_TONE  -127 INFO_TONE  -128 INFO_MULTIPLE_DEVICE
```

App 每次连上耳机做的固定开场（读全部参数）：

```
→ 00 27 01 00 04 ff 02 02 34      握手
← 00 27 02 00 03 ff 01 ff
→ 00 27 01 00 02 84 00   ← 84 02 00 31     (INFO_PID)
→ 00 27 01 00 02 01 00   ← 01 03 64 5a 52  (电量 100/90/82)
→ 8f(-113 LDAC) / 82(提示音) / 08(工作模式) / 80(多设备) / 04(EQ) / 07(播放状态)
→ 8b(-117) / 0c(ANC) / 8c / 8a(-118) / 89(-119) / 87(-121) / 05(按键) / 90 / 82 …
```

## 5. APP_SETTING control type（`0x01` 命令）

`com.bluetrum.devicemanager.cmd.request.AppSettingRequest`：

```
1  = SETTING_TYPE_MUlTI_DEVICE     3  = SETTING_TYPE_PROMPT_TONE
5  = SETTING_TYPE_NIGHT_RUNNING    6  = SETTING_TYPE_MULTI_CONNECT
7  = SETTING_TYPE_KEY_LONG_PRESS   8  = SETTING_TYPE_HALF_IN_EAR_MODE
```
f39 扩展：9 = OPEN_EAR_REACTION, 10 = SLEEP, 11 = OFFLINE, 12 = ACTIVATE_LDAC, 13 = LDAC
**18 (`0x12`) = 单耳降噪**（实测得到）

## 6. Linux 侧最小可用客户端

```python
import socket
s = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
s.connect(("B0:F1:A3:2B:19:A8", 6))          # SPP channel 6

def frame(cmd, ctype, payload):
    return bytes([0x00, cmd, ctype, 0x00, len(payload)]) + bytes(payload)

s.send(frame(0x27, 0x01, [0xFF, 0x02, 0x02, 0x34]))   # 握手
s.recv(64)
s.send(frame(0x27, 0x01, [0x01, 0x00]))                # 读电量
print(s.recv(64).hex(' '))                              # .. 27 02 00 05 01 03 64 5a 52
s.send(frame(0x25, 0x01, [0x01]))                       # 低延时开
s.send(frame(0x01, 0x01, [0x12, 0x01, 0x01]))           # 单耳降噪开
```

注意：首次连接收到的第一帧可能带一个 `0x8B` 前缀（RFCOMM credit/PF 残留），不是协议头，重连即消失。

## 7. 反编译资料

`jadx --no-res --no-imports -d out fiil.apk`，关键类：

- `com/bluetrum/devicemanager/cmd/Command.java` — 全部 command/INFO 常量
- `com/bluetrum/devicemanager/cmd/request/*.java` — 各命令的 payload 结构
- `com/bluetrum/devicemanager/f39/*` — 该型号扩展（LDAC/降噪…）
- `com/fengeek/main/i/d/e.java` — `FIILF045Command`：App UI 操作 ↔ commandId 的映射（带中文日志，如 `setLowDelay: 低延时状态--->`）
- `com/fengeek/main/i/d/b.java` — 设备操作接口（设备能力总表）

---

# 补充（第二轮挖掘：App 方法名 ↔ 帧字节 全表）

来源：`com.fengeek.main.i.d.{b,e}.java`（都叫 `FIILF045Command`，basic/full 两个变体）+ `com.bluetrum.devicemanager.cmd.Command`。
帧格式回顾：`[seq][classifyId][type][0x00][len][commandId][…data]`，App 里
`setCommandClassifyId(X)` → 帧第 2 字节，`setCommandId(Y)` → payload 第 1 字节。

## INFO 读（classifyId = 39 = 0x27），`00 27 01 00 02 <id> 00`

| id | 有符号 | App 方法 | 含义 |
|---|---|---|---|
| 1 | 1 | getElectricity | **电量**：3 字节 [左,右,盒]，每字节 bit7=充电中，bit0-6=0-100 |
| 2 | 2 | getVersion | 固件版本 |
| 4 | 4 | getEQStatus | EQ 设置 |
| 5 | 5 | getKey | 按键映射 |
| 7 | 7 | getMusicStatus | 播放状态 |
| 8 | 8 | getLowDelayStatus | 工作模式（低延时） |
| 12 | 12 | getMAFStatus | 降噪/MAF 模式 |
| 22 | 22 | getMainEar | 主耳 |
| 24 | 24 | getSpatialAudioStatus | 3D 音效 |
| 128 | -128 | getChangLanStatus | 多设备/蓝牙畅连 |
| 130 | -126 | getToneStatus | 提示音 |
| 132 | -124 | getPID | 产品 ID |
| 138 | -118 | getLeftHeadsetUUID | 左耳 LDAC UUID |
| 139 | -117 | getRightHeadsetUUID | 右耳 LDAC UUID |
| 142 | -114 | getLdacUnlockStatus | LDAC 解锁状态 |
| 143 | -113 | getLdacSwitchStatus | LDAC 开关 |
| **144** | **-112** | **getCodecStatus** | **CODEC 状态（就是之前没解出来的 0x90）** |
| 145 | -111 | getCallState | 通话状态 |
| 135 | -121 | (半入耳) | HALF_IN_EAR_MODE |
| 137 | -119 | (online sleep) | 在线休眠 |
| 255 | -1 | setUnlock | 握手/解锁，payload `{2, 0x34}`（即 `00 27 01 00 04 ff 02 02 34`） |

## 写（classifyId = 1 时是 APP_SETTING，payload `[controlType][count][value…]`）

| controlType | 名称 | App 方法 |
|---|---|---|
| 1 | 多设备（蓝牙畅连） | setChangLan |
| 3 | 提示音音量 | setTone（1=中 2=低，0/3 其余档） |
| 5 | 夜间跑步 | nightRunning |
| 6 | 多连接 | multiConnect |
| 7 | 按键长按 | KeyLongPress |
| 8 | 半入耳模式 | setHalfInEarMode |
| 9 | 开启耳反 | earReaction |
| 10/11 | 睡眠/离线 | setSleep/setOfflineSleep |
| 12 | 激活 LDAC（license） | setActivationLdac |
| 13 | LDAC 开关 | setLdacSwitch |
| **18 (0x12)** | **单耳降噪** | （F045 直接拼 buffer） |

其它写命令：

| classifyId | 名称 | payload |
|---|---|---|
| 37 (0x25) | WORK_MODE 低延时 | `{0/1}` |
| 44 (0x2c) | ANC_MODE | `{0=off,1=on,2=通透}` |
| 32 (0x20) | EQ | `{段数, eqMode, 各段增益…}`（预设 mode=0..14，自定义 = mode+15） |
| 33 (0x21) | 音乐控制 | `{1=音量+ / 0=音量- / 2=播放暂停 / 4=上一首 / 5=下一首}` |
| 34 (0x22) | KEY 按键映射 | `{keyIndex, action}` |
| 35 (0x23) | 自动关机 | |
| 36 (0x24) | 恢复出厂 | |
| 42 (0x2a) | 查找耳机 | |
| 47 (0x2f) | 清除配对记录 | |
| 48 (0x30) | ANC 增益 | |
| 49 (0x31) | 通透增益 | |
| 50 (0x32) | 3D 音效 | |

## 通知（classifyId = 40 = 0x28，payload `[infoId][len][data…]`）

- `01 03 64 5a 52` → 电量：左 100%、右 90%、盒 82%（bit7 表示充电中）
- `05 …` → 按键映射，格式 `[keyIndex][count=1][action]` 重复（实测 8 条：`01 01 05 / 02 01 04 / 03 01 07 / 04 01 07 / 05 01 06 / 06 01 03 / 07 01 09 / 08 01 09`）
- `88(-120) 79 …` → 设备列表类通知：内含 [flags][6 字节 MAC][设备名] 记录（抓到手机名、`CSR - bc7`、耳机自身名 `CRY574Pro`）

## App 内其它型号实现（同协议，能力不同）

`com/fengeek/main/i/d/{b,e}=F045`、`{c}=F046`、`{d}=F047`、`{f}=F049`、`{g}=F051`、`{h}=F053`
（F046 才有 getCodecStatus/SpatialAudio/MAF，F045 的 full 版是 e.java）

---

# 更正 & 型号覆盖（第三轮）

## 重要更正：FIIL Key Pro2 = **F049**，不是 F045

App 内置产品目录 `assets/header_list_zh.json` 把 `headsetModel` → 产品名映射得很清楚：

```
F002 Wireless | F005 Diva Pro/Diva2 Pro | F008 Diva/Diva2 | F017 T1 | F021 CC
F026 T1 XS    | F027 T1 Pro | F028 CC Pro  | F029 T1 Lite | F030 CC2 | F031 CG
F033 CG Pro   | F035 T2 Pro | F037 CC nano | F038 Key | F039 CC Pro2 | F041 Key Pro
F043 GS       | F045 GS Lite | F046 Key Max | F047 Atom | F048 GS Links
F049 Key Pro2 ← 我们这台 | F051 CC3
```

所以 Key Pro2 的真实驱动类是 **`com.fengeek/main/i/d/f.java` = `FIILF049Command`**（不是 e.java/F045=GS Lite，两者协议同族但能力不同）。

## FIILF049Command（Key Pro2）完整表

读（classifyId 39 = 0x27）：

| cmdId | 方法 | 含义 |
|---|---|---|
| 1 | getElectricity | 电量 |
| 2 | getVersion | 版本 |
| 4 | getEQStatus | EQ |
| 5 | getKey | 按键 |
| 7 | getMusicStatus | 播放状态 |
| 8 | getLowDelayStatus | 低延时 |
| 10 | getLanguage | 语言 |
| 12 | getMAF | 降噪模式 |
| 132 (-124) | getPID | 产品 ID |
| -112 (0x90) | **getSingleMAFStatus** | **单耳降噪状态**（之前没解出来的 0x90） |
| -113 (0x8f) | getLdacSwitchStatus | LDAC 开关 |
| -116 (0x8c) | getWindAdaptive | 风噪自适应 |
| -117 (0x8b) | getNoiseAdaptive | 降噪自适应 |
| -118 (0x8a) | getEarAdaptive | 入耳自适应 |
| -119 (0x89) | getEQAdaptive | EQ 自适应 |
| -120 (0x88) | **getBluetoothPairList** | **蓝牙配对列表**（抓到的 128 字节大帧就是它） |
| -121 (0x87) | getTuning | 调音 |
| -126 (0x82) | getToneStatus | 提示音 |
| -128 (0x80) | getChangLanStatus | 畅连/多设备 |
| -1 (0xff) | setUnlock | 握手 `{2, 0x34}` |

写：

| 方法 | classifyId | cmdId | 含义 |
|---|---|---|---|
| setSingle | 1 | **18** | **单耳降噪**（实测帧 `00 01 01 00 03 12 01 0x`） |
| setChangLan | 1 | 1 | 多设备/畅连 |
| setPair/setConnectBluetooth/setDisconnectBluetooth/setDeleteBluetooth | 1 | 6 | 配对列表管理 |
| setTone | 1 | 3 | 提示音音量 |
| setTuning | 1 | 8 | 调音 |
| setLdacSwitch | 1 | 13 | LDAC 开关 |
| setEQAdaptive | 1 | 14 | EQ 自适应 |
| setEarAdaptive | 1 | 15 | 入耳自适应 |
| setNoiseAdaptive | 1 | 16 | 降噪自适应 |
| setWindAdaptive | 1 | 17 | 风噪自适应 |
| setLowDelay | 37 (0x25) | – | 低延时 |
| setMAF | 44 (0x2c) | – | 降噪模式 |
| setEQ | 32 | – | EQ |
| setKey | 34 | – | 按键映射 |
| setLanguage | 41 | – | 语言 |
| setReset | 36 | – | 恢复出厂 |
| 音乐控制 | 33 | 1/0/2/4/5 | 音量±/播放暂停/上下一首 |

## 协议族划分（决定能不能复用这套驱动）

| 族 | 传输 | 型号 | 可用本驱动？ |
|---|---|---|---|
| Bluetrum + FIIL SPP | RFCOMM 通道 6，UUID `d52b47bc-…`（`BluetoothSppService`） | **F045 GS Lite / F046 Key Max / F047 Atom / F048 GS Links / F049 Key Pro2 / F051 CC3**（+代码里 f040/f053 同族） | ✅ 直接可用 |
| BLE 私有通道 | GATT，UUID `3E48516C-47C0-4FE5-AA96-E039B51039D9`，classifyId 2305~2312 | F042 | ❌ 要照 `com.fengeek.main.ble.*` 再解一遍 |
| FIIL 旧协议（GAIA 风格，`com.fiil.sdk`，UI 在 `com.fengeek/f002/**`） | 老 SPP/自定义 | F002 Wireless、F005/F008 Diva*、F017 T1、F021 CC、F026 T1 XS、F027 T1 Pro、F028 CC Pro、F029 T1 Lite、F030 CC2、F031 CG、F033 CG Pro、F035 T2 Pro、F037 CC nano、F038 Key、F039 CC Pro2、F041 Key Pro（~17 款） | ❌ 另一套协议 |
| Airoha（MTK AB15xx） | 私有（PRIM UUID / libab153x-peq） | F043 GS、部分 CC Pro2/老型号 | ❌ |

**判定某副 FIIL 耳机能否用本驱动**：
```bash
bluetoothctl info <MAC> | grep -E "d52b47bc|3E48516C"
```
`d52b47bc` → 直连可用，再逐个 INFO 探测能力（**空响应 `len=0` 表示该参数不支持**）。

---

# 第四轮：帧构造规则 & 新解出的三张表（来自 `com/fengeek/main/i/b.java`）

## 帧构造规则（与实测逐字节吻合）

```java
// 普通命令（b.java write()）
[0x00][classifyId][0x01][0x00]
if (commandId != 0) { [buffer.length + 2] [commandId] }
[buffer.length][buffer...]
```

| App 调用 | classifyId | commandId | buffer | 实际帧 |
|---|---|---|---|---|
| getElectricity | 39 | 1 | {} | `00 27 01 00 02 01 00` |
| setUnlock | 39 | 0xff | {2,0x34} | `00 27 01 00 04 ff 02 02 34` |
| setLowDelay | 37 | 0 | {v} | `00 25 01 00 01 <v>` |
| setMAF | 44 | 0 | {mode} | `00 2c 01 00 01 <mode>` |
| setTone | 1 | 3 | {v} | `00 01 01 00 03 03 01 <v>` |
| setSingle（单耳降噪） | 1 | 18 | {v} | `00 01 01 00 03 12 01 <v>` |
| **setKey** | 34 | key | {action} | **`00 22 01 00 03 <key> 01 <action>`** |
| **setEQ** | 32 | 0 | 12 字节 EQ buffer | **`00 20 01 00 0c <10><mode><gains…>`** |
| **配对管理** | 1 | 6 | {动作, MAC…} | **`00 01 01 00 09 06 07 <动作><MAC6>`**（动作 4=断开 5=连接 6=删除）|
| setPair（进入配对） | 1 | 6 | {1} | `00 01 01 00 03 06 01 01` |

## EQ

- 读：`00 27 01 00 02 04 00` → `… 04 0c 0a <mode> <10 段增益>`（10 段，单位 ≈1dB，范围 -10..+10）
- 预设 mode：**3 + 序号**（见下表），自定义 = **15**，出厂默认 = 0（原声，全 0 增益）
- 预设名（App 资源 `preset_sound_name`，顺序即 mode 顺序）：

| mode | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 名称 | 怀旧 | 爵士 | 轻柔 | 剧院 | 金属 | 流行 | R&B | 舞曲 | 摇滚 | 电子 | 重低 | 律动 |

- 10 段频点（`customize_eq`）：31 / 63 / 125 / 250 / 500 / 1k / 2k / 4k / 8k / 16k
- 实测：`TX 00 20 01 00 0c 0a 00 00×10` → `RX 00 20 02 00 01 00`（成功）

## 按键

- 读：`00 27 01 00 02 05 00` → `… 05 18 [idx][count][action]×8`
- 写：`00 22 01 00 03 <idx> 01 <action>`（实测 idx8: 09→00→09 均成功）
- 动作码（`F049SetFragment.J()` 的 switch）：

| 码 | 0 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| 动作 | 无操作 | 语音助手 | 上一首 | 下一首 | 音量加 | 音量减 | 播放/暂停 | 低延时 | 降噪设置 |

- 键位 1-4 通常对应左耳（点1下/点2下/按2秒/点3下），5-8 对应右耳；默认映射实测为 `5,4,7,7,6,3,9,9`

## 配对列表

- 读：`00 27 01 00 02 88 00` → data = `[条数]` + 条数 × `[标志][MAC 6][名称 23]`（**标志 1 = 当前连接**）
- 实测（笔记本 + 手机已配对时）：`xzl(88:F4:DA:07:C2:CB) / 🕶的REDMI K80 Pro(D4:A3:65:9C:60:C5) / QT68D(00:13:EF:BC:03:1F) / CSR - bc7(00:13:EF:B2:00:1B)`
- 管理：AppSetting type 6，payload `[6][值个数][动作][MAC6]`；动作 4=断开 5=连接 6=删除 1=进入配对模式

## 键位序号 ↔ 手势（反推，实测吻合）

设备返回的按键数组 `00 27 01 00 02 05 00 → … 05 18 [idx][1][action]×8`，实测默认值
`5,4,7,7,6,3,9,9`（音量加/下一首/播放暂停/播放暂停/音量减/上一首/降噪/降噪）。

与 App「左耳按键」页显示的默认值（点1下=音量加、点2下=播放暂停、点3下=音量减、按2秒=降噪设置）比对，
**4/4 吻合的排列**是：

| idx | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| 手势 | 左·点1下 | 右·点1下 | 左·点2下 | 右·点2下 | 左·点3下 | 右·点3下 | 左·按2秒 | 右·按2秒 |

（即先按手势分组，每组内左右耳相邻；写命令仍是 `00 22 01 00 03 <idx> 01 <action>`）

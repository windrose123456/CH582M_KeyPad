# BLE HID 键盘 ENTER/CTRL 幽灵长按排查指南

## 已知事实
- 扫描 printf 值正确 → GPIO 读取没问题
- 注释 USB 初始化后问题依旧 → 问题在 BLE 路径
- 只有 ENTER(PB18) 和 CTRL(PB20) 异常

---

## 排查方向 1：BLE 协议模式（最优先）

BLE HID 同时注册了 Report Protocol 和 Boot Protocol。如果主机把 Protocol Mode 写为 Boot(2)，自定义 bitmap 会被按标准 Boot Keyboard 格式解析，bit 位置全部错位。

**验证方法：**
1. 安装 nRF Connect（手机或 PC）
2. 连接设备的 BLE HID 服务（UUID 0x1812）
3. 找到 Protocol Mode 特征值（UUID 0x2A4E）
4. 读取当前值：`01` = Report Protocol，`02` = Boot Protocol
5. 如果是 `02`，这就是根因

**修复：** 在 `hidkbdservice.c` 的 `Hid_SetParameter` 中拦截 Protocol Mode 写操作，强制返回错误或忽略，不允许切换到 Boot Protocol。

---

## 排查方向 2：BLE 连接时序与初始状态

`hidkbd.c` 第 539 行，当主机开启 CCCD 通知时：
```c
tmos_start_task(hidEmuTaskId, START_REPORT_EVT, 500);
```
但 `START_REPORT_EVT` 的处理代码全被注释掉了（第 312-321 行），意味着连接后 500ms 不会发送初始状态报告。

**可能的后果：** 某些 BLE HID 主机在连接后会读取 Report 特征值。如果此时 `hidReportKeyIn` 有残留非零值（虽然初始化为 0，但BLE协议栈可能有缓存），就会出现幽灵按键。

**验证方法：**
- 在 `hidEmuSendKbdReport` 函数入口加一行 printf，打印实际发送的两个字节
- 按下再松开所有键，观察松开后是否真的发送了全零报告

---

## 排查方向 3：PB18/PB20 引脚复用

CH582M 的 PB18 和 PB20 有复用功能（UART1、Timer 等）。BLE 协议栈初始化时如果改变了这些引脚的模式，会导致 GPIO 读取异常。

**验证方法：**
在 `KeyPad_Init()` 之后加调试代码，读取并打印 PB18 和 PB20 的引脚模式寄存器值：
```c
// 在 KeyPad_Init() 后执行
printf("PB18 mode: 0x%08X\n", R32_GPIOB_MODE & GPIO_Pin_18);
printf("PB20 mode: 0x%08X\n", R32_GPIOB_MODE & GPIO_Pin_20);
```
正常值应为输入上拉模式。如果变成输出或复用功能，说明被覆盖了。

---

## 排查方向 4：DevEP1_OUT_Deal 回环污染

`main.c` 第 113-122 行的 `DevEP1_OUT_Deal` 函数在收到 USB EP1 OUT 数据后将其取反并通过 EP1 IN 发回。虽然 USB 初始化被注释了，但如果 USB 硬件中断仍然触发（未完全禁用），可能会污染数据缓冲区。

**验证方法：** 在 `DevEP1_OUT_Deal` 入口加 printf，确认是否有意外调用。

---

## 排查方向 5：BLE HID 报告描述符与实际发送不匹配

BLE 和 USB 使用相同的自定义 bitmap 格式报告描述符（11位数字键 + Delete + Ctrl + Alt + Win + 1位填充 = 16位）。

**验证方法：**
1. 用 nRF Connect 订阅 HID Report 特征值的通知
2. 按下 ENTER → 观察通知数据是否为 `[00 04]`（bit10 = 0x0400）
3. 按下 CTRL → 观察通知数据是否为 `[00 10]`（bit12 = 0x1000）
4. 如果通知数据与预期不符，说明打包逻辑有误

---

## 快速排查清单

按优先级依次检查：

- [ ] 用 nRF Connect 读取 Protocol Mode → 是 Report(01) 还是 Boot(02)？
- [ ] 在 `hidEmuSendKbdReport` 加 printf → 发送的字节是否正确？
- [ ] 无按键时 BLE 通知数据是否为 `[00 00]`？
- [ ] PB18/PB20 的 GPIO 模式寄存器值是否为输入上拉？
- [ ] `DevEP1_OUT_Deal` 是否被意外调用？

## 涉及文件

- `Firmware_BLE/App/hidkbd.c` — 调试点：hidEmuSendKbdReport、START_KEYSCAN_EVT
- `Firmware_BLE/App/keypad.c` — 调试点：KeyPad_Init、KeyPad_ReadPin
- `Firmware_BLE/App/Profile/hidkbdservice.c` — Protocol Mode 相关
- `Firmware_BLE/App/main.c` — DevEP1_OUT_Deal

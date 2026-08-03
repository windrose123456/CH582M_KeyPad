# Firmware_BLE 重构规格文档

## 1. 项目现状分析

### 1.1 当前文件清单

| 文件 | 行数 | 职责 |
|------|------|------|
| `App/main.c` | 378 | 系统入口、USB中断、GPIO中断、睡眠/唤醒、调试工具 |
| `App/main.h` | 18 | main.c对外接口 |
| `App/hidkbd.c` | 694 | BLE HID键盘应用 + 按键扫描 + EC11处理 + 指纹测试 + USB/BLE路由 |
| `App/hidkbd.h` | 63 | BLE HID对外接口 |
| `App/hid_report.c` | 582 | USB描述符 + USB控制传输处理 + USB HID发送 |
| `App/hid_report.h` | 19 | USB HID对外接口 |
| `App/keypad.c` | 96 | 15键GPIO扫描 + 消抖 (独立，无需改动) |
| `App/keypad.h` | 106 | 按键引脚定义 + 接口 |
| `App/ec11.c` | 80 | 旋转编码器GPIO中断驱动 (独立，无需改动) |
| `App/ec11.h` | 25 | EC11引脚定义 + 接口 |
| `App/fingerprint_drv.c` | 369 | 指纹模块UART3通信协议 (独立，无需改动) |
| `App/fingerprint_drv.h` | 255 | 指纹协议常量 + 接口 |
| `App/battery.c` | 63 | ADC电池电压采集 (独立，无需改动) |
| `App/battery.h` | 21 | 电池接口 |
| `App/ws2812b.c` | 94 | WS2812B LED驱动 (独立，无需改动) |
| `App/ws2812b.h` | 12 | LED接口 |
| `App/ring_buffer.c` | 80 | 通用环形缓冲区 (独立，无需改动) |
| `App/ring_buffer.h` | 103 | 环形缓冲区接口 |
| `HAL/` | ~5文件 | WCH BLE HAL层 (不可修改) |
| `App/Profile/` | ~5文件 | BLE GATT Profile (不可修改) |
| `LIB/` | 2文件 | BLE协议栈库 (不可修改) |

### 1.2 核心问题

**问题1: `hidkbd.c` 是一个"上帝文件" (694行)**
- BLE GAP广播/连接管理
- BLE HID报告发送 (`hidEmuSendKbdReport`)
- 按键扫描调度 (`START_KEYSCAN_EVT` 处理)
- EC11旋转编码器处理 (旋转+按键)
- 指纹模块测试逻辑
- USB/BLE发送路由判断 (`HID_IsReady()` 分支)
- 全部挤在 `HidEmu_ProcessEvent()` 一个函数里

**问题2: `main.c` 职责过多 (378行)**
- USB中断处理函数 (`USB_IRQHandler`)
- USB端点数据处理 (`DevEP1_OUT_Deal`)
- GPIO中断处理 (`GPIOA_IRQHandler` - 按键唤醒 + 指纹触摸)
- 睡眠/唤醒完整流程 (`Enter_SleepMode`, `Wakeup_Reinit`, `Sleep_WakeupConfig`)
- 调试工具函数 (`hex_to_ascii`, `debug_hex_dump`)
- 系统初始化 (`main`)

**问题3: USB/BLE路由是隐式的**
- `hidkbd.c:464-477` 通过 `HID_IsReady()` 判断走USB还是BLE
- 没有统一的HID报告抽象层
- USB和BLE的发送逻辑耦合在BLE任务代码中

**问题4: `hid_report.c` 命名不准确**
- 实际内容是USB HID相关的描述符和传输处理
- 应该叫 `usb_hid.c`

**问题5: 输入处理嵌入BLE任务**
- EC11旋转/按键、指纹处理都在 `hidkbd.c` 的 `START_KEYSCAN_EVT` 中
- 这些与BLE传输无关，应该独立

---

## 2. 重构目标架构

### 2.1 设计原则

1. **单一职责**: 每个文件只负责一个功能领域
2. **传输无关**: HID报告发送不依赖具体传输方式(USB/BLE)
3. **可扩展**: 后续新建input TMOS任务时，只需调整调用关系
4. **最小改动**: 不修改已有的独立模块(keypad, ec11, fingerprint, battery, ws2812b, ring_buffer)

### 2.2 重构后文件结构

```
Firmware_BLE/App/
  main.c                  -- 系统入口 + 主循环 + GPIO中断 + 睡眠管理
  main.h                  -- 对外接口 (sleep/wakeup/debug)
  debug_utils.c           -- 调试工具函数
  debug_utils.h           -- 调试工具接口

  usb_hid.c               -- USB HID传输层
  usb_hid.h               -- USB HID接口
  ble_hid.c               -- BLE HID传输层
  ble_hid.h               -- BLE HID接口
  hid_report.c            -- 统一HID报告调度层 + 输入处理
  hid_report.h            -- 统一HID接口

  keypad.c / .h           -- (不变) 按键GPIO扫描
  ec11.c / .h             -- (不变) 旋转编码器
  fingerprint_drv.c / .h  -- (不变) 指纹模块驱动
  battery.c / .h          -- (不变) 电池ADC
  ws2812b.c / .h          -- (不变) LED驱动
  ring_buffer.c / .h      -- (不变) 环形缓冲区

Firmware_BLE/HAL/         -- (不变) WCH HAL
Firmware_BLE/App/Profile/ -- (不变) BLE GATT Profile
Firmware_BLE/LIB/         -- (不变) BLE协议栈
```

### 2.3 模块依赖关系

```
                    main.c
                       |
                       v
                  hid_report.c  <--- 统一调度入口
                  /         \
                 v           v
           usb_hid.c     ble_hid.c
                  \           /
                   v         v
             keypad.c    ec11.c    (输入设备)
                        fingerprint_drv.c

  main.c ---+---> debug_utils.c
            +---> fingerprint_drv.c
            +---> battery.c, ws2812b.c, ring_buffer.c
            +---> HAL/, Profile/ (WCH SDK)
```

**无循环依赖**: `hid_report.c` 调用 `usb_hid.c` 和 `ble_hid.c`，但后两者不回调 `hid_report.c`。

---

## 3. 各模块详细规格

### 3.1 `usb_hid.c` / `usb_hid.h` -- USB HID传输层

#### 3.1.1 职责

- USB设备描述符、HID描述符、配置描述符、字符串描述符
- USB控制传输处理 (SETUP事务、标准请求、HID类请求)
- USB端点数据收发 (EP0控制、EP1中断IN/OUT)
- USB中断处理
- USB设备状态管理 (Ready标志)

#### 3.1.2 从 `hid_report.c` 迁移的内容

| 类型 | 名称 | 原位置 | 说明 |
|------|------|--------|------|
| 常量数组 | `MyDevDescr[]` | hid_report.c:6 | USB设备描述符 |
| 常量数组 | `HIDDescr[]` | hid_report.c:9-97 | HID报告描述符 (键盘+Consumer) |
| 常量数组 | `MyCfgDescr[]` | hid_report.c:100-106 | USB配置描述符 |
| 常量数组 | `MyLangDescr[]` | hid_report.c:109 | 语言描述符 |
| 常量数组 | `MyManuInfo[]` | hid_report.c:111-119 | 厂商字符串 |
| 常量数组 | `MyProdInfo[]` | hid_report.c:121-130 | 产品字符串 |
| 变量 | `DevConfig` | hid_report.c:133 | USB配置值 |
| 变量 | `Ready` | hid_report.c:133 | USB就绪标志 |
| 变量 | `SetupReqCode` | hid_report.c:134 | SETUP请求码 |
| 变量 | `SetupReqLen` | hid_report.c:135 | SETUP请求长度 |
| 变量 | `pDescr` | hid_report.c:136 | 描述符指针 |
| 变量 | `Report_Value[]` | hid_report.c:137 | HID Report值 |
| 变量 | `Idle_Value[]` | hid_report.c:138 | HID Idle值 |
| 变量 | `USB_SleepStatus` | hid_report.c:139 | USB睡眠状态 |
| 数组 | `EP0_Databuf[]` | hid_report.c:145 | EP0端点缓冲区 |
| 数组 | `EP1_Databuf[]` | hid_report.c:146 | EP1端点缓冲区 |
| 函数 | `USB_DevTransProcess()` | hid_report.c:157-549 | USB传输处理(核心) |
| 函数 | `DevHIDReport()` | hid_report.c:559-566 | USB HID报告发送 |
| 函数 | `HID_IsReady()` | hid_report.c:568-571 | 查询USB就绪状态 |
| 函数 | `Clear_Ready()` | hid_report.c:573-576 | 清除就绪标志 |
| 函数 | `HID_InitUSBBuffer()` | hid_report.c:578-582 | 初始化USB缓冲区 |

#### 3.1.3 从 `main.c` 迁移的内容

| 类型 | 名称 | 原位置 | 说明 |
|------|------|--------|------|
| 函数 | `USB_IRQHandler()` | main.c:180-185 | USB中断处理函数 |
| 函数 | `DevEP1_OUT_Deal()` | main.c:161-170 | EP1 OUT数据处理 |

#### 3.1.4 头文件 `usb_hid.h`

```c
#ifndef __USB_HID_H__
#define __USB_HID_H__

#include "CH58x_common.h"

/* ======================== USB Constants ======================== */
#define DevEP0SIZE    0x40   /* EP0 max packet size: 64 bytes */
#define DevEP1SIZE    0x40   /* EP1 max packet size: 64 bytes */

/* ======================== Interface Count ======================== */
#define USB_INTERFACE_MAX_NUM       1
#define USB_INTERFACE_MAX_INDEX     0

/* ======================== Public API ======================== */

/**
 * @brief  Initialize USB HID subsystem.
 *         Sets up endpoint buffers, calls USB_DeviceInit(), enables USB IRQ.
 */
void USB_HID_Init(void);

/**
 * @brief  Send a HID report via USB EP1 IN.
 * @param  data  Pointer to report data.
 * @param  len   Report length (clamped to DevEP1SIZE).
 */
void USB_HID_SendReport(uint8_t *data, uint8_t len);

/**
 * @brief  Check if USB is ready (enumerated and not suspended).
 * @return 1 if ready, 0 otherwise.
 */
uint8_t USB_HID_IsReady(void);

/**
 * @brief  Clear USB ready flag (e.g. on suspend).
 */
void USB_HID_ClearReady(void);

#endif /* __USB_HID_H__ */
```

#### 3.1.5 内部实现要点

`USB_HID_Init()` 实现:
```c
void USB_HID_Init(void) {
    pEP0_RAM_Addr = EP0_Databuf;
    pEP1_RAM_Addr = EP1_Databuf;
    USB_DeviceInit();
    PFIC_EnableIRQ(USB_IRQn);
}
```

`USB_HID_SendReport()` 实现:
```c
void USB_HID_SendReport(uint8_t *data, uint8_t len) {
    if (len > DevEP1SIZE) {
        len = DevEP1SIZE;
    }
    memcpy(pEP1_IN_DataBuf, data, len);
    DevEP1_IN_Deal(len);
}
```

`USB_IRQHandler` 和 `DevEP1_OUT_Deal` 直接从 main.c 搬过来，不改实现。

---

### 3.2 `ble_hid.c` / `ble_hid.h` -- BLE HID传输层

#### 3.2.1 职责

- BLE GAP广播配置 (广播数据、扫描响应数据)
- BLE连接管理 (连接参数更新、PHY更新)
- BLE配对/绑定配置
- BLE HID服务注册
- BLE HID报告发送 (通过 `HidDev_Report`)
- BLE状态回调处理
- TMOS事件处理 (BLE相关事件)
- 按键扫描定时调度 (START_KEYSCAN_EVT)

#### 3.2.2 从 `hidkbd.c` 迁移的内容

| 类型 | 名称 | 原位置 | 说明 |
|------|------|--------|------|
| 宏 | `HID_KEYBOARD_IN_RPT_LEN` | hidkbd.c:32 | BLE HID输入报告长度 |
| 宏 | `HID_LED_OUT_RPT_LEN` | hidkbd.c:35 | BLE HID LED报告长度 |
| 宏 | `START_PARAM_UPDATE_EVT_DELAY` | hidkbd.c:41 | 参数更新延迟 |
| 宏 | `START_PHY_UPDATE_DELAY` | hidkbd.c:44 | PHY更新延迟 |
| 宏 | `DEFAULT_HID_IDLE_TIMEOUT` | hidkbd.c:47 | HID空闲超时 |
| 宏 | `DEFAULT_DESIRED_MIN_CONN_INTERVAL` | hidkbd.c:50 | 最小连接间隔 |
| 宏 | `DEFAULT_DESIRED_MAX_CONN_INTERVAL` | hidkbd.c:53 | 最大连接间隔 |
| 宏 | `DEFAULT_DESIRED_SLAVE_LATENCY` | hidkbd.c:56 | 从机延迟 |
| 宏 | `DEFAULT_DESIRED_CONN_TIMEOUT` | hidkbd.c:59 | 连接超时 |
| 宏 | `DEFAULT_PASSCODE` | hidkbd.c:62 | 默认配对密码 |
| 宏 | `DEFAULT_PAIRING_MODE` | hidkbd.c:65 | 配对模式 |
| 宏 | `DEFAULT_MITM_MODE` | hidkbd.c:68 | MITM模式 |
| 宏 | `DEFAULT_BONDING_MODE` | hidkbd.c:71 | 绑定模式 |
| 宏 | `DEFAULT_IO_CAPABILITIES` | hidkbd.c:74 | IO能力 |
| 宏 | `DEFAULT_BATT_CRITICAL_LEVEL` | hidkbd.c:77 | 电池低电量阈值 |
| 变量 | `hidEmuTaskId` | hidkbd.c:88 | TMOS任务ID |
| 数组 | `scanRspData[]` | hidkbd.c:103-123 | BLE扫描响应数据 |
| 数组 | `advertData[]` | hidkbd.c:126-152 | BLE广播数据 |
| 常量 | `attDeviceName[]` | hidkbd.c:155 | 设备名称 |
| 结构体 | `hidEmuCfg` | hidkbd.c:158-161 | HID设备配置 |
| 变量 | `hidEmuConnHandle` | hidkbd.c:163 | 连接句柄 |
| 回调 | `hidEmuHidCBs` | hidkbd.c:181-185 | HID回调结构体 |
| 函数 | `HidEmu_Init()` | hidkbd.c:205-254 | BLE HID初始化 |
| 函数 | `HidEmu_ProcessEvent()` | hidkbd.c:269-488 | TMOS事件处理(核心) |
| 函数 | `hidEmu_ProcessTMOSMsg()` | hidkbd.c:499-506 | TMOS消息处理 |
| 函数 | `hidEmuSendKbdReport()` | hidkbd.c:517-525 | BLE键盘报告发送 |
| 函数 | `hidEmuStateCB()` | hidkbd.c:536-603 | GAP状态回调 |
| 函数 | `hidEmuRcvReport()` | hidkbd.c:615-627 | 接收HID报告处理 |
| 函数 | `hidEmuRptCB()` | hidkbd.c:643-676 | HID报告回调 |
| 函数 | `hidEmuEvtCB()` | hidkbd.c:687-691 | HID事件回调 |

#### 3.2.3 从 `hidkbd.h` 迁移的内容

| 类型 | 名称 | 原位置 | 说明 |
|------|------|--------|------|
| 宏 | `START_DEVICE_EVT` | hidkbd.h:29 | 事件标志 |
| 宏 | `START_REPORT_EVT` | hidkbd.h:30 | 事件标志 |
| 宏 | `START_PARAM_UPDATE_EVT` | hidkbd.h:31 | 事件标志 |
| 宏 | `START_PHY_UPDATE_EVT` | hidkbd.h:32 | 事件标志 |
| 宏 | `START_KEYSCAN_EVT` | hidkbd.h:33 | 事件标志 |

#### 3.2.4 头文件 `ble_hid.h`

```c
#ifndef __BLE_HID_H__
#define __BLE_HID_H__

#include "CH58x_common.h"

/* ======================== TMOS Task Events ======================== */
#define START_DEVICE_EVT          0x0001
#define START_REPORT_EVT          0x0002
#define START_PARAM_UPDATE_EVT    0x0004
#define START_PHY_UPDATE_EVT      0x0008
#define START_KEYSCAN_EVT         0x0010

/* ======================== Public API ======================== */

/**
 * @brief  Initialize BLE HID subsystem.
 *         Registers TMOS task, configures GAP, sets up HID service.
 */
void BLE_HID_Init(void);

/**
 * @brief  Send a keyboard bitmap report via BLE HID.
 * @param  bitmap  16-bit key bitmap (bit0=key1, bit1=key2, ...).
 */
void BLE_HID_SendKbdReport(uint16_t bitmap);

/**
 * @brief  BLE HID TMOS event processor.
 *         Must be called from the TMOS event dispatcher.
 * @param  task_id  TMOS task ID.
 * @param  events   Event flags.
 * @return Unprocessed events.
 */
uint16_t BLE_HID_ProcessEvent(uint8_t task_id, uint16_t events);

#endif /* __BLE_HID_H__ */
```

#### 3.2.5 `HidEmu_ProcessEvent()` 中 `START_KEYSCAN_EVT` 的处理

重构后的 `START_KEYSCAN_EVT` 处理:

```c
if (events & START_KEYSCAN_EVT)
{
    // --- 指纹触摸IRQ处理 (暂时保留在此，后续移至input task) ---
    extern uint8_t touch_irq_flag;
    static uint8_t fp_test_flag = 0;
    if (touch_irq_flag) {
        touch_irq_flag = 0;
        if (fp_test_flag == 0) {
            FP_GetImage();
            fp_test_flag++;
        } else if (fp_test_flag == 1) {
            FP_Search(1, 0, 100);
            fp_test_flag = 0;
        }
    }
    FP_Process();
    FP_AckPacket_t *ack;
    int fp_ret = FP_GetResult(ack);

    // --- 统一输入处理 ---
    HID_ProcessInputs();  // 调用hid_report.c中的统一输入处理

    // --- 下次扫描 ---
    tmos_start_task(task_id, START_KEYSCAN_EVT, KEYSCAN_INTERVAL_TICK);
    return (events ^ START_KEYSCAN_EVT);
}
```

#### 3.2.6 `BLE_HID_SendKbdReport()` 实现

```c
void BLE_HID_SendKbdReport(uint16_t bitmap) {
    uint8_t report[HID_KEYBOARD_IN_RPT_LEN];
    report[0] = bitmap & 0xFF;
    report[1] = (bitmap >> 8) & 0xFF;
    HidDev_Report(HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT,
                  HID_KEYBOARD_IN_RPT_LEN, report);
}
```

---

### 3.3 `hid_report.c` / `hid_report.h` -- 统一HID报告调度层

#### 3.3.1 职责

- 提供与传输无关的HID报告发送接口
- 根据USB就绪状态自动路由到USB或BLE
- 统一处理输入设备 (EC11旋转编码器、EC11按键、键盘矩阵)
- 管理输入状态 (last_key_tick更新、唤醒逻辑)

#### 3.3.2 头文件 `hid_report.h`

```c
#ifndef __HID_REPORT_H__
#define __HID_REPORT_H__

#include "CH58x_common.h"

/* ======================== Public API ======================== */

/**
 * @brief  Initialize both USB and BLE HID subsystems.
 *         Calls USB_HID_Init() and BLE_HID_Init() internally.
 */
void HID_Init(void);

/**
 * @brief  Send a keyboard report via the active transport (USB or BLE).
 *         Automatically selects USB if ready, otherwise BLE.
 * @param  bitmap  16-bit key bitmap.
 */
void HID_SendKeyboardReport(uint16_t bitmap);

/**
 * @brief  Send a consumer control report via the active transport.
 * @param  data  Report data pointer.
 * @param  len   Report length.
 */
void HID_SendConsumerReport(uint8_t *data, uint8_t len);

/**
 * @brief  Process all input devices (EC11, keypad).
 *         Scans inputs, detects changes, sends HID reports.
 *         Called from BLE TMOS task (START_KEYSCAN_EVT).
 *
 *  NOTE: This function will be moved to a dedicated input TMOS task
 *        in a future refactoring step. For now it lives here so that
 *        both USB and BLE paths can reuse the same input logic.
 */
void HID_ProcessInputs(void);

#endif /* __HID_REPORT_H__ */
```

#### 3.3.3 `hid_report.c` 实现

```c
#include "hid_report.h"
#include "usb_hid.h"
#include "ble_hid.h"
#include "keypad.h"
#include "ec11.h"
#include "main.h"
#include <stdio.h>

/* ======================== Initialization ======================== */

void HID_Init(void) {
    USB_HID_Init();
    BLE_HID_Init();
}

/* ======================== Report Sending ======================== */

void HID_SendKeyboardReport(uint16_t bitmap) {
    if (USB_HID_IsReady()) {
        /* USB mode: report ID=0x01, 2-byte bitmap */
        uint8_t report[3];
        report[0] = 0x01;
        report[1] = bitmap & 0xFF;
        report[2] = (bitmap >> 8) & 0xFF;
        USB_HID_SendReport(report, 3);
    } else {
        /* BLE mode: direct bitmap, no report ID in payload */
        BLE_HID_SendKbdReport(bitmap);
    }
}

void HID_SendConsumerReport(uint8_t *data, uint8_t len) {
    if (USB_HID_IsReady()) {
        USB_HID_SendReport(data, len);
    }
    /* BLE consumer control: extend here if BLE supports it */
}

/* ======================== Input Processing ======================== */

void HID_ProcessInputs(void) {
    /*
     * EC11 Rotation
     * - Detect rotation direction (step != 0)
     * - Send volume up/down consumer report
     * - Send release report after 30ms debounce
     */
    int16_t step = EC11_GetStep();
    if (step != 0) {
        last_key_tick = TMOS_GetSystemClock();

        /* Wake up USB if suspended */
        if (!USB_HID_IsReady()) {
            DevWakeup();
            mDelaymS(15);
        }

        static uint32_t ec11_rot_tick = 0;
        if (ec11_rot_tick == 0) {
            uint8_t consumerReport[2];
            consumerReport[0] = 0x02;  /* Report ID = 0x02 (Consumer) */
            consumerReport[1] = (step > 0) ? 0x01 : 0x02;  /* VolUp / VolDn */
            HID_SendConsumerReport(consumerReport, 2);
            ec11_rot_tick = TMOS_GetSystemClock();
        }
        if (TMOS_GetSystemClock() - ec11_rot_tick > 30) {
            uint8_t release_report[2] = {0x02, 0x00};
            HID_SendConsumerReport(release_report, 2);
            ec11_rot_tick = 0;
            EC11_ResetStep();
        }
    }

    /*
     * EC11 Button Press
     * - Send mute toggle consumer report
     * - Send release report after 30ms debounce
     */
    if (EC11_GetKeyState()) {
        last_key_tick = TMOS_GetSystemClock();

        if (!USB_HID_IsReady()) {
            DevWakeup();
            mDelaymS(15);
        }

        static uint32_t ec11_key_tick = 0;
        if (ec11_key_tick == 0) {
            uint8_t consumerReport[2] = {0x02, 0x04};  /* Mute */
            HID_SendConsumerReport(consumerReport, 2);
            ec11_key_tick = TMOS_GetSystemClock();
        }
        if (TMOS_GetSystemClock() - ec11_key_tick > 30) {
            uint8_t release_report[2] = {0x02, 0x00};
            HID_SendConsumerReport(release_report, 2);
            ec11_key_tick = 0;
            EC11_ResetKey();
        }
    }

    /*
     * Keypad Matrix Scan
     * - Scan all 15 keys with debounce
     * - If bitmap changed, send keyboard report
     */
    KeyPad_Scan();
    static uint16_t last_bitmap = 0;
    uint16_t current_bitmap = KeyPad_GetBitmap();

    if (current_bitmap != last_bitmap) {
        last_key_tick = TMOS_GetSystemClock();

        /* Wake up USB if suspended */
        if (!USB_HID_IsReady()) {
            DevWakeup();
            mDelaymS(15);
        }

        HID_SendKeyboardReport(current_bitmap);
        last_bitmap = current_bitmap;
    }
}
```

#### 3.3.4 关于后续 input TMOS 任务的说明

当前 `HID_ProcessInputs()` 由 `ble_hid.c` 的 `START_KEYSCAN_EVT` 调用。
后续你新建独立的 input TMOS 任务时:

1. 在新任务中注册 TMOS 事件处理器
2. 将 `HID_ProcessInputs()` 调用从 `ble_hid.c` 移到新任务
3. 新任务通过 `tmos_msg_send()` 向 BLE/USB 任务发送键值
4. `HID_SendKeyboardReport()` 和 `HID_SendConsumerReport()` 接口不变

---

### 3.4 `debug_utils.c` / `debug_utils.h` -- 调试工具

#### 3.4.1 职责

- 提供十六进制数据转ASCII字符串的工具函数
- 提供通过UART0输出十六进制dump的调试函数

#### 3.4.2 从 `main.c` 迁移的内容

| 类型 | 名称 | 原位置 | 说明 |
|------|------|--------|------|
| 函数 | `hex_to_ascii()` | main.c:318-331 | 字节数组转十六进制ASCII |
| 函数 | `debug_hex_dump()` | main.c:339-376 | 通过UART0输出hex dump |

#### 3.4.3 头文件 `debug_utils.h`

```c
#ifndef __DEBUG_UTILS_H__
#define __DEBUG_UTILS_H__

#include "CH58x_common.h"

/**
 * @brief  Convert byte array to hex ASCII string with space separators.
 * @param  data     Input byte array.
 * @param  len      Number of bytes.
 * @param  out_buf  Output buffer (must be at least len*3 bytes).
 * @param  out_len  Output: string length (excluding '\0').
 */
void hex_to_ascii(const uint8_t *data, uint16_t len, char *out_buf, uint16_t *out_len);

/**
 * @brief  Dump byte array as hex string via UART0.
 * @param  data   Input byte array.
 * @param  len    Number of bytes.
 * @param  prefix Optional prefix string (e.g. "TX: "), or NULL.
 */
void debug_hex_dump(const uint8_t *data, uint16_t len, const char *prefix);

#endif /* __DEBUG_UTILS_H__ */
```

#### 3.4.4 实现

直接从 `main.c:318-376` 复制，无需修改。注意:
- 需要 `#include <string.h>` (for `strlen`)
- 需要 `#include <stdio.h>` (for `sprintf`)
- 使用 `UART0_SendString()` (来自WCH SDK)

---

### 3.5 `main.c` -- 系统入口 (精简后)

#### 3.5.1 职责 (保留)

- 系统初始化 (`main` 函数)
- TMOS主循环 (`Main_Circulation`)
- USB唤醒 (`DevWakeup`)
- GPIO中断处理 (`GPIOA_IRQHandler` - 按键唤醒 + 指纹触摸)
- 睡眠管理 (`Enter_SleepMode`, `Wakeup_Reinit`, `Sleep_WakeupConfig`)
- 全局变量 (`last_key_tick`, `enter_sleep_flag`, `wakeup_source`, `touch_irq_flag`)

#### 3.5.2 移除的内容

| 移除 | 去向 | 原位置 |
|------|------|--------|
| `USB_IRQHandler()` | `usb_hid.c` | main.c:180-185 |
| `DevEP1_OUT_Deal()` | `usb_hid.c` | main.c:161-170 |
| `hex_to_ascii()` | `debug_utils.c` | main.c:318-331 |
| `debug_hex_dump()` | `debug_utils.c` | main.c:339-376 |

#### 3.5.3 更新的 `#include`

```c
/* 移除 */
// #include "hidkbd.h"         -- 替换为 ble_hid.h (通过 hid_report.h 间接包含)
// #include "hid_report.h"     -- 旧的USB HID，替换为新的统一接口

/* 新增 */
#include "hid_report.h"        // 新的统一HID接口 (包含HID_Init)
#include "debug_utils.h"       // 调试工具
```

#### 3.5.4 更新的 `main()` 初始化序列

```c
int main(void) {
    /* ... 时钟、GPIO、UART初始化不变 ... */

    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();

    /* 旧代码:
     * HidEmu_Init();
     * HID_InitUSBBuffer();
     * USB_DeviceInit();
     * PFIC_EnableIRQ(USB_IRQn);
     */

    /* 新代码: */
    HID_Init();   // 内部调用 USB_HID_Init() + BLE_HID_Init()

    mDelaymS(100);

    KeyPad_Init();
    EC11_Init();
    FP_Init();

    last_key_tick = TMOS_GetSystemClock();
    printf("all device init done\n");
    Main_Circulation();
}
```

#### 3.5.5 更新的 `Wakeup_Reinit()`

```c
void Wakeup_Reinit(void) {
    /* ... GPIO、时钟、UART恢复不变 ... */

    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();

    /* 旧代码:
     * HidEmu_Init();
     * HID_InitUSBBuffer();
     * USB_DeviceInit();
     * PFIC_EnableIRQ(USB_IRQn);
     */

    /* 新代码: */
    HID_Init();

    mDelaymS(100);

    KeyPad_Init();
    EC11_Init();
}
```

#### 3.5.6 `main.h` 保持不变

`main.h` 已经声明了 `DevWakeup()`、`last_key_tick` 等，这些被 `hid_report.c` 使用，无需修改。

---

### 3.6 不变的模块

以下模块在本次重构中不需要修改:

| 文件 | 理由 |
|------|------|
| `keypad.c/h` | 纯GPIO扫描，接口清晰 |
| `ec11.c/h` | 纯GPIO中断驱动，接口清晰 |
| `fingerprint_drv.c/h` | 纯UART协议驱动，接口清晰 |
| `battery.c/h` | 纯ADC采集，接口清晰 |
| `ws2812b.c/h` | 纯GPIO位操作，接口清晰 |
| `ring_buffer.c/h` | 通用数据结构，接口清晰 |
| `HAL/*` | WCH SDK，不可修改 |
| `App/Profile/*` | WCH BLE Profile，不可修改 |
| `LIB/*` | BLE协议栈库，不可修改 |

唯一例外: `fingerprint_drv.c` 中使用了 `debug_hex_dump()`，需要将 `#include "main.h"` 改为 `#include "debug_utils.h"`。

---

## 4. 调用关系详图

### 4.1 初始化流程

```
main()
  |
  +-> CH58X_BLEInit()          // BLE协议栈初始化 (WCH SDK)
  +-> HAL_Init()               // HAL层初始化 (WCH SDK)
  +-> GAPRole_PeripheralInit() // GAP角色初始化 (WCH SDK)
  +-> HidDev_Init()            // HID设备初始化 (Profile)
  |
  +-> HID_Init()               // *** 新的统一入口 ***
  |     +-> USB_HID_Init()     // USB端点缓冲区 + USB_DeviceInit() + IRQ
  |     +-> BLE_HID_Init()     // GAP配置 + HID服务注册 + TMOS任务
  |
  +-> KeyPad_Init()            // 按键GPIO初始化
  +-> EC11_Init()              // EC11 GPIO + 中断初始化
  +-> FP_Init()                // 指纹模块 UART3 + GPIO初始化
  +-> Main_Circulation()       // 进入主循环
```

### 4.2 主循环

```
Main_Circulation()
  while(1)
    +-> TMOS_SystemProcess()        // TMOS调度 (BLE事件在这里分发)
    |     |
    |     +-> BLE_HID_ProcessEvent()
    |           |
    |           +-> START_KEYSCAN_EVT:
    |                 +-> FP_Process()           // 指纹状态机
    |                 +-> FP_GetResult()
    |                 +-> HID_ProcessInputs()    // *** 统一输入处理 ***
    |                       +-> EC11_GetStep()
    |                       +-> HID_SendConsumerReport()
    |                       +-> EC11_GetKeyState()
    |                       +-> KeyPad_Scan()
    |                       +-> KeyPad_GetBitmap()
    |                       +-> HID_SendKeyboardReport()
    |                             +-> USB_HID_IsReady() ?
    |                             |     YES -> USB_HID_SendReport()
    |                             |     NO  -> BLE_HID_SendKbdReport()
    |
    +-> sleep timeout check
          +-> Enter_SleepMode()
          +-> Wakeup_Reinit()
```

### 4.3 USB中断路径

```
USB_IRQHandler()              // usb_hid.c
  +-> USB_DevTransProcess()   // usb_hid.c (USB控制传输处理)
```

### 4.4 GPIO中断路径

```
GPIOA_IRQHandler()            // main.c (不变)
  +-> key wakeup handling
  +-> fingerprint touch_irq_flag

GPIOB_IRQHandler()            // ec11.c (不变)
  +-> EC11 rotation (s_step)
  +-> EC11 button (s_key_state)
  +-> key wakeup handling
```

---

## 5. 文件迁移清单 (精确到行号)

### 5.1 新建文件

| 文件 | 内容来源 |
|------|----------|
| `usb_hid.c` | hid_report.c 全部 (1-582行) + main.c:161-185 (USB IRQ + EP1 OUT) |
| `usb_hid.h` | 新写 (见3.1.4) |
| `ble_hid.c` | hidkbd.c 全部 (1-694行)，修改 START_KEYSCAN_EVT 处理 |
| `ble_hid.h` | 新写 (见3.2.4)，合并 hidkbd.h 的事件宏 |
| `debug_utils.c` | main.c:318-376 (hex_to_ascii + debug_hex_dump) |
| `debug_utils.h` | 新写 (见3.4.3) |

### 5.2 重写文件

| 文件 | 变化 |
|------|------|
| `hid_report.c` | 完全重写为统一调度层 (见3.3.3) |
| `hid_report.h` | 完全重写为统一接口 (见3.3.2) |

### 5.3 修改文件

| 文件 | 修改内容 |
|------|----------|
| `main.c` | 移除USB IRQ/EP1处理/debug函数；更新# include；更新初始化序列 |
| `fingerprint_drv.c` | `#include "main.h"` 改为 `#include "debug_utils.h"` (如果只用debug函数) |

### 5.4 删除文件

| 文件 | 替代 |
|------|------|
| `hidkbd.c` | `ble_hid.c` |
| `hidkbd.h` | `ble_hid.h` |

---

## 6. 构建配置更新

### 6.1 MounRiver Studio 项目配置

在 `.cproject` 或 `subdir.mk` 中添加新源文件:

```
App/usb_hid.c
App/ble_hid.c
App/debug_utils.c
App/hid_report.c   (已存在，内容替换)
```

移除旧文件引用:
```
App/hidkbd.c       (删除)
```

### 6.2 编译顺序

链接顺序不影响 (GCC自动处理)，但建议按以下分组:
1. 驱动层: keypad.c, ec11.c, fingerprint_drv.c, battery.c, ws2812b.c, ring_buffer.c
2. 传输层: usb_hid.c, ble_hid.c
3. 调度层: hid_report.c
4. 应用层: main.c, debug_utils.c

---

## 7. 验证清单

### 7.1 编译验证

- [ ] MounRiver Studio Build (Ctrl+B) 无错误
- [ ] 无未定义符号 (检查所有extern声明)
- [ ] 无重复定义 (检查全局变量是否只在一个.c中定义)

### 7.2 USB模式功能验证

- [ ] USB枚举成功 (设备管理器识别为HID键盘)
- [ ] 数字键0-9正常发送
- [ ] Enter键正常发送
- [ ] Delete键正常发送
- [ ] Ctrl/Alt/Win修饰键正常发送
- [ ] EC11旋转 -> 音量加减
- [ ] EC11按键 -> 静音切换

### 7.3 BLE模式功能验证

- [ ] BLE广播正常 (手机可发现 "HID Keyboard")
- [ ] BLE配对/绑定正常
- [ ] BLE键盘报告正常发送
- [ ] BLE断连后自动重新广播

### 7.4 睡眠/唤醒验证

- [ ] 10分钟无操作进入睡眠
- [ ] 按键GPIO唤醒
- [ ] 唤醒后USB重新枚举
- [ ] 唤醒后BLE重新连接

### 7.5 指纹模块验证

- [ ] 触摸IRQ触发FP_GetImage
- [ ] FP_Search正常执行
- [ ] UART3通信正常 (通过debug_hex_dump确认)

---

## 8. 注意事项

1. **编码**: 所有源文件必须保存为 **GBK编码** (非UTF-8)
2. **注释**: 注释必须使用 **英文**，避免编码问题
3. **SDK文件**: `HAL/` 和 `App/Profile/` 是WCH SDK代码，**不可修改**
4. **extern变量**: `last_key_tick`、`enter_sleep_flag`、`wakeup_source` 在 main.h 中已extern声明，hid_report.c 可直接使用
5. **DevWakeup()**: 在 main.c 中定义，main.h 中已声明，hid_report.c 可直接调用
6. **BLE任务ID**: `hidEmuTaskId` 是 ble_hid.c 的static变量，外部不可访问
7. **HID报告ID**: USB模式使用Report ID (0x01=键盘, 0x02=Consumer)，BLE模式Report ID由BLE服务定义

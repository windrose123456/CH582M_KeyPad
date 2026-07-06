# Firmware_BLE 工程分析报告

## 1. 工程概述

Firmware_BLE 是一个基于 WCH CH582M 芯片的 **BLE（低功耗蓝牙）HID 键盘固件工程**。与传统的 USB HID 键盘工程（Firmware）不同，这个工程使用蓝牙低功耗协议栈实现无线键盘功能。

### 核心特性
- **通信方式**: BLE 4.2 低功耗蓝牙（非 USB）
- **设备类型**: HID 键盘（Human Interface Device）
- **蓝牙协议栈**: WCH CH58xBLE 库（预编译静态库 `libCH58xBLE.a`）
- **开发环境**: MounRiver Studio (MRS)

---

## 2. 工程目录结构

```
Firmware_BLE/
├── App/                    # 应用代码（可编辑）
│   ├── main.c              # 入口点：BLE 初始化、键盘扫描循环
│   ├── keypad.c/.h         # 15 键 GPIO 扫描（去抖动）
│   ├── hidkbd.c/.h         # BLE HID 键盘核心逻辑
│   ├── hid_report.c/.h     # USB HID 报告处理（当前未启用）
│   ├── led_control.c/.h    # LED 控制（空实现）
│   └── Profile/            # BLE 服务定义
│       ├── hidkbdservice.c/.h      # HID 键盘服务
│       ├── battservice.c/.h        # 电池服务
│       ├── devinfoservice.c/.h     # 设备信息服务
│       ├── hiddev.c/.h             # HID 设备抽象层
│       └── scanparamservice.c/.h   # 扫描参数服务
├── Drivers/                # WCH 外设库（勿修改）
│   └── CH58x_StdPeriph/    # 标准外设驱动
├── HAL/                    # 硬件抽象层
│   ├── config.h            # BLE 配置参数（重要）
│   ├── HAL.h               # HAL 接口定义
│   ├── MCU.c               # MCU 初始化
│   ├── KEY.c/.h            # 按键驱动
│   ├── LED.c/.h            # LED 驱动
│   ├── SLEEP.c/.h          # 低功耗管理
│   └── RTC.c/.h            # RTC 时钟
├── LIB/                    # BLE 协议栈库（勿修改）
│   ├── CH58xBLE_LIB.h      # BLE 库头文件
│   ├── CH58xBLE_ROM.h      # ROM 接口定义
│   └── libCH58xBLE.a       # BLE 协议栈静态库
├── Ld/
│   └── Link.ld             # 链接器脚本
└── HID_Keyboard.wvproj     # MRS 工程文件
```

---

## 3. 与 Firmware 工程的关键区别

| 特性 | Firmware (USB) | Firmware_BLE (BLE) |
|------|----------------|---------------------|
| **通信接口** | USB 有线 | BLE 无线 |
| **协议栈** | 无（直接 USB） | CH58xBLE 库 |
| **配置文件** | 无 | `config.h` |
| **HID 报告** | `hid_report.c` | `hidkbd.c` |
| **额外依赖** | 无 | `libCH58xBLE.a` |
| **功耗** | 较高 | 低功耗设计 |
| **配对支持** | 无 | 配对/绑定 |

---

## 4. 核心模块分析

### 4.1 BLE 初始化流程 (`main.c`)

```c
int main(void)
{
    // 1. DCDC 配置（可选）
    PWR_DCDCCfg(ENABLE);
    
    // 2. 系统时钟配置（60MHz）
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    
    // 3. GPIO 配置（低功耗）
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    
    // 4. BLE 协议栈初始化
    CH58X_BLEInit();           // 初始化 BLE 库
    HAL_Init();                // HAL 层初始化
    GAPRole_PeripheralInit();  // GAP 外设角色初始化
    HidDev_Init();             // HID 设备初始化
    HidEmu_Init();             // HID 键盘应用初始化
    
    // 5. 键盘扫描初始化
    KeyPad_Init();
    
    // 6. 主循环（TMOS 事件处理）
    Main_Circulation();        // while(1) { TMOS_SystemProcess(); }
}
```

### 4.2 BLE HID 键盘实现 (`hidkbd.c`)

**关键参数配置：**
- 连接间隔：8 × 1.25ms = 10ms
- 从机延迟：0
- 监督超时：500 × 10ms = 5s
- 配对模式：等待请求
- 绑定模式：启用

**广播数据：**
- 设备名称：`HID Keyboard`
- 外观：HID 键盘（0x03C4）
- 服务 UUID：HID (0x1812) + Battery (0x180F)

**HID 报告结构：**
- 输入报告长度：2 字节
- 位图格式：15 位对应 15 个按键

### 4.3 按键扫描 (`keypad.c`)

**硬件连接：**
- 数字键 0-9：PB0-PB9
- Enter：PB18
- Delete：PB19
- Ctrl：PB20
- Alt：PB21
- Win：PB22

**扫描参数：**
- 扫描间隔：3 × 0.625ms = 1.875ms（约 533Hz）
- 去抖动：5 次采样（约 9.4ms）
- 电平检测：低电平有效（按下为 0）

### 4.4 BLE 配置 (`config.h`)

**重要配置项：**
```c
#define BLE_MAC                FALSE      // 使用芯片 MAC
#define DCDC_ENABLE            FALSE      // DCDC 禁用
#define HAL_SLEEP              FALSE      // 休眠禁用
#define BLE_MEMHEAP_SIZE       (1024*6)   // 6KB 协议栈内存
#define BLE_BUFF_MAX_LEN       27         // ATT_MTU=23
#define BLE_TX_POWER           LL_TX_POWEER_0_DBM  // 0dBm
#define PERIPHERAL_MAX_CONNECTION  1      // 最大 1 个连接
```

---

## 5. 内存布局

根据 `Link.ld`：
- **Flash**: 0x00000000 - 0x0006FFFF (448KB)
- **RAM**: 0x20000000 - 0x20007FFF (32KB)

**BLE 内存占用：**
- 协议栈堆：6KB（`BLE_MEMHEAP_SIZE`）
- 应用可用：约 26KB

---

## 6. 构建配置

**编译器选项：**
- 工具链：GNU MCU RISC-V GCC 8
- 优化：`-Os`（尺寸优化）
- 语言标准：GNU C99
- 调试：`DEBUG=1`

**链接库：**
- `libISP583.a`（ISP 编程库）
- `libCH58xBLE.a`（BLE 协议栈）

---

## 7. 调试与烧录

**调试接口：** WCH-Link（1-wire serial）

**OpenOCD 配置：**
```
-f wch-riscv.cfg
```

**GDB 命令：**
```
set architecture riscv:rv32
set remotetimeout unlimited
```

---

## 8. 功能流程

### 8.1 正常工作流程
1. 系统上电 → 初始化硬件
2. BLE 初始化 → 开始广播
3. 主机扫描 → 发现设备
4. 配对连接 → 建立 HID 服务
5. 按键扫描 → 生成 HID 报告
6. 发送报告 → 主机接收按键

### 8.2 按键处理流程
```
KeyPad_Scan() → KeyPad_GetBitmap() → hidEmuSendKbdReport()
                    ↓                        ↓
              生成 15 位位图          打包为 2 字节 HID 报告
```

---

## 9. 已知问题与注意事项

### 9.1 代码问题
1. **`hid_report.c` 未使用**：包含 USB HID 代码，但 BLE 工程中未启用
2. **`led_control.c` 空实现**：只有头文件包含
3. **`DevEP1_OUT_Deal` 未定义**：`main.c` 中声明但未实现（USB 相关）

### 9.2 配置注意事项
1. **MAC 地址**：默认使用芯片 MAC，如需自定义需修改 `main.c`
2. **功耗**：当前禁用了 DCDC 和休眠，如需低功耗需启用
3. **MTU**：默认 23 字节，如需传输大数据需增大 `BLE_BUFF_MAX_LEN`

### 9.3 调试建议
1. 启用 `DEBUG` 宏可查看串口输出
2. 使用 nRF Connect 等 BLE 工具扫描设备
3. 检查连接参数是否符合应用需求

---

## 10. 与 USB 版本的对比

### 优势
- 无线连接，使用更灵活
- 低功耗设计，适合电池供电
- 支持配对和绑定
- 符合 BLE HID 标准

### 劣势
- 依赖 BLE 协议栈，代码复杂度增加
- 延迟略高于 USB（连接间隔 10ms）
- 需要主机支持 BLE HID
- 功耗管理更复杂

---

## 11. 开发建议

### 11.1 功能扩展
- 启用 DCDC 和休眠以降低功耗
- 实现电池电量上报
- 添加 OLED 显示（参考 Firmware 工程）
- 支持多设备切换

### 11.2 优化方向
- 调整连接参数以平衡延迟和功耗
- 优化按键扫描频率
- 实现快捷键映射
- 添加 LED 状态指示

### 11.3 测试要点
- 配对和绑定功能
- 连接稳定性
- 按键响应延迟
- 电池续航

---

## 12. 总结

Firmware_BLE 工程是一个功能完整的 BLE HID 键盘实现，基于 WCH CH582M 和 CH58xBLE 协议栈。相比 USB 版本，它提供了无线连接能力，适合需要移动性和低功耗的应用场景。工程结构清晰，代码组织合理，但部分模块（如 LED 控制）尚未实现。在实际开发中，建议重点关注 BLE 连接稳定性、功耗优化和用户体验。
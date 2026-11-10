# BTB 连接器引脚定义（核心板 ⇄ 底板）

> 对应计划 §3.6。目标：160pin（2×80），电源/地 ≥30%，高速同组靠近出线边。
> **先与底板机械/电气联合定稿，再冻结核心板。**

---

## 0. 连接器与机械

| 项目 | 定义 |
|------|------|
| 连接器 | 2× 80pin BTB，**0.5mm 间距优先**（0.4mm 需确认制程） |
| 建议料号 | DF40C-80DP-0.4V + DF40C-80DS-0.4V；或 0.5mm 系列 DF12/JAE/国产 |
| 排布 | J1（调试侧）+ J2（外设侧），板长边各一 |
| 定位 | 每连接器侧 2×定位孔 ⌀1.5mm；板角 4×M2.5 安装孔 |
| 高度 | mating height 全系统统一（如 3.0mm），公母配套采购 |
| 防呆 | 丝印 J1/J2 + Pin1 三角；键位不同 |

**引脚编号规则**：Jx_yy = 连接器 Jx 第 yy 脚（1 起）。底板为镜像视图，出图时 3D 核对。

---

## 1. 电源与地（合计 52 pin，占 32.5%）

| 网络 | J1 脚位 | J2 脚位 | 说明 |
|------|---------|---------|------|
| **+5V_IN** | 1, 2, 79, 80 | — | 板级主输入 5V±5% ≥2A；核心板所有 DCDC 由此供电 |
| **GND** | 3,4,9,10,15,16,21,22,27,28,33,34,39,40,45,46,51,52,57,58,63,64,69,70,75,76 | 3,4,9,10,15,16,21,22,27,28,33,34,39,40,45,46,51,52,57,58,63,64,69,70,75,76 | **双侧交错满布**；与电源脚相邻 |
| **+3V3_OUT**（可选出） | 77, 78 | 1, 2 | 核心板 DCDC_3V3 输出给底板（≤500mA）；不用可 NC |
| **VDD_SNVS_BAT** | 5 | — | 纽扣电池正极（可选从底板接入）；与板载电池二极管或 |
| **+3V3_IO**（可选） | 6 | — | 若底板独立 3.3V IO 供电（与 DCDC_3V3 隔离场景） |

> 电源脚尽量靠近连接器两端 + 中部，减小压降。GND 与信号脚交错排，降低串扰。

---

## 2. J1（80pin）：电源 / 调试 / 启动 / USB / 以太网

### 2.1 调试与控制（Pin 7–14, 17–20）

| J1 | 网络名 | SoC 信号（IOMUX 默认/常用） | 方向 | 备注 |
|----|--------|---------------------------|------|------|
| 7 | UART1_TX | UART1_TX_DATA (GPIO1_IO02 ALT…) | 输出 | **第一调试串口**，3.3V |
| 8 | UART1_RX | UART1_RX_DATA (GPIO1_IO03) | 输入 | |
| 11 | POR_B | POR_B | 输入/双向 | 可外部复位；板上有复位芯片 |
| 12 | ONOFF | ONOFF | 输入 | 电源键，低有效 |
| 13 | PMIC_ON_REQ | SNVS_PMIC_ON_REQ | 输出 | 控制底板电源使能 |
| 14 | PMIC_STBY_REQ | CCM_PMIC_STBY_REQ | 输出 | DVFS/待机 |
| 17 | JTAG_TCK | JTAG_TCK | 输入 | 2.54mm 座或 BTB 二选一 |
| 18 | JTAG_TMS | JTAG_TMS | 输入 | |
| 19 | JTAG_TDI | JTAG_TDI | 输入 | |
| 20 | JTAG_TDO | JTAG_TDO | 输出 | **禁止外加上下拉** |

### 2.2 启动配置（Pin 23–26）

| J1 | 网络名 | 说明 |
|----|--------|------|
| 23 | BOOT_MODE0 | 拨码在底板或核心板，默认 Internal Boot(10) |
| 24 | BOOT_MODE1 | |
| 25 | BOOT_CFG1[0] | 启动设备选择（eMMC/SD/QSPI），详见原理图 BOOT 页 |
| 26 | BOOT_CFG2[0] | 补充配置 |

### 2.3 USB（Pin 29–32, 35–38）——救砖通路必须出

| J1 | 网络名 | 说明 |
|----|--------|------|
| 29 | USB_OTG1_DP | 差分 90Ω；Serial Download 用 OTG1 |
| 30 | USB_OTG1_DN | |
| 31 | USB_OTG1_VBUS | 5V 检测/供电 |
| 32 | USB_OTG1_ID | OTG 角色 |
| 35 | USB_OTG2_DP | 第二路 USB Host/OTG |
| 36 | USB_OTG2_DN | |
| 37 | USB_OTG2_VBUS | |
| 38 | USB_OTG2_ID | |

### 2.4 以太网 RMII ×2（Pin 41–50, 53–62）——两路 10/100M

**ENET1（Pin 41–50）**

| J1 | 网络名 | J1 | 网络名 |
|----|--------|----|--------|
| 41 | ENET1_REF_CLK | 46 | ENET1_TXD0 |
| 42 | ENET1_RXD0 | 47 | ENET1_TXD1 |
| 43 | ENET1_RXD1 | 48 | ENET1_TX_EN |
| 44 | ENET1_CRS_DV | 49 | ENET1_MDC |
| 45 | ENET1_RX_ER | 50 | ENET1_MDIO |

**ENET2（Pin 53–62）**：同结构 ENET2_REF_CLK / RXD0 / RXD1 / CRS_DV / RX_ER / TXD0 / TXD1 / TX_EN / MDC / MDIO

> RMII 单端 50Ω，REF_CLK 50Ω，长度组内匹配 ±50mil。MDIO/MDC 可两路共享或独立。

### 2.5 J1 其余（65–76 为 GND 交错，77–80 电源）

---

## 3. J2（80pin）：LCD / CSI / 总线 / 模拟

### 3.1 LCD RGB（Pin 5–40，36 信号）

| J2 | 网络 | J2 | 网络 | J2 | 网络 |
|----|------|----|------|----|------|
| 5 | LCD_CLK | 17 | LCD_DATA8 | 29 | LCD_DATA18 |
| 6 | LCD_HSYNC | 18 | LCD_DATA9 | 30 | LCD_DATA19 |
| 7 | LCD_VSYNC | 19 | LCD_DATA10 | 31 | LCD_DATA20 |
| 8 | LCD_ENABLE | 20 | LCD_DATA11 | 32 | LCD_DATA21 |
| 11 | LCD_DATA0 | 23 | LCD_DATA12 | 35 | LCD_DATA22 |
| 12 | LCD_DATA1 | 24 | LCD_DATA13 | 36 | LCD_DATA23 |
| 13 | LCD_DATA2 | 25 | LCD_DATA14 | 37 | LCD_PWM（背光） |
| 14 | LCD_DATA3 | 26 | LCD_DATA15 | 38 | LCD_RESET |
| 17 | LCD_DATA4 | 29 | LCD_DATA16 | 39 | LCD_BL_EN |
| 18 | LCD_DATA5 | 30 | LCD_DATA17 | 40 | CCM_CLK1_P（可选） |
| — | … | — | … | — | — |

> 实际占用 Pin 5–8, 11–14, 17–20, 23–26, 29–32, 35–40（GND 交错位不占）。
> 16bpp 只贴 DATA0–15，DATA16–23 可引出备用。

### 3.2 CSI 摄像头（Pin 41–50）

| J2 | 网络 | J2 | 网络 |
|----|------|----|------|
| 41 | CSI_MCLK | 46 | CSI_D2 |
| 42 | CSI_PIXCLK | 47 | CSI_D3 |
| 43 | CSI_VSYNC | 48 | CSI_D4 |
| 44 | CSI_HSYNC | 49 | CSI_D5 |
| 45 | CSI_D0 | 50 | CSI_D6/D7（择一） |

### 3.3 工业总线 / 音频（Pin 53–62）

| J2 | 网络 | J2 | 网络 |
|----|------|----|------|
| 53 | CAN1_TX | 58 | SAI1_TXD（I2S） |
| 54 | CAN1_RX | 59 | SAI1_RXD |
| 55 | CAN2_TX | 60 | SAI1_BCLK |
| 56 | CAN2_RX | 61 | SAI1_SYNC |
| 57 | SAI1_MCLK | 62 | SPDIF_OUT |

### 3.4 SPI / I2C / UART / PWM / ADC / GPIO（Pin 65–80）

| J2 | 网络 | 功能 |
|----|------|------|
| 65 | I2C1_SCL | 400kHz，底板传感器/触摸 |
| 66 | I2C1_SDA | |
| 67 | I2C2_SCL | |
| 68 | I2C2_SDA | |
| 69→GND | | |
| 70→GND | | |
| 71 | ECSPI1_MOSI | |
| 72 | ECSPI1_MISO | |
| 73 | ECSPI1_SCLK | |
| 74 | ECSPI1_SS0 | |
| 75→GND | | |
| 76→GND | | |
| 77 | UART2_TX | 第二串口 / RS485 |
| 78 | UART2_RX | |
| 79 | UART2_RTS | |
| 80 | UART2_CTS | |

**扩展 GPIO（建议再挤入 J1 空余或做成 1.27mm 边侧排针）**：

| 网络 | 建议 IOMUX | 用途 |
|------|-----------|------|
| PWM1_OUT | GPIO1_IO08 ALT1 | 背光/蜂鸣器 |
| PWM2_OUT | GPIO1_IO09 | — |
| ADC1_IN0/1 | ADC1_IN0/1 | 电池电压/温度 |
| GPIO1_IO04 | ENET1_REF_CLK 相关 | 通用 |
| GPIO1_IO05 | — | 通用 |
| GPIO1_IO06/07 | MDIO/MDC 若不用网口 | 通用 |
| SNVS_TAMPER0–3 | GPIO5_IO0–3 | 按键/唤醒 |
| WDOG_B | GPIO1_IO00 ALT | 外部看门狗 |

> IOMUX 复用冲突表见附录；**同一物理脚只能一个功能**，底板原理图要按复用表校核。

---

## 4. 复用冲突提示（设计前必读）

| 物理资源 | 互斥功能 | 建议默认 |
|---------|---------|---------|
| GPIO1_IO00/01 | I2C2 ↔ WDOG ↔ OTG1_ID | I2C2 |
| GPIO1_IO02/03 | I2C1 ↔ UART1 ↔ USDHC1 WP/CD | **UART1 调试**（I2C1 换脚） |
| GPIO1_IO04/05 | ENET_REF_CLK ↔ PWM ↔ USDHC1 RESET/VSELECT | ENET_REF_CLK |
| ENET1/2 全部 RMII | 不可同时做普通 GPIO | 网口 |
| LCD_DATA[23:0] | 与 CSI/SIM 等复用 | LCD |
| USDHC1 | SD 卡 | 引出到 J1/J2 边侧或底板卡槽 |
| USDHC2 | **eMMC 在核心板** | 不出 BTB |
| SNVS_TAMPER0–9 | GPIO5 ↔ 防拆 | 低功耗唤醒用 2–4 个即可 |

**USDHC1（TF 卡）若需要**，再占 6–10 pin：

| 网络 | 网络 |
|------|------|
| SD1_CMD | SD1_D1 |
| SD1_CLK | SD1_D2 |
| SD1_D0 | SD1_D3 |
| SD1_CD_B | SD1_WP / SD1_RESET_B |

（可挤占 J2 未用 CSI 脚或单独 1.27mm 小排针。）

---

## 5. 布线约束（BTB 区）

1. USB_D±、ETH RMII、LCD_CLK/HSYNC/VSYNC、CSI_PIXCLK：**50Ω（差分 90Ω）**，短直，不跨平面缝。
2. 电源脚两端+中间进铜皮/过孔到内层；BTB 周围一圈 GND via 护栏。
3. 同组高速相邻排（USB 两脚相邻、RMII 总线成束）。
4. 信号完整序：J1 电源/地/JTAG/UART 靠近调试侧；网口靠近 RJ45 方向。

---

## 6. 底板最小验证板建议（一版成关键）

核心板 + **极简验证底板**（先做这个，再做完整产品底板）：

| 必备 | 器件 |
|------|------|
| 5V DC 座 + 开关 | 供电 |
| USB Micro-B 接 OTG1 | **uuu 烧录** |
| USB-TTL（CP2102/CH340）接 UART1 | 启动日志 |
| TF 卡座接 USDHC1（或 eMMC 启动） | 启动介质 |
| BOOT 拨码 ×2 | 切 Internal/Serial Download |
| 复位按键 | POR_B |
| 电源指示 LED | DCDC_3V3_PG |

跑通后再扩展 ETH/LCD/CAN。

# i.MX6ULL 核心板选型表与 BOM

> 对应计划 §2。主选 = NXP EVK 验证过 / 常见供货；替代 = pin-to-pin 或需小改。
> 建库前把「封装」列的 footprint 尺寸与 datasheet 核对一遍。

---

## 1. 核心器件

| 位号 | 功能 | 主选型号 | 封装 | 关键参数 | 替代（需核对） | 预估单价 | 备注 |
|------|------|---------|------|---------|---------------|---------|------|
| U1 | SoC | **MCIMX6Y2CVM08AB** | MAPBGA-289 14×14mm 0.8mm pitch | 792MHz，工业级 -40~105℃，全功能 | MCIMX6Y2DVM09AB(900MHz) / MCIMX6Y2CVM05AB(528MHz) | ¥60–90 | 与 EVK 引脚兼容；封装选 **VM 14×14**，不要下成 VK 9×9 |
| U2 | DDR3L 512MB | **MT41K256M16TW-107:P** | 96-ball FBGA 8×10mm 0.8mm | 4Gb×16，DDR3L-1866(1.35V) | K4B4G1646E-BCK0 / H5TQ4G63CFR-RDC / NT5CC256M16ER-EK | ¥15–25 | EVK 同款；速度等级 -107 跑 400MHz 足够 |
| U3 | eMMC 8GB | **MTFC8GACAAAM-1M WT** | BGA-153 11.5×13mm | 8GB，eMMC 5.0，HS200 | KLM8G1GETF-B041 / SDINBDG4-8G / THGBMJG6C1LBAIL | ¥20–35 | **确认 VCCQ 支持 1.8V**；优先选 EVK 同封装 |
| U4 | QSPI NOR（可选） | MT25QL256ABA1EW9-0SIT | WSON-8 8×6mm | 256Mb | W25Q128JVSIQ / MX25L25635F | ¥5–10 | 首版可 DNP；做启动备份/放 boot |
| — | NAND（可选） | MT29F32G08CBADBWP | TSOP-48 | 32Gb | — | — | **首版不做**，占地方且 eMMC 已够 |

---

## 2. 电源器件（方案 A：EVK 同款分立，推荐）

| 位号 | 功能 | 主选型号 | 封装 | 输入 | 输出 | 替代 | 备注 |
|------|------|---------|------|------|------|------|------|
| U701 | SNVS 常开 LDO | RT9169-33GVL | SOT-23-5 | 5V/电池 | 3.3V / 100mA，Iq=4µA | XC6206P332 / ME6211C33 | 给 VDD_SNVS_IN；**必须最先上电** |
| U702 | 3.3V 主 DCDC | MP2144GJ | TSOT-23-8 | 5V | 3.3V / 2A | TPS562201 / SY8089 / AP3401KTR(限 1A 不够) | 产生 DCDC_3V3；PG 后级使能 |
| U703 | CPU 核心 DCDC | AP3401KTTR-G1 | SOT-23-5 | 3.3V/5V | 1.35V（792MHz）/ 1A | TPS562023 / SY8088 / RT8059 | **Vo=1.35V**：FB=0.6V 时 R1/R2≈125k/100k；预留 DVFS 切换 |
| U704 | DDR 电源 DCDC | AP3401KTTR-G1 | SOT-23-5 | 3.3V/5V | **1.35V** / 1A | TPS562023 / SY8088 | NVCC_DRAM；DDR3L 专用 1.35V |
| U705 | 1.8V LDO | RT9169-18GVL | SOT-23-5 | 3.3V | 1.8V / 100mA | XC6206P182 / ME6211C18 | eMMC VCCQ / NVCC_NAND |
| U706 | VDDA_ADC LDO | RT9193-33GB | SOT-23-5 | 5V | 3.3V / 300mA，PSRR 高 | RT9013-33 / XC6210B332 | **必须上电**（即使用不到 ADC） |
| U707 | NVCC_SD 负载开关 | UM1750S-00 | SOT-23-5 | 3.3V | 1.8/3.3V / 500mA | SY6280 / TPS22918 / XC8107 | SD 卡 3.3V；eMMC HS200 时 VCCQ 用 1.8V |
| U708 | USB 负载开关（可选） | SY6280AAC | SOT-23-5 | 5V | 5V / 1A | TPS22916 | USB OTG VBUS 输出控制 |

**方案 B（TI PMIC/多路，备选）**：

| 方案 | 器件 | 说明 |
|------|------|------|
| TI PMIC | TPS65217CRSL / TPS65218DR | 多路 + 时序好，QFN 难焊，贵 |
| TI 多路 DCDC | TPS65261 / TPS65530 | 集成 2–3 路 BUCK |
| NXP PMIC | MMPF0100F0AEP | i.MX6Q 常用，**i.MX6ULL EVK 未采用**，不推荐首版 |

**电压反馈电阻速查**（Vfb=0.6V，Vo=0.6×(R1+R2)/R2）：

| 目标 Vo | R1 | R2 | 备注 |
|---------|----|----|------|
| 1.20V | 100k | 100k | 仅 528MHz LDO bypass 场景 |
| 1.275V | 113k | 100k | 528MHz LDO enabled |
| **1.35V（792MHz）** | **125k** | **100k** | **主选**；可用 124k |
| 1.40V | 133k | 100k | 900MHz / DVFS 高档 |
| 3.30V | 450k | 100k | — |

> DVFS 双档（EVK 做法）：R1 拆成「固定 R + MOSFET 并联电阻」，GPIO_DVFS 控制切换 1.275/1.4V；PMIC_STBY_REQ 进 GPIO。

---

## 3. 时钟 / 复位 / 阻容 / 连接器

| 位号 | 功能 | 主选型号 | 封装 | 关键参数 | 替代 |
|------|------|---------|------|---------|------|
| Y1 | 24MHz 晶振 | **TSX-3225 24.0000MF15X-AC3** | 3225 | 24MHz，CL=12pF，ESR≤80Ω，驱动≤250µW | X322524MSB4SI / NX3225SA-24M |
| Y2 | 32.768kHz 晶振 | MC-306 / FC-135 32.768kHz | 3215/2125 | CL=10pF，ESR≤100kΩ | Q13FC1350000400 |
| U501 | 复位芯片 | **UM803RS** | SOT-23-3 | 监控 3.3V，低有效 | MAX809STR / SP706SEN / STM809 |
| R_ZQ | ZQ 校准 | 240Ω **±1%** | 0402 | 240Ω | — |
| J1/J2 | BTB 连接器 | **DF40-80DP-0.4V(51)** ×2 | 0.4mm 间距 80pin | 额定 0.3A/pin，Hirose | 1452180-2(TE) / 国产 BL-B80 / **嘉立创 0.5mm 更稳** |
| J_DBG | JTAG 20pin | 2.54mm IDC-20 | 标准 ARM JTAG | — | — |
| J_UART | 调试串口 | 2.54mm 排针 4pin | 或直接走 BTB | — | — |
| BT1 | 纽扣电池座 | ML414R / CR1220 座 | — | 串肖特基 RB521S30 或 R≥400Ω | — |

**BTB 备选封装建议**：若 0.4mm 担心打样/焊接，选 **0.5mm 间距 BTB**（如 DF12/DF40 0.5 系列），过孔扇出容易得多，一版成概率更高。

---

## 4. 阻容（关键值，其余建库时补）

| 用途 | 值 | 封装 | 数量级 |
|------|----|------|--------|
| 电源去耦 100nF | X7R 16V | 0402 | 60–80 颗（每电源球） |
| 大容量储能 22µF | X5R 6.3V/10V | 0805 | \*_CAP 脚各 1；电源轨各 2 |
| 储能 10µF | X5R | 0805/0603 | 每 DCDC 输入输出 |
| 22µF/4.7µF DCDC 输出 | 低 ESR | 0805 | 按 DCDC datasheet |
| 晶振负载 C | 8–12pF NP0 | 0402 | 按晶振 CL 计算 |
| VREF 电容 | 0.22µF | 0402 | 1（DRAM_VREF） |
| 上拉 10k/47k/100k | 1% | 0402 | BOOT/I2C/复位 |
| ZQ 电阻 | 240Ω 1% | 0402 | 1 |

---

## 5. EVK BOM 对照（可采购参考）

EVK 核心板（SCH-29364 B1）已验证主料：

| EVK 位号族 | 型号 | 我们采用？ |
|-----------|------|-----------|
| U101 | MCIMX6Y2DVM09AB | 改为 **CVM08AB**（792MHz） |
| U201 | MT41K256M16TW-107:P | **同款** |
| U301 | MTFC8GACAAAM | **同款或 KLM8G1GETF** |
| U302 | MT25QL256（QSPI） | 可选 |
| U701 | RT9169-33GVL | **同款** |
| U702/U706 | AP3401KTTR-G1 | **同款 ×2** |
| U703 | RT9193-33GB | **同款** |
| U704 | MP2144GJ | **同款** |
| U501 | UM803RS | **同款** |
| Y501 | TSX-3225 24MHz | **同款** |
| D102/D103 | RB521S30T1G | 纽扣防倒灌 |

> 完整 EVK BOM 见 `MCIMX6ULL_CM_DESIGNFILES\BOM\BOM-29364_B1.xls`。

---

## 6. 采购/下单清单（国庆 D2 执行）

**必买样品（每料 5–10 片）**：
- MCIMX6Y2CVM08AB ×5
- MT41K256M16TW-107:P ×5
- 8GB eMMC ×5
- 电源 IC 全套 ×10
- 24MHz / 32.768kHz 晶振 ×10
- UM803RS ×10
- BTB 公母各 5 套（**先买连接器**，机械要先定）
- 240Ω 1%、0402 阻容包、0.22µF ×20

**下单前核对**：
1. CPU 料号后缀 AB = 量产硅 Rev 1.1；AA = 样片 — 尽量买 AB
2. eMMC 是否 **工业级**（-40~85/105℃）与 CPU 温度等级匹配
3. DDR 封装图 ball map 与原理图库一致
4. BTB 公/母、贴片高度（mating height）与底板统一

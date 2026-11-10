# i.MX6ULL 核心板设计输出 · 使用说明

国庆设计时按顺序读；每份文档末尾有自查项。

| 顺序 | 文件 | 何时用 |
|------|------|--------|
| 0 | `..\核心板设计指南.md` | 背景（注意其中电源电压等已勘误，见计划 §7） |
| 1 | **`01_选型表与BOM.md`** | D1–D2 选型下单、建库 |
| 2 | **`02_BTB引脚定义.md`** | D1 定接口；与底板联合冻结 |
| 3 | **`03_原理图设计笔记.md`** | D3–D5 逐页画原理图 |
| 4 | **`04_PCB设计规则.md`** | D6–D8 布局布线 |
| 5 | **`05_调试与检查清单.md`** | D5/D9 出图勾选；节后焊接调试 |
| 6 | **`06_网络经验汇总_成功与失败案例.md`** | D1 先读一遍；他人翻车点 + 对我们设计的增量修改 |
| 附 | `SPF-29364_B1.pdf` | NXP EVK 核心板原理图 PDF（13 页，从 SCH zip 解出） |

## 原理图 PDF 页码索引（SPF-29364_B1.pdf）

| 页 | 内容 | 我们对应 |
|----|------|---------|
| 1 | 封面/修订 | 页 1 |
| 2 | 框图 | 页 2 |
| **3** | **PWR TREE** | 页 3 |
| **4** | **CPU PWR** | 页 4 |
| **5** | **LvDDR3** | 页 5 |
| **6** | **eMMC/NAND/TF/QSPI** | 页 6 |
| 7–8 | CPU PERI / RESET / JTAG | 页 7–8 |
| **9** | **BOOT CFG / FUSE MAP** | 页 9 |
| **10** | **PWR MGR（分立电源）** | 页 10 |
| 11 | SO-DIMM 200（改为我们 BTB） | 页 11 |
| 12 | Note / DNP | 页 12 |
| 13 | IOMUX 表 | 页 13 |

## 其它原始资料位置

```
E:\IMX6ULL硬件设计资料\
├── IMX6ULLIEC.pdf          datasheet（电源时序 §4.2 / 特殊信号 Table 3 / 电压 Table 10）
├── IMX6ULLRM.pdf           Reference Manual（IOMUXC / CCM / PMU / MMDC）
├── IMX6ULLEVKHUG.pdf       EVK 硬件指南（核心板 4 层 6.76×4.24cm）
├── iMX6ULL Power Consumption Application .pdf   AN5345 功耗
└── MCIMX6ULL-EVK-DESIGNFILES\MCIMX6ULL_CM_DESIGNFILES\
    ├── Schematic\SCH-29364_B1.zip
    ├── Layout\REF-29364_B.pdf    ← 布局参考图（强烈建议打印）
    ├── Layout\LAY-29364_B.zip
    └── BOM\BOM-29364_B1.xls
```

## 已定设计决策

| 项 | 值 |
|----|-----|
| SoC | MCIMX6Y2CVM08AB（792MHz，14×14） |
| DDR3L | 512MB MT41K256M16TW-107:P |
| eMMC | 8GB |
| 形态 | BTB 160pin 核心板 |
| 电源 | 分立 DCDC（EVK 同款 MP2144+AP3401+LDO） |
| PCB | 4 层优先 |

# BLE HID 幽灵按键排查 v2

## 核心现象
- 复位后 ENTER/3/CTRL 自动出现（未按任何键）
- 按一次任意键后恢复正常
- bitmap 打印值正确（0x0000）
- proto_mode = 1（Report Protocol）

## 结论
问题不在扫描逻辑，而在 BLE 连接建立过程中**某个环节向主机暴露了脏数据**。

---

## 排查点 1：GATT Read 返回脏数据

主机连接后会 Read Report 特征值。虽然 `Hid_GetParameter` 对 INPUT 类型返回 `*pLen = 0`，但 GATT 层可能仍通过 attribute 的 pValue 直接返回了原始字节。

**验证方法：** 在 `HidDev_ReadAttrCB`（hiddev.c 第 449 行）的 REPORT_UUID 处理分支加调试：

```c
if(uuid == REPORT_UUID ||
   uuid == BOOT_KEY_INPUT_UUID ||
   uuid == BOOT_KEY_OUTPUT_UUID ||
   uuid == BOOT_MOUSE_INPUT_UUID)
{
    if((pRpt = hidDevRptByHandle(pAttr->handle)) != NULL)
    {
        status = (*pHidDevCB->reportCB)(pRpt->id, pRpt->type, uuid,
                                        HID_DEV_OPER_READ, pLen, pValue);
        // === 加这行 ===
        printf("READ rpt id=%d type=%d uuid=0x%04X pLen=%d data=[%02X %02X]\n",
               pRpt->id, pRpt->type, uuid, *pLen, pValue[0], (*pLen>1)?pValue[1]:0xFF);
    }
}
```

如果打印出非零的 data，说明 GATT 层在回调之外还返回了 attribute 的原始值。`hidReportKeyIn` 虽然是 1 字节 0x00，但紧邻它的内存可能有垃圾数据。

---

## 排查点 2：hidReportKeyIn 内存布局

`hidkbdservice.c` 中变量声明顺序：

```
第 231 行: static uint8_t hidReportKeyInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
第 232 行: static uint8_t hidReportKeyIn;                    // ← 这个是 Report 特征值
第 233 行: static gattCharCfg_t hidReportKeyInClientCharCfg[GATT_MAX_NUM_CONN];
```

`hidReportKeyIn` 只有 1 字节，但 HID Report 描述符定义了 16 位（2 字节）的输入报告。如果 GATT 层按描述符长度读取 2 字节，就会越界读到 `hidReportKeyInClientCharCfg` 的内容。

**修复方法：** 将 `hidReportKeyIn` 改为 2 字节数组：

```c
// 原来：
static uint8_t       hidReportKeyIn;

// 改为：
static uint8_t       hidReportKeyIn[2];  // 匹配 HID 报告的 2 字节长度
```

同时修改 GATT 属性表中对应的 pValue（第 361 行）：

```c
// 原来：
&hidReportKeyIn},

// 改为：
hidReportKeyIn},
```

---

## 排查点 3：连接后立即发送全零报告

最直接的修复：在 BLE 连接成功后立即发送一次全零报告，清除主机可能持有的脏状态。

在 `hidkbd.c` 的 `hidEmuStateCB` 函数中，`GAPROLE_CONNECTED` 分支里加：

```c
case GAPROLE_CONNECTED:
    if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
    {
        gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *)pEvent;
        hidEmuConnHandle = event->connectionHandle;
        tmos_start_task(hidEmuTaskId, START_PARAM_UPDATE_EVT, START_PARAM_UPDATE_EVT_DELAY);

        // === 加这行：连接后立即发全零报告清除脏状态 ===
        hidEmuSendKbdReport(0x0000);

        PRINT("Connected..\n");
    }
    break;
```

**注意：** 这行代码在连接刚建立时执行，此时 CCCD 可能尚未启用，`HidDev_Report` 会返回 `bleNotReady`。如果不行，可以放到 `HID_DEV_OPER_ENABLE` 回调中：

```c
else if(oper == HID_DEV_OPER_ENABLE)
{
    // === 加这行：通知启用后立即发全零报告 ===
    hidEmuSendKbdReport(0x0000);
    tmos_start_task(hidEmuTaskId, START_REPORT_EVT, 500);
}
```

---

## 排查点 4：DevEP1_OUT_Deal 回环污染（USB 相关）

即使 USB 初始化被注释，`DevEP1_OUT_Deal` 和 `USB_IRQHandler` 仍在代码中。如果 USB 硬件引脚被主机拉高，可能触发中断导致内存损坏。

**验证方法：** 在 `main.c` 的 `DevEP1_OUT_Deal` 入口加 printf，确认是否有意外调用。

**修复方法：** 将 `DevEP1_OUT_Deal` 改为空函数：

```c
void DevEP1_OUT_Deal(uint8_t l)
{
    (void)l;
}
```

---

## 建议排查顺序

1. **先试排查点 3**（最简单）：在 `HID_DEV_OPER_ENABLE` 回调中加 `hidEmuSendKbdReport(0x0000)` → 测试是否解决
2. **再试排查点 2**（根本修复）：将 `hidReportKeyIn` 改为 2 字节数组
3. **同时加排查点 1 的 printf**：确认 GATT Read 是否返回脏数据
4. **最后排查点 4**：清理 USB 残留代码

## 涉及文件

- `Firmware_BLE/App/Profile/hidkbdservice.c` — 排查点 1、2
- `Firmware_BLE/App/hidkbd.c` — 排查点 3
- `Firmware_BLE/App/main.c` — 排查点 4

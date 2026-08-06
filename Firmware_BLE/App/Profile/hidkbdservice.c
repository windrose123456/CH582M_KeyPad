/********************************** (C) COPYRIGHT *******************************
 * File Name          : hidkbdservice.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : 键盘服务
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "hidkbdservice.h"
#include "hiddev.h"
#include "battservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// HID service
const uint8_t hidServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID)};

// HID Boot Keyboard Input Report characteristic
const uint8_t hidBootKeyInputUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(BOOT_KEY_INPUT_UUID), HI_UINT16(BOOT_KEY_INPUT_UUID)};

// HID Boot Keyboard Output Report characteristic
const uint8_t hidBootKeyOutputUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(BOOT_KEY_OUTPUT_UUID), HI_UINT16(BOOT_KEY_OUTPUT_UUID)};

// HID Information characteristic
const uint8_t hidInfoUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_INFORMATION_UUID), HI_UINT16(HID_INFORMATION_UUID)};

// HID Report Map characteristic
const uint8_t hidReportMapUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(REPORT_MAP_UUID), HI_UINT16(REPORT_MAP_UUID)};

// HID Control Point characteristic
const uint8_t hidControlPointUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_CTRL_PT_UUID), HI_UINT16(HID_CTRL_PT_UUID)};

// HID Report characteristic
const uint8_t hidReportUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(REPORT_UUID), HI_UINT16(REPORT_UUID)};

// HID Protocol Mode characteristic
const uint8_t hidProtocolModeUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(PROTOCOL_MODE_UUID), HI_UINT16(PROTOCOL_MODE_UUID)};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// HID Information characteristic value
static const uint8_t hidInfo[HID_INFORMATION_LEN] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), // bcdHID (USB HID version)
    0x00,                                 // bCountryCode
    HID_FEATURE_FLAGS                     // Flags
};

/*HID类报表描述符*/
static const uint8_t hidReportMap[] = {0x05, 0x01,                                                   //Generic Desktop
                            0x09, 0x06,                                                   //Keyboard   
                            0xA1, 0x01,                                                   //集合开始
                            0x85, 0x01,        // <--- 新增：Report ID (1) 用于键盘

                            // ----- 第一段：数字1~9、0、Enter（0x1E ~ 0x28，共11个）-----
                            0x05, 0x07,                                                   //Keyboard Application
                            0x19, 0x1E,                                                   //Uasge  Minimun
                            0x29, 0x28,                                                   //Usage  Maximun
                            0x15, 0x00,                                                   //Logical  Minimun
                            0x25, 0x01,                                                   //Logical  Maximun
                            0x75, 0x01,                                                   //Report Size
                            0x95, 0x0B,                                                   //Report Counet
                            0x81, 0x02,                                                   //Input

                            // ----- 第二段：Delete（0x4C）-----
                            0x05, 0x07,        // Usage Page (Keyboard)
                            0x19, 0x4C,        // Usage Minimum (Delete)
                            0x29, 0x4C,        // Usage Maximum (Delete)
                            0x15, 0x00,        // Logical Minimum (0)
                            0x25, 0x01,        // Logical Maximum (1)
                            0x75, 0x01,        // Report Size (1 bit)
                            0x95, 0x01,        // Report Count (1)     // 占1位
                            0x81, 0x02,        // Input (Data,Var,Abs)

                            // ----- 第三段：Ctrl（0xE0）-----
                            0x05, 0x07,        // Usage Page (Keyboard)
                            0x19, 0xE0,        // Usage Minimum (Ctrl)
                            0x29, 0xE0,        // Usage Maximum (Ctrl)
                            0x15, 0x00,        // Logical Minimum (0)
                            0x25, 0x01,        // Logical Maximum (1)
                            0x75, 0x01,        // Report Size (1 bit)
                            0x95, 0x01,        // Report Count (1)     // 占1位
                            0x81, 0x02,        // Input (Data,Var,Abs)

                            // ----- 第四段：Alt（0xE2）-----
                            0x05, 0x07,        // Usage Page (Keyboard)
                            0x19, 0xE2,        // Usage Minimum (Alt)
                            0x29, 0xE2,        // Usage Maximum (Alt)
                            0x15, 0x00,        // Logical Minimum (0)
                            0x25, 0x01,        // Logical Maximum (1)
                            0x75, 0x01,        // Report Size (1 bit)
                            0x95, 0x01,        // Report Count (1)     // 占1位
                            0x81, 0x02,        // Input (Data,Var,Abs)

                            // ----- 第五段：Win（0xE3）-----
                            0x05, 0x07,        // Usage Page (Keyboard)
                            0x19, 0xE3,        // Usage Minimum (Win)
                            0x29, 0xE3,        // Usage Maximum (Win)
                            0x15, 0x00,        // Logical Minimum (0)
                            0x25, 0x01,        // Logical Maximum (1)
                            0x75, 0x01,        // Report Size (1 bit)
                            0x95, 0x01,        // Report Count (1)     // 占1位
                            0x81, 0x02,        // Input (Data,Var,Abs)

                            // 字节补全
                            0x75, 0x01,                                                   //Report Size
                            0x95, 0x01,                                                   //Report Counet
                            0x81, 0x03,                                                   //Input (Const)
                            0xC0,

                            // ----- Consumer Control（多媒体键）-----
                            0x05, 0x0C,           // Usage Page (Consumer)
                            0x09, 0x01,           // Usage (Consumer Control)
                            0xA1, 0x01,           // Collection (Application)
                            0x85, 0x02,           //   Report ID (2)
                            
                            // -------- 音量增大（位 0） --------
                            0x09, 0xE9,           //   Usage (Volume Increment)
                            0x15, 0x00,           //   Logical Minimum (0)
                            0x25, 0x01,           //   Logical Maximum (1)
                            0x75, 0x01,           //   Report Size (1)
                            0x95, 0x01,           //   Report Count (1)
                            0x81, 0x02,           //   Input (Data, Var, Abs)

                            // -------- 音量减小（位 1） --------
                            0x09, 0xEA,           //   Usage (Volume Decrement)
                            0x81, 0x02,           //   Input (Data, Var, Abs)

                            // -------- 静音切换（bit 2） --------
                            0x09, 0xE2,           //   Usage (Mute)
                            0x81, 0x02,           //   Input (Data, Var, Abs)

                            // -------- 填充5位，对齐到整字节 --------
                            0x75, 0x05,           //   Report Size (5)   ← 从6改为5
                            0x95, 0x01,           //   Report Count (1)
                            0x81, 0x01,           //   Input (Cnst, Var, Abs)

                            0xC0};

// HID report map length
uint16_t hidReportMapLen = sizeof(hidReportMap);

// HID report mapping table
static hidRptMap_t hidRptMap[HID_NUM_REPORTS];

/*********************************************************************
 * Profile Attributes - variables
 */

// ==================== 1. HID Service 声明 ====================
static const gattAttrType_t hidService = {ATT_BT_UUID_SIZE, hidServUUID};

// ==================== 2. 包含服务（电池） ====================
static uint16_t include = GATT_INVALID_HANDLE;  // 将由协议栈自动填充句柄

// ==================== 3. HID 信息特性 ====================
static uint8_t hidInfoProps = GATT_PROP_READ;

// ==================== 4. HID 报告映射特性 ====================
static uint8_t hidReportMapProps = GATT_PROP_READ;

// ==================== 5. HID 协议模式特性 ====================
// 注意：虽然这个特性是“可选”的，但很多主机（如 Windows）会要求它存在，
// 所以建议保留，值设为 REPORT 协议模式（0x01）
static uint8_t hidProtocolModeProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP;
uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;  // 0x01
// HID Control Point
static uint8_t hidControlPointProps = GATT_PROP_WRITE;
static uint8_t hidControlPoint;

// ==================== 6. 键盘输入报告（ID = 1） ====================
static uint8_t       hidReportKeyInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t       hidReportKeyIn[2];  // 长度 2 字节（不含 ID）
static uint16_t hidReportKeyInClientCharCfg = 0;

static uint8_t hidReportRefKeyIn[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT};

// ==================== 7. 多媒体输入报告（ID = 2） ====================
static uint8_t       hidReportMediaInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t       hidReportMediaIn[1];   // 1 字节载荷（不含 ID），与描述符一致
static uint16_t      hidReportMediaInClientCharCfg = 0;  // 单连接场景，单个变量即可

static uint8_t hidReportRefMediaIn[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_MEDIA_IN, HID_REPORT_TYPE_INPUT};

// ==================== 8. LED 输出报告（ID = 3，留待后续启用） ====================
// static uint8_t hidReportLedOutProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
// static uint8_t hidReportLedOut;
// static uint8_t hidReportRefLedOut[HID_REPORT_REF_LEN] =
//     {HID_RPT_ID_LED_OUT, HID_REPORT_TYPE_OUTPUT};

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t hidAttrTbl[] = {
    // ====== 1. HID Service 声明 ======
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&hidService                  /* pValue */
    },

    // Included service (battery)
    {
        {ATT_BT_UUID_SIZE, includeUUID},
        GATT_PERMIT_READ,
        0,
        (uint8_t *)&include
    },

    // ====== 2. HID 信息特性 ======
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidInfoProps
    },

    // HID Information characteristic
    {
        {ATT_BT_UUID_SIZE, hidInfoUUID},
        GATT_PERMIT_READ,
        0,
        (uint8_t *)hidInfo
    },

    // ====== 3. HID 协议模式特性 ======
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidProtocolModeProps
    },

    // HID Protocol Mode characteristic
    {
        {ATT_BT_UUID_SIZE, hidProtocolModeUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &hidProtocolMode
    },

    // ====== HID Control Point ======
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidControlPointProps
    },
    // HID Control Point characteristic
    {
        {ATT_BT_UUID_SIZE, hidControlPointUUID},
        GATT_PERMIT_WRITE,
        0,
        &hidControlPoint
    },

    // ====== 4. HID 报告映射特性（必须） ======
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidReportMapProps
    },

    {
        {ATT_BT_UUID_SIZE, hidReportMapUUID},
        GATT_PERMIT_READ,
        0,
        (uint8_t *)hidReportMap
    },

    // ====== 5. 键盘输入报告（ID = 1） ======
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidReportKeyInProps
    },

    // HID Report characteristic, key input
    {
        {ATT_BT_UUID_SIZE, hidReportUUID},
        GATT_PERMIT_READ,
        0,
        hidReportKeyIn},

    // HID Report characteristic client characteristic configuration
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)&hidReportKeyInClientCharCfg
    },

    // HID Report Reference characteristic descriptor, key input
    {
        {ATT_BT_UUID_SIZE, reportRefUUID},
        GATT_PERMIT_READ,
        0,
        hidReportRefKeyIn
    },

    // ====== 6. 多媒体输入报告（ID = 2） ======
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidReportMediaInProps     
    },
    {
        {ATT_BT_UUID_SIZE, hidReportUUID},
        GATT_PERMIT_READ,
        0,
        hidReportMediaIn          
    },
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)&hidReportMediaInClientCharCfg 
    },
    {
        {ATT_BT_UUID_SIZE, reportRefUUID},
        GATT_PERMIT_READ,
        0,
        hidReportRefMediaIn        // {ID=2, TYPE=INPUT}
    },

    // // HID Report characteristic, LED output declaration
    // {
    //     {ATT_BT_UUID_SIZE, characterUUID},
    //     GATT_PERMIT_READ,
    //     0,
    //     &hidReportLedOutProps
    // },

    // // HID Report characteristic, LED output
    // {
    //     {ATT_BT_UUID_SIZE, hidReportUUID},
    //     GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE,
    //     0,
    //     &hidReportLedOut
    // },
};

// Attribute index enumeration-- these indexes match array elements above
enum {
    HID_SERVICE_IDX,
    HID_INCLUDED_SERVICE_IDX,
    HID_INFO_CHAR_IDX,
    HID_INFO_IDX,
    HID_PROTOCOL_MODE_CHAR_IDX,
    HID_PROTOCOL_MODE_IDX,
    HID_CONTROL_POINT_CHAR_IDX,
    HID_CONTROL_POINT_IDX,
    HID_REPORT_MAP_CHAR_IDX,
    HID_REPORT_MAP_IDX,
    // 键盘输入报告
    HID_REPORT_KEY_IN_CHAR_IDX,
    HID_REPORT_KEY_IN_IDX,
    HID_REPORT_KEY_IN_CCCD_IDX,
    HID_REPORT_KEY_IN_REF_IDX,
    // 多媒体输入报告
    HID_REPORT_MEDIA_IN_CHAR_IDX,
    HID_REPORT_MEDIA_IN_IDX,
    HID_REPORT_MEDIA_IN_CCCD_IDX,
    HID_REPORT_MEDIA_IN_REF_IDX,
    HID_NUM_ATTRS  // 总属性数
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * PROFILE CALLBACKS
 */

// Service Callbacks
gattServiceCBs_t hidKbdCBs = {
    HidDev_ReadAttrCB,  // Read callback function pointer
    HidDev_WriteAttrCB, // Write callback function pointer
    NULL                // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      BleHid_AddService
 *
 * @brief   Initializes the HID Service by registering
 *          GATT attributes with the GATT server.
 *
 * @return  Success or Failure
 */
bStatus_t BleHid_AddService(void)
{
    uint8_t status = SUCCESS;

    // Initialize Client Characteristic Configuration attributes
    // GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportKeyInClientCharCfg);
    // GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportBootKeyInClientCharCfg);

    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService(hidAttrTbl, GATT_NUM_ATTRS(hidAttrTbl), GATT_MAX_ENCRYPT_KEY_SIZE, &hidKbdCBs);
    
    // Set up included service
    Batt_GetParameter(BATT_PARAM_SERVICE_HANDLE,
                      &GATT_INCLUDED_HANDLE(hidAttrTbl, HID_INCLUDED_SERVICE_IDX));

    // Construct map of reports to characteristic handles
    // Each report is uniquely identified via its ID and type

    // Key input report
    hidRptMap[0].id = hidReportRefKeyIn[0];
    hidRptMap[0].type = hidReportRefKeyIn[1];
    hidRptMap[0].handle = hidAttrTbl[HID_REPORT_KEY_IN_IDX].handle;
    hidRptMap[0].cccdHandle = hidAttrTbl[HID_REPORT_KEY_IN_CCCD_IDX].handle;
    hidRptMap[0].mode = HID_PROTOCOL_MODE_REPORT;

    // 多媒体输入报告
    hidRptMap[1].id = hidReportRefMediaIn[0];
    hidRptMap[1].type = hidReportRefMediaIn[1];
    hidRptMap[1].handle = hidAttrTbl[HID_REPORT_MEDIA_IN_IDX].handle;   // 你需要定义这个索引宏
    hidRptMap[1].cccdHandle = hidAttrTbl[HID_REPORT_MEDIA_IN_CCCD_IDX].handle; // 同上
    hidRptMap[1].mode = HID_PROTOCOL_MODE_REPORT;

    // LED output report
    // hidRptMap[1].id = hidReportRefLedOut[0];
    // hidRptMap[1].type = hidReportRefLedOut[1];
    // hidRptMap[1].handle = hidAttrTbl[HID_REPORT_LED_OUT_IDX].handle;
    // hidRptMap[1].cccdHandle = 0;
    // hidRptMap[1].mode = HID_PROTOCOL_MODE_REPORT;

    // Setup report ID map
    HidDev_RegisterReports(HID_NUM_REPORTS, hidRptMap);
    PRINT("HID Service registered, handle: 0x%04x\n", hidAttrTbl[HID_SERVICE_IDX].handle);
    PRINT("Report Map handle: 0x%04x\n", hidAttrTbl[HID_REPORT_MAP_IDX].handle);
    PRINT("Key input handle: 0x%04x, CCCD handle: 0x%04x\n", 
        hidAttrTbl[HID_REPORT_KEY_IN_IDX].handle,
        hidAttrTbl[HID_REPORT_KEY_IN_CCCD_IDX].handle);
    PRINT("Media input handle: 0x%04x, CCCD handle: 0x%04x\n", 
       hidAttrTbl[HID_REPORT_MEDIA_IN_IDX].handle,
       hidAttrTbl[HID_REPORT_MEDIA_IN_CCCD_IDX].handle);

// 新增：打印服务中属性总数
    PRINT("Total attributes in service: %d\n", GATT_NUM_ATTRS(hidAttrTbl));

    return (status);
}

/*********************************************************************
 * @fn      Hid_SetParameter
 *
 * @brief   Set a HID Kbd parameter.
 *
 * @param   id - HID report ID.
 * @param   type - HID report type.
 * @param   uuid - attribute uuid.
 * @param   len - length of data to right.
 * @param   pValue - pointer to data to write.  This is dependent on
 *          the input parameters and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  GATT status code.
 */
uint8_t Hid_SetParameter(uint8_t id, uint8_t type, uint16_t uuid, uint8_t len, void *pValue)
{
    bStatus_t ret = SUCCESS;
    return (ret);
}

/*********************************************************************
 * @fn      Hid_GetParameter
 *
 * @brief   Get a HID Kbd parameter.
 *
 * @param   id - HID report ID.
 * @param   type - HID report type.
 * @param   uuid - attribute uuid.
 * @param   pLen - length of data to be read
 * @param   pValue - pointer to data to get.  This is dependent on
 *          the input parameters and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  GATT status code.
 */
uint8_t Hid_GetParameter(uint8_t id, uint8_t type, uint16_t uuid, uint16_t *pLen, void *pValue)
{
    PRINT("Hid_GetParameter: uuid=0x%04x, id=%d, type=%d\n", uuid, id, type);

    // 处理 Report 特性读取（uuid = 0x2A4D）
    if (uuid == REPORT_UUID)
    {
        // 输入报告（键盘、多媒体）
        if (type == HID_REPORT_TYPE_INPUT)
        {
            if (id == HID_RPT_ID_KEY_IN) // ID = 1，键盘
            {
                if (*pLen >= 2)
                {
                    memcpy(pValue, hidReportKeyIn, 2);
                    *pLen = 2;
                    return SUCCESS;
                }
                else
                {
                    return ATT_ERR_INVALID_VALUE_SIZE; // 缓冲区不足
                }
            }
            else if (id == HID_RPT_ID_MEDIA_IN) // ID = 2，多媒体
            {
                if (*pLen >= 1)
                {
                    *(uint8_t *)pValue = hidReportMediaIn[0];
                    *pLen = 1;
                    return SUCCESS;
                }
                else
                {
                    return ATT_ERR_INVALID_VALUE_SIZE;
                }
            }
            else
            {
                // 未知的 Report ID
                return ATT_ERR_ATTR_NOT_FOUND;
            }
        }
        // 输出或特性报告（你已删除，返回错误）
        else
        {
            return ATT_ERR_ATTR_NOT_FOUND;
        }
    }
    // 其他 UUID（如 Boot 输出）已删除，一律返回未找到
    return ATT_ERR_ATTR_NOT_FOUND;
}

/*********************************************************************
*********************************************************************/

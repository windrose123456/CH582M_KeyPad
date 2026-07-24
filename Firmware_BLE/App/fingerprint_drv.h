/**
 * @file fingerprint_drv.h
 * @brief 指纹模组驱动头文件，基于《指纹模组产品用户手册》V1.1协议定义。
 */

#ifndef __FINGERPRINT_DRV_H__
#define __FINGERPRINT_DRV_H__

#include "CH58x_common.h"

/* ======================== 用户配置区域 ======================== */
// 请根据实际硬件修改此文件中的参数

// 通信接口相关 (例如: UART)
#define FP_BAUD_RATE_DEFAULT    57600  // 默认波特率
#define FP_RX_TIMEOUT_MS    500  // 超时时间
/* ======================== 硬件引脚定义 ======================== */
#define TOUCH_IRQ_PIN      GPIO_Pin_6   
#define TOUCH_IRQ_GPIO     GPIOA   

/* ======================== 协议常量定义 (手册第3章) ======================== */
// 包标识 (Packet Identifier)
#define FP_PACKET_TYPE_CMD      0x01  // 命令包
#define FP_PACKET_TYPE_DATA     0x02  // 数据包 (有后续包)
#define FP_PACKET_TYPE_END      0x08  // 最后一个数据包 (结束包)

// 包头与设备地址
#define FP_PACKET_HEADER        0xEF01
#define FP_DEVICE_ADDR_DEFAULT  0xFFFFFFFF  // 默认设备地址

// 确认码定义 (手册第3.2节)
#define FP_CONFIRM_OK                   0x00  // 指令执行成功
#define FP_CONFIRM_ERR_RECV             0x01  // 数据包接收错误
#define FP_CONFIRM_ERR_NO_FINGER        0x02  // 传感器上没有手指
#define FP_CONFIRM_ERR_IMAGE            0x03  // 录入指纹图像失败
#define FP_CONFIRM_ERR_IMAGE_DRY        0x04  // 指纹图像太干、太淡
#define FP_CONFIRM_ERR_IMAGE_WET        0x05  // 指纹图像太湿、太潮
#define FP_CONFIRM_ERR_IMAGE_MESSY      0x06  // 指纹图像太乱
#define FP_CONFIRM_ERR_FEATURE_LITTLE   0x07  // 特征点太少
#define FP_CONFIRM_ERR_NOT_MATCH        0x08  // 指纹不匹配
#define FP_CONFIRM_ERR_NOT_FOUND        0x09  // 没搜索到指纹
#define FP_CONFIRM_ERR_COMBINE          0x0A  // 特征合并失败
#define FP_CONFIRM_ERR_ADDRESS_OVERRUN  0x0B  // 地址序号超出范围
#define FP_CONFIRM_ERR_LOAD             0x0C  // 从指纹库读模板出错或无效
#define FP_CONFIRM_ERR_UPLOAD           0x0D  // 上传特征失败
#define FP_CONFIRM_ERR_PACKET           0x0E  // 模组不能接收后续数据包
#define FP_CONFIRM_ERR_UPLOAD_IMAGE     0x0F  // 上传图像失败
#define FP_CONFIRM_ERR_DELETE           0x10  // 删除模板失败
#define FP_CONFIRM_ERR_EMPTY_DB         0x11  // 清空指纹库失败
#define FP_CONFIRM_ERR_LOW_POWER        0x12  // 不能进入低功耗状态
#define FP_CONFIRM_ERR_PASSWORD         0x13  // 口令不正确
#define FP_CONFIRM_ERR_BUFFER_IMAGE     0x15  // 缓冲区内无有效原始图
#define FP_CONFIRM_ERR_VERIFY           0x17  // 残留指纹或两次采集间手指没有移动过
#define FP_CONFIRM_ERR_FLASH            0x18  // 读写 FLASH 出错
#define FP_CONFIRM_ERR_RANDOM           0x19  // 随机数生成失败
#define FP_CONFIRM_ERR_REGISTER         0x1E  // 自动注册失败
#define FP_CONFIRM_ERR_DB_FULL          0x1F  // 指纹库满
#define FP_CONFIRM_ERR_EXIST            0x27  // 指纹已存在
#define FP_CONFIRM_ERR_FEATURE_RELATED        0x28  // 指纹特征有关联
#define FP_CONFIRM_ERR_SENSOR_OP_FAIL         0x29  // 传感器操作失败
#define FP_CONFIRM_ERR_MODULE_INFO_NON_EMPTY  0x2A  // 模组信息非空
#define FP_CONFIRM_ERR_MODULE_INFO_EMPTY      0x2B  // 模组信息为空
#define FP_CONFIRM_ERR_OTP_OP_FAIL            0x2C  // OTP 操作失败
#define FP_CONFIRM_ERR_KEY_GEN_FAIL           0x2D  // 秘钥生成失败
#define FP_CONFIRM_ERR_KEY_NOT_EXIST          0x2E  // 秘钥不存在
#define FP_CONFIRM_ERR_SECURE_ALGO_FAIL       0x2F  // 安全算法执行失败
#define FP_CONFIRM_ERR_SECURE_CRYPT_RESULT    0x30  // 安全算法加解密结果有误
#define FP_CONFIRM_ERR_FUNC_ENC_LEVEL_MISMATCH 0x31 // 功能与加密等级不匹配
#define FP_CONFIRM_ERR_KEY_LOCKED             0x32  // 秘钥已锁定
#define FP_CONFIRM_ERR_IMAGE_AREA_SMALL       0x33  // 图像面积小
#define FP_CONFIRM_ERR_IMAGE_UNAVAILABLE      0x34  // 图像不可用
#define FP_CONFIRM_ERR_INVALID_DATA           0x35  // 非法数据
#define FP_CONFIRM_ERR_RESERVED               0x36  // Reserve（保留）

/* ======================== 指令代码定义 (手册第3.3节) ======================== */
// 通用指令集
#define FP_CMD_GET_IMAGE        0x01  // PS_GetImage
#define FP_CMD_GEN_CHAR         0x02  // PS_GenChar
#define FP_CMD_MATCH            0x03  // PS_Match
#define FP_CMD_SEARCH           0x04  // PS_Search
#define FP_CMD_REG_MODEL        0x05  // PS_RegModel
#define FP_CMD_STORE_CHAR       0x06  // PS_StoreChar
#define FP_CMD_LOAD_CHAR        0x07  // PS_LoadChar
#define FP_CMD_UP_CHAR          0x08  // PS_UpChar
#define FP_CMD_DOWN_CHAR        0x09  // PS_DownChar
#define FP_CMD_DELET_CHAR       0x0C  // PS_DeleChar
#define FP_CMD_EMPTY            0x0D  // PS_Empty
// 模块指令集
#define FP_CMD_AUTO_ENROLL      0x31  // PS_AutoEnroll
#define FP_CMD_AUTO_IDENTIFY    0x32  // PS_AutoIdentify
// 维护类指令集
#define FP_CMD_UP_IMAGE         0x0A  // PS_UpImage
#define FP_CMD_DOWN_IMAGE       0x0B  // PS_DownImage
#define FP_CMD_GET_CHIP_SN      0x34  // PS_GetChipSN
#define FP_CMD_HAND_SHAKE       0x35  // PS_HandShake
// 定制类指令集
#define FP_CMD_SET_PWD          0x12  // PS_SetPwd
#define FP_CMD_VERIFY_PWD       0x13  // PS_VfyPwd
#define FP_CMD_GET_RANDOM_CODE  0x14  // PS_GetRandomCode
#define FP_CMD_SET_ADDR         0x15  // PS_SetChipAddr
// 安全指令集 (需结合加密等级使用)
#define FP_CMD_GET_KEYT         0xE0  // PS_GetKeyt
#define FP_CMD_LOCK_KEYT        0xE1  // PS_LockKeyt
#define FP_CMD_SEC_SEARCH       0xE4  // PS_SecuritySearch

/* ======================== 数据结构与类型定义 ======================== */
typedef struct {
    uint16_t packet_header;  // 0xEF01
    uint32_t device_addr;
    uint8_t  packet_type;
    uint16_t packet_len;     // 包长度 = 字节数 - 校验和字节数
    uint8_t  cmd_code;
    uint8_t  params[16];     // 可变参数，根据具体指令调整
} FP_CmdPacket_t;

// 应答包结构体 (手册第3.2节 表3-4)
typedef struct {
    uint16_t packet_header;  // 0xEF01
    uint32_t device_addr;
    uint8_t  packet_type;    // 应答包类型为0x07
    uint16_t packet_len;
    uint8_t  confirm_code;
    uint8_t  return_params[16]; // 可变返回参数
} FP_AckPacket_t;


/* ======================== 缓冲区大小 ======================== */
#define FP_BUFFER_SIZE          128

/* ======================== 驱动函数声明 ======================== */

/**
 * @brief 初始化指纹模组驱动，配置UART等硬件。
 * @return 0: 成功, 非0: 错误码
 */
int FP_Init(void);

/**
 * @brief 发送握手指令，检测模组是否正常工作。(指令代码 0x35)
 * @return 0: 成功 (收到确认码 0x00), 非0: 错误码
 */
int FP_Handshake(void);

/**
 * @brief 获取指纹图像。(指令代码 0x01)
 * @return 确认码，0表示成功
 */
int FP_GetImage(void);

/**
 * @brief 从图像生成特征，存入BufferID指定的缓冲区。(指令代码 0x02)
 * @param buffer_id 缓冲区ID (BufferID)
 * @return 确认码，0表示成功
 */
int FP_GenChar(uint8_t buffer_id);

/**
 * @brief 精确比对Buffer1和Buffer2中的特征。(指令代码 0x03)
 * @param score [out] 匹配分数 (需根据协议解析应答包)
 * @return 确认码，0表示匹配成功
 */
int FP_Match(uint16_t *score);

/**
 * @brief 以Buffer中的特征搜索指纹库。(指令代码 0x04)
 * @param buffer_id 缓冲区ID
 * @param start_page 起始页
 * @param page_num 页数
 * @param page_id [out] 匹配的页码 (需解析应答)
 * @param score [out] 匹配分数
 * @return 确认码，0表示搜索到
 */
int FP_Search(uint8_t buffer_id, uint16_t start_page, uint16_t page_num, uint16_t *page_id, uint16_t *score);

/**
 * @brief 将特征文件融合后生成一个模板。(指令代码 0x05)
 * @return 确认码，0表示成功
 */
int FP_RegModel(void);

/**
 * @brief 将模板存入指纹库指定位置。(指令代码 0x06)
 * @param buffer_id 缓冲区ID (默认1)
 * @param page_id 指纹库位置号
 * @return 确认码，0表示成功
 */
int FP_StoreChar(uint8_t buffer_id, uint16_t page_id);

/**
 * @brief 从指纹库读出模板到缓冲区。(指令代码 0x07)
 * @param buffer_id 缓冲区ID (默认2)
 * @param page_id 指纹库位置号
 * @return 确认码，0表示成功
 */
int FP_LoadChar(uint8_t buffer_id, uint16_t page_id);

/**
 * @brief 上传缓冲区中的模板。(指令代码 0x08)
 * @param buffer_id 缓冲区ID
 * @return 确认码，0表示成功，后续需接收数据包
 */
int FP_UpChar(uint8_t buffer_id);

/**
 * @brief 下载模板到模组缓冲区。(指令代码 0x09)
 * @param buffer_id 缓冲区ID (默认1)
 * @return 确认码，0表示成功，后续需发送数据包
 */
int FP_DownChar(uint8_t buffer_id);

/**
 * @brief 删除指纹库中的模板。(指令代码 0x0C)
 * @param page_id 起始页码
 * @param count 删除的模板个数
 * @return 确认码，0表示成功
 */
int FP_DeleteChar(uint16_t page_id, uint16_t count);

/**
 * @brief 清空指纹库。(指令代码 0x0D)
 * @return 确认码，0表示成功
 */
int FP_Empty(void);

/**
 * @brief 将特征文件融合后生成一个模板。(指令代码 0x05)
 * @return 确认码，0表示成功
 */
int FP_RegModel(void);

/**
 * @brief 从指纹库读出模板到缓冲区。(指令代码 0x07)
 * @param buffer_id 缓冲区ID (默认2)
 * @param page_id 指纹库位置号
 * @return 确认码，0表示成功
 */
int FP_LoadChar(uint8_t buffer_id, uint16_t page_id);

/**
 * @brief 删除指纹库中指定ID开始的N个模板。(指令代码 0x0C)
 * @param page_id 起始页码
 * @param count 删除的模板个数
 * @return 确认码，0表示成功
 */
int FP_DeleteChar(uint16_t page_id, uint16_t count);

/**
 * @brief 清空指纹库。(指令代码 0x0D)
 * @return 确认码，0表示成功
 */
int FP_Empty(void);

#endif /* FINGERPRINT_DRV_H */
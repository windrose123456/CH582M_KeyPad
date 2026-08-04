/**
 * @file fingerprint_drv.c
 * @brief 指纹模组驱动实现，基于《指纹模组产品用户手册》V1.1协议。
 */

#include "fingerprint_drv.h"
#include <string.h> // for memcpy
#include "CONFIG.h"
#include "ring_buffer.h"
#include "main.h"

/* ======================== 静态变量 ======================== */
static uint8_t uart3_tx_buf[FP_BUFFER_SIZE];
static uint8_t uart3_rx_buf[FP_BUFFER_SIZE];
// 创建独立的环形缓冲区实例
static ring_buffer_t uart3_rx_ring;

/* ======================== 状态机变量 ======================== */
typedef enum {
    FP_STATE_IDLE = 0,         // 空闲，等待发送指令
    FP_STATE_WAITING,          // 已发送指令，等待应答
    FP_STATE_DONE,             // 收到应答或超时
} FP_State_t;

static volatile FP_State_t g_fp_state = FP_STATE_IDLE;
static volatile uint8_t  g_frame_ready = 0;   // 由 UART 超时中断置位
static uint32_t g_wait_start_tick = 0;
static uint8_t  g_cmd_sent = 0;                // 最近发送的指令码
static int      g_result_code = 0;             // 最终结果（确认码或负错误码）
static FP_AckPacket_t g_last_ack;              // 存储最后收到的应答

/* ======================== 内部函数声明 ======================== */
static uint16_t calculate_checksum(uint8_t *data, uint16_t len);
static void send_packet(uint8_t packet_type, uint8_t cmd_code, uint8_t *params, uint16_t param_len);
static int parse_ack_packet(uint8_t *buf, uint16_t len, FP_AckPacket_t *ack);

/* ======================== 公开接口函数实现 ======================== */

int FP_Init(void) {
    // 修改模块波特率

    // 模块中断引脚初始化
    GPIOA_ModeCfg(TOUCH_IRQ_PIN, GPIO_ModeIN_PU);
    GPIOA_ITModeCfg(TOUCH_IRQ_PIN, GPIO_ITMode_RiseEdge);
    PFIC_EnableIRQ(GPIO_A_IRQn);

    // UART3 init
    GPIOA_SetBits(bTXD3);
    GPIOA_ModeCfg(bRXD3, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(bTXD3, GPIO_ModeOut_PP_5mA);
    UART3_DefInit();
    UART3_ByteTrigCfg(UART_4BYTE_TRIG);
    UART3_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
    PFIC_EnableIRQ(UART3_IRQn);

    // 超时定时器
    TMR0_TimerInit(600000);
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    PFIC_EnableIRQ(TMR0_IRQn);
    TMR0_Disable();

    // 初始化环形缓冲区
    ring_buffer_init(&uart3_rx_ring, uart3_rx_buf, sizeof(uart3_rx_buf));

    // 接收模块上电完成字节 FF 55

    g_fp_state = FP_STATE_IDLE;
    g_frame_ready = 0;

    return 0;
}

int FP_Handshake(void) {
    if (g_fp_state != FP_STATE_IDLE) return -1;
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_HAND_SHAKE, NULL, 0);
    g_cmd_sent = FP_CMD_HAND_SHAKE;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_GetImage(void) {
    if (g_fp_state != FP_STATE_IDLE) return -1;
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_GET_IMAGE, NULL, 0);
    g_cmd_sent = FP_CMD_GET_IMAGE;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_GenChar(uint8_t buffer_id) {
    if (g_fp_state != FP_STATE_IDLE) return -1;
    uint8_t params[1] = {buffer_id};
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_GEN_CHAR, params, 1);
    g_cmd_sent = FP_CMD_GEN_CHAR;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_Match() {
    if (g_fp_state != FP_STATE_IDLE) return -1;
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_MATCH, NULL, 0);
    g_cmd_sent = FP_CMD_MATCH;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_Search(uint8_t buffer_id, uint16_t start_page, uint16_t page_num) {
    if (g_fp_state != FP_STATE_IDLE) return -1;
    uint8_t params[5] = {
        buffer_id,
        (start_page >> 8) & 0xFF,
        start_page & 0xFF,
        (page_num >> 8) & 0xFF,
        page_num & 0xFF
    };
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_SEARCH, params, 5);
    g_cmd_sent = FP_CMD_SEARCH;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_RegModel(void) {
    // 合并特征，无参数
    // 验证数据：EF 01 FF FF FF FF 01 00 03 05 00 09
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_REG_MODEL, NULL, 0);
    g_cmd_sent = FP_CMD_REG_MODEL;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_StoreChar(uint8_t buffer_id, uint16_t page_id) {
    if (g_fp_state != FP_STATE_IDLE) return -1;
    uint8_t params[3] = {
        buffer_id,
        (page_id >> 8) & 0xFF,
        page_id & 0xFF
    };
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_STORE_CHAR, params, 3);
    g_cmd_sent = FP_CMD_STORE_CHAR;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_LoadChar(uint8_t buffer_id, uint16_t page_id) {
    // 从指纹库读出模板到缓冲区
    // 验证数据：EF 01 FF FF FF FF 01 00 06 07 02 00 00 00 10
    // 参数：BufferID(1B) + PageID(2B)
    uint8_t params[3];
    params[0] = buffer_id;                 // 缓冲区号，默认2
    params[1] = (page_id >> 8) & 0xFF;    // 位置号高字节
    params[2] = page_id & 0xFF;           // 位置号低字节


    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_LOAD_CHAR, params, sizeof(params));
    g_cmd_sent = FP_CMD_LOAD_CHAR;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_DeleteChar(uint16_t page_id, uint16_t count) {
    // 删除指纹库中指定ID开始的N个模板
    // 验证数据：EF 01 FF FF FF FF 01 00 07 0C 00 00 00 01 00 15
    // 参数：PageID(2B) + N(2B)
    uint8_t params[4];
    params[0] = (page_id >> 8) & 0xFF;    // 起始页码高字节
    params[1] = page_id & 0xFF;           // 起始页码低字节
    params[2] = (count >> 8) & 0xFF;      // 删除个数高字节
    params[3] = count & 0xFF;             // 删除个数低字节

    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_DELET_CHAR, params, sizeof(params));
    g_cmd_sent = FP_CMD_DELET_CHAR;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_Empty(void) {
    // 清空指纹库，无参数
    // 验证数据：EF 01 FF FF FF FF 01 00 03 0D 00 11
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_EMPTY, NULL, 0);
    g_cmd_sent = FP_CMD_EMPTY;
    g_wait_start_tick = TMOS_GetSystemClock();
    g_fp_state = FP_STATE_WAITING;
    return 0;
}

int FP_WriteReg(void)
{
    // send_packet(FP_PACKET_TYPE_CMD, FP_CMD_WRITE_REG, NULL, 0);
    // g_cmd_sent = FP_CMD_WRITE_REG;
    // g_wait_start_tick = TMOS_GetSystemClock();
    // g_fp_state = FP_STATE_WAITING;
    return 0;   
}

void FP_Process(void) {
    if (g_fp_state != FP_STATE_WAITING) return;

    // 1. 检查硬件超时标志
    if (g_frame_ready) {
        g_frame_ready = 0;

        // 从环形缓冲区取出所有数据
        uint8_t packet[FP_BUFFER_SIZE];
        uint16_t len = ring_buffer_available(&uart3_rx_ring);
        if (len > 0) {
            ring_buffer_pop_multiple(&uart3_rx_ring, packet, len);
        }
        debug_hex_dump(packet, len, "FP rx: ");
        // 解析应答包
        if (parse_ack_packet(packet, len, &g_last_ack) == 0) {
            g_result_code = g_last_ack.confirm_code;  // 成功返回确认码
        } else {
            g_result_code = -2;  // 解析失败（校验错等）
        }
        g_fp_state = FP_STATE_DONE;
        return;
    }

    // // 2. 软件超时检测（兜底）
    // if (TMOS_GetSystemClock() - g_wait_start_tick > FP_RX_TIMEOUT_MS) {
    //     g_result_code = -1;     // 超时
    //     g_fp_state = FP_STATE_DONE;
    // }
}

int FP_GetResult(FP_AckPacket_t *ack) {
    if (g_fp_state != FP_STATE_DONE) {
        return -3;  // 尚未完成
    }
    if (ack != NULL) {
        memcpy(ack, &g_last_ack, sizeof(FP_AckPacket_t));
    }
    g_fp_state = FP_STATE_IDLE;   // 消费后自动重置
    return g_result_code;
}

int FP_IsBusy(void) {
    return (g_fp_state == FP_STATE_WAITING) ? 1 : 0;
}

/* ======================== 内部函数实现 ======================== */

static uint16_t calculate_checksum(uint8_t *data, uint16_t len) {
    // 校验和是从包标识至校验和之间所有字节之和 (手册第3.1节)
    uint32_t sum = 0;
    // 跳过包头(2B)和设备地址(4B)，从包标识开始计算
    for (uint16_t i = 6; i < len; i++) {
        sum += data[i];
    }
    return (uint16_t)(sum & 0xFFFF); // 超出2字节忽略进位
}

static void send_packet(uint8_t packet_type, uint8_t cmd_code, uint8_t *params, uint16_t param_len) {
    uint16_t packet_len = 1 + param_len + 2; // 包长度 = 包标识(1B) + 指令码(1B) + 参数(param_len B) + 校验和(2B) - 校验和(2B)本身不计入？ *需再确认协议*
    // 手册第3.1节：包长度 = 包长度至校验和(指令、参数或数据)的总字节数，包含校验和，但不包含包长度本身的字节数。
    // 因此，对于命令包：总字节数 = 包头(2) + 设备地址(4) + 包标识(1) + 包长度(2) + 指令码(1) + 参数(N) + 校验和(2) = 10 + N
    // 包长度字段的值 = 总字节数 - 4 (包头+设备地址) - 2 (包长度字段本身) = 4 + N? *需严格按照示例计算*
    // 这里我们根据示例简化计算，实际需精确遵循协议
    uint16_t content_len = 1 + param_len + 2; // 包标识(1) + 指令码(1) + 参数(N) + 校验和(2) = N+4
    uint8_t *packet = uart3_tx_buf;
    uint16_t idx = 0;

    // 包头
    packet[idx++] = (FP_PACKET_HEADER >> 8) & 0xFF;
    packet[idx++] = FP_PACKET_HEADER & 0xFF;
    // 设备地址
    packet[idx++] = (FP_DEVICE_ADDR_DEFAULT >> 24) & 0xFF;
    packet[idx++] = (FP_DEVICE_ADDR_DEFAULT >> 16) & 0xFF;
    packet[idx++] = (FP_DEVICE_ADDR_DEFAULT >> 8) & 0xFF;
    packet[idx++] = FP_DEVICE_ADDR_DEFAULT & 0xFF;
    // 包标识
    packet[idx++] = packet_type;
    // 包长度
    packet[idx++] = (content_len >> 8) & 0xFF;
    packet[idx++] = content_len & 0xFF;
    // 指令码
    packet[idx++] = cmd_code;
    // 参数
    if (params != NULL && param_len > 0) {
        memcpy(&packet[idx], params, param_len);
        idx += param_len;
    }
    // 计算校验和
    uint16_t checksum = calculate_checksum(packet, idx);
    packet[idx++] = (checksum >> 8) & 0xFF;
    packet[idx++] = checksum & 0xFF;

    // 发送数据包 (需要实际的UART发送函数)
    UART3_SendString(packet, idx);
    debug_hex_dump(packet, idx, "FP tx: ");
}

static int parse_ack_packet(uint8_t *buf, uint16_t len, FP_AckPacket_t *ack) {
    if (len < 12) return -1;   // 最小应答包长度
    if (buf[0] != 0xEF || buf[1] != 0x01) return -1;

    // 校验和（从包标识开始到数据结束）
    uint16_t calc = 0;
    for (uint16_t i = 6; i < len - 2; i++) {
        calc += buf[i];
    }
    uint16_t recv = (buf[len-2] << 8) | buf[len-1];
    if (calc != recv) return -2;

    ack->packet_header = FP_PACKET_HEADER;
    ack->device_addr = 0xFFFFFFFF;
    ack->packet_type = buf[6];
    ack->packet_len = (buf[7] << 8) | buf[8];
    ack->confirm_code = buf[9];
    uint8_t plen = ack->packet_len - 3;
    if (plen > 16) plen = 16;
    memcpy(ack->return_params, &buf[10], plen);
    return 0;
}

uint16_t FP_drv_ProcessEvent(uint8_t task_id, uint16_t events)
{
    return 0;
}

/* ======================== 中断处理 ======================== */

__INTERRUPT __HIGH_CODE void UART3_IRQHandler(void)
{
    switch(UART3_GetITFlag())
    {
        case UART_II_RECV_RDY: // 数据达到设置触发点
            while (UART3_GetLinSTA() & STA_RECV_DATA) {
                uint8_t byte = UART3_RecvByte();
                ring_buffer_push(&uart3_rx_ring, byte);  // 存入实例
                //UART3_SendString(&byte, 1);
    
                // 清0计数器
                R8_TMR0_CTRL_MOD |= RB_TMR_ALL_CLEAR;   // 复位计数器
                R8_TMR0_CTRL_MOD &= ~RB_TMR_ALL_CLEAR;  // 解除复位
                TMR0_Enable();
            }
            break;

        case UART_II_RECV_TOUT: // 接收超时，暂时一帧数据接收完成
            while (UART3_GetLinSTA() & STA_RECV_DATA) {
                uint8_t byte = UART3_RecvByte();
                ring_buffer_push(&uart3_rx_ring, byte);
            }
            break;

        default:
            break;
    }
}

__INTERRUPT __HIGH_CODE void TMR0_IRQHandler(void)
{
    if (TMR0_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
        TMR0_Disable();
        // 置帧完成标志
        g_frame_ready = 1;
    }
}


        // static uint8_t fp_test_flag = 0; // 简单测试使用
        // extern uint8_t touch_irq_flag;
        // if(touch_irq_flag)
        // {
        //     touch_irq_flag = 0;
        //     if (fp_test_flag == 0) 
        //     {
        //         FP_GetImage();
        //         fp_test_flag++;
        //     }
        //     else if (fp_test_flag == 1)
        //     {
        //         FP_Search(1, 0, 100);
        //         fp_test_flag = 0;
        //     }
            // 下面是录入指纹过程
            // else if (fp_test_flag == 1)
            // {
            //     FP_GenChar(1); //
            //     fp_test_flag++;
            // }
            // else if (fp_test_flag == 2)
            // {
            //     FP_GetImage();
            //     fp_test_flag++;
            // }
            // else if (fp_test_flag == 3)
            // {
            //     FP_GenChar(2); //
            //     fp_test_flag++;
            // }
            // else if (fp_test_flag == 4)
            // {
            //     FP_RegModel();
            //     fp_test_flag++;
            // }
            // else if (fp_test_flag == 5)
            // {
            //     FP_StoreChar(1, 0);
            //     fp_test_flag = 0;
            // }
        // }

        // FP_Process(); // 后续这个新建一个tmos任务标志位，或者也可以不新建，得先看怎么样解锁电脑
        // FP_AckPacket_t *ack;
        // int fp_ret = FP_GetResult(ack);
/**
 * @file fingerprint_drv.c
 * @brief 指纹模组驱动实现，基于《指纹模组产品用户手册》V1.1协议。
 */

#include "fingerprint_drv.h"
#include <string.h> // for memcpy
#include "CONFIG.h"

/* ======================== 静态变量 ======================== */
static uint8_t g_tx_buffer[FP_BUFFER_SIZE];
static uint8_t g_rx_buffer[FP_BUFFER_SIZE];
// 接收完成标志（由超时中断置位）
static volatile uint8_t g_frame_ready = 0;
static uint16_t g_frame_len = 0;  // 实际接收到的字节数

/* ======================== 内部函数声明 ======================== */
static uint16_t calculate_checksum(uint8_t *data, uint16_t len);
static void send_packet(uint8_t packet_type, uint8_t cmd_code, uint8_t *params, uint16_t param_len);
static int receive_ack(FP_AckPacket_t *ack);

/* ======================== 公开接口函数实现 ======================== */

int FP_Init(void) {
    // UART3 init
    GPIOA_SetBits(bTXD3);
    GPIOA_ModeCfg(bTXD3, GPIO_ModeOut_PP_5mA);
    UART3_DefInit();
    UART3_INTCfg(ENABLE, UART_II_RECV_RDY | UART_II_RECV_TOUT | RB_IER_LINE_STAT);
    // 使能UART3的全局中断响应
    PFIC_EnableIRQ(UART3_IRQn);
    // 2. 可选择发送握手指令测试连接

    return FP_Handshake();
}

int FP_Handshake(void) {
    // 指令格式：无参数
    // 发送指令包
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_HAND_SHAKE, NULL, 0);

    // 接收应答包
    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    return (ret == 0) ? ack.confirm_code : ret;
}

int FP_GetImage(void) {
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_GET_IMAGE, NULL, 0);
    
    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    return (ret == 0) ? ack.confirm_code : ret;
}

int FP_GenChar(uint8_t buffer_id) {
    uint8_t params[1];
    params[0] = buffer_id; // BufferID
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_GEN_CHAR, params, sizeof(params));

    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    return (ret == 0) ? ack.confirm_code : ret;
}

int FP_Match(uint16_t *score) {
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_MATCH, NULL, 0);

    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    if (ret == 0 && ack.confirm_code == FP_CONFIRM_OK && score != NULL) {
        // 根据手册，应答包包含2字节得分，高字节在前
        *score = (ack.return_params[0] << 8) | ack.return_params[1];
    }
    return (ret == 0) ? ack.confirm_code : ret;
}

int FP_Search(uint8_t buffer_id, uint16_t start_page, uint16_t page_num, uint16_t *page_id, uint16_t *score) {
    uint8_t params[6];
    params[0] = buffer_id; // BufferID
    params[1] = (start_page >> 8) & 0xFF; // StartPage High
    params[2] = start_page & 0xFF;        // StartPage Low
    params[3] = (page_num >> 8) & 0xFF;   // PageNum High
    params[4] = page_num & 0xFF;          // PageNum Low
    // 第6字节为参数，手册示例中为0，可扩展
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_SEARCH, params, 5); // 注意包长度

    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    if (ret == 0 && ack.confirm_code == FP_CONFIRM_OK) {
        // 根据手册，应答包包含页码(2字节)和得分(2字节)
        if (page_id != NULL) {
            *page_id = (ack.return_params[0] << 8) | ack.return_params[1];
        }
        if (score != NULL) {
            *score = (ack.return_params[2] << 8) | ack.return_params[3];
        }
    }
    return (ret == 0) ? ack.confirm_code : ret;
}

int FP_RegModel(void) {
    // 合并特征，无参数
    // 验证数据：EF 01 FF FF FF FF 01 00 03 05 00 09
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_REG_MODEL, NULL, 0);

    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    return (ret == 0) ? ack.confirm_code : ret;
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

    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    return (ret == 0) ? ack.confirm_code : ret;
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

    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    return (ret == 0) ? ack.confirm_code : ret;
}

int FP_Empty(void) {
    // 清空指纹库，无参数
    // 验证数据：EF 01 FF FF FF FF 01 00 03 0D 00 11
    send_packet(FP_PACKET_TYPE_CMD, FP_CMD_EMPTY, NULL, 0);

    int ret = 0;
    FP_AckPacket_t ack;
    ret = receive_ack(&ack);
    return (ret == 0) ? ack.confirm_code : ret;
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
    uint16_t packet_len_field = content_len; // 实际协议中此值为 content_len
    uint8_t *packet = g_tx_buffer;
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
    packet[idx++] = (packet_len_field >> 8) & 0xFF;
    packet[idx++] = packet_len_field & 0xFF;
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
}

__INTERRUPT __HIGH_CODE void UART3_IRQHandler(void) {
    uint8_t int_flag = UART3_GetITFlag();
    uint16_t idx = 0;

    if (int_flag == UART_II_RECV_RDY) {
        // 接收数据可用，读取所有字节到环形缓冲区
        
        while (UART3_GetLinSTA() & RB_LSR_RECV_RDY) {
            uint8_t byte = UART3_RecvByte();
            if (idx <= FP_BUFFER_SIZE) {  // 防止溢出
                g_rx_buffer[idx] = byte;
                idx++;
            } else {
                // 缓冲区溢出，可置错误标志
            }
        }
        // 重置超时计数器（由硬件自动）
    } else if (int_flag == UART_II_RECV_TOUT) {
        // 接收超时：一帧数据结束
        g_frame_len = idx;
        if (g_frame_len > 0) {
            g_frame_ready = 1;  // 通知主循环有完整帧
        }
    } else if (int_flag == UART_II_LINE_STAT) {
        // 处理线路错误，读取状态寄存器清除标志
        UART3_GetLinSTA();
    }
}

static int receive_ack(FP_AckPacket_t *ack) {
    uint32_t start_tick = TMOS_GetSystemClock();
    while (!g_frame_ready) {
        if ((TMOS_GetSystemClock() - start_tick) > FP_RX_TIMEOUT_MS) {
            return -1;  // 超时
        }
    }

    while (idx < (9 + pkt_len)) {
        // if (UART_ReceiveByte(&g_rx_buffer[idx], FP_RX_TIMEOUT_MS) != 0)
        //     return -1;
        idx++;
    }

    // 2. 校验包头
    if (g_rx_buffer[0] != 0xEF || g_rx_buffer[1] != 0x01) {
        return -1;  // 包头错误
    }

    // 4. 校验和验证
    uint16_t calc_sum = calculate_checksum(g_rx_buffer, idx - 2);
    uint16_t recv_sum = (g_rx_buffer[idx - 2] << 8) | g_rx_buffer[idx - 1];
    if (calc_sum != recv_sum) {
        return -2;  // 校验和错误
    }

    // 5. 解析应答字段
    ack->packet_header = FP_PACKET_HEADER;
    ack->device_addr   = 0xFFFFFFFF;
    ack->packet_type   = g_rx_buffer[6];
    ack->packet_len    = pkt_len;
    ack->confirm_code  = g_rx_buffer[9];

    // 6. 解析返回参数（从第10字节开始，长度 = pkt_len - 3）
    uint8_t param_len = pkt_len - 3;  // 减去确认码(1) + 校验和(2)
    if (param_len > 16) param_len = 16;
    memcpy(ack->return_params, &g_rx_buffer[10], param_len);

    return 0;
}


/* ======================== 示例主函数 (用于演示驱动用法) ======================== */
/*
#include <stdio.h>

int main(void) {
    printf("Initializing fingerprint driver...\n");
    if (FP_Init() != 0) {
        printf("Driver init failed!\n");
        return 1;
    }
    printf("Handshake success!\n");

    printf("Place finger on sensor...\n");
    while (FP_GetImage() != FP_CONFIRM_OK) {
        // 等待手指按压
    }
    printf("Finger captured.\n");

    // 生成特征到Buffer1
    if (FP_GenChar(1) == FP_CONFIRM_OK) {
        // 在指纹库中搜索 (假设从0页开始搜100页)
        uint16_t found_page, score;
        if (FP_Search(1, 0, 100, &found_page, &score) == FP_CONFIRM_OK) {
            printf("Fingerprint matched! PageID: %d, Score: %d\n", found_page, score);
        } else {
            printf("No match found.\n");
        }
    }

    return 0;
}
*/
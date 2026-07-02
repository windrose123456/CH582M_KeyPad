// 小屏幕驱动（如SSD1306/ST7789）。封装屏幕初始化、显示字符/图标。

#include "oled.h"
#include "font.h"  // 字模数组，见下文
#include <stdlib.h>

#ifdef OLED_HARDWARE_I2C

/* ========== 硬件I2C底层 ========== */
static void I2C_WriteCmd(uint8_t cmd)
{
    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));  // 等待START发送完成

    I2C_Send7bitAddress(OLED_ADDRESS, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(0x00);  // Co=0, D/C#=0 (命令)
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(cmd);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(ENABLE);
}

static void I2C_WriteData(uint8_t data)
{
    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(OLED_ADDRESS, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(0x40);  // Co=0, D/C#=1 (数据)
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(data);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(ENABLE);
}

static void I2C_WriteMultiData(uint8_t *data, uint16_t len)
{
    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(OLED_ADDRESS, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(0x40);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    for(uint16_t i = 0; i < len; i++) {
        I2C_SendData(data[i]);
        while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }

    I2C_GenerateSTOP(ENABLE);
}

/* ========== 显存 ========== */
static uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];  // 128 * 32 / 8 = 512字节

/* ========== 初始化 ========== */
void OLED_Init(void)
{
    // I2C硬件初始化（PB12-SDA, PB13-SCL）
    GPIOB_ModeCfg(GPIO_Pin_12, GPIO_ModeOut_PP_20mA);
    GPIOB_ModeCfg(GPIO_Pin_13, GPIO_ModeOut_PP_20mA);
    I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, 0x00);

    // SSD1306初始化序列
    I2C_WriteCmd(0xAE);  // 关闭显示
    I2C_WriteCmd(0xD5);  // 设置时钟分频
    I2C_WriteCmd(0x80);
    I2C_WriteCmd(0xA8);  // 设置行数
    I2C_WriteCmd(0x1F);  // 32行 (128x32)
    I2C_WriteCmd(0xD3);  // 设置显示偏移
    I2C_WriteCmd(0x00);
    I2C_WriteCmd(0x40);  // 设置起始行
    I2C_WriteCmd(0x8D);  // 电荷泵设置
    I2C_WriteCmd(0x14);  // 开启电荷泵
    I2C_WriteCmd(0x20);  // 设置内存寻址模式
    I2C_WriteCmd(0x02);  // 页寻址模式
    I2C_WriteCmd(0xA1);  // 段重映射 (列127映射到SEG0)
    I2C_WriteCmd(0xC0);  // 不反转COM输出
    I2C_WriteCmd(0xDA);  // COM引脚硬件配置
    I2C_WriteCmd(0x02);
    I2C_WriteCmd(0x81);  // 对比度设置
    I2C_WriteCmd(0x7F);
    I2C_WriteCmd(0xD9);  // 预充电周期
    I2C_WriteCmd(0xF1);
    I2C_WriteCmd(0xDB);  // VCOM检测电平
    I2C_WriteCmd(0x40);
    I2C_WriteCmd(0xA4);  // 使用GDDRAM内容显示
    I2C_WriteCmd(0xA6);  // 正常显示（0=亮）
    I2C_WriteCmd(0xAF);  // 开启显示

    OLED_Clear();
    OLED_Update();
}

/* ========== 显存操作 ========== */
void OLED_Clear(void)
{
    for(uint16_t i = 0; i < sizeof(OLED_Buffer); i++) {
        OLED_Buffer[i] = 0x00;
    }
}

void OLED_Update(void)
{
    // 设置页地址和列起始地址
    I2C_WriteCmd(0x21);  // 列地址范围
    I2C_WriteCmd(0x00);  // 起始列
    I2C_WriteCmd(0x7F);  // 结束列 (127)
    I2C_WriteCmd(0x22);  // 页地址范围
    I2C_WriteCmd(0x00);  // 起始页
    I2C_WriteCmd(0x03);  // 结束页 (3)

    I2C_WriteMultiData(OLED_Buffer, sizeof(OLED_Buffer));
}

#endif

#ifndef OLED_HARDWARE_I2C

/* ========== 软件I2C引脚定义 ========== */
#define OLED_SCL_PIN   GPIO_Pin_13
#define OLED_SDA_PIN   GPIO_Pin_12

#define OLED_SCL_HIGH()  GPIOB_SetBits(OLED_SCL_PIN)
#define OLED_SCL_LOW()   GPIOB_ResetBits(OLED_SCL_PIN)
#define OLED_SDA_HIGH()  GPIOB_SetBits(OLED_SDA_PIN)
#define OLED_SDA_LOW()   GPIOB_ResetBits(OLED_SDA_PIN)
#define OLED_SDA_READ()  GPIOB_ReadPortPin(OLED_SDA_PIN)

/* ========== 软件I2C底层 ========== */
static void I2C_Delay(void)
{
    for(volatile uint32_t i = 0; i < 50; i++);
}

static void I2C_Start(void)
{
    OLED_SDA_HIGH();
    OLED_SCL_HIGH();
    I2C_Delay();
    OLED_SDA_LOW();
    I2C_Delay();
    OLED_SCL_LOW();
}

static void I2C_Stop(void)
{
    OLED_SDA_LOW();
    OLED_SCL_HIGH();
    I2C_Delay();
    OLED_SDA_HIGH();
    I2C_Delay();
}

static uint8_t I2C_WaitAck(void)
{
    OLED_SDA_HIGH();
    OLED_SCL_HIGH();
    I2C_Delay();
    uint8_t ack = OLED_SDA_READ();
    OLED_SCL_LOW();
    return ack;
}

static void I2C_WriteByte(uint8_t data)
{
    for(uint8_t i = 0; i < 8; i++) {
        if(data & 0x80) OLED_SDA_HIGH();
        else OLED_SDA_LOW();
        data <<= 1;
        OLED_SCL_HIGH();
        I2C_Delay();
        OLED_SCL_LOW();
        I2C_Delay();
    }
}

static void I2C_WriteBytes(uint8_t addr, uint8_t *data, uint16_t len)
{
    I2C_Start();
    I2C_WriteByte(addr << 1);  // 7位地址左移1位变为8位写地址
    I2C_WaitAck();
    for(uint16_t i = 0; i < len; i++) {
        I2C_WriteByte(data[i]);
        I2C_WaitAck();
    }
    I2C_Stop();
}

static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};  // 0x00 = 命令
    I2C_WriteBytes(OLED_ADDRESS, buf, 2);
}

static void OLED_WriteData(uint8_t data)
{
    uint8_t buf[2] = {0x40, data};  // 0x40 = 数据
    I2C_WriteBytes(OLED_ADDRESS, buf, 2);
}

static void OLED_WriteMultiData(uint8_t *data, uint16_t len)
{
    uint8_t *buf = malloc(len + 1);
    if(!buf) return;
    buf[0] = 0x40;
    for(uint16_t i = 0; i < len; i++) buf[i + 1] = data[i];
    I2C_WriteBytes(OLED_ADDRESS, buf, len + 1);
    free(buf);
}

/* ========== 显存 ========== */
static uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];

/* ========== 初始化 ========== */
void OLED_Init(void)
{
    GPIOB_ModeCfg(OLED_SCL_PIN, GPIO_ModeOut_PP_20mA);
    GPIOB_ModeCfg(OLED_SDA_PIN, GPIO_ModeOut_PP_20mA);
    OLED_SCL_HIGH();
    OLED_SDA_HIGH();

    // 初始化序列（与硬件版本相同）
    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0xD5); OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xA8); OLED_WriteCmd(0x1F);
    OLED_WriteCmd(0xD3); OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D); OLED_WriteCmd(0x14);
    OLED_WriteCmd(0x20); OLED_WriteCmd(0x02);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xC0);
    OLED_WriteCmd(0xDA); OLED_WriteCmd(0x02);
    OLED_WriteCmd(0x81); OLED_WriteCmd(0x7F);
    OLED_WriteCmd(0xD9); OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDB); OLED_WriteCmd(0x40);
    OLED_WriteCmd(0xA4);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xAF);

    OLED_Clear();
    OLED_Update();
}

/* ========== 显存操作 ========== */
void OLED_Clear(void)
{
    for(uint16_t i = 0; i < sizeof(OLED_Buffer); i++) {
        OLED_Buffer[i] = 0x00;
    }
}

void OLED_Update(void)
{
    OLED_WriteCmd(0x21); OLED_WriteCmd(0x00); OLED_WriteCmd(0x7F);
    OLED_WriteCmd(0x22); OLED_WriteCmd(0x00); OLED_WriteCmd(0x03);
    OLED_WriteMultiData(OLED_Buffer, sizeof(OLED_Buffer));
}

#endif

/* ========== 绘图函数 ========== */
static void OLED_SetPixel(uint8_t x, uint8_t y, uint8_t on)
{
    if(x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint16_t idx = x + (y / 8) * OLED_WIDTH;
    if(on) {
        OLED_Buffer[idx] |= (1 << (y % 8));
    } else {
        OLED_Buffer[idx] &= ~(1 << (y % 8));
    }
}

/* ========== 字符显示 ========== */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t width, uint8_t height)
{
    if(x > OLED_WIDTH - width || y > OLED_HEIGHT - height) return;

    uint8_t c = ch - ' ';
    uint8_t *font;

    if(width == 8 && height == 8) {
        font = (uint8_t*)FONT_8x8[c];
    } else if(width == 6 && height == 8) {
        font = (uint8_t*)FONT_8x6[c];   // 注意你的数组名叫 FONT_8x6，实际是 6x8
    } else {
        return;  // 不支持的字体
    }

    for(uint8_t col = 0; col < width; col++) {
        for(uint8_t row = 0; row < height; row++) {
            if(font[col] & (1 << row)) {
                OLED_SetPixel(x + col, y + row, 1);
            }
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t width, uint8_t height)
{
    uint8_t x_pos = x;
    uint8_t char_width = width;

    while(*str) {
        if(x_pos + char_width > OLED_WIDTH) {
            x_pos = 0;
            y += height;
            if(y + height > OLED_HEIGHT) break;
        }
        OLED_ShowChar(x_pos, y, *str, width, height);
        x_pos += char_width;
        str++;
    }
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t width, uint8_t height)
{
    char str[12];
    for(uint8_t i = 0; i < len; i++) {
        str[len - 1 - i] = '0' + (num % 10);
        num /= 10;
    }
    str[len] = '\0';
    OLED_ShowString(x, y, str, width, height);
}


void IIC_test_func()
{
    I2C_Start();
    I2C_WriteByte(0x78);  // 试另一个地址
    I2C_WriteByte(0x00);
    I2C_WriteByte(0xAE);  // 关闭显示命令（最安全的测试命令）
    I2C_Stop();
}
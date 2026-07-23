// battery.c
#include "battery.h"
#include <math.h>

// 存储ADC校准值（全局变量，由ADC_DataCalib_Rough填充）
__attribute__((aligned(4))) int32_t RoughCalib_Value = 0;

void Battery_Init(void)
{
    // 1. 配置PA0为模拟浮空输入模式
    GPIOA_ModeCfg(BAT_ADC_PIN, GPIO_ModeIN_Floating);

    // 2. 初始化ADC：采样频率3.2MHz，PGA增益0dB（1倍）
    //    [reference:0][reference:1]
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);

    // 3. 获取ADC内部校准值（必须先初始化ADC再调用）
    //    [reference:2][reference:3]
    RoughCalib_Value = ADC_DataCalib_Rough();

    ADC_AutoConverCycle(192); 

    // 4. 配置通道为AIN9（PA0）
    ADC_ChannelCfg(BAT_ADC_CH);
}

uint16_t Battery_ReadRaw(void)
{
    uint16_t adc_value = 0;

    // 读取原始ADC值
    adc_value = ADC_ExcutSingleConver() + RoughCalib_Value;

    return adc_value;
}

// 多次采样取平均值的示例
uint16_t Battery_ReadRaw_Average(uint8_t times)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < times; i++) {
        sum += Battery_ReadRaw();
    }
    return (uint16_t)(sum / times);
}

float Battery_GetVoltage(void)
{
    uint16_t raw = Battery_ReadRaw();
    float v_ref = 1.05f;           // CH582M内部参考电压 1.05V[reference:4]
    float adc_max = 4095.0f;       // 12位ADC最大值

    // 1. 计算ADC引脚上的实际电压
    //    公式：Vpin = (ADC值 / 4095) * Vref[reference:5][reference:6]
    //    注意：这里假设PGA增益为0dB，若使用其他增益需调整公式
    float v_pin = ((float)raw / adc_max) * v_ref;

    // 2. 根据分压电阻反推电池电压
    //    Vbat = Vpin * (R1 + R2) / R2
     float v_bat = v_pin * (R1_DOWN + R2_UP) / R2_UP;

    return v_bat;
}
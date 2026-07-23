// battery.h
#ifndef __BATTERY_H__
#define __BATTERY_H__

#include "CH58x_common.h"

// 分压电阻值（单位：Ω）
#define R1_DOWN     100000.0f   // 下拉电阻（接地）
#define R2_UP       47000.0f    // 上拉电阻（接VBAT）

// ADC通道与引脚配置
#define BAT_ADC_PIN     GPIO_Pin_0
#define BAT_ADC_PORT    GPIOA
#define BAT_ADC_CH      9     // PA0 对应 AIN9

void Battery_Init(void);
uint16_t Battery_ReadRaw(void);
uint16_t Battery_ReadRaw_Average(uint8_t times);
float Battery_GetVoltage(void);

#endif
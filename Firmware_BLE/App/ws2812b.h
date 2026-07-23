#ifndef __WS2812_H__
#define __WS2812_H__

#include "CH58x_common.h"

#define WS2812B_PIN     GPIO_Pin_5
#define WS2812B_PORT    GPIOB

void WS2812B_Init(void);
void WS2812B_SendArray(uint8_t *data, uint16_t len);

#endif
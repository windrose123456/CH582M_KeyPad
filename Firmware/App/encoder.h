#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "CH58x_common.h"

void Encoder_Init();

uint8_t Encoder_Read(); 

uint8_t Encoder_GetDelta();

#endif
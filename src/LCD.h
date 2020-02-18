#ifndef FLYSIGHT_LCD_H
#define FLYSIGHT_LCD_H

#include "UBX.h"

void LCD_Init(void);
void LCD_Task(void);
void LCD_DisplayAltitude(UBX_saved_t *current);
void LCD_DisplayCharge(uint8_t charge_state);

#endif
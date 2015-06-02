#ifndef FLYSIGHT_LCD_H
#define FLYSIGHT_LCD_H

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t y, uint8_t x);
void LCD_Show(const char *text);

#endif
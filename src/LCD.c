#include <avr/io.h>
#include <util/delay.h>

#include "i2cmaster.h"

static void LCD_SendPacket(
	uint8_t *data,
	uint8_t size)
{
	i2c_start(0x78);	// set device address and write mode

	while (size--)
	{
		i2c_write(*(data++));
	}

	i2c_stop();
}

static void LCD_Command(
	uint8_t c) 
{
	uint8_t packet[2];
	
	packet[0] = 0x00;			// Control Byte; C0_bit=0, D/C_bit=0 -> following Data Byte contains command
	packet[1] = c;				// Data Byte: the command to be executed by the display
	LCD_SendPacket(packet, 2);	// transmits the two bytes
}

static void LCD_Data(
	uint8_t d) 
{
	uint8_t packet[2];
	
	packet[0] = 0x40;			// Control Byte; C0_bit=0, D/C_bit=1 -> following Data Byte contains data
	packet[1] = d;				// Data Byte: the character to be displayed
	LCD_SendPacket(packet, 2);	// transmits the two bytes
}

static void LCD_WriteString(
	char *str)
{
	while (*str)
	{
		LCD_Data(*(str++));
	}
}

static void LCD_Test(void)
{
	LCD_Command(0x84);	// return home
	LCD_WriteString("     10750 m    ");
	LCD_Command(0xC4);	// line 2
	LCD_WriteString(" ------*------- ");
}

void LCD_Init(void)
{
    unsigned char ret;

	PORTD |= 0x03;		// internal pull-ups for SCL and SDA

	DDRD |= (1 << 4);	// reset
	_delay_ms(100);		// delay

	i2c_init();

	PORTD |= (1 << 4);	// reset HIGH – inactive
	_delay_ms(1);		// delay
	LCD_Command(0x2A);	// function set (extended LCD_Command set)
	LCD_Command(0x71);	// function selection A
	LCD_Data(0x00);		// disable internal VDD regulator (2.8V I/O). LCD_Data(0x5C) = enable regulator (5V I/O)
	LCD_Command(0x28);	// function set (fundamental LCD_Command set)
	LCD_Command(0x08);	// display off, cursor off, blink off
	LCD_Command(0x2A);	// function set (extended LCD_Command set)
	LCD_Command(0x79);	// OLED LCD_Command set enabled
	LCD_Command(0xD5);	// set display clock divide ratio/oscillator frequency
	LCD_Command(0x70);	// set display clock divide ratio/oscillator frequency
	LCD_Command(0x78);	// OLED LCD_Command set disabled
	LCD_Command(0x08);	// extended function set (2-lines)
	LCD_Command(0x05);	// COM SEG direction
	LCD_Command(0x72);	// function selection B
	LCD_Data(0x00);		// ROM CGRAM selection
	LCD_Command(0x2A);	// function set (extended LCD_Command set)
	LCD_Command(0x79);	// OLED LCD_Command set enabled
	LCD_Command(0xDA);	// set SEG pins hardware configuration
	LCD_Command(0x10);	// set SEG pins hardware configuration
	LCD_Command(0xDC);	// function selection C
	LCD_Command(0x00);	// function selection C
	LCD_Command(0x81);	// set contrast control
	LCD_Command(0x7F);	// set contrast control
	LCD_Command(0xD9);	// set phase length
	LCD_Command(0xF1);	// set phase length
	LCD_Command(0xDB);	// set VCOMH deselect level
	LCD_Command(0x40);	// set VCOMH deselect level
	LCD_Command(0x78);	// OLED LCD_Command set disabled
	LCD_Command(0x28);	// function set (fundamental LCD_Command set)
	LCD_Command(0x01);	// clear display
	LCD_Command(0x80);	// set DDRAM address to 0x00
	LCD_Command(0x0C);	// display ON
	_delay_ms(100);		// delay
	
	LCD_Test();
}

void LCD_Task(void)
{
	
}

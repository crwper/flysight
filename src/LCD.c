#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#include "UBX.h"

#define LCD_CLK_PORT  PORTD
#define LCD_CLK_PIN   0
#define LCD_DATA_PORT PORTD
#define LCD_DATA_PIN  1

#define LCD_CLK_LO    LCD_CLK_PORT  &= ~(1<<LCD_CLK_PIN)
#define LCD_CLK_HI    LCD_CLK_PORT  |=  (1<<LCD_CLK_PIN)
#define LCD_DATA_LO   LCD_DATA_PORT &= ~(1<<LCD_DATA_PIN)
#define LCD_DATA_HI   LCD_DATA_PORT |=  (1<<LCD_DATA_PIN)

static void LCD_SendByte(
	uint8_t data)
{
	uint8_t i;

	for (i = 0; i < 8; ++i)
	{
		if (data & 0x01) LCD_DATA_HI;
		else             LCD_DATA_LO;

		LCD_CLK_LO;
		LCD_CLK_HI;

		data >>= 1;
	}
}

static void LCD_SendPacket(
	uint8_t *data,
	uint8_t size)
{
	while (size--)
	{
		LCD_SendByte(*(data++));
	}
}

static void LCD_Command(
	uint8_t c) 
{
	uint8_t packet[3];

	packet[0] = 0x1F;			// Control Byte; C0_bit=0, D/C_bit=0 -> following Data Byte contains command
	packet[1] = (c & 0x0F);		// Data Byte: the command to be executed by the display
	packet[2] = (c & 0xF0) >> 4;
	LCD_SendPacket(packet, 3);	// transmits the three bytes
}

static void LCD_Data(
	uint8_t d) 
{
	uint8_t packet[3];

	packet[0] = 0x5F;			// Control Byte; C0_bit=0, D/C_bit=1 -> following Data Byte contains data
	packet[1] = (d & 0x0F);		// Data Byte: the character to be displayed
	packet[2] = (d & 0xF0) >> 4;
	LCD_SendPacket(packet, 3);	// transmits the three bytes
}

static void LCD_WriteString(
	char *str)
{
	while (*str)
	{
		LCD_Data(*(str++));
	}
}

void LCD_Init(void)
{
	LCD_CLK_HI;
	LCD_DATA_HI;

	DDRD |= (1<<LCD_CLK_PIN);
	DDRD |= (1<<LCD_DATA_PIN);

	DDRD |= (1 << 4);	// reset
	_delay_ms(100);		// delay

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
}

void LCD_Task(void)
{
	
}

void LCD_Update(
	UBX_saved_t *current)
{
	char temp[17];
	char line1[17];
	char line2[17];
	char *ptr;

	uint8_t len, i;
	uint8_t top, bot;

	int32_t alt = current->hMSL - UBX_dz_elev;
	int32_t top_alt = alt + 2 * (int32_t) current->vAcc;
	int32_t bot_alt = alt - 2 * (int32_t) current->vAcc;
	
	uint8_t flag_low = 0;
	uint8_t flag_high = 0;

	if (current->gpsFix == 0x03)
	{
		len = sprintf(temp, "%ld ft", (alt * 10) / 3048);

		ptr = line1;
		for (i = 0; i < (16 - len) / 2; ++i)
		{
			*(ptr++) = ' ';
		}

		strcpy(ptr, temp);
		ptr += strlen(temp);

		for (i = 0; i < (16 - len + 1) / 2; ++i)
		{
			*(ptr++) = ' ';
		}

		*(ptr++) = 0;

		if (bot_alt < UBX_exit_bot)
		{
			bot = 0;
			flag_low = 1;
		}
		else if (bot_alt < UBX_exit_top)
		{
			bot = (14 * (bot_alt - UBX_exit_bot)) / (UBX_exit_top - UBX_exit_bot);
		}
		else
		{
			bot = 14;
		}
		
		if (top_alt >= UBX_exit_top)
		{
			top = 14;
			flag_high = 1;
		}
		else if (top_alt >= UBX_exit_bot)
		{
			top = (14 * (top_alt - UBX_exit_bot)) / (UBX_exit_top - UBX_exit_bot) + 1;
		}
		else
		{
			top = 0;
		}

		ptr = line2;
		
		if (flag_low) *(ptr++) = '<';
		else          *(ptr++) = ' ';
		
		for (i = 0; i < bot; ++i)
		{
			*(ptr++) = '-';
		}
		
		for (i = bot; i < top; ++i)
		{
			*(ptr++) = '*';
		}
		
		for (i = top; i < 14; ++i)
		{
			*(ptr++) = '-';
		}
		
		if (flag_high) *(ptr++) = '>';
		else           *(ptr++) = ' ';
	}
	else
	{
		strcpy(line1, "     No Fix     ");
		strcpy(line2, "                ");
	}

	LCD_Command(0x84);	// line 1
	LCD_WriteString(line1);
	LCD_Command(0xC4);	// line 2
	LCD_WriteString(line2);
/*
	LCD_Command(0x84);	// line 1
	LCD_WriteString("     10750 m    ");
	LCD_Command(0xC4);	// line 2
	LCD_WriteString(" ------*------- ");
*/
}

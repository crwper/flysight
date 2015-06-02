#include <avr/io.h>
#include <util/delay.h>

#include "i2cmaster.h"

// LCD Command set
static const uint8_t DISP_CMD       = 0x0;  // Command for the display
static const uint8_t RAM_WRITE_CMD  = 0x40; // Write to display RAM
static const uint8_t CLEAR_DISP_CMD = 0x01; // Clear display command
static const uint8_t HOME_CMD       = 0x02; // Set cursos at home (0,0)
static const uint8_t DISP_ON_CMD    = 0x0C; // Display on command
static const uint8_t DISP_OFF_CMD   = 0x08; // Display off Command
static const uint8_t SET_DDRAM_CMD  = 0x80; // Set DDRAM address command
static const uint8_t CONTRAST_CMD   = 0x70; // Set contrast LCD command
static const uint8_t FUNC_SET_TBL0  = 0x38; // Function set - 8 bit, 2 line display 5x8, inst table 0
static const uint8_t FUNC_SET_TBL1  = 0x39; // Function set - 8 bit, 2 line display 5x8, inst table 1

// Driver DDRAM addressing
static const uint8_t LCD_disp_addr [3] =
{
   { 0x00, 0x40, 0x00 }  // Two line display address
};

void LCD_Init(void)
{
    unsigned char ret;
    
	i2c_init();

    ret = i2c_start(0x7C);	// set device address and write mode
    if (ret)
	{
        // Failed to issue start condition, possibly no device found
        i2c_stop();
    }
	else
	{
		i2c_write(0x00);	// Send command to display
		i2c_write(0x38);	// Function set - 8 bit, 2 line display 5x8, inst table 0
		_delay_ms(10);
		i2c_write(0x39);	// Function set - 8 bit, 2 line display 5x8, inst table 1
		_delay_ms(10);
		i2c_write(0x14);	// Set BIAS - 1/5
//		i2c_write(0x73);	// Set contrast
		i2c_write(0x5E);	// ICON disp on, Booster on, Contrast high byte 
		i2c_write(0x6D);	// Follower circuit (internal), amp ratio (6)
		i2c_write(0x0C);	// Display on
		i2c_write(0x01);	// Clear display
		i2c_write(0x06);	// Entry mode set - increment
		_delay_ms(10);
		i2c_stop();
    }	
}

static void LCD_Command(
	uint8_t value) 
{
    unsigned char ret;
	
    ret = i2c_start(0x7C);	// set device address and write mode
    if (ret)
	{
        // Failed to issue start condition, possibly no device found
        i2c_stop();
	}
	else
	{
		i2c_write(DISP_CMD);
		i2c_write(value);
		i2c_stop();
		_delay_ms(1);
	}
}

void LCD_Clear(void)
{
	LCD_Command(CLEAR_DISP_CMD);
}

void LCD_SetCursor(
	uint8_t y, 
	uint8_t x)
{
	uint8_t base = 0x00;
   
	// set the baseline address
	base = LCD_disp_addr[y];
	
	LCD_Command(SET_DDRAM_CMD + base + x);
}

void LCD_Show(
	const char *text)
{
    unsigned char ret;
	
    ret = i2c_start(0x7C);	// set device address and write mode
    if (ret)
	{
        // Failed to issue start condition, possibly no device found
        i2c_stop();
	}
	else
	{
		i2c_write(0x40);
		while (*text)
		{
			i2c_write(*text);
			++text;
		}
		i2c_stop();
	}
}

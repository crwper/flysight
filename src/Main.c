#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "Board/LEDs.h"
#include "Lib/MMC.h"
#include "Config.h"
#include "i2cmaster.h"
#include "Log.h"
#include "Main.h"
#include "Power.h"
#include "Signature.h"
#include "Timer.h"
#include "Tone.h"
#include "uart.h"
#include "UBX.h"
#include "UsbInterface.h"

#define CHARGE_STATUS_DDR  DDRC
#define CHARGE_STATUS_PORT PORTC
#define CHARGE_STATUS_PIN  PINC
#define CHARGE_STATUS_MASK (1 << 3)

#if defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__)
	#define BOOTLOADER_START_ADDR (0xF000)
#elif defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB647__)
	#define BOOTLOADER_START_ADDR (0x7800)
#endif
#define BOOTLOADER_COUNT_ADDR ((uint8_t *) 0x01)

uint8_t Main_activeLED;

static FATFS    Main_fs;
       FIL      Main_file;
       uint8_t  Main_buffer[MAIN_BUFFER_SIZE];

static uint8_t Main_mmcInitialized;

static void delay_ms(
	uint16_t ms)
{
	while (ms)
	{
		_delay_ms(1);
		--ms;
	}
}

static void LCD_Init(void)
{
    unsigned char ret;
    
	i2c_init();

    ret = i2c_start(0x7C);	// set device address and write mode
    if (ret)
	{
        // Failed to issue start condition, possibly no device found
        i2c_stop();
		
		LEDs_TurnOnLEDs(LEDS_RED | LEDS_GREEN);
		while (1);
    }
	else
	{
		i2c_write(0x00);	// Send command to display
		i2c_write(0x38);	// Function set - 8 bit, 2 line display 5x8, inst table 0
		delay_ms(10);
		i2c_write(0x39);	// Function set - 8 bit, 2 line display 5x8, inst table 1
		delay_ms(10);
		i2c_write(0x14);	// Set BIAS - 1/5
//		i2c_write(0x73);	// Set contrast
		i2c_write(0x5E);	// ICON disp on, Booster on, Contrast high byte 
		i2c_write(0x6D);	// Follower circuit (internal), amp ratio (6)
		i2c_write(0x0C);	// Display on
		i2c_write(0x01);	// Clear display
		i2c_write(0x06);	// Entry mode set - increment
		delay_ms(10);
		i2c_stop();
    }	
}

static void LCD_Show(
	const char *text)
{
    unsigned char ret;
	
    ret = i2c_start(0x7C);	// set device address and write mode
    if (ret)
	{
        // Failed to issue start condition, possibly no device found
        i2c_stop();
		
		LEDs_TurnOnLEDs(LEDS_RED | LEDS_GREEN);
		while (1);
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

void SetupHardware(void)
{
#ifdef MAIN_DEBUG
	uint8_t i;
#endif

	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	CLKPR = (1 << CLKPCE);
	CLKPR = 0;

#ifdef MAIN_DEBUG
	MCUCR |= (1 << JTD); 
	MCUCR |= (1 << JTD); 
   
	DDRF  = 0xff;
	
	for (i = 0; i < 7; ++i)
	{
		PORTF = (1 << i);
		delay_ms(1);
	}
	
	PORTF = 0x00;
#endif

	USB_Init();
	LEDs_Init();
	
	f_mount(0, &Main_fs);
	Main_mmcInitialized = MMC_Init();
	
	Tone_Init();

	LCD_Init();
	LCD_Show("Hello world!");
}

int main(void)
{
	typedef void (*AppPtr_t) (void);
	AppPtr_t Bootloader = (AppPtr_t) BOOTLOADER_START_ADDR; 

	const uint8_t count = eeprom_read_byte(BOOTLOADER_COUNT_ADDR);

	DDRB |= (1 << 6) | (1 << 5);	// pull audio pins down
	
	if (count == 3)
	{
		eeprom_write_byte(BOOTLOADER_COUNT_ADDR, 0);
		Bootloader();
	}

	SetupHardware();

	eeprom_write_byte(BOOTLOADER_COUNT_ADDR, count + 1);
	delay_ms(500);
	eeprom_write_byte(BOOTLOADER_COUNT_ADDR, 0);

	if (USB_VBUS_GetStatus())
	{
		if (!Main_mmcInitialized)
		{
			USB_Disable();
		}
		
		uart_init(12);
		
		for (;;)
		{
			CHARGE_STATUS_PORT |= CHARGE_STATUS_MASK ;
			
			if (Main_mmcInitialized)
			{
				USBInterfaceTask();
				USB_USBTask();
			}
			
			if (CHARGE_STATUS_PIN & CHARGE_STATUS_MASK)
			{
				Main_activeLED = LEDS_GREEN;
			}
			else
			{
				Main_activeLED = LEDS_RED;
			}

			LEDs_ChangeLEDs(LEDS_ALL_LEDS, Main_activeLED);
		}
	}
	else
	{
		USB_Disable();
		
		if (Main_mmcInitialized)
		{
			Main_activeLED = LEDS_GREEN;
		}
		else
		{
			Main_activeLED = LEDS_RED;
		}
		LEDs_ChangeLEDs(LEDS_ALL_LEDS, Main_activeLED);

		Power_Hold();
		Signature_Write();
		Config_Read();
		Power_Release();
		
		Timer_Init();
		UBX_Init();

		for (;;)
		{
			UBX_Task();
			Tone_Task();
		}
	}
}

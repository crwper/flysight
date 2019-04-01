/***************************************************************************
**                                                                        **
**  FlySight firmware                                                     **
**  Copyright 2018 Michael Cooper, Will Glynn                             **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>. **
**                                                                        **
****************************************************************************
**  Contact: Michael Cooper                                               **
**  Website: http://flysight.ca/                                          **
****************************************************************************/

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "Board/LEDs.h"
#include "Lib/MMC.h"
#include "Config.h"
#include "Key.h"
#include "Log.h"
#include "Main.h"
#include "Power.h"
#include "Signature.h"
#include "Timer.h"
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

static uint8_t EEPROM saved_count;

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

void SetupHardware(void)
{
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	CLKPR = (1 << CLKPCE);
	CLKPR = 0;

	USB_Init();
	LEDs_Init();
	
	f_mount(0, &Main_fs);
	Main_mmcInitialized = MMC_Init();
}

int main(void)
{
	typedef void (*AppPtr_t) (void);
	AppPtr_t Bootloader = (AppPtr_t) BOOTLOADER_START_ADDR; 

	const uint8_t count = eeprom_read_byte(&saved_count);

	DDRB |= (1 << 6) | (1 << 5);	// pull audio pins down
	
	if (count == 3)
	{
		eeprom_write_byte(&saved_count, 0);
		Bootloader();
	}

	SetupHardware();

	eeprom_write_byte(&saved_count, count + 1);
	delay_ms(500);
	eeprom_write_byte(&saved_count, 0);

	Power_Hold();
	Signature_Write();
	Config_Read();
	Key_Read();
	Power_Release();

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

		Timer_Init();
		UBX_Init();

		for (;;)
		{
			UBX_Task();
		}
	}
}

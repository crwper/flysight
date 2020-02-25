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
#include <avr/pgmspace.h>

#include <stdlib.h>
#include <string.h>

#include "Board/LEDs.h"
#include "FatFS/ff.h"
#include "Config.h"
#include "Debug.h"
#include "Log.h"
#include "Main.h"
#include "UBX.h"
#include "Version.h"

#define FALSE 0
#define TRUE  (!FALSE)

static const char Config_default[] PROGMEM = "\
; Firmware version " FLYSIGHT_VERSION "\r\n\
\r\n\
; For information on configuring FlySight, please go to\r\n\
;     http://flysight.ca/wiki\r\n\
\r\n\
; GPS settings\r\n\
\r\n\
Model:     7     ; Dynamic model\r\n\
                 ;   0 = Portable\r\n\
                 ;   2 = Stationary\r\n\
                 ;   3 = Pedestrian\r\n\
                 ;   4 = Automotive\r\n\
                 ;   5 = Sea\r\n\
                 ;   6 = Airborne with < 1 G acceleration\r\n\
                 ;   7 = Airborne with < 2 G acceleration\r\n\
                 ;   8 = Airborne with < 4 G acceleration\r\n\
Rate:      200   ; Measurement rate (ms)\r\n\
\r\n\
; Competition settings\r\n\
\r\n\
DZ_Elev:       0 ; Ground elevation (m above sea level)\r\n\
Exit_Top:   3353 ; Top of exit window (m above ground level)\r\n\
Exit_Bot:   3200 ; Bottom of exit window (m above ground level)\r\n\
Rotated:       0 ; Rotated display\r\n\
                 ;   0 = Normal\r\n\
                 ;   1 = Rotated display\r\n";

static const char Config_Model[] PROGMEM      = "Model";
static const char Config_Rate[] PROGMEM       = "Rate";
static const char Config_DZ_Elev[] PROGMEM    = "DZ_Elev";
static const char Config_Exit_Top[] PROGMEM   = "Exit_Top";
static const char Config_Exit_Bot[] PROGMEM   = "Exit_Bot";
static const char Config_Rotated[] PROGMEM    = "Rotated";

char Config_buf[80];

static void Config_WriteString_P(
	const char *str,
	FIL        *file)
{
	char ch;

	while ((ch = pgm_read_byte(str++)))
	{
		f_putc(ch, file);
	}
}

static FRESULT Config_ReadSingle(
	const char *dir,
	const char *filename)
{
	size_t  len;
	char    *name;
	char    *result;
	int32_t val;

	FRESULT res;

	res = f_chdir(dir);
	if (res != FR_OK) return res;
	
	res = f_open(&Main_file, filename, FA_READ);
	if (res != FR_OK) return res;

	while (!f_eof(&Main_file))
	{
		f_gets(Config_buf, sizeof(Config_buf), &Main_file);

		len = strcspn(Config_buf, ";");
		Config_buf[len] = 0;
		
		name = strtok(Config_buf, " \r\n\t:");
		if (name == 0) continue ;
		
		result = strtok(0, " \r\n\t:");
		if (result == 0) continue ;
		
		val = atol(result);
		
		#define HANDLE_VALUE(s,w,r,t) \
			if ((t) && !strcmp_P(name, (s))) { (w) = (r); }

		HANDLE_VALUE(Config_Model,     UBX_model,        val, val >= 0 && val <= 8);
		HANDLE_VALUE(Config_Rate,      UBX_rate,         val, val >= 100);
		HANDLE_VALUE(Config_DZ_Elev,   UBX_dz_elev,      val * 1000, TRUE);
		HANDLE_VALUE(Config_Exit_Top,  UBX_exit_top,     val * 1000, TRUE);
		HANDLE_VALUE(Config_Exit_Bot,  UBX_exit_bot,     val * 1000, TRUE);
		HANDLE_VALUE(Config_Rotated,   UBX_rotated,      val, TRUE);
		
		#undef HANDLE_VALUE
	}
	
	f_close(&Main_file);
	
	return FR_OK;
}

void Config_Read(void)
{
	FRESULT res;

	res = Config_ReadSingle("\\", "config.txt");
	
	if (res != FR_OK)
	{
		res = f_chdir("\\");
		res = f_open(&Main_file, "config.txt", FA_WRITE | FA_CREATE_ALWAYS);
		if (res != FR_OK) 
		{
			Main_activeLED = LEDS_RED;
			LEDs_ChangeLEDs(LEDS_ALL_LEDS, Main_activeLED);
			return ;
		}

		Config_WriteString_P(Config_default, &Main_file);
		f_close(&Main_file);
	}

	eeprom_read_block(UBX_buf, CONFIG_FNAME_ADDR, CONFIG_FNAME_LEN);

	if (UBX_buf[0] != 0 && UBX_buf[0] != 0xff)
	{
		res = Config_ReadSingle("\\config", UBX_buf);
	}
}

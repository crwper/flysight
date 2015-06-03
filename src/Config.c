#include <avr/pgmspace.h>

#include <stdlib.h>
#include <string.h>

#include "Board/LEDs.h"
#include "FatFS/ff.h"
#include "Log.h"
#include "Main.h"
#include "Tone.h"
#include "UBX.h"
#include "Version.h"

#define FALSE 0
#define TRUE  (!FALSE)

static const char Config_default[] PROGMEM = "\
; For information on configuring FlySight, please go to\r\n\
;     http://flysight.ca/wiki\r\n\
\r\n\
; Tone settings\r\n\
\r\n\
Volume:    6     ; 0 (min) to 8 (max)\r\n\
\r\n\
; Speech settings\r\n\
\r\n\
Sp_Volume: 8     ; 0 (min) to 8 (max)\r\n\
\r\n\
; Miscellaneous\r\n\
\r\n\
TZ_Offset: 0     ; Timezone offset of output files in seconds\r\n\
                 ;   -14400 = UTC-4 (EDT)\r\n\
                 ;   -18000 = UTC-5 (EST, CDT)\r\n\
                 ;   -21600 = UTC-6 (CST, MDT)\r\n\
                 ;   -25200 = UTC-7 (MST, PDT)\r\n\
                 ;   -28800 = UTC-8 (PST)";

static const char Config_Volume[] PROGMEM     = "Volume";
static const char Config_Sp_Volume[] PROGMEM  = "Sp_Volume";
static const char Config_TZ_Offset[] PROGMEM  = "TZ_Offset";

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

void Config_Read(void)
{
	char    buf[80];
	size_t  len;
	char    *name;
	char    *result;
	
	int32_t val;
	int32_t dz_elev = 0;

	FRESULT res;
	
	res = f_open(&Main_file, "config.txt", FA_READ);
	if (res != FR_OK)
	{
		res = f_open(&Main_file, "config.txt", FA_WRITE | FA_CREATE_ALWAYS);
		if (res != FR_OK) 
		{
			Main_activeLED = LEDS_RED;
			LEDs_ChangeLEDs(LEDS_ALL_LEDS, Main_activeLED);
			return ;
		}

		Config_WriteString_P(Config_default, &Main_file);
		f_close(&Main_file);

		res = f_open(&Main_file, "config.txt", FA_READ);
		if (res != FR_OK)
		{
			Main_activeLED = LEDS_RED;
			LEDs_ChangeLEDs(LEDS_ALL_LEDS, Main_activeLED);
			return ;
		}
	}
	
	while (!f_eof(&Main_file))
	{
		f_gets(buf, sizeof(buf), &Main_file);

		len = strcspn(buf, ";");
		buf[len] = 0;
		
		name = strtok(buf, " \t:");
		if (name == 0) continue ;
		
		result = strtok(0, " \t:");
		if (result == 0) continue ;
		
		val = atol(result);
		
		#define HANDLE_VALUE(s,w,r,t) \
			if ((t) && !strcmp_P(name, (s))) { (w) = (r); }

		HANDLE_VALUE(Config_Volume,    Tone_volume,      8 - val, val >= 0 && val <= 8);
		HANDLE_VALUE(Config_Sp_Volume, Tone_sp_volume,   8 - val, val >= 0 && val <= 8);
		HANDLE_VALUE(Config_TZ_Offset, Log_tz_offset,    val, TRUE);
		
		#undef HANDLE_VALUE
	}
	
	f_close(&Main_file);
}

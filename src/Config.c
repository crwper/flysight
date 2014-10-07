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
; Firmware version " FLYSIGHT_VERSION "\r\n\
\r\n\
; GPS settings\r\n\
\r\n\
Model:     6     ; Dynamic model\r\n\
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
; Tone settings\r\n\
\r\n\
Mode:      2     ; Measurement mode\r\n\
                 ;   0 = Horizontal speed\r\n\
                 ;   1 = Vertical speed\r\n\
                 ;   2 = Glide ratio\r\n\
                 ;   3 = Inverse glide ratio\r\n\
                 ;   4 = Total speed\r\n\
Min:       0     ; Lowest pitch value\r\n\
                 ;   cm/s        in Mode 0, 1, or 4\r\n\
                 ;   ratio * 100 in Mode 2 or 3\r\n\
Max:       300   ; Highest pitch value\r\n\
                 ;   cm/s        in Mode 0, 1, or 4\r\n\
                 ;   ratio * 100 in Mode 2 or 3\r\n\
Reference: 150   ; Reference pitch value\r\n\
                 ;   cm/s        in Mode 0, 1, or 4\r\n\
                 ;   ratio * 100 in Mode 2 or 3\r\n\
Lock:      1     ; Tone lock speed (0-256)\r\n\
Volume:    6     ; 0 (min) to 8 (max)\r\n\
\r\n\
; Miscellaneous\r\n\
\r\n\
TZ_Offset: 0     ; Timezone offset of output files in seconds\r\n\
                 ;   -14400 = UTC-4 (EDT)\r\n\
                 ;   -18000 = UTC-5 (EST, CDT)\r\n\
                 ;   -21600 = UTC-6 (CST, MDT)\r\n\
                 ;   -25200 = UTC-7 (MST, PDT)\r\n\
                 ;   -28800 = UTC-8 (PST)\r\n";

static const char Config_Model[] PROGMEM      = "Model";
static const char Config_Rate[] PROGMEM       = "Rate";
static const char Config_Mode[] PROGMEM       = "Mode";
static const char Config_Min[] PROGMEM        = "Min";
static const char Config_Max[] PROGMEM        = "Max";
static const char Config_Reference[] PROGMEM  = "Reference";
static const char Config_Limits[] PROGMEM     = "Limits";
static const char Config_Lock[] PROGMEM       = "Lock";
static const char Config_Volume[] PROGMEM     = "Volume";
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
	int32_t dz_elev;

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

		HANDLE_VALUE(Config_Model,     UBX_model,        val, val >= 0 && val <= 8);
		HANDLE_VALUE(Config_Rate,      UBX_rate,         val, val >= 100);
		HANDLE_VALUE(Config_Mode,      UBX_mode,         val, val >= 0 && val <= 4);
		HANDLE_VALUE(Config_Min,       UBX_min,          val, TRUE);
		HANDLE_VALUE(Config_Max,       UBX_max,          val, TRUE);
		HANDLE_VALUE(Config_Reference, UBX_reference,    val, TRUE);
		HANDLE_VALUE(Config_Limits,    UBX_limits,       val, val >= 0 && val <= 1);
		HANDLE_VALUE(Config_Volume,    Tone_volume,      8 - val, val >= 0 && val <= 8);
		HANDLE_VALUE(Config_Lock,      Tone_lock,        val, val >= 0 && val <= 256);
		HANDLE_VALUE(Config_TZ_Offset, Log_tz_offset,    val, TRUE);
		
		#undef HANDLE_VALUE
	}
	
	f_close(&Main_file);
}

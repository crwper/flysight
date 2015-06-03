#include <avr/io.h>

#include "LCD.h"
#include "Time.h"
#include "UBX.h"

static enum
{
	menu_idle,
	menu_active
}
Menu_state = menu_idle;

void Menu_Task(void)
{
	static uint32_t prevTime = 0;
	uint32_t curTime;

	char buf[32];

	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  hour;
	uint8_t  min;
	uint8_t  sec;
	
	if (Menu_state == menu_idle)
	{
		if ((PINE & (1 << 4)) == 0)
		{
			Menu_state = menu_active;
		}
	}
	else if (Menu_state == menu_active)
	{
		if ((PINE & (1 << 5)) == 0)
		{
			Menu_state = menu_idle;
		}
	}
	
	if (Menu_state == menu_idle)
	{
		curTime = UBX_curTime + (UBX_curMillisecond / 1000);
	
		if (UBX_curMillisecond < UBX_MAX_MILLISECOND)
		{
			if (curTime != prevTime)
			{
				gmtime_r(curTime, &year, &month, &day, &hour, &min, &sec);
				sprintf(buf, "    %02d:%02d:%02d", hour, min, sec);
				prevTime = curTime;

				LCD_Clear();
				LCD_Show(buf);
			}
		}
		else
		{
			LCD_Clear();
			LCD_Show("Invalid time");
		}
	}
	else
	{
		sprintf(buf, "Active");
		
		LCD_Clear();
		LCD_Show(buf);
	}
}

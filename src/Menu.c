#include <avr/io.h>

#include "LCD.h"
#include "Time.h"
#include "UBX.h"

typedef enum
{
	menu_init,
	menu_idle_no_fix,
	menu_idle_with_fix,
	menu_active
}
Menu_state_t;

static Menu_state_t Menu_state = menu_init;

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
	
	Menu_state_t old_state = Menu_state;
	uint8_t need_redraw = 0;
	
	// Update state
	if (Menu_state == menu_init)
	{
		Menu_state = menu_idle_no_fix;
		need_redraw = 1;
	}
	else if (Menu_state == menu_idle_no_fix)
	{
		if (UBX_curMillisecond < UBX_MAX_MILLISECOND)
		{
			Menu_state = menu_idle_with_fix;
			need_redraw = 1;
		}
	}
	else if (Menu_state == menu_idle_with_fix)
	{
		if (UBX_curMillisecond >= UBX_MAX_MILLISECOND)
		{
			Menu_state = menu_idle_no_fix;
			need_redraw = 1;
		}
		else if ((PINE & (1 << 4)) == 0)
		{
			Menu_state = menu_active;
			need_redraw = 1;
		}
	}
	else if (Menu_state == menu_active)
	{
		if ((PINE & (1 << 5)) == 0)
		{
			Menu_state = menu_idle_no_fix;
			need_redraw = 1;
		}
	}

	// Check if we need to redraw
	if (Menu_state == menu_idle_with_fix)
	{
		curTime = UBX_curTime + (UBX_curMillisecond / 1000);
		if (curTime != prevTime)
		{
			need_redraw = 1;
		}
	}

	// Redraw if needed
	if (need_redraw)
	{
		if (Menu_state == menu_idle_no_fix)
		{
			LCD_Clear();
			LCD_Show("Waiting for fix");
		}
		else if (Menu_state == menu_idle_with_fix)
		{
			gmtime_r(curTime, &year, &month, &day, &hour, &min, &sec);
			sprintf(buf, "    %02d:%02d:%02d", hour, min, sec);
			prevTime = curTime;

			LCD_Clear();
			LCD_Show(buf);
		}
		else if (Menu_state == menu_active)
		{
			LCD_Clear();
			LCD_Show("Counting down");
		}
	}
}

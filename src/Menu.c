#include <avr/io.h>

#include "LCD.h"
#include "Time.h"
#include "Tone.h"
#include "UBX.h"

#define MENU_FLAGS_PLAY_NEXT 1

typedef enum
{
	menu_init,
	menu_idle_no_fix,
	menu_idle_with_fix,
	menu_active,
	menu_hold
}
Menu_state_t;

static volatile Menu_state_t Menu_state = menu_init;
static volatile uint8_t      Menu_flags = 0;

static volatile uint8_t      Menu_countdown;
static volatile uint16_t     Menu_countdown_timer;

static volatile uint32_t     Menu_start_time;
static volatile uint16_t     Menu_start_time_ms;
static volatile uint8_t      Menu_start_time_valid = 0;

void Menu_Update(void)
{
	if (Menu_state == menu_active)
	{
		if (--Menu_countdown_timer == 0)
		{
			Menu_countdown_timer = 1000;
			--Menu_countdown;
			Menu_flags |= MENU_FLAGS_PLAY_NEXT;
		}
	}
}

void Menu_Task(void)
{
	static uint32_t prevTime = 0;
	uint32_t curTime;
	
	static uint8_t red_button_prev = 0;
	uint8_t red_button;

	static uint8_t grn_button_prev = 0;
	uint8_t grn_button;

	char buf[32];

	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  hour;
	uint8_t  min;
	uint8_t  sec;
	
	uint8_t need_redraw = 0;
	
	// Update buttons
	red_button = PINE & (1 << 5);
	grn_button = PINE & (1 << 4);
	
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
		else if (grn_button_prev && !grn_button)
		{
			Menu_state = menu_active;
			Menu_countdown = 6;
			Menu_countdown_timer = 1000;
			need_redraw = 1;
		}
		else if (red_button_prev && !red_button && Menu_start_time_valid)
		{
			Menu_state = menu_hold;
			need_redraw = 1;
		}
	}
	else if (Menu_state == menu_active)
	{
		if (red_button_prev && !red_button)
		{
			Menu_state = menu_idle_no_fix;
			need_redraw = 1;
		}
	}
	else if (Menu_state == menu_hold)
	{
		if (red_button_prev && !red_button)
		{
			Menu_state = menu_idle_no_fix;
			need_redraw = 1;
		}
	}

	// Handle flags
	if (Menu_flags & MENU_FLAGS_PLAY_NEXT)
	{
		if (Menu_countdown == 0)
		{
			Menu_start_time = UBX_curTime + (UBX_curMillisecond / 1000);
			Menu_start_time_ms = UBX_curMillisecond % 1000;
			Menu_start_time_valid = 1;

			Tone_Beep(TONE_MAX_PITCH, 0, 4 * TONE_LENGTH_125_MS);

			Menu_state = menu_hold;
			need_redraw = 1;
		}
		else
		{
			buf[0] = '0' + Menu_countdown;
			buf[1] = '.';
			buf[2] = 'w';
			buf[3] = 'a';
			buf[4] = 'v';
			buf[5] = 0;
			
			Tone_Play(buf);
		}

		Menu_flags &= ~MENU_FLAGS_PLAY_NEXT;
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
		else if (Menu_state == menu_hold)
		{
			gmtime_r(Menu_start_time, &year, &month, &day, &hour, &min, &sec);
			sprintf(buf, "  %02d:%02d:%02d.%03d", hour, min, sec, Menu_start_time_ms);

			LCD_Clear();
			LCD_Show(buf);
		}
	}
	
	// Update buttons
	red_button_prev = red_button;
	grn_button_prev = grn_button;
}

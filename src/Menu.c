#include <avr/io.h>

static enum
{
	menu_idle,
	menu_active
}
Menu_state = menu_idle;

void Menu_Task(void)
{
	char buf[32];

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
		sprintf(buf, 
		        "Idle");
		
		LCD_Clear();
		LCD_Show(buf);
	}
	else
	{
		sprintf(buf, 
		        "Active");
		
		LCD_Clear();
		LCD_Show(buf);
	}
}

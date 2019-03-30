/***************************************************************************
**                                                                        **
**  FlySight firmware                                                     **
**  Copyright 2019 Michael Cooper                                         **
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

#include <stdlib.h>
#include <string.h>

#include "FatFS/ff.h"
#include "Config.h"
#include "Main.h"
#include "UBX.h"

static char EEPROM saved_key[16];

static uint8_t Key_CharToVal(
	char    ch,
	uint8_t *val)
{
	if (ch >= '0' && ch <= '9')
	{
		*val = ch - '0';
		return 0;
	}
	else if (ch >= 'a' && ch <= 'f')
	{
		*val = ch - 'a' + 10;
		return 0;
	}
	else if (ch >= 'A' && ch <= 'F')
	{
		*val = ch - 'A' + 10;
		return 0;
	}
	return 1;
}

static void Key_ReadFromFile(
	const char *dir,
	const char *filename)
{
	size_t  len, i;
	uint8_t ch1, ch2;

	FRESULT res;

	res = f_chdir(dir);
	if (res != FR_OK) return;
	
	res = f_open(&Main_file, filename, FA_READ|FA_WRITE);
	if (res != FR_OK) return;

	f_gets(Config_buf, sizeof(Config_buf), &Main_file);
	
	len = strcspn(Config_buf, " \r\n\t");
	Config_buf[len] = 0;

	if (strlen(Config_buf) == 32)
	{
		for (i = 0; i < 16; ++i)
		{
			if (Key_CharToVal(Config_buf[2 * i], &ch1) ||
			    Key_CharToVal(Config_buf[2 * i + 1], &ch2))
			{
				break;
			}
			else
			{
				UBX_key[i] = (ch1 << 4) + ch2;
			}
		}
		
		if (i == 16)
		{
			eeprom_write_block(&UBX_key, saved_key, 16);
		}
	}

	f_lseek(&Main_file, 0);
	f_puts("................................", &Main_file);
	
	f_close(&Main_file);
	f_unlink(filename);
	
	return FR_OK;
}

void Key_Read(void)
{
	Key_ReadFromFile("\\", "key.txt");
	eeprom_read_block(&UBX_key, saved_key, 16);
}

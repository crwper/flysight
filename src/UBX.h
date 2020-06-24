/***************************************************************************
**                                                                        **
**  FlySight firmware                                                     **
**  Copyright 2018 Michael Cooper, Luke Hederman                          **
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

#ifndef MGC_UBX_H
#define MGC_UBX_H

#include <avr/io.h>

#define UBX_MAX_ALARMS   10
#define UBX_MAX_WINDOWS  2
#define UBX_MAX_SPEECH   3

#define UBX_BUFFER_LEN   150
#define UBX_FILENAME_LEN 13

#define UBX_UNITS_KMH       0
#define UBX_UNITS_MPH       1

#define MODE_Horizontal_speed           0
#define MODE_Vertical_speed             1
#define MODE_Glide_ratio                2
#define MODE_Inverse_glide_ratio        3
#define MODE_Total_speed                4
#define MODE_Direction_to_destination   5
#define MODE_Distance_to_destination    6
#define MODE_Direction_to_bearing       7
#define MODE_Magnitude_of_Value_1       8
#define MODE_Change_in_Value_1          9
#define MODE_LeftRight                 10
#define MODE_Dive_angle                11

#define SP_MODE_Horizontal_speed           0
#define SP_MODE_Vertical_speed             1
#define SP_MODE_Glide_ratio                2
#define SP_MODE_Inverse_glide_ratio        3
#define SP_MODE_Total_speed                4
#define SP_MODE_Direction_to_destination   5
#define SP_MODE_Distance_to_destination    6
#define SP_MODE_Direction_to_bearing       7
#define SP_MODE_Dive_angle                11
#define SP_MODE_Altitude                  12

#define MODEL_Portable                  0
#define MODEL_Stationary                2
#define MODEL_Pedestrian                3
#define MODEL_Automotive                4
#define MODEL_Sea                       5
#define MODEL_Airborne1G                6
#define MODEL_Airborne2G                7
#define MODEL_Airborne4G                8

#define UBX_UNITS_METERS    0
#define UBX_UNITS_FEET      1

typedef struct
{
	int32_t elev;
	uint8_t type;
	char    filename[9];
}
UBX_alarm_t;

typedef struct
{
	int32_t top;
	int32_t bottom;
}
UBX_window_t;

typedef struct
{
	uint8_t mode;
	uint8_t units;
	int32_t decimals;
}
UBX_speech_t;

typedef struct
{
	char buffer[UBX_BUFFER_LEN - UBX_FILENAME_LEN];
	char filename[UBX_FILENAME_LEN];
}
UBX_buffer_t;

extern uint8_t   UBX_model;
extern uint16_t  UBX_rate;
extern uint8_t   UBX_mode;
extern int32_t   UBX_min;
extern int32_t   UBX_max;

extern uint8_t   UBX_mode_2;
extern int32_t   UBX_min_2;
extern int32_t   UBX_max_2;
extern int32_t   UBX_min_rate;
extern int32_t   UBX_max_rate;
extern uint8_t   UBX_flatline;
extern uint8_t   UBX_limits;
extern uint8_t   UBX_use_sas;

extern int32_t   UBX_threshold;
extern int32_t   UBX_hThreshold;

extern UBX_alarm_t UBX_alarms[UBX_MAX_ALARMS];
extern uint8_t   UBX_num_alarms;
extern int32_t   UBX_alarm_window_above;
extern int32_t   UBX_alarm_window_below;

extern UBX_speech_t UBX_speech[UBX_MAX_SPEECH];
extern uint8_t      UBX_num_speech;
extern uint8_t      UBX_cur_speech;
extern uint16_t     UBX_sp_rate;

extern uint8_t   UBX_alt_units;
extern int32_t   UBX_alt_step;

extern uint8_t   UBX_init_mode;
extern char      UBX_init_filename[9];

extern int32_t   UBX_dLat;
extern int32_t   UBX_dLon;
extern int16_t   UBX_bearing;
extern uint16_t  UBX_end_nav;
extern uint16_t  UBX_max_dist;
extern uint16_t  UBX_min_angle;

extern UBX_buffer_t UBX_buffer;

extern UBX_window_t UBX_windows[UBX_MAX_WINDOWS];
extern uint8_t    UBX_num_windows;

extern int32_t    UBX_dz_elev;

void UBX_Init(void);
void UBX_Task(void);
void UBX_Update(void);

#endif

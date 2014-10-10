#ifndef MGC_UBX_H
#define MGC_UBX_H

#include <avr/io.h>

extern uint8_t   UBX_model;
extern uint16_t  UBX_rate;
extern uint8_t   UBX_mode;
extern int32_t   UBX_min;
extern int32_t   UBX_max;
extern int32_t   UBX_reference;
extern uint8_t   UBX_divider;
extern uint8_t   UBX_limits;

void UBX_Init(void);
void UBX_Task(void);
void UBX_Update(void);

#endif

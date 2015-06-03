#ifndef MGC_UBX_H
#define MGC_UBX_H

#include <avr/io.h>

#define UBX_MAX_MILLISECOND 2000

extern volatile uint32_t UBX_curTime;
extern volatile uint16_t UBX_curMillisecond;

void UBX_Init(void);
void UBX_Task(void);
void UBX_Update(void);

#endif

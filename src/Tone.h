#ifndef MGC_TONE_H
#define MGC_TONE_H

#include <avr/io.h>

#define TONE_MAX_PITCH 65280

extern uint32_t Tone_lock;
extern uint16_t Tone_volume;

void Tone_Init(void);

void Tone_SetPitch(uint16_t index);
void Tone_SetReference(uint16_t index);

void Tone_Task(void);

void Tone_Start(void);
void Tone_Stop(void);

uint8_t Tone_CanWrite(void);
uint8_t Tone_IsIdle(void);

#endif

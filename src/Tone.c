#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "Board/LEDs.h"
#include "FatFS/ff.h"
#include "Log.h"
#include "Main.h"
#include "Power.h"
#include "Tone.h"

#define MIN(a,b) (((a) < (b)) ?  (a) : (b))
#define MAX(a,b) (((a) > (b)) ?  (a) : (b))

#define TONE_BUFFER_LEN   MAIN_BUFFER_SIZE		 // size of circular buffer
#define TONE_BUFFER_CHUNK (TONE_BUFFER_LEN / 8)  // maximum bytes read in one operation
#define TONE_BUFFER_WRITE (TONE_BUFFER_LEN - TONE_BUFFER_CHUNK)  // buffered samples required to allow write/flush

#define TONE_SAMPLE_LEN  4  // number of repeated PWM samples

#define TONE_STATE_IDLE  0
#define TONE_STATE_PLAY  1

#define TONE_FLAGS_LOAD  1
#define TONE_FLAGS_STOP  2
#define TONE_FLAGS_BEEP  4

static const uint8_t Tone_sine_table[] PROGMEM =
{
	128, 131, 134, 137, 140, 143, 146, 149,
	153, 156, 159, 162, 165, 168, 171, 174,
	177, 180, 182, 185, 188, 191, 194, 196,
	199, 201, 204, 207, 209, 211, 214, 216,
	218, 220, 223, 225, 227, 229, 231, 232,
	234, 236, 238, 239, 241, 242, 243, 245,
	246, 247, 248, 249, 250, 251, 252, 253,
	253, 254, 254, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 254, 254, 253,
	253, 252, 251, 251, 250, 249, 248, 247,
	245, 244, 243, 241, 240, 238, 237, 235,
	233, 232, 230, 228, 226, 224, 222, 219,
	217, 215, 213, 210, 208, 205, 203, 200,
	198, 195, 192, 189, 187, 184, 181, 178,
	175, 172, 169, 166, 163, 160, 157, 154,
	151, 148, 145, 142, 139, 135, 132, 129,
	126, 123, 120, 116, 113, 110, 107, 104,
	 101, 98,  95,  92,  89,  86,  83,  80,
	 77,  74,  71,  68,  66,  63,  60,  57,
	 55,  52,  50,  47,  45,  42,  40,  38,
	 36,  33,  31,  29,  27,  25,  23,  22,
	 20,  18,  17,  15,  14,  12,  11,  10,
	  8,   7,   6,   5,   4,   4,   3,   2,
	  2,   1,   1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   1,   1,   2,
	  2,   3,   4,   5,   6,   7,   8,   9,
	 10,  12,  13,  14,  16,  17,  19,  21,
	 23,  24,  26,  28,  30,  32,  35,  37,
	 39,  41,  44,  46,  48,  51,  54,  56,
	 59,  61,  64,  67,  70,  73,  75,  78,
	 81,  84,  87,  90,  93,  96,  99, 102,
	106, 109, 112, 115, 118, 121, 124, 128
};

static volatile uint16_t Tone_read;
static volatile uint16_t Tone_write;

static          uint32_t Tone_step;
static          uint32_t Tone_step_ref;

static volatile uint8_t  Tone_state = TONE_STATE_IDLE;

static          FIL      Tone_file;

                uint32_t Tone_lock   = 1;
                uint16_t Tone_volume = 2;

static volatile uint8_t  Tone_flags = 0;

ISR(TIMER1_OVF_vect)
{
	static uint8_t i = 0;
	static uint16_t s1, s2, step;

	if (i++ % TONE_SAMPLE_LEN)
	{
		s1 += step;
	}
	else if (Tone_read == Tone_write)
	{
		TCCR1A = 0;
		TCCR1B = 0;
		TIMSK1 = 0;

		Tone_flags |= TONE_FLAGS_STOP;
	}
	else 
	{
		s1 = s2;
		s2 = (uint16_t) Main_buffer[Tone_read % TONE_BUFFER_LEN] << 8;
		
		// The contortions below are necessary to ensure that the division by 
		// TONE_SAMPLE_LEN uses shift operations instead of calling a signed 
		// integer division function.
		
		if (s1 <= s2)
		{
			step = (s2 - s1) / TONE_SAMPLE_LEN;
		}
		else
		{
			step = -((s1 - s2) / TONE_SAMPLE_LEN);
		}
		
		++Tone_read;
	}

	OCR1A = OCR1B = s1 >> 8;
}

void Tone_Init(void)
{
	DDRB |= (1 << 6) | (1 << 5);
}

void Tone_SetPitch(
	uint16_t index)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		Tone_step = ((int32_t) index * 3242 + 30212096) * TONE_SAMPLE_LEN;
	}
}

void Tone_SetReference(
	uint16_t index)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		Tone_step_ref = ((int32_t) index * 3242 + 30212096) * TONE_SAMPLE_LEN;
	}
}

static void Tone_LoadTable(void)
{
	static uint16_t phase     = 0;
	static uint16_t phase_ref = 0;
	       uint8_t  val;
		   uint16_t read;
	       uint16_t size, i;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		read = Tone_read;
	}

	size = read + TONE_BUFFER_LEN - Tone_write;

	for (i = 0; i < size; ++i)
	{
		val =  pgm_read_byte(&Tone_sine_table[phase >> 8]) / 2;
		val += pgm_read_byte(&Tone_sine_table[phase_ref >> 8]) / 2;

		phase     += Tone_step >> 16;
		phase_ref += Tone_step_ref >> 16;

		phase_ref = ((256 - Tone_lock) * phase_ref + Tone_lock * phase) / 256;

		val = 128 - (128 >> Tone_volume) + (val >> Tone_volume);
		Main_buffer[(Tone_write + i) % TONE_BUFFER_LEN] = val;
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		Tone_write += size;
	}
}

static void Tone_Load(void)
{
	Tone_LoadTable();
}

void Tone_Start(void)
{
	if (Tone_state == TONE_STATE_IDLE)
	{
		Tone_state = TONE_STATE_PLAY;
		
		Tone_flags |= TONE_FLAGS_LOAD;
		
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			Tone_read  = 0;
			Tone_write = 0;
		}

		Tone_Load();
		
		TCNT1 = 255;
		OCR1A = OCR1B = Main_buffer[0];
		
		TCCR1A = (1 << COM1A1) | (1 << COM1A0) | (1 << COM1B1) | (1 << WGM10);
		TCCR1B = (1 << WGM12) | (1 << CS10);
		TIMSK1 = (1 << TOIE1);
	}
}

void Tone_Stop(void)
{
	if (Tone_state != TONE_STATE_IDLE)
	{
		TCCR1A = 0;
		TCCR1B = 0;
		TIMSK1 = 0;

		Tone_state = TONE_STATE_IDLE;
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		Tone_flags &= ~TONE_FLAGS_STOP;
		Tone_flags &= ~TONE_FLAGS_LOAD;
	}
}

void Tone_Task(void)
{
	if (Tone_flags & TONE_FLAGS_LOAD)
	{
		Tone_Load();
	}
}

uint8_t Tone_CanWrite(void)
{
	uint16_t c;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		c = Tone_write - Tone_read;
	}

	return (c > TONE_BUFFER_WRITE);
}

uint8_t Tone_IsIdle(void)
{
	return Tone_state == TONE_STATE_IDLE;
}
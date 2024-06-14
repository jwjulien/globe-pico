/* =====================================================================================================================
 *      File:  /src/main.cpp
 *   Project:  Firmware
 *    Author:  Jared Julien <jaredjulien@exsystems.net>
 * Copyright:  (c) 2024 Jared Julien, eX Systems
 * ---------------------------------------------------------------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ---------------------------------------------------------------------------------------------------------------------
 */
// =====================================================================================================================
// Includes
// ---------------------------------------------------------------------------------------------------------------------
#include "Arduino.h"
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "apa102.pio.h"




//======================================================================================================================
// Definitions
//----------------------------------------------------------------------------------------------------------------------
#define PIN_CLK_A 18
#define PIN_DATA_A 19
#define PIN_CLK_B 20
#define PIN_DATA_B 21

#define PIN_HALL 27

#define N_LEDS 52
#define SERIAL_FREQ (5 * 1000 * 1000)

// Global brightness value 0->31
#define BRIGHTNESS 16

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif




//======================================================================================================================
// Module Variables
//----------------------------------------------------------------------------------------------------------------------
#define TABLE_SIZE (1 << 8)
static uint8_t wave_table[TABLE_SIZE];





//======================================================================================================================
// Helpers
//----------------------------------------------------------------------------------------------------------------------
void put_start_frame(PIO pio, uint sm)
{
	pio_sm_put_blocking(pio, sm, 0u);
}


//----------------------------------------------------------------------------------------------------------------------
void put_end_frame(PIO pio, uint sm)
{
	pio_sm_put_blocking(pio, sm, ~0u);
}


//----------------------------------------------------------------------------------------------------------------------
void put_rgb888(PIO pio, uint sm, uint8_t r, uint8_t g, uint8_t b)
{
    pio_sm_put_blocking(pio, sm,
                        0x7 << 29 |                   // magic
                        (BRIGHTNESS & 0x1f) << 24 |   // global brightness parameter
                        (uint32_t) b << 16 |
                        (uint32_t) g << 8 |
                        (uint32_t) r << 0
    );
}






//======================================================================================================================
// Setup Function
//----------------------------------------------------------------------------------------------------------------------
void setup(void)
{
	Serial.begin(115200);

	uint8_t offset = pio_add_program(pio0, &apa102_mini_program);
	apa102_mini_program_init(pio0, 0, offset, SERIAL_FREQ, PIN_CLK_A, PIN_DATA_A);
	apa102_mini_program_init(pio0, 1, offset, SERIAL_FREQ, PIN_CLK_B, PIN_DATA_B);

	// Generate a color table for later lookup.
	for (int i = 0; i < TABLE_SIZE; ++i)
		wave_table[i] = (uint8_t) (powf(sinf(i * M_PI / TABLE_SIZE), 5.f) * 255);

	// Setup hall effect input.
	pinMode(PIN_HALL, INPUT_PULLUP);
}




//======================================================================================================================
// Loop Function
//----------------------------------------------------------------------------------------------------------------------
void loop(void)
{
	static int t = 0;

	put_start_frame(pio0, 0);
	for (int i = 0; i < N_LEDS; ++i) {
		if (digitalRead(PIN_HALL) == LOW)
		{
			put_rgb888(pio0, 0,
						wave_table[(i + t) % TABLE_SIZE],
						wave_table[(2 * i + 3 * 2) % TABLE_SIZE],
						wave_table[(3 * i + 4 * t) % TABLE_SIZE]
			);
		}
		else
		{
			put_rgb888(pio0, 0, 0, 0, 0);
		}
	}
	put_end_frame(pio0, 0);
	put_start_frame(pio0, 1);
	for (int i = 0; i < N_LEDS; ++i) {
		if (digitalRead(PIN_HALL) == LOW)
		{
			put_rgb888(pio0, 1,
						wave_table[(i + t) % TABLE_SIZE],
						wave_table[(2 * i + 3 * 2) % TABLE_SIZE],
						wave_table[(3 * i + 4 * t) % TABLE_SIZE]
			);
		}
		else
		{
			put_rgb888(pio0, 1, 0, 0, 0);
		}
	}
	put_end_frame(pio0, 1);
	sleep_ms(10);
	t++;
}



/* End of File */

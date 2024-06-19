/* =====================================================================================================================
 *      File:  /src/main.cpp
 *   Project:  POV Globe
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
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "apa102.pio.h"

#include "constants.h"
#include "images.h"
#include "pins.h"
#include "rtt.h"




//======================================================================================================================
// Module Variables
//----------------------------------------------------------------------------------------------------------------------
static uint32_t column_left_a[COLUMN_BUFFER_SIZE];
static uint32_t column_left_b[COLUMN_BUFFER_SIZE];
static uint32_t column_right_a[COLUMN_BUFFER_SIZE];
static uint32_t column_right_b[COLUMN_BUFFER_SIZE];
static uint32_t frame_a[RES_HORIZ][RES_VERT];
static bool use_a = true;




//======================================================================================================================
// Helpers
//----------------------------------------------------------------------------------------------------------------------
uint32_t convert_rgb_to_apa102(uint32_t value)
{
	uint8_t red = (value >> 16) & 0xFF;
	uint8_t green = (value >> 8) & 0xFF;
	uint8_t blue = value & 0xFF;

	uint32_t apa102 = 0x7 << 29
		| (BRIGHTNESS & 0x1f) << 24
		| (uint32_t) blue << 16
		| (uint32_t) green << 8
		| (uint32_t) red << 0;

	return apa102;
}


//----------------------------------------------------------------------------------------------------------------------
static void render(String active_regions)
{
	for (uint8_t idx = 0; idx < REGION_COUNT; idx++)
	{
		const Region_t * region = Regions[idx];

		bool is_active = active_regions[idx] == '1';
		uint32_t color = is_active ? ColorMap[idx] : INACTIVE_COLOR;

		for (uint8_t column = 0; column < REGION_WIDTH; column++)
		{
			for (uint8_t row = 0; row < REGION_HEIGHT; row++)
			{
				uint8_t byte = row / 8;
				uint8_t bit = row % 8;
				uint8_t mask = 1 << (7 - bit);
				if ((*region)[column][byte] & mask)
				{
					frame_a[column + REGION_OFFSET_X][row + REGION_OFFSET_Y] |= color;
				}
			}
		}
	}
}





//======================================================================================================================
// Setup Function
//----------------------------------------------------------------------------------------------------------------------
void setup(void)
{
	Serial.begin(115200);

	uint8_t offset = pio_add_program(LED_PIO, &apa102_mini_program);
	apa102_mini_program_init(LED_PIO, LED_SM_A, offset, SERIAL_FREQ, PIN_CLK_A, PIN_DATA_A);
	apa102_mini_program_init(LED_PIO, LED_SM_B, offset, SERIAL_FREQ, PIN_CLK_B, PIN_DATA_B);

	// Setup DMA to load LED PIO outputs from RAM buffers.
	dma_channel_config config = dma_channel_get_default_config(LED_SM_A);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
	channel_config_set_read_increment(&config, true);
	channel_config_set_write_increment(&config, false);
	channel_config_set_dreq(&config, pio_get_dreq(LED_PIO, LED_SM_A, true));
	dma_channel_configure(LED_SM_A, &config, &LED_PIO->txf[LED_SM_A], column_left_a, COLUMN_BUFFER_SIZE, false);

	config = dma_channel_get_default_config(LED_SM_B);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
	channel_config_set_read_increment(&config, true);
	channel_config_set_write_increment(&config, false);
	channel_config_set_dreq(&config, pio_get_dreq(LED_PIO, LED_SM_B, true));
	dma_channel_configure(LED_SM_B, &config, &LED_PIO->txf[LED_SM_B], column_right_a, COLUMN_BUFFER_SIZE, false);

	column_left_a[0] = 0;
	column_left_a[COLUMN_BUFFER_SIZE - 1] = ~0;
	column_left_b[0] = 0;
	column_left_b[COLUMN_BUFFER_SIZE - 1] = ~0;
	column_right_a[0] = 0;
	column_right_a[COLUMN_BUFFER_SIZE - 1] = ~0;
	column_right_b[0] = 0;
	column_right_b[COLUMN_BUFFER_SIZE - 1] = ~0;

	render(String("1000000001000000"));
}

void setup1(void)
{
	rtt_setup();
}




//======================================================================================================================
// Loop Function
//----------------------------------------------------------------------------------------------------------------------
void loop(void)
{
	static uint8_t previous_column = -1;
	static uint32_t offset = 60;
	static uint32_t previous_time = 0;

	if (rtt_rotating())
	{
		uint8_t current_column = rtt_column();

		if (current_column != previous_column)
		{
			previous_column = current_column;

			// Add the offset to slowly rotate the image.
			current_column = ((uint32_t)current_column + offset) % RES_HORIZ;

			// Trigger DMA to run output.
			dma_channel_set_read_addr(LED_SM_A, use_a ? column_left_a : column_left_b, false);
			dma_channel_set_read_addr(LED_SM_B, use_a ? column_right_a : column_right_b, false);
			dma_channel_set_trans_count(LED_SM_A, COLUMN_BUFFER_SIZE, true);
			dma_channel_set_trans_count(LED_SM_B, COLUMN_BUFFER_SIZE, true);

			// Swap Buffers
			use_a = !use_a;

			uint8_t opposite_column = (uint8_t)(((uint16_t)current_column + (RES_HORIZ / 2)) % RES_HORIZ);

			// Rebuild the next buffer for the next loop.
			for (int idx = 0; idx < LED_COUNT; idx++)
			{
				uint32_t left = frame_a[current_column][idx * 2 + 1];
				uint32_t right = frame_a[opposite_column][idx * 2];
				left = convert_rgb_to_apa102(left);
				right = convert_rgb_to_apa102(right);
				if (use_a)
				{
					column_left_a[LED_COUNT - idx] = left;
					column_right_a[LED_COUNT - idx] = right;
				}
				else
				{
					column_left_b[LED_COUNT - idx] = left;
					column_right_b[LED_COUNT - idx] = right;
				}
			}
		}
	}
	else
	{
		while (dma_channel_is_busy(LED_SM_A) || dma_channel_is_busy(LED_SM_B));

		// Go black when not rotating.
		for (int idx = 0; idx < LED_COUNT; idx++)
		{
			column_left_a[idx + 1] = convert_rgb_to_apa102(0);
			column_right_a[idx + 1] = convert_rgb_to_apa102(0);
		}
		dma_channel_set_read_addr(LED_SM_A, column_left_a, false);
		dma_channel_set_read_addr(LED_SM_B, column_right_a, false);
		dma_channel_set_trans_count(LED_SM_A, COLUMN_BUFFER_SIZE, true);
		dma_channel_set_trans_count(LED_SM_B, COLUMN_BUFFER_SIZE, true);
	}

	uint32_t elapsed = millis() - previous_time;
	if (elapsed > 250)
	{
		previous_time += 250;
		offset++;
	}
}




/* End of File */

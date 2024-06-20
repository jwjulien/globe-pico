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
#include <WiFi.h>
#include <ArduinoHttpClient.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "apa102.pio.h"

#include "constants.h"
#include "images.h"
#include "pins.h"
#include "rtt.h"




//======================================================================================================================
// Data Types
//----------------------------------------------------------------------------------------------------------------------
typedef uint32_t Frame_t[RES_HORIZ][RES_VERT];





//======================================================================================================================
// Constants
//----------------------------------------------------------------------------------------------------------------------
char ssid[] = "NXTRVISITOR";
char pass[] = "";
char host[] = "starmap.generationnextcoding.com";
int port = 80;
char url[] = "/compasseval.php";




//======================================================================================================================
// Module Variables
//----------------------------------------------------------------------------------------------------------------------
static uint32_t column_left_a[COLUMN_BUFFER_SIZE];
static uint32_t column_left_b[COLUMN_BUFFER_SIZE];
static uint32_t column_right_a[COLUMN_BUFFER_SIZE];
static uint32_t column_right_b[COLUMN_BUFFER_SIZE];
static Frame_t frame_a;
static Frame_t frame_b;
static bool use_column_a = true;
static bool use_frame_a = true;
PIO led_pio = pio1;
uint8_t led_a_sm;
uint8_t led_b_sm;
uint8_t led_a_dma;
uint8_t led_b_dma;

WiFiClient client;
HttpClient http = HttpClient(client, host, port);



//======================================================================================================================
// Helpers
//----------------------------------------------------------------------------------------------------------------------
uint32_t __time_critical_func(convert_rgb_to_apa102)(uint32_t value)
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
static void clear(Frame_t * frame)
{
	for (uint8_t column = 0; column < RES_HORIZ; column++)
	{
		for (uint8_t row = 0; row < RES_VERT; row++)
		{
			(*frame)[column][row] = 0;
		}
	}
}


//----------------------------------------------------------------------------------------------------------------------
static void render(String active_regions)
{
	// Build into currently unused frame buffer.
	Frame_t * frame = use_frame_a ? &frame_b : &frame_a;

	clear(frame);

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
					(*frame)[column + REGION_OFFSET_X][row + REGION_OFFSET_Y] |= color;
				}
			}
		}
	}

	// Swap frame buffers.
	use_frame_a = !use_frame_a;
}


//----------------------------------------------------------------------------------------------------------------------
static void refresh(void)
{
	if (WiFi.status() == WL_CONNECTED)
	{
		http.get(url);
		int statusCode = http.responseStatusCode();

		if (statusCode == 200)
		{
			String response = http.responseBody();
			if (response.length() >= 16)
			{
				render(response);
			}
		}

		http.stop();
	}
}





//======================================================================================================================
// Setup Function
//----------------------------------------------------------------------------------------------------------------------
void setup1(void)
{
	Serial.begin(115200);
	// uint32_t timeoutStart = millis();
	// while (!Serial && ((millis() - timeoutStart) < 5000));
	// delay(2000);

	uint8_t offset = pio_add_program(led_pio, &apa102_mini_program);
	led_a_sm = pio_claim_unused_sm(led_pio, true);
	apa102_mini_program_init(led_pio, led_a_sm, offset, SERIAL_FREQ, PIN_CLK_A, PIN_DATA_A);
	led_b_sm = pio_claim_unused_sm(led_pio, true);
	apa102_mini_program_init(led_pio, led_b_sm, offset, SERIAL_FREQ, PIN_CLK_B, PIN_DATA_B);

	// Setup DMA to load LED PIO outputs from RAM buffers.
	led_a_dma = dma_claim_unused_channel(true);
	dma_channel_config config = dma_channel_get_default_config(led_a_dma);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
	channel_config_set_read_increment(&config, true);
	channel_config_set_write_increment(&config, false);
	channel_config_set_dreq(&config, pio_get_dreq(led_pio, led_a_dma, true));
	dma_channel_configure(led_a_dma, &config, &led_pio->txf[led_a_dma], column_left_a, COLUMN_BUFFER_SIZE, false);

	led_b_dma = dma_claim_unused_channel(true);
	config = dma_channel_get_default_config(led_b_dma);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
	channel_config_set_read_increment(&config, true);
	channel_config_set_write_increment(&config, false);
	channel_config_set_dreq(&config, pio_get_dreq(led_pio, led_b_dma, true));
	dma_channel_configure(led_b_dma, &config, &led_pio->txf[led_b_dma], column_right_a, COLUMN_BUFFER_SIZE, false);

	column_left_a[0] = 0;
	column_left_a[COLUMN_BUFFER_SIZE - 1] = ~0;
	column_left_b[0] = 0;
	column_left_b[COLUMN_BUFFER_SIZE - 1] = ~0;
	column_right_a[0] = 0;
	column_right_a[COLUMN_BUFFER_SIZE - 1] = ~0;
	column_right_b[0] = 0;
	column_right_b[COLUMN_BUFFER_SIZE - 1] = ~0;

	render(String("1111111111111111"));
	rtt_setup();
}

void setup(void)
{
	WiFi.begin(ssid, pass);
}




//======================================================================================================================
// Loop Function
//----------------------------------------------------------------------------------------------------------------------
void __time_critical_func(loop1)(void)
{
	static uint8_t previous_column = -1;
	static uint32_t offset = 60;
	static uint32_t previous_rotation = 0;
	static bool running = false;

	if (rtt_rotating())
	{
		uint8_t current_column = rtt_column();
		running = true;

		if (current_column != previous_column)
		{
			previous_column = current_column;

			// Add the offset to slowly rotate the image.
			current_column = ((uint32_t)current_column + offset) % RES_HORIZ;

			// Trigger DMA to run output.
			dma_channel_set_read_addr(led_a_dma, use_column_a ? column_left_a : column_left_b, false);
			dma_channel_set_read_addr(led_b_dma, use_column_a ? column_right_a : column_right_b, false);
			dma_channel_set_trans_count(led_a_dma, COLUMN_BUFFER_SIZE, true);
			dma_channel_set_trans_count(led_b_dma, COLUMN_BUFFER_SIZE, true);

			// Swap Buffers
			use_column_a = !use_column_a;

			uint8_t opposite_column = (uint8_t)(((uint16_t)current_column + (RES_HORIZ / 2)) % RES_HORIZ);

			Frame_t * frame = use_frame_a ? &frame_a : &frame_b;

			// Rebuild the next buffer for the next loop.
			for (int idx = 0; idx < LED_COUNT; idx++)
			{
				uint32_t left = (*frame)[current_column][idx * 2 + 1];
				uint32_t right = (*frame)[opposite_column][idx * 2];
				left = convert_rgb_to_apa102(left);
				right = convert_rgb_to_apa102(right);
				if (use_column_a)
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
		if (running)
		{
			running = false;
			while (dma_channel_is_busy(led_a_dma) || dma_channel_is_busy(led_b_dma));

			// Go black when not rotating.
			for (int idx = 0; idx < LED_COUNT; idx++)
			{
				column_left_a[idx + 1] = convert_rgb_to_apa102(0);
				column_right_a[idx + 1] = convert_rgb_to_apa102(0);
			}
			dma_channel_set_read_addr(led_a_dma, column_left_a, false);
			dma_channel_set_read_addr(led_b_dma, column_right_a, false);
			dma_channel_set_trans_count(led_a_dma, COLUMN_BUFFER_SIZE, true);
			dma_channel_set_trans_count(led_b_dma, COLUMN_BUFFER_SIZE, true);
		}
	}

	uint32_t elapsed = millis() - previous_rotation;
	if (elapsed > ROTATION_TIME)
	{
		previous_rotation += ROTATION_TIME;
		offset++;
		// digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	}
}


void __time_critical_func(loop)(void)
{
	static uint32_t previous_refresh = 0;

	uint32_t elapsed = millis() - previous_refresh;
	if (elapsed > REFRESH_TIME)
	{
		refresh();
		previous_refresh = millis();
	}
}



/* End of File */

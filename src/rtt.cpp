/* =====================================================================================================================
 *      File:  /src/rtt.cpp
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
#include "constants.h"
#include "pins.h"
#include "rtt.h"




#define ROTATIONS 32

//======================================================================================================================
// Module Variables
//----------------------------------------------------------------------------------------------------------------------
static critical_section_t _critical_section;
static uint32_t _deltas[ROTATIONS];
static uint8_t _head;
static absolute_time_t _last_event;




//======================================================================================================================
// IRQ Callback
//----------------------------------------------------------------------------------------------------------------------
static void _gpio_hall_sensor_callback(uint gpio, uint32_t events)
{
	if (gpio != PIN_HALL)
	{
		return;
	}

	absolute_time_t now = get_absolute_time();

	critical_section_enter_blocking(&_critical_section);
	if (!is_nil_time(_last_event))
	{
		_deltas[_head] = absolute_time_diff_us(_last_event, now);
		_head = (_head + 1) % ROTATIONS;
	}
	_last_event = now;
	critical_section_exit(&_critical_section);
}


//----------------------------------------------------------------------------------------------------------------------
static uint32_t _rev_time(void)
{
	uint32_t sum = 0;
	critical_section_enter_blocking(&_critical_section);
	for (uint8_t idx = 0; idx < ROTATIONS; idx++)
	{
		sum += _deltas[idx];
	}
	critical_section_exit(&_critical_section);
	sum /= ROTATIONS;
	return sum;
}




//======================================================================================================================
// Round Trip Timer Setup
//----------------------------------------------------------------------------------------------------------------------
void rtt_setup(void)
{
	_head = 0;
	_last_event = nil_time;

	critical_section_init(&_critical_section);

	pinMode(PIN_HALL, INPUT_PULLUP);
	gpio_set_irq_enabled_with_callback(PIN_HALL, GPIO_IRQ_EDGE_FALL, true, _gpio_hall_sensor_callback);
}




//======================================================================================================================
// RTT Column Function
//----------------------------------------------------------------------------------------------------------------------
/* Return the current column index. */
uint8_t rtt_column(void)
{
	absolute_time_t now = get_absolute_time();
	uint32_t rot_delta = absolute_time_diff_us(_last_event, now);
	uint8_t column = rot_delta * RES_HORIZ / _rev_time();

	return column % RES_HORIZ;
}


//----------------------------------------------------------------------------------------------------------------------
bool rtt_rotating(void)
{
	return !is_nil_time(_last_event) && (_rev_time() < 100000);
}




/* End of File */

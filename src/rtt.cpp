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




//======================================================================================================================
// Module Variables
//----------------------------------------------------------------------------------------------------------------------
static critical_section_t _critical_section;
static uint32_t _rev_time;
static absolute_time_t _last_hall_sensor_event;




//======================================================================================================================
// IRQ Callback
//----------------------------------------------------------------------------------------------------------------------
static void _gpio_hall_sensor_callback(uint gpio, uint32_t events)
{
	if (gpio != PIN_HALL)
	{
		return;
	}

	const absolute_time_t curr_time = get_absolute_time();

	critical_section_enter_blocking(&_critical_section);
	if (!is_nil_time(_last_hall_sensor_event)){
		uint64_t new_dt = absolute_time_diff_us(_last_hall_sensor_event, curr_time);
		_rev_time = (0.25 * new_dt) + (0.75 * _rev_time);
	}
	_last_hall_sensor_event = curr_time;
	critical_section_exit(&_critical_section);
}




//======================================================================================================================
// Round Trip Timer Setup
//----------------------------------------------------------------------------------------------------------------------
void rtt_setup(void)
{
	_rev_time = 0;
	_last_hall_sensor_event = nil_time;

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
	critical_section_enter_blocking(&_critical_section);
	absolute_time_t last_hall_event = _last_hall_sensor_event;
	uint32_t rev_time = _rev_time;
	critical_section_exit(&_critical_section);

	absolute_time_t curr_time = get_absolute_time();
	uint32_t rot_delta = absolute_time_diff_us(last_hall_event, curr_time);
	uint8_t column = rot_delta * RES_HORIZ / rev_time;

	return column % RES_HORIZ;
}


//----------------------------------------------------------------------------------------------------------------------
bool rtt_rotating(void)
{
	return _rev_time < 100000;
}



/* End of File */

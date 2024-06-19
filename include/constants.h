/* =====================================================================================================================
 *      File:  /include/constants.h
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
#ifndef CONSTANTS_H
#define CONSTANTS_H
// =====================================================================================================================
// Definitions
// ---------------------------------------------------------------------------------------------------------------------
#define LED_COUNT 52

#define RES_VERT (LED_COUNT * 2)
#define RES_HORIZ 120

#define COLUMN_BUFFER_SIZE (LED_COUNT + 2)

#define SERIAL_FREQ (16 * 1000 * 1000)

// Global brightness value 0->31
#define BRIGHTNESS 6

// PIO configuration is hard-coded for this project.
#define ROTATION_TIME 200
#define REFRESH_TIME 5000

#define INACTIVE_COLOR 0x00050505




#endif
/* End of File */

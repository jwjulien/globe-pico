/* =====================================================================================================================
 *      File:  /C:/Users/jared/Desktop/Nextcloud/Projects/Work/Globe/Firmware/include/images.h
 *   Project:  Converter
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
#ifndef IMAGES_H
#define IMAGES_H
// =====================================================================================================================
// Includes
// ---------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>




//======================================================================================================================
// Definitions
//----------------------------------------------------------------------------------------------------------------------
#define REGION_COUNT 16
#define REGION_WIDTH 118
#define REGION_HEIGHT 47
#define REGION_BYTES ((REGION_HEIGHT + 1) / 8)
#define REGION_OFFSET_X 1
#define REGION_OFFSET_Y 24




//======================================================================================================================
// Type Definitions
//----------------------------------------------------------------------------------------------------------------------
typedef uint8_t Region_t[REGION_WIDTH][REGION_BYTES];




//======================================================================================================================
// Globals
//----------------------------------------------------------------------------------------------------------------------
extern const uint32_t ColorMap[];
extern const Region_t * Regions[];




#endif
/* End of File */

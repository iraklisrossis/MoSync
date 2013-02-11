/*
Copyright (C) 2011 MoSync AB

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/

/**
 * @file main.cpp
 * @author Florin Leu
 *
 * This is the main entry point for the example application
 * that demonstrates Sensors on MoSync.
 */

#include <MAUtil/Moblet.h>

#include <conprint.h>

#include "consts.h"

// The list with sensor error codes.
int gSensorError[SENSOR_TYPES + 1];
// Values returned by sensors.
float gSensorValue[SENSOR_TYPES + 1][SENSOR_VALUES];
// Names of the available sensors.
const char *gSensorName[SENSOR_TYPES + 1] =
{
	TXT_NONE,
	TXT_SENSOR_ACCELEROMETER,
	TXT_SENSOR_MAG_FIELD,
	TXT_NONE,
	TXT_SENSOR_GYROSCOPE,
	TXT_SENSOR_PROXIMITY,
	TXT_SENSOR_COMPASS,
};

// Screen dimensions.
int gScreenWidth;
int gScreenHeight;

static int sFontHeight = 0;

/**
 * Get the screen size into the instance variables
 * mScreenWidth and mScreenHeight.
 */
static void updateScreenDimensions()
{
	/// Get screen dimensions.
	MAExtent size = maGetScrSize();

	/// Extract the screen width
	gScreenWidth = EXTENT_X(size);

	/// Extract the screen height
	gScreenHeight = EXTENT_Y(size);
}

/*
 * Get the error message using the sensor error code.
 */
static const char* getErrorText(int errorCode)
{
	switch (errorCode)
	{
		case MA_SENSOR_ERROR_NOT_AVAILABLE:
			return TXT_NOT_AVAILABLE;
		case MA_SENSOR_ERROR_INTERVAL_NOT_SET:
			return TXT_INTERVAL;
		case MA_SENSOR_ERROR_ALREADY_ENABLED:
			return TXT_ENABLED;
		default:
			return TXT_AVAILABLE;
	}
}

/*
 * @brief Register to all sensors and get the error codes
 */
static void registerSensors()
{
	for (int type=1; type<=SENSOR_TYPES; type++)
	{
		gSensorError[type] = maSensorStart(type, MA_SENSOR_RATE_NORMAL);
	}
}

/*
 * @brief Unregister all the sensors
 */
static void unregisterSensors()
{
	for (int type=1; type<=SENSOR_TYPES; type++)
	{
		maSensorStop(type);
	}
}

/*
 * @brief Draw the sensor values.
 * @param index Current sensor index.
 * @param x Coordinate x where to draw the values.
 * @param y Coordinate y where to draw the values.
 */
static void drawSensorValue(int index, int x, int y)
{
	float* values = gSensorValue[index];
	char buffer[BUFFER_SIZE];
	switch (index)
	{
		case MA_SENSOR_TYPE_ACCELEROMETER:
		case MA_SENSOR_TYPE_MAGNETIC_FIELD:
		case MA_SENSOR_TYPE_GYROSCOPE:
			sprintf(buffer, "Values: X:%0.4f; Y:%0.4f; Z:%0.4f",
					values[0], values[1], values[2]);
			break;
		case MA_SENSOR_TYPE_PROXIMITY:
			if (values[0] == MA_SENSOR_PROXIMITY_VALUE_FAR)
				sprintf(buffer, "Value: FAR");
			else if (values[0] == MA_SENSOR_PROXIMITY_VALUE_NEAR)
				sprintf(buffer, "Value: NEAR");
			else
				sprintf(buffer, "Value: %f", values[0]);
			break;
		//case MA_SENSOR_TYPE_COMPASS:
				//sprintf(buffer, "Value: %f", values[0]);
			//break;
		default:
			sprintf(buffer, "Unknown sensor %i\n", index);
	}
	maDrawText(x, y, buffer);
}

/*
 * @brief Draw the sensor status.
 * @param index Current sensor index.
 * @param x Coordinate x where to draw the values.
 * @param y Coordinate y where to draw the values.
 */
static void drawSensorStatus(int index, int x, int y)
{
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%s: %s",
			gSensorName[index], getErrorText(gSensorError[index]));
	maDrawText(x, y, buffer);
}

/*
 * @brief Displays the sensor values
 * or an error message if the sensor cannot register.
 */
static void drawSensorOutput()
{
	//clean the screen
	maSetColor(BG_COLOR);
	maFillRect(0, 0, gScreenWidth, gScreenHeight);

	//set output text color
	maSetColor(TEXT_COLOR);

	if(sFontHeight <= 0) {
		sFontHeight = EXTENT_Y(maGetTextSize("ABCDg"));
	}

	int posY = 0;
	for (int i=1; i<=SENSOR_TYPES; i++)
	{
		if(strcmp(gSensorName[i], TXT_NONE) == 0)
			continue;
		drawSensorStatus(i, 0, posY);
		posY += OFFSET_Y;

		// skip is sensor couldn't register
		if (gSensorError[i] != 0)
			continue;

		drawSensorValue(i, OFFSET_X, posY);
		posY += OFFSET_Y;
	}
}

/*
 * @brief Sets the font
 * to display the sensors values.
 */
static void setFont()
{
	MAHandle defaultFont = maFontLoadDefault(FONT_TYPE_SANS_SERIF, 0, TEXT_SIZE);
	//Check if it's implemented on the current platform.
	if (0 > defaultFont)
	{
		//maPanic(0, "Device fonts is only available on Android and iOS.");
	} else {
		maFontSetCurrent(defaultFont);
	}
}

extern "C" int MAMain()
{
	bool run = true;

	updateScreenDimensions();
	registerSensors();
	setFont();

	// Force output for devices which hasn't got any sensors
	drawSensorOutput();
	maUpdateScreen();

	while (run)
	{
		/// Get any available events.
		/// On back press close program.
		MAEvent event;
		while(maGetEvent(&event))
		{
			if(event.type == EVENT_TYPE_SENSOR)
			{
#if 0
				lprintfln("got SENSOR %i %f %f %f. voff: %i",
					event.sensor.type, event.sensor.values[0], event.sensor.values[1], event.sensor.values[2],
					(char*)event.sensor.values - (char*)&event);
#endif
				memcpy(gSensorValue[event.sensor.type],
						event.sensor.values, SENSOR_VALUES * sizeof(float));
			}
			else if((event.type == EVENT_TYPE_KEY_PRESSED) &&
					(event.key == MAK_BACK))
			{
				run = false;
			}
			else if(event.type == EVENT_TYPE_CLOSE)
			{
				run = false;
			}
		}
		drawSensorOutput();
		maUpdateScreen();
		maWait(-1);
	}

	unregisterSensors();
	return 0;
}

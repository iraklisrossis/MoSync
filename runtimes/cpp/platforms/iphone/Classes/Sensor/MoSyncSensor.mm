/* Copyright (C) 2010 MoSync AB

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License, version 2, as published by
 the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.

 You should have received a copy of the GNU General Public License
 along with this program; see the file COPYING.  If not, write to the Free
 Software Foundation, 59 Temple Place - Suite 330, Boston, MA
 02111-1307, USA.
*/

// sensor update interval values in milliseconds
#define MA_SENSOR_RATE_FASTEST_IOS 50
#define MA_SENSOR_RATE_GAME_IOS 80
#define MA_SENSOR_RATE_NORMAL_IOS 140
#define MA_SENSOR_RATE_UI_IOS 160

// used for converting milliseconds in seconds
#define SECOND 1000.0

#include <helpers/cpp_defs.h>
#include "Platform.h"

#import "MoSyncSensor.h"

@implementation MoSyncSensor

/**
 * Init function.
 */
-(id) init
{
    self = [super init];
    if (self)
    {
        motionManager = [[CMMotionManager alloc] init];
        locationManager = [[CLLocationManager alloc] init];

        dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        motionManagerTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, globalQueue);
        locationManagerTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, globalQueue);
        compassManagerTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, globalQueue);

        isProximitySensorRunning = FALSE;
        isMagnetometerSensorRunning = FALSE;
    }

	return self;
}

/**
 * Start a sensor.
 * @param sensorType What type of sensor to start.
 * @param value Update interval value.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been started, or a code error otherwise.
 */
-(int) startSensor: (int)sensorType  interval:(int)value
{
	int result = MA_SENSOR_ERROR_NONE;
	switch (sensorType)
    {
		case MA_SENSOR_TYPE_ACCELEROMETER:
			result = [self startAccelerometer:value];
			break;
		case MA_SENSOR_TYPE_GYROSCOPE:
			result = [self startGyroscope:value];
			break;
		case MA_SENSOR_TYPE_PROXIMITY:
			result = [self startProximity];
			break;
		case MA_SENSOR_TYPE_MAGNETIC_FIELD:
			result = [self startMagnetometer:value];
			break;
		case MA_SENSOR_TYPE_COMPASS:
			result = [self startCompass:value];
			break;
		default:
			result = MA_SENSOR_ERROR_NOT_AVAILABLE;
	}

	return result;
}

/**
 * Stop a sensor.
 * @param sensorType What sensor to stop.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been stopped, or a code error otherwise.
 */
-(int) stopSensor: (int)sensorType
{
	int result = MA_SENSOR_ERROR_NONE;
	switch (sensorType)
    {
		case MA_SENSOR_TYPE_ACCELEROMETER:
			result = [self stopAccelerometer];
			break;
		case MA_SENSOR_TYPE_GYROSCOPE:
			result = [self stopGyroscope];
			break;
		case MA_SENSOR_TYPE_PROXIMITY:
			result = [self stopProximity];
			break;
		case MA_SENSOR_TYPE_MAGNETIC_FIELD:
			result = [self stopMagnetometer];
			break;
		case MA_SENSOR_TYPE_COMPASS:
			result = [self stopCompass];
			break;
		default:
			result = MA_SENSOR_ERROR_NOT_AVAILABLE;
	}

	return result;
}

/**
 * Start the accelerometer sensor.
 * @param interval Update interval value.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been started, or a code error otherwise.
 */
-(int) startAccelerometer:(int)interval
{
	if(accelerometer)
	{
		return MA_SENSOR_ERROR_ALREADY_ENABLED;
	}

	if(MA_SENSOR_RATE_UI > interval)
	{
		return MA_SENSOR_ERROR_INTERVAL_NOT_SET;
	}

	double intervalInMilliseconds = (double) [self getUpdateIntervalFromRate:interval];
	accelerometer = [UIAccelerometer sharedAccelerometer];

	// set the update interval(the value must be in seconds so we need to convert it from milliseconds).
	accelerometer.updateInterval = intervalInMilliseconds / SECOND;
	accelerometer.delegate = self;

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Stop the accelerometer sensor.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been stopped, or a code error otherwise.
 */
-(int) stopAccelerometer
{
	if(!accelerometer)
	{
		return MA_SENSOR_ERROR_NOT_ENABLED;
	}

	accelerometer.delegate = nil;
	accelerometer = nil;

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Start the gyroscope sensor.
 * @param interval Update interval value.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been started, or a code error otherwise.
 */
-(int) startGyroscope:(int)interval
{
	if([motionManager isGyroActive])
	{
		return MA_SENSOR_ERROR_ALREADY_ENABLED;
	}

	if(MA_SENSOR_RATE_UI > interval)
	{
		return MA_SENSOR_ERROR_INTERVAL_NOT_SET;
	}

	// check if gyroscope is available.
	if([motionManager isGyroAvailable])
    {
        double intervalInMilliseconds = (double) [self getUpdateIntervalFromRate:interval];
        NSTimeInterval updateInterval = intervalInMilliseconds / SECOND ;

        motionManager.gyroUpdateInterval = updateInterval;
        [motionManager startGyroUpdates];

        int repeatTime = (int) intervalInMilliseconds * NSEC_PER_MSEC;
        dispatch_source_set_timer(motionManagerTimer,
                                  dispatch_time(DISPATCH_TIME_NOW, repeatTime),
                                  repeatTime,
                                  0);
        dispatch_source_set_event_handler(motionManagerTimer, ^{
            CMGyroData* gyroData = [motionManager gyroData];
            if (gyroData)
            {
                CMRotationRate rotate = gyroData.rotationRate;
                MAEvent event;
                event.type = EVENT_TYPE_SENSOR;
                event.sensor.type = MA_SENSOR_TYPE_GYROSCOPE;
                event.sensor.values[0] = rotate.x;
                event.sensor.values[1] = rotate.y;
                event.sensor.values[2] = rotate.z;
                Base::gEventQueue.putSafe(event);
            }
        });
        dispatch_resume(motionManagerTimer);
	}
	else
	{
		return MA_SENSOR_ERROR_NOT_AVAILABLE;
	}

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Stop the gyroscope sensor.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been stopped, or a code error otherwise.
 */
-(int) stopGyroscope
{
	if(![motionManager isGyroActive])
	{
		return MA_SENSOR_ERROR_NOT_ENABLED;
	}

    dispatch_suspend(motionManagerTimer);
	[motionManager stopGyroUpdates];

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Start the proximity sensor.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been started, or a code error otherwise.
 */
-(int)startProximity
{
	UIDevice *device = [UIDevice currentDevice];

	if(isProximitySensorRunning)
	{
		return MA_SENSOR_ERROR_ALREADY_ENABLED;
	}

	// start the proximity sensor.
	device.proximityMonitoringEnabled = YES;

	// check if the proximity sensor is available.
	if(YES == device.proximityMonitoringEnabled)
	{
		[[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(proximityChanged)
                                                     name:UIDeviceProximityStateDidChangeNotification
                                                   object:nil];
	}
	else
	{
		return MA_SENSOR_ERROR_NOT_AVAILABLE;
	}

	isProximitySensorRunning = TRUE;
	return MA_SENSOR_ERROR_NONE;
}

/**
 * Stop the proximity sensor.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been stopped, or a code error otherwise.
 */
-(int)stopProximity
{
	UIDevice *device = [UIDevice currentDevice];

	if(isProximitySensorRunning)
	{
		device.proximityMonitoringEnabled = NO;
		[[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:UIDeviceProximityStateDidChangeNotification
                                                      object:nil];
		isProximitySensorRunning = FALSE;
	}
	else
	{
		return MA_SENSOR_ERROR_NOT_ENABLED;
	}

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Start the magnetometer sensor.
 * @param interval How fast to read data(time interval in milliseconds).
 * @return MA_SENSOR_ERROR_NONE if the sensor has been started, or a code error otherwise.
 */
-(int)startMagnetometer:(const int)interval
{
	if(![CLLocationManager headingAvailable])
	{
		return MA_SENSOR_ERROR_NOT_AVAILABLE;
	}

	if(isMagnetometerSensorRunning)
	{
		return MA_SENSOR_ERROR_ALREADY_ENABLED;
	}

    // Start the magnetometer sensor.
    // Location manager does not have an update interval property and it sends too fast data
    // for our current event system.
    // Used a timer that reads location data at a specified interval.
    int intervalInMilliseconds = [self getUpdateIntervalFromRate:interval];
    int repeatTime = intervalInMilliseconds * NSEC_PER_MSEC;
    dispatch_source_set_timer(locationManagerTimer,
                              dispatch_time(DISPATCH_TIME_NOW, repeatTime),
                              repeatTime,
                              0);
    dispatch_source_set_event_handler(locationManagerTimer, ^{
        CLHeading* heading = [locationManager heading];
        if (heading)
        {
            MAEvent event;
            event.type = EVENT_TYPE_SENSOR;
            event.sensor.type = MA_SENSOR_TYPE_MAGNETIC_FIELD;
            event.sensor.values[0] = heading.x;
            event.sensor.values[1] = heading.y;
            event.sensor.values[2] = heading.z;
            Base::gEventQueue.putSafe(event);
        }
    });

    isMagnetometerSensorRunning = TRUE;
    [locationManager startUpdatingHeading];
    dispatch_resume(locationManagerTimer);

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Stop the magnetometer sensor.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been stopped, or a code error otherwise.
 */
-(int)stopMagnetometer
{
	if(isMagnetometerSensorRunning)
	{
		isMagnetometerSensorRunning = FALSE;
		dispatch_suspend(locationManagerTimer);
		if(!isCompassRunning)
		{
			[locationManager stopUpdatingHeading];
		}
	}
	else
	{
		return MA_SENSOR_ERROR_NOT_ENABLED;
	}

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Start the compass.
 * @param interval How fast to read data(time interval in milliseconds).
 * @return MA_SENSOR_ERROR_NONE if the sensor has been started, or a code error otherwise.
 */
-(int)startCompass:(const int)interval
{
	if(![CLLocationManager headingAvailable])
	{
		return MA_SENSOR_ERROR_NOT_AVAILABLE;
	}

	if(isCompassRunning)
	{
		return MA_SENSOR_ERROR_ALREADY_ENABLED;
	}

    // Start the compass sensor.
    // Location manager does not have an update interval property and it sends too fast data
    // for our current event system.
    // Used a timer that reads location data at a specified interval.
    int intervalInMilliseconds = [self getUpdateIntervalFromRate:interval];
    int repeatTime = intervalInMilliseconds * NSEC_PER_MSEC;
    dispatch_source_set_timer(compassManagerTimer,
                              dispatch_time(DISPATCH_TIME_NOW, repeatTime),
                              repeatTime,
                              0);
    dispatch_source_set_event_handler(compassManagerTimer, ^{
        CLHeading* heading = [locationManager heading];
        if (heading)
        {
            MAEvent event;
            event.type = EVENT_TYPE_SENSOR;
            event.sensor.type = MA_SENSOR_TYPE_COMPASS;
            event.sensor.values[0] = heading.magneticHeading;
            Base::gEventQueue.putSafe(event);
        }
    });

    [locationManager startUpdatingHeading];
    isCompassRunning = TRUE;
    dispatch_resume(compassManagerTimer);

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Stop the compass.
 * @return MA_SENSOR_ERROR_NONE if the sensor has been stopped, or a code error otherwise.
 */
-(int)stopCompass
{
	if(isCompassRunning)
	{
		isCompassRunning = FALSE;
		dispatch_suspend(compassManagerTimer);
		if(!isMagnetometerSensorRunning)
		{
			[locationManager stopUpdatingHeading];
		}
	}
	else
	{
		return MA_SENSOR_ERROR_NOT_ENABLED;
	}

	return MA_SENSOR_ERROR_NONE;
}

/**
 * Get the update interval associated with a rate constant.
 * @param rate One of the next constants:
 * - MA_SENSOR_RATE_FASTEST
 * - MA_SENSOR_RATE_GAME
 * - MA_SENSOR_RATE_NORMAL
 * - MA_SENSOR_RATE_UI
 * @return The update interval associated with the rate, or rate parameter
 * if it's not one of the above constants.
 */
-(int) getUpdateIntervalFromRate:(const int) rate
{
	int returnValue = 0;
	switch (rate)
	{
		case MA_SENSOR_RATE_FASTEST:
			returnValue = MA_SENSOR_RATE_FASTEST_IOS;
			break;
		case MA_SENSOR_RATE_GAME:
			returnValue = MA_SENSOR_RATE_GAME_IOS;
			break;
		case MA_SENSOR_RATE_NORMAL:
			returnValue = MA_SENSOR_RATE_NORMAL_IOS;
			break;
		case MA_SENSOR_RATE_UI:
			returnValue = MA_SENSOR_RATE_UI_IOS;
			break;
		default:
			returnValue = rate;
	}

    return returnValue;
}

/**
 * Delivers the latest acceleration data.
 * @param accelerometer The application's accelerometer object.
 * @param acceleration The most recent acceleration data.
 */
- (void) accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration
{
	MAEvent event;
	event.type = EVENT_TYPE_SENSOR;
	event.sensor.type = MA_SENSOR_TYPE_ACCELEROMETER;

	event.sensor.values[0] = acceleration.x;
	event.sensor.values[1] = acceleration.y;
	event.sensor.values[2] = acceleration.z;

	Base::gEventQueue.putSafe(event);
}

/**
 * Delivers the latest data from proximity sensor.
 */
-(void)proximityChanged
{
	UIDevice *device = [UIDevice currentDevice];

	float sensorValue;
	if([device proximityState])
	{
		sensorValue = MA_SENSOR_PROXIMITY_VALUE_NEAR;
	}
	else
	{
		sensorValue = MA_SENSOR_PROXIMITY_VALUE_FAR;
	}

	MAEvent event;
	event.type = EVENT_TYPE_SENSOR;
	event.sensor.type = MA_SENSOR_TYPE_PROXIMITY;
	event.sensor.values[0] = sensorValue;
	Base::gEventQueue.putSafe(event);
}

/**
 * Release all the objects.
 */
- (void) dealloc
{
	// stop all sensors
	[self stopAccelerometer];
	[self stopGyroscope];
	[self stopProximity];
	[self stopMagnetometer];
	[self stopCompass];

	[motionManager release];
	[locationManager release];

	dispatch_source_cancel(motionManagerTimer);
	dispatch_source_cancel(locationManagerTimer);
	dispatch_source_cancel(compassManagerTimer);
	dispatch_release(motionManagerTimer);
	dispatch_release(locationManagerTimer);
	dispatch_release(compassManagerTimer);

	[super dealloc];
}

@end

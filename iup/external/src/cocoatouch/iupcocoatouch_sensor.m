/** \file
 * \brief iOS Sensor (CoreMotion)
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>
#import <CoreMotion/CoreMotion.h>
#import <CoreLocation/CoreLocation.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_sensor.h"

#define IUPCOCOATOUCH_SENSOR_KEY "_IUPCOCOATOUCH_SENSOR"
#define IUPCOCOATOUCH_SENSOR_G -9.80665

@interface IupCocoaTouchSensor : NSObject <CLLocationManagerDelegate>
@property(nonatomic, assign) Ihandle* ih;
@property(nonatomic, retain) CMMotionManager* motion;
@property(nonatomic, retain) NSOperationQueue* queue;
@property(nonatomic, retain) CLLocationManager* heading;
@end

static long long cocoatouchSensorTimestamp(NSTimeInterval uptime)
{
  NSTimeInterval now = [[NSDate date] timeIntervalSince1970];
  NSTimeInterval boot = [[NSProcessInfo processInfo] systemUptime];
  return (long long)((now - (boot - uptime)) * 1000.0);
}

static void cocoatouchSensorPostReading(Ihandle* ih, double x, double y, double z, long long timestamp)
{
  IsensorMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_READING;
  msg.x = x;
  msg.y = y;
  msg.z = z;
  msg.timestamp = timestamp;
  iupSensorPost(ih, &msg);
}

static void cocoatouchSensorPostError(Ihandle* ih, const char* text)
{
  IsensorMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), text);
  iupSensorPost(ih, &msg);
}

@implementation IupCocoaTouchSensor

- (void)dealloc
{
  self.heading.delegate = nil;
  self.heading = nil;
  self.motion = nil;
  self.queue = nil;
  [super dealloc];
}

- (void)locationManager:(CLLocationManager*)manager didUpdateHeading:(CLHeading*)heading
{
  (void)manager;
  cocoatouchSensorPostReading(self.ih, heading.magneticHeading,
                              heading.trueHeading >= 0? heading.trueHeading: -1,
                              heading.headingAccuracy >= 0? heading.headingAccuracy: -1,
                              (long long)([heading.timestamp timeIntervalSince1970] * 1000.0));
}

- (void)locationManager:(CLLocationManager*)manager didFailWithError:(NSError*)error
{
  (void)manager;
  cocoatouchSensorPostError(self.ih, [[error localizedDescription] UTF8String]);
}

@end

static IupCocoaTouchSensor* cocoatouchSensorGet(Ihandle* ih, int create)
{
  IupCocoaTouchSensor* sensor = (IupCocoaTouchSensor*)iupAttribGet(ih, IUPCOCOATOUCH_SENSOR_KEY);
  if (!sensor && create)
  {
    sensor = [[IupCocoaTouchSensor alloc] init];
    sensor.ih = ih;
    sensor.motion = [[[CMMotionManager alloc] init] autorelease];
    sensor.queue = [[[NSOperationQueue alloc] init] autorelease];
    iupAttribSet(ih, IUPCOCOATOUCH_SENSOR_KEY, (char*)sensor);
  }
  return sensor;
}

static int cocoatouchSensorAvailable(CMMotionManager* motion, int type)
{
  switch (type)
  {
  case IUP_SENSOR_ACCELEROMETER: return motion.accelerometerAvailable;
  case IUP_SENSOR_GYROSCOPE: return motion.gyroAvailable;
  case IUP_SENSOR_COMPASS: return [CLLocationManager headingAvailable];
  default: return motion.deviceMotionAvailable;
  }
}

IUP_SDK_API int iupdrvSensorIsAvailable(int type)
{
  CMMotionManager* motion = [[[CMMotionManager alloc] init] autorelease];
  return cocoatouchSensorAvailable(motion, type)? 1: 0;
}

IUP_SDK_API int iupdrvSensorStart(Ihandle* ih)
{
  IupCocoaTouchSensor* sensor = cocoatouchSensorGet(ih, 1);
  CMMotionManager* motion = sensor.motion;
  int type = iupSensorGetType(ih);
  NSTimeInterval interval = iupAttribGetInt(ih, "INTERVAL") / 1000.0;

  if (!cocoatouchSensorAvailable(motion, type))
  {
    cocoatouchSensorPostError(ih, "Sensor not available");
    return 0;
  }

  switch (type)
  {
  case IUP_SENSOR_ACCELEROMETER:
    motion.accelerometerUpdateInterval = interval;
    [motion startAccelerometerUpdatesToQueue:sensor.queue withHandler:^(CMAccelerometerData* data, NSError* error) {
      if (data)
        cocoatouchSensorPostReading(ih, data.acceleration.x * IUPCOCOATOUCH_SENSOR_G, data.acceleration.y * IUPCOCOATOUCH_SENSOR_G,
                                    data.acceleration.z * IUPCOCOATOUCH_SENSOR_G, cocoatouchSensorTimestamp(data.timestamp));
      else if (error)
        cocoatouchSensorPostError(ih, [[error localizedDescription] UTF8String]);
    }];
    break;
  case IUP_SENSOR_GYROSCOPE:
    motion.gyroUpdateInterval = interval;
    [motion startGyroUpdatesToQueue:sensor.queue withHandler:^(CMGyroData* data, NSError* error) {
      if (data)
        cocoatouchSensorPostReading(ih, data.rotationRate.x, data.rotationRate.y, data.rotationRate.z, cocoatouchSensorTimestamp(data.timestamp));
      else if (error)
        cocoatouchSensorPostError(ih, [[error localizedDescription] UTF8String]);
    }];
    break;
  case IUP_SENSOR_COMPASS:
    if (!sensor.heading)
    {
      sensor.heading = [[[CLLocationManager alloc] init] autorelease];
      sensor.heading.delegate = sensor;
      sensor.heading.headingFilter = kCLHeadingFilterNone;
    }
    [sensor.heading startUpdatingHeading];
    break;
  default:
    motion.deviceMotionUpdateInterval = interval;
    [motion startDeviceMotionUpdatesUsingReferenceFrame:CMAttitudeReferenceFrameXMagneticNorthZVertical toQueue:sensor.queue withHandler:^(CMDeviceMotion* data, NSError* error) {
      if (!data)
      {
        if (error)
          cocoatouchSensorPostError(ih, [[error localizedDescription] UTF8String]);
        return;
      }
      long long timestamp = cocoatouchSensorTimestamp(data.timestamp);
      if (type == IUP_SENSOR_GRAVITY)
        cocoatouchSensorPostReading(ih, data.gravity.x * IUPCOCOATOUCH_SENSOR_G, data.gravity.y * IUPCOCOATOUCH_SENSOR_G, data.gravity.z * IUPCOCOATOUCH_SENSOR_G, timestamp);
      else if (type == IUP_SENSOR_LINEARACCELERATION)
        cocoatouchSensorPostReading(ih, data.userAcceleration.x * IUPCOCOATOUCH_SENSOR_G, data.userAcceleration.y * IUPCOCOATOUCH_SENSOR_G, data.userAcceleration.z * IUPCOCOATOUCH_SENSOR_G, timestamp);
      else if (type == IUP_SENSOR_MAGNETOMETER)
        cocoatouchSensorPostReading(ih, data.magneticField.field.x, data.magneticField.field.y, data.magneticField.field.z, timestamp);
      else
      {
        double azimuth = 270.0 - data.attitude.yaw * 180.0 / M_PI;
        while (azimuth < 0) azimuth += 360.0;
        while (azimuth >= 360.0) azimuth -= 360.0;
        cocoatouchSensorPostReading(ih, azimuth, data.attitude.pitch * 180.0 / M_PI, data.attitude.roll * 180.0 / M_PI, timestamp);
      }
    }];
    break;
  }
  return 1;
}

IUP_SDK_API void iupdrvSensorStop(Ihandle* ih)
{
  IupCocoaTouchSensor* sensor = cocoatouchSensorGet(ih, 0);
  if (!sensor)
    return;
  [sensor.motion stopAccelerometerUpdates];
  [sensor.motion stopGyroUpdates];
  [sensor.motion stopDeviceMotionUpdates];
  [sensor.heading stopUpdatingHeading];
}

IUP_SDK_API char* iupdrvSensorGetPermission(Ihandle* ih)
{
  return iupdrvSensorIsAvailable(iupSensorGetType(ih))? "GRANTED": "UNAVAILABLE";
}

IUP_SDK_API void iupdrvSensorDestroy(Ihandle* ih)
{
  IupCocoaTouchSensor* sensor = cocoatouchSensorGet(ih, 0);
  if (!sensor)
    return;
  iupdrvSensorStop(ih);
  [sensor release];
  iupAttribSet(ih, IUPCOCOATOUCH_SENSOR_KEY, NULL);
}

IUP_SDK_API void iupdrvSensorInitClass(Iclass* ic)
{
  (void)ic;
}

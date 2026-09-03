/** \file
 * \brief Location (CoreLocation, macOS and iOS)
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_location.h"

#define IUPCOCOA_LOCATION_KEY "_IUPCOCOA_LOCATION"

@interface IupCocoaLocation : NSObject <CLLocationManagerDelegate>
@property(nonatomic, assign) Ihandle* ih;
@property(nonatomic, retain) CLLocationManager* manager;
@property(nonatomic, assign) BOOL wantUpdates;
@property(nonatomic, assign) BOOL answered;
@end

static void cocoaLocationPostError(Ihandle* ih, const char* text)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), text);
  iupLocationPost(ih, &msg);
}

static void cocoaLocationPostPermission(Ihandle* ih, int granted)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_PERMISSION;
  msg.granted = granted;
  iupLocationPost(ih, &msg);
}

static int cocoaLocationAuthorized(CLAuthorizationStatus status)
{
#if TARGET_OS_IPHONE
  return status == kCLAuthorizationStatusAuthorizedWhenInUse || status == kCLAuthorizationStatusAuthorizedAlways;
#else
  return status == kCLAuthorizationStatusAuthorizedAlways;
#endif
}

@implementation IupCocoaLocation

- (void)dealloc
{
  self.manager.delegate = nil;
  self.manager = nil;
  [super dealloc];
}

- (void)locationManagerDidChangeAuthorization:(CLLocationManager*)manager
{
  CLAuthorizationStatus status = manager.authorizationStatus;

  if (status == kCLAuthorizationStatusNotDetermined || !self.wantUpdates)
    return;

  if (!self.answered)
  {
    self.answered = YES;
    cocoaLocationPostPermission(self.ih, cocoaLocationAuthorized(status));
  }

  if (cocoaLocationAuthorized(status))
    [manager startUpdatingLocation];
  else
    cocoaLocationPostError(self.ih, "Location access denied");
}

- (void)locationManager:(CLLocationManager*)manager didUpdateLocations:(NSArray<CLLocation*>*)locations
{
  CLLocation* location = [locations lastObject];
  IlocationMsg msg;
  (void)manager;

  if (!location)
    return;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_FIX;
  msg.latitude = location.coordinate.latitude;
  msg.longitude = location.coordinate.longitude;
  msg.accuracy = location.horizontalAccuracy;
  msg.altitude = location.altitude;
  msg.has_altitude = location.verticalAccuracy >= 0;
  msg.speed = location.speed;
  msg.has_speed = location.speed >= 0;
  msg.heading = location.course;
  msg.has_heading = location.course >= 0;
  msg.timestamp = (long long)([location.timestamp timeIntervalSince1970] * 1000.0);
  iupLocationPost(self.ih, &msg);
}

- (void)locationManager:(CLLocationManager*)manager didFailWithError:(NSError*)error
{
  (void)manager;
  if (error.code == kCLErrorDenied)
    cocoaLocationPostError(self.ih, "Location access denied");
  else if (error.code == kCLErrorLocationUnknown)
    cocoaLocationPostError(self.ih, "Location unknown");
  else
    cocoaLocationPostError(self.ih, [[error localizedDescription] UTF8String]);
}

@end

static IupCocoaLocation* cocoaLocationGet(Ihandle* ih, int create)
{
  IupCocoaLocation* loc = (IupCocoaLocation*)iupAttribGet(ih, IUPCOCOA_LOCATION_KEY);
  if (!loc && create)
  {
    loc = [[IupCocoaLocation alloc] init];
    loc.ih = ih;
    loc.manager = [[[CLLocationManager alloc] init] autorelease];
    loc.manager.delegate = loc;
    iupAttribSet(ih, IUPCOCOA_LOCATION_KEY, (char*)loc);
  }
  return loc;
}

IUP_SDK_API int iupdrvLocationIsAvailable(void)
{
  return [CLLocationManager locationServicesEnabled]? 1: 0;
}

IUP_SDK_API int iupdrvLocationStart(Ihandle* ih)
{
  IupCocoaLocation* loc = cocoaLocationGet(ih, 1);
  CLAuthorizationStatus status;

  if (![CLLocationManager locationServicesEnabled])
  {
    cocoaLocationPostError(ih, "Location services disabled");
    return 0;
  }

  loc.manager.desiredAccuracy = iupStrEqualNoCase(iupAttribGetStr(ih, "ACCURACY"), "FINE")? kCLLocationAccuracyBest: kCLLocationAccuracyHundredMeters;
  loc.manager.distanceFilter = iupAttribGetDouble(ih, "DISTANCE") > 0? iupAttribGetDouble(ih, "DISTANCE"): kCLDistanceFilterNone;
  loc.wantUpdates = YES;

  status = loc.manager.authorizationStatus;
  if (status == kCLAuthorizationStatusNotDetermined)
  {
    [loc.manager requestWhenInUseAuthorization];
    return 1;
  }

  if (!cocoaLocationAuthorized(status))
  {
    loc.wantUpdates = NO;
    cocoaLocationPostPermission(ih, 0);
    cocoaLocationPostError(ih, "Location access denied");
    return 0;
  }

  [loc.manager startUpdatingLocation];
  return 1;
}

IUP_SDK_API void iupdrvLocationStop(Ihandle* ih)
{
  IupCocoaLocation* loc = cocoaLocationGet(ih, 0);
  if (!loc)
    return;
  loc.wantUpdates = NO;
  [loc.manager stopUpdatingLocation];
}

IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih)
{
  IupCocoaLocation* loc = cocoaLocationGet(ih, 1);
  CLAuthorizationStatus status = loc.manager.authorizationStatus;

  if (![CLLocationManager locationServicesEnabled])
    return "UNAVAILABLE";
  if (cocoaLocationAuthorized(status))
    return "GRANTED";
  if (status == kCLAuthorizationStatusNotDetermined)
    return "PROMPT";
  return "DENIED";
}

IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih)
{
  IupCocoaLocation* loc = cocoaLocationGet(ih, 0);
  if (!loc)
    return;
  [loc.manager stopUpdatingLocation];
  [loc release];
  iupAttribSet(ih, IUPCOCOA_LOCATION_KEY, NULL);
}

IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic)
{
  (void)ic;
}

/** \file
 * \brief WinUI Location (Windows.Devices.Geolocation)
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Geolocation.h>

#include <cstring>
#include <cmath>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_location.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Geolocation;

struct IupWinUILocation
{
  Geolocator locator{ nullptr };
  event_token token{};
  int permission = 0;
};

#define IUPWINUI_LOCATION_KEY "_IUPWINUI_LOCATION"

static void winuiLocationPostError(Ihandle* ih, const char* text)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), text);
  iupLocationPost(ih, &msg);
}

static void winuiLocationPostPermission(Ihandle* ih, int granted)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_PERMISSION;
  msg.granted = granted;
  iupLocationPost(ih, &msg);
}

static void winuiLocationPostFix(Ihandle* ih, Geoposition const& pos)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_FIX;

  auto point = pos.Coordinate().Point().Position();
  msg.latitude = point.Latitude;
  msg.longitude = point.Longitude;
  msg.accuracy = pos.Coordinate().Accuracy();
  msg.altitude = point.Altitude;
  msg.has_altitude = pos.Coordinate().AltitudeAccuracy() != nullptr && !std::isnan(point.Altitude);
  auto speed = pos.Coordinate().Speed();
  msg.has_speed = speed != nullptr && !std::isnan(speed.Value());
  msg.speed = msg.has_speed? speed.Value(): 0;
  auto heading = pos.Coordinate().Heading();
  msg.has_heading = heading != nullptr && !std::isnan(heading.Value());
  msg.heading = msg.has_heading? heading.Value(): 0;
  msg.timestamp = (long long)(winrt::clock::to_time_t(pos.Coordinate().Timestamp())) * 1000;
  iupLocationPost(ih, &msg);
}

static IupWinUILocation* winuiLocationGet(Ihandle* ih, int create)
{
  IupWinUILocation* loc = (IupWinUILocation*)iupAttribGet(ih, IUPWINUI_LOCATION_KEY);
  if (!loc && create)
  {
    loc = new IupWinUILocation();
    iupAttribSet(ih, IUPWINUI_LOCATION_KEY, (char*)loc);
  }
  return loc;
}

extern "C" IUP_SDK_API int iupdrvLocationIsAvailable(void)
{
  try
  {
    Geolocator locator;
    return locator.LocationStatus() != PositionStatus::NotAvailable? 1: 0;
  }
  catch (...) { return 0; }
}

extern "C" IUP_SDK_API int iupdrvLocationStart(Ihandle* ih)
{
  IupWinUILocation* loc = winuiLocationGet(ih, 1);
  int fine = iupStrEqualNoCase(iupAttribGetStr(ih, "ACCURACY"), "FINE");

  try
  {
    loc->locator = Geolocator();
    loc->locator.DesiredAccuracy(fine? PositionAccuracy::High: PositionAccuracy::Default);
    loc->locator.ReportInterval((uint32_t)iupAttribGetInt(ih, "INTERVAL"));
    loc->locator.MovementThreshold(iupAttribGetDouble(ih, "DISTANCE"));

    loc->token = loc->locator.PositionChanged([ih](Geolocator const&, PositionChangedEventArgs const& args) {
      winuiLocationPostFix(ih, args.Position());
    });

    Geolocator::RequestAccessAsync().Completed([ih, loc](IAsyncOperation<GeolocationAccessStatus> const& op, AsyncStatus status) {
      GeolocationAccessStatus access = status == AsyncStatus::Completed? op.GetResults(): GeolocationAccessStatus::Unspecified;
      if (access != GeolocationAccessStatus::Allowed)
      {
        loc->permission = 2;
        winuiLocationPostPermission(ih, 0);
        winuiLocationPostError(ih, access == GeolocationAccessStatus::Denied? "Location access denied": "Location not available");
        return;
      }
      if (loc->permission != 1)
      {
        loc->permission = 1;
        winuiLocationPostPermission(ih, 1);
      }
      if (loc->locator)
        loc->locator.GetGeopositionAsync().Completed([ih](IAsyncOperation<Geoposition> const& gop, AsyncStatus gstatus) {
          if (gstatus == AsyncStatus::Completed)
            winuiLocationPostFix(ih, gop.GetResults());
        });
    });
    return 1;
  }
  catch (hresult_error const& e)
  {
    winuiLocationPostError(ih, winrt::to_string(e.message()).c_str());
    loc->locator = nullptr;
    return 0;
  }
}

extern "C" IUP_SDK_API void iupdrvLocationStop(Ihandle* ih)
{
  IupWinUILocation* loc = winuiLocationGet(ih, 0);
  if (!loc || !loc->locator)
    return;
  loc->locator.PositionChanged(loc->token);
  loc->locator = nullptr;
}

extern "C" IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih)
{
  IupWinUILocation* loc = winuiLocationGet(ih, 0);
  if (!iupdrvLocationIsAvailable()) return (char*)"UNAVAILABLE";
  if (loc && loc->permission == 1) return (char*)"GRANTED";
  if (loc && loc->permission == 2) return (char*)"DENIED";
  return (char*)"PROMPT";
}

extern "C" IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih)
{
  IupWinUILocation* loc = winuiLocationGet(ih, 0);
  if (!loc)
    return;
  iupdrvLocationStop(ih);
  delete loc;
  iupAttribSet(ih, IUPWINUI_LOCATION_KEY, NULL);
}

extern "C" IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic)
{
  (void)ic;
}

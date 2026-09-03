/** \file
 * \brief Win32 Location (Windows Location API)
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <initguid.h>
#include <locationapi.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_location.h"

#define IUPWIN_LOCATION_KEY "_IUPWIN_LOCATION"

typedef struct _IupWinLocation
{
  ILocationEvents events;
  LONG refcount;
  Ihandle* ih;
  ILocation* location;
  int registered;
  int permission;
} IupWinLocation;

static void winLocationPostError(Ihandle* ih, const char* text)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), text);
  iupLocationPost(ih, &msg);
}

static void winLocationPostPermission(Ihandle* ih, int granted)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_PERMISSION;
  msg.granted = granted;
  iupLocationPost(ih, &msg);
}

static HRESULT STDMETHODCALLTYPE winLocationQueryInterface(ILocationEvents* This, REFIID riid, void** ppv)
{
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ILocationEvents))
  {
    *ppv = This;
    This->lpVtbl->AddRef(This);
    return S_OK;
  }
  *ppv = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE winLocationAddRef(ILocationEvents* This)
{
  return (ULONG)InterlockedIncrement(&((IupWinLocation*)This)->refcount);
}

static ULONG STDMETHODCALLTYPE winLocationRelease(ILocationEvents* This)
{
  IupWinLocation* loc = (IupWinLocation*)This;
  LONG count = InterlockedDecrement(&loc->refcount);
  if (count == 0)
    free(loc);
  return (ULONG)count;
}

static HRESULT STDMETHODCALLTYPE winLocationOnLocationChanged(ILocationEvents* This, REFIID reportType, ILocationReport* report)
{
  IupWinLocation* loc = (IupWinLocation*)This;
  ILatLongReport* latlong = NULL;
  IlocationMsg msg;
  SYSTEMTIME st;
  FILETIME ft;
  ULARGE_INTEGER time;

  if (!IsEqualIID(reportType, &IID_ILatLongReport) || !report)
    return S_OK;
  if (FAILED(report->lpVtbl->QueryInterface(report, &IID_ILatLongReport, (void**)&latlong)) || !latlong)
    return S_OK;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_FIX;
  ILatLongReport_GetLatitude(latlong, &msg.latitude);
  ILatLongReport_GetLongitude(latlong, &msg.longitude);
  ILatLongReport_GetErrorRadius(latlong, &msg.accuracy);
  {
    DOUBLE altitude_error;
    msg.has_altitude = SUCCEEDED(ILatLongReport_GetAltitude(latlong, &msg.altitude)) &&
                       SUCCEEDED(ILatLongReport_GetAltitudeError(latlong, &altitude_error));
  }

  if (SUCCEEDED(ILatLongReport_GetTimestamp(latlong, &st)) && SystemTimeToFileTime(&st, &ft))
  {
    time.LowPart = ft.dwLowDateTime;
    time.HighPart = ft.dwHighDateTime;
    msg.timestamp = (long long)(time.QuadPart / 10000ULL) - 11644473600000LL;
  }

  latlong->lpVtbl->Release(latlong);
  iupLocationPost(loc->ih, &msg);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winLocationOnStatusChanged(ILocationEvents* This, REFIID reportType, LOCATION_REPORT_STATUS status)
{
  IupWinLocation* loc = (IupWinLocation*)This;

  if (!IsEqualIID(reportType, &IID_ILatLongReport))
    return S_OK;

  if (status == REPORT_ACCESS_DENIED)
  {
    loc->permission = 2;
    winLocationPostPermission(loc->ih, 0);
    winLocationPostError(loc->ih, "Location access denied");
  }
  else if (status == REPORT_RUNNING && loc->permission != 1)
  {
    loc->permission = 1;
    winLocationPostPermission(loc->ih, 1);
  }
  else if (status == REPORT_NOT_SUPPORTED || status == REPORT_ERROR)
    winLocationPostError(loc->ih, status == REPORT_ERROR? "Location report error": "Location not supported");

  return S_OK;
}

static ILocationEventsVtbl winLocationEventsVtbl = {
  winLocationQueryInterface,
  winLocationAddRef,
  winLocationRelease,
  winLocationOnLocationChanged,
  winLocationOnStatusChanged
};

static IupWinLocation* winLocationGet(Ihandle* ih, int create)
{
  IupWinLocation* loc = (IupWinLocation*)iupAttribGet(ih, IUPWIN_LOCATION_KEY);
  if (!loc && create)
  {
    loc = (IupWinLocation*)calloc(1, sizeof(IupWinLocation));
    if (!loc)
      return NULL;
    loc->events.lpVtbl = &winLocationEventsVtbl;
    loc->refcount = 1;
    loc->ih = ih;
    iupAttribSet(ih, IUPWIN_LOCATION_KEY, (char*)loc);
  }
  return loc;
}

static ILocation* winLocationCreate(void)
{
  ILocation* location = NULL;
  if (FAILED(CoCreateInstance(&CLSID_Location, NULL, CLSCTX_INPROC_SERVER, &IID_ILocation, (void**)&location)))
    return NULL;
  return location;
}

IUP_SDK_API int iupdrvLocationIsAvailable(void)
{
  ILocation* location = winLocationCreate();
  if (!location)
    return 0;
  location->lpVtbl->Release(location);
  return 1;
}

IUP_SDK_API int iupdrvLocationStart(Ihandle* ih)
{
  IupWinLocation* loc = winLocationGet(ih, 1);
  IID report = IID_ILatLongReport;
  int fine;
  HRESULT hr;

  if (!loc)
    return 0;

  if (!loc->location)
  {
    loc->location = winLocationCreate();
    if (!loc->location)
    {
      winLocationPostError(ih, "Location service not available");
      return 0;
    }
  }

  fine = iupStrEqualNoCase(iupAttribGetStr(ih, "ACCURACY"), "FINE");
  loc->location->lpVtbl->SetDesiredAccuracy(loc->location, &IID_ILatLongReport, fine? LOCATION_DESIRED_ACCURACY_HIGH: LOCATION_DESIRED_ACCURACY_DEFAULT);

  hr = loc->location->lpVtbl->RequestPermissions(loc->location, NULL, &report, 1, TRUE);
  if (FAILED(hr))
  {
    loc->permission = 2;
    winLocationPostPermission(ih, 0);
    winLocationPostError(ih, "Location access denied");
    return 0;
  }

  hr = loc->location->lpVtbl->RegisterForReport(loc->location, &loc->events, &IID_ILatLongReport, (DWORD)iupAttribGetInt(ih, "INTERVAL"));
  if (FAILED(hr))
  {
    LOCATION_REPORT_STATUS status = REPORT_ERROR;
    loc->location->lpVtbl->GetReportStatus(loc->location, &IID_ILatLongReport, &status);
    if (status == REPORT_ACCESS_DENIED || hr == E_ACCESSDENIED)
    {
      loc->permission = 2;
      winLocationPostPermission(ih, 0);
      winLocationPostError(ih, "Location access denied");
    }
    else
      winLocationPostError(ih, "Location report registration failed");
    return 0;
  }
  loc->registered = 1;

  {
    ILocationReport* current = NULL;
    if (SUCCEEDED(loc->location->lpVtbl->GetReport(loc->location, &IID_ILatLongReport, &current)) && current)
    {
      winLocationOnLocationChanged(&loc->events, &IID_ILatLongReport, current);
      current->lpVtbl->Release(current);
    }
  }
  return 1;
}

IUP_SDK_API void iupdrvLocationStop(Ihandle* ih)
{
  IupWinLocation* loc = winLocationGet(ih, 0);
  if (!loc || !loc->location || !loc->registered)
    return;
  loc->location->lpVtbl->UnregisterForReport(loc->location, &IID_ILatLongReport);
  loc->registered = 0;
}

IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih)
{
  IupWinLocation* loc = winLocationGet(ih, 0);
  if (loc && loc->permission == 1) return "GRANTED";
  if (loc && loc->permission == 2) return "DENIED";
  if (!iupdrvLocationIsAvailable()) return "UNAVAILABLE";
  return "PROMPT";
}

IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih)
{
  IupWinLocation* loc = winLocationGet(ih, 0);
  if (!loc)
    return;
  iupdrvLocationStop(ih);
  if (loc->location)
  {
    loc->location->lpVtbl->Release(loc->location);
    loc->location = NULL;
  }
  iupAttribSet(ih, IUPWIN_LOCATION_KEY, NULL);
  loc->events.lpVtbl->Release(&loc->events);
}

IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic)
{
  (void)ic;
}

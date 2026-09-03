/** \file
 * \brief Win32 Sensor (Windows Sensor API)
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <initguid.h>
#include <sensorsapi.h>
#include <sensors.h>
#include <portabledevicetypes.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_sensor.h"

#define IUPWIN_SENSOR_KEY "_IUPWIN_SENSOR"
#define IUPWIN_SENSOR_G 9.80665
#define IUPWIN_SENSOR_DEG2RAD 0.017453292519943295

typedef struct _IupWinSensor
{
  ISensorEvents events;
  LONG refcount;
  Ihandle* ih;
  ISensor* sensor;
  int type;
} IupWinSensor;

static void winSensorPostError(Ihandle* ih, const char* text)
{
  IsensorMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), text);
  iupSensorPost(ih, &msg);
}

static const GUID* winSensorTypeGuid(int type)
{
  switch (type)
  {
  case IUP_SENSOR_ACCELEROMETER: return &SENSOR_TYPE_ACCELEROMETER_3D;
  case IUP_SENSOR_GYROSCOPE: return &SENSOR_TYPE_GYROMETER_3D;
  case IUP_SENSOR_MAGNETOMETER:
  case IUP_SENSOR_COMPASS: return &SENSOR_TYPE_COMPASS_3D;
  case IUP_SENSOR_ORIENTATION: return &SENSOR_TYPE_INCLINOMETER_3D;
  default: return NULL;
  }
}

static double winSensorValue(ISensorDataReport* report, REFPROPERTYKEY key, double scale, int* ok)
{
  PROPVARIANT value;
  double result = 0;
  PropVariantInit(&value);
  if (SUCCEEDED(report->lpVtbl->GetSensorValue(report, key, &value)) && (value.vt == VT_R8 || value.vt == VT_R4))
  {
    result = (value.vt == VT_R8? value.dblVal: value.fltVal) * scale;
    if (ok) *ok = 1;
  }
  else if (ok)
    *ok = 0;
  PropVariantClear(&value);
  return result;
}

static void winSensorReport(IupWinSensor* sensor, ISensorDataReport* report)
{
  IsensorMsg msg;
  SYSTEMTIME st;
  FILETIME ft;
  ULARGE_INTEGER time;
  int ok;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_READING;

  switch (sensor->type)
  {
  case IUP_SENSOR_ACCELEROMETER:
    msg.x = winSensorValue(report, &SENSOR_DATA_TYPE_ACCELERATION_X_G, IUPWIN_SENSOR_G, NULL);
    msg.y = winSensorValue(report, &SENSOR_DATA_TYPE_ACCELERATION_Y_G, IUPWIN_SENSOR_G, NULL);
    msg.z = winSensorValue(report, &SENSOR_DATA_TYPE_ACCELERATION_Z_G, IUPWIN_SENSOR_G, NULL);
    break;
  case IUP_SENSOR_GYROSCOPE:
    msg.x = winSensorValue(report, &SENSOR_DATA_TYPE_ANGULAR_VELOCITY_X_DEGREES_PER_SECOND, IUPWIN_SENSOR_DEG2RAD, NULL);
    msg.y = winSensorValue(report, &SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Y_DEGREES_PER_SECOND, IUPWIN_SENSOR_DEG2RAD, NULL);
    msg.z = winSensorValue(report, &SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Z_DEGREES_PER_SECOND, IUPWIN_SENSOR_DEG2RAD, NULL);
    break;
  case IUP_SENSOR_MAGNETOMETER:
    msg.x = winSensorValue(report, &SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_X_MILLIGAUSS, 0.1, NULL);
    msg.y = winSensorValue(report, &SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_Y_MILLIGAUSS, 0.1, NULL);
    msg.z = winSensorValue(report, &SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_Z_MILLIGAUSS, 0.1, NULL);
    break;
  case IUP_SENSOR_COMPASS:
    msg.x = winSensorValue(report, &SENSOR_DATA_TYPE_MAGNETIC_HEADING_COMPENSATED_MAGNETIC_NORTH_DEGREES, 1, NULL);
    msg.y = winSensorValue(report, &SENSOR_DATA_TYPE_MAGNETIC_HEADING_COMPENSATED_TRUE_NORTH_DEGREES, 1, &ok);
    if (!ok) msg.y = -1;
    msg.z = -1;
    break;
  case IUP_SENSOR_ORIENTATION:
    msg.x = winSensorValue(report, &SENSOR_DATA_TYPE_TILT_Z_DEGREES, 1, NULL);
    msg.y = winSensorValue(report, &SENSOR_DATA_TYPE_TILT_X_DEGREES, 1, NULL);
    msg.z = winSensorValue(report, &SENSOR_DATA_TYPE_TILT_Y_DEGREES, 1, NULL);
    break;
  }

  if (SUCCEEDED(report->lpVtbl->GetTimestamp(report, &st)) && SystemTimeToFileTime(&st, &ft))
  {
    time.LowPart = ft.dwLowDateTime;
    time.HighPart = ft.dwHighDateTime;
    msg.timestamp = (long long)(time.QuadPart / 10000ULL) - 11644473600000LL;
  }

  iupSensorPost(sensor->ih, &msg);
}

static HRESULT STDMETHODCALLTYPE winSensorQueryInterface(ISensorEvents* This, REFIID riid, void** ppv)
{
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ISensorEvents))
  {
    *ppv = This;
    This->lpVtbl->AddRef(This);
    return S_OK;
  }
  *ppv = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE winSensorAddRef(ISensorEvents* This)
{
  return (ULONG)InterlockedIncrement(&((IupWinSensor*)This)->refcount);
}

static ULONG STDMETHODCALLTYPE winSensorRelease(ISensorEvents* This)
{
  IupWinSensor* sensor = (IupWinSensor*)This;
  LONG count = InterlockedDecrement(&sensor->refcount);
  if (count == 0)
    free(sensor);
  return (ULONG)count;
}

static HRESULT STDMETHODCALLTYPE winSensorOnStateChanged(ISensorEvents* This, ISensor* pSensor, SensorState state)
{
  IupWinSensor* sensor = (IupWinSensor*)This;
  (void)pSensor;
  if (state == SENSOR_STATE_ACCESS_DENIED)
    winSensorPostError(sensor->ih, "Sensor access denied");
  else if (state == SENSOR_STATE_ERROR || state == SENSOR_STATE_NOT_AVAILABLE)
    winSensorPostError(sensor->ih, state == SENSOR_STATE_ERROR? "Sensor error": "Sensor not available");
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winSensorOnDataUpdated(ISensorEvents* This, ISensor* pSensor, ISensorDataReport* pNewData)
{
  (void)pSensor;
  if (pNewData)
    winSensorReport((IupWinSensor*)This, pNewData);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winSensorOnEvent(ISensorEvents* This, ISensor* pSensor, REFGUID eventID, IPortableDeviceValues* pEventData)
{
  (void)This; (void)pSensor; (void)eventID; (void)pEventData;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winSensorOnLeave(ISensorEvents* This, REFSENSOR_ID ID)
{
  IupWinSensor* sensor = (IupWinSensor*)This;
  (void)ID;
  winSensorPostError(sensor->ih, "Sensor removed");
  return S_OK;
}

static ISensorEventsVtbl winSensorEventsVtbl = {
  winSensorQueryInterface,
  winSensorAddRef,
  winSensorRelease,
  winSensorOnStateChanged,
  winSensorOnDataUpdated,
  winSensorOnEvent,
  winSensorOnLeave
};

static IupWinSensor* winSensorGet(Ihandle* ih, int create)
{
  IupWinSensor* sensor = (IupWinSensor*)iupAttribGet(ih, IUPWIN_SENSOR_KEY);
  if (!sensor && create)
  {
    sensor = (IupWinSensor*)calloc(1, sizeof(IupWinSensor));
    if (!sensor)
      return NULL;
    sensor->events.lpVtbl = &winSensorEventsVtbl;
    sensor->refcount = 1;
    sensor->ih = ih;
    iupAttribSet(ih, IUPWIN_SENSOR_KEY, (char*)sensor);
  }
  return sensor;
}

static ISensor* winSensorFind(int type, int request)
{
  ISensorManager* manager = NULL;
  ISensorCollection* collection = NULL;
  ISensor* sensor = NULL;
  const GUID* guid = winSensorTypeGuid(type);
  ULONG count = 0;

  if (!guid)
    return NULL;
  if (FAILED(CoCreateInstance(&CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, &IID_ISensorManager, (void**)&manager)))
    return NULL;

  if (SUCCEEDED(manager->lpVtbl->GetSensorsByType(manager, guid, &collection)) && collection)
  {
    if (SUCCEEDED(collection->lpVtbl->GetCount(collection, &count)) && count > 0)
    {
      if (!request || SUCCEEDED(manager->lpVtbl->RequestPermissions(manager, NULL, collection, TRUE)))
        collection->lpVtbl->GetAt(collection, 0, &sensor);
    }
    collection->lpVtbl->Release(collection);
  }
  manager->lpVtbl->Release(manager);
  return sensor;
}

static void winSensorSetInterval(ISensor* sensor, ULONG interval)
{
  IPortableDeviceValues* values = NULL;
  IPortableDeviceValues* results = NULL;

  if (FAILED(CoCreateInstance(&CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER, &IID_IPortableDeviceValues, (void**)&values)))
    return;
  values->lpVtbl->SetUnsignedIntegerValue(values, &SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, interval);
  if (SUCCEEDED(sensor->lpVtbl->SetProperties(sensor, values, &results)) && results)
    results->lpVtbl->Release(results);
  values->lpVtbl->Release(values);
}

IUP_SDK_API int iupdrvSensorIsAvailable(int type)
{
  ISensor* sensor = winSensorFind(type, 0);
  if (!sensor)
    return 0;
  sensor->lpVtbl->Release(sensor);
  return 1;
}

IUP_SDK_API int iupdrvSensorStart(Ihandle* ih)
{
  IupWinSensor* sensor = winSensorGet(ih, 1);
  ISensorDataReport* report = NULL;

  if (!sensor)
    return 0;

  sensor->type = iupSensorGetType(ih);
  sensor->sensor = winSensorFind(sensor->type, 1);
  if (!sensor->sensor)
  {
    winSensorPostError(ih, "Sensor not available");
    return 0;
  }

  winSensorSetInterval(sensor->sensor, (ULONG)iupAttribGetInt(ih, "INTERVAL"));

  if (FAILED(sensor->sensor->lpVtbl->SetEventSink(sensor->sensor, &sensor->events)))
  {
    sensor->sensor->lpVtbl->Release(sensor->sensor);
    sensor->sensor = NULL;
    winSensorPostError(ih, "Sensor event registration failed");
    return 0;
  }

  if (SUCCEEDED(sensor->sensor->lpVtbl->GetData(sensor->sensor, &report)) && report)
  {
    winSensorReport(sensor, report);
    report->lpVtbl->Release(report);
  }
  return 1;
}

IUP_SDK_API void iupdrvSensorStop(Ihandle* ih)
{
  IupWinSensor* sensor = winSensorGet(ih, 0);
  if (!sensor || !sensor->sensor)
    return;
  sensor->sensor->lpVtbl->SetEventSink(sensor->sensor, NULL);
  sensor->sensor->lpVtbl->Release(sensor->sensor);
  sensor->sensor = NULL;
}

IUP_SDK_API char* iupdrvSensorGetPermission(Ihandle* ih)
{
  return iupdrvSensorIsAvailable(iupSensorGetType(ih))? "GRANTED": "UNAVAILABLE";
}

IUP_SDK_API void iupdrvSensorDestroy(Ihandle* ih)
{
  IupWinSensor* sensor = winSensorGet(ih, 0);
  if (!sensor)
    return;
  iupdrvSensorStop(ih);
  iupAttribSet(ih, IUPWIN_SENSOR_KEY, NULL);
  sensor->events.lpVtbl->Release(&sensor->events);
}

IUP_SDK_API void iupdrvSensorInitClass(Iclass* ic)
{
  (void)ic;
}

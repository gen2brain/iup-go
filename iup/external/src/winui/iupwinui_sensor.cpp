/** \file
 * \brief WinUI Sensor (Windows.Devices.Sensors)
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Sensors.h>

#include <cstring>
#include <cmath>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_sensor.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Sensors;

#define IUPWINUI_SENSOR_KEY "_IUPWINUI_SENSOR"
#define IUPWINUI_SENSOR_G 9.80665
#define IUPWINUI_SENSOR_DEG2RAD 0.017453292519943295

struct IupWinUISensor
{
  IInspectable device{ nullptr };
  event_token token{};
};

static void winuiSensorPostReading(Ihandle* ih, double x, double y, double z, DateTime timestamp)
{
  IsensorMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_READING;
  msg.x = x;
  msg.y = y;
  msg.z = z;
  msg.timestamp = (long long)winrt::clock::to_time_t(timestamp) * 1000;
  iupSensorPost(ih, &msg);
}

static void winuiSensorPostError(Ihandle* ih, const char* text)
{
  IsensorMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), text);
  iupSensorPost(ih, &msg);
}

static IupWinUISensor* winuiSensorGet(Ihandle* ih, int create)
{
  IupWinUISensor* sensor = (IupWinUISensor*)iupAttribGet(ih, IUPWINUI_SENSOR_KEY);
  if (!sensor && create)
  {
    sensor = new IupWinUISensor();
    iupAttribSet(ih, IUPWINUI_SENSOR_KEY, (char*)sensor);
  }
  return sensor;
}

static IInspectable winuiSensorDefault(int type)
{
  switch (type)
  {
  case IUP_SENSOR_GYROSCOPE: return Gyrometer::GetDefault();
  case IUP_SENSOR_MAGNETOMETER: return Magnetometer::GetDefault();
  case IUP_SENSOR_GRAVITY: return Accelerometer::GetDefault(AccelerometerReadingType::Gravity);
  case IUP_SENSOR_LINEARACCELERATION: return Accelerometer::GetDefault(AccelerometerReadingType::Linear);
  case IUP_SENSOR_ORIENTATION: return Inclinometer::GetDefault(SensorReadingType::Absolute);
  case IUP_SENSOR_COMPASS: return Compass::GetDefault();
  default: return Accelerometer::GetDefault();
  }
}

static double winuiSensorCompassAccuracy(MagnetometerAccuracy accuracy)
{
  switch (accuracy)
  {
  case MagnetometerAccuracy::High: return 5;
  case MagnetometerAccuracy::Approximate: return 15;
  case MagnetometerAccuracy::Unreliable: return 45;
  default: return -1;
  }
}

extern "C" IUP_SDK_API int iupdrvSensorIsAvailable(int type)
{
  try
  {
    return winuiSensorDefault(type)? 1: 0;
  }
  catch (...) { return 0; }
}

extern "C" IUP_SDK_API int iupdrvSensorStart(Ihandle* ih)
{
  IupWinUISensor* sensor = winuiSensorGet(ih, 1);
  int type = iupSensorGetType(ih);
  uint32_t interval = (uint32_t)iupAttribGetInt(ih, "INTERVAL");

  try
  {
    IInspectable device = winuiSensorDefault(type);
    if (!device)
    {
      winuiSensorPostError(ih, "Sensor not available");
      return 0;
    }
    sensor->device = device;

    switch (type)
    {
    case IUP_SENSOR_GYROSCOPE:
    {
      Gyrometer gyro = device.as<Gyrometer>();
      gyro.ReportInterval(interval < gyro.MinimumReportInterval()? gyro.MinimumReportInterval(): interval);
      sensor->token = gyro.ReadingChanged([ih](Gyrometer const&, GyrometerReadingChangedEventArgs const& args) {
        auto r = args.Reading();
        winuiSensorPostReading(ih, r.AngularVelocityX() * IUPWINUI_SENSOR_DEG2RAD, r.AngularVelocityY() * IUPWINUI_SENSOR_DEG2RAD, r.AngularVelocityZ() * IUPWINUI_SENSOR_DEG2RAD, r.Timestamp());
      });
      break;
    }
    case IUP_SENSOR_MAGNETOMETER:
    {
      Magnetometer mag = device.as<Magnetometer>();
      mag.ReportInterval(interval < mag.MinimumReportInterval()? mag.MinimumReportInterval(): interval);
      sensor->token = mag.ReadingChanged([ih](Magnetometer const&, MagnetometerReadingChangedEventArgs const& args) {
        auto r = args.Reading();
        winuiSensorPostReading(ih, r.MagneticFieldX(), r.MagneticFieldY(), r.MagneticFieldZ(), r.Timestamp());
      });
      break;
    }
    case IUP_SENSOR_ORIENTATION:
    {
      Inclinometer incl = device.as<Inclinometer>();
      incl.ReportInterval(interval < incl.MinimumReportInterval()? incl.MinimumReportInterval(): interval);
      sensor->token = incl.ReadingChanged([ih](Inclinometer const&, InclinometerReadingChangedEventArgs const& args) {
        auto r = args.Reading();
        winuiSensorPostReading(ih, r.YawDegrees(), r.PitchDegrees(), r.RollDegrees(), r.Timestamp());
      });
      break;
    }
    case IUP_SENSOR_COMPASS:
    {
      Compass compass = device.as<Compass>();
      compass.ReportInterval(interval < compass.MinimumReportInterval()? compass.MinimumReportInterval(): interval);
      sensor->token = compass.ReadingChanged([ih](Compass const&, CompassReadingChangedEventArgs const& args) {
        auto r = args.Reading();
        auto true_north = r.HeadingTrueNorth();
        winuiSensorPostReading(ih, r.HeadingMagneticNorth(), true_north? true_north.Value(): -1, winuiSensorCompassAccuracy(r.HeadingAccuracy()), r.Timestamp());
      });
      break;
    }
    default:
    {
      Accelerometer accel = device.as<Accelerometer>();
      accel.ReportInterval(interval < accel.MinimumReportInterval()? accel.MinimumReportInterval(): interval);
      sensor->token = accel.ReadingChanged([ih](Accelerometer const&, AccelerometerReadingChangedEventArgs const& args) {
        auto r = args.Reading();
        winuiSensorPostReading(ih, r.AccelerationX() * IUPWINUI_SENSOR_G, r.AccelerationY() * IUPWINUI_SENSOR_G, r.AccelerationZ() * IUPWINUI_SENSOR_G, r.Timestamp());
      });
      break;
    }
    }
    return 1;
  }
  catch (hresult_error const& e)
  {
    winuiSensorPostError(ih, winrt::to_string(e.message()).c_str());
    sensor->device = nullptr;
    return 0;
  }
}

extern "C" IUP_SDK_API void iupdrvSensorStop(Ihandle* ih)
{
  IupWinUISensor* sensor = winuiSensorGet(ih, 0);
  if (!sensor || !sensor->device)
    return;
  if (auto accel = sensor->device.try_as<Accelerometer>()) { accel.ReadingChanged(sensor->token); accel.ReportInterval(0); }
  else if (auto gyro = sensor->device.try_as<Gyrometer>()) { gyro.ReadingChanged(sensor->token); gyro.ReportInterval(0); }
  else if (auto mag = sensor->device.try_as<Magnetometer>()) { mag.ReadingChanged(sensor->token); mag.ReportInterval(0); }
  else if (auto incl = sensor->device.try_as<Inclinometer>()) { incl.ReadingChanged(sensor->token); incl.ReportInterval(0); }
  else if (auto compass = sensor->device.try_as<Compass>()) { compass.ReadingChanged(sensor->token); compass.ReportInterval(0); }
  sensor->device = nullptr;
}

extern "C" IUP_SDK_API char* iupdrvSensorGetPermission(Ihandle* ih)
{
  return iupdrvSensorIsAvailable(iupSensorGetType(ih))? (char*)"GRANTED": (char*)"UNAVAILABLE";
}

extern "C" IUP_SDK_API void iupdrvSensorDestroy(Ihandle* ih)
{
  IupWinUISensor* sensor = winuiSensorGet(ih, 0);
  if (!sensor)
    return;
  iupdrvSensorStop(ih);
  delete sensor;
  iupAttribSet(ih, IUPWINUI_SENSOR_KEY, NULL);
}

extern "C" IUP_SDK_API void iupdrvSensorInitClass(Iclass* ic)
{
  (void)ic;
}

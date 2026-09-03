/** \file
 * \brief Android Location JNI bridge
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_location.h"

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupLocationHelper_dispatchFix(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr,
  jdouble latitude, jdouble longitude, jdouble altitude, jboolean has_altitude, jfloat accuracy,
  jfloat speed, jboolean has_speed, jfloat bearing, jboolean has_bearing, jlong time_ms)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  IlocationMsg msg;
  (void)jni_env; (void)cls;

  if (!ih || !iupObjectCheck(ih)) return;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_FIX;
  msg.latitude = latitude;
  msg.longitude = longitude;
  msg.altitude = altitude;
  msg.has_altitude = has_altitude? 1: 0;
  msg.accuracy = accuracy;
  msg.speed = speed;
  msg.has_speed = has_speed? 1: 0;
  msg.heading = bearing;
  msg.has_heading = has_bearing? 1: 0;
  msg.timestamp = (long long)time_ms;
  iupLocationPost(ih, &msg);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupLocationHelper_dispatchPermission(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jboolean granted)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  IlocationMsg msg;
  (void)jni_env; (void)cls;

  if (!ih || !iupObjectCheck(ih)) return;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_PERMISSION;
  msg.granted = granted? 1: 0;
  iupLocationPost(ih, &msg);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupLocationHelper_dispatchError(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring text)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  IlocationMsg msg;
  const char* utf;
  (void)cls;

  if (!ih || !iupObjectCheck(ih)) return;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_ERROR;
  utf = text? (*jni_env)->GetStringUTFChars(jni_env, text, NULL): NULL;
  iupStrCopyN(msg.error, sizeof(msg.error), utf? utf: "Location error");
  if (utf) (*jni_env)->ReleaseStringUTFChars(jni_env, text, utf);
  iupLocationPost(ih, &msg);
}

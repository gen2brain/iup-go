/** \file
 * \brief Android Sensor JNI bridge
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_sensor.h"

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupSensorHelper_dispatchReading(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr,
  jdouble x, jdouble y, jdouble z, jlong time_ms)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  IsensorMsg msg;
  (void)jni_env; (void)cls;

  if (!ih || !iupObjectCheck(ih)) return;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_READING;
  msg.x = x;
  msg.y = y;
  msg.z = z;
  msg.timestamp = (long long)time_ms;
  iupSensorPost(ih, &msg);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupSensorHelper_dispatchError(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring text)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  IsensorMsg msg;
  const char* utf;
  (void)cls;

  if (!ih || !iupObjectCheck(ih)) return;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_ERROR;
  utf = text? (*jni_env)->GetStringUTFChars(jni_env, text, NULL): NULL;
  iupStrCopyN(msg.error, sizeof(msg.error), utf? utf: "Sensor error");
  if (utf) (*jni_env)->ReleaseStringUTFChars(jni_env, text, utf);
  iupSensorPost(ih, &msg);
}

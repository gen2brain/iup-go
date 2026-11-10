/** \file
 * \brief Audio device, miniaudio AAudio backend with the outputs listed through IupVideoHelper
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <jni.h>

#include "iup.h"
#include "iup_str.h"
#include "iup_miniaudio.h"
#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"

IUPJNI_DECLARE_CLASS_STATIC(IupVideoHelper);

static ma_context iandroid_audio_context;
static int iandroid_audio_context_ready = 0;

ma_device* iupdrvAudioDeviceInit(ma_engine* engine)
{
  (void)engine;
  return NULL;
}

void iupdrvAudioDeviceRelease(void)
{
  if (iandroid_audio_context_ready)
  {
    ma_context_uninit(&iandroid_audio_context);
    iandroid_audio_context_ready = 0;
  }
}

ma_context* iupdrvAudioContext(void)
{
  if (!iandroid_audio_context_ready)
  {
    ma_backend backends[] = { ma_backend_aaudio };
    if (ma_context_init(backends, 1, NULL, &iandroid_audio_context) != MA_SUCCESS)
      return NULL;
    iandroid_audio_context_ready = 1;
  }
  return &iandroid_audio_context;
}

int iupdrvAudioDeviceList(ma_device_info** devices)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_audioDevices);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_audioDevices, env, cls, "audioDevices", "()[Ljava/lang/String;");
  jobjectArray values = (jobjectArray)(*env)->CallStaticObjectMethod(env, cls, method);
  int i, total, count = 0;

  *devices = NULL;
  iupAndroid_CheckException(env, "IupVideoHelper.audioDevices");

  if (values && iupdrvAudioContext())
  {
    total = (int)(*env)->GetArrayLength(env, values);
    if (total >= 2)
      *devices = (ma_device_info*)calloc((size_t)(total / 2), sizeof(ma_device_info));

    for (i = 0; *devices && i + 1 < total; i += 2)
    {
      jstring id = (jstring)(*env)->GetObjectArrayElement(env, values, i);
      jstring name = (jstring)(*env)->GetObjectArrayElement(env, values, i + 1);
      const char* id_text = id ? (*env)->GetStringUTFChars(env, id, NULL) : NULL;
      const char* name_text = name ? (*env)->GetStringUTFChars(env, name, NULL) : NULL;
      int device_id;

      if (id_text && iupStrToInt(id_text, &device_id))
      {
        ma_device_info* info = &(*devices)[count++];
        info->id.aaudio = (ma_int32)device_id;
        if (name_text)
          iupStrCopyN(info->name, (int)sizeof(info->name), name_text);
      }

      if (id_text)
        (*env)->ReleaseStringUTFChars(env, id, id_text);
      if (name_text)
        (*env)->ReleaseStringUTFChars(env, name, name_text);
      if (id)
        (*env)->DeleteLocalRef(env, id);
      if (name)
        (*env)->DeleteLocalRef(env, name);
    }

    if (*devices && count == 0)
    {
      free(*devices);
      *devices = NULL;
    }
  }

  if (values)
    (*env)->DeleteLocalRef(env, values);
  (*env)->DeleteLocalRef(env, cls);
  return count;
}

/** \file
 * \brief Video control, MediaPlayer playback
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_video.h"
#include "iup_str.h"
#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"

IUPJNI_DECLARE_CLASS_STATIC(IupVideoHelper);

typedef struct _IandroidVideo
{
  jobject player;
} IandroidVideo;

static int androidVideoAvailable(void)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  iupAndroid_CheckException(env, "IupVideoHelper");
  if (!cls)
    return 0;
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static void androidVideoAudioDevice(IvideoSession* session, const char* id)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_audioDevice);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_audioDevice, env, cls, "audioDevice", "(Ljava/lang/Object;I)V");
  int device = 0;
  if (id)
    iupStrToInt(id, &device);
  (*env)->CallStaticVoidMethod(env, cls, method, context->player, (jint)device);
  iupAndroid_CheckException(env, "IupVideoHelper.audioDevice");
  (*env)->DeleteLocalRef(env, cls);
}

static int androidVideoAudioDevices(IvideoAudioDevice** devices)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_audioDevices);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_audioDevices, env, cls, "audioDevices", "()[Ljava/lang/String;");
  jobjectArray values = (jobjectArray)(*env)->CallStaticObjectMethod(env, cls, method);
  int i, total, count = 0;
  iupAndroid_CheckException(env, "IupVideoHelper.audioDevices");
  if (values)
  {
    total = (int)(*env)->GetArrayLength(env, values);
    for (i = 0; i + 1 < total; i += 2)
    {
      jstring id = (jstring)(*env)->GetObjectArrayElement(env, values, i);
      jstring name = (jstring)(*env)->GetObjectArrayElement(env, values, i + 1);
      const char* id_text = id ? (*env)->GetStringUTFChars(env, id, NULL) : NULL;
      const char* name_text = name ? (*env)->GetStringUTFChars(env, name, NULL) : NULL;
      if (id_text)
        count = iupVideoAddAudioDevice(devices, count, id_text, name_text);
      if (id_text)
        (*env)->ReleaseStringUTFChars(env, id, id_text);
      if (name_text)
        (*env)->ReleaseStringUTFChars(env, name, name_text);
      if (id)
        (*env)->DeleteLocalRef(env, id);
      if (name)
        (*env)->DeleteLocalRef(env, name);
    }
    (*env)->DeleteLocalRef(env, values);
  }
  (*env)->DeleteLocalRef(env, cls);
  return count;
}

static int androidVideoOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_create);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  IandroidVideo* context = (IandroidVideo*)calloc(1, sizeof(IandroidVideo));
  jstring text;
  jobject player;
  jmethodID method;
  if (!context || !cls)
  {
    iupAndroid_CheckException(env, "IupVideoHelper");
    if (cls)
      (*env)->DeleteLocalRef(env, cls);
    free(context);
    return 0;
  }
  session->context = context;
  method = IUPJNI_GetStaticMethodID(IupVideoHelper_create, env, cls, "create", "(Landroid/view/View;JLjava/lang/String;Z)Ljava/lang/Object;");
  text = iupAndroid_NewStringUTF(env, source);
  if (!text)
  {
    iupAndroid_CheckException(env, "IupVideoHelper source");
    (*env)->DeleteLocalRef(env, cls);
    return 0;
  }
  player = (*env)->CallStaticObjectMethod(env, cls, method, ih->handle, (jlong)(intptr_t)session, text, (jboolean)url);
  iupAndroid_CheckException(env, "IupVideoHelper.create");
  if (player)
  {
    context->player = (*env)->NewGlobalRef(env, player);
    (*env)->DeleteLocalRef(env, player);
  }
  (*env)->DeleteLocalRef(env, text);
  (*env)->DeleteLocalRef(env, cls);
  if (!context->player)
  {
    iupVideoError(session, "Cannot create Android video player");
    return 0;
  }
  if (session->audio_device)
    androidVideoAudioDevice(session, session->audio_device);
  return 1;
}

static void androidVideoClose(IvideoSession* session)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_close);
  IandroidVideo* context = (IandroidVideo*)session->context;
  if (!context)
    return;
  if (context->player)
  {
    JNIEnv* env = iupAndroid_GetEnvThreadSafe();
    jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
    jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_close, env, cls, "close", "(Ljava/lang/Object;)V");
    (*env)->CallStaticVoidMethod(env, cls, method, context->player);
    iupAndroid_CheckException(env, "IupVideoHelper.close");
    (*env)->DeleteGlobalRef(env, context->player);
    (*env)->DeleteLocalRef(env, cls);
  }
  free(context);
  session->context = NULL;
}

static void androidVideoPlay(IvideoSession* session, int play)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_play);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_play, env, cls, "play", "(Ljava/lang/Object;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, method, context->player, (jboolean)play);
  iupAndroid_CheckException(env, "IupVideoHelper.play");
  (*env)->DeleteLocalRef(env, cls);
}

static void androidVideoSeek(IvideoSession* session, double position, int fast)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_seek);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_seek, env, cls, "seek", "(Ljava/lang/Object;DZ)V");
  (*env)->CallStaticVoidMethod(env, cls, method, context->player, position, (jboolean)(fast != 0));
  iupAndroid_CheckException(env, "IupVideoHelper.seek");
  (*env)->DeleteLocalRef(env, cls);
}

static void androidVideoVolume(IvideoSession* session, int volume, int mute)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_volume);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_volume, env, cls, "volume", "(Ljava/lang/Object;IZ)V");
  (*env)->CallStaticVoidMethod(env, cls, method, context->player, volume, (jboolean)mute);
  iupAndroid_CheckException(env, "IupVideoHelper.volume");
  (*env)->DeleteLocalRef(env, cls);
}

static void androidVideoLayout(IvideoSession* session, Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_layout);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_layout, env, cls, "layout", "(Ljava/lang/Object;)V");
  (void)ih;
  (*env)->CallStaticVoidMethod(env, cls, method, context->player);
  iupAndroid_CheckException(env, "IupVideoHelper.layout");
  (*env)->DeleteLocalRef(env, cls);
}

static void androidVideoPoll(IvideoSession* session)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_poll);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_poll, env, cls, "poll", "(Ljava/lang/Object;)[D");
  jdoubleArray values = (jdoubleArray)(*env)->CallStaticObjectMethod(env, cls, method, context->player);
  iupAndroid_CheckException(env, "IupVideoHelper.poll");
  if (values && (*env)->GetArrayLength(env, values) == 10)
  {
    jdouble data[10];
    IvideoInfo info;
    (*env)->GetDoubleArrayRegion(env, values, 0, 10, data);
    memset(&info, 0, sizeof(info));
    info.state = (int)data[0];
    info.ready = (int)data[1];
    info.ended = (int)data[2];
    info.position = data[3];
    info.duration = data[4];
    info.seekable = (int)data[5];
    info.video = (int)data[6];
    info.audio = (int)data[7];
    info.width = (int)data[8];
    info.height = (int)data[9];
    info.aspect = info.height > 0 ? (double)info.width / info.height : 0;
    iupVideoInfo(session, &info);
  }
  if (values)
    (*env)->DeleteLocalRef(env, values);
  (*env)->DeleteLocalRef(env, cls);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupVideoHelper_error(JNIEnv* env, jclass cls, jlong pointer, jstring message)
{
  IvideoSession* session = (IvideoSession*)(intptr_t)pointer;
  const char* text;
  (void)cls;
  if (!session)
    return;
  text = message ? (*env)->GetStringUTFChars(env, message, NULL) : NULL;
  iupVideoError(session, text ? text : "Android video playback failed");
  if (text)
    (*env)->ReleaseStringUTFChars(env, message, text);
}

static int androidVideoSubtitleCount(IvideoSession* session)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_subtitleCount);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_subtitleCount, env, cls, "subtitleCount", "(Ljava/lang/Object;)I");
  int count = (*env)->CallStaticIntMethod(env, cls, method, context->player);
  iupAndroid_CheckException(env, "IupVideoHelper.subtitleCount");
  (*env)->DeleteLocalRef(env, cls);
  return count;
}

static char* androidVideoSubtitleName(IvideoSession* session, int index, int language)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_subtitleName);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_subtitleName, env, cls, "subtitleName", "(Ljava/lang/Object;IZ)Ljava/lang/String;");
  jstring value = (jstring)(*env)->CallStaticObjectMethod(env, cls, method, context->player, (jint)index, (jboolean)language);
  char* result = NULL;
  iupAndroid_CheckException(env, "IupVideoHelper.subtitleName");
  if (value)
  {
    const char* text = (*env)->GetStringUTFChars(env, value, NULL);
    if (text)
    {
      result = iupStrReturnStr(text);
      (*env)->ReleaseStringUTFChars(env, value, text);
    }
    (*env)->DeleteLocalRef(env, value);
  }
  (*env)->DeleteLocalRef(env, cls);
  return result;
}

static int androidVideoSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_subtitleSelect);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_subtitleSelect, env, cls, "subtitleSelect", "(Ljava/lang/Object;ID)Z");
  int result = (*env)->CallStaticBooleanMethod(env, cls, method, context->player, (jint)index, (jdouble)delay);
  iupAndroid_CheckException(env, "IupVideoHelper.subtitleSelect");
  (*env)->DeleteLocalRef(env, cls);
  return result;
}

static int androidVideoSubtitleText(IvideoSession* session, const char* text, int size, const char* color)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_subtitleText);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_subtitleText, env, cls, "subtitleText", "(Ljava/lang/Object;Ljava/lang/String;II)V");
  jstring value = iupAndroid_NewStringUTF(env, text ? text : "");
  unsigned char r = 255, g = 255, b = 255;
  iupStrToRGB(color, &r, &g, &b);
  if (value)
  {
    (*env)->CallStaticVoidMethod(env, cls, method, context->player, value, (jint)size,
      (jint)(0xff000000u | ((unsigned)r << 16) | ((unsigned)g << 8) | b));
    (*env)->DeleteLocalRef(env, value);
  }
  iupAndroid_CheckException(env, "IupVideoHelper.subtitleText");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupVideoHelper_subtitleCue(JNIEnv* env, jclass cls, jlong pointer, jbyteArray value)
{
  IvideoSession* session = (IvideoSession*)(intptr_t)pointer;
  char* text = NULL;
  (void)cls;
  if (!session)
    return;
  if (value)
  {
    jsize length = (*env)->GetArrayLength(env, value);
    text = (char*)malloc((size_t)length + 1);
    if (text)
    {
      (*env)->GetByteArrayRegion(env, value, 0, length, (jbyte*)text);
      text[length] = 0;
    }
  }
  iupVideoSubtitleCue(session, text);
  free(text);
}

void iupdrvVideoInhibitScreenSaver(int inhibit)
{
  (void)inhibit;
}

char* iupdrvVideoReadSubtitle(const char* filename)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_readSubtitle);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_readSubtitle, env, cls, "readSubtitle", "(Ljava/lang/String;)[B");
  jstring name = iupAndroid_NewStringUTF(env, filename);
  jbyteArray value = name ? (jbyteArray)(*env)->CallStaticObjectMethod(env, cls, method, name) : NULL;
  char* text = NULL;
  iupAndroid_CheckException(env, "IupVideoHelper.readSubtitle");
  if (value)
  {
    jsize length = (*env)->GetArrayLength(env, value);
    if (length <= 16 * 1024 * 1024)
    {
      text = (char*)malloc((size_t)length + 1);
      if (text)
      {
        (*env)->GetByteArrayRegion(env, value, 0, length, (jbyte*)text);
        text[length] = 0;
        if (memchr(text, 0, (size_t)length))
        {
          free(text);
          text = NULL;
        }
      }
    }
    (*env)->DeleteLocalRef(env, value);
  }
  if (name)
    (*env)->DeleteLocalRef(env, name);
  (*env)->DeleteLocalRef(env, cls);
  return text;
}

static const IvideoBackend androidVideoBackend = {"NATIVE", androidVideoAvailable, androidVideoOpen, androidVideoClose, androidVideoPlay, androidVideoSeek, androidVideoVolume, androidVideoPoll, androidVideoLayout, androidVideoSubtitleCount, androidVideoSubtitleName, androidVideoSubtitleSelect, androidVideoSubtitleText, 0, androidVideoAudioDevices, androidVideoAudioDevice, NULL};

const IvideoBackend* iupdrvVideoBackend(int index)
{
  return index == 0 ? &androidVideoBackend : NULL;
}

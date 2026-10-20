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

static void androidVideoSeek(IvideoSession* session, double position)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupVideoHelper_seek);
  IandroidVideo* context = (IandroidVideo*)session->context;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupVideoHelper, env, "io/github/gen2brain/iupgo/IupVideoHelper");
  jmethodID method = IUPJNI_GetStaticMethodID(IupVideoHelper_seek, env, cls, "seek", "(Ljava/lang/Object;D)V");
  (*env)->CallStaticVoidMethod(env, cls, method, context->player, position);
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

static const IvideoBackend androidVideoBackend = {"NATIVE", androidVideoAvailable, androidVideoOpen, androidVideoClose, androidVideoPlay, androidVideoSeek, androidVideoVolume, androidVideoPoll, androidVideoLayout};

const IvideoBackend* iupdrvVideoBackend(int index)
{
  return index == 0 ? &androidVideoBackend : NULL;
}

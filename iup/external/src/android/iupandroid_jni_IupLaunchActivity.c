/** \file
 * \brief JNI bridge: IupLaunchActivity (Java -> C process entry point).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stddef.h>
#include <dlfcn.h>
#include <unistd.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"
#include "iupcbs.h"

#include "iupandroid_drv.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupLaunchActivity_IupEntry(JNIEnv* jni_env, jobject thiz, jobject current_activity, jstring j_entry_function_name, jstring j_library_name)
{
  (void)thiz;
  (void)current_activity;

  /* Serialised: Go's package init may be racing this on another thread. */
  iupAndroid_OpenOnce();

  iupAndroid_BeforeEntry();

  /* Poll ENTRY_POINT for up to ~500ms: cold start races Go c-shared init. */
  IFentry entry_callback = (IFentry)IupGetFunction("ENTRY_POINT");
  for (int i = 0; entry_callback == NULL && i < 50; i++)
  {
    usleep(10000);
    entry_callback = (IFentry)IupGetFunction("ENTRY_POINT");
  }

  if (entry_callback == NULL && j_entry_function_name != NULL)
  {
    const char* c_entry_function_name = (*jni_env)->GetStringUTFChars(jni_env, j_entry_function_name, NULL);
    entry_callback = (IFentry)dlsym(RTLD_DEFAULT, c_entry_function_name);
    (*jni_env)->ReleaseStringUTFChars(jni_env, j_entry_function_name, c_entry_function_name);
  }

  if (entry_callback == NULL)
  {
    entry_callback = (IFentry)dlsym(RTLD_DEFAULT, "IupEntryPoint");

    /* Fallback: Android 7.0+ hardening can blank RTLD_DEFAULT for our own symbols. */
    if (entry_callback == NULL && j_library_name != NULL)
    {
      const char* c_entry_library_name = (*jni_env)->GetStringUTFChars(jni_env, j_library_name, NULL);
      void* dl_handle = dlopen(c_entry_library_name, RTLD_LOCAL | RTLD_NOW);
      entry_callback = (IFentry)dlsym(dl_handle, "IupEntryPoint");
      (*jni_env)->ReleaseStringUTFChars(jni_env, j_library_name, c_entry_library_name);
    }
  }

  if (entry_callback != NULL)
    entry_callback();
  else
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "IupLaunchActivity: no entry point found");
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupLaunchActivity_doPause(JNIEnv* env, jobject thiz)
{
  (void)env;
  (void)thiz;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupLaunchActivity_doResume(JNIEnv* env, jobject thiz)
{
  (void)env;
  (void)thiz;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupLaunchActivity_doDestroy(JNIEnv* env, jobject thiz)
{
  (void)env;
  (void)thiz;
}

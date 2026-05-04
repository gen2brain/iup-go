/** \file
 * \brief Android Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


/* Java owns the loop; MainLoop is a no-op, LoopStep/Flush drain the main Looper. */

IUPJNI_DECLARE_CLASS_STATIC(IupDialogHelper);
IUPJNI_DECLARE_CLASS_STATIC(IupIdleHelper);
IUPJNI_DECLARE_CLASS_EXTERN(IupCommon);

static IFidle android_idle_cb = NULL;


static void androidIdleSet(int enable)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupIdleHelper_setIdle);
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupIdleHelper, jni_env, "io/github/gen2brain/iupgo/IupIdleHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupIdleHelper_setIdle, jni_env, cls, "setIdle", "(Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jboolean)(enable ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, "IupIdleHelper.setIdle");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

void iupdrvSetIdleFunction(Icallback f)
{
  IFidle prev = android_idle_cb;
  android_idle_cb = (IFidle)f;
  if (android_idle_cb && !prev)
    androidIdleSet(1);
  else if (!android_idle_cb && prev)
    androidIdleSet(0);
}

int iupAndroid_DispatchIdle(void)
{
  IFidle cb = android_idle_cb;
  if (!cb) return 0;
  int ret = cb();
  if (ret == IUP_CLOSE)
  {
    android_idle_cb = NULL;
    IupExitLoop();
    return 0;
  }
  if (ret == IUP_IGNORE)
  {
    android_idle_cb = NULL;
    return 0;
  }
  return 1;
}

void IupExitLoop(void)
{
  /* IUP_CLOSE from a callback should finish only the top Activity, not every dialog. */
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jobject current = iupAndroid_GetCurrentActivity(jni_env);
  if (!current) return;

  jclass activity_cls = (*jni_env)->GetObjectClass(jni_env, current);
  jmethodID finish_mid = (*jni_env)->GetMethodID(jni_env, activity_cls, "finish", "()V");
  (*jni_env)->CallVoidMethod(jni_env, current, finish_mid);
  iupAndroid_CheckException(jni_env, "Activity.finish");

  (*jni_env)->DeleteLocalRef(jni_env, activity_cls);
  (*jni_env)->DeleteLocalRef(jni_env, current);
}

static int s_modal_pump_level = 0;
static int s_entry_finished = 0;

/* Process is reused across launches; clear sticky flags so each entry starts fresh. */
void iupAndroid_BeforeEntry(void)
{
  s_entry_finished = 0;
  s_modal_pump_level = 0;
}

int IupMainLoopLevel(void)
{
  return s_modal_pump_level;
}

/* first call from user main() returns immediately (Activity owns the looper); nested call from iup.Popup pumps until onDestroy */
int IupMainLoop(void)
{
  if (!s_entry_finished)
  {
    s_entry_finished = 1;
    return IUP_NOERROR;
  }

  s_modal_pump_level++;
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupCommon_modalPumpRun);
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID m = IUPJNI_GetStaticMethodID(IupCommon_modalPumpRun, jni_env, cls, "modalPumpRun", "()V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m);
  iupAndroid_CheckException(jni_env, "IupCommon.modalPumpRun");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  s_modal_pump_level--;
  return IUP_NOERROR;
}

/* Pumps the main Looper until currently-queued messages are dispatched. */
static void androidLoopFlush(void)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "loopStepFlush", "()V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupCommon.loopStepFlush");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

int IupLoopStepWait(void)
{
  androidLoopFlush();
  return IUP_DEFAULT;
}

int IupLoopStep(void)
{
  androidLoopFlush();
  return IUP_DEFAULT;
}

void IupFlush(void)
{
  androidLoopFlush();
}

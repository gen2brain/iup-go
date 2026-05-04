/** \file
 * \brief JNI bridge: IupActivity (Java -> C callbacks for activity lifecycle).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"

#include "iupandroid_drv.h"


/* Pings Java refreshTheme() on custom-drawn classes (table/tree/frame). */
static void androidThemeCallJavaRefresh(Ihandle* c)
{
  if (!c || !c->handle || !c->iclass || !c->iclass->name) return;

  const char* class_name = c->iclass->name;
  const char* helper_class = NULL;
  if (iupStrEqual(class_name, "table")) helper_class = "io/github/gen2brain/iupgo/IupTableHelper";
  else if (iupStrEqual(class_name, "tree"))  helper_class = "io/github/gen2brain/iupgo/IupTreeHelper";
  else if (iupStrEqual(class_name, "frame")) helper_class = "io/github/gen2brain/iupgo/IupAndroidFrame";

  if (!helper_class) return;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = (*env)->FindClass(env, helper_class);
  if (!cls) { if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env); return; }

  if (iupStrEqual(class_name, "frame"))
  {
    jmethodID m = (*env)->GetMethodID(env, cls, "refreshTheme", "()V");
    if (m) (*env)->CallVoidMethod(env, (jobject)c->handle, m);
  }
  else
  {
    jmethodID m = (*env)->GetStaticMethodID(env, cls, "refreshTheme", "(Landroid/view/View;)V");
    if (m) (*env)->CallStaticVoidMethod(env, cls, m, (jobject)c->handle);
  }
  iupAndroid_CheckException(env, "refreshTheme");
  (*env)->DeleteLocalRef(env, cls);
}

/* Re-resolves inherited BGCOLOR/FGCOLOR on every mapped descendant (Qt-style walk). */
static void androidThemeRefreshChildren(Ihandle* ih)
{
  for (Ihandle* c = ih; c; c = c->brother)
  {
    if (c->handle)
    {
      /* custom-drawn widgets refresh their palette via refreshTheme; their Id2 BGCOLOR setter would misbehave on NULL */
      const char* class_name = (c->iclass && c->iclass->name) ? c->iclass->name : "";
      int has_custom_refresh =
        iupStrEqual(class_name, "table") ||
        iupStrEqual(class_name, "tree")  ||
        iupStrEqual(class_name, "frame");

      if (!has_custom_refresh)
      {
        int inherit;
        if (!iupAttribGet(c, "BGCOLOR"))
          iupClassObjectSetAttribute(c, "BGCOLOR", NULL, &inherit);
        if (!iupAttribGet(c, "FGCOLOR"))
          iupClassObjectSetAttribute(c, "FGCOLOR", NULL, &inherit);
      }

      androidThemeCallJavaRefresh(c);
    }

    if (c->firstchild) androidThemeRefreshChildren(c->firstchild);
  }
}


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupActivity_OnActivityResult(JNIEnv* jni_env, jobject thiz, jlong ihandle_ptr, jint request_code, jint result_code, jobject intent_data)
{
  (void)jni_env;
  (void)thiz;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  IFniiv callback_function = (IFniiv)IupGetCallback(ih, "ONACTIVITYRESULT_CB");
  if (callback_function)
    callback_function(ih, request_code, result_code, intent_data);
}

/* Re-applies Dialog attributes deferred during map (handle was a ViewGroup). */
JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupActivity_DialogActivityCreated(JNIEnv* jni_env, jobject thiz, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)thiz;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  iupAndroid_DialogActivityCreated(ih);
}

/* clear the defer flag so user's later IupDestroy completes; iup.Popup readback needs the Ihandle alive */
JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupActivity_FinalizeDialogDestroy(JNIEnv* jni_env, jobject thiz, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)thiz;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  iupAttribSet(ih, "_IUP_DIALOG_DEFER_DESTROY", NULL);
}

/* Dark-mode flip: refresh color defaults + THEMECHANGED_CB (called from onConfigurationChanged). */
JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupActivity_NotifyThemeChanged(JNIEnv* jni_env, jobject thiz, jlong ihandle_ptr, jint dark_mode)
{
  (void)jni_env;
  (void)thiz;

  iupAndroidUpdateGlobalColors();

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  /* Mirrors iupqt_dialog.cpp qtDialogRefreshThemeColors. */
  int inherit;
  if (!iupAttribGet(ih, "BGCOLOR"))
    iupClassObjectSetAttribute(ih, "BGCOLOR", NULL, &inherit);
  if (!iupAttribGet(ih, "FGCOLOR"))
    iupClassObjectSetAttribute(ih, "FGCOLOR", NULL, &inherit);

  if (ih->firstchild) androidThemeRefreshChildren(ih->firstchild);

  IFni cb = (IFni)IupGetCallback(ih, "THEMECHANGED_CB");
  if (cb)
  {
    int ret = cb(ih, (int)dark_mode);
    if (ret == IUP_CLOSE) IupExitLoop();
  }
}

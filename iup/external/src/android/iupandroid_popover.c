/** \file
 * \brief Popover Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_class.h"
#include "iup_popover.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupPopoverHelper);

/* GlobalRef to the inner IupAndroidFixed; returned by GetInnerNativeContainerHandle */
#define IUPANDROID_POPOVER_INNER "_IUP_ANDROID_POPOVER_INNER"


static int androidPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupPopoverHelper, jni_env, "io/github/gen2brain/iupgo/IupPopoverHelper");

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    if (!anchor || !anchor->handle)
    {
      (*jni_env)->DeleteLocalRef(jni_env, java_class);
      return 0;
    }

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
      {
        (*jni_env)->DeleteLocalRef(jni_env, java_class);
        return 0;
      }
    }

    /* Layout first so iupPopoverCalcPosition sees the child's real size. */
    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    int ax = 0, ay = 0;
    iupdrvClientToScreen(anchor, &ax, &ay);
    int aw = anchor->currentwidth;
    int ah = anchor->currentheight;

    int x, y;
    iupPopoverCalcPosition(ih, ax, ay, aw, ah,
      ih->currentwidth, ih->currentheight, &x, &y);

    int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");

    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "show",
      "(Landroid/widget/PopupWindow;Landroid/view/View;IIIIZ)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
      ih->handle, anchor->handle,
      (jint)x, (jint)y,
      (jint)ih->currentwidth, (jint)ih->currentheight,
      (jboolean)(autohide ? JNI_TRUE : JNI_FALSE));
    iupAndroid_CheckException(jni_env, "IupPopoverHelper.show");

    IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    if (show_cb) show_cb(ih, IUP_SHOW);
  }
  else if (ih->handle)
  {
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "dismiss", "(Landroid/widget/PopupWindow;)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle);
    iupAndroid_CheckException(jni_env, "IupPopoverHelper.dismiss");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static char* androidPopoverGetVisibleAttrib(Ihandle* ih)
{
  if (!ih->handle) return "NO";

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupPopoverHelper, jni_env, "io/github/gen2brain/iupgo/IupPopoverHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "isShowing", "(Landroid/widget/PopupWindow;)Z");
  jboolean showing = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, ih->handle);
  iupAndroid_CheckException(jni_env, "IupPopoverHelper.isShowing");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return iupStrReturnBoolean(showing);
}

static void androidPopoverLayoutUpdateMethod(Ihandle* ih)
{
  /* Native size is applied at show() time; just recurse into children. */
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static void* androidPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, IUPANDROID_POPOVER_INNER);
}

static int androidPopoverMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupPopoverHelper, jni_env, "io/github/gen2brain/iupgo/IupPopoverHelper");

  jmethodID create_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "create", "(J)Landroid/widget/PopupWindow;");
  jobject popup = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, create_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupPopoverHelper.create");

  if (!popup)
  {
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
    return IUP_ERROR;
  }

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, popup));

  jmethodID inner_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getInner", "(Landroid/widget/PopupWindow;)Lio/github/gen2brain/iupgo/IupAndroidFixed;");
  jobject inner = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, inner_id, popup);
  iupAndroid_CheckException(jni_env, "IupPopoverHelper.getInner");
  if (inner)
    iupAttribSet(ih, IUPANDROID_POPOVER_INNER, (char*)(*jni_env)->NewGlobalRef(jni_env, inner));

  (*jni_env)->DeleteLocalRef(jni_env, inner);
  (*jni_env)->DeleteLocalRef(jni_env, popup);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return IUP_NOERROR;
}

static void androidPopoverUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupPopoverHelper, jni_env, "io/github/gen2brain/iupgo/IupPopoverHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "dismiss", "(Landroid/widget/PopupWindow;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle);
  iupAndroid_CheckException(jni_env, "IupPopoverHelper.dismiss");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  jobject inner = (jobject)iupAttribGet(ih, IUPANDROID_POPOVER_INNER);
  if (inner)
  {
    (*jni_env)->DeleteGlobalRef(jni_env, inner);
    iupAttribSet(ih, IUPANDROID_POPOVER_INNER, NULL);
  }

  iupAndroid_ReleaseIhandle(jni_env, ih);
}

void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = androidPopoverMapMethod;
  ic->UnMap = androidPopoverUnMapMethod;
  ic->LayoutUpdate = androidPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = androidPopoverGetInnerNativeContainerHandleMethod;

  /* Setter maps on demand, VISIBLE is per-widget. */
  iupClassRegisterAttribute(ic, "VISIBLE",
    androidPopoverGetVisibleAttrib, androidPopoverSetVisibleAttrib,
    NULL, NULL,
    IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}

/** \file
 * \brief Tabs Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_tabs.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupTabsHelper);

static jclass androidTabsFindHelper(JNIEnv* jni_env)
{
  return IUPJNI_FindClass(IupTabsHelper, jni_env, "io/github/gen2brain/iupgo/IupTabsHelper");
}

int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvTabsExtraMargin(void)
{
  return 0;
}

int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidTabsFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setCurrentTab", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.setCurrentTab");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidTabsFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getCurrentTab", "(Landroid/view/View;)I");
  jint pos = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.getCurrentTab");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)pos;
}

int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  Ihandle* parent = child ? child->parent : NULL;
  if (!parent || !parent->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTabsFindHelper(jni_env);
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "getTabVisible", "(Landroid/view/View;I)Z");
  jboolean v = (*jni_env)->CallStaticBooleanMethod(jni_env, cls, m, (jobject)parent->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.getTabVisible");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return v ? 1 : 0;
}

void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int w = 0, h = 0;
  if (tab_title)
    iupdrvFontGetMultiLineStringSize(ih, tab_title, &w, &h);
  if (tab_image)
  {
    int iw = 0, ih_img = 0;
    iupImageGetInfo(tab_image, &iw, &ih_img, NULL);
    iupTabsScaleImageSize(ih, iw, ih_img, &iw, &ih_img);
    w += iw + iupAndroid_DpToPx(8.0f);
    if (ih_img > h) h = ih_img;
  }
  if (tab_width) *tab_width = w + iupAndroid_DpToPx(32.0f);
  if (tab_height) *tab_height = h + iupAndroid_DpToPx(16.0f);
}

static int androidTabsIconSizePx(Ihandle* ih)
{
  int box_w = 0, box_h = 0;
  iupTabsGetImageBoxSize(ih, &box_w, &box_h);
  int side = box_w > box_h ? box_w : box_h;
  return side > 0 ? side : 0;
}

static int androidTabsAppendChild(Ihandle* ih, Ihandle* child, int pos)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidTabsFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "addTab", "(Landroid/view/View;ILjava/lang/String;Landroid/graphics/Bitmap;I)Landroid/view/View;");

  const char* title = iupAttribGetId(ih, "TABTITLE", pos);
  if (!title) title = iupAttribGet(child, "TABTITLE");
  char* stripped_title = title ? iupStrProcessMnemonic(title, NULL, 0) : NULL;
  jstring j_title = stripped_title ? (*jni_env)->NewStringUTF(jni_env, stripped_title) : NULL;

  const char* image = iupAttribGetId(ih, "TABIMAGE", pos);
  if (!image) image = iupAttribGet(child, "TABIMAGE");
  jobject j_bitmap = image ? (jobject)iupImageGetImage(image, ih, 0, NULL) : NULL;
  jint icon_size = (jint)androidTabsIconSizePx(ih);

  jobject page = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jobject)ih->handle, (jint)pos, j_title, j_bitmap, icon_size);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.addTab");

  if (j_title) (*jni_env)->DeleteLocalRef(jni_env, j_title);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (stripped_title && stripped_title != title) free(stripped_title);

  if (!page) return IUP_ERROR;

  jobject page_global = (*jni_env)->NewGlobalRef(jni_env, page);
  (*jni_env)->DeleteLocalRef(jni_env, page);
  iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)page_global);
  return IUP_NOERROR;
}

static void androidTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (!ih->handle) return;
  if (iupAttribGet(ih, "_IUPTABS_REORDERING")) return;
  int pos = IupGetChildPos(ih, child);
  androidTabsAppendChild(ih, child, pos);
}

static void androidTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (!ih->handle) return;
  if (iupAttribGet(ih, "_IUPTABS_REORDERING")) return;

  jobject page = (jobject)iupAttribGet(child, "_IUPTAB_CONTAINER");
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jclass java_class = androidTabsFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "removeTab", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.removeTab");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (page) (*jni_env)->DeleteGlobalRef(jni_env, page);
  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
}

static int androidTabsMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidTabsFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "createTabs", "(J)Landroid/view/View;");

  jobject widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.createTabs");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, widget));
  (*jni_env)->DeleteLocalRef(jni_env, widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);

  /* Append any children that were added to the IUP tree before mapping. */
  Ihandle* child = ih->firstchild;
  int pos = 0;
  while (child)
  {
    if (androidTabsAppendChild(ih, child, pos) != IUP_NOERROR)
      break;
    child = child->brother;
    pos++;
  }

  return IUP_NOERROR;
}

static void androidTabsUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  /* Release per-child page global refs. */
  Ihandle* child = ih->firstchild;
  while (child)
  {
    jobject page = (jobject)iupAttribGet(child, "_IUPTAB_CONTAINER");
    if (page) (*jni_env)->DeleteGlobalRef(jni_env, page);
    iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
    child = child->brother;
  }

  iupAndroid_RemoveFromParent(jni_env, ih);
  iupAndroid_ReleaseIhandle(jni_env, ih);
}

static int androidTabsSetTabTitleAttribId(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child) iupAttribSetStr(child, "TABTITLE", value);
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTabsFindHelper(jni_env);
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setTabTitle", "(Landroid/view/View;ILjava/lang/String;)V");

  /* TABTITLE supports mnemonic '&' just like TITLE; strip on Android. */
  char* stripped = value ? iupStrProcessMnemonic(value, NULL, 0) : NULL;
  const char* effective = stripped ? stripped : value;
  jstring j_title = effective ? (*jni_env)->NewStringUTF(jni_env, effective) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos, j_title);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.setTabTitle");
  if (j_title) (*jni_env)->DeleteLocalRef(jni_env, j_title);
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  if (stripped && stripped != value) free(stripped);
  return 1;
}

static int androidTabsSetTabImageAttribId(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child) iupAttribSetStr(child, "TABIMAGE", value);
  if (!ih->handle) return 1;

  jobject bitmap = value ? (jobject)iupImageGetImage(value, ih, 0, NULL) : NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTabsFindHelper(jni_env);
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setTabIcon", "(Landroid/view/View;ILandroid/graphics/Bitmap;I)V");
  jint icon_size = (jint)androidTabsIconSizePx(ih);
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos, bitmap, icon_size);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.setTabIcon");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidTabsSetTabVisibleAttribId(Ihandle* ih, int pos, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTabsFindHelper(jni_env);
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setTabVisible", "(Landroid/view/View;IZ)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos, (jboolean)iupStrBoolean(value));
  iupAndroid_CheckException(jni_env, "IupTabsHelper.setTabVisible");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

/* pos == IUP_INVALID_ID: global SHOWCLOSE; else per-tab override. */
static int androidTabsSetShowCloseAttribId(Ihandle* ih, int pos, const char* value)
{
  if (pos == IUP_INVALID_ID)
    ih->data->show_close = iupStrBoolean(value);
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTabsFindHelper(jni_env);
  if (pos == IUP_INVALID_ID)
  {
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setShowClose", "(Landroid/view/View;Z)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jboolean)ih->data->show_close);
    iupAndroid_CheckException(jni_env, "IupTabsHelper.setShowClose");
  }
  else
  {
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setShowCloseAt", "(Landroid/view/View;IZ)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos, (jboolean)iupStrBoolean(value));
    iupAndroid_CheckException(jni_env, "IupTabsHelper.setShowCloseAt");
  }
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidTabsFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setReorderable", "(Landroid/view/View;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, (jboolean)iupStrBoolean(value));
  iupAndroid_CheckException(jni_env, "IupTabsHelper.setReorderable");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!ih->handle || !iupStrToRGB(value, &r, &g, &b)) return 1;

  jint color = (jint)(0xFF000000 | (r << 16) | (g << 8) | b);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidTabsFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setFgColor", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, color);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.setFgColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = androidTabsMapMethod;
  ic->UnMap = androidTabsUnMapMethod;
  ic->ChildAdded = androidTabsChildAddedMethod;
  ic->ChildRemoved = androidTabsChildRemovedMethod;

  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, androidTabsSetAllowReorderAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttributeId(ic, "TABTITLE", NULL, androidTabsSetTabTitleAttribId, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, androidTabsSetTabImageAttribId, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", NULL, androidTabsSetTabVisibleAttribId, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", NULL, androidTabsSetShowCloseAttribId, IUPAF_NO_INHERIT);
}

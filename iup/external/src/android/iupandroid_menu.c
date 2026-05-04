/** \file
 * \brief Menu for the Android Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_drv.h"
#include "iup_menu.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupMenuHelper);

/* Placeholder Object ref so IUP sees the menu node as mapped; Java inflates on demand. */
static jobject androidMenuCreateMarker(JNIEnv* jni_env)
{
  jclass obj_cls = (*jni_env)->FindClass(jni_env, "java/lang/Object");
  jmethodID ctor = (*jni_env)->GetMethodID(jni_env, obj_cls, "<init>", "()V");
  jobject obj = (*jni_env)->NewObject(jni_env, obj_cls, ctor);
  jobject ref = (*jni_env)->NewGlobalRef(jni_env, obj);
  (*jni_env)->DeleteLocalRef(jni_env, obj);
  (*jni_env)->DeleteLocalRef(jni_env, obj_cls);
  return ref;
}

static void androidMenuReleaseMarker(Ihandle* ih)
{
  if (!ih || !ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  (*jni_env)->DeleteGlobalRef(jni_env, (jobject)ih->handle);
  ih->handle = NULL;
}

/* Notifies the hosting Activity that its menu hierarchy has changed. */
static void androidMenuInvalidateActivity(Ihandle* menu_ih)
{
  Ihandle* dlg = menu_ih->parent;
  if (!dlg || !dlg->handle) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupMenuHelper, jni_env, "io/github/gen2brain/iupgo/IupMenuHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "attachToActivity", "(Ljava/lang/Object;J)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dlg->handle, (jlong)(intptr_t)menu_ih);
  iupAndroid_CheckException(jni_env, "IupMenuHelper.attachToActivity");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static int androidMenuItemRefreshAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (!ih) return 1;
  Ihandle* cur = ih->parent;
  while (cur)
  {
    if (iupMenuIsMenuBar(cur)) { androidMenuInvalidateActivity(cur); break; }
    if (!cur->parent) break;
    cur = cur->parent->parent;
  }
  return 1;
}

static void androidMenuDetachActivity(Ihandle* menu_ih)
{
  Ihandle* dlg = menu_ih->parent;
  if (!dlg || !dlg->handle) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupMenuHelper, jni_env, "io/github/gen2brain/iupgo/IupMenuHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "detachFromActivity", "(Ljava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dlg->handle);
  iupAndroid_CheckException(jni_env, "IupMenuHelper.detachFromActivity");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static int androidMenuMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  ih->handle = androidMenuCreateMarker(jni_env);

  /* deferred attach: dialog handle is still a ViewGroup placeholder at map time */
  return IUP_NOERROR;
}

/* Dialog and menu passed separately to avoid iup_dialog.h's IcontrolData clash. */
void iupAndroid_MenuAttachActivity(Ihandle* dlg, Ihandle* menu_ih)
{
  (void)dlg;
  if (!menu_ih) return;
  androidMenuInvalidateActivity(menu_ih);
}

void iupAndroid_DrawerAttachActivity(Ihandle* dlg, Ihandle* drawer_menu)
{
  if (!dlg || !dlg->handle || !drawer_menu || !drawer_menu->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupMenuHelper, jni_env, "io/github/gen2brain/iupgo/IupMenuHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "attachDrawer", "(Ljava/lang/Object;J)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dlg->handle, (jlong)(intptr_t)drawer_menu);
  iupAndroid_CheckException(jni_env, "IupMenuHelper.attachDrawer");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupAndroid_DrawerDetachActivity(Ihandle* dlg)
{
  if (!dlg || !dlg->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupMenuHelper, jni_env, "io/github/gen2brain/iupgo/IupMenuHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "detachDrawer", "(Ljava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dlg->handle);
  iupAndroid_CheckException(jni_env, "IupMenuHelper.detachDrawer");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidMenuUnMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
    androidMenuDetachActivity(ih);
  androidMenuReleaseMarker(ih);
}

static int androidMenuItemMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  ih->handle = androidMenuCreateMarker(jni_env);
  return IUP_NOERROR;
}

static int androidSubmenuMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  ih->handle = androidMenuCreateMarker(jni_env);
  return IUP_NOERROR;
}

static int androidSeparatorMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  ih->handle = androidMenuCreateMarker(jni_env);
  return IUP_NOERROR;
}

int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupMenuHelper, jni_env, "io/github/gen2brain/iupgo/IupMenuHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "popup", "(JII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jint)x, (jint)y);
  iupAndroid_CheckException(jni_env, "IupMenuHelper.popup");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return IUP_NOERROR;
}

int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  (void)ih;
  /* Material toolbar is 56dp. */
  return iupAndroid_DpToPx(56.0f);
}

void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = androidMenuMapMethod;
  ic->UnMap = androidMenuUnMapMethod;
}

void iupdrvMenuItemInitClass(Iclass* ic)
{
  ic->Map = androidMenuItemMapMethod;
  ic->UnMap = androidMenuUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE",  NULL, androidMenuItemRefreshAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE",  NULL, androidMenuItemRefreshAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, androidMenuItemRefreshAttrib, "YES", NULL, IUPAF_NO_INHERIT);
}

void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = androidSubmenuMapMethod;
  ic->UnMap = androidMenuUnMapMethod;

  iupClassRegisterAttribute(ic, "EXPANDABLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATE",      NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}

void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
  ic->Map = androidSeparatorMapMethod;
  ic->UnMap = androidMenuUnMapMethod;
}

/* Recent items stored as menu Ihandle attribs; Java prepends them before regular children. */

static Ihandle* androidRecentFindMenuBar(Ihandle* menu)
{
  Ihandle* cur = menu;
  while (cur)
  {
    if (iupMenuIsMenuBar(cur)) return cur;
    /* menu -> submenu -> menu -> ... */
    Ihandle* p = cur->parent;
    if (!p) return NULL;
    cur = p->parent;
  }
  return NULL;
}

int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  int max_recent, existing, i;
  if (!menu) return -1;

  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");
  if (count > max_recent) count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  for (i = 0; i < count; i++)
  {
    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);
  }
  for (; i < existing; i++)
  {
    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, attr_name, NULL);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);

  /* Rebuild the action-bar if attached to a menu bar; popup menus rebuild on show. */
  Ihandle* bar = androidRecentFindMenuBar(menu);
  if (bar) androidMenuInvalidateActivity(bar);

  return 0;
}

/** \file
 * \brief JNI bridge: IupMenuHelper (tree walk + ACTION dispatch).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_str.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"

IUPJNI_DECLARE_CLASS_STATIC(IupFileDlgHelper);


/* Type codes returned to Java; must match IupMenuHelper constants. */
#define IUP_MENU_NODE_UNKNOWN    0
#define IUP_MENU_NODE_MENU       1
#define IUP_MENU_NODE_SUBMENU    2
#define IUP_MENU_NODE_ITEM       3
#define IUP_MENU_NODE_SEPARATOR  4

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetType(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !ih->iclass || !ih->iclass->name) return IUP_MENU_NODE_UNKNOWN;

  const char* name = ih->iclass->name;
  if (strcmp(name, "menu") == 0) return IUP_MENU_NODE_MENU;
  if (strcmp(name, "submenu") == 0) return IUP_MENU_NODE_SUBMENU;
  if (strcmp(name, "menuitem") == 0) return IUP_MENU_NODE_ITEM;
  if (strcmp(name, "menuseparator") == 0) return IUP_MENU_NODE_SEPARATOR;
  return IUP_MENU_NODE_UNKNOWN;
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetChildCount(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 0;
  return IupGetChildCount(ih);
}

JNIEXPORT jlong JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetChild(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint index)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 0;
  Ihandle* child = IupGetChild(ih, index);
  return (intptr_t)child;
}

JNIEXPORT jstring JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetTitle(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return NULL;
  const char* title = iupAttribGet(ih, "TITLE");
  if (!title) return NULL;
  char* stripped = iupStrProcessMnemonic(title, NULL, 0);
  const char* src = stripped ? stripped : title;
  /* Truncate at the IUP accelerator separator; Android menus have no shortcut column. */
  const char* tab = strchr(src, '\t');
  jstring j_str;
  if (tab)
  {
    int len = (int)(tab - src);
    char* tmp = (char*)malloc((size_t)len + 1);
    memcpy(tmp, src, (size_t)len);
    tmp[len] = '\0';
    j_str = (*jni_env)->NewStringUTF(jni_env, tmp);
    free(tmp);
  }
  else
    j_str = (*jni_env)->NewStringUTF(jni_env, src);
  if (stripped && stripped != title) free(stripped);
  return j_str;
}

JNIEXPORT jboolean JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeIsActive(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return JNI_FALSE;
  const char* active = iupAttribGet(ih, "ACTIVE");
  if (!active) return JNI_TRUE;
  return iupStrBoolean(active) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetBoolAttribute(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_name)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !j_name) return JNI_FALSE;
  const char* name = (*jni_env)->GetStringUTFChars(jni_env, j_name, NULL);
  const char* val = iupAttribGet(ih, name);
  jboolean result = (val && iupStrBoolean(val)) ? JNI_TRUE : JNI_FALSE;
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_name, name);
  return result;
}

JNIEXPORT jstring JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetStringAttribute(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_name)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !j_name) return NULL;
  const char* name = (*jni_env)->GetStringUTFChars(jni_env, j_name, NULL);
  const char* val = iupAttribGet(ih, name);
  jstring result = val ? (*jni_env)->NewStringUTF(jni_env, val) : NULL;
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_name, name);
  return result;
}

/* IMPRESS-when-VALUE > IMAGE > TITLEIMAGE. */
JNIEXPORT jobject JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetImage(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  const char* name = NULL;
  if (!ih) return NULL;
  if (iupAttribGetBoolean(ih, "VALUE"))
    name = iupAttribGet(ih, "IMPRESS");
  if (!name) name = iupAttribGet(ih, "IMAGE");
  if (!name) name = iupAttribGet(ih, "TITLEIMAGE");
  if (!name || !*name) return NULL;
  return (jobject)iupImageGetImage(name, ih, 0, NULL);
}

JNIEXPORT jboolean JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeIsParentRadio(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !ih->parent) return JNI_FALSE;
  return iupAttribGetBoolean(ih->parent, "RADIO") ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeDispatchAction(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  Ihandle* parent_menu = ih->parent;
  if (parent_menu && iupAttribGetBoolean(parent_menu, "RADIO"))
  {
    int n = IupGetChildCount(parent_menu);
    for (int i = 0; i < n; i++)
    {
      Ihandle* sib = IupGetChild(parent_menu, i);
      if (sib && sib->iclass && iupStrEqual(sib->iclass->name, "menuitem"))
        IupSetStrAttribute(sib, "VALUE", sib == ih ? "ON" : "OFF");
    }
  }
  else if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
  {
    const char* cur = iupAttribGet(ih, "VALUE");
    IupSetStrAttribute(ih, "VALUE", iupStrEqualNoCase(cur, "ON") ? "OFF" : "ON");
  }

  Icallback cb = IupGetCallback(ih, "ACTION");
  if (!cb) return;
  int ret = cb(ih);
  if (ret == IUP_CLOSE) IupExitLoop();
}


JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetRecentCount(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 0;
  return iupAttribGetInt(ih, "_IUP_RECENT_COUNT");
}

JNIEXPORT jstring JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeGetRecentFile(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint index)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return NULL;
  char attr[32];
  snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", (int)index);
  const char* path = iupAttribGet(ih, attr);
  if (!path) return NULL;
  return (*jni_env)->NewStringUTF(jni_env, path);
}

/* Re-stage content:// URIs to cache and fire RECENT_CB with RECENTFILENAME on config. */
JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupMenuHelper_nativeDispatchRecent(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint index)
{
  (void)cls;
  Ihandle* menu = (Ihandle*)ihandle_ptr;
  if (!menu) return;

  char attr[32];
  snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", (int)index);
  const char* filename = iupAttribGet(menu, attr);
  if (!filename) return;

  Icallback recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  Ihandle* config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");
  if (!recent_cb || !config) return;

  char* staged = NULL;
  const char* dispatch_name = filename;
  if (strncmp(filename, "content://", 10) == 0)
  {
    jclass java_class = IUPJNI_FindClass(IupFileDlgHelper, jni_env, "io/github/gen2brain/iupgo/IupFileDlgHelper");
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "stageRecentUri", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring j_uri = (*jni_env)->NewStringUTF(jni_env, filename);
    jstring j_path = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, j_uri);
    iupAndroid_CheckException(jni_env, "IupFileDlgHelper.stageRecentUri");
    (*jni_env)->DeleteLocalRef(jni_env, j_uri);
    (*jni_env)->DeleteLocalRef(jni_env, java_class);

    if (j_path)
    {
      const char* c_path = (*jni_env)->GetStringUTFChars(jni_env, j_path, NULL);
      if (c_path && *c_path)
      {
        staged = strdup(c_path);
        dispatch_name = staged;
      }
      (*jni_env)->ReleaseStringUTFChars(jni_env, j_path, c_path);
      (*jni_env)->DeleteLocalRef(jni_env, j_path);
    }
  }

  IupSetStrAttribute(config, "RECENTFILENAME", dispatch_name);
  IupSetStrAttribute(config, "TITLE", dispatch_name);
  config->parent = menu;
  recent_cb(config);
  config->parent = NULL;
  IupSetAttribute(config, "RECENTFILENAME", NULL);
  IupSetAttribute(config, "TITLE", NULL);

  if (staged) free(staged);
}

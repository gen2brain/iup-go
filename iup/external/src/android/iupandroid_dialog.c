/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"

#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupActivity);
IUPJNI_DECLARE_CLASS_STATIC(IupDialogHelper);

/* Placeholder until the first Java layout pass reports the real size. */
#define ANDROID_DEFAULT_DIALOG_WIDTH 1024
#define ANDROID_DEFAULT_DIALOG_HEIGHT 1920

/* uncached JNI lookup; dialog attr changes are cold enough */
static void androidDialogCallVoidWithString(Ihandle* ih, const char* method_name, const char* utf8)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jclass java_class = IUPJNI_FindClass(IupDialogHelper, jni_env, "io/github/gen2brain/iupgo/IupDialogHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "(Ljava/lang/Object;Ljava/lang/String;)V");

  if (method_id)
  {
    jstring j_string = utf8 ? (*jni_env)->NewStringUTF(jni_env, utf8) : NULL;
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_string);
    iupAndroid_CheckException(jni_env, method_name);
    if (j_string)
      (*jni_env)->DeleteLocalRef(jni_env, j_string);
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidDialogCallVoidWithBool(Ihandle* ih, const char* method_name, int value)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jclass java_class = IUPJNI_FindClass(IupDialogHelper, jni_env, "io/github/gen2brain/iupgo/IupDialogHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "(Ljava/lang/Object;Z)V");

  if (method_id)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jboolean)(value ? JNI_TRUE : JNI_FALSE));
    iupAndroid_CheckException(jni_env, method_name);
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidDialogCallVoidWithFloat(Ihandle* ih, const char* method_name, float value)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jclass java_class = IUPJNI_FindClass(IupDialogHelper, jni_env, "io/github/gen2brain/iupgo/IupDialogHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "(Ljava/lang/Object;F)V");

  if (method_id)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jfloat)value);
    iupAndroid_CheckException(jni_env, method_name);
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidDialogCallVoidNoArg(Ihandle* ih, const char* method_name)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jclass java_class = IUPJNI_FindClass(IupDialogHelper, jni_env, "io/github/gen2brain/iupgo/IupDialogHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "(Ljava/lang/Object;)V");

  if (method_id)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle);
    iupAndroid_CheckException(jni_env, method_name);
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidDialogCallSetIcon(Ihandle* ih, jobject bitmap)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jclass java_class = IUPJNI_FindClass(IupDialogHelper, jni_env, "io/github/gen2brain/iupgo/IupDialogHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setIcon", "(Ljava/lang/Object;Landroid/graphics/Bitmap;)V");

  if (method_id)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, bitmap);
    iupAndroid_CheckException(jni_env, "IupDialogHelper.setIcon");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidDialogSetBgColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jclass java_class = IUPJNI_FindClass(IupDialogHelper, jni_env, "io/github/gen2brain/iupgo/IupDialogHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setBgColor", "(Ljava/lang/Object;III)V");

  if (method_id)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
    iupAndroid_CheckException(jni_env, "IupDialogHelper.setBgColor");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

int iupdrvDialogIsVisible(Ihandle* ih)
{
  /* An Activity-backed dialog is always "visible" once mapped. */
  (void)ih;
  return 1;
}

void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
  (void)handle;
  if (w) *w = ih ? ih->currentwidth : ANDROID_DEFAULT_DIALOG_WIDTH;
  if (h) *h = ih ? ih->currentheight : ANDROID_DEFAULT_DIALOG_HEIGHT;
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  /* Show → foreground, Hide → background; teardown is reserved for IupDestroy. */
  androidDialogCallVoidNoArg(ih, visible ? "bringToFront" : "moveToBack");
}

void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
  (void)ih;
  (void)handle;
  if (x) *x = 0;
  if (y) *y = 0;
}

void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
  /* The window manager owns Activity positioning. */
  (void)ih;
  (void)x;
  (void)y;
}

void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
  /* IupAndroidFixed absorbs system bars as padding; IUP sees no external decoration */
  (void)ih;
  if (border) *border = 0;
  if (caption) *caption = 0;
  if (menu) *menu = 0;
}

int iupdrvDialogSetPlacement(Ihandle* ih)
{
  char* placement;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    androidDialogCallVoidWithBool(ih, "setFullscreen", 1);
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    ih->data->show_state = IUP_SHOW;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    androidDialogCallVoidNoArg(ih, "moveToBack");
    ih->data->show_state = IUP_MINIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED") || iupStrEqualNoCase(placement, "FULL"))
  {
    androidDialogCallVoidWithBool(ih, "setFullscreen", 1);
    ih->data->show_state = IUP_MAXIMIZE;
  }
  else  /* NORMAL or anything else */
  {
    androidDialogCallVoidWithBool(ih, "setFullscreen", 0);
    ih->data->show_state = IUP_SHOW;
  }

  iupAttribSet(ih, "PLACEMENT", NULL);
  return 1;
}

void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  /* Activities stack naturally, so parenting is not independently addressable. */
  (void)ih;
  (void)parent;
}

static char* androidDialogGetClientSizeAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->currentwidth, ih->currentheight, 'x');
}

static char* androidDialogGetClientOffsetAttrib(Ihandle* ih)
{
  /* IupAndroidFixed already eats insets, so (0,0) is the safe area */
  (void)ih;
  return iupStrReturnIntInt(0, 0, 'x');
}

static int androidDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  androidDialogCallVoidWithString(ih, "setTitle", value);
  return 1;
}

static int androidDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r = 0, g = 0, b = 0;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  androidDialogSetBgColor(ih, r, g, b);
  return 1;
}

static int androidDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  androidDialogCallVoidWithBool(ih, "setFullscreen", iupStrBoolean(value));
  return 1;
}

static int androidDialogSetHideTitleBarAttrib(Ihandle* ih, const char* value)
{
  androidDialogCallVoidWithBool(ih, "setHideTitleBar", iupStrBoolean(value));
  return 1;
}

static int androidDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  jobject bmp = value ? (jobject)iupImageGetImage(value, ih, 0, NULL) : NULL;
  androidDialogCallSetIcon(ih, bmp);
  return 1;
}

static int androidDialogSetTitleCenteredAttrib(Ihandle* ih, const char* value)
{
  androidDialogCallVoidWithBool(ih, "setTitleCentered", iupStrBoolean(value));
  return 1;
}

static int androidDialogSetTitleBarStyleAttrib(Ihandle* ih, const char* value)
{
  androidDialogCallVoidWithString(ih, "setTitleBarStyle", value);
  return 1;
}

static int androidDialogSetOrientationAttrib(Ihandle* ih, const char* value)
{
  androidDialogCallVoidWithString(ih, "setOrientation", value);
  return 1;
}

static int androidDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    androidDialogCallVoidNoArg(ih, "bringToFront");
  return 0;
}

static int androidDialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
  int op = 255;
  if (value && !iupStrToInt(value, &op)) op = 255;
  if (op < 0) op = 0;
  if (op > 255) op = 255;
  androidDialogCallVoidWithFloat(ih, "setOpacity", (float)op / 255.0f);
  return 1;
}

static int androidDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  /* Stored for IUP-internal layout clamping only; Android owns window size. */
  return iupBaseSetMinSizeAttrib(ih, value);
}

static int androidDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  return iupBaseSetMaxSizeAttrib(ih, value);
}

static int androidDialogSetDrawerAttrib(Ihandle* ih, const char* value)
{
  Ihandle* drawer = value ? IupGetHandle(value) : NULL;
  if (drawer && !drawer->handle)
    IupMap(drawer);
  if (!ih->handle) return 1;  /* deferred to Activity create */
  if (drawer && drawer->handle)
    iupAndroid_DrawerAttachActivity(ih, drawer);
  else
    iupAndroid_DrawerDetachActivity(ih);
  return 1;
}

void iupAndroid_DialogActivityCreated(Ihandle* ih)
{
  char* value;

  if (!ih)
    return;

  /* Always set: when TITLE is unset, Activity would otherwise fall back to the manifest android:label. */
  androidDialogSetTitleAttrib(ih, iupAttribGet(ih, "TITLE"));

  value = iupAttribGet(ih, "BGCOLOR");
  if (value)
    androidDialogSetBgColorAttrib(ih, value);

  if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
    androidDialogSetHideTitleBarAttrib(ih, "YES");

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
    androidDialogSetFullScreenAttrib(ih, "YES");

  value = iupAttribGet(ih, "ORIENTATION");
  if (value)
    androidDialogSetOrientationAttrib(ih, value);

  value = iupAttribGet(ih, "ICON");
  if (value)
    androidDialogSetIconAttrib(ih, value);

  value = iupAttribGet(ih, "TITLECENTERED");
  if (value)
    androidDialogSetTitleCenteredAttrib(ih, value);

  value = iupAttribGet(ih, "TITLEBARSTYLE");
  if (value)
    androidDialogSetTitleBarStyleAttrib(ih, value);

  value = iupAttribGet(ih, "OPACITY");
  if (value)
    androidDialogSetOpacityAttrib(ih, value);

  /* Re-attach the menu bar now that ih->handle is the real Activity. */
  if (ih->data && ih->data->menu && ih->data->menu->handle)
    iupAndroid_MenuAttachActivity(ih, ih->data->menu);

  value = iupAttribGet(ih, "DRAWER");
  if (value)
    androidDialogSetDrawerAttrib(ih, value);
}

static int androidDialogMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupActivity_createActivity);
  JNIEnv* jni_env;
  jclass java_class;
  jmethodID method_id;
  jobject current_activity;
  jobject view_group;

  jni_env = iupAndroid_GetEnvThreadSafe();

  current_activity = iupAndroid_GetCurrentActivity(jni_env);
  if (current_activity == NULL)
  {
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "androidDialogMapMethod: current_activity is NULL");
    return IUP_ERROR;
  }

  java_class = IUPJNI_FindClass(IupActivity, jni_env, "io/github/gen2brain/iupgo/IupActivity");
  method_id = IUPJNI_GetStaticMethodID(IupActivity_createActivity, jni_env, java_class, "createActivity", "(Landroid/app/Activity;J)Landroid/view/ViewGroup;");
  view_group = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, current_activity, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupActivity.createActivity");

  /* ViewGroup placeholder now; Activity.onCreate swaps to the Activity later. */
  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, view_group));
  iupAttribSet(ih, "_IUP_DIALOG_DEFER_DESTROY", "1");  /* cleared in Activity.onDestroy */

  (*jni_env)->DeleteLocalRef(jni_env, view_group);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  (*jni_env)->DeleteLocalRef(jni_env, current_activity);

  ih->currentwidth = ANDROID_DEFAULT_DIALOG_WIDTH;
  ih->currentheight = ANDROID_DEFAULT_DIALOG_HEIGHT;

  return IUP_NOERROR;
}

static void androidDialogUnMapMethod(Ihandle* ih)
{
  if (ih && ih->handle)
  {
    IUPJNI_DECLARE_METHOD_ID_STATIC(IupActivity_unMapActivity);
    jclass java_class;
    jmethodID method_id;
    JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

    /* unMapActivity handles both the live Activity and the pre-onCreate placeholder */
    java_class = IUPJNI_FindClass(IupActivity, jni_env, "io/github/gen2brain/iupgo/IupActivity");
    method_id = IUPJNI_GetStaticMethodID(IupActivity_unMapActivity, jni_env, java_class, "unMapActivity", "(Ljava/lang/Object;J)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jlong)(intptr_t)ih);
    iupAndroid_CheckException(jni_env, "IupActivity.unMapActivity");

    (*jni_env)->DeleteLocalRef(jni_env, java_class);

    iupAndroid_ReleaseIhandle(jni_env, ih);
  }
}

static void androidDialogLayoutUpdateMethod(Ihandle* ih)
{
  /* Android owns window sizing via the Activity; no native resize to drive. */
  (void)ih;
}

void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = androidDialogMapMethod;
  ic->UnMap = androidDialogUnMapMethod;
  ic->LayoutUpdate = androidDialogLayoutUpdateMethod;

  /* Touch-UI defaults: shrink content to viewport width (height overflows for vertical scroll), and skip auto-focus so requestFocus doesn't silently land on a text field. */
  iupClassRegisterReplaceAttribDef(ic, "SHRINK", "YES", NULL);
  iupClassRegisterReplaceAttribDef(ic, "SHOWNOFOCUS", "YES", NULL);

  /* Fired from IupActivity.onConfigurationChanged when UI_MODE_NIGHT_MASK flips. */
  iupClassRegisterCallback(ic, "THEMECHANGED_CB", "i");

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", androidDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", androidDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidDialogSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", NULL, androidDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, androidDialogSetFullScreenAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, androidDialogSetHideTitleBarAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, androidDialogSetBringFrontAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* Android-specific: activity requested orientation */
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, androidDialogSetOrientationAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Android-specific: NavigationDrawer menu; independent of MENU. */
  iupClassRegisterAttribute(ic, "DRAWER", NULL, androidDialogSetDrawerAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MINSIZE", NULL, androidDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, androidDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);

  /* Not applicable on Android. */
  iupClassRegisterAttribute(ic, "MAXBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAMESIMULATE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETASKBAR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", NULL, androidDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SIMULATEMODAL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NATIVEPARENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PARENTDIALOG", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ICON", NULL, androidDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLECENTERED", NULL, androidDialogSetTitleCenteredAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEBARSTYLE", NULL, androidDialogSetTitleBarStyleAttrib, IUPAF_SAMEASSYSTEM, "FLAT", IUPAF_NO_INHERIT);

  /* Tray (no tray on Android). */
  iupClassRegisterAttribute(ic, "TRAY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPMARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPDELAY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

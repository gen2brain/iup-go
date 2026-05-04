/** \file
 * \brief Notify Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_notify.h"
#include "iup_image.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupNotifyHelper);

static int androidNotifyUrgency(Ihandle* ih)
{
  int u = IupGetInt(ih, "URGENCY");
  if (u < 0) return 0;
  if (u > 2) return 2;
  return u;
}

int iupdrvNotifyShow(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupNotifyHelper, jni_env, "io/github/gen2brain/iupgo/IupNotifyHelper");

  const char* title = IupGetAttribute(ih, "TITLE");
  const char* body = IupGetAttribute(ih, "BODY");

  /* STYLE=TOAST: custom in-app overlay with optional ICON bitmap. */
  const char* style = IupGetAttribute(ih, "STYLE");
  if (style && iupStrEqualNoCase(style, "TOAST"))
  {
    const char* toast_icon = IupGetAttribute(ih, "ICON");
    jobject j_toast_icon = NULL;
    if (toast_icon && *toast_icon)
      j_toast_icon = (jobject)iupImageGetImage(toast_icon, ih, 0, NULL);

    jstring j_title = title ? (*jni_env)->NewStringUTF(jni_env, title) : NULL;
    jstring j_body  = body  ? (*jni_env)->NewStringUTF(jni_env, body)  : NULL;
    jmethodID toast_m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "showToast",
      "(JLjava/lang/String;Ljava/lang/String;Landroid/graphics/Bitmap;I)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, toast_m,
      (jlong)(intptr_t)ih, j_title, j_body, j_toast_icon, (jint)IupGetInt(ih, "TIMEOUT"));
    iupAndroid_CheckException(jni_env, "IupNotifyHelper.showToast");
    if (j_title) (*jni_env)->DeleteLocalRef(jni_env, j_title);
    if (j_body)  (*jni_env)->DeleteLocalRef(jni_env, j_body);
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
    return 1;
  }

  jmethodID next_id_m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "nextId", "()I");

  int notify_id = iupAttribGetInt(ih, "_IUPANDROID_NOTIFY_ID");
  if (notify_id == 0)
  {
    notify_id = (int)(*jni_env)->CallStaticIntMethod(jni_env, java_class, next_id_m);
    iupAttribSetInt(ih, "_IUPANDROID_NOTIFY_ID", notify_id);
    IupSetInt(ih, "ID", notify_id);
  }

  const char* icon = IupGetAttribute(ih, "ICON");
  if (!icon) icon = IupGetAttribute(ih, "APPICON");
  const char* a1 = IupGetAttribute(ih, "ACTION1");
  const char* a2 = IupGetAttribute(ih, "ACTION2");
  const char* a3 = IupGetAttribute(ih, "ACTION3");
  const char* a4 = IupGetAttribute(ih, "ACTION4");

  jobject j_icon = NULL;
  if (icon && *icon)
    j_icon = (jobject)iupImageGetImage(icon, ih, 0, NULL);

  /* PROGRESS: integer (0..max) or "INDETERMINATE"; absent = no progress bar. */
  const char* progress_str = IupGetAttribute(ih, "PROGRESS");
  int progress_current = -1, progress_max = 0, progress_indeterminate = 0;
  if (progress_str && *progress_str)
  {
    if (iupStrEqualNoCase(progress_str, "INDETERMINATE"))
    {
      progress_indeterminate = 1;
    }
    else
    {
      iupStrToInt(progress_str, &progress_current);
      if (progress_current < 0) progress_current = 0;
      progress_max = 100;
      if (progress_current > 100) progress_current = 100;
    }
  }

  const char* image_name = IupGetAttribute(ih, "IMAGE");
  jobject j_image = NULL;
  if (image_name && *image_name)
    j_image = (jobject)iupImageGetImage(image_name, ih, 0, NULL);

  const char* channel_id = IupGetAttribute(ih, "CHANNELID");
  const char* importance = IupGetAttribute(ih, "IMPORTANCE");

  jstring j_title   = title      ? (*jni_env)->NewStringUTF(jni_env, title)      : NULL;
  jstring j_body    = body       ? (*jni_env)->NewStringUTF(jni_env, body)       : NULL;
  jstring j_a1      = a1         ? (*jni_env)->NewStringUTF(jni_env, a1)         : NULL;
  jstring j_a2      = a2         ? (*jni_env)->NewStringUTF(jni_env, a2)         : NULL;
  jstring j_a3      = a3         ? (*jni_env)->NewStringUTF(jni_env, a3)         : NULL;
  jstring j_a4      = a4         ? (*jni_env)->NewStringUTF(jni_env, a4)         : NULL;
  jstring j_channel = channel_id ? (*jni_env)->NewStringUTF(jni_env, channel_id) : NULL;
  jstring j_imp     = importance ? (*jni_env)->NewStringUTF(jni_env, importance) : NULL;

  jmethodID show_m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "show",
    "(JILjava/lang/String;Ljava/lang/String;Landroid/graphics/Bitmap;IIZZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIZLandroid/graphics/Bitmap;Ljava/lang/String;Ljava/lang/String;)I");

  jint posted = (*jni_env)->CallStaticIntMethod(jni_env, java_class, show_m,
    (jlong)(intptr_t)ih, (jint)notify_id, j_title, j_body, j_icon,
    (jint)IupGetInt(ih, "TIMEOUT"),
    (jint)androidNotifyUrgency(ih),
    (jboolean)(IupGetInt(ih, "SILENT") ? JNI_TRUE : JNI_FALSE),
    (jboolean)(IupGetInt(ih, "TRANSIENT") ? JNI_TRUE : JNI_FALSE),
    j_a1, j_a2, j_a3, j_a4,
    (jint)progress_current, (jint)progress_max, (jboolean)(progress_indeterminate ? JNI_TRUE : JNI_FALSE),
    j_image,
    j_channel, j_imp);
  iupAndroid_CheckException(jni_env, "IupNotifyHelper.show");

  if (j_title)   (*jni_env)->DeleteLocalRef(jni_env, j_title);
  if (j_body)    (*jni_env)->DeleteLocalRef(jni_env, j_body);
  if (j_a1)      (*jni_env)->DeleteLocalRef(jni_env, j_a1);
  if (j_a2)      (*jni_env)->DeleteLocalRef(jni_env, j_a2);
  if (j_a3)      (*jni_env)->DeleteLocalRef(jni_env, j_a3);
  if (j_a4)      (*jni_env)->DeleteLocalRef(jni_env, j_a4);
  if (j_channel) (*jni_env)->DeleteLocalRef(jni_env, j_channel);
  if (j_imp)     (*jni_env)->DeleteLocalRef(jni_env, j_imp);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (posted == 0)
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb) error_cb(ih, "Notification permission denied");
    return 0;
  }
  return 1;
}

int iupdrvNotifyClose(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupNotifyHelper, jni_env, "io/github/gen2brain/iupgo/IupNotifyHelper");

  const char* style = IupGetAttribute(ih, "STYLE");
  if (style && iupStrEqualNoCase(style, "TOAST"))
  {
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "cancelToast", "(J)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, m, (jlong)(intptr_t)ih);
    iupAndroid_CheckException(jni_env, "IupNotifyHelper.cancelToast");
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
    return 1;
  }

  int notify_id = iupAttribGetInt(ih, "_IUPANDROID_NOTIFY_ID");
  if (notify_id == 0) { (*jni_env)->DeleteLocalRef(jni_env, java_class); return 0; }

  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "close", "(I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, m, (jint)notify_id);
  iupAndroid_CheckException(jni_env, "IupNotifyHelper.close");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  IFni cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih, 3);
    if (ret == IUP_CLOSE) IupExitLoop();
  }
  return 1;
}

void iupdrvNotifyDestroy(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupNotifyHelper, jni_env, "io/github/gen2brain/iupgo/IupNotifyHelper");

  const char* style = IupGetAttribute(ih, "STYLE");
  if (style && iupStrEqualNoCase(style, "TOAST"))
  {
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "cancelToast", "(J)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, m, (jlong)(intptr_t)ih);
    iupAndroid_CheckException(jni_env, "IupNotifyHelper.cancelToast");
  }
  else
  {
    int notify_id = iupAttribGetInt(ih, "_IUPANDROID_NOTIFY_ID");
    if (notify_id != 0)
    {
      jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "close", "(I)V");
      (*jni_env)->CallStaticVoidMethod(jni_env, java_class, m, (jint)notify_id);
      iupAndroid_CheckException(jni_env, "IupNotifyHelper.close");
    }
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  iupAttribSet(ih, "_IUPANDROID_NOTIFY_ID", NULL);
}

int iupdrvNotifyIsAvailable(void)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupNotifyHelper, jni_env, "io/github/gen2brain/iupgo/IupNotifyHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "isAvailable", "()Z");
  jboolean ok = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, m);
  iupAndroid_CheckException(jni_env, "IupNotifyHelper.isAvailable");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return ok ? 1 : 0;
}

void iupdrvNotifyInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "URGENCY", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPICON", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRANSIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /* STYLE=TOAST → android.widget.Toast; default NOTIFICATION → shade via NotificationManager. */
  iupClassRegisterAttribute(ic, "STYLE", NULL, NULL, IUPAF_SAMEASSYSTEM, "NOTIFICATION", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PROGRESS", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CHANNELID", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPORTANCE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}

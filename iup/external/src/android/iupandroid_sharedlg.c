/** \file
 * \brief IupShareDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupShareHelper);

static int androidShareDlgPopup(Ihandle* ih, int x, int y)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupShareHelper, jni_env, "io/github/gen2brain/iupgo/IupShareHelper");
  jstring j_title = iupAndroid_NewStringUTF(jni_env, iupAttribGet(ih, "TITLE"));
  jstring j_text = iupAndroid_NewStringUTF(jni_env, iupAttribGet(ih, "TEXT"));
  jstring j_url = iupAndroid_NewStringUTF(jni_env, iupAttribGet(ih, "URL"));
  jstring j_file = iupAndroid_NewStringUTF(jni_env, iupAttribGet(ih, "FILE"));
  jmethodID m;
  jint status;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "share", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
  status = (*jni_env)->CallStaticIntMethod(jni_env, java_class, m, j_title, j_text, j_url, j_file);
  iupAndroid_CheckException(jni_env, "IupShareHelper.share");

  if (j_title) (*jni_env)->DeleteLocalRef(jni_env, j_title);
  if (j_text) (*jni_env)->DeleteLocalRef(jni_env, j_text);
  if (j_url) (*jni_env)->DeleteLocalRef(jni_env, j_url);
  if (j_file) (*jni_env)->DeleteLocalRef(jni_env, j_file);

  iupAttribSet(ih, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvShareDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = androidShareDlgPopup;
}

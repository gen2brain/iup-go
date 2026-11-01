/** \file
 * \brief IupPrintDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>
#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupPrintHelper);

static int androidPrintDlgPopup(Ihandle* ih, int x, int y)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupPrintHelper, jni_env, "io/github/gen2brain/iupgo/IupPrintHelper");
  jstring j_name = iupAndroid_NewStringUTF(jni_env, iupPrintDlgGetJobName(ih));
  int landscape = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE");
  jmethodID m;
  jint status;

  (void)x;
  (void)y;

  m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "print", "(JLjava/lang/String;Z)I");
  status = (*jni_env)->CallStaticIntMethod(jni_env, java_class, m, (jlong)(intptr_t)ih, j_name, (jboolean)(landscape ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, "IupPrintHelper.print");
  if (j_name) (*jni_env)->DeleteLocalRef(jni_env, j_name);

  m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "lastLandscape", "()Z");
  landscape = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, m);
  iupAndroid_CheckException(jni_env, "IupPrintHelper.lastLandscape");

  iupAttribSet(ih, "ORIENTATION", landscape ? "LANDSCAPE" : "PORTRAIT");
  iupAttribSet(ih, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = androidPrintDlgPopup;
}

/** \file
 * \brief JNI bridge: IupPrintHelper (PrintDocumentAdapter -> IupPrintDlg).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_stdcontrols.h"

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupPrintHelper_nativePaginate(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint width, jint height)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 1;
  return iupPrintDlgPaginate(ih, width, height);
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupPrintHelper_nativeDrawPage(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jobject view, jint page, jint width, jint height)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return IUP_CLOSE;
  return iupPrintDlgDrawPage(ih, view, page, width, height);
}

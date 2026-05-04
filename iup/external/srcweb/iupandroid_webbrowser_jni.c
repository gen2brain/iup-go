/** \file
 * \brief JNI bridge: IupWebBrowserHelper (load lifecycle -> IUP callbacks).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"

#include "iupandroid_drv.h"


/* Invokes NAVIGATE_CB. Returns IUP_IGNORE (-1) to let the WebViewClient
   cancel the navigation; any other value lets it proceed. */
JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupWebBrowserHelper_dispatchNavigate(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_url)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return IUP_DEFAULT;

  IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
  if (!cb) return IUP_DEFAULT;

  const char* url = j_url ? (*jni_env)->GetStringUTFChars(jni_env, j_url, NULL) : "";
  int rc = cb(ih, (char*)url);
  if (j_url) (*jni_env)->ReleaseStringUTFChars(jni_env, j_url, url);
  return rc;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupWebBrowserHelper_dispatchCompleted(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_url)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
  if (!cb) return;

  const char* url = j_url ? (*jni_env)->GetStringUTFChars(jni_env, j_url, NULL) : "";
  cb(ih, (char*)url);
  if (j_url) (*jni_env)->ReleaseStringUTFChars(jni_env, j_url, url);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupWebBrowserHelper_dispatchError(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_url)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
  if (!cb) return;

  const char* url = j_url ? (*jni_env)->GetStringUTFChars(jni_env, j_url, NULL) : "";
  cb(ih, (char*)url);
  if (j_url) (*jni_env)->ReleaseStringUTFChars(jni_env, j_url, url);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupWebBrowserHelper_dispatchNewWindow(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_url)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
  if (!cb) return;

  const char* url = j_url ? (*jni_env)->GetStringUTFChars(jni_env, j_url, NULL) : "";
  cb(ih, (char*)url);
  if (j_url) (*jni_env)->ReleaseStringUTFChars(jni_env, j_url, url);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupWebBrowserHelper_dispatchUpdate(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  IFn cb = (IFn)IupGetCallback(ih, "UPDATE_CB");
  if (cb) cb(ih);
}

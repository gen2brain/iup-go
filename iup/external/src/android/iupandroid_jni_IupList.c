/** \file
 * \brief JNI bridge: IupListHelper (List/Spinner selection -> ACTION callback).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_image.h"

#include "iupandroid_drv.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchAction(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_text, jint item, jint state)
{
  (void)cls; (void)jni_env; (void)j_text; (void)state;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  iupAndroidListDispatchSelection(ih, item);
}

/* VIRTUALMODE adapter asks for a visible row's text. pos is 0-based; IUP VALUE_CB is 1-based. */
JNIEXPORT jstring JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListValueCb(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint pos)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return NULL;
  sIFni cb = (sIFni)IupGetCallback(ih, "VALUE_CB");
  if (!cb) return NULL;
  char* value = cb(ih, (int)pos + 1);
  return value ? (*jni_env)->NewStringUTF(jni_env, value) : NULL;
}

/* VIRTUALMODE per-row image: IMAGE_CB returns an image name; resolve to a Bitmap. */
JNIEXPORT jobject JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListImageCb(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint pos)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return NULL;
  sIFni cb = (sIFni)IupGetCallback(ih, "IMAGE_CB");
  if (!cb) return NULL;
  char* name = cb(ih, (int)pos + 1);
  if (!name || !*name) return NULL;
  return (jobject)iupImageGetImage(name, ih, 0, NULL);
}

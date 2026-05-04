/** \file
 * \brief JNI bridge: IupTabsHelper (TABCHANGE_CB dispatch).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupTabsHelper_reorder);

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTabsHelper_dispatchTabChange(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint new_pos)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  int old_pos = iupAttribGetInt(ih, "_IUPANDROID_ACTIVE_TAB");
  if (old_pos == new_pos) return;
  iupAttribSetInt(ih, "_IUPANDROID_ACTIVE_TAB", new_pos);

  Ihandle* new_child = IupGetChild(ih, new_pos);
  Ihandle* old_child = IupGetChild(ih, old_pos);

  IFnnn cb_change = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  if (cb_change) cb_change(ih, new_child, old_child);

  IFnii cb_change_pos = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
  if (cb_change_pos) cb_change_pos(ih, new_pos, old_pos);

  char buf[32];
  snprintf(buf, sizeof(buf), "%d", new_pos);
  iupAttribSet(ih, "VALUEPOS", buf);
}


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTabsHelper_dispatchReorder(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint source_pos, jint target_pos)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || source_pos == target_pos) return;

  IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  if (cb && cb(ih, source_pos, target_pos) == IUP_IGNORE) return;

  Ihandle* src = IupGetChild(ih, source_pos);
  if (!src) return;

  jclass java_class = IUPJNI_FindClass(IupTabsHelper_reorder, jni_env, "io/github/gen2brain/iupgo/IupTabsHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "moveTab", "(Landroid/view/View;II)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, source_pos, target_pos);
  iupAndroid_CheckException(jni_env, "IupTabsHelper.moveTab");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  int ref_pos = (source_pos < target_pos) ? target_pos + 1 : target_pos;
  Ihandle* ref = IupGetChild(ih, ref_pos);
  iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
  IupReparent(src, ih, ref);
  iupAttribSet(ih, "_IUPTABS_REORDERING", NULL);
}


/* TABCLOSE_CB: DEFAULT/no-cb hides; CONTINUE removes + destroys child; IGNORE keeps */
JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTabsHelper_dispatchTabClose(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint pos)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
  int ret = cb ? cb(ih, pos) : IUP_DEFAULT;

  if (ret == IUP_IGNORE) return;

  if (ret == IUP_CONTINUE)
  {
    Ihandle* child = IupGetChild(ih, pos);
    if (child) IupDestroy(child);
    return;
  }

  /* IUP_DEFAULT or no callback: hide the tab via the indexed setter. */
  IupSetAttributeId(ih, "TABVISIBLE", pos, "NO");
}

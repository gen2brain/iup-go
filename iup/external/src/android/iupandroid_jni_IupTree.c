/** \file
 * \brief JNI bridge: IupTreeHelper (EXECUTELEAF_CB, BRANCHOPEN_CB, BRANCHCLOSE_CB, SELECTION_CB).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_attrib.h"
#include "iup_object.h"

#include "iupandroid_drv.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchExecuteLeaf(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint id)
{
  (void)env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFni cb = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
  if (!cb) return;
  int ret = cb(ih, (int)id);
  if (ret == IUP_CLOSE) IupExitLoop();
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchBranchOpen(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint id)
{
  (void)env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFni cb = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
  if (!cb) return;
  int ret = cb(ih, (int)id);
  if (ret == IUP_CLOSE) IupExitLoop();
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchBranchClose(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint id)
{
  (void)env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFni cb = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
  if (!cb) return;
  int ret = cb(ih, (int)id);
  if (ret == IUP_CLOSE) IupExitLoop();
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchSelection(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint id, jint status)
{
  (void)env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFnii cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (!cb) return;
  int ret = cb(ih, (int)id, (int)status);
  if (ret == IUP_CLOSE) IupExitLoop();
}

/* returns 1 if RIGHTCLICK_CB consumed; Java falls back to SHOWRENAME otherwise */
JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchRightClick(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint id)
{
  (void)env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 0;

  IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
  if (!cb) return 0;
  int ret = cb(ih, (int)id);
  if (ret == IUP_CLOSE) IupExitLoop();
  return 1;
}

/* DRAGDROP_CB dispatch: default/CONTINUE = move, ctrl held = copy, IGNORE = reject */
JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchDragDrop(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint drag_id, jint drop_id, jint is_shift, jint is_control)
{
  (void)env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;
  if (drag_id == drop_id && !iupAttribGetBoolean(ih, "DROPEQUALDRAG")) return;

  int ret = IUP_CONTINUE;
  IFniiii cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
  if (cb)
  {
    ret = cb(ih, (int)drag_id, (int)drop_id, (int)is_shift, (int)is_control);
    if (ret == IUP_CLOSE) { IupExitLoop(); return; }
  }
  if (ret == IUP_IGNORE) return;
  if (ret != IUP_CONTINUE && cb) return;  /* IUP_DEFAULT with explicit cb leaves layout alone */

  char buf[16];
  snprintf(buf, sizeof(buf), "%d", (int)drop_id);
  if (is_control)
    IupSetAttributeId(ih, "COPYNODE", drag_id, buf);
  else
    IupSetAttributeId(ih, "MOVENODE", drag_id, buf);
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchShowRename(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint id)
{
  (void)env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 0;

  IFni cb = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (!cb) return 0;
  int ret = cb(ih, (int)id);
  if (ret == IUP_CLOSE) IupExitLoop();
  return (ret == IUP_IGNORE) ? 1 : 0;
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupTreeHelper_dispatchRename(
    JNIEnv* env, jclass cls, jlong ihandle_ptr, jint id, jstring j_title)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 0;

  IFnis cb = (IFnis)IupGetCallback(ih, "RENAME_CB");
  if (!cb) return 0;

  const char* title = j_title ? (*env)->GetStringUTFChars(env, j_title, NULL) : "";
  int ret = cb(ih, (int)id, (char*)title);
  if (j_title) (*env)->ReleaseStringUTFChars(env, j_title, title);
  if (ret == IUP_CLOSE) IupExitLoop();
  return (ret == IUP_IGNORE) ? 1 : 0;
}

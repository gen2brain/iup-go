/** \file
 * \brief Android IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupMessageDialogHelper);

#define ANDROID_MSGDLG_HELP     -1
#define ANDROID_MSGDLG_CANCELED  0

static jstring androidMessageDlgNewString(JNIEnv* jni_env, const char* str)
{
  return str ? (*jni_env)->NewStringUTF(jni_env, str) : NULL;
}

/* PARENTDIALOG resolves to an Activity or a detached ViewGroup; Java handles both */
static jobject androidMessageDlgResolveParent(Ihandle* ih)
{
  const char* parent_name = iupAttribGet(ih, "PARENTDIALOG");
  if (!parent_name) return NULL;

  Ihandle* parent = IupGetHandle(parent_name);
  if (!parent || !iupObjectCheck(parent)) return NULL;

  return (jobject)parent->handle;
}

static int androidMessageDlgInvoke(Ihandle* ih, const char* buttons)
{
  const char* title      = iupAttribGet(ih, "TITLE");
  const char* value      = iupAttribGet(ih, "VALUE");
  const char* dialogtype = iupAttribGetStr(ih, "DIALOGTYPE");
  const char* btn_style  = iupAttribGet(ih, "BUTTONSTYLE");
  const char* corner     = iupAttribGet(ih, "CORNERSTYLE");
  int button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  int has_help = IupGetCallback(ih, "HELP_CB") ? 1 : 0;

  const char* ok     = IupGetLanguageString("IUP_OK");
  const char* cancel = IupGetLanguageString("IUP_CANCEL");
  const char* yes    = IupGetLanguageString("IUP_YES");
  const char* no     = IupGetLanguageString("IUP_NO");
  const char* retry  = IupGetLanguageString("IUP_RETRY");
  const char* help   = IupGetLanguageString("IUP_HELP");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupMessageDialogHelper, jni_env, "io/github/gen2brain/iupgo/IupMessageDialogHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "showModal",
    "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

  jobject parent    = androidMessageDlgResolveParent(ih);
  jstring j_title   = androidMessageDlgNewString(jni_env, title);
  jstring j_value   = androidMessageDlgNewString(jni_env, value);
  jstring j_type    = androidMessageDlgNewString(jni_env, dialogtype);
  jstring j_buttons = androidMessageDlgNewString(jni_env, buttons);
  jstring j_ok      = androidMessageDlgNewString(jni_env, ok);
  jstring j_cancel  = androidMessageDlgNewString(jni_env, cancel);
  jstring j_yes     = androidMessageDlgNewString(jni_env, yes);
  jstring j_no      = androidMessageDlgNewString(jni_env, no);
  jstring j_retry   = androidMessageDlgNewString(jni_env, retry);
  jstring j_help    = androidMessageDlgNewString(jni_env, help);
  jstring j_style   = androidMessageDlgNewString(jni_env, btn_style);
  jstring j_corner  = androidMessageDlgNewString(jni_env, corner);

  jint response = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id,
    parent, j_title, j_value, j_type, j_buttons, (jint)button_def, (jboolean)has_help,
    j_ok, j_cancel, j_yes, j_no, j_retry, j_help, j_style, j_corner);
  iupAndroid_CheckException(jni_env, "IupMessageDialogHelper.showModal");

  if (j_title)   (*jni_env)->DeleteLocalRef(jni_env, j_title);
  if (j_value)   (*jni_env)->DeleteLocalRef(jni_env, j_value);
  if (j_type)    (*jni_env)->DeleteLocalRef(jni_env, j_type);
  if (j_buttons) (*jni_env)->DeleteLocalRef(jni_env, j_buttons);
  if (j_ok)      (*jni_env)->DeleteLocalRef(jni_env, j_ok);
  if (j_cancel)  (*jni_env)->DeleteLocalRef(jni_env, j_cancel);
  if (j_yes)     (*jni_env)->DeleteLocalRef(jni_env, j_yes);
  if (j_no)      (*jni_env)->DeleteLocalRef(jni_env, j_no);
  if (j_retry)   (*jni_env)->DeleteLocalRef(jni_env, j_retry);
  if (j_help)    (*jni_env)->DeleteLocalRef(jni_env, j_help);
  if (j_style)   (*jni_env)->DeleteLocalRef(jni_env, j_style);
  if (j_corner)  (*jni_env)->DeleteLocalRef(jni_env, j_corner);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return (int)response;
}

static int androidMessageDlgPopup(Ihandle* ih, int x, int y)
{
  const char* buttons;
  int response;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  buttons = iupAttribGetStr(ih, "BUTTONS");

  /* HELP dismisses and re-shows unless the callback returns IUP_CLOSE. */
  do
  {
    response = androidMessageDlgInvoke(ih, buttons);

    if (response == ANDROID_MSGDLG_HELP)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
      {
        if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
          response = 3;
        else if (iupStrEqualNoCase(buttons, "OK") || !buttons)
          response = 1;
        else
          response = 2;
      }
    }
  } while (response == ANDROID_MSGDLG_HELP);

  /* Fallback to the rightmost button on unexpected dismissal. */
  if (response == ANDROID_MSGDLG_CANCELED)
  {
    if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
      response = 3;
    else if (iupStrEqualNoCase(buttons, "OKCANCEL") ||
             iupStrEqualNoCase(buttons, "RETRYCANCEL") ||
             iupStrEqualNoCase(buttons, "YESNO"))
      response = 2;
    else
      response = 1;
  }

  IupSetInt(ih, "BUTTONRESPONSE", response);
  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = androidMessageDlgPopup;

  iupClassRegisterAttribute(ic, "BUTTONSTYLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE);
  iupClassRegisterAttribute(ic, "CORNERSTYLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE);
}

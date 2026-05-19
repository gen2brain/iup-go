/** \file
 * \brief Haiku MessageDlg (BAlert)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Alert.h>
#include <Button.h>

extern "C" {
#include "iup.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
}


/* BAlert::Go() is itself blocking and modal - perfect fit for IupMessageDlg's
 * DlgPopup contract. No nested run loop / window subset needed. */

static alert_type haikuAlertType(const char* dialog_type)
{
  if (!dialog_type) return B_INFO_ALERT;
  if (iupStrEqualNoCase(dialog_type, "ERROR"))       return B_STOP_ALERT;
  if (iupStrEqualNoCase(dialog_type, "WARNING"))     return B_WARNING_ALERT;
  if (iupStrEqualNoCase(dialog_type, "QUESTION"))    return B_IDEA_ALERT;
  if (iupStrEqualNoCase(dialog_type, "INFORMATION")) return B_INFO_ALERT;
  if (iupStrEqualNoCase(dialog_type, "MESSAGE"))     return B_EMPTY_ALERT;
  return B_INFO_ALERT;
}

static int haikuRunAlert(const char* title, const char* text, alert_type type, const char* b1, const char* b2, const char* b3, int default_idx)
{
  BAlert* alert = new BAlert(title ? title : "", text ? text : "", b1, b2, b3, B_WIDTH_AS_USUAL, type);
  if (default_idx >= 0)
    if (BButton* btn = alert->ButtonAt(default_idx)) btn->MakeDefault(true);
  /* Go() deletes the BAlert. */
  int32 r = alert->Go();
  return r + 1;
}

static int haikuMessageDlgPopup(Ihandle* ih, int /*x*/, int /*y*/)
{
  const char* title = iupAttribGet(ih, "TITLE");
  const char* text  = iupAttribGet(ih, "VALUE");
  const char* dlgt  = iupAttribGet(ih, "DIALOGTYPE");
  const char* btns  = iupAttribGet(ih, "BUTTONS");
  if (!btns) btns = "OK";
  int btn_default = iupAttribGetInt(ih, "BUTTONDEFAULT");

  alert_type at = haikuAlertType(dlgt);
  int result = 0;

  if (iupStrEqualNoCase(btns, "OKCANCEL"))
  {
    int def = (btn_default == 2) ? 0 : (btn_default == 1) ? 1 : -1;
    result = haikuRunAlert(title, text, at, "Cancel", "OK", NULL, def) == 2 ? 1 : 2;
  }
  else if (iupStrEqualNoCase(btns, "RETRYCANCEL"))
  {
    int def = (btn_default == 2) ? 0 : (btn_default == 1) ? 1 : -1;
    result = haikuRunAlert(title, text, at, "Cancel", "Retry", NULL, def) == 2 ? 1 : 2;
  }
  else if (iupStrEqualNoCase(btns, "YESNO"))
  {
    int def = (btn_default == 2) ? 0 : (btn_default == 1) ? 1 : -1;
    result = haikuRunAlert(title, text, at, "No", "Yes", NULL, def) == 2 ? 1 : 2;
  }
  else if (iupStrEqualNoCase(btns, "YESNOCANCEL"))
  {
    int def = (btn_default == 3) ? 0 : (btn_default == 2) ? 1 : (btn_default == 1) ? 2 : -1;
    int r = haikuRunAlert(title, text, at, "Cancel", "No", "Yes", def);
    if      (r == 3) result = 1;  /* Yes */
    else if (r == 2) result = 2;  /* No  */
    else             result = 3;  /* Cancel (also B_ESCAPE -> r==1) */
  }
  else /* "OK" or unknown */
    result = haikuRunAlert(title, text, at, "OK", NULL, NULL, btn_default == 1 ? 0 : -1);

  iupAttribSetInt(ih, "BUTTONRESPONSE", result);
  return IUP_NOERROR;
}

static char* haikuMessageDlgGetAutoModalAttrib(Ihandle* /*ih*/) { return (char*)"1"; }

extern "C" IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = haikuMessageDlgPopup;

  iupClassRegisterAttribute(ic, "AUTOMODAL", haikuMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
}

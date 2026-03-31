/** \file
 * \brief IupMessageDlg pre-defined dialog - FLTK implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/fl_ask.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
}

#include "iupfltk_drv.h"


static void fltkMessageDlgSetPosition(Ihandle* ih)
{
  Fl_Window* parent_win = iupfltkGetParentWidget(ih);
  if (parent_win)
    fl_message_position(parent_win);
}

static int fltkMessageDlgShowChoice(const char* value, const char* b0, const char* b1, const char* b2)
{
  if (b2)
    return fl_choice_n("%s", b0, b1, b2, value);
  else
    return fl_choice_n("%s", b0, b1, 0, value);
}

static int fltkMessageDlgPopup(Ihandle* ih, int x, int y)
{
  int response = 1;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  const char* value = iupAttribGet(ih, "VALUE");
  if (!value) value = "";

  const char* title = iupAttribGet(ih, "TITLE");
  if (!title)
  {
    Fl_Window* parent_win = iupfltkGetParentWidget(ih);
    if (parent_win && parent_win->label())
      title = parent_win->label();
  }
  if (title)
    fl_message_title(title);

  const char* dialogtype = iupAttribGetStr(ih, "DIALOGTYPE");
  const char* buttons = iupAttribGetStr(ih, "BUTTONS");

  const char* ok_str = IupGetLanguageString("IUP_OK");
  const char* cancel_str = IupGetLanguageString("IUP_CANCEL");
  const char* yes_str = IupGetLanguageString("IUP_YES");
  const char* no_str = IupGetLanguageString("IUP_NO");
  const char* retry_str = IupGetLanguageString("IUP_RETRY");

  if (!ok_str) ok_str = "OK";
  if (!cancel_str) cancel_str = "Cancel";
  if (!yes_str) yes_str = "Yes";
  if (!no_str) no_str = "No";
  if (!retry_str) retry_str = "Retry";

  int buttondefault = iupAttribGetInt(ih, "BUTTONDEFAULT");

  fltkMessageDlgSetPosition(ih);

  if (iupStrEqualNoCase(buttons, "OK") || !buttons)
  {
    if (iupStrEqualNoCase(dialogtype, "ERROR") || iupStrEqualNoCase(dialogtype, "WARNING"))
    {
      fl_message_icon_label("!");
      fl_alert("%s", value);
    }
    else if (iupStrEqualNoCase(dialogtype, "QUESTION"))
    {
      fl_message_icon_label("?");
      fl_message("%s", value);
    }
    else
      fl_message("%s", value);

    response = 1;
  }
  else if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    if (iupStrEqualNoCase(dialogtype, "ERROR") || iupStrEqualNoCase(dialogtype, "WARNING"))
      fl_message_icon_label("!");
    else if (iupStrEqualNoCase(dialogtype, "QUESTION"))
      fl_message_icon_label("?");

    if (buttondefault == 2)
    {
      int r = fltkMessageDlgShowChoice(value, ok_str, cancel_str, NULL);
      response = (r == 1) ? 2 : 1;
    }
    else
    {
      int r = fltkMessageDlgShowChoice(value, cancel_str, ok_str, NULL);
      response = (r == 1) ? 1 : 2;
    }
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    if (iupStrEqualNoCase(dialogtype, "ERROR") || iupStrEqualNoCase(dialogtype, "WARNING"))
      fl_message_icon_label("!");

    if (buttondefault == 2)
    {
      int r = fltkMessageDlgShowChoice(value, retry_str, cancel_str, NULL);
      response = (r == 1) ? 2 : 1;
    }
    else
    {
      int r = fltkMessageDlgShowChoice(value, cancel_str, retry_str, NULL);
      response = (r == 1) ? 1 : 2;
    }
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    if (buttondefault == 2)
    {
      int r = fltkMessageDlgShowChoice(value, yes_str, no_str, NULL);
      response = (r == 1) ? 2 : 1;
    }
    else
    {
      int r = fltkMessageDlgShowChoice(value, no_str, yes_str, NULL);
      response = (r == 1) ? 1 : 2;
    }
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    if (buttondefault == 2)
    {
      int r = fltkMessageDlgShowChoice(value, cancel_str, no_str, yes_str);
      response = (r == 0) ? 3 : (r == 1) ? 2 : 1;
    }
    else if (buttondefault == 3)
    {
      int r = fltkMessageDlgShowChoice(value, yes_str, cancel_str, no_str);
      response = (r == 0) ? 1 : (r == 1) ? 3 : 2;
    }
    else
    {
      int r = fltkMessageDlgShowChoice(value, cancel_str, yes_str, no_str);
      if (r == 1)
        response = 1;
      else if (r == 2)
        response = 2;
      else
        response = 3;
    }
  }

  iupAttribSet(ih, "BUTTONRESPONSE", iupStrReturnInt(response));

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = fltkMessageDlgPopup;
}

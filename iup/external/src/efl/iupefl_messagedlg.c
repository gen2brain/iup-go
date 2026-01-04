/** \file
 * \brief IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iupefl_drv.h"


static int efl_msgdlg_response = 0;

static void eflMessageDlgButtonCallback(void* data, const Efl_Event* ev)
{
  int response = (int)(intptr_t)data;

  (void)ev;

  efl_msgdlg_response = response;
  iupeflModalLoopQuit();
}

static void eflMessageDlgCloseCallback(void* data, const Efl_Event* ev)
{
  (void)data;
  (void)ev;

  efl_msgdlg_response = 0;
  iupeflModalLoopQuit();
}

static void eflMessageDlgMeasureText(Evas* evas, const char* text, int* w, int* h)
{
  Evas_Object* measure;
  int tw = 0, th = 0;

  if (!text || !text[0])
  {
    *w = 0;
    *h = 16;
    return;
  }

  measure = evas_object_text_add(evas);
  evas_object_text_font_set(measure, "Sans", 12);
  evas_object_text_text_set(measure, text);
  evas_object_geometry_get(measure, NULL, NULL, &tw, &th);
  evas_object_del(measure);

  if (tw <= 0) tw = (int)strlen(text) * 8;
  if (th <= 0) th = 16;

  *w = tw;
  *h = th;
}

static const char* eflMessageDlgGetIconName(const char* dialogtype)
{
  if (iupStrEqualNoCase(dialogtype, "ERROR"))
    return "dialog-error";
  else if (iupStrEqualNoCase(dialogtype, "WARNING"))
    return "dialog-warning";
  else if (iupStrEqualNoCase(dialogtype, "INFORMATION"))
    return "dialog-information";
  else if (iupStrEqualNoCase(dialogtype, "QUESTION"))
    return "dialog-question";
  return NULL;
}

static int eflMessageDlgPopup(Ihandle* ih, int x, int y)
{
  Eo* win;
  Eo* vbox;
  Eo* content_hbox;
  Eo* label;
  Eo* button_hbox;
  Eo* btn1 = NULL;
  Eo* btn2 = NULL;
  Eo* btn3 = NULL;
  char *buttons, *title, *value, *dialogtype;
  const char *ok, *cancel, *yes, *no, *help, *retry;
  const char *icon_name;
  int button_def;
  int text_w = 0, text_h = 0;
  int win_w, win_h;
  int num_buttons = 1;
  int has_icon = 0;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  title = iupAttribGet(ih, "TITLE");

  win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                efl_text_set(efl_added, title ? iupeflStrConvertToSystem(title) : "Message"),
                efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_DIALOG_BASIC),
                efl_ui_win_autodel_set(efl_added, EINA_FALSE));
  if (!win)
    return IUP_ERROR;

  efl_event_callback_add(win, EFL_UI_WIN_EVENT_DELETE_REQUEST, eflMessageDlgCloseCallback, NULL);

  vbox = efl_add(EFL_UI_BOX_CLASS, win,
                 efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
                 efl_gfx_arrangement_content_padding_set(efl_added, 0, 10),
                 efl_gfx_hint_margin_set(efl_added, 15, 15, 15, 15),
                 efl_content_set(win, efl_added));

  dialogtype = iupAttribGetStr(ih, "DIALOGTYPE");
  icon_name = eflMessageDlgGetIconName(dialogtype);

  content_hbox = efl_add(EFL_UI_BOX_CLASS, win,
                         efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                         efl_gfx_arrangement_content_padding_set(efl_added, 10, 0),
                         efl_gfx_hint_weight_set(efl_added, 1.0, 1.0),
                         efl_gfx_hint_align_set(efl_added, -1.0, -1.0),
                         efl_pack(vbox, efl_added));

  if (icon_name)
  {
    Evas_Object* icon = elm_icon_add(win);
    if (icon && elm_icon_standard_set(icon, icon_name))
    {
      evas_object_size_hint_min_set(icon, 48, 48);
      evas_object_size_hint_max_set(icon, 48, 48);
      efl_gfx_hint_weight_set(icon, 0.0, 0.0);
      efl_gfx_hint_align_set(icon, 0.5, 0.5);
      efl_pack(content_hbox, icon);
      evas_object_show(icon);
      has_icon = 1;
    }
    else if (icon)
    {
      evas_object_del(icon);
    }
  }

  value = iupAttribGet(ih, "VALUE");

  label = efl_add(EFL_UI_TEXTBOX_CLASS, win,
                  efl_text_interactive_editable_set(efl_added, EINA_FALSE),
                  efl_text_multiline_set(efl_added, EINA_TRUE),
                  efl_text_wrap_set(efl_added, EFL_TEXT_FORMAT_WRAP_WORD),
                  efl_text_horizontal_align_set(efl_added, has_icon ? 0.0 : 0.5),
                  efl_text_vertical_align_set(efl_added, 0.5),
                  efl_gfx_hint_weight_set(efl_added, 1.0, 1.0),
                  efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                  efl_pack(content_hbox, efl_added));

  if (value)
  {
    efl_text_set(label, iupeflStrConvertToSystem(value));
    eflMessageDlgMeasureText(evas_object_evas_get(win), value, &text_w, &text_h);
  }

  button_hbox = efl_add(EFL_UI_BOX_CLASS, win,
                        efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                        efl_gfx_arrangement_content_padding_set(efl_added, 10, 0),
                        efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                        efl_gfx_hint_align_set(efl_added, -1.0, 1.0),
                        efl_pack(vbox, efl_added));

  ok = IupGetLanguageString("IUP_OK");
  cancel = IupGetLanguageString("IUP_CANCEL");
  yes = IupGetLanguageString("IUP_YES");
  no = IupGetLanguageString("IUP_NO");
  help = IupGetLanguageString("IUP_HELP");
  retry = IupGetLanguageString("IUP_RETRY");

  buttons = iupAttribGetStr(ih, "BUTTONS");

  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    num_buttons = 2;

    btn1 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, ok),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)1),
                   efl_pack(button_hbox, efl_added));

    btn2 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, cancel),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)2),
                   efl_pack(button_hbox, efl_added));
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    num_buttons = 2;

    btn1 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, retry),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)1),
                   efl_pack(button_hbox, efl_added));

    btn2 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, cancel),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)2),
                   efl_pack(button_hbox, efl_added));
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    num_buttons = 2;

    btn1 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, yes),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)1),
                   efl_pack(button_hbox, efl_added));

    btn2 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, no),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)2),
                   efl_pack(button_hbox, efl_added));
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    num_buttons = 3;

    btn1 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, yes),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)1),
                   efl_pack(button_hbox, efl_added));

    btn2 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, no),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)2),
                   efl_pack(button_hbox, efl_added));

    btn3 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, cancel),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)3),
                   efl_pack(button_hbox, efl_added));
  }
  else
  {
    num_buttons = 1;

    btn1 = efl_add(EFL_UI_BUTTON_CLASS, win,
                   efl_text_set(efl_added, ok),
                   efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                   efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
                   efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)1),
                   efl_pack(button_hbox, efl_added));
  }

  if (IupGetCallback(ih, "HELP_CB"))
  {
    num_buttons++;
    efl_add(EFL_UI_BUTTON_CLASS, win,
            efl_text_set(efl_added, help),
            efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
            efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
            efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflMessageDlgButtonCallback, (void*)(-1)),
            efl_pack(button_hbox, efl_added));
  }

  win_w = text_w + 60;
  if (has_icon)
    win_w += 58;
  if (win_w < 250) win_w = 250;
  if (win_w < num_buttons * 90) win_w = num_buttons * 90;
  if (win_w > 500) win_w = 500;

  win_h = text_h + 100;
  if (has_icon && win_h < 140)
    win_h = 140;
  else if (win_h < 120)
    win_h = 120;

  efl_gfx_entity_size_set(win, EINA_SIZE2D(win_w, win_h));
  efl_gfx_entity_visible_set(win, EINA_TRUE);
  efl_ui_win_center(win, EINA_TRUE, EINA_TRUE);
  elm_win_activate(win);

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def == 3 && btn3)
    efl_ui_focus_util_focus(btn3);
  else if (button_def == 2 && btn2)
    efl_ui_focus_util_focus(btn2);
  else if (btn1)
    efl_ui_focus_util_focus(btn1);

  efl_msgdlg_response = 0;

  do
  {
    iupeflModalLoopRun();

    if (efl_msgdlg_response == -1)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
      {
        if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
          efl_msgdlg_response = 3;
        else if (iupStrEqualNoCase(buttons, "OK"))
          efl_msgdlg_response = 1;
        else
          efl_msgdlg_response = 2;
      }
    }
  } while (efl_msgdlg_response == -1);

  if (efl_msgdlg_response == 3)
    IupSetAttribute(ih, "BUTTONRESPONSE", "3");
  else if (efl_msgdlg_response == 2)
    IupSetAttribute(ih, "BUTTONRESPONSE", "2");
  else
    IupSetAttribute(ih, "BUTTONRESPONSE", "1");

  efl_del(win);

  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = eflMessageDlgPopup;
}

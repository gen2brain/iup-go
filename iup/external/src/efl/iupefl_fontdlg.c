/** \file
 * \brief IupFontDlg pre-defined dialog - EFL Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iupefl_drv.h"

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"


static int efl_fontdlg_status = 0;
static char efl_fontdlg_value[256] = "";

static void eflFontDlgOkCallback(void* data, const Efl_Event* ev)
{
  (void)data;
  (void)ev;

  efl_fontdlg_status = 1;
  iupeflModalLoopQuit();
}

static void eflFontDlgCancelCallback(void* data, const Efl_Event* ev)
{
  (void)data;
  (void)ev;

  efl_fontdlg_status = 0;
  iupeflModalLoopQuit();
}

static void eflFontDlgAddItem(Eo* list, const char* text)
{
  Eo* item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, list);
  if (item)
  {
    efl_text_set(item, text);
    efl_pack_end(list, item);
  }
}

static int eflFontDlgPopup(Ihandle* ih, int x, int y)
{
  Eo* win;
  Eo* box;
  Eo* list;
  Eo* btn_box;
  Eo* btn_ok;
  Eo* btn_cancel;
  char* title;
  char* font;
  const char* ok_str;
  const char* cancel_str;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  title = iupAttribGet(ih, "TITLE");
  if (!title)
    title = (char*)IupGetLanguageString("IUP_FONTDLG");

  win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
    efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_DIALOG_BASIC),
    efl_text_set(efl_added, iupeflStrConvertToSystem(title)));
  if (!win)
    return IUP_ERROR;

  box = efl_add(EFL_UI_BOX_CLASS, win,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
    efl_gfx_arrangement_content_padding_set(efl_added, 0, 10),
    efl_gfx_hint_margin_set(efl_added, 10, 10, 10, 10),
    efl_gfx_hint_weight_set(efl_added, 1.0, 1.0),
    efl_content_set(win, efl_added));

  list = efl_add(EFL_UI_LIST_CLASS, box);
  efl_gfx_hint_weight_set(list, 1.0, 1.0);
  efl_gfx_hint_fill_set(list, EINA_TRUE, EINA_TRUE);

  eflFontDlgAddItem(list, "Sans 10");
  eflFontDlgAddItem(list, "Sans 12");
  eflFontDlgAddItem(list, "Sans 14");
  eflFontDlgAddItem(list, "Sans 16");
  eflFontDlgAddItem(list, "Sans 18");
  eflFontDlgAddItem(list, "Sans 24");
  eflFontDlgAddItem(list, "Serif 10");
  eflFontDlgAddItem(list, "Serif 12");
  eflFontDlgAddItem(list, "Serif 14");
  eflFontDlgAddItem(list, "Serif 16");
  eflFontDlgAddItem(list, "Monospace 10");
  eflFontDlgAddItem(list, "Monospace 12");
  eflFontDlgAddItem(list, "Monospace 14");

  efl_pack_end(box, list);

  btn_box = efl_add(EFL_UI_BOX_CLASS, box,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
    efl_gfx_arrangement_content_padding_set(efl_added, 10, 0),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
    efl_gfx_hint_align_set(efl_added, -1.0, 1.0));

  ok_str = IupGetLanguageString("IUP_OK");
  cancel_str = IupGetLanguageString("IUP_CANCEL");

  btn_ok = efl_add(EFL_UI_BUTTON_CLASS, btn_box,
    efl_text_set(efl_added, ok_str),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
    efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
    efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflFontDlgOkCallback, ih));
  efl_pack_end(btn_box, btn_ok);

  btn_cancel = efl_add(EFL_UI_BUTTON_CLASS, btn_box,
    efl_text_set(efl_added, cancel_str),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
    efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
    efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflFontDlgCancelCallback, ih));
  efl_pack_end(btn_box, btn_cancel);

  efl_pack_end(box, btn_box);

  efl_gfx_entity_size_set(win, EINA_SIZE2D(300, 400));
  efl_gfx_entity_visible_set(win, EINA_TRUE);

  font = iupAttribGet(ih, "VALUE");
  if (font)
    strncpy(efl_fontdlg_value, font, sizeof(efl_fontdlg_value) - 1);
  else
    strcpy(efl_fontdlg_value, "Sans 12");

  efl_fontdlg_status = 0;

  iupeflModalLoopRun();

  if (efl_fontdlg_status == 1)
  {
    Eo* sel = efl_ui_selectable_last_selected_get(list);
    if (sel)
    {
      const char* text = efl_text_get(sel);
      if (text)
        iupAttribSetStr(ih, "VALUE", text);
    }
    IupSetAttribute(ih, "STATUS", "1");
  }
  else
  {
    IupSetAttribute(ih, "STATUS", "0");
  }

  efl_event_callback_del(btn_ok, EFL_INPUT_EVENT_CLICKED, eflFontDlgOkCallback, ih);
  efl_event_callback_del(btn_cancel, EFL_INPUT_EVENT_CLICKED, eflFontDlgCancelCallback, ih);

  efl_del(win);

  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = eflFontDlgPopup;
}

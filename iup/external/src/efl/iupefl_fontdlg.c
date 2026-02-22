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
#include "iup_drvfont.h"


static const int efl_fontdlg_sizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
#define EFL_FONTDLG_SIZE_COUNT (sizeof(efl_fontdlg_sizes) / sizeof(efl_fontdlg_sizes[0]))

typedef struct {
  Eo* family_list;
  Eo* style_list;
  Eo* size_list;
  Eo* preview_label;

  Eina_Hash* fonts_hash;
  char** sorted_families;
  int family_count;

  char selected_family[256];
  char selected_style[64];
  int selected_size;

  int status;
} EflFontDlgData;

static int eflFontDlgFamilyCompare(const void* a, const void* b)
{
  return strcasecmp(*(const char**)a, *(const char**)b);
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

static void eflFontDlgUpdatePreview(EflFontDlgData* dlg)
{
  char font_str[400];
  char style_str[128] = "";

  if (dlg->selected_style[0] &&
      strcasecmp(dlg->selected_style, "Regular") != 0 &&
      strcasecmp(dlg->selected_style, "Normal") != 0 &&
      strcasecmp(dlg->selected_style, "Book") != 0 &&
      strcasecmp(dlg->selected_style, "Roman") != 0)
  {
    snprintf(style_str, sizeof(style_str), ":style=%s", dlg->selected_style);
  }

  snprintf(font_str, sizeof(font_str),
    "DEFAULT='font=%s%s font_size=%d'",
    dlg->selected_family, style_str, dlg->selected_size);

  evas_object_textblock_style_set(dlg->preview_label, NULL);
  {
    Evas_Textblock_Style* ts = evas_textblock_style_new();
    evas_textblock_style_set(ts, font_str);
    evas_object_textblock_style_set(dlg->preview_label, ts);
    evas_textblock_style_free(ts);
  }
  evas_object_textblock_text_markup_set(dlg->preview_label, "AaBbYyZz 0123");
}

static void eflFontDlgPopulateStyles(EflFontDlgData* dlg, const char* family)
{
  Elm_Font_Properties* props;

  efl_pack_clear(dlg->style_list);

  props = eina_hash_find(dlg->fonts_hash, family);
  if (props && props->styles)
  {
    Eina_List* l;
    const char* style;
    EINA_LIST_FOREACH(props->styles, l, style)
    {
      eflFontDlgAddItem(dlg->style_list, style);
    }
  }
  else
  {
    eflFontDlgAddItem(dlg->style_list, "Regular");
  }
}

static void eflFontDlgFamilySelectedCallback(void* data, const Efl_Event* ev)
{
  EflFontDlgData* dlg = (EflFontDlgData*)data;
  Eo* item;
  const char* text;

  (void)ev;

  item = efl_ui_selectable_last_selected_get(dlg->family_list);
  if (!item)
    return;

  text = efl_text_get(item);
  if (!text)
    return;

  strncpy(dlg->selected_family, text, sizeof(dlg->selected_family) - 1);
  dlg->selected_family[sizeof(dlg->selected_family) - 1] = 0;

  eflFontDlgPopulateStyles(dlg, text);

  strcpy(dlg->selected_style, "Regular");

  eflFontDlgUpdatePreview(dlg);
}

static void eflFontDlgStyleSelectedCallback(void* data, const Efl_Event* ev)
{
  EflFontDlgData* dlg = (EflFontDlgData*)data;
  Eo* item;
  const char* text;

  (void)ev;

  item = efl_ui_selectable_last_selected_get(dlg->style_list);
  if (!item)
    return;

  text = efl_text_get(item);
  if (!text)
    return;

  strncpy(dlg->selected_style, text, sizeof(dlg->selected_style) - 1);
  dlg->selected_style[sizeof(dlg->selected_style) - 1] = 0;

  eflFontDlgUpdatePreview(dlg);
}

static void eflFontDlgSizeSelectedCallback(void* data, const Efl_Event* ev)
{
  EflFontDlgData* dlg = (EflFontDlgData*)data;
  Eo* item;
  const char* text;

  (void)ev;

  item = efl_ui_selectable_last_selected_get(dlg->size_list);
  if (!item)
    return;

  text = efl_text_get(item);
  if (!text)
    return;

  dlg->selected_size = atoi(text);
  if (dlg->selected_size <= 0)
    dlg->selected_size = 12;

  eflFontDlgUpdatePreview(dlg);
}

static void eflFontDlgOkCallback(void* data, const Efl_Event* ev)
{
  EflFontDlgData* dlg = (EflFontDlgData*)data;
  (void)ev;
  dlg->status = 1;
  iupeflModalLoopQuit();
}

static void eflFontDlgCancelCallback(void* data, const Efl_Event* ev)
{
  EflFontDlgData* dlg = (EflFontDlgData*)data;
  (void)ev;
  dlg->status = 0;
  iupeflModalLoopQuit();
}

static void eflFontDlgBuildFontList(EflFontDlgData* dlg, Evas* evas)
{
  Eina_List* fonts_list;
  Eina_Iterator* it;
  const char* key;
  int capacity = 256;
  int count = 0;

  fonts_list = evas_font_available_list(evas);
  if (!fonts_list)
    return;

  dlg->fonts_hash = elm_font_available_hash_add(fonts_list);
  if (!dlg->fonts_hash)
  {
    evas_font_available_list_free(evas, fonts_list);
    return;
  }

  dlg->sorted_families = (char**)malloc(capacity * sizeof(char*));

  it = eina_hash_iterator_key_new(dlg->fonts_hash);
  EINA_ITERATOR_FOREACH(it, key)
  {
    if (count >= capacity)
    {
      capacity *= 2;
      dlg->sorted_families = (char**)realloc(dlg->sorted_families, capacity * sizeof(char*));
    }
    dlg->sorted_families[count] = strdup(key);
    count++;
  }
  eina_iterator_free(it);

  dlg->family_count = count;

  qsort(dlg->sorted_families, count, sizeof(char*), eflFontDlgFamilyCompare);

  evas_font_available_list_free(evas, fonts_list);
}

static void eflFontDlgFreeFontList(EflFontDlgData* dlg)
{
  int i;
  if (dlg->sorted_families)
  {
    for (i = 0; i < dlg->family_count; i++)
      free(dlg->sorted_families[i]);
    free(dlg->sorted_families);
    dlg->sorted_families = NULL;
  }

  if (dlg->fonts_hash)
  {
    elm_font_available_hash_del(dlg->fonts_hash);
    dlg->fonts_hash = NULL;
  }
}

static void eflFontDlgBuildValueString(EflFontDlgData* dlg, char* result, int result_size)
{
  int has_style = 0;

  if (dlg->selected_style[0] &&
      strcasecmp(dlg->selected_style, "Regular") != 0 &&
      strcasecmp(dlg->selected_style, "Normal") != 0 &&
      strcasecmp(dlg->selected_style, "Book") != 0 &&
      strcasecmp(dlg->selected_style, "Roman") != 0)
  {
    has_style = 1;
  }

  if (has_style)
    snprintf(result, result_size, "%s, %s %d", dlg->selected_family, dlg->selected_style, dlg->selected_size);
  else
    snprintf(result, result_size, "%s, %d", dlg->selected_family, dlg->selected_size);
}

static int eflFontDlgPopup(Ihandle* ih, int x, int y)
{
  Eo* win;
  Eo* main_box;
  Eo* lists_box;
  Eo* preview_box;
  Eo* btn_box;
  Eo* btn_ok;
  Eo* btn_cancel;
  Eo* family_label;
  Eo* style_label;
  Eo* size_label;
  Eo* family_box;
  Eo* style_box;
  Eo* size_box;
  char* title;
  char* font;
  const char* ok_str;
  const char* cancel_str;
  Evas* evas;
  EflFontDlgData dlg;
  int i;

  memset(&dlg, 0, sizeof(dlg));
  dlg.selected_size = 12;
  strcpy(dlg.selected_family, "Sans");
  strcpy(dlg.selected_style, "Regular");

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

  evas = evas_object_evas_get(win);
  eflFontDlgBuildFontList(&dlg, evas);

  main_box = efl_add(EFL_UI_BOX_CLASS, win,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
    efl_gfx_arrangement_content_padding_set(efl_added, 0, 5),
    efl_gfx_hint_margin_set(efl_added, 10, 10, 10, 10),
    efl_gfx_hint_weight_set(efl_added, 1.0, 1.0),
    efl_content_set(win, efl_added));

  lists_box = efl_add(EFL_UI_BOX_CLASS, main_box,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
    efl_gfx_arrangement_content_padding_set(efl_added, 5, 0),
    efl_gfx_hint_weight_set(efl_added, 1.0, 1.0),
    efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_TRUE));

  /* Family column */
  family_box = efl_add(EFL_UI_BOX_CLASS, lists_box,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
    efl_gfx_arrangement_content_padding_set(efl_added, 0, 2),
    efl_gfx_hint_weight_set(efl_added, 1.0, 1.0),
    efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_TRUE));

  family_label = efl_add(EFL_UI_TEXTBOX_CLASS, family_box,
    efl_text_set(efl_added, IupGetLanguageString("IUP_FAMILY")),
    efl_text_interactive_editable_set(efl_added, EINA_FALSE),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.0));
  efl_pack_end(family_box, family_label);

  dlg.family_list = efl_add(EFL_UI_LIST_CLASS, family_box);
  efl_gfx_hint_weight_set(dlg.family_list, 1.0, 1.0);
  efl_gfx_hint_fill_set(dlg.family_list, EINA_TRUE, EINA_TRUE);

  for (i = 0; i < dlg.family_count; i++)
    eflFontDlgAddItem(dlg.family_list, dlg.sorted_families[i]);

  efl_event_callback_add(dlg.family_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED,
    eflFontDlgFamilySelectedCallback, &dlg);

  efl_pack_end(family_box, dlg.family_list);
  efl_pack_end(lists_box, family_box);

  /* Style column */
  style_box = efl_add(EFL_UI_BOX_CLASS, lists_box,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
    efl_gfx_arrangement_content_padding_set(efl_added, 0, 2),
    efl_gfx_hint_weight_set(efl_added, 0.4, 1.0),
    efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_TRUE));

  style_label = efl_add(EFL_UI_TEXTBOX_CLASS, style_box,
    efl_text_set(efl_added, IupGetLanguageString("IUP_STYLE")),
    efl_text_interactive_editable_set(efl_added, EINA_FALSE),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.0));
  efl_pack_end(style_box, style_label);

  dlg.style_list = efl_add(EFL_UI_LIST_CLASS, style_box);
  efl_gfx_hint_weight_set(dlg.style_list, 1.0, 1.0);
  efl_gfx_hint_fill_set(dlg.style_list, EINA_TRUE, EINA_TRUE);

  eflFontDlgAddItem(dlg.style_list, "Regular");

  efl_event_callback_add(dlg.style_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED,
    eflFontDlgStyleSelectedCallback, &dlg);

  efl_pack_end(style_box, dlg.style_list);
  efl_pack_end(lists_box, style_box);

  /* Size column */
  size_box = efl_add(EFL_UI_BOX_CLASS, lists_box,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
    efl_gfx_arrangement_content_padding_set(efl_added, 0, 2),
    efl_gfx_hint_weight_set(efl_added, 0.3, 1.0),
    efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_TRUE));

  size_label = efl_add(EFL_UI_TEXTBOX_CLASS, size_box,
    efl_text_set(efl_added, IupGetLanguageString("IUP_SIZE")),
    efl_text_interactive_editable_set(efl_added, EINA_FALSE),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.0));
  efl_pack_end(size_box, size_label);

  dlg.size_list = efl_add(EFL_UI_LIST_CLASS, size_box);
  efl_gfx_hint_weight_set(dlg.size_list, 1.0, 1.0);
  efl_gfx_hint_fill_set(dlg.size_list, EINA_TRUE, EINA_TRUE);

  for (i = 0; i < (int)EFL_FONTDLG_SIZE_COUNT; i++)
  {
    char size_str[16];
    snprintf(size_str, sizeof(size_str), "%d", efl_fontdlg_sizes[i]);
    eflFontDlgAddItem(dlg.size_list, size_str);
  }

  efl_event_callback_add(dlg.size_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED,
    eflFontDlgSizeSelectedCallback, &dlg);

  efl_pack_end(size_box, dlg.size_list);
  efl_pack_end(lists_box, size_box);

  efl_pack_end(main_box, lists_box);

  /* Preview area */
  preview_box = efl_add(EFL_UI_BOX_CLASS, main_box,
    efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.2),
    efl_gfx_hint_fill_set(efl_added, EINA_TRUE, EINA_TRUE));

  dlg.preview_label = evas_object_textblock_add(evas);
  {
    Evas_Textblock_Style* ts = evas_textblock_style_new();
    evas_textblock_style_set(ts, "DEFAULT='font=Sans font_size=12'");
    evas_object_textblock_style_set(dlg.preview_label, ts);
    evas_textblock_style_free(ts);
  }
  evas_object_textblock_text_markup_set(dlg.preview_label, "AaBbYyZz 0123");
  efl_gfx_hint_weight_set(dlg.preview_label, 1.0, 1.0);
  efl_gfx_hint_fill_set(dlg.preview_label, EINA_TRUE, EINA_TRUE);
  evas_object_show(dlg.preview_label);
  efl_pack_end(preview_box, dlg.preview_label);

  efl_pack_end(main_box, preview_box);

  /* Buttons */
  btn_box = efl_add(EFL_UI_BOX_CLASS, main_box,
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
    efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflFontDlgOkCallback, &dlg));
  efl_pack_end(btn_box, btn_ok);

  btn_cancel = efl_add(EFL_UI_BUTTON_CLASS, btn_box,
    efl_text_set(efl_added, cancel_str),
    efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
    efl_gfx_hint_align_set(efl_added, -1.0, 0.5),
    efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, eflFontDlgCancelCallback, &dlg));
  efl_pack_end(btn_box, btn_cancel);

  efl_pack_end(main_box, btn_box);

  /* Parse initial VALUE to pre-select */
  font = iupAttribGet(ih, "VALUE");
  if (font)
  {
    char typeface[256] = "";
    int fontsize = 0;
    int bold = 0, italic = 0, underline = 0, strikeout = 0;

    if (iupFontParsePango(font, typeface, &fontsize, &bold, &italic, &underline, &strikeout))
    {
      if (typeface[0])
        strncpy(dlg.selected_family, typeface, sizeof(dlg.selected_family) - 1);
      if (fontsize > 0)
        dlg.selected_size = fontsize;

      if (bold && italic)
        strcpy(dlg.selected_style, "Bold Italic");
      else if (bold)
        strcpy(dlg.selected_style, "Bold");
      else if (italic)
        strcpy(dlg.selected_style, "Italic");
      else
        strcpy(dlg.selected_style, "Regular");

      eflFontDlgPopulateStyles(&dlg, dlg.selected_family);
    }
  }

  eflFontDlgUpdatePreview(&dlg);

  efl_gfx_entity_size_set(win, EINA_SIZE2D(500, 450));
  efl_gfx_entity_visible_set(win, EINA_TRUE);

  dlg.status = 0;

  iupeflModalLoopRun();

  if (dlg.status == 1)
  {
    char value[400];
    eflFontDlgBuildValueString(&dlg, value, sizeof(value));
    iupAttribSetStr(ih, "VALUE", value);
    IupSetAttribute(ih, "STATUS", "1");
  }
  else
  {
    IupSetAttribute(ih, "STATUS", "0");
  }

  efl_event_callback_del(dlg.family_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED,
    eflFontDlgFamilySelectedCallback, &dlg);
  efl_event_callback_del(dlg.style_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED,
    eflFontDlgStyleSelectedCallback, &dlg);
  efl_event_callback_del(dlg.size_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED,
    eflFontDlgSizeSelectedCallback, &dlg);
  efl_event_callback_del(btn_ok, EFL_INPUT_EVENT_CLICKED, eflFontDlgOkCallback, &dlg);
  efl_event_callback_del(btn_cancel, EFL_INPUT_EVENT_CLICKED, eflFontDlgCancelCallback, &dlg);

  eflFontDlgFreeFontList(&dlg);

  efl_del(win);

  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = eflFontDlgPopup;
}

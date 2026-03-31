/** \file
 * \brief List Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multi_Label.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"
#include "iup_childtree.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Custom Widget Classes
 ****************************************************************************/

static int fltkListBrowserHandleMouseEvent(Fl_Browser* browser, Ihandle* ih, int event)
{
  if (event == FL_PUSH || event == FL_RELEASE)
  {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      int button = IUP_BUTTON1;
      if (Fl::event_button() == FL_MIDDLE_MOUSE) button = IUP_BUTTON2;
      else if (Fl::event_button() == FL_RIGHT_MOUSE) button = IUP_BUTTON3;

      int pressed = (event == FL_PUSH) ? 1 : 0;
      int mx = Fl::event_x() - browser->x();
      int my = Fl::event_y() - browser->y();

      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupfltkButtonKeySetStatus(Fl::event_state(), button, status, (event == FL_PUSH && Fl::event_clicks() > 0) ? 1 : 0);

      cb(ih, button, pressed, mx, my, status);
    }

    if (event == FL_PUSH && Fl::event_clicks() > 0 && Fl::event_button() == FL_LEFT_MOUSE)
    {
      IFnis dblclick_cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
      if (dblclick_cb)
      {
        int line = browser->value();
        if (line > 0)
          iupListSingleCallDblClickCb(ih, dblclick_cb, line);
      }
    }
  }
  return 0;
}

class IupFltkHoldBrowser : public Fl_Hold_Browser
{
public:
  Ihandle* iup_handle;
  int drag_item;
  int drag_target;
  int drag_start_x, drag_start_y;
  int dragging;

  IupFltkHoldBrowser(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Hold_Browser(x, y, w, h), iup_handle(ih),
      drag_item(-1), drag_target(-1), drag_start_x(0), drag_start_y(0), dragging(0) {}

  int findLineAt(int ey)
  {
    void* item = find_item(ey);
    if (!item) return -1;

    void* l = item_first();
    int idx = 1;
    while (l)
    {
      if (l == item) return idx;
      l = item_next(l);
      idx++;
    }
    return -1;
  }

protected:
  void draw() override
  {
    Fl_Hold_Browser::draw();

    int show_indicator = (dragging && drag_target >= 0);
    if (!show_indicator)
    {
      int dnd_target = iupAttribGetInt(iup_handle, "_IUPFLTK_DND_TARGET_LINE");
      if (dnd_target > 0)
      {
        drag_target = dnd_target;
        show_indicator = 1;
      }
    }

    if (show_indicator)
    {
      int target_y = y() + Fl::box_dy(box());
      if (has_scrollbar() & VERTICAL)
        target_y -= vposition();

      void* l = item_first();
      int idx = 1;
      while (l && idx < drag_target)
      {
        target_y += item_height(l);
        l = item_next(l);
        idx++;
      }

      int bx = x() + Fl::box_dx(box());
      int bw = w() - Fl::box_dw(box());
      if (has_scrollbar() & VERTICAL)
        bw -= Fl::scrollbar_size();

      fl_color(FL_SELECTION_COLOR);
      fl_line_style(FL_SOLID, 2);
      fl_line(bx, target_y, bx + bw, target_y);
      fl_line_style(FL_SOLID, 1);
    }
  }

  int handle(int event) override
  {
    switch (event)
    {
      case FL_DND_ENTER: case FL_DND_DRAG: case FL_DND_LEAVE: case FL_DND_RELEASE: case FL_PASTE:
        if (iupfltkDragDropHandleEvent(this, iup_handle, event))
          return 1;
        break;
      case FL_FOCUS: case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event); break;
      case FL_ENTER: case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event); break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle)) return 1; break;
      case FL_PUSH:
        iupfltkDragDropHandleEvent(this, iup_handle, event);
        fltkListBrowserHandleMouseEvent(this, iup_handle, event);
        if (iup_handle->data->show_dragdrop && !iup_handle->data->is_dropdown &&
            !iup_handle->data->is_multiple && Fl::event_button() == FL_LEFT_MOUSE)
        {
          int line = findLineAt(Fl::event_y());
          if (line > 0)
          {
            drag_item = line;
            drag_target = -1;
            drag_start_x = Fl::event_x();
            drag_start_y = Fl::event_y();
            dragging = 0;
          }
        }
        break;
      case FL_DRAG:
        if (iupfltkDragDropHandleEvent(this, iup_handle, event))
          return 1;
        if (drag_item > 0 && iup_handle->data->show_dragdrop)
        {
          int dx = Fl::event_x() - drag_start_x;
          int dy = Fl::event_y() - drag_start_y;
          if (!dragging && (dx * dx + dy * dy) >= 25)
            dragging = 1;

          if (dragging)
          {
            int line = findLineAt(Fl::event_y());
            int new_target = line > 0 ? line : size() + 1;
            if (new_target != drag_target)
            {
              drag_target = new_target;
              redraw();
            }
            return 1;
          }
        }
        break;
      case FL_RELEASE:
      {
        int src = drag_item;
        int tgt = drag_target;
        int was_dragging = dragging;

        drag_item = -1;
        drag_target = -1;
        dragging = 0;

        if (was_dragging && src > 0 && tgt > 0 && src != tgt)
        {
          int is_ctrl = (Fl::event_state() & FL_CTRL) ? 1 : 0;
          int drop_id = tgt > src ? tgt - 2 : tgt - 1;
          if (drop_id < 0) drop_id = 0;

          if (iupListCallDragDropCb(iup_handle, src - 1, drop_id, &is_ctrl) == IUP_CONTINUE)
          {
            const char* text_src = text(src);
            Fl_Image* icon_src = icon(src);
            char* text_copy = text_src ? strdup(text_src) : NULL;

            remove(src);

            int insert_pos = tgt > src ? tgt - 1 : tgt;
            if (insert_pos > size()) insert_pos = size() + 1;

            insert(insert_pos, text_copy ? text_copy : "");
            if (icon_src)
              this->icon(insert_pos, icon_src);

            value(insert_pos);
            iupAttribSetInt(iup_handle, "_IUPLIST_OLDVALUE", insert_pos);

            if (text_copy) free(text_copy);
          }

          redraw();
          return 1;
        }

        redraw();
        fltkListBrowserHandleMouseEvent(this, iup_handle, event);
        break;
      }
    }

    if (event == FL_DRAG && drag_item > 0)
      return 1;

    return Fl_Hold_Browser::handle(event);
  }
};

class IupFltkMultiBrowser : public Fl_Multi_Browser
{
public:
  Ihandle* iup_handle;

  IupFltkMultiBrowser(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Multi_Browser(x, y, w, h), iup_handle(ih) {}

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS: case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event); break;
      case FL_ENTER: case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event); break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle)) return 1; break;
      case FL_PUSH:
      case FL_RELEASE:
        fltkListBrowserHandleMouseEvent(this, iup_handle, event);
        break;
    }
    return Fl_Multi_Browser::handle(event);
  }
};

class IupFltkChoice : public Fl_Choice
{
public:
  Ihandle* iup_handle;

  IupFltkChoice(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Choice(x, y, w, h), iup_handle(ih) {}

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS: case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event); break;
      case FL_ENTER: case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event); break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle)) return 1; break;
    }
    return Fl_Choice::handle(event);
  }
};

class IupFltkInputChoice : public Fl_Input_Choice
{
public:
  Ihandle* iup_handle;

  IupFltkInputChoice(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Input_Choice(x, y, w, h), iup_handle(ih) {}
};

class IupFltkListInput : public Fl_Input
{
public:
  Ihandle* iup_handle;

  IupFltkListInput(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Input(x, y, w, h), iup_handle(ih) {}

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS: case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event); break;
      case FL_ENTER: case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event); break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle)) return 1; break;
    }
    return Fl_Input::handle(event);
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static Fl_Browser* fltkListGetBrowser(Ihandle* ih)
{
  if (ih->data->is_dropdown)
    return NULL;

  if (ih->data->has_editbox)
    return (Fl_Browser*)iupAttribGet(ih, "_IUPFLTK_LIST");

  return (Fl_Browser*)ih->handle;
}

static Fl_Input* fltkListGetEditBox(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  if (ih->data->is_dropdown)
  {
    Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
    return input_choice->input();
  }

  return (Fl_Input*)iupAttribGet(ih, "_IUPFLTK_EDIT");
}

static int fltkListGetChoiceCount(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
    const Fl_Menu_Item* menu = input_choice->menubutton()->menu();
    if (!menu)
      return 0;
    return input_choice->menubutton()->size() - 1;
  }
  else
  {
    Fl_Choice* choice = (Fl_Choice*)ih->handle;
    const Fl_Menu_Item* menu = choice->menu();
    if (!menu)
      return 0;
    return choice->size() - 1;
  }
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void fltkListChoiceCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  Fl_Choice* choice = (Fl_Choice*)w;
  int index = choice->value();
  if (index < 0)
    return;

  int pos = index + 1;
  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
    iupListSingleCallActionCb(ih, cb, pos);

  iupBaseCallValueChangedCb(ih);
}

static void fltkListInputChoiceCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  Fl_Input_Choice* input_choice = (Fl_Input_Choice*)w;
  int index = input_choice->menubutton()->value();
  if (index < 0)
    return;

  int pos = index + 1;
  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
    iupListSingleCallActionCb(ih, cb, pos);

  iupBaseCallValueChangedCb(ih);
}

static void fltkListBrowserCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  Fl_Browser* browser = (Fl_Browser*)w;

  if (!ih->data->is_multiple)
  {
    int line = browser->value();
    if (line <= 0)
      return;

    int pos = line;

    IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
    if (cb)
      iupListSingleCallActionCb(ih, cb, pos);

    if (ih->data->has_editbox)
    {
      Fl_Input* edit = (Fl_Input*)iupAttribGet(ih, "_IUPFLTK_EDIT");
      if (edit)
      {
        const char* text = browser->text(line);
        if (text)
        {
          iupAttribSet(ih, "_IUPFLTK_DISABLE_TEXT_CB", "1");
          edit->value(text);
          iupAttribSet(ih, "_IUPFLTK_DISABLE_TEXT_CB", NULL);
        }
      }
    }
  }
  else
  {
    IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
    IFnsii action_cb = (IFnsii)IupGetCallback(ih, "ACTION");

    if (multi_cb || action_cb)
    {
      int count = browser->size();
      int sel_count = 0;
      int* pos = NULL;

      for (int i = 1; i <= count; i++)
      {
        if (browser->selected(i))
          sel_count++;
      }

      if (sel_count > 0)
      {
        pos = (int*)malloc(sel_count * sizeof(int));
        int j = 0;
        for (int i = 1; i <= count; i++)
        {
          if (browser->selected(i))
            pos[j++] = i;
        }

        iupListMultipleCallActionCb(ih, action_cb, multi_cb, pos, sel_count);
        free(pos);
      }
    }
  }

  iupBaseCallValueChangedCb(ih);
}

static void fltkListEditCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupAttribGet(ih, "_IUPFLTK_DISABLE_TEXT_CB"))
    return;

  Fl_Input* edit = (Fl_Input*)w;

  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  if (cb)
  {
    const char* value = edit->value();
    int pos = edit->insert_position();
    iupEditCallActionCb(ih, cb, value, pos, pos, ih->data->mask, ih->data->nc, 0, 1);
  }

  iupBaseCallValueChangedCb(ih);
}

static void fltkListCaretCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  Fl_Input* edit = (Fl_Input*)w;

  IFnii cb = (IFnii)IupGetCallback(ih, "CARET_CB");
  if (cb)
  {
    int pos = edit->insert_position() + 1;
    cb(ih, pos, 0);
  }
}

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
  (void)ih;
  (void)h;
}

extern "C" IUP_SDK_API void iupdrvListAddBorders(Ihandle* ih, int* x, int* y)
{
  if (ih->data->is_dropdown)
  {
    *x += 22;
    *y += 6;
  }
  else
  {
    *x += 4 + 6;
    *y += 4;

    if (ih->data->has_editbox)
    {
      int char_height;
      iupdrvFontGetCharSize(ih, NULL, &char_height);
      int edit_height = char_height + 8;

      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
      if (visiblelines > 0)
      {
        int item_height = char_height;
        iupdrvListAddItemSpace(ih, &item_height);
        *y -= item_height;
      }

      *y += edit_height;
    }
  }
}

extern "C" IUP_SDK_API int iupdrvListGetCount(Ihandle* ih)
{
  if (ih->data->is_dropdown)
    return fltkListGetChoiceCount(ih);

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (browser)
    return browser->size();

  return 0;
}

extern "C" IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      input_choice->add(value);
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      choice->add(value);
    }
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
      browser->add(value);
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

extern "C" IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      input_choice->menubutton()->insert(pos, value, 0, NULL);
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      choice->insert(pos, value, 0, NULL);
    }
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
      browser->insert(pos + 1, value);
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  iupListUpdateOldValue(ih, pos, 0);
}

extern "C" IUP_SDK_API void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      Fl_Menu_Button* mb = input_choice->menubutton();
      int count = mb->size() - 1;

      if (pos >= 0 && pos < count)
        mb->remove(pos);
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      int count = choice->size() - 1;
      int curval = choice->value();

      if (pos >= 0 && pos < count)
      {
        if (pos == curval)
        {
          if (curval > 0)
            choice->value(curval - 1);
          else if (count > 1)
            choice->value(0);
          else
            choice->value(-1);
        }

        choice->remove(pos);
      }
    }
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
      browser->remove(pos + 1);
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  iupListUpdateOldValue(ih, pos, 1);
}

static void fltkListDropdownFreeMultiLabels(Ihandle* ih, int count)
{
  for (int i = 0; i < count; i++)
  {
    char ml_key[32];
    snprintf(ml_key, sizeof(ml_key), "_IUPFLTK_ML%d", i);
    Fl_Multi_Label* ml = (Fl_Multi_Label*)iupAttribGet(ih, ml_key);
    if (ml)
    {
      delete ml;
      iupAttribSet(ih, ml_key, NULL);
    }
  }
}

extern "C" IUP_SDK_API void iupdrvListRemoveAllItems(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    fltkListDropdownFreeMultiLabels(ih, fltkListGetChoiceCount(ih));

    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      input_choice->clear();
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      choice->clear();
      choice->value(-1);
    }
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
      browser->clear();
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

static void fltkListDropdownSetImage(Ihandle* ih, int pos, Fl_Image* image)
{
  const Fl_Menu_Item* menu = NULL;
  int menu_size = 0;

  if (ih->data->has_editbox)
  {
    Fl_Input_Choice* ic = (Fl_Input_Choice*)ih->handle;
    menu = ic->menubutton()->menu();
    menu_size = ic->menubutton()->size() - 1;
  }
  else
  {
    Fl_Choice* choice = (Fl_Choice*)ih->handle;
    menu = choice->menu();
    menu_size = choice->size() - 1;
  }

  if (!menu || pos < 0 || pos >= menu_size)
    return;

  Fl_Menu_Item* item = (Fl_Menu_Item*)&menu[pos];

  char ml_key[32];
  snprintf(ml_key, sizeof(ml_key), "_IUPFLTK_ML%d", pos);

  Fl_Multi_Label* ml = (Fl_Multi_Label*)iupAttribGet(ih, ml_key);
  if (!ml)
  {
    ml = new Fl_Multi_Label;
    iupAttribSet(ih, ml_key, (char*)ml);
  }

  ml->typea = FL_IMAGE_LABEL;
  ml->labela = (const char*)image;
  ml->typeb = FL_NORMAL_LABEL;
  ml->labelb = item->text;

  ml->label(item);
}

static Fl_Image* fltkListFitImage(Ihandle* ih, Fl_Image* image)
{
  if (!image || !ih->data->fit_image)
    return image;

  int charheight = 0;
  iupdrvFontGetCharSize(ih, NULL, &charheight);
  int available_height = charheight + 2 * ih->data->spacing;

  if (image->h() > available_height && available_height > 0)
  {
    int scaled_w = (image->w() * available_height) / image->h();
    return image->copy(scaled_w, available_height);
  }

  return image;
}

extern "C" IUP_SDK_API int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  if (ih->data->is_dropdown)
  {
    Fl_Image* img = fltkListFitImage(ih, (Fl_Image*)hImage);
    fltkListDropdownSetImage(ih, id, img);
    return 1;
  }

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (!browser)
    return 0;

  int line = id + 1;
  if (line < 1 || line > browser->size())
    return 0;

  Fl_Image* img = fltkListFitImage(ih, (Fl_Image*)hImage);
  browser->icon(line, img);

  if (img)
  {
    if (img->w() > ih->data->maximg_w)
      ih->data->maximg_w = img->w();
    if (img->h() > ih->data->maximg_h)
      ih->data->maximg_h = img->h();
  }

  return 1;
}

extern "C" IUP_SDK_API void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  if (ih->data->is_dropdown)
    return NULL;

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (!browser)
    return NULL;

  if (id < 1 || id > browser->size())
    return NULL;

  return (void*)browser->icon(id);
}

static void fltkListVirtualLoadImages(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  if (!iupObjectCheck(ih) || !ih->handle)
    return;

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (!browser)
    return;

  int batch_start = iupAttribGetInt(ih, "_IUPFLTK_VIMG_POS");
  int count = browser->size();
  int batch_end = batch_start + 50;
  if (batch_end > count) batch_end = count;

  for (int i = batch_start; i < batch_end; i++)
  {
    char* image_name = iupListGetItemImageCb(ih, i + 1);
    if (image_name)
    {
      Fl_Image* image = (Fl_Image*)iupImageGetImage(image_name, ih, 0, NULL);
      Fl_Image* scaled = fltkListFitImage(ih, image);
      if (scaled)
        browser->icon(i + 1, scaled);
    }
  }

  if (batch_end < count)
  {
    iupAttribSetInt(ih, "_IUPFLTK_VIMG_POS", batch_end);
    Fl::add_idle(fltkListVirtualLoadImages, data);
  }
  else
    iupAttribSet(ih, "_IUPFLTK_VIMG_POS", NULL);
}

extern "C" IUP_SDK_API void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  if (!ih->data->is_virtual)
    return;

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (!browser)
    return;

  Fl::remove_idle(fltkListVirtualLoadImages, (void*)ih);
  browser->clear();

  for (int i = 0; i < count; i++)
  {
    char* text = iupListGetItemValueCb(ih, i + 1);
    browser->add(text ? text : "");
  }

  if (IupGetCallback(ih, "IMAGE_CB"))
  {
    iupAttribSetInt(ih, "_IUPFLTK_VIMG_POS", 0);
    Fl::add_idle(fltkListVirtualLoadImages, (void*)ih);
  }
}

/****************************************************************************
 * Attribute Getters/Setters
 ****************************************************************************/

static char* fltkListGetIdValueAttrib(Ihandle* ih, int id)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos < 0)
    return NULL;

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      int count = fltkListGetChoiceCount(ih);
      if (pos < count)
      {
        const Fl_Menu_Item* menu = input_choice->menubutton()->menu();
        return iupStrReturnStr(menu[pos].label());
      }
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      int count = fltkListGetChoiceCount(ih);
      if (pos < count)
      {
        const Fl_Menu_Item* menu = choice->menu();
        return iupStrReturnStr(menu[pos].label());
      }
    }
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
    {
      int line = pos + 1;
      if (line >= 1 && line <= browser->size())
        return iupStrReturnStr(browser->text(line));
    }
  }

  return NULL;
}

static char* fltkListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    Fl_Input* edit = fltkListGetEditBox(ih);
    if (edit)
      return iupStrReturnStr(edit->value());
  }
  else if (ih->data->is_dropdown)
  {
    Fl_Choice* choice = (Fl_Choice*)ih->handle;
    int val = choice->value();
    if (val >= 0)
      return iupStrReturnInt(val + 1);
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (!browser)
      return NULL;

    if (!ih->data->is_multiple)
    {
      int line = browser->value();
      if (line > 0)
        return iupStrReturnInt(line);
    }
    else
    {
      int count = browser->size();
      char* str = iupStrGetMemory(count + 1);
      memset(str, '-', count);
      str[count] = 0;

      for (int i = 1; i <= count; i++)
      {
        if (browser->selected(i))
          str[i - 1] = '+';
      }
      return str;
    }
  }

  return NULL;
}

static int fltkListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    Fl_Input* edit = fltkListGetEditBox(ih);
    if (edit)
    {
      iupAttribSet(ih, "_IUPFLTK_DISABLE_TEXT_CB", "1");
      edit->value(value ? value : "");
      iupAttribSet(ih, "_IUPFLTK_DISABLE_TEXT_CB", NULL);
    }
  }
  else if (ih->data->is_dropdown)
  {
    Fl_Choice* choice = (Fl_Choice*)ih->handle;
    int pos;

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

    if (iupStrToInt(value, &pos) == 1 && pos > 0 && pos <= fltkListGetChoiceCount(ih))
    {
      choice->value(pos - 1);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    }
    else
    {
      choice->value(-1);
      iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (!browser)
      return 0;

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

    if (!ih->data->is_multiple)
    {
      int pos;
      if (iupStrToInt(value, &pos) == 1 && pos > 0 && pos <= browser->size())
      {
        browser->value(pos);
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
      }
      else
      {
        browser->deselect();
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
      }
    }
    else
    {
      int i, len, count;

      for (i = 1; i <= browser->size(); i++)
        browser->select(i, 0);

      if (!value)
      {
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        return 0;
      }

      len = (int)strlen(value);
      count = browser->size();
      if (len < count)
        count = len;

      for (i = 0; i < count; i++)
      {
        if (value[i] == '+')
          browser->select(i + 1, 1);
      }

      iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  }

  return 0;
}

static int fltkListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

static int fltkListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos;
    if (iupStrToInt(value, &pos) && pos > 0)
    {
      Fl_Browser* browser = fltkListGetBrowser(ih);
      if (browser)
        browser->topline(pos);
    }
  }
  return 0;
}

static int fltkListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);
  return 0;
}

static int fltkListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Color color = fl_rgb_color(r, g, b);

  if (ih->data->is_dropdown)
  {
    Fl_Widget* w = (Fl_Widget*)ih->handle;
    w->color(color);
    w->redraw();
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
    {
      browser->color(color);
      browser->redraw();
    }
  }

  return 1;
}

static int fltkListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Color color = fl_rgb_color(r, g, b);

  if (ih->data->is_dropdown)
  {
    Fl_Widget* w = (Fl_Widget*)ih->handle;
    w->labelcolor(color);
    w->redraw();
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
    {
      browser->textcolor(color);
      browser->redraw();
    }
  }

  return 1;
}

static int fltkListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle && ih->data->has_editbox && !ih->data->is_dropdown)
  {
    Fl_Input* edit = (Fl_Input*)iupAttribGet(ih, "_IUPFLTK_EDIT");
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (edit)
      iupfltkUpdateWidgetFont(ih, edit);
    if (browser)
      iupfltkUpdateWidgetFont(ih, browser);
  }

  return 1;
}

static char* fltkListGetReadOnlyAttrib(Ihandle* ih)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (edit)
    return iupStrReturnBoolean(edit->readonly());
  return NULL;
}

static int fltkListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (edit)
    edit->readonly(iupStrBoolean(value));
  return 0;
}

static char* fltkListGetSelectedTextAttrib(Ihandle* ih)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return NULL;

  int start = edit->insert_position();
  int end = edit->mark();
  if (start == end)
    return NULL;

  if (start > end)
  {
    int tmp = start;
    start = end;
    end = tmp;
  }

  const char* value = edit->value();
  int len = end - start;
  char* str = iupStrGetMemory(len + 1);
  memcpy(str, value + start, len);
  str[len] = 0;
  return str;
}

static int fltkListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return 0;

  int start = edit->insert_position();
  int end = edit->mark();
  if (start != end)
  {
    if (start > end)
    {
      int tmp = start;
      start = end;
      end = tmp;
    }
    edit->replace(start, end, value ? value : "");
  }

  return 0;
}

static char* fltkListGetSelectionAttrib(Ihandle* ih)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return NULL;

  int start = edit->insert_position();
  int end = edit->mark();

  if (start == end)
    return NULL;

  if (start > end)
  {
    int tmp = start;
    start = end;
    end = tmp;
  }

  return iupStrReturnIntInt(start + 1, end, ':');
}

static int fltkListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    int pos = edit->insert_position();
    edit->insert_position(pos, pos);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    edit->insert_position(0, (int)strlen(edit->value()));
    return 0;
  }

  int start = 1, end = 1;
  if (iupStrToIntInt(value, &start, &end, ':') == 2)
  {
    if (start < 1) start = 1;
    if (end < 1) end = 1;
    start--;
    end--;
    if (end < start)
      end = start;
    edit->insert_position(start, end + 1);
  }

  return 0;
}

static char* fltkListGetCaretAttrib(Ihandle* ih)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (edit)
    return iupStrReturnInt(edit->insert_position() + 1);
  return NULL;
}

static int fltkListSetCaretAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return 0;

  int pos;
  if (iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    pos--;
    edit->insert_position(pos);
  }
  return 0;
}

static int fltkListSetInsertAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (edit && value)
  {
    int pos = edit->insert_position();
    edit->replace(pos, pos, value);
  }
  return 0;
}

static int fltkListSetAppendAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (edit && value)
  {
    int len = (int)strlen(edit->value());
    edit->replace(len, len, value);
  }
  return 0;
}

static int fltkListSetNCAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return 0;

  int max;
  if (iupStrToInt(value, &max))
  {
    if (max < 0) max = 0;
    edit->maximum_size(max);
  }
  return 0;
}

static int fltkListSetClipboardAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
    edit->copy(1);
  else if (iupStrEqualNoCase(value, "CUT"))
    edit->cut();
  else if (iupStrEqualNoCase(value, "PASTE"))
    Fl::paste(*edit, 1);
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    int start = edit->insert_position();
    int end = edit->mark();
    if (start != end)
    {
      if (start > end)
      {
        int tmp = start;
        start = end;
        end = tmp;
      }
      edit->replace(start, end, "");
    }
  }

  return 0;
}

static int fltkListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  Fl_Input* edit = fltkListGetEditBox(ih);
  if (!edit)
    return 0;

  int pos;
  if (iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    pos--;
    edit->insert_position(pos);
  }
  return 0;
}

static int fltkListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  return 0;
}

static int fltkListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos < 0)
    return 0;

  if (ih->data->is_dropdown)
  {
    if (value)
    {
      Fl_Image* image = (Fl_Image*)iupImageGetImage(value, ih, 0, NULL);
      Fl_Image* scaled = fltkListFitImage(ih, image);
      fltkListDropdownSetImage(ih, pos, scaled);
    }
    return 1;
  }

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (!browser)
    return 0;

  int line = pos + 1;
  if (line < 1 || line > browser->size())
    return 0;

  Fl_Image* image = (Fl_Image*)iupImageGetImage(value, ih, 0, NULL);
  Fl_Image* scaled = fltkListFitImage(ih, image);
  browser->icon(line, scaled);

  if (scaled)
  {
    if (scaled->w() > ih->data->maximg_w)
      ih->data->maximg_w = scaled->w();
    if (scaled->h() > ih->data->maximg_h)
      ih->data->maximg_h = scaled->h();
  }

  return 1;
}

static char* fltkListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  return (char*)iupdrvListGetImageHandle(ih, id);
}

static int fltkListSetVisibleItemsAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

/****************************************************************************
 * Convert XY to Position
 ****************************************************************************/

static int fltkListConvertXYToPos(Ihandle* ih, int x, int y)
{
  (void)x;

  if (ih->data->is_dropdown)
    return -1;

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (!browser)
    return -1;

  int count = browser->size();
  if (count <= 0)
    return -1;

  int char_height;
  iupdrvFontGetCharSize(ih, NULL, &char_height);
  int item_height = char_height + ih->data->spacing + 4;

  int topline = browser->topline();
  int line = topline + y / item_height;

  if (line < 1)
    line = 1;
  if (line > count)
    return -1;

  return line;
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int fltkListMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      IupFltkInputChoice* input_choice = new IupFltkInputChoice(0, 0, 10, 10, ih);
      ih->handle = (InativeHandle*)input_choice;

      iupfltkUpdateWidgetFont(ih, input_choice);

      input_choice->callback(fltkListInputChoiceCallback, (void*)ih);

      Fl_Input* edit = input_choice->input();
      if (edit)
      {
        edit->callback(fltkListEditCallback, (void*)ih);
        edit->when(FL_WHEN_CHANGED);

        if (ih->data->nc > 0)
          edit->maximum_size(ih->data->nc);
      }

      iupfltkAddToParent(ih);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupfltkSetCanFocus(input_choice, 0);

      if (ih->data->is_virtual && ih->data->item_count > 0)
        iupdrvListSetItemCount(ih, ih->data->item_count);
      else
        iupListSetInitialItems(ih);
    }
    else
    {
      IupFltkChoice* choice = new IupFltkChoice(0, 0, 10, 10, ih);
      ih->handle = (InativeHandle*)choice;

      iupfltkUpdateWidgetFont(ih, choice);

      choice->callback(fltkListChoiceCallback, (void*)ih);

      iupfltkAddToParent(ih);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        choice->visible_focus(0);

      if (ih->data->is_virtual && ih->data->item_count > 0)
        iupdrvListSetItemCount(ih, ih->data->item_count);
      else
        iupListSetInitialItems(ih);

      choice->value(-1);
    }
  }
  else if (ih->data->has_editbox)
  {
    Fl_Group* group = iupfltkNativeContainerNew();

    int char_height;
    iupdrvFontGetCharSize(ih, NULL, &char_height);
    int edit_height = char_height + 8;

    IupFltkListInput* edit = new IupFltkListInput(0, 0, 10, edit_height, ih);
    group->add(edit);
    edit->when(FL_WHEN_CHANGED);

    IupFltkHoldBrowser* browser = new IupFltkHoldBrowser(0, edit_height, 10, 10, ih);
    browser->has_scrollbar(Fl_Browser_::VERTICAL);
    group->add(browser);

    ih->handle = (InativeHandle*)group;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)group);
    iupAttribSet(ih, "_IUPFLTK_EDIT", (char*)edit);
    iupAttribSet(ih, "_IUPFLTK_LIST", (char*)browser);

    iupfltkUpdateWidgetFont(ih, edit);
    iupfltkUpdateWidgetFont(ih, browser);

    edit->callback(fltkListEditCallback, (void*)ih);
    browser->callback(fltkListBrowserCallback, (void*)ih);
    browser->when(FL_WHEN_RELEASE);

    if (ih->data->nc > 0)
      edit->maximum_size(ih->data->nc);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    {
      edit->visible_focus(0);
      browser->visible_focus(0);
    }

    if (ih->data->is_virtual && ih->data->item_count > 0)
      iupdrvListSetItemCount(ih, ih->data->item_count);
    else
      iupListSetInitialItems(ih);
  }
  else if (ih->data->is_multiple)
  {
    Fl_Group* group = iupfltkNativeContainerNew();

    IupFltkMultiBrowser* browser = new IupFltkMultiBrowser(0, 0, 10, 10, ih);
    browser->has_scrollbar(Fl_Browser_::VERTICAL);
    group->add(browser);

    ih->handle = (InativeHandle*)browser;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)group);

    iupfltkUpdateWidgetFont(ih, browser);

    browser->callback(fltkListBrowserCallback, (void*)ih);
    browser->when(FL_WHEN_RELEASE);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      browser->visible_focus(0);

    if (ih->data->is_virtual && ih->data->item_count > 0)
      iupdrvListSetItemCount(ih, ih->data->item_count);
    else
      iupListSetInitialItems(ih);
  }
  else
  {
    Fl_Group* group = iupfltkNativeContainerNew();

    IupFltkHoldBrowser* browser = new IupFltkHoldBrowser(0, 0, 10, 10, ih);
    browser->has_scrollbar(Fl_Browser_::VERTICAL);
    group->add(browser);

    ih->handle = (InativeHandle*)browser;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)group);

    iupfltkUpdateWidgetFont(ih, browser);

    browser->callback(fltkListBrowserCallback, (void*)ih);
    browser->when(FL_WHEN_RELEASE);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      browser->visible_focus(0);

    if (ih->data->is_virtual && ih->data->item_count > 0)
      iupdrvListSetItemCount(ih, ih->data->item_count);
    else
      iupListSetInitialItems(ih);
  }

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)fltkListConvertXYToPos);

  return IUP_NOERROR;
}

/****************************************************************************
 * Layout Update Method
 ****************************************************************************/

static void fltkListLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->data->has_editbox && !ih->data->is_dropdown)
  {
    iupdrvBaseLayoutUpdateMethod(ih);

    Fl_Group* group = (Fl_Group*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    Fl_Input* edit = (Fl_Input*)iupAttribGet(ih, "_IUPFLTK_EDIT");
    Fl_Browser* browser = (Fl_Browser*)iupAttribGet(ih, "_IUPFLTK_LIST");

    if (group && edit && browser)
    {
      int ox = group->x();
      int oy = group->y();

      int char_height;
      iupdrvFontGetCharSize(ih, NULL, &char_height);
      int edit_height = char_height + 8;

      edit->resize(ox, oy, group->w(), edit_height);
      browser->resize(ox, oy + edit_height, group->w(), group->h() - edit_height);
    }
  }
  else if (!ih->data->is_dropdown)
  {
    iupdrvBaseLayoutUpdateMethod(ih);

    Fl_Group* group = (Fl_Group*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    Fl_Browser* browser = fltkListGetBrowser(ih);

    if (group && browser)
      browser->resize(group->x(), group->y(), group->w(), group->h());
  }
  else
  {
    iupdrvBaseLayoutUpdateMethod(ih);
  }
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = fltkListMapMethod;
  ic->LayoutUpdate = fltkListLayoutUpdateMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FONT", NULL, fltkListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* IupList only */
  iupClassRegisterAttributeId(ic, "IDVALUE", fltkListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", fltkListGetValueAttrib, fltkListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, fltkListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, fltkListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_SUPPORTED | IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, fltkListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, fltkListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, fltkListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SCROLLBAR", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);

  /* Editbox attributes */
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", fltkListGetSelectedTextAttrib, fltkListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", fltkListGetSelectionAttrib, fltkListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", fltkListGetCaretAttrib, fltkListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, fltkListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, fltkListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", fltkListGetReadOnlyAttrib, fltkListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, fltkListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, fltkListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* Image support */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, fltkListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", fltkListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);
}

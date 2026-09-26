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

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Custom Widget Classes
 ****************************************************************************/

template <class B>
static int fltkListBrowserHandleMouseEvent(B* browser, Ihandle* ih, int event)
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

public:
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
      case FL_MOVE:
        iupfltkMouseMoveEvent(this, iup_handle);
        break;
      case FL_DRAG:
        iupfltkMouseMoveEvent(this, iup_handle);
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
      case FL_RELEASE:
        fltkListBrowserHandleMouseEvent(this, iup_handle, event);
        break;
      case FL_MOVE:
      case FL_DRAG:
        iupfltkMouseMoveEvent(this, iup_handle);
        break;
    }
    return Fl_Multi_Browser::handle(event);
  }
};

static bool fltkListIsDropdownTrigger(int event)
{
  if (event == FL_PUSH)
    return true;
  if (event == FL_KEYBOARD && Fl::event_key() == ' ' &&
      !(Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META)))
    return true;
  return false;
}

static void fltkListCallDropdownCb(Ihandle* ih, int show)
{
  IFni cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  if (cb) cb(ih, show);
}

class IupFltkChoice : public Fl_Choice
{
public:
  Ihandle* iup_handle;

  IupFltkChoice(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Choice(x, y, w, h), iup_handle(ih) {}

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

    /* Fl_Choice::handle blocks inside pulldown() during FL_PUSH/space; wrap with DROPDOWN_CB. */
    bool drop = fltkListIsDropdownTrigger(event) && menu() && menu()->text;
    if (drop) fltkListCallDropdownCb(iup_handle, 1);
    int ret = Fl_Choice::handle(event);
    if (drop) fltkListCallDropdownCb(iup_handle, 0);
    return ret;
  }
};

class IupFltkInputChoice : public Fl_Input_Choice
{
public:
  Ihandle* iup_handle;
  bool iup_focused;

  IupFltkInputChoice(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Input_Choice(x, y, w, h), iup_handle(ih), iup_focused(false) {}

  int handle(int event) override
  {
    if (event == FL_FOCUS && !iup_focused)
    {
      iup_focused = true;
      iupfltkFocusInOutEvent(this, iup_handle, FL_FOCUS);
    }
    else if (event == FL_UNFOCUS && iup_focused)
    {
      iup_focused = false;
      iupfltkFocusInOutEvent(this, iup_handle, FL_UNFOCUS);
    }
    /* InputMenuButton is private; intercept at group level when event targets the button child. */
    Fl_Widget* mb = menubutton();
    bool drop = false;
    if (mb && mb->takesevents())
    {
      if (event == FL_PUSH && Fl::event_inside(mb))
        drop = true;
      else if (event == FL_KEYBOARD && Fl::focus() == mb &&
               Fl::event_key() == ' ' &&
               !(Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META)))
        drop = true;
    }

    if (drop) fltkListCallDropdownCb(iup_handle, 1);
    int ret = Fl_Input_Choice::handle(event);
    if (drop) fltkListCallDropdownCb(iup_handle, 0);
    return ret;
  }
};

class IupFltkListInput : public Fl_Input
{
public:
  Ihandle* iup_handle;

  IupFltkListInput(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Input(x, y, w, h), iup_handle(ih) {}

  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS: case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event); break;
      case FL_ENTER: case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event); break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle)) return 1;
        if (iupfltkEditCheckMask(iup_handle, this, event, "EDIT_CB", iup_handle->data->mask, iup_handle->data->nc)) return 1;
        break;
    }
    return Fl_Input::handle(event);
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

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

class IupFltkVirtualBrowser : public Fl_Browser_
{
  struct IconEntry
  {
    Fl_Image* image;
    bool owned;
  };

  int count;
  std::vector<char> sel;
  mutable std::map<std::string, IconEntry> icons;
  mutable int row_h;
  mutable int icon_h;

  static void* item(int line) { return (void*)(intptr_t)line; }
  static int line(void* item) { return (int)(intptr_t)item; }

  int rowHeight() const
  {
    if (!row_h)
    {
      fl_font(textfont(), textsize());
      row_h = fl_height();
      if (icon_h + 2 > row_h)
        row_h = icon_h + 2;
    }
    return row_h;
  }

  Fl_Image* icon(int l) const
  {
    if (!iup_handle->data->show_image)
      return NULL;

    char* name = iupListGetItemImageCb(iup_handle, l);
    if (!name)
      return NULL;

    std::map<std::string, IconEntry>::iterator found = icons.find(name);
    if (found != icons.end())
      return found->second.image;

    Fl_Image* image = (Fl_Image*)iupImageGetImage(name, iup_handle, 0, NULL);
    Fl_Image* fitted = fltkListFitImage(iup_handle, image);
    IconEntry entry = { fitted, fitted != image };
    icons[name] = entry;

    if (fitted && fitted->h() > icon_h)
    {
      icon_h = fitted->h();
      row_h = 0;
    }
    if (fitted && fitted->w() > iup_handle->data->maximg_w)
      iup_handle->data->maximg_w = fitted->w();
    if (fitted && fitted->h() > iup_handle->data->maximg_h)
      iup_handle->data->maximg_h = fitted->h();

    return fitted;
  }

protected:
  void* item_first() const override { return count > 0 ? item(1) : NULL; }
  void* item_next(void* i) const override { return line(i) < count ? item(line(i) + 1) : NULL; }
  void* item_prev(void* i) const override { return line(i) > 1 ? item(line(i) - 1) : NULL; }
  void* item_last() const override { return count > 0 ? item(count) : NULL; }
  void* item_at(int index) const override { return (index >= 1 && index <= count) ? item(index) : NULL; }

  int item_height(void* i) const override { (void)i; return rowHeight(); }
  int item_quick_height(void* i) const override { (void)i; return rowHeight(); }
  int full_height() const override { return count * rowHeight(); }
  int incr_height() const override { return rowHeight(); }

  int item_width(void* i) const override
  {
    const char* str = text(line(i));
    Fl_Image* img = icon(line(i));
    fl_font(textfont(), textsize());
    return (img ? img->w() + 2 : 0) + (int)fl_width(str ? str : "") + 6;
  }

  void item_draw(void* i, int X, int Y, int W, int H) const override
  {
    Fl_Image* img = icon(line(i));
    if (img)
    {
      img->draw(X + 2, Y + 1);
      X += img->w() + 2;
      W -= img->w() + 2;
    }

    Fl_Color lcol = textcolor();
    if (item_selected(i))
      lcol = fl_contrast(lcol, selection_color());
    if (!active_r())
      lcol = fl_inactive(lcol);

    const char* str = text(line(i));
    fl_font(textfont(), textsize());
    fl_color(lcol);
    fl_draw(str ? str : "", X + 3, Y, W - 6, H, FL_ALIGN_LEFT, 0, 0);
  }

  void item_select(void* i, int val) override
  {
    if (type() == FL_MULTI_BROWSER)
      sel[line(i)] = val ? 1 : 0;
  }

  int item_selected(void* i) const override
  {
    if (type() == FL_MULTI_BROWSER)
      return sel[line(i)];
    return Fl_Browser_::item_selected(i);
  }

public:
  Ihandle* iup_handle;

  IupFltkVirtualBrowser(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Browser_(x, y, w, h), count(0), row_h(0), icon_h(0), iup_handle(ih)
  {
    type(ih->data->is_multiple ? FL_MULTI_BROWSER : FL_HOLD_BROWSER);
  }

  ~IupFltkVirtualBrowser()
  {
    for (std::map<std::string, IconEntry>::iterator it = icons.begin(); it != icons.end(); ++it)
    {
      if (it->second.owned)
        delete it->second.image;
    }
  }

  void setCount(int n)
  {
    count = n;
    sel.assign((size_t)n + 1, 0);
    if (count > 0)
      icon(1);
    new_list();
    redraw();
  }

  void textFont(Fl_Font font, Fl_Fontsize size)
  {
    textfont(font);
    textsize(size);
    row_h = 0;
    redraw();
  }

  int size() const { return count; }
  const char* text(int l) const { return iupListGetItemValueCb(iup_handle, l); }
  int value() const { return line(selection()); }
  void value(int l) { if (l > 0 && l <= count) select_only(item(l)); else deselect(); }
  int selected(int l) const { return item_selected(item(l)); }
  void select(int l, int val) { Fl_Browser_::select(item(l), val); }
  int topline() const { return top() ? line(top()) : 1; }
  void topline(int l) { vposition((l - 1) * rowHeight()); }
  int lineAt(int ey) { return line(find_item(ey)); }

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
      case FL_RELEASE:
        fltkListBrowserHandleMouseEvent(this, iup_handle, event);
        break;
      case FL_MOVE:
      case FL_DRAG:
        iupfltkMouseMoveEvent(this, iup_handle);
        break;
    }
    return Fl_Browser_::handle(event);
  }
};

static Fl_Browser* fltkListGetBrowser(Ihandle* ih)
{
  if (ih->data->is_dropdown || ih->data->is_virtual)
    return NULL;

  if (ih->data->has_editbox)
    return (Fl_Browser*)iupAttribGet(ih, "_IUPFLTK_LIST");

  return (Fl_Browser*)ih->handle;
}

static IupFltkVirtualBrowser* fltkListGetVirtualBrowser(Ihandle* ih)
{
  if (!ih->data->is_virtual)
    return NULL;
  return (IupFltkVirtualBrowser*)ih->handle;
}

static Fl_Browser_* fltkListGetBrowserBase(Ihandle* ih)
{
  if (ih->data->is_virtual)
    return fltkListGetVirtualBrowser(ih);
  return fltkListGetBrowser(ih);
}

static void fltkListUpdateTextFont(Ihandle* ih, int font, int size)
{
  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      input_choice->textfont((Fl_Font)font);
      input_choice->textsize((Fl_Fontsize)size);
      input_choice->menubutton()->textfont((Fl_Font)font);
      input_choice->menubutton()->textsize((Fl_Fontsize)size);
      input_choice->redraw();
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      choice->textfont((Fl_Font)font);
      choice->textsize((Fl_Fontsize)size);
      choice->redraw();
    }
    return;
  }

  if (ih->data->has_editbox)
  {
    Fl_Input* edit = (Fl_Input*)iupAttribGet(ih, "_IUPFLTK_EDIT");
    if (edit)
    {
      edit->textfont((Fl_Font)font);
      edit->textsize((Fl_Fontsize)size);
      edit->redraw();
    }
  }

  IupFltkVirtualBrowser* vbrowser = fltkListGetVirtualBrowser(ih);
  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (vbrowser)
    vbrowser->textFont((Fl_Font)font, (Fl_Fontsize)size);
  else if (browser)
  {
    browser->textfont((Fl_Font)font);
    browser->textsize((Fl_Fontsize)size);
    browser->redraw();
  }
}

static void fltkListInitTextFont(Ihandle* ih)
{
  int font, size;
  if (iupfltkGetFont(ih, &font, &size))
    fltkListUpdateTextFont(ih, font, size);
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

  iupAttribSetStr(ih, "_IUPFLTK_EDIT_LAST", input_choice->value());

  int pos = index + 1;
  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
    iupListSingleCallActionCb(ih, cb, pos);

  iupBaseCallValueChangedCb(ih);
}

template <class B>
static void fltkListBrowserNotify(Ihandle* ih, B* browser)
{
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
          iupAttribSetStr(ih, "_IUPFLTK_EDIT_LAST", text);
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

static void fltkListBrowserCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  if (ih->data->is_virtual)
    fltkListBrowserNotify(ih, (IupFltkVirtualBrowser*)w);
  else
    fltkListBrowserNotify(ih, (Fl_Browser*)w);
}

static void fltkListEditCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupAttribGet(ih, "_IUPFLTK_DISABLE_TEXT_CB"))
    return;

  Fl_Input* edit = (Fl_Input*)w;
  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  const char* value = edit->value();
  const char* typed = Fl::event_text();
  int key = (typed && typed[0] >= 32 && !typed[1]) ? typed[0] : 0;
  int ret = 1;

  if (ih->data->nc && (int)strlen(value) > ih->data->nc)
    ret = 0;
  else if (ih->data->mask && iupMaskCheck((Imask*)ih->data->mask, value) == 0)
  {
    IFns fail_cb = (IFns)IupGetCallback(ih, "MASKFAIL_CB");
    if (fail_cb) fail_cb(ih, (char*)value);
    ret = 0;
  }
  else if (cb)
  {
    int cb_ret = cb(ih, key, (char*)value);
    if (cb_ret == IUP_IGNORE)
      ret = 0;
    else if (cb_ret == IUP_CLOSE)
    {
      IupExitLoop();
      ret = 0;
    }
  }

  if (ret == 0)
  {
    char* last = iupAttribGet(ih, "_IUPFLTK_EDIT_LAST");
    int pos = edit->insert_position();
    iupAttribSet(ih, "_IUPFLTK_DISABLE_TEXT_CB", "1");
    edit->value(last ? last : "");
    edit->insert_position(pos > 0 ? pos - 1 : 0);
    iupAttribSet(ih, "_IUPFLTK_DISABLE_TEXT_CB", NULL);
    return;
  }

  iupAttribSetStr(ih, "_IUPFLTK_EDIT_LAST", value);
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

  if (ih->data->is_virtual)
    return ih->data->item_count;

  Fl_Browser* browser = fltkListGetBrowser(ih);
  if (browser)
    return browser->size();

  return 0;
}

static int fltkListBrowserSortPos(Fl_Browser* b, const char* value)  /* 1-based */
{
  int n = b->size();
  for (int i = 1; i <= n; i++)
  {
    const char* t = b->text(i);
    if (iupStrCompare(t ? t : "", value, 0, 1) > 0) return i;
  }
  return n + 1;
}

static int fltkListMenuSortPos(Fl_Menu_* m, const char* value)  /* 0-based */
{
  int n = m->size() - 1;  /* size() counts the terminating empty item */
  const Fl_Menu_Item* items = m->menu();
  for (int i = 0; i < n; i++)
  {
    const char* t = items[i].text;
    if (iupStrCompare(t ? t : "", value, 0, 1) > 0) return i;
  }
  return n;
}

extern "C" IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  int sort = iupAttribGetBoolean(ih, "SORT");
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      if (sort)
        input_choice->menubutton()->insert(fltkListMenuSortPos(input_choice->menubutton(), value), value, 0, NULL);
      else
        input_choice->add(value);
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      if (sort)
        choice->insert(fltkListMenuSortPos(choice, value), value, 0, NULL);
      else
        choice->add(value);
    }
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
    {
      if (sort)
        browser->insert(fltkListBrowserSortPos(browser, value), value);
      else
        browser->add(value);
    }
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

extern "C" IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  int sort = iupAttribGetBoolean(ih, "SORT");
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      Fl_Input_Choice* input_choice = (Fl_Input_Choice*)ih->handle;
      input_choice->menubutton()->insert(sort ? fltkListMenuSortPos(input_choice->menubutton(), value) : pos, value, 0, NULL);
    }
    else
    {
      Fl_Choice* choice = (Fl_Choice*)ih->handle;
      choice->insert(sort ? fltkListMenuSortPos(choice, value) : pos, value, 0, NULL);
    }
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
      browser->insert(sort ? fltkListBrowserSortPos(browser, value) : pos + 1, value);
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

extern "C" IUP_SDK_API void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  IupFltkVirtualBrowser* browser = fltkListGetVirtualBrowser(ih);
  if (browser)
    browser->setCount(count);
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
  else if (ih->data->is_virtual)
  {
    if (pos < ih->data->item_count)
      return iupStrReturnStr(iupListGetItemValueCb(ih, pos + 1));
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

template <class B>
static char* fltkListBrowserGetValue(Ihandle* ih, B* browser)
{
  if (!ih->data->is_multiple)
  {
    int line = browser->value();
    if (line > 0)
      return iupStrReturnInt(line);
    return NULL;
  }

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

template <class B>
static void fltkListBrowserSetValue(Ihandle* ih, B* browser, const char* value)
{
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

    if (value)
    {
      len = (int)strlen(value);
      count = browser->size();
      if (len < count)
        count = len;

      for (i = 0; i < count; i++)
      {
        if (value[i] == '+')
          browser->select(i + 1, 1);
      }
    }

    iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
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
  else if (ih->data->is_virtual)
  {
    IupFltkVirtualBrowser* browser = fltkListGetVirtualBrowser(ih);
    if (browser)
      return fltkListBrowserGetValue(ih, browser);
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
      return fltkListBrowserGetValue(ih, browser);
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
      iupAttribSetStr(ih, "_IUPFLTK_EDIT_LAST", value);
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
  else if (ih->data->is_virtual)
  {
    IupFltkVirtualBrowser* browser = fltkListGetVirtualBrowser(ih);
    if (browser)
      fltkListBrowserSetValue(ih, browser, value);
  }
  else
  {
    Fl_Browser* browser = fltkListGetBrowser(ih);
    if (browser)
      fltkListBrowserSetValue(ih, browser, value);
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
      IupFltkVirtualBrowser* vbrowser = fltkListGetVirtualBrowser(ih);
      Fl_Browser* browser = fltkListGetBrowser(ih);
      if (vbrowser)
        vbrowser->topline(pos);
      else if (browser)
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
    Fl_Browser_* browser = fltkListGetBrowserBase(ih);
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
    Fl_Browser_* browser = fltkListGetBrowserBase(ih);
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

  if (ih->handle)
  {
    int font, size;
    if (iupfltkGetFontFromString(value, &font, &size))
      fltkListUpdateTextFont(ih, font, size);
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
  if (!ih->data->show_image || pos < 0)
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

  IupFltkVirtualBrowser* vbrowser = fltkListGetVirtualBrowser(ih);
  if (vbrowser)
  {
    int line = vbrowser->lineAt(vbrowser->y() + y);
    return line > 0 ? line : -1;
  }

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

/* SCROLLBAR=NO -> none; otherwise vertical, always-on when AUTOHIDE=NO */
static void fltkListSetScrollbarMode(Fl_Browser_* browser, Ihandle* ih)
{
  if (!ih->data->sb)
    browser->has_scrollbar(0);
  else if (iupAttribGetBoolean(ih, "AUTOHIDE"))
    browser->has_scrollbar(Fl_Browser_::VERTICAL);
  else
    browser->has_scrollbar(Fl_Browser_::VERTICAL_ALWAYS);
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
      fltkListInitTextFont(ih);

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

      iupListSetInitialItems(ih);
    }
    else
    {
      IupFltkChoice* choice = new IupFltkChoice(0, 0, 10, 10, ih);
      ih->handle = (InativeHandle*)choice;

      iupfltkUpdateWidgetFont(ih, choice);
      fltkListInitTextFont(ih);

      choice->callback(fltkListChoiceCallback, (void*)ih);

      iupfltkAddToParent(ih);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        choice->visible_focus(0);

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
    fltkListSetScrollbarMode(browser, ih);
    group->add(browser);

    ih->handle = (InativeHandle*)group;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)group);
    iupAttribSet(ih, "_IUPFLTK_EDIT", (char*)edit);
    iupAttribSet(ih, "_IUPFLTK_LIST", (char*)browser);

    iupfltkUpdateWidgetFont(ih, edit);
    iupfltkUpdateWidgetFont(ih, browser);
    fltkListInitTextFont(ih);

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

    iupListSetInitialItems(ih);
  }
  else if (ih->data->is_virtual)
  {
    Fl_Group* group = iupfltkNativeContainerNew();

    IupFltkVirtualBrowser* browser = new IupFltkVirtualBrowser(0, 0, 10, 10, ih);
    fltkListSetScrollbarMode(browser, ih);
    group->add(browser);

    ih->handle = (InativeHandle*)browser;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)group);

    iupfltkUpdateWidgetFont(ih, browser);
    fltkListInitTextFont(ih);

    browser->callback(fltkListBrowserCallback, (void*)ih);
    browser->when(FL_WHEN_RELEASE);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      browser->visible_focus(0);

    browser->setCount(ih->data->item_count);
  }
  else if (ih->data->is_multiple)
  {
    Fl_Group* group = iupfltkNativeContainerNew();

    IupFltkMultiBrowser* browser = new IupFltkMultiBrowser(0, 0, 10, 10, ih);
    fltkListSetScrollbarMode(browser, ih);
    group->add(browser);

    ih->handle = (InativeHandle*)browser;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)group);

    iupfltkUpdateWidgetFont(ih, browser);
    fltkListInitTextFont(ih);

    browser->callback(fltkListBrowserCallback, (void*)ih);
    browser->when(FL_WHEN_RELEASE);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      browser->visible_focus(0);

    iupListSetInitialItems(ih);
  }
  else
  {
    Fl_Group* group = iupfltkNativeContainerNew();

    IupFltkHoldBrowser* browser = new IupFltkHoldBrowser(0, 0, 10, 10, ih);
    fltkListSetScrollbarMode(browser, ih);
    group->add(browser);

    ih->handle = (InativeHandle*)browser;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)group);

    iupfltkUpdateWidgetFont(ih, browser);
    fltkListInitTextFont(ih);

    browser->callback(fltkListBrowserCallback, (void*)ih);
    browser->when(FL_WHEN_RELEASE);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      browser->visible_focus(0);

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
    Fl_Browser_* browser = fltkListGetBrowserBase(ih);

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

  iupClassRegisterAttribute(ic, "SELECTEDTEXT", fltkListGetSelectedTextAttrib, fltkListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", fltkListGetSelectionAttrib, fltkListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", fltkListGetCaretAttrib, fltkListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, fltkListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, fltkListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", fltkListGetReadOnlyAttrib, fltkListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, fltkListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, fltkListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, fltkListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", fltkListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);
}

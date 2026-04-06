/** \file
 * \brief Tabs Control - FLTK Implementation
 *
 * FLTK Fl_Tabs uses child Fl_Group widgets as tab pages. The visible
 * tab is controlled by Fl_Tabs::value(). Tab labels come from the
 * child Fl_Group::label().
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>

#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_tabs.h"
}

#include "iupfltk_drv.h"


class IupFltkTabs : public Fl_Tabs
{
public:
  Ihandle* iup_handle;
  int prev_index;
  int drag_source;
  int drag_target;
  int drag_start_x, drag_start_y;
  int is_dragging;

  IupFltkTabs(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Tabs(x, y, w, h), iup_handle(ih), prev_index(-1),
      drag_source(-1), drag_target(-1), drag_start_x(0), drag_start_y(0), is_dragging(0)
  {
  }

  int findTabIndex(int ex, int ey)
  {
    Fl_Widget* w = which(ex, ey);
    if (w)
      return find(w);
    return -1;
  }

protected:
  void draw() override
  {
    Fl_Tabs::draw();

    if (is_dragging && drag_source >= 0 && drag_target >= 0 && drag_source != drag_target)
    {
      /* Draw insertion indicator */
      int th = tab_height();
      int ind_y, ind_h;

      if (th > 0) { ind_y = y(); ind_h = th; }
      else { ind_y = y() + h() + th; ind_h = -th; }

      /* Find x position of target tab by querying tab_positions */
      tab_positions();

      int ind_x = x();
      if (drag_target < tab_count && tab_pos)
        ind_x = x() + tab_pos[drag_target];
      else if (tab_count > 0 && tab_pos && tab_width)
        ind_x = x() + tab_pos[tab_count - 1] + tab_width[tab_count - 1];

      fl_color(FL_SELECTION_COLOR);
      fl_line_style(FL_SOLID, 2);
      fl_line(ind_x, ind_y, ind_x, ind_y + ind_h);
      fl_line_style(FL_SOLID, 1);
    }
  }

public:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event);
        break;
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        break;
    }

    if (event == FL_PUSH && iup_handle->data->show_close)
    {
      /* Let close button handling happen first */
    }

    if (event == FL_PUSH && iupAttribGetBoolean(iup_handle, "ALLOWREORDER"))
    {
      int idx = findTabIndex(Fl::event_x(), Fl::event_y());
      if (idx >= 0)
      {
        drag_source = idx;
        drag_target = idx;
        drag_start_x = Fl::event_x();
        drag_start_y = Fl::event_y();
        is_dragging = 0;
      }
    }

    if (event == FL_DRAG && drag_source >= 0 && iupAttribGetBoolean(iup_handle, "ALLOWREORDER"))
    {
      int dx = Fl::event_x() - drag_start_x;
      int dy = Fl::event_y() - drag_start_y;
      if (!is_dragging && (dx * dx + dy * dy) >= 25)
        is_dragging = 1;

      if (is_dragging)
      {
        /* Find target using tab_positions */
        tab_positions();

        int mx = Fl::event_x() - x();
        int new_target = tab_count;

        for (int i = 0; i < tab_count; i++)
        {
          if (tab_pos && tab_width)
          {
            int mid = tab_pos[i] + tab_width[i] / 2;
            if (mx < mid)
            {
              new_target = i;
              break;
            }
          }
        }

        if (new_target != drag_target)
        {
          drag_target = new_target;
          redraw();
        }
      }
      return 1;
    }

    if (event == FL_RELEASE && drag_source >= 0 && iupAttribGetBoolean(iup_handle, "ALLOWREORDER"))
    {
      int src = drag_source;
      int tgt = drag_target;
      int was_dragging = is_dragging;

      drag_source = -1;
      drag_target = -1;
      is_dragging = 0;

      if (was_dragging && src >= 0 && tgt >= 0)
      {
        /* tgt is the insertion point index (where the line was drawn)
           Skip if dropping at the same position or right after source */
        int dst = tgt > src ? tgt - 1 : tgt;

        if (dst != src)
        {
          IFnii reorder_cb = (IFnii)IupGetCallback(iup_handle, "REORDER_CB");
          int ret = IUP_DEFAULT;
          if (reorder_cb)
            ret = reorder_cb(iup_handle, src, dst);

          if (ret != IUP_IGNORE)
          {
            iupAttribSet(iup_handle, "_IUPTABS_REORDERING", "1");

            Fl_Widget* fl_child = child(src);

            /* Reorder FLTK children using insert which handles same-group move */
            insert(*fl_child, tgt);

            /* Reorder IUP children to match */
            Ihandle* src_child = IupGetChild(iup_handle, src);
            if (src_child)
            {
              /* Find the new ref child after FLTK reorder */
              int new_pos = find(fl_child);
              Ihandle* ref_child = NULL;
              if (new_pos < IupGetChildCount(iup_handle))
                ref_child = IupGetChild(iup_handle, new_pos);

              IupReparent(src_child, iup_handle, ref_child);

              value(fl_child);
              prev_index = new_pos;
            }

            iupAttribSet(iup_handle, "_IUPTABS_REORDERING", NULL);
          }
        }
      }

      redraw();
      redraw_tabs();
      /* Don't pass FL_RELEASE to Fl_Tabs if we were dragging */
      if (was_dragging)
        return 1;
    }

    int ret = Fl_Tabs::handle(event);

    if (event == FL_PUSH && drag_source < 0)
    {
      Fl_Widget* cur = value();
      if (cur)
      {
        int new_index = find(cur);
        if (new_index != prev_index && new_index >= 0 && new_index < children())
        {
          Ihandle* child_ih = IupGetChild(iup_handle, new_index);
          Ihandle* prev_child = prev_index >= 0 ? IupGetChild(iup_handle, prev_index) : NULL;

          IFnnn cb = (IFnnn)IupGetCallback(iup_handle, "TABCHANGE_CB");
          if (cb)
            cb(iup_handle, child_ih, prev_child);
          else
          {
            IFnii cb2 = (IFnii)IupGetCallback(iup_handle, "TABCHANGEPOS_CB");
            if (cb2 && prev_index >= 0)
              cb2(iup_handle, new_index, prev_index);
          }

          prev_index = new_index;
        }
      }
    }

    return ret;
  }
};

static int fltkTabsGetTabHeight(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, NULL, &ch);
  return ch + 8;
}

/****************************************************************************
 * Tab Size / Position
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

extern "C" IUP_SDK_API int iupdrvTabsExtraMargin(void)
{
  return 0;
}

extern "C" IUP_SDK_API int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

extern "C" IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int w = 0, h = 0;

  if (tab_title)
  {
    iupdrvFontGetMultiLineStringSize(ih, tab_title, &w, &h);
    w += 16;
    h += 8;
  }
  else
  {
    iupdrvFontGetCharSize(ih, &w, &h);
    w = 40;
    h += 8;
  }

  if (tab_image)
  {
    void* img = iupImageGetImage(tab_image, ih, 0, NULL);
    if (img)
    {
      Fl_Image* fl_img = (Fl_Image*)img;
      w += fl_img->w() + 4;
      if (fl_img->h() + 8 > h)
        h = fl_img->h() + 8;
    }
  }

  if (ih->data->show_close)
    w += FL_NORMAL_SIZE / 2 + 2;

  if (tab_width) *tab_width = w;
  if (tab_height) *tab_height = h;
}

/****************************************************************************
 * Tab Navigation
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return;

  if (pos >= 0 && pos < tabs->children())
  {
    tabs->value(tabs->child(pos));
    tabs->prev_index = pos;
    tabs->redraw();
  }
}

extern "C" IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return -1;

  Fl_Widget* cur = tabs->value();
  if (cur)
    return tabs->find(cur);

  return -1;
}

extern "C" IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  (void)child;
  (void)pos;
  return 1;
}

/****************************************************************************
 * Attribute Handlers
 ****************************************************************************/

static int fltkTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return 0;

  if (pos >= 0 && pos < tabs->children())
  {
    Fl_Widget* page = tabs->child(pos);
    page->copy_label(value ? value : "");
    tabs->redraw();
    return 1;
  }
  return 0;
}

static int fltkTabsSetTabTipAttrib(Ihandle* ih, int pos, const char* value)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return 0;

  if (pos >= 0 && pos < tabs->children())
  {
    Fl_Widget* page = tabs->child(pos);
    page->copy_tooltip(value ? value : "");
    return 1;
  }
  return 0;
}

static int fltkTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return 0;

  Ihandle* child = IupGetChild(ih, pos);
  if (!child) return 0;

  if (pos >= 0 && pos < tabs->children())
  {
    Fl_Widget* page = tabs->child(pos);
    if (value)
    {
      const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
      Fl_Image* image = (Fl_Image*)iupImageGetImage(value, ih, 0, bgcolor);
      if (image)
      {
        page->image(image);
        tabs->tab_align(FL_ALIGN_IMAGE_NEXT_TO_TEXT);
      }
    }
    else
    {
      page->image(NULL);
    }
    tabs->redraw();
  }
  return 1;
}

static void fltkTabsCloseCallback(Fl_Widget* w, void* data)
{
  Ihandle* child = (Ihandle*)data;
  if (!child) return;

  if (Fl::callback_reason() != FL_REASON_CLOSED)
    return;

  Ihandle* ih = child->parent;
  if (!ih) return;

  int pos = IupGetChildPos(ih, child);

  IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
  if (cb)
  {
    int ret = cb(ih, pos);
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_CONTINUE)
    {
      IupDetach(child);
      IupDestroy(child);
    }
  }
}

static void fltkTabsApplyShowClose(Ihandle* ih, IupFltkTabs* tabs, int pos, Ihandle* child, int show)
{
  if (pos < 0 || pos >= tabs->children())
    return;

  Fl_Widget* page = tabs->child(pos);
  if (show)
  {
    page->when(page->when() | FL_WHEN_CLOSED);
    page->callback(fltkTabsCloseCallback, (void*)child);
  }
  else
  {
    page->when(page->when() & ~FL_WHEN_CLOSED);
  }
}

static int fltkTabsSetShowCloseAttrib(Ihandle* ih, int pos, const char* value)
{
  if (pos == IUP_INVALID_ID)
  {
    ih->data->show_close = iupStrBoolean(value);

    if (ih->handle)
    {
      IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
      for (int i = 0; i < tabs->children(); i++)
      {
        Ihandle* child = IupGetChild(ih, i);
        if (child)
          fltkTabsApplyShowClose(ih, tabs, i, child, ih->data->show_close);
      }
      tabs->redraw();
    }
    return 1;
  }

  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return 0;

  Ihandle* child = IupGetChild(ih, pos);
  if (child)
    iupAttribSetStr(child, "SHOWCLOSE", value);

  fltkTabsApplyShowClose(ih, tabs, pos, child, iupStrBoolean(value));
  tabs->redraw();

  return 0;
}

static int fltkTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return 0;

  if (pos >= 0 && pos < tabs->children())
  {
    Fl_Widget* page = tabs->child(pos);
    if (iupStrBoolean(value))
      page->show();
    else
      page->hide();
    tabs->redraw();
  }
  return 0;
}

static int fltkTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int fltkTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  return 1;
}

static int fltkTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (tabs)
  {
    tabs->selection_color(fl_rgb_color(r, g, b));
    tabs->redraw();
  }
  return 1;
}

static int fltkTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (tabs)
  {
    tabs->labelcolor(fl_rgb_color(r, g, b));
    tabs->redraw();
  }
  return 1;
}

static int fltkTabsSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  return 1;
}

static char* fltkTabsGetClientSizeAttrib(Ihandle* ih)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return NULL;

  int tab_h = fltkTabsGetTabHeight(ih);
  int w = tabs->w() - 4;
  int h = tabs->h() - tab_h - 4;
  if (w < 0) w = 0;
  if (h < 0) h = 0;

  return iupStrReturnIntInt(w, h, 'x');
}

static char* fltkTabsGetClientOffsetAttrib(Ihandle* ih)
{
  int tab_h = fltkTabsGetTabHeight(ih);
  return iupStrReturnIntInt(2, 2 + tab_h, 'x');
}

/****************************************************************************
 * Child Management
 ****************************************************************************/

static void* fltkTabsGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  Ihandle* parent = child;
  while (parent && parent->parent != ih)
    parent = parent->parent;

  if (parent)
  {
    Fl_Group* page = (Fl_Group*)iupAttribGet(parent, "_IUPTAB_CONTAINER");
    if (page)
      return (void*)page;
  }

  return (void*)ih->handle;
}

static void fltkTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (ih->handle)
  {
    IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
    int pos = IupGetChildPos(ih, child);
    int tab_h = fltkTabsGetTabHeight(ih);

    tabs->begin();

    Fl_Group* page = new Fl_Group(tabs->x() + 2, tabs->y() + tab_h + 2,
                                  tabs->w() - 4, tabs->h() - tab_h - 4);
    page->end();
    page->box(FL_NO_BOX);
    page->resizable(NULL);

    char* tabtitle = iupAttribGet(child, "TABTITLE");
    if (!tabtitle)
    {
      tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
      if (tabtitle)
        iupAttribSetStr(child, "TABTITLE", tabtitle);
    }

    if (!tabtitle)
      tabtitle = (char*)"     ";

    page->copy_label(tabtitle);

    char* tabimage = iupAttribGet(child, "TABIMAGE");
    if (!tabimage)
      tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
    if (tabimage)
    {
      const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
      Fl_Image* img = (Fl_Image*)iupImageGetImage(tabimage, ih, 0, bgcolor);
      if (img)
      {
        page->image(img);
        tabs->tab_align(FL_ALIGN_IMAGE_NEXT_TO_TEXT);
      }
    }

    char* child_show_close = iupAttribGet(child, "SHOWCLOSE");
    int do_show_close = child_show_close ? iupStrBoolean(child_show_close) : ih->data->show_close;
    if (do_show_close)
    {
      page->when(page->when() | FL_WHEN_CLOSED);
      page->callback(fltkTabsCloseCallback, (void*)child);
    }

    char* tabtip = iupAttribGet(child, "TABTIP");
    if (!tabtip)
      tabtip = iupAttribGetId(ih, "TABTIP", pos);
    if (tabtip)
      page->copy_tooltip(tabtip);

    tabs->end();

    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)page);
    iupAttribSet(child, "_IUPTAB_PAGE", (char*)page);

    if (pos != iupdrvTabsGetCurrentTab(ih))
      page->hide();

    if (tabs->children() == 1)
    {
      tabs->value(page);
      tabs->prev_index = 0;
    }
  }
}

static void fltkTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  if (ih->handle)
  {
    IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
    Fl_Group* page = (Fl_Group*)iupAttribGet(child, "_IUPTAB_CONTAINER");

    if (page)
    {
      tabs->remove(page);
      delete page;
    }

    iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
    iupAttribSet(child, "_IUPTAB_PAGE", NULL);

    if (tabs->children() > 0)
    {
      if (pos >= tabs->children())
        pos = tabs->children() - 1;
      iupdrvTabsSetCurrentTab(ih, pos);
    }

    tabs->redraw();
  }
}

/****************************************************************************
 * Layout
 ****************************************************************************/

static void fltkTabsLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (!tabs) return;

  int tab_h = fltkTabsGetTabHeight(ih);
  int page_x = tabs->x() + 2;
  int page_y = tabs->y() + tab_h + 2;
  int page_w = tabs->w() - 4;
  int page_h = tabs->h() - tab_h - 4;
  if (page_w < 1) page_w = 1;
  if (page_h < 1) page_h = 1;

  for (int i = 0; i < tabs->children(); i++)
  {
    Fl_Widget* page = tabs->child(i);
    page->resize(page_x, page_y, page_w, page_h);
  }
}

/****************************************************************************
 * Map / UnMap
 ****************************************************************************/

static int fltkTabsMapMethod(Ihandle* ih)
{
  IupFltkTabs* tabs = new IupFltkTabs(0, 0, 10, 10, ih);
  tabs->end();

  ih->handle = (InativeHandle*)tabs;

  iupfltkAddToParent(ih);

  if (ih->firstchild)
  {
    Ihandle* child;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");

    for (child = ih->firstchild; child; child = child->brother)
      fltkTabsChildAddedMethod(ih, child);

    if (current_child)
    {
      int pos = IupGetChildPos(ih, current_child);
      if (pos >= 0)
        iupdrvTabsSetCurrentTab(ih, pos);

      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
    }
    else
    {
      iupdrvTabsSetCurrentTab(ih, 0);
    }
  }

  return IUP_NOERROR;
}

static void fltkTabsUnMapMethod(Ihandle* ih)
{
  IupFltkTabs* tabs = (IupFltkTabs*)ih->handle;
  if (tabs)
  {
    delete tabs;
    ih->handle = NULL;
  }
}

/****************************************************************************
 * Class Registration
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = fltkTabsMapMethod;
  ic->UnMap = fltkTabsUnMapMethod;
  ic->LayoutUpdate = fltkTabsLayoutUpdateMethod;
  ic->ChildAdded = fltkTabsChildAddedMethod;
  ic->ChildRemoved = fltkTabsChildRemovedMethod;
  ic->GetInnerNativeContainerHandle = fltkTabsGetInnerNativeContainerHandleMethod;

  iupClassRegisterCallback(ic, "TABCHANGE_CB", "nn");
  iupClassRegisterCallback(ic, "TABCHANGEPOS_CB", "ii");
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  iupClassRegisterAttribute(ic, "FONT", NULL, fltkTabsSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, fltkTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, (IattribSetIdFunc)fltkTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, (IattribSetIdFunc)fltkTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, (IattribSetIdFunc)fltkTabsSetTabImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTIP", NULL, (IattribSetIdFunc)fltkTabsSetTabTipAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", NULL, (IattribSetIdFunc)fltkTabsSetShowCloseAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, fltkTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* only TOP and BOTTOM, LEFT/RIGHT not supported */

  iupClassRegisterAttribute(ic, "CLIENTSIZE", fltkTabsGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", fltkTabsGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
}

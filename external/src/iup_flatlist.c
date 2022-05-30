/** \file
 * \brief List Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_assert.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_register.h"
#include "iup_flatscrollbar.h"



typedef struct _iFlatListItem {
  char* title;
  char* image;
  char* fg_color;
  char* bg_color;
  char* tip;
  char* font;
  int selected;
} iFlatListItem;

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  Iarray *items_array;

  /* aux */
  int line_height, line_width;
  int dragover_pos, dragged_pos;
  int has_focus, focus_pos;

  /* attributes */
  int horiz_padding, vert_padding;  /* item internal margin */
  int spacing, 
    icon_spacing, img_position;        /* used when both text and image are displayed */
  int horiz_alignment, vert_alignment;
  int border_width;
  int is_multiple;
  int show_dragdrop;
};


static int iFlatListGetScrollbar(Ihandle* ih)
{
  int flat = iupFlatScrollBarGet(ih);
  if (flat != IUP_SB_NONE)
    return flat;
  else
  {
    if (!ih->handle)
      ih->data->canvas.sb = iupBaseGetScrollbar(ih);

    return ih->data->canvas.sb;
  }
}

static int iFlatListGetScrollbarSize(Ihandle* ih)
{
  if (iupFlatScrollBarGet(ih) != IUP_SB_NONE)
  {
    if (iupAttribGetBoolean(ih, "SHOWFLOATING"))
      return 0;
    else
      return iupAttribGetInt(ih, "SCROLLBARSIZE");
  }
  else
    return iupdrvGetScrollbarSize();
}

static int iFlatListConvertXYToPos(Ihandle* ih, int x, int y)
{
  int posy = IupGetInt(ih, "POSY");
  int count = iupArrayCount(ih->data->items_array);
  int pos = ((y + posy) / (ih->data->line_height + ih->data->spacing)) + 1; /* pos starts at 1 */

  if (y + posy < 0 || pos < 1 || pos > count)
    return -1;

  (void)x;
  return pos;
}

static void iFlatListCopyItem(Ihandle *ih, int from, int to)
{
  int count = iupArrayCount(ih->data->items_array);
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  iFlatListItem copy = items[from - 1];
  int i;

  if (to <= count)
  {
    items = (iFlatListItem*)iupArrayInsert(ih->data->items_array, to - 1, 1);
    i = to - 1;
  }
  else
  {
    items = (iFlatListItem*)iupArrayInc(ih->data->items_array);
    i = count;
  }

  items[i].title = iupStrDup(copy.title);
  items[i].image = iupStrDup(copy.image);
  items[i].fg_color = iupStrDup(copy.fg_color);
  items[i].bg_color = iupStrDup(copy.bg_color);
  items[i].tip = iupStrDup(copy.tip);
  items[i].font = iupStrDup(copy.font);
  items[i].selected = 0;
}

static void iFlatListRemoveItem(Ihandle *ih, int start, int remove_count)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int i;
  for (i = start; i < remove_count; i++)
  {
    if (items[i].title)
      free(items[i].title);

    if (items[i].image)
      free(items[i].image);

    if (items[i].fg_color)
      free(items[i].fg_color);

    if (items[i].bg_color)
      free(items[i].bg_color);

    if (items[i].tip)
      free(items[i].tip);

    if (items[i].font)
      free(items[i].font);
  }
  iupArrayRemove(ih->data->items_array, start, remove_count);
}

static void iFlatListSetItemFont(Ihandle* ih, const char* font)
{
  if (font)
    iupAttribSetStr(ih, "DRAWFONT", font);
  else
  {
    font = IupGetAttribute(ih, "FONT");
    iupAttribSetStr(ih, "DRAWFONT", font);
  }
}

static void iFlatListCalcItemMaxSize(Ihandle *ih, iFlatListItem* items, int count, int *max_w, int *max_h)
{
  int i;

  *max_w = 0;
  *max_h = 0;

  iupdrvFontGetCharSize(ih, NULL, max_h);

  for (i = 0; i < count; i++)
  {
    int item_width, item_height;
    char *text = items[i].title;
    char* imagename = items[i].image;

    iFlatListSetItemFont(ih, items[i].font);

    iupFlatDrawGetIconSize(ih, ih->data->img_position, ih->data->icon_spacing, ih->data->horiz_padding, ih->data->vert_padding, imagename, text, &item_width, &item_height, 0.0);

    if (item_width > *max_w) *max_w = item_width;
    if (item_height > *max_h) *max_h = item_height;
  }
}

static void iFlatListUpdateScrollBar(Ihandle *ih)
{
  int canvas_width = ih->currentwidth;
  int canvas_height = ih->currentheight;
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int sb, max_w, max_h, view_width, view_height;

  if (iupAttribGetBoolean(ih, "BORDER")) /* native border around scrollbars */
  {
    canvas_width -= 2;
    canvas_height -= 2;
  }

  canvas_width -= 2 * ih->data->border_width;
  canvas_height -= 2 * ih->data->border_width;

  iFlatListCalcItemMaxSize(ih, items, count, &max_w, &max_h);

  ih->data->line_width = iupMAX(max_w, canvas_width);
  ih->data->line_height = max_h;

  view_width = max_w;
  view_height = max_h * count;
  view_height += (count - 1) * ih->data->spacing;

  if (ih->data->show_dragdrop || iupAttribGetBoolean(ih, "DRAGDROPLIST"))
    view_height += ih->data->line_height/2; /* additional space for drop area */

  sb = iFlatListGetScrollbar(ih);
  if (sb)
  {
    int sb_size = iFlatListGetScrollbarSize(ih);
    int noscroll_width = canvas_width;
    int noscroll_height = canvas_height;

    if (sb & IUP_SB_HORIZ)
    {
      IupSetInt(ih, "XMAX", view_width);

      if (view_height > noscroll_height)  /* affects horizontal size */
        canvas_width -= sb_size;
    }
    else
      IupSetAttribute(ih, "XMAX", "0");

    if (sb & IUP_SB_VERT)
    {
      IupSetInt(ih, "YMAX", view_height);

      if (view_width > noscroll_width)  /* affects vertical size */
        canvas_height -= sb_size;
    }
    else
      IupSetAttribute(ih, "YMAX", "0");

    /* check again, adding a scrollbar may affect the other scrollbar need if not done already */
    if (sb & IUP_SB_HORIZ && view_height <= noscroll_height && view_height > canvas_height)
      canvas_width -= sb_size;
    if (sb & IUP_SB_VERT && view_width <= noscroll_width && view_width > canvas_width)
      canvas_height -= sb_size;

    if (canvas_width < 0) canvas_width = 0;
    if (canvas_height < 0) canvas_height = 0;

    if (sb & IUP_SB_HORIZ)
      IupSetInt(ih, "DX", canvas_width);
    else
      IupSetAttribute(ih, "DX", "0");

    if (sb & IUP_SB_VERT)
      IupSetInt(ih, "DY", canvas_height);
    else
      IupSetAttribute(ih, "DY", "0");

    IupSetfAttribute(ih, "LINEY", "%d", ih->data->line_height + ih->data->spacing);
  }
  else
  {
    IupSetAttribute(ih, "XMAX", "0");
    IupSetAttribute(ih, "YMAX", "0");

    IupSetAttribute(ih, "DX", "0");
    IupSetAttribute(ih, "DY", "0");
  }
}


/*******************************************************************************************************/


static int iFlatListRedraw_CB(Ihandle* ih)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int text_flags = iupDrawGetTextFlags(ih, "TABSTEXTALIGNMENT", "TABSTEXTWRAP", "TABSTEXTELLIPSIS");
  char* foreground_color = iupAttribGetStr(ih, "FGCOLOR");
  char* background_color = iupAttribGetStr(ih, "BGCOLOR");
  int posx = IupGetInt(ih, "POSX");
  int posy = IupGetInt(ih, "POSY");
  char* back_image = iupAttribGet(ih, "BACKIMAGE");
  int i, x, y, make_inactive = 0;
  int border_width = ih->data->border_width;
  int active = IupGetInt(ih, "ACTIVE");  /* native implementation */
  int focus_feedback = iupAttribGetBoolean(ih, "FOCUSFEEDBACK");
  int width, height;

  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);

  iupdrvDrawGetSize(dc, &width, &height);

  iupFlatDrawBox(dc, border_width, width - border_width - 1, border_width, height - border_width - 1, background_color, background_color, 1);

  if (back_image)
  {
    int backimage_zoom = iupAttribGetBoolean(ih, "BACKIMAGEZOOM");
    if (backimage_zoom)
      iupdrvDrawImage(dc, back_image, 0, background_color, border_width, border_width, width - border_width, height - border_width);
    else
      iupdrvDrawImage(dc, back_image, 0, background_color, border_width, border_width, -1, -1);
  }

  if (!active)
    make_inactive = 1;

  x = -posx + border_width;
  y = -posy + border_width;

  for (i = 0; i < count; i++)
  {
    char *fgcolor = (items[i].fg_color) ? items[i].fg_color : foreground_color;
    char *bgcolor = (items[i].bg_color) ? items[i].bg_color : background_color;

    if (items[i].selected)
    {
      char* ps_color = iupAttribGetStr(ih, "PSCOLOR");
      char* text_ps_color = iupAttribGetStr(ih, "TEXTPSCOLOR");
      if (text_ps_color)
        fgcolor = text_ps_color;
      if (ps_color)
        bgcolor = ps_color;
    }

    /* item background */
    iupFlatDrawBox(dc, x, x + ih->data->line_width - 1, y, y + ih->data->line_height - 1, bgcolor, bgcolor, 1);

    iFlatListSetItemFont(ih, items[i].font);

    /* text and image */
    iupFlatDrawIcon(ih, dc, x, y, ih->data->line_width, ih->data->line_height,
                    ih->data->img_position, ih->data->icon_spacing, ih->data->horiz_alignment, ih->data->vert_alignment, ih->data->horiz_padding, ih->data->vert_padding,
                    items[i].image, make_inactive, items[i].title, text_flags, 0, fgcolor, bgcolor, active);

    if (items[i].selected || (ih->data->show_dragdrop && ih->data->dragover_pos == i + 1))
    {
      unsigned char a = (unsigned char)iupAttribGetInt(ih, "HLCOLORALPHA");
      if (a != 0)
      {
        long selcolor;
        unsigned char red, green, blue;
        char* hlcolor = iupAttribGetStr(ih, "HLCOLOR");

        if (ih->data->show_dragdrop && ih->data->dragover_pos == i + 1)
          a = (2 * a) / 3;

        iupStrToRGB(hlcolor, &red, &green, &blue);
        selcolor = iupDrawColor(red, green, blue, a);

        iupdrvDrawRectangle(dc, x, y, x + ih->data->line_width - 1, y + ih->data->line_height - 1, selcolor, IUP_DRAW_FILL, 1);
      }
    }

    if (ih->data->has_focus && ih->data->focus_pos == i+1 && focus_feedback)
      iupdrvDrawFocusRect(dc, x, y, x + width - border_width - 1, y + ih->data->line_height - 1);

    y += ih->data->line_height + ih->data->spacing;
  }

  if (border_width)
  {
    char* bordercolor = iupAttribGetStr(ih, "BORDERCOLOR");
    iupFlatDrawBorder(dc, 0, width - 1,
                          0, height - 1,
                          border_width, bordercolor, background_color, active);
  }

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static char* iFlatListGetIdValueAttrib(Ihandle* ih, int pos);

static void iFlatListCallActionCallback(Ihandle* ih, IFnsii cb, int pos, int state)
{
  char *text;

  if (pos < 1 || !cb)
    return;

  text = iFlatListGetIdValueAttrib(ih, pos);

  if (cb(ih, text, pos, state) == IUP_CLOSE)
    IupExitLoop();
}

static void iFlatListSingleCallActionCb(Ihandle* ih, IFnsii cb, IFn valuechanged_cb, int pos, int old_pos)
{
  int unchanged = 1;

  if (old_pos != -1)
  {
    if (old_pos != pos)
    {
      iFlatListCallActionCallback(ih, cb, old_pos, 0);
      iFlatListCallActionCallback(ih, cb, pos, 1);
      unchanged = 0;
    }
  }
  else
  {
    iFlatListCallActionCallback(ih, cb, pos, 1);
    unchanged = 0;
  }

  if (!unchanged && valuechanged_cb)
    valuechanged_cb(ih);
}

static void iFlatListMultipleCallActionCb(Ihandle* ih, IFnsii cb, IFns multi_cb, IFn valuechanged_cb, char* str, int count)
{
  int unchanged = 1;

  if (multi_cb)
  {
    if (multi_cb(ih, str) == IUP_CLOSE)
      IupExitLoop();
  }
  else
  {
    int i;

    /* must simulate the click on each item */
    for (i = 0; i < count; i++)
    {
      if (str[i] != 'x')
      {
        if (str[i] == '+')
          iFlatListCallActionCallback(ih, cb, i + 1, 1);
        else
          iFlatListCallActionCallback(ih, cb, i + 1, 0);
        unchanged = 0;
      }
    }
  }

  if (!unchanged && valuechanged_cb)
    valuechanged_cb(ih);
}

static void iFlatListSelectItem(Ihandle* ih, int pos, int ctrlPressed, int shftPressed)
{
  IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
  IFnsii cb = (IFnsii)IupGetCallback(ih, "FLAT_ACTION");
  IFn vc_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);

  ih->data->focus_pos = pos;

  if (ih->data->is_multiple)
  {
    int i, start, end;
    char* str;
    char *val = iupAttribGet(ih, "_IUPFLATLIST_LASTSELECTED");
    int last_pos = (val) ? atoi(val) : 0;
    if (pos <= last_pos)
    {
      start = pos - 1;
      end = last_pos - 1;
    }
    else
    {
      start = last_pos - 1;
      end = pos - 1;
    }

    str = malloc(count + 1);
    memset(str, 'x', count); /* mark all as unchanged */
    str[count] = 0;

    if (!ctrlPressed)
    {
      /* un-select all */
      for (i = 0; i < count; i++)
      {
        if (items[i].selected)
        {
          str[i] = '-';
          items[i].selected = 0;
        }
      }
    }

    if (shftPressed)
    {
      /* select interval */
      for (i = start; i <= end; i++)
      {
        if (!items[i].selected)
        {
          str[i] = '+';
          items[i].selected = 1;
        }
      }
    }
    else
    {
      i = pos - 1;

      if (ctrlPressed)
      {
        /* toggle selection */
        if (items[i].selected)
        {
          str[i] = '-';
          items[i].selected = 0;
        }
        else
        {
          str[i] = '+';
          items[i].selected = 1;
        }
      }
      else
      {
        if (!items[i].selected)
        {
          str[i] = '+';
          items[i].selected = 1;
        }
      }
    }

    if (multi_cb || cb)
      iFlatListMultipleCallActionCb(ih, cb, multi_cb, vc_cb, str, count);

    free(str);
  }
  else
  {
    int i, old_pos = -1;

    for (i = 0; i < count; i++)
    {
      if (!items[i].selected)
        continue;
      items[i].selected = 0;
      old_pos = i + 1;
      break;
    }
    items[pos - 1].selected = 1;

    if (cb || vc_cb)
      iFlatListSingleCallActionCb(ih, cb, vc_cb, pos, old_pos);
  }

  if (!shftPressed)
    iupAttribSetInt(ih, "_IUPFLATLIST_LASTSELECTED", pos);
}

static int iFlatListCallDragDropCb(Ihandle* ih, int drag_id, int drop_id, int is_ctrl, int is_shift)
{
  IFniiii cbDragDrop = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");

  /* ignore a drop that will do nothing */
  if (is_ctrl == 0 && (drag_id + 1 == drop_id || drag_id == drop_id))
    return IUP_DEFAULT;
  if (is_ctrl != 0 && drag_id == drop_id)
    return IUP_DEFAULT;

  drag_id++;
  if (drop_id < 0)
    drop_id = -1;
  else
    drop_id++;

  if (cbDragDrop)
    return cbDragDrop(ih, drag_id, drop_id, is_shift, is_ctrl);  /* starts at 1 */

  return IUP_CONTINUE; /* allow to move/copy by default if callback not defined */
}

static int iFlatListButton_CB(Ihandle* ih, int button, int pressed, int x, int y, char* status)
{
  IFniiiis button_cb = (IFniiiis)IupGetCallback(ih, "FLAT_BUTTON_CB");
  int pos = iFlatListConvertXYToPos(ih, x, y);

  if (button_cb)
  {
    if (button_cb(ih, button, pressed, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (button == IUP_BUTTON1 && !pressed && ih->data->show_dragdrop && ih->data->dragged_pos > 0)
  {
    if (pos == -1)
    {
      if (y < 0) 
        pos = 1;
      else
      {
        int count = iupArrayCount(ih->data->items_array);
        pos = count + 1;
      }
    }

    if (iFlatListCallDragDropCb(ih, ih->data->dragged_pos, pos, iup_iscontrol(status), iup_isshift(status)) == IUP_CONTINUE)
    {
      iFlatListCopyItem(ih, ih->data->dragged_pos, pos);

      /* select the dropped item */
      iFlatListSelectItem(ih, pos, 0, 0); /* force no ctrl and no shift for selection */

      if (!iup_iscontrol(status))
      {
        if (ih->data->dragged_pos < pos)
          ih->data->dragged_pos--;

        iFlatListRemoveItem(ih, ih->data->dragged_pos, 1);
      }
    }

    ih->data->dragover_pos = 0;
    ih->data->dragged_pos = 0;

    iFlatListUpdateScrollBar(ih);
    IupUpdate(ih);
    return IUP_DEFAULT;
  }

  if (pos == -1)
    return IUP_DEFAULT;

  if (button == IUP_BUTTON1 && pressed)
  {
    iFlatListSelectItem(ih, pos, iup_iscontrol(status), iup_isshift(status));

    if (ih->data->show_dragdrop)
      ih->data->dragged_pos = pos;
  }

  if (iup_isdouble(status))
  {
    IFnis dc_cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
    if (dc_cb)
    {
      iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
      if (dc_cb(ih, pos, items[pos - 1].title) == IUP_IGNORE)
        return IUP_DEFAULT;
    }
  }

  IupUpdate(ih);
  return IUP_DEFAULT;
}

static int iFlatListMotion_CB(Ihandle* ih, int x, int y, char* status)
{
  IFniis motion_cb = (IFniis)IupGetCallback(ih, "FLAT_MOTION_CB");
  int pos;

  iupFlatScrollBarMotionUpdate(ih, x, y);

  if (motion_cb)
  {
    if (motion_cb(ih, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  pos = iFlatListConvertXYToPos(ih, x, y);
  if (pos == -1)
  {
    iupFlatItemResetTip(ih);
    return IUP_DEFAULT;
  }
  else
  {
    iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
    char* item_tip = items[pos-1].tip;
    if (item_tip)
      iupFlatItemSetTip(ih, item_tip);
    else
      iupFlatItemResetTip(ih);
  }

  if (!iup_isbutton1(status))
    return IUP_IGNORE;

  /* button1 is pressed => dragging */

  if (ih->data->is_multiple && !ih->data->show_dragdrop)
    iFlatListSelectItem(ih, pos, 0, 1);

  if (y < 0 || y > ih->currentheight)
  {
    /* scroll if dragging out of canvas */
    int posy = (pos - 1) * (ih->data->line_height + ih->data->spacing);
    IupSetInt(ih, "POSY", posy);
  }

  if (ih->data->show_dragdrop && ih->data->dragged_pos > 0)
    ih->data->dragover_pos = pos;

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatListLeaveWindow_CB(Ihandle* ih)
{
  IFn cb = (IFn)IupGetCallback(ih, "FLAT_LEAVEWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  iupFlatItemResetTip(ih);

  return IUP_DEFAULT;
}

static int iFlatListFocus_CB(Ihandle* ih, int focus)
{
  IFni cb = (IFni)IupGetCallback(ih, "FLAT_FOCUS_CB");
  if (cb)
  {
    if (cb(ih, focus) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  ih->data->has_focus = focus;
  iupdrvRedrawNow(ih);

  return IUP_DEFAULT;
}

static int iFlatListScroll_CB(Ihandle* ih, int action, float posx, float posy)
{
  (void)action;
  (void)posx;
  (void)posy;
  iupdrvRedrawNow(ih);  /* so FLATSCROLLBAR can also work */
  return IUP_DEFAULT;
}

static int iFlatListResize_CB(Ihandle* ih, int width, int height)
{
  (void)width;
  (void)height;

  iFlatListUpdateScrollBar(ih);

  return IUP_DEFAULT;
}

static void iFlatListScrollFocusVisible(Ihandle* ih)
{
  int focus_posy = (ih->data->focus_pos - 1) * (ih->data->line_height + ih->data->spacing);
  int posy = IupGetInt(ih, "POSY");
  if (focus_posy < posy)
    IupSetInt(ih, "POSY", focus_posy);
  else
  {
    int item_height = ih->data->line_height + ih->data->spacing;
    int dy = IupGetInt(ih, "DY");
    if (focus_posy + item_height > posy + dy)
    {
      posy = focus_posy + item_height - dy;
      IupSetInt(ih, "POSY", posy);
    }
  }
}

static int iFlatListKUp_CB(Ihandle* ih)
{
  if (ih->data->has_focus)
  {
    if (ih->data->focus_pos > 1)
    {
      int ctrlPressed = 0; /* behave as no ctrl key pressed when using arrow keys */
      int shftPressed = IupGetInt(NULL, "SHIFTKEY");

      iFlatListSelectItem(ih, ih->data->focus_pos - 1, ctrlPressed, shftPressed);

      iFlatListScrollFocusVisible(ih);
      IupUpdate(ih);
    }
  }
  return IUP_DEFAULT;
}

static int iFlatListKDown_CB(Ihandle* ih)
{
  if (ih->data->has_focus)
  {
    int count = iupArrayCount(ih->data->items_array);
    if (ih->data->focus_pos < count)
    {
      int ctrlPressed = 0; /* behave as no ctrl key pressed when using arrow keys */
      int shftPressed = IupGetInt(NULL, "SHIFTKEY");

      iFlatListSelectItem(ih, ih->data->focus_pos + 1, ctrlPressed, shftPressed);

      iFlatListScrollFocusVisible(ih);
      IupUpdate(ih);
    }
  }
  return IUP_DEFAULT;
}

static int iFlatListFindNextItem(Ihandle* ih, char c)
{
  int count = iupArrayCount(ih->data->items_array);
  int start = (ih->data->focus_pos - 1) + 1;
  int i, end = count;
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);

  if (start == end)
  {
    end = start - 1;
    start = 0;
  }

  c = iup_tolower(c);

  for (i = start; i < end; i++)
  {
    char* title = items[i].title;

    if (title && iup_tolower(title[0]) == c)
      return i + 1;
  }

  return -1;
}

static int iFlatListKAny_CB(Ihandle* ih, int c)
{
  if (ih->data->has_focus)
  {
    int pos = iFlatListFindNextItem(ih, (char)c);
    if (pos > 0)
    {
      int ctrlPressed = 0; /* behave as no ctrl key pressed when using arrow keys */
      int shftPressed = IupGetInt(NULL, "SHIFTKEY");

      iFlatListSelectItem(ih, pos, ctrlPressed, shftPressed);

      iFlatListScrollFocusVisible(ih);
      IupUpdate(ih);
    }
  }
  return IUP_CONTINUE;
}

static int iFlatListKPgUp_CB(Ihandle* ih)
{
  if (ih->data->has_focus)
  {
    int posy = IupGetInt(ih, "POSY");
    int dy = IupGetInt(ih, "DY");

    posy -= dy;
    IupSetInt(ih, "POSY", posy);

    IupUpdate(ih);
  }
  return IUP_DEFAULT;
}

static int iFlatListKPgDn_CB(Ihandle* ih)
{
  if (ih->data->has_focus)
  {
    int posy = IupGetInt(ih, "POSY");
    int dy = IupGetInt(ih, "DY");

    posy += dy;
    IupSetInt(ih, "POSY", posy);

    IupUpdate(ih);
  }
  return IUP_DEFAULT;
}


/******************************************************************************************/


static int iFlatListSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char value1[30], value2[30];

  iupStrToStrStr(value, value1, value2, ':');

  ih->data->horiz_alignment = iupFlatGetHorizontalAlignment(value1);
  ih->data->vert_alignment = iupFlatGetVerticalAlignment(value2);

  if (ih->handle)
    iupdrvRedrawNow(ih);

  return 1;
}

static char* iFlatListGetAlignmentAttrib(Ihandle *ih)
{
  char* horiz_align2str[3] = { "ALEFT", "ACENTER", "ARIGHT" };
  char* vert_align2str[3] = { "ATOP", "ACENTER", "ABOTTOM" };
  return iupStrReturnStrf("%s:%s", horiz_align2str[ih->data->horiz_alignment], vert_align2str[ih->data->vert_alignment]);
}

static char* iFlatListGetSpacingAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->spacing);
}

static char* iFlatListGetHasFocusAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->has_focus);
}

static char* iFlatListGetIdValueAttrib(Ihandle* ih, int pos)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);

  if (pos < 1 || pos > count)
    return 0;

  return items[pos - 1].title;
}

static int iFlatListSetIdValueAttrib(Ihandle* ih, int pos, const char* value)
{
  int count = iupArrayCount(ih->data->items_array);

  if (pos < 1)
    return 0;

  if (!value) /* remove remaining items */
  {
    if (pos <= count)
      iFlatListRemoveItem(ih, pos-1, count - (pos-1));
  }
  else if (pos <= count) /* change an existing item */
  {
    iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);

    if (items[pos - 1].title)
      free(items[pos - 1].title);
    items[pos - 1].title = iupStrDup(value);
  }
  else /* add a new item */
  {
    iFlatListItem* items = (iFlatListItem*)iupArrayInsert(ih->data->items_array, count, (pos-1) - (count-1));
    items[pos - 1].title = iupStrDup(value);
  }

  if (ih->handle)
  {
    iFlatListUpdateScrollBar(ih);
    IupUpdate(ih);
  }

  return 0;
}

static int iFlatListSetAppendItemAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    iFlatListItem* items = (iFlatListItem*)iupArrayInc(ih->data->items_array);
    int count = iupArrayCount(ih->data->items_array);
    items[count - 1].title = iupStrDup(value);
  }

  if (ih->handle)
  {
    iFlatListUpdateScrollBar(ih);
    IupUpdate(ih);
  }
  return 0;
}

static int iFlatListSetInsertItemAttrib(Ihandle* ih, int pos, const char* value)
{
  int count = iupArrayCount(ih->data->items_array);

  if (pos < 1 || pos > count)
    return 0;

  if (value)
  {
    iFlatListItem* items = (iFlatListItem*)iupArrayInsert(ih->data->items_array, pos-1, 1);
    items[pos - 1].title = iupStrDup(value);
  }

  if (ih->handle)
  {
    iFlatListUpdateScrollBar(ih);
    IupUpdate(ih);
  }

  return 0;
}

static int iFlatListSetRemoveItemAttrib(Ihandle* ih, const char* value)
{
  if (!value || iupStrEqualNoCase(value, "ALL"))
    iFlatListRemoveItem(ih, 0, iupArrayCount(ih->data->items_array));
  else
  {
    int pos;
    if (iupStrToInt(value, &pos))
      iFlatListRemoveItem(ih, pos-1, 1);
  }

  if (ih->handle)
  {
    iFlatListUpdateScrollBar(ih);
    IupUpdate(ih);
  }

  return 0;
}

static int iFlatListSetImageAttribId(Ihandle* ih, int pos, const char* value)
{
  int count = iupArrayCount(ih->data->items_array);
  iFlatListItem *items;

  if (pos < 1 || pos > count)
    return 0;

  items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);

  if (items[pos - 1].image)
    free(items[pos - 1].image);
  items[pos-1].image = iupStrDup(value);

  if (ih->handle)
  {
    iFlatListUpdateScrollBar(ih);
    IupUpdate(ih);
  }

  return 0;
}

static char* iFlatListGetImageAttribId(Ihandle* ih, int pos)
{
  int count = iupArrayCount(ih->data->items_array);
  iFlatListItem *items;

  if (pos < 1 || pos > count)
    return 0;

  items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  return items[pos - 1].image;
}

static char* iFlatListGetImageNativeHandleAttribId(Ihandle* ih, int pos)
{
  int count = iupArrayCount(ih->data->items_array);
  iFlatListItem *items;

  if (pos < 1 || pos > count)
    return 0;

  items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  if (items[pos - 1].image)
    return iupImageGetImage(items[pos - 1].image, ih, 0, NULL);

  return NULL;
}

static int iFlatListSetImagePositionAttrib(Ihandle* ih, const char* value)
{
  ih->data->img_position = iupFlatGetImagePosition(value);

  if (ih->handle)
    IupUpdate(ih);

  return 0;
}

static char* iFlatListGetImagePositionAttrib(Ihandle *ih)
{
  char* img_pos2str[4] = { "LEFT", "RIGHT", "TOP", "BOTTOM" };
  return img_pos2str[ih->data->img_position];
}

static char* iFlatListGetShowDragDropAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->show_dragdrop);
}

static int iFlatListSetShowDragDropAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->show_dragdrop = 1;
  else
    ih->data->show_dragdrop = 0;

  return 0;
}

static int iFlatListDropData_CB(Ihandle *ih, char* type, void* data, int len, int x, int y)
{
  int pos = IupConvertXYToPos(ih, x, y);
  int is_ctrl = 0;
  char key[5];

  /* Data is not the pointer, it contains the pointer */
  Ihandle* ih_source;
  memcpy((void*)&ih_source, data, len);  /* but ih_source can be IupList or IupFlatList, can NOT use ih_source->data here */

  if (!IupClassMatch(ih_source, "flatlist") && !IupClassMatch(ih_source, "list"))
    return IUP_DEFAULT;

  /* A copy operation is enabled with the CTRL key pressed, or else a move operation will occur.
     A move operation will be possible only if the attribute DRAGSOURCEMOVE is Yes.
     When no key is pressed the default operation is copy when DRAGSOURCEMOVE=No and move when DRAGSOURCEMOVE=Yes. */
  iupdrvGetKeyState(key);
  if (key[1] == 'C')
    is_ctrl = 1;

  if (IupGetInt(ih_source, "MULTIPLE"))
  {
    char *buffer = IupGetAttribute(ih_source, "VALUE");

    /* Copy all selected items */
    int src_pos = 1;  /* IUP starts at 1 */
    while (buffer[src_pos - 1] != '\0')
    {
      if (buffer[src_pos - 1] == '+')
      {
        iFlatListItem* items = (iFlatListItem*)iupArrayInsert(ih->data->items_array, pos - 1, 1);
        items[pos - 1].title = iupStrDup(IupGetAttributeId(ih_source, "", src_pos));
        items[pos - 1].image = iupStrDup(IupGetAttributeId(ih_source, "IMAGE", src_pos));   /* works for IupFlatList only, in IupList is write-only */
        items[pos - 1].fg_color = iupStrDup(IupGetAttributeId(ih_source, "ITEMFGCOLOR", src_pos));
        items[pos - 1].bg_color = iupStrDup(IupGetAttributeId(ih_source, "ITEMBGCOLOR", src_pos));
        items[pos - 1].tip = iupStrDup(IupGetAttributeId(ih_source, "ITEMTIP", src_pos));
        items[pos - 1].font = iupStrDup(IupGetAttributeId(ih_source, "ITEMFONT", src_pos));
        items[pos - 1].selected = 0;
        pos++;
      }

      src_pos++;
    }

    if (IupGetInt(ih_source, "DRAGSOURCEMOVE") && !is_ctrl)
    {
      /* Remove all item from source if MOVE */
      src_pos = 1;  /* IUP starts at 1 */
      while (*buffer != '\0')
      {
        if (*buffer == '+')
          IupSetInt(ih_source, "REMOVEITEM", src_pos);

        src_pos++;
        buffer++;
      }
    }
  }
  else
  {
    iFlatListItem* items = (iFlatListItem*)iupArrayInsert(ih->data->items_array, pos - 1, 1);
    int src_pos = IupGetInt(ih_source, "VALUE");
    items[pos - 1].title = iupStrDup(IupGetAttributeId(ih_source, "", src_pos));
    items[pos - 1].image = iupStrDup(IupGetAttributeId(ih_source, "IMAGE", src_pos));   /* works for IupFlatList only, in IupList is write-only */
    items[pos - 1].fg_color = iupStrDup(IupGetAttributeId(ih_source, "ITEMFGCOLOR", src_pos));
    items[pos - 1].bg_color = iupStrDup(IupGetAttributeId(ih_source, "ITEMBGCOLOR", src_pos));
    items[pos - 1].tip = iupStrDup(IupGetAttributeId(ih_source, "ITEMTIP", src_pos));
    items[pos - 1].font = iupStrDup(IupGetAttributeId(ih_source, "ITEMFONT", src_pos));
    items[pos - 1].selected = 0;

    if (IupGetInt(ih_source, "DRAGSOURCEMOVE") && !is_ctrl)
    {
      src_pos = iupAttribGetInt(ih_source, "_IUP_LIST_SOURCEPOS");
      IupSetInt(ih_source, "REMOVEITEM", src_pos);
    }
  }

  if (ih->handle)
  {
    iFlatListUpdateScrollBar(ih);
    IupUpdate(ih);
  }

  (void)type;
  return IUP_DEFAULT;
}

static int iFlatListDragData_CB(Ihandle *ih, char* type, void *data, int len)
{
  int pos = iupAttribGetInt(ih, "_IUP_LIST_SOURCEPOS");
  if (pos < 1)
    return IUP_DEFAULT;

  if (ih->data->is_multiple)
  {
    char *buffer = IupGetAttribute(ih, "VALUE");

    /* It will not drag all selected items only
    when the user begins to drag an item not selected.
    In this case, unmark all and mark only this item.  */
    if (buffer[pos - 1] == '-')
    {
      int buf_len = (int)strlen(buffer);
      IupSetAttribute(ih, "SELECTION", "NONE");
      memset(buffer, '-', buf_len);
      buffer[pos - 1] = '+';
      IupSetAttribute(ih, "VALUE", buffer);
    }
  }
  else
  {
    /* Single selection */
    IupSetInt(ih, "VALUE", pos);
  }

  /* Copy source handle */
  memcpy(data, (void*)&ih, len);

  (void)type;
  return IUP_DEFAULT;
}


static int iFlatListDragDataSize_CB(Ihandle* ih, char* type)
{
  (void)ih;
  (void)type;
  return sizeof(Ihandle*);
}

static int iFlatListDragBegin_CB(Ihandle* ih, int x, int y)
{
  int pos = IupConvertXYToPos(ih, x, y);
  iupAttribSetInt(ih, "_IUP_LIST_SOURCEPOS", pos);  /* works for IupList and IupFlatList */
  return IUP_DEFAULT;
}

static int iFlatListDragEnd_CB(Ihandle *ih, int del)
{
  iupAttribSetInt(ih, "_IUP_LIST_SOURCEPOS", 0);
  (void)del;
  return IUP_DEFAULT;
}

static int iFlatListSetDragDropListAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    /* Register callbacks to enable drag and drop between lists */
    IupSetCallback(ih, "DRAGBEGIN_CB", (Icallback)iFlatListDragBegin_CB);
    IupSetCallback(ih, "DRAGDATASIZE_CB", (Icallback)iFlatListDragDataSize_CB);
    IupSetCallback(ih, "DRAGDATA_CB", (Icallback)iFlatListDragData_CB);
    IupSetCallback(ih, "DRAGEND_CB", (Icallback)iFlatListDragEnd_CB);
    IupSetCallback(ih, "DROPDATA_CB", (Icallback)iFlatListDropData_CB);
  }
  else
  {
    /* Unregister callbacks */
    IupSetCallback(ih, "DRAGBEGIN_CB", NULL);
    IupSetCallback(ih, "DRAGDATASIZE_CB", NULL);
    IupSetCallback(ih, "DRAGDATA_CB", NULL);
    IupSetCallback(ih, "DRAGEND_CB", NULL);
    IupSetCallback(ih, "DROPDATA_CB", NULL);
  }

  return 1;
}

static int iFlatListSetIconSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->icon_spacing);
  if (ih->handle)
    IupUpdate(ih);
  return 0;
}

static char* iFlatListGetIconSpacingAttrib(Ihandle *ih)
{
  return iupStrReturnInt(ih->data->icon_spacing);
}

static char* iFlatListGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(iupArrayCount(ih->data->items_array));
}

static int iFlatListSetValueAttrib(Ihandle* ih, const char* value)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i;

  if (!value)
  {
    /* un-select all */
    for (i = 0; i < count; i++)
      items[i].selected = 0;
  }
  else
  {
    if (ih->data->is_multiple)
    {
      int len = (int)strlen(value);
      if (len != count)
        return 1;

      for (i = 0; i < count; i++)
      {
        if (value[i] == '+')
        {
          items[i].selected = 1;
          iupAttribSetInt(ih, "_IUPFLATLIST_LASTSELECTED", i);
        }
        else if (value[i] == '-')
          items[i].selected = 0;
        /* else does nothing, ignore item */
      }
    }
    else
    {
      int pos;
      if (iupStrToInt(value, &pos) == 1 && pos > 0 && pos <= count)
      {
        for (i = 0; i < count; i++)
        {
          if (!items[i].selected)
            continue;
          items[i].selected = 0;
          break;
        }

        items[pos - 1].selected = 1;
        iupAttribSetInt(ih, "_IUPFLATLIST_LASTSELECTED", pos);
      }
    }
  }

  if (ih->handle)
    IupUpdate(ih);

  return 0;
}

static char* iFlatListGetValueStringAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiple)
  {
    int i = IupGetInt(ih, "VALUE");
    return IupGetAttributeId(ih, "", i);
  }
  return NULL;
}

static int iFlatListSetValueStringAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiple)
  {
    int i;
    int count = iupArrayCount(ih->data->items_array);

    for (i = 1; i <= count; i++)
    {
      char* item = IupGetAttributeId(ih, "", i);
      if (iupStrEqual(value, item))
      {
        IupSetInt(ih, "VALUE", i);
        return 0;
      }
    }
  }

  return 0;
}

static char* iFlatListGetValueAttrib(Ihandle* ih)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i;
  char *retval = NULL;

  if (ih->data->is_multiple)
  {
    char *val = (char *)malloc((count + 1)*sizeof(char));
    for (i = 0; i < count; i++)
      val[i] = (items[i].selected) ? '+' : '-';
    val[i] = '\0';
    retval = iupStrReturnStr(val);
    free(val);
    return retval;
  }
  else
  {
    for (i = 0; i < count; i++)
    {
      if (items[i].selected == 0)
        continue;
      retval = iupStrReturnInt(i + 1);
      break;
    }
  }

  return retval;
}

static int iFlatListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);
  if (ih->handle)
    IupUpdate(ih);
  return 0;
}

static char* iFlatListGetItemFGColorAttrib(Ihandle* ih, int pos)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  if (pos < 1 || pos > count)
    return 0;

  return items[pos - 1].fg_color;
}

static int iFlatListSetItemFGColorAttrib(Ihandle* ih, int pos, const char* value)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;

  if (pos < 1 || pos > count)
    return 0;

  if (items[i].fg_color)
    free(items[i].fg_color);
  items[i].fg_color = iupStrDup(value);

  if (ih->handle)
    IupUpdate(ih);

  return 0;
}

static char* iFlatListGetItemBGColorAttrib(Ihandle* ih, int pos)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);

  if (pos < 1 || pos > count)
    return 0;

  return items[pos - 1].bg_color;
}

static int iFlatListSetItemBGColorAttrib(Ihandle* ih, int pos, const char* value)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;

  if (pos < 1 || pos > count)
    return 0;

  if (items[i].bg_color)
    free(items[i].bg_color);
  items[i].bg_color = iupStrDup(value);

  if (ih->handle)
    IupUpdate(ih);

  return 0;
}

static char* iFlatListGetItemTipAttrib(Ihandle* ih, int pos)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);

  if (pos < 1 || pos > count)
    return 0;

  return items[pos - 1].tip;
}

static int iFlatListSetItemTipAttrib(Ihandle* ih, int pos, const char* value)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;

  if (pos < 1 || pos > count)
    return 0;

  if (items[i].tip)
    free(items[i].tip);
  items[i].tip = iupStrDup(value);

  if (ih->handle)
    IupUpdate(ih);

  return 0;
}

static char* iFlatListGetItemFontAttrib(Ihandle* ih, int pos)
{
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  if (pos < 1 || pos > count)
    return 0;

  return items[pos - 1].font;
}

static int iFlatListSetItemFontAttrib(Ihandle* ih, int pos, const char* value)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;

  if (pos < 1 || pos > count)
    return 0;

  if (items[i].font)
    free(items[i].font);
  items[i].font = iupStrDup(value);

  if (ih->handle)
    IupUpdate(ih);

  return 0;
}

static int iFlatListSetItemFontStyleAttrib(Ihandle* ih, int pos, const char* value)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  if (pos < 1 || pos > count)
    return 0;

  font = items[i].font;
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId(ih, "ITEMFONT", pos, "%s, %s %d", typeface, value, size);

  return 0;
}

static char* iFlatListGetItemFontStyleAttrib(Ihandle* ih, int pos)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (pos < 1 || pos > count)
    return 0;

  font = items[i].font;
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnStrf("%s%s%s%s", is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "");
}

static int iFlatListSetItemFontSizeAttrib(Ihandle* ih, int pos, const char* value)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  if (pos < 1 || pos > count)
    return 0;

  font = items[i].font;
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId(ih, "ITEMFONT", pos, "%s, %s %d", typeface, is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "", value);

  return 0;
}

static char* iFlatListGetItemFontSizeAttrib(Ihandle* ih, int pos)
{
  iFlatListItem *items = (iFlatListItem *)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int i = pos - 1;
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (pos < 1 || pos > count)
    return 0;

  font = items[i].font;
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnInt(size);
}

static int iFlatListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
    IupUpdate(ih);
  return 0;
}

static char* iFlatListGetPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

static int iFlatListSetMultipleAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->is_multiple = 1;
  else
    ih->data->is_multiple = 0;

  return 0;
}

static char* iFlatListGetMultipleAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->is_multiple);
}

static int iFlatListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  if (iupStrToInt(value, &pos))
  {
    int count = iupArrayCount(ih->data->items_array);
    int posy;

    if (pos < 1 || pos > count)
      return 0;

    posy = (pos - 1) * (ih->data->line_height + ih->data->spacing);
    IupSetInt(ih, "POSY", posy);

    IupUpdate(ih);
  }
  return 0;
}

static int iFlatListWheel_CB(Ihandle* ih, float delta)
{
  iupFlatScrollBarWheelUpdate(ih, delta);
  return IUP_DEFAULT;
}

static int iFlatListSetFlatScrollbarAttrib(Ihandle* ih, const char* value)
{
  /* can only be set before map */
  if (ih->handle)
    return IUP_DEFAULT;

  if (value && !iupStrEqualNoCase(value, "NO"))
  {
    if (iupFlatScrollBarCreate(ih))
    {
      IupSetAttribute(ih, "SCROLLBAR", "NO");
      IupSetCallback(ih, "WHEEL_CB", (Icallback)iFlatListWheel_CB);
    }
    return 1;
  }
  else
    return 0;
}

static int iFlatListSetBorderWidthAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->border_width);
  if (ih->handle)
    iupdrvRedrawNow(ih);
  return 0;
}

static char* iFlatListGetBorderWidthAttrib(Ihandle *ih)
{
  return iupStrReturnInt(ih->data->border_width);
}

static int iFlatListSetAttribPostRedraw(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
}


/*****************************************************************************************/


static void iFlatListComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int num_lines;
  int fit2backimage = iupAttribGetBoolean(ih, "FITTOBACKIMAGE");
  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  char* back_image = iupAttribGet(ih, "BACKIMAGE");
  iFlatListItem* items = (iFlatListItem*)iupArrayGetData(ih->data->items_array);
  int count = iupArrayCount(ih->data->items_array);
  int sb, max_h, max_w;

  (void)children_expand; /* unset if not a container */

  if (fit2backimage && back_image)
  {
    iupImageGetInfo(back_image, w, h, NULL);
    *w += 2 * ih->data->border_width;
    *h += 2 * ih->data->border_width;

    if (iupAttribGetBoolean(ih, "BORDER")) /* native border around scrollbars */
    {
      *w += 2;
      *h += 2;
    }
    return;
  }

  iFlatListCalcItemMaxSize(ih, items, count, &max_w, &max_h);

  if (visiblecolumns)
  {
    *w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
    *w = (visiblecolumns*(*w)) / 10;
  }
  else
    *w = max_w;

  if (visiblelines)
    num_lines = visiblelines;
  else
    num_lines = count;

  if (max_h == 0)
    iupdrvFontGetCharSize(ih, NULL, &max_h);

  *h = max_h * num_lines;
  *h += (num_lines - 1) * ih->data->spacing;

  sb = iFlatListGetScrollbar(ih);
  if (sb)
  {
    int sb_size = iFlatListGetScrollbarSize(ih);

    if (sb & IUP_SB_VERT && visiblelines && visiblelines < count)
      *w += sb_size;  /* affects horizontal size (width) */

    if (sb & IUP_SB_HORIZ && visiblecolumns && visiblecolumns < max_w)
      *h += sb_size;  /* affects vertical size (height) */
  }

  *w += 2 * ih->data->border_width;
  *h += 2 * ih->data->border_width;

  if (iupAttribGetBoolean(ih, "BORDER")) /* native border around scrollbars */
  {
    *w += 2;
    *h += 2;
  }
}

static void iFlatListSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  if (iupFlatScrollBarGet(ih) != IUP_SB_NONE)
    iupFlatScrollBarSetChildrenCurrentSize(ih, shrink);
}

static void iFlatListSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (iupFlatScrollBarGet(ih) != IUP_SB_NONE)
    iupFlatScrollBarSetChildrenPosition(ih);

  (void)x;
  (void)y;
}

static void iFlatListDestroyMethod(Ihandle* ih)
{
  int i, count = iupArrayCount(ih->data->items_array);
  iFlatListItem* items = iupArrayGetData(ih->data->items_array);

  iupFlatScrollBarRelease(ih);

  for (i = 0; i < count; i++)
  {
    if (items[i].title)
      free(items[i].title);

    if (items[i].image)
      free(items[i].image);

    if (items[i].fg_color)
      free(items[i].fg_color);

    if (items[i].bg_color)
      free(items[i].bg_color);

    if (items[i].tip)
      free(items[i].tip);

    if (items[i].font)
      free(items[i].font);
  }

  iupArrayDestroy(ih->data->items_array);
}

static int iFlatListCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  /* change the IupCanvas default values */
  ih->expand = IUP_EXPAND_NONE;

  /* free the data allocated by IupCanvas, and reallocate */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* non zero default values */
  ih->data->img_position = IUP_IMGPOS_LEFT;
  ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
  ih->data->vert_alignment = IUP_ALIGN_ACENTER;
  ih->data->horiz_padding = 2;
  ih->data->vert_padding = 2;
  ih->data->icon_spacing = 2;

  ih->data->items_array = iupArrayCreate(10, sizeof(iFlatListItem));

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)iFlatListConvertXYToPos);

  /* internal callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iFlatListRedraw_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iFlatListButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)iFlatListMotion_CB);
  IupSetCallback(ih, "LEAVEWINDOW_CB", (Icallback)iFlatListLeaveWindow_CB);
  IupSetCallback(ih, "RESIZE_CB", (Icallback)iFlatListResize_CB);
  IupSetCallback(ih, "FOCUS_CB", (Icallback)iFlatListFocus_CB);
  IupSetCallback(ih, "SCROLL_CB", (Icallback)iFlatListScroll_CB);
  IupSetCallback(ih, "K_UP", (Icallback)iFlatListKUp_CB);
  IupSetCallback(ih, "K_DOWN", (Icallback)iFlatListKDown_CB);
  IupSetCallback(ih, "K_sUP", (Icallback)iFlatListKUp_CB);
  IupSetCallback(ih, "K_sDOWN", (Icallback)iFlatListKDown_CB);
  IupSetCallback(ih, "K_cUP", (Icallback)iFlatListKUp_CB);
  IupSetCallback(ih, "K_cDOWN", (Icallback)iFlatListKDown_CB);
  IupSetCallback(ih, "K_ANY", (Icallback)iFlatListKAny_CB);
  IupSetCallback(ih, "K_PGUP", (Icallback)iFlatListKPgUp_CB);
  IupSetCallback(ih, "K_PGDN", (Icallback)iFlatListKPgDn_CB);

  return IUP_NOERROR;
}


/******************************************************************************/


IUP_API Ihandle* IupFlatList(void)
{
  return IupCreate("flatlist");
}

Iclass* iupFlatListNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "flatlist";
  ic->cons = "FlatList";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  /* Class functions */
  ic->New = iupFlatListNewClass;
  ic->Create = iFlatListCreateMethod;
  ic->Destroy = iFlatListDestroyMethod;
  ic->ComputeNaturalSize = iFlatListComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iFlatListSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iFlatListSetChildrenPositionMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "FLAT_ACTION", "sii");
  iupClassRegisterCallback(ic, "MULTISELECT_CB", "s");
  iupClassRegisterCallback(ic, "DBLCLICK_CB", "is");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupClassRegisterCallback(ic, "DRAGDROP_CB", "iiii");
  iupClassRegisterCallback(ic, "FLAT_BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "FLAT_MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "FLAT_FOCUS_CB", "i");
  iupClassRegisterCallback(ic, "FLAT_LEAVEWINDOW_CB", "");

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupFlatSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIP", NULL, iupFlatItemSetTipAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IDVALUE", iFlatListGetIdValueAttrib, iFlatListSetIdValueAttrib, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MULTIPLE", iFlatListGetMultipleAttrib, iFlatListSetMultipleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iFlatListGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iFlatListGetValueAttrib, iFlatListSetValueAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUESTRING", iFlatListGetValueStringAttrib, iFlatListSetValueStringAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BORDERCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, IUP_FLAT_BORDERCOLOR, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "BORDERWIDTH", iFlatListGetBorderWidthAttrib, iFlatListSetBorderWidthAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);  /* inheritable */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iFlatListSetAttribPostRedraw, IUP_FLAT_FORECOLOR, NULL, IUPAF_NOT_MAPPED);  /* force the new default value */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iFlatListSetAttribPostRedraw, IUP_FLAT_BACKCOLOR, NULL, IUPAF_NOT_MAPPED);  /* force the new default value */
  iupClassRegisterAttribute(ic, "HLCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTHLCOLOR", IUPAF_NO_INHERIT);  /* selection box, not highlight */
  iupClassRegisterAttribute(ic, "HLCOLORALPHA", NULL, NULL, IUPAF_SAMEASSYSTEM, "128", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PSCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);  /* selection, not pressed */
  iupClassRegisterAttribute(ic, "TEXTPSCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);  /* selection, not pressed */
  iupClassRegisterAttributeId(ic, "ITEMFGCOLOR", iFlatListGetItemFGColorAttrib, iFlatListSetItemFGColorAttrib, IUPAF_NO_INHERIT | IUPAF_NOT_MAPPED);
  iupClassRegisterAttributeId(ic, "ITEMBGCOLOR", iFlatListGetItemBGColorAttrib, iFlatListSetItemBGColorAttrib, IUPAF_NO_INHERIT | IUPAF_NOT_MAPPED);
  iupClassRegisterAttributeId(ic, "ITEMTIP", iFlatListGetItemTipAttrib, iFlatListSetItemTipAttrib, IUPAF_NO_INHERIT | IUPAF_NOT_MAPPED);
  iupClassRegisterAttributeId(ic, "ITEMFONT", iFlatListGetItemFontAttrib, iFlatListSetItemFontAttrib, IUPAF_NO_INHERIT | IUPAF_NOT_MAPPED);
  iupClassRegisterAttributeId(ic, "ITEMFONTSTYLE", iFlatListGetItemFontStyleAttrib, iFlatListSetItemFontStyleAttrib, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMFONTSIZE", iFlatListGetItemFontSizeAttrib, iFlatListSetItemFontSizeAttrib, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iFlatListGetSpacingAttrib, iFlatListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CSPACING", iupBaseGetCSpacingAttrib, iupBaseSetCSpacingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iFlatListGetPaddingAttrib, iFlatListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "2x2", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CPADDING", iupBaseGetCPaddingAttrib, iupBaseSetCPaddingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "HASFOCUS", iFlatListGetHasFocusAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", iFlatListGetAlignmentAttrib, iFlatListSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSFEEDBACK", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "INSERTITEM", NULL, iFlatListSetInsertItemAttrib, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPENDITEM", NULL, iFlatListSetAppendItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEITEM", NULL, iFlatListSetRemoveItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", iFlatListGetImageAttribId, iFlatListSetImageAttribId, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", iFlatListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", iFlatListGetImagePositionAttrib, iFlatListSetImagePositionAttrib, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICONSPACING", iFlatListGetIconSpacingAttrib, iFlatListSetIconSpacingAttrib, IUPAF_SAMEASSYSTEM, "2", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTALIGNMENT", NULL, NULL, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTWRAP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTELLIPSIS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITTOBACKIMAGE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWDRAGDROP", iFlatListGetShowDragDropAttrib, iFlatListSetShowDragDropAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROPLIST", NULL, iFlatListSetDragDropListAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, NULL, "5", NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TOPITEM", NULL, iFlatListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribDef(ic, "SCROLLBAR", "YES", NULL);  /* change the default to Yes */
  iupClassRegisterAttribute(ic, "YAUTOHIDE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* will be always Yes */
  iupClassRegisterAttribute(ic, "XAUTOHIDE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* will be always Yes */

  /* Flat Scrollbar */
  iupFlatScrollBarRegister(ic);

  iupClassRegisterAttribute(ic, "FLATSCROLLBAR", NULL, iFlatListSetFlatScrollbarAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  return ic;
}

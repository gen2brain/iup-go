/** \file
 * \brief iupflatscrollbar utilities
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_drv.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_childtree.h"
#include "iup_flatscrollbar.h"


#define SB_NONE -1

static Ihandle* iFlatScrollBarGetVertical(Ihandle *ih)
{
  return ih->firstchild;  /* sb_vert */
}

static Ihandle* iFlatScrollBarGetHorizontal(Ihandle *ih)
{
  return ih->firstchild->brother;  /* sb_horiz */
}

static void iFlatScrollBarRedrawVertical(Ihandle* ih)
{
  Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
  if (sb_vert->handle)
  {
    int show_transparent = iupAttribGetBoolean(ih, "SHOWTRANSPARENT");
    if (show_transparent)
    {
      iupFlatScrollBarSetChildrenCurrentSize(ih, 0);
      iupFlatScrollBarSetChildrenPosition(ih);
      iupLayoutUpdate(sb_vert);
    }

    iupdrvRedrawNow(sb_vert);
  }
}

static void iFlatScrollBarRedrawHorizontal(Ihandle* ih)
{
  Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);
  if (sb_horiz->handle)
  {
    int show_transparent = iupAttribGetBoolean(ih, "SHOWTRANSPARENT");
    if (show_transparent)
    {
      iupFlatScrollBarSetChildrenCurrentSize(ih, 0);
      iupFlatScrollBarSetChildrenPosition(ih);
      iupLayoutUpdate(sb_horiz);
    }

    iupdrvRedrawNow(sb_horiz);
  }
}

static void iFlatScrollBarNormalizePos(int *pos, int max, int d)
{
  if (*pos > max - d) *pos = max - d;
  if (*pos < 0) *pos = 0;
}

static int iFlatScrollBarGetLineY(Ihandle* ih, int dy)
{
  int liney = dy / 10;
  char* liney_str = iupAttribGet(ih, "LINEY");
  if (liney_str) iupStrToInt(liney_str, &liney);
  return liney;
}

static int iFlatScrollBarGetLineX(Ihandle* ih, int dx)
{
  int linex = dx / 10;
  char* linex_str = iupAttribGet(ih, "LINEX");
  if (linex_str) iupStrToInt(linex_str, &linex);
  return linex;
}

static void iFlatScrollBarNotify(Ihandle *ih, int handler)
{
  if (handler == SB_NONE)
  {
    IFn cb = IupGetCallback(ih, "FLATSCROLL_CB");  /* Used only in IupFlatScrollBox */
    if (cb) cb(ih);
  }
  else
  {
    IFniff cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (cb)
    {
      int sb_size = iupAttribGetInt(ih, "SCROLLBARSIZE");
      int posx = iupAttribGetInt(ih, "POSX");
      int posy = iupAttribGetInt(ih, "POSY");

      if (iupAttribGetInt(ih, "DX") > iupAttribGetInt(ih, "XMAX") - sb_size)
        posx = 0;
      if (iupAttribGetInt(ih, "DY") > iupAttribGetInt(ih, "YMAX") - sb_size)
        posy = 0;

      cb(ih, handler, (float)posx, (float)posy);
    }
  }
}

/* used only in IupFlatScrollBox */
IUP_SDK_API void iupFlatScrollBarSetPos(Ihandle *ih, int posx, int posy)
{
  iFlatScrollBarNormalizePos(&posx, iupAttribGetInt(ih, "XMAX"), iupAttribGetInt(ih, "DX"));
  iFlatScrollBarNormalizePos(&posy, iupAttribGetInt(ih, "YMAX"), iupAttribGetInt(ih, "DY"));

  iupAttribSetInt(ih, "POSX", posx);
  iupAttribSetInt(ih, "POSY", posy);

  iFlatScrollBarRedrawVertical(ih);
  iFlatScrollBarRedrawHorizontal(ih);

  iFlatScrollBarNotify(ih, SB_NONE);
}

static int iFlatScrollBarCalcHandler(int size, int arrow_size, int max, int d, int sb_size, int pos, int* pos1, int* pos2)
{
  int pos_p, d_p;
  int range_p = size - 1 - 2 * arrow_size;

  if (max == 0 || max <= d)
    return 0;

  d_p = (d * range_p) / max;
  if (d_p < sb_size) d_p = sb_size;

  pos_p = (pos * (range_p - d_p)) / (max - d);
  pos_p += arrow_size;

  *pos1 = pos_p;
  *pos2 = pos_p + d_p;

  return 1;
}

static int iFlatScrollBarMoveHandler(int size, int arrow_size, int max, int d, int sb_size, int pos, int diff)
{
  int pos_p;
  int range_p = size - 1 - 2 * arrow_size;

  int d_p = (d * range_p) / max;
  if (d_p < sb_size) d_p = sb_size;

  pos_p = (pos * (range_p - d_p)) / (max - d);
  /* pos_p += arrow_size; */

  pos_p += diff;

  /* pos_p -= arrow_size; */
  pos = (pos_p * (max - d)) / (range_p - d_p);
  return pos;
}


/*************************************************************************/


static void iFlatScrollBarDrawVertical(Ihandle* sb_vert, IdrawCanvas* dc, int active, const char* fgcolor, const char* bgcolor, int pressed,
                                       int highlight, int ymax, int dy, int sb_size, int has_horiz_scroll)
{
  int height = sb_vert->currentheight;
  int pos1, pos2;
  int posy = iupAttribGetInt(sb_vert->parent, "POSY");
  int show_arrows = iupAttribGetInt(sb_vert->parent, "SHOWARROWS");
  int show_transparent = iupAttribGetBoolean(sb_vert->parent, "SHOWTRANSPARENT");
  int arrow_size = sb_size;

  const char *fgcolor_inc = fgcolor,
    *fgcolor_dec = fgcolor,
    *fgcolor_drag = fgcolor;

  if (!show_arrows || show_transparent)
    arrow_size = 0;

  if (pressed != SB_NONE)
  {
    char* presscolor = iupAttribGetStr(sb_vert->parent, "SB_PRESSCOLOR");
    if (presscolor)
    {
      if (pressed == IUP_SBDN)
        fgcolor_inc = presscolor;
      else if (pressed == IUP_SBUP)
        fgcolor_dec = presscolor;
      else if (pressed == IUP_SBDRAGV)
        fgcolor_drag = presscolor;
    }
  }
  else if (highlight != SB_NONE)
  {
    char* hlcolor = iupAttribGetStr(sb_vert->parent, "SB_HIGHCOLOR");
    if (hlcolor)
    {
      if (highlight == IUP_SBDN)
        fgcolor_inc = hlcolor;
      else if (highlight == IUP_SBUP)
        fgcolor_dec = hlcolor;
      else if (highlight == IUP_SBDRAGV)
        fgcolor_drag = hlcolor;
    }
  }

  if (show_transparent)
  {
    iupFlatDrawBox(dc, 0, sb_size - 1, 0, height-1, fgcolor_drag, bgcolor, active);
    return;
  }

  if (has_horiz_scroll)
    height -= sb_size;

  /* draw arrows */
  if (show_arrows)
  {
    int arrow_images = iupAttribGetInt(sb_vert->parent, "ARROWIMAGES");
    if (arrow_images)
    {
      int make_inactive;
      const char* image;

      image = iupFlatGetImageName(sb_vert->parent, "SB_IMAGETOP", NULL, pressed != SB_NONE, highlight != SB_NONE, active, &make_inactive);
      iupdrvDrawImage(dc, image, make_inactive, bgcolor, 0, 0, -1, -1);

      image = iupFlatGetImageName(sb_vert->parent, "SB_IMAGEBOTTOM", NULL, pressed != SB_NONE, highlight != SB_NONE, active, &make_inactive);
      iupdrvDrawImage(dc, image, make_inactive, bgcolor, height - 1 - arrow_size, 0, -1, -1);
    }
    else
    {
      iupFlatDrawArrow(dc, 2, 2, arrow_size - 5, fgcolor_dec, bgcolor, active, IUPDRAW_ARROW_TOP);
      iupFlatDrawArrow(dc, 2, height - 1 - arrow_size + 3, arrow_size - 5, fgcolor_inc, bgcolor, active, IUPDRAW_ARROW_BOTTOM);
    }
  }

  /* draw handler */
  if (iFlatScrollBarCalcHandler(height, arrow_size, ymax, dy, sb_size, posy, &pos1, &pos2))
    iupFlatDrawBox(dc, 2, sb_size - 1 - 2, pos1, pos2, fgcolor_drag, bgcolor, active);
}

static void iFlatScrollBarDrawHorizontal(Ihandle* sb_horiz, IdrawCanvas* dc, int active, const char* fgcolor, const char* bgcolor, int pressed,
                                         int highlight, int xmax, int dx, int sb_size, int has_vert_scroll)
{
  int width = sb_horiz->currentwidth;
  int pos1, pos2;
  int posx = iupAttribGetInt(sb_horiz->parent, "POSX");
  int show_arrows = iupAttribGetInt(sb_horiz->parent, "SHOWARROWS");
  int show_transparent = iupAttribGetBoolean(sb_horiz->parent, "SHOWTRANSPARENT");
  int arrow_size = sb_size;

  const char *fgcolor_inc = fgcolor,
    *fgcolor_dec = fgcolor,
    *fgcolor_drag = fgcolor;

  if (!show_arrows || show_transparent)
    arrow_size = 0;

  if (pressed != SB_NONE)
  {
    char* presscolor = iupAttribGetStr(sb_horiz->parent, "SB_PRESSCOLOR");
    if (presscolor)
    {
      if (pressed == IUP_SBRIGHT)
        fgcolor_inc = presscolor;
      else if (pressed == IUP_SBLEFT)
        fgcolor_dec = presscolor;
      else if (pressed == IUP_SBDRAGH)
        fgcolor_drag = presscolor;
    }
  }
  else if (highlight != SB_NONE)
  {
    char* hlcolor = iupAttribGetStr(sb_horiz->parent, "SB_HIGHCOLOR");
    if (hlcolor)
    {
      if (highlight == IUP_SBRIGHT)
        fgcolor_inc = hlcolor;
      else if (highlight == IUP_SBLEFT)
        fgcolor_dec = hlcolor;
      else if (highlight == IUP_SBDRAGH)
        fgcolor_drag = hlcolor;
    }
  }

  if (show_transparent)
  {
    iupFlatDrawBox(dc, 0, width-1, 0, sb_size - 1, fgcolor_drag, bgcolor, active);
    return;
  }

  if (has_vert_scroll)
    width -= sb_size;

  /* draw arrows */
  if (show_arrows)
  {
    int arrow_images = iupAttribGetInt(sb_horiz->parent, "ARROWIMAGES");
    if (arrow_images)
    {
      int make_inactive;
      const char* image;
      
      image = iupFlatGetImageName(sb_horiz->parent, "SB_IMAGELEFT", NULL, pressed != SB_NONE, highlight != SB_NONE, active, &make_inactive);
      iupdrvDrawImage(dc, image, make_inactive, bgcolor, 0, 0, -1, -1);

      image = iupFlatGetImageName(sb_horiz->parent, "SB_IMAGERIGHT", NULL, pressed != SB_NONE, highlight != SB_NONE, active, &make_inactive);
      iupdrvDrawImage(dc, image, make_inactive, bgcolor, width - 1 - arrow_size, 0, -1, -1);
    }
    else
    {
      iupFlatDrawArrow(dc, 2, 2, arrow_size - 5, fgcolor_dec, bgcolor, active, IUPDRAW_ARROW_LEFT);
      iupFlatDrawArrow(dc, width - 1 - arrow_size + 3, 2, arrow_size - 5, fgcolor_inc, bgcolor, active, IUPDRAW_ARROW_RIGHT);
    }
  }

  /* draw handler */
  if (iFlatScrollBarCalcHandler(width, arrow_size, xmax, dx, sb_size, posx, &pos1, &pos2))
    iupFlatDrawBox(dc, pos1, pos2, 2, sb_size - 1 - 2, fgcolor_drag, bgcolor, active);
}

static int iFlatScrollBarAction_CB(Ihandle* sb_ih)
{
  int sb_size = iupAttribGetInt(sb_ih->parent, "SCROLLBARSIZE");
  int xmax = iupAttribGetInt(sb_ih->parent, "XMAX");
  int ymax = iupAttribGetInt(sb_ih->parent, "YMAX");
  int dx = iupAttribGetInt(sb_ih->parent, "DX");
  int dy = iupAttribGetInt(sb_ih->parent, "DY");
  char* fgcolor = iupAttribGetStr(sb_ih->parent, "SB_FORECOLOR");
  char* bgcolor = iupAttribGetStr(sb_ih->parent, "SB_BACKCOLOR");
  int highlight = iupAttribGetInt(sb_ih, "_IUP_HIGHLIGHT_HANDLER");
  int pressed = iupAttribGetInt(sb_ih, "_IUP_PRESSED_HANDLER");
  int active = IupGetInt(sb_ih->parent, "ACTIVE");
  int is_vert_scrollbar = 0;
  int has_vert_scroll = 0;
  int has_horiz_scroll = 0;

  IdrawCanvas* dc = iupdrvDrawCreateCanvas(sb_ih);

  if (!bgcolor)
    bgcolor = iupBaseNativeParentGetBgColorAttrib(sb_ih);

  if (iupAttribGet(sb_ih, "SB_VERT"))
    is_vert_scrollbar = 1;

  if (xmax > dx)  /* has horizontal scrollbar */
    has_horiz_scroll = 1;

  if (ymax > dy)  /* has vertical scrollbar */
    has_vert_scroll = 1;

  /* draw background */
  iupFlatDrawBox(dc, 0, sb_ih->currentwidth - 1, 0, sb_ih->currentheight - 1, bgcolor, NULL, 1);

  if (is_vert_scrollbar)
  {
    if (has_vert_scroll)
      iFlatScrollBarDrawVertical(sb_ih, dc, active, fgcolor, bgcolor, pressed, highlight, ymax, dy, sb_size, has_horiz_scroll);
  }
  else
  {
    if (has_horiz_scroll)
      iFlatScrollBarDrawHorizontal(sb_ih, dc, active, fgcolor, bgcolor, pressed, highlight, xmax, dx, sb_size, has_vert_scroll);
  }

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static int iFlatScrollBarGetHandler(Ihandle* sb_ih, int x, int y)
{
  int sb_size = iupAttribGetInt(sb_ih->parent, "SCROLLBARSIZE");
  int xmax = iupAttribGetInt(sb_ih->parent, "XMAX");
  int ymax = iupAttribGetInt(sb_ih->parent, "YMAX");
  int dy = iupAttribGetInt(sb_ih->parent, "DY");
  int dx = iupAttribGetInt(sb_ih->parent, "DX");
  int show_arrows = iupAttribGetInt(sb_ih->parent, "SHOWARROWS");
  int show_transparent = iupAttribGetBoolean(sb_ih->parent, "SHOWTRANSPARENT");
  int arrow_size = sb_size;
  int is_vert_scrollbar = 0;
  int pos1, pos2;

  if (iupAttribGet(sb_ih, "SB_VERT"))
    is_vert_scrollbar = 1;

  if (show_transparent)
  {
    if (is_vert_scrollbar)
      return IUP_SBDRAGV;
    else
      return IUP_SBDRAGH;
  }

  if (!show_arrows)
    arrow_size = 0;

  if (is_vert_scrollbar)
  {
    int posy = iupAttribGetInt(sb_ih->parent, "POSY");

    int height = sb_ih->currentheight;
    if (xmax > dx)  /* has horizontal scrollbar */
      height -= sb_size;

    if (!iFlatScrollBarCalcHandler(height, arrow_size, ymax, dy, sb_size, posy, &pos1, &pos2))
      return SB_NONE;

    if (y < arrow_size)
      return IUP_SBUP;
    else if (y < pos1)
      return IUP_SBPGUP;
    else if (y < pos2)
      return IUP_SBDRAGV;
    else if (y < height - arrow_size)
      return IUP_SBPGDN;
    else if (y < height && show_arrows)
      return IUP_SBDN;
  }
  else
  {
    int posx = iupAttribGetInt(sb_ih->parent, "POSX");

    int width = sb_ih->currentwidth;
    if (ymax > dy)  /* has vertical scrollbar */
      width -= sb_size;

    if (!iFlatScrollBarCalcHandler(width, arrow_size, xmax, dx, sb_size, posx, &pos1, &pos2))
      return SB_NONE;

    if (x < arrow_size)
      return IUP_SBLEFT;
    else if (x < pos1)
      return IUP_SBPGLEFT;
    else if (x < pos2)
      return IUP_SBDRAGH;
    else if (x < width - arrow_size)
      return IUP_SBPGRIGHT;
    else if (x < width && show_arrows)
      return IUP_SBRIGHT;
  }

  return SB_NONE;
}

static void iFlatScrollBarUpdateInteractive(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "SHOWFLOATING"))
  {
    /* restart the timer */
    Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_FLOATTIMER");
    IupSetAttribute(timer, "RUN", "NO");
    IupSetAttribute(timer, "RUN", "YES");
  }
}

static void iFlatScrollBarPressX(Ihandle* sb_horiz, int handler)
{
  int xmax = iupAttribGetInt(sb_horiz->parent, "XMAX");
  int dx = iupAttribGetInt(sb_horiz->parent, "DX");
  int posx = iupAttribGetInt(sb_horiz->parent, "POSX");

  if (handler == IUP_SBRIGHT)
  {
    int linex = iFlatScrollBarGetLineX(sb_horiz->parent, dx);
    posx += linex;
  }
  else if (handler == IUP_SBPGRIGHT)
    posx += dx;
  if (handler == IUP_SBLEFT)
  {
    int linex = iFlatScrollBarGetLineX(sb_horiz->parent, dx);
    posx -= linex;
  }
  else if (handler == IUP_SBPGLEFT)
    posx -= dx;

  iFlatScrollBarNormalizePos(&posx, xmax, dx);

  iupAttribSetInt(sb_horiz->parent, "POSX", posx);
}

static void iFlatScrollBarPressY(Ihandle* sb_vert, int handler)
{
  int ymax = iupAttribGetInt(sb_vert->parent, "YMAX");
  int dy = iupAttribGetInt(sb_vert->parent, "DY");
  int posy = iupAttribGetInt(sb_vert->parent, "POSY");

  if (handler == IUP_SBDN)
  {
    int liney = iFlatScrollBarGetLineY(sb_vert->parent, dy);
    posy += liney;
  }
  else if (handler == IUP_SBPGDN)
    posy += dy;
  if (handler == IUP_SBUP)
  {
    int liney = iFlatScrollBarGetLineY(sb_vert->parent, dy);
    posy -= liney;
  }
  else if (handler == IUP_SBPGUP)
    posy -= dy;

  iFlatScrollBarNormalizePos(&posy, ymax, dy);

  iupAttribSetInt(sb_vert->parent, "POSY", posy);
}

static int iFlatScrollBarMoveX(Ihandle* sb_horiz, int diff, int start_posx)
{
  int xmax = iupAttribGetInt(sb_horiz->parent, "XMAX");
  int ymax = iupAttribGetInt(sb_horiz->parent, "YMAX");
  int dx = iupAttribGetInt(sb_horiz->parent, "DX");
  int dy = iupAttribGetInt(sb_horiz->parent, "DY");
  int sb_size = iupAttribGetInt(sb_horiz->parent, "SCROLLBARSIZE");
  int posx;
  int width = sb_horiz->parent->currentwidth;
  int show_arrows = iupAttribGetInt(sb_horiz->parent, "SHOWARROWS");
  int arrow_size = sb_size;

  if (xmax == 0 || xmax <= dx)
    return 0;

  if (!show_arrows)
    arrow_size = 0;

  if (ymax > dy)  /* has vertical scrollbar */
    width -= sb_size;

  posx = iFlatScrollBarMoveHandler(width, arrow_size, xmax, dx, sb_size, start_posx, diff);

  iFlatScrollBarNormalizePos(&posx, xmax, dx);

  if (posx != start_posx)
  {
    iupAttribSetInt(sb_horiz->parent, "POSX", posx);
    return 1;
  }

  return 0;
}

static int iFlatScrollBarMoveY(Ihandle* sb_vert, int diff, int start_posy)
{
  int xmax = iupAttribGetInt(sb_vert->parent, "XMAX");
  int ymax = iupAttribGetInt(sb_vert->parent, "YMAX");
  int dx = iupAttribGetInt(sb_vert->parent, "DX");
  int dy = iupAttribGetInt(sb_vert->parent, "DY");
  int sb_size = iupAttribGetInt(sb_vert->parent, "SCROLLBARSIZE");
  int posy;
  int height = sb_vert->parent->currentheight;
  int show_arrows = iupAttribGetInt(sb_vert->parent, "SHOWARROWS");
  int arrow_size = sb_size;

  if (ymax == 0 || ymax <= dy)
    return 0;

  if (!show_arrows)
    arrow_size = 0;

  if (xmax > dx)  /* has horizontal scrollbar */
    height -= sb_size;

  posy = iFlatScrollBarMoveHandler(height, arrow_size, ymax, dy, sb_size, start_posy, diff);

  iFlatScrollBarNormalizePos(&posy, ymax, dy);

  if (posy != start_posy)
  {
    iupAttribSetInt(sb_vert->parent, "POSY", posy);
    return 1;
  }

  return 0;
}

static int iFlatScrollBarButton_CB(Ihandle* sb_ih, int button, int press, int x, int y)
{
  iFlatScrollBarUpdateInteractive(sb_ih->parent);

  if (button != IUP_BUTTON1)
  {
    iupAttribSetInt(sb_ih, "_IUP_PRESSED_HANDLER", SB_NONE);
    return IUP_DEFAULT;
  }

  if (press)
  {
    int handler = iFlatScrollBarGetHandler(sb_ih, x, y);
    iupAttribSetInt(sb_ih, "_IUP_PRESSED_HANDLER", handler);

    if (handler == IUP_SBDRAGH || handler == IUP_SBDRAGV)
    {
      int show_transparent = iupAttribGetBoolean(sb_ih->parent, "SHOWTRANSPARENT");
      if (show_transparent)
        iupStrToIntInt(IupGetGlobal("CURSORPOS"), &x, &y, 'x');

      iupAttribSetInt(sb_ih, "_IUP_START_X", x);
      iupAttribSetInt(sb_ih, "_IUP_START_Y", y);

      if (handler == IUP_SBDRAGH)
        iupAttribSetStr(sb_ih, "_IUP_START_POSX", iupAttribGet(sb_ih->parent, "POSX"));
      else
        iupAttribSetStr(sb_ih, "_IUP_START_POSY", iupAttribGet(sb_ih->parent, "POSY"));
    }

    if (handler != SB_NONE)
    {
      iupdrvRedrawNow(sb_ih);
      return IUP_DEFAULT;
    }
  }
  else
  {
    int press_handler = iupAttribGetInt(sb_ih, "_IUP_PRESSED_HANDLER");
    int handler = iFlatScrollBarGetHandler(sb_ih, x, y);

    if (handler != SB_NONE && handler == press_handler)
    {
      if (handler == IUP_SBRIGHT || handler == IUP_SBPGRIGHT ||
          handler == IUP_SBLEFT || handler == IUP_SBPGLEFT)
        iFlatScrollBarPressX(sb_ih, handler);
      else if (handler == IUP_SBDN || handler == IUP_SBPGDN ||
          handler == IUP_SBUP || handler == IUP_SBPGUP)
        iFlatScrollBarPressY(sb_ih, handler);
      else if (handler == IUP_SBDRAGH)
        handler = IUP_SBPOSH;
      else if (handler == IUP_SBDRAGV)
        handler = IUP_SBPOSV;

      iFlatScrollBarNotify(sb_ih->parent, handler);
    }
    iupAttribSetInt(sb_ih, "_IUP_PRESSED_HANDLER", SB_NONE);

    if (handler != SB_NONE)
    {
      iupdrvRedrawNow(sb_ih);
      return IUP_DEFAULT;
    }
  }

  return IUP_DEFAULT;
}

static int iFlatScrollBarMotion_CB(Ihandle *sb_ih, int x, int y, char* status)
{
  int redraw = 0, old_handler;
  int handler = iFlatScrollBarGetHandler(sb_ih, x, y);

  iFlatScrollBarUpdateInteractive(sb_ih->parent);

  /* special highlight processing for scrollbar area */
  old_handler = iupAttribGetInt(sb_ih, "_IUP_HIGHLIGHT_HANDLER");
  if (old_handler != handler)
  {
    redraw = 1;
    iupAttribSetInt(sb_ih, "_IUP_HIGHLIGHT_HANDLER", handler);
  }

  if (iup_isbutton1(status))
  {
    handler = iupAttribGetInt(sb_ih, "_IUP_PRESSED_HANDLER");

    if (handler == IUP_SBDRAGH || handler == IUP_SBDRAGV)
    {
      int show_transparent = iupAttribGetBoolean(sb_ih->parent, "SHOWTRANSPARENT");
      if (show_transparent)
        iupStrToIntInt(IupGetGlobal("CURSORPOS"), &x, &y, 'x');
    }

    if (handler == IUP_SBDRAGH)
    {
      int start_x = iupAttribGetInt(sb_ih, "_IUP_START_X");
      int start_posx = iupAttribGetInt(sb_ih, "_IUP_START_POSX");

      if (x != start_x && iFlatScrollBarMoveX(sb_ih, x - start_x, start_posx))
      {
        iFlatScrollBarNotify(sb_ih->parent, handler);
        redraw = 1;
      }
    }
    else if (handler == IUP_SBDRAGV)
    {
      int start_y = iupAttribGetInt(sb_ih, "_IUP_START_Y");
      int start_posy = iupAttribGetInt(sb_ih, "_IUP_START_POSY");

      if (y != start_y && iFlatScrollBarMoveY(sb_ih, y - start_y, start_posy))
      {
        iFlatScrollBarNotify(sb_ih->parent, handler);
        redraw = 1;
      }
    }
  }

  if (redraw)
    iupdrvRedrawNow(sb_ih);

  return IUP_DEFAULT;
}

static int iFlatScrollBarLeaveWindow_CB(Ihandle* sb_ih)
{
  int handler = iupAttribGetInt(sb_ih, "_IUP_HIGHLIGHT_HANDLER");
  if (handler != SB_NONE)
  {
    iupAttribSetInt(sb_ih, "_IUP_HIGHLIGHT_HANDLER", SB_NONE);
    iupdrvRedrawNow(sb_ih);
  }
  return IUP_DEFAULT;
}

IUP_SDK_API void iupFlatScrollBarWheelUpdate(Ihandle* ih, float delta)
{
  int posy = iupAttribGetInt(ih, "POSY");
  int dy = iupAttribGetInt(ih, "DY");
  int liney = iFlatScrollBarGetLineY(ih, dy);
  int ymax = iupAttribGetInt(ih, "YMAX");

  if (dy >= ymax)
    return;

  if (iupAttribGetBoolean(ih, "WHEELDROPFOCUS"))
  {
    Ihandle* ih_focus = IupGetFocus();
    if (iupObjectCheck(ih_focus))
      iupAttribSetClassObject(ih_focus, "SHOWDROPDOWN", "NO");
  }

  posy -= (int)(delta * liney);
  iFlatScrollBarNormalizePos(&posy, ymax, dy);
  iupAttribSetInt(ih, "POSY", posy);
  iFlatScrollBarRedrawVertical(ih);
  iFlatScrollBarNotify(ih, delta>0 ? IUP_SBUP: IUP_SBDN);

  if (iupAttribGetBoolean(ih, "SHOWFLOATING"))
  {
    if (iupFlatScrollBarGet(ih) & IUP_SB_VERT)
    {
      if (!iupAttribGetBoolean(ih, "YHIDDEN"))
      {
        Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
        IupSetAttribute(sb_vert, "VISIBLE", "Yes");
        IupSetAttribute(sb_vert, "ZORDER", "TOP");
      }
    }
  }
}

static int iFlatScrollBarWheel_CB(Ihandle* sb_ih, float delta)
{
  iupFlatScrollBarWheelUpdate(sb_ih->parent, delta);
  return IUP_DEFAULT;
}

static int iFlatScrollBarFloatTimer_CB(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUP_FLATSCROLLBAR");
  Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
  Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);

  int press_handler = iupAttribGetInt(sb_vert, "_IUP_PRESSED_HANDLER");
  if (press_handler == SB_NONE)
    IupSetAttribute(sb_vert, "VISIBLE", "NO");

  press_handler = iupAttribGetInt(sb_horiz, "_IUP_PRESSED_HANDLER");
  if (press_handler == SB_NONE)
    IupSetAttribute(sb_horiz, "VISIBLE", "NO");

  IupSetAttribute(timer, "RUN", "NO");

  return IUP_DEFAULT;
}


/*****************************************************************************/


static IattribSetFunc iupCanvasSetDXAttrib = NULL;
static IattribSetFunc iupCanvasSetDYAttrib = NULL;
static IattribSetFunc iupCanvasSetPosXAttrib = NULL;
static IattribSetFunc iupCanvasSetPosYAttrib = NULL;
static IattribGetFunc iupCanvasGetPosXAttrib = NULL;
static IattribGetFunc iupCanvasGetPosYAttrib = NULL;

static int iFlatScrollBarSetDXAttrib(Ihandle* ih, const char *value)
{
  if (iupFlatScrollBarGet(ih) & IUP_SB_HORIZ)
  {
    int dx;
    if (iupStrToInt(value, &dx))
    {
      Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);
      int xmax = iupAttribGetInt(ih, "XMAX");

      iupAttribSet(ih, "SB_RESIZE", NULL);

      if (dx >= xmax)
      {
        if (IupGetInt(sb_horiz, "VISIBLE"))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        IupSetAttribute(sb_horiz, "VISIBLE", "NO");
        iupAttribSet(ih, "XHIDDEN", "YES");
        iupAttribSet(ih, "POSX", "0");
      }
      else
      {
        int posx = iupAttribGetInt(ih, "POSX");
        if (posx > xmax - dx)
          iupAttribSetInt(ih, "POSX", xmax - dx);

        if (!iupAttribGetBoolean(ih, "SHOWFLOATING"))
        {
          if (!IupGetInt(sb_horiz, "VISIBLE"))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          IupSetAttribute(sb_horiz, "VISIBLE", "Yes");
          IupSetAttribute(sb_horiz, "ZORDER", "TOP");
        }

        iupAttribSet(ih, "XHIDDEN", "NO");

        iFlatScrollBarRedrawHorizontal(ih);  /* force a redraw if it is already visible */

        iFlatScrollBarNotify(ih, SB_NONE);
      }

      iFlatScrollBarRedrawVertical(ih);  /* force a redraw of the other scrollbar */
    }

    return 1;
  }
  else
    return iupCanvasSetDXAttrib(ih, value);
}

static int iFlatScrollBarSetDYAttrib(Ihandle* ih, const char *value)
{
  if (iupFlatScrollBarGet(ih) & IUP_SB_VERT)
  {
    int dy;
    if (iupStrToInt(value, &dy))
    {
      Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
      int ymax = iupAttribGetInt(ih, "YMAX");

      iupAttribSet(ih, "SB_RESIZE", NULL);

      if (dy >= ymax)
      {
        if (IupGetInt(sb_vert, "VISIBLE"))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        IupSetAttribute(sb_vert, "VISIBLE", "NO");
        iupAttribSet(ih, "YHIDDEN", "YES");
        iupAttribSet(ih, "POSY", "0");
      }
      else
      {
        int posy = iupAttribGetInt(ih, "POSY");
        if (posy > ymax - dy)
          iupAttribSetInt(ih, "POSY", ymax - dy);

        if (!iupAttribGetBoolean(ih, "SHOWFLOATING"))
        {
          if (!IupGetInt(sb_vert, "VISIBLE"))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          IupSetAttribute(sb_vert, "VISIBLE", "Yes");
          IupSetAttribute(sb_vert, "ZORDER", "TOP");
        }

        iupAttribSet(ih, "YHIDDEN", "NO");

        iFlatScrollBarRedrawVertical(ih);  /* force a redraw if it is already visible */

        iFlatScrollBarNotify(ih, SB_NONE);
      }

      iFlatScrollBarRedrawHorizontal(ih);  /* force a redraw of the other scrollbar */
    }

    return 1;
  }
  else
    return iupCanvasSetDYAttrib(ih, value);
}

static int iFlatScrollBarSetPosXAttrib(Ihandle* ih, const char *value)
{
  if (iupFlatScrollBarGet(ih) & IUP_SB_HORIZ)
  {
    int xmax, dx;
    int posx;

    if (!iupStrToInt(value, &posx))
      return 0;

    xmax = iupAttribGetInt(ih, "XMAX");
    dx = iupAttribGetInt(ih, "DX");

    iFlatScrollBarNormalizePos(&posx, xmax, dx);

    iupAttribSetInt(ih, "POSX", posx);

    iFlatScrollBarRedrawHorizontal(ih);

    iFlatScrollBarNotify(ih, SB_NONE);

    return 0;
  }
  else
    return iupCanvasSetPosXAttrib(ih, value);
}

static int iFlatScrollBarSetPosYAttrib(Ihandle* ih, const char *value)
{
  if (iupFlatScrollBarGet(ih) & IUP_SB_VERT)
  {
    int ymax, dy;
    int posy;

    if (!iupStrToInt(value, &posy))
      return 0;

    ymax = iupAttribGetInt(ih, "YMAX");
    dy = iupAttribGetInt(ih, "DY");

    iFlatScrollBarNormalizePos(&posy, ymax, dy);

    iupAttribSetInt(ih, "POSY", posy);

    iFlatScrollBarRedrawVertical(ih);

    iFlatScrollBarNotify(ih, SB_NONE);

    return 0;
  }
  else
    return iupCanvasSetPosYAttrib(ih, value);
}

static char* iFlatScrollBarGetPosYAttrib(Ihandle* ih)
{
  if (iupFlatScrollBarGet(ih) & IUP_SB_VERT)
    return NULL;
  else
    return iupCanvasGetPosYAttrib(ih);
}

static char* iFlatScrollBarGetPosXAttrib(Ihandle* ih)
{
  if (iupFlatScrollBarGet(ih) & IUP_SB_HORIZ)
    return NULL;
  else
    return iupCanvasGetPosXAttrib(ih);
}

static int iFlatScrollBarSetShowFloatingAttrib(Ihandle* ih, const char *value)
{
  if (iupStrBoolean(value))
  {
    Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_FLOATTIMER");
    Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
    Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);

    IupSetAttribute(sb_vert, "VISIBLE", "NO");
    IupSetAttribute(sb_horiz, "VISIBLE", "NO");

    if (!timer)
    {
      timer = IupTimer();
      IupSetCallback(timer, "ACTION_CB", iFlatScrollBarFloatTimer_CB);
      iupAttribSet(timer, "_IUP_FLATSCROLLBAR", (char*)ih);
      iupAttribSet(ih, "_IUP_FLOATTIMER", (char*)timer);
    }

    IupSetStrAttribute(timer, "TIME", iupAttribGetStr(ih, "FLOATINGDELAY"));
  }
  else
  {
    int sb = iupFlatScrollBarGet(ih);
    Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
    Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);

    if (sb & IUP_SB_VERT)
      IupSetAttribute(sb_vert, "VISIBLE", "Yes");

    if (sb & IUP_SB_HORIZ)
      IupSetAttribute(sb_horiz, "VISIBLE", "Yes");
  }

  return 1;
}

static int iFlatScrollBarSetShowTransparentAttrib(Ihandle* ih, const char *value)
{
  if (iupStrBoolean(value))
  {
    IupSetAttribute(ih, "SHOWFLOATING", "Yes");
    IupSetAttribute(ih, "SHOWARROWS", "No");
  }
  else
  {
    /* reset to default */
    IupSetAttribute(ih, "SHOWFLOATING", NULL);
    IupSetAttribute(ih, "SHOWARROWS", NULL);
  }

  return 1;
}


/*******************************************************************************************************/


IUP_SDK_API void iupFlatScrollBarSetChildrenCurrentSize(Ihandle* ih, int shrink)
{
  int show_transparent = iupAttribGetBoolean(ih, "SHOWTRANSPARENT");
  Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
  Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);
  int sb_size = iupAttribGetInt(ih, "SCROLLBARSIZE");

  /* sb_horiz->currentheight == sb_size always */
  /* sb_vert->currentwidth == sb_size always   */

  if (show_transparent)
  {
    int pos1, pos2;
    int xmax = iupAttribGetInt(ih, "XMAX");
    int ymax = iupAttribGetInt(ih, "YMAX");
    int dx = iupAttribGetInt(ih, "DX");
    int dy = iupAttribGetInt(ih, "DY");
    int posx = iupAttribGetInt(ih, "POSX");
    int posy = iupAttribGetInt(ih, "POSY");

    if (!iupAttribGetBoolean(ih, "YHIDDEN"))
    {
      if (iFlatScrollBarCalcHandler(ih->currentheight, 0, ymax, dy, sb_size, posy, &pos1, &pos2))
        iupBaseSetCurrentSize(sb_vert, sb_size, pos2 - pos1 + 1, shrink);  /* sb_vert->currentheight == dy in pixels */
    }

    if (!iupAttribGetBoolean(ih, "XHIDDEN"))
    {
      if (iFlatScrollBarCalcHandler(ih->currentwidth, 0, xmax, dx, sb_size, posx, &pos1, &pos2))
        iupBaseSetCurrentSize(sb_horiz, pos2 - pos1 + 1, sb_size, shrink);  /* sb_horiz->currentwidth == dx in pixels */
    }
  }
  else
  {
    iupBaseSetCurrentSize(sb_vert, sb_size, ih->currentheight, shrink);  /* sb_vert->currentheight == ih->currentheight */
    iupBaseSetCurrentSize(sb_horiz, ih->currentwidth, sb_size, shrink);  /* sb_horiz->currentwidth == ih->currentwidth */
  }
}

IUP_SDK_API void iupFlatScrollBarSetChildrenPosition(Ihandle* ih)
{
  int show_transparent = iupAttribGetBoolean(ih, "SHOWTRANSPARENT");
  Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
  Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);
  int sb_size = iupAttribGetInt(ih, "SCROLLBARSIZE");

  if (show_transparent)
  {
    int pos1, pos2;
    int xmax = iupAttribGetInt(ih, "XMAX");
    int ymax = iupAttribGetInt(ih, "YMAX");
    int dx = iupAttribGetInt(ih, "DX");
    int dy = iupAttribGetInt(ih, "DY");
    int posx = iupAttribGetInt(ih, "POSX");
    int posy = iupAttribGetInt(ih, "POSY");

    if (!iupAttribGetBoolean(ih, "YHIDDEN"))
    {
      if (iFlatScrollBarCalcHandler(ih->currentheight, 0, ymax, dy, sb_size, posy, &pos1, &pos2))
        iupBaseSetPosition(sb_vert, ih->currentwidth - sb_size, pos1);
    }

    if (!iupAttribGetBoolean(ih, "XHIDDEN"))
    {
      if (iFlatScrollBarCalcHandler(ih->currentwidth, 0, xmax, dx, sb_size, posx, &pos1, &pos2))
        iupBaseSetPosition(sb_horiz, pos1, ih->currentheight - sb_size);
    }
  }
  else
  {
    iupBaseSetPosition(sb_vert, ih->currentwidth - sb_size, 0);
    iupBaseSetPosition(sb_horiz, 0, ih->currentheight - sb_size);
  }

  IupSetAttribute(sb_vert, "ZORDER", "TOP");
  IupSetAttribute(sb_horiz, "ZORDER", "TOP");
}

IUP_SDK_API void iupFlatScrollBarMotionUpdate(Ihandle* ih, int x, int y)
{
  if (iupAttribGetBoolean(ih, "SHOWFLOATING"))
  {
    int sb_size = iupAttribGetInt(ih, "SCROLLBARSIZE");
    int sb = iupFlatScrollBarGet(ih);

    if (sb & IUP_SB_VERT)
    {
      if (x > ih->currentwidth - sb_size)
      {
        if (!iupAttribGetBoolean(ih, "YHIDDEN"))
        {
          Ihandle* sb_vert = iFlatScrollBarGetVertical(ih);
          IupSetAttribute(sb_vert, "VISIBLE", "Yes");
          IupSetAttribute(sb_vert, "ZORDER", "TOP");
        }
      }
    }

    if (sb & IUP_SB_HORIZ)
    {
      if (y > ih->currentheight - sb_size)
      {
        if (!iupAttribGetBoolean(ih, "XHIDDEN"))
        {
          Ihandle* sb_horiz = iFlatScrollBarGetHorizontal(ih);
          IupSetAttribute(sb_horiz, "VISIBLE", "Yes");
          IupSetAttribute(sb_horiz, "ZORDER", "TOP");
        }
      }
    }
  }
}


/******************************************************************************/


IUP_SDK_API int iupFlatScrollBarGet(Ihandle* ih)
{
  int sb = IUP_SB_NONE;  /* NO scrollbar by default */
  char* value = iupAttribGetStr(ih, "FLATSCROLLBAR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "YES"))
      sb = IUP_SB_HORIZ | IUP_SB_VERT;
    else if (iupStrEqualNoCase(value, "HORIZONTAL"))
      sb = IUP_SB_HORIZ;
    else if (iupStrEqualNoCase(value, "VERTICAL"))
      sb = IUP_SB_VERT;
  }
  return sb;
}

static void iChildTreeInsertFirst(Ihandle* ih, Ihandle* child)
{
  if (ih->firstchild == NULL)
  {
    child->parent = ih;
  }
  else
  {
    child->parent = ih;
    child->brother = ih->firstchild;
  }

  ih->firstchild = child;
}

IUP_SDK_API int iupFlatScrollBarCreate(Ihandle* ih)
{
  Ihandle* sb_vert, *sb_horiz;

  if (ih->firstchild && iupAttribGet(ih->firstchild, "SB_VERT"))
    return 0;

  sb_horiz = IupCanvas(NULL);
  IupSetAttribute(sb_horiz, "BORDER", "NO");
  IupSetAttribute(sb_horiz, "ZORDER", "TOP");
  iupAttribSet(sb_horiz, "SB_HORIZ", "YES");
  IupSetAttribute(sb_horiz, "CANFOCUS", "NO");
  IupSetAttribute(sb_horiz, "VISIBLE", "NO");
  IupSetCallback(sb_horiz, "ACTION", (Icallback)iFlatScrollBarAction_CB);
  IupSetCallback(sb_horiz, "BUTTON_CB", (Icallback)iFlatScrollBarButton_CB);
  IupSetCallback(sb_horiz, "MOTION_CB", (Icallback)iFlatScrollBarMotion_CB);
  IupSetCallback(sb_horiz, "LEAVEWINDOW_CB", (Icallback)iFlatScrollBarLeaveWindow_CB);
  IupSetCallback(sb_horiz, "WHEEL_CB", (Icallback)iFlatScrollBarWheel_CB);
  iupAttribSetInt(sb_horiz, "_IUP_PRESSED_HANDLER", SB_NONE);
  iupAttribSetInt(sb_horiz, "_IUP_HIGHLIGHT_HANDLER", SB_NONE);

  iChildTreeInsertFirst(ih, sb_horiz);  /* sb_horiz will always be the firstchild->brother */
  sb_horiz->flags |= IUP_INTERNAL;
  iupAttribSet(ih, "XHIDDEN", "YES");

  sb_vert = IupCanvas(NULL);
  IupSetAttribute(sb_vert, "BORDER", "NO");
  IupSetAttribute(sb_vert, "ZORDER", "TOP");
  iupAttribSet(sb_vert, "SB_VERT", "YES");
  IupSetAttribute(sb_vert, "CANFOCUS", "NO");
  IupSetAttribute(sb_vert, "VISIBLE", "NO");
  IupSetCallback(sb_vert, "ACTION", (Icallback)iFlatScrollBarAction_CB);
  IupSetCallback(sb_vert, "BUTTON_CB", (Icallback)iFlatScrollBarButton_CB);
  IupSetCallback(sb_vert, "MOTION_CB", (Icallback)iFlatScrollBarMotion_CB);
  IupSetCallback(sb_vert, "LEAVEWINDOW_CB", (Icallback)iFlatScrollBarLeaveWindow_CB);
  IupSetCallback(sb_vert, "WHEEL_CB", (Icallback)iFlatScrollBarWheel_CB);
  iupAttribSetInt(sb_vert, "_IUP_PRESSED_HANDLER", SB_NONE);
  iupAttribSetInt(sb_vert, "_IUP_HIGHLIGHT_HANDLER", SB_NONE);

  iChildTreeInsertFirst(ih, sb_vert);  /* sb_vert will always be the firstchild */
  sb_vert->flags |= IUP_INTERNAL;
  iupAttribSet(ih, "YHIDDEN", "YES");

  return 1;
}

IUP_SDK_API void iupFlatScrollBarRelease(Ihandle* ih)
{
  Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_FLOATTIMER");
  if (timer)
  {
    IupSetAttribute(timer, "RUN", "NO");
    IupDestroy(timer);
  }
}

IUP_SDK_API void iupFlatScrollBarRegister(Iclass* ic)
{
  iupClassRegisterGetAttribute(ic, "DX", NULL, &iupCanvasSetDXAttrib, NULL, NULL, NULL);
  iupClassRegisterGetAttribute(ic, "DY", NULL, &iupCanvasSetDYAttrib, NULL, NULL, NULL);
  iupClassRegisterGetAttribute(ic, "POSX", &iupCanvasGetPosXAttrib, &iupCanvasSetPosXAttrib, NULL, NULL, NULL);
  iupClassRegisterGetAttribute(ic, "POSY", &iupCanvasGetPosYAttrib, &iupCanvasSetPosYAttrib, NULL, NULL, NULL);

  iupClassRegisterAttribute(ic, "DX", NULL, iFlatScrollBarSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, iFlatScrollBarSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iFlatScrollBarGetPosXAttrib, iFlatScrollBarSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iFlatScrollBarGetPosYAttrib, iFlatScrollBarSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWFLOATING", NULL, iFlatScrollBarSetShowFloatingAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FLOATINGDELAY", NULL, NULL, IUPAF_SAMEASSYSTEM, "2000", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWTRANSPARENT", NULL, iFlatScrollBarSetShowTransparentAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SCROLLBARSIZE", NULL, NULL, IUPAF_SAMEASSYSTEM, "15", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_FORECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "220 220 220", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_BACKCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_HIGHCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "132 132 132", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_PRESSCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "96 96 96", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWARROWS", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ARROWIMAGES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SB_IMAGELEFT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGELEFTPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGELEFTHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGELEFTINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SB_IMAGERIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGERIGHTPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGERIGHTHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGERIGHTINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SB_IMAGETOP", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGETOPPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGETOPHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGETOPINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SB_IMAGEBOTTOM", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGEBOTTOMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGEBOTTOMHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SB_IMAGEBOTTOMINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}

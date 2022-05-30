/** \file
 * \brief Colorbar Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iupdraw.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_image.h"
#include "iup_register.h"


#define ICOLORBAR_DEFAULT_NUM_CELLS 16  /* default number of cells */
#define ICOLORBAR_NO_COLOR 0xff000000   /* no color                */
#define ICOLORBAR_PREVIEW_DELTA 5       /* preview margin          */
#define ICOLORBAR_DELTA 2               /* cell margin             */
#define ICOLORBAR_PRIMARY -1
#define ICOLORBAR_SECONDARY -2


struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  int w;                  /* size of the canvas - width                             */
  int h;                  /* size of the canvas - height                            */
  long colors[256];       /* color array                                            */
  int num_cells;          /* number of cells at the widgets                         */
  int num_parts;          /* number of sections used to split the colors cells area */
  int vertical;           /* vertical orientation flag                              */
  int squared;            /* square cells attribute flag                            */
  int shadowed;           /* 3D shadowed attribute flag                             */
  long bgcolor;           /* control background color                               */
  long light_shadow;      /* }                                                      */
  long mid_shadow;        /* } 3D shadowed color                                    */
  long dark_shadow;       /* }                                                      */
  long transparency;      /* transparency color                                     */
  int show_secondary;     /* secondary color selection flag                         */
  int preview_size;       /* preview size (pixels) 0=disabled, -1=automatic         */
  int fgcolor_idx;        /* current primary index selected                         */
  int bgcolor_idx;        /* current secondary index selected                       */
  int focus_cell;         /* cell with focus                                        */
  int has_focus;          /* 1 if the control has the focus, else 0                 */
  int focus_select;       /* select when focus is changed                           */
  long flatcolor;
};

/* Default colors used for a widget */
static struct {
  unsigned int r;
  unsigned int g;
  unsigned int b;
} default_colors[ICOLORBAR_DEFAULT_NUM_CELLS] = {
  {   0,   0,   0 }, { 128,   0,   0 }, {   0, 128,   0 }, { 128, 128,   0 },    
  {   0,   0, 128 }, { 128,   0, 128 }, {   0, 128, 128 }, { 192, 192, 192 },    
  { 128, 128, 128 }, { 255,   0,   0 }, {   0, 255,   0 }, { 255, 255,   0 },
  {   0,   0, 255 }, { 255,   0, 255 }, {   0, 255, 255 }, { 255, 255, 255 }
};


/* This function is used to draw a box for a cell. */
static void iColorbarDrawBox(Ihandle* ih, int xmin, int xmax, int ymin, int ymax, int idx)
{
  long color = ih->data->colors[idx];

  iupAttribSet(ih, "DRAWSTYLE", "FILL");

  if (color == ih->data->transparency)
  { 
    int xm = (xmin+xmax)/2;
    int ym = (ymin+ymax)/2;
    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(238, 238, 238, 255));
    IupDrawRectangle(ih, xmin, ymin, xm, ym);
    IupDrawRectangle(ih, xm, ym, xmax, ymax);
    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(204, 204, 204, 255));
    IupDrawRectangle(ih, xmin, ym, xm, ymax);
    IupDrawRectangle(ih, xm, ymin, xmax, ym);
  }
  else
  {
    if (!iupdrvIsActive(ih))
      color = iupDrawColorMakeInactive(color, ih->data->bgcolor);

    iupDrawSetColor(ih, "DRAWCOLOR", color);
    IupDrawRectangle(ih, xmin, ymin, xmax, ymax);
  }

  iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(0, 0, 0, 255));
  iupAttribSet(ih, "DRAWSTYLE", "STROKE");
  IupDrawRectangle(ih, xmin, ymin, xmax, ymax);
}

/* This function is used to get the largest square of a cell bounding box. */
static void iColorbarFitSquare(int* xmin, int* xmax, int* ymin, int* ymax)
{
  int mx = (*xmax + *xmin) / 2; 
  int my = (*ymax + *ymin) / 2;
  int dx = (*xmax - *xmin) / 2; 
  int dy = (*ymax - *ymin) / 2;

  if (dx < dy)
  { 
    *ymin = my - dx; 
    *ymax = my + dx; 
  }
  else
  { 
    *xmin = mx - dy; 
    *xmax = mx + dy; 
  }
}

/* This function is used to get the preview area bounding box. */
static void iColorbarGetPreviewLimit(Ihandle* ih, int* xmin, int* xmax, int* ymin, int* ymax)
{
  int num_itens = ih->data->num_cells / ih->data->num_parts + 1;  /* include space for preview area */

  *xmin = 0; 

  if (ih->data->vertical)
  { 
    *xmax = ih->data->w - 1;
    *ymax = ih->data->h - 1;
    if (ih->data->preview_size > 0)
      *ymin = *ymax - ih->data->preview_size;
    else
      *ymin = *ymax - ih->data->h / num_itens;
  }
  else
  { 
    *ymin = 0;
    *ymax = ih->data->h - 1;
    if (ih->data->preview_size > 0)
      *xmax = *xmin + ih->data->preview_size;
    else
      *xmax = ih->data->w / num_itens;
  }
  if (ih->data->squared)
    iColorbarFitSquare(xmin, xmax, ymin, ymax);
}

/* This function is used to get a cell bounding box. */
static void iColorbarGetCellLimit(Ihandle* ih, int idx, int* xmin, int* xmax, int* ymin, int* ymax)
{
  int delta;
  int wcell, hcell;
  int preview_w = 0, preview_h = 0;
  int p_xmax = 0, p_ymin = ih->data->h - 1;
  int posx, posy;
  int num_itens = ih->data->num_cells / ih->data->num_parts;

  if (ih->data->preview_size != 0)
  {
    int p_xmin, p_ymax;
    iColorbarGetPreviewLimit(ih, &p_xmin, &p_xmax, &p_ymin, &p_ymax);
    preview_w = p_xmax - p_xmin + 1;
    preview_h = p_ymax - p_ymin + 1;
  }

  if (ih->data->vertical)  /* Vertical orientation */
  { 
    wcell = ih->data->w / ih->data->num_parts;
    hcell = (ih->data->h - preview_h) / num_itens;
    posx = idx / num_itens;
    posy = idx % num_itens;
    if (ih->data->squared)
    { 
      wcell = wcell < hcell ? wcell : hcell;
      hcell = wcell;
    }
    delta = (ih->data->w - (ih->data->num_parts * wcell)) / 2;
    *xmin = delta  + (posx + 0) * wcell;
    *xmax = delta  + (posx + 1) * wcell;
    *ymin = p_ymin - (posy + 1) * hcell;
    *ymax = p_ymin - (posy + 0) * hcell;
  }
  else  /* Horizontal orientation */
  {  
    hcell = ih->data->h / ih->data->num_parts;
    wcell = (ih->data->w - preview_w) / num_itens;
    posx = idx % num_itens;
    posy = idx / num_itens;
    if (ih->data->squared)
    { 
      wcell = wcell < hcell ? wcell : hcell;
      hcell = wcell;
    }
    delta = (ih->data->h - (ih->data->num_parts * hcell)) / 2;
    *xmin = p_xmax + (posx + 0) * wcell;
    *xmax = p_xmax + (posx + 1) * wcell;
    *ymin = delta  + (posy + 0) * hcell;
    *ymax = delta  + (posy + 1) * hcell;
  }
}

/* This function is used to get the index color of a canvas coordinate. */
static int iColorbarGetIndexColor(Ihandle* ih, int x, int y)
{
  int i;
  int xmin, ymin;
  int xmax, ymax;

  for (i = 0; i < ih->data->num_cells; i++)
  { 
    iColorbarGetCellLimit(ih, i, &xmin, &xmax, &ymin, &ymax);
    if (x > xmin && x < xmax && y > ymin && y < ymax)
      return i;
  }

  return -1;
}

/* This function is used to repaint the preview area. */
static void iColorbarRenderPreview(Ihandle* ih)
{
  int delta = ICOLORBAR_PREVIEW_DELTA;
  int xmin,  ymin;
  int xmax,  ymax;
  int xhalf, yhalf;
  int bg = ih->data->bgcolor_idx;
  int fg = ih->data->fgcolor_idx;

  if (ih->data->preview_size == 0)
    return;

  iColorbarGetPreviewLimit(ih, &xmin, &xmax, &ymin, &ymax);

  if (xmax-xmin < delta || ymax-ymin < delta)
    delta = 0;
  
  if (ih->data->show_secondary)
  { 
    xhalf = 2 * (xmax - xmin - 2 * delta) / 3 + delta;
    yhalf = 2 * (ymax - ymin - 2 * delta) / 3 + delta;

    iColorbarDrawBox(ih, xmax - xhalf, xmax - delta, 
                         ymax - yhalf, ymax - delta, bg);  /* secondary bellow */
    iColorbarDrawBox(ih, xmin + delta, xmin + xhalf, 
                         ymin + delta, ymin + yhalf, fg);  /* primary above */
  }
  else
    iColorbarDrawBox(ih, xmin + delta, xmax - delta, ymin + delta, ymax - delta, fg);
}

static void iColorbarDrawFocusCell(Ihandle* ih)
{
  int xmin, ymin;
  int xmax, ymax;

  iColorbarGetCellLimit(ih, ih->data->focus_cell, &xmin, &xmax, &ymin, &ymax);

  IupDrawFocusRect(ih, xmin, ymin, xmax, ymax);
}

/* This function is used to repaint a cell. */
static void iColorbarRenderCell(Ihandle* ih, int idx)
{
  int delta = ICOLORBAR_DELTA;
  int xmin, ymin;
  int xmax, ymax;

  iColorbarGetCellLimit(ih, idx, &xmin, &xmax, &ymin, &ymax);
  xmin += delta;
  xmax -= delta;
  ymin += delta;
  ymax -= delta;
  
  iColorbarDrawBox(ih, xmin, xmax, ymin, ymax, idx);
  
  if (ih->data->shadowed)
    iupDrawSunkenRect(ih, xmin, ymin, xmax, ymax, ih->data->light_shadow, ih->data->mid_shadow, ih->data->dark_shadow);
}

/* This function loops all cells, repainting them. */
static void iColorbarRenderCells(Ihandle* ih)
{
  int i;
  for (i = 0; i < ih->data->num_cells; i++)
    iColorbarRenderCell(ih, i);
}

static int iColorbarCheckPreview(Ihandle* ih, int x, int y)
{
  int xmin,  ymin;
  int xmax,  ymax;
  int xhalf, yhalf;
  int delta = ICOLORBAR_PREVIEW_DELTA;

  if (ih->data->preview_size == 0)
    return 0;

  iColorbarGetPreviewLimit(ih, &xmin, &xmax, &ymin, &ymax);

  if (ih->data->show_secondary)
  { 
    xhalf = 2 * (xmax - xmin - 2 * delta) / 3 + delta;
    yhalf = 2 * (ymax - ymin - 2 * delta) / 3 + delta;

    if (x > xmin + delta && x < xmin + xhalf && 
        y > ymin + delta && y < ymin + yhalf)
      return ICOLORBAR_PRIMARY;
    if (x > xmax - xhalf && x < xmax - delta && 
        y > ymax - yhalf && y < ymax - delta)
      return ICOLORBAR_SECONDARY;
    if (x > xmin && x < xmax && y > ymin && y < ymax)
      return 1;  /* switch */
  }
  else
  { 
    if (x > xmin + delta && x < xmax - delta && y > ymin + delta && y < ymax - delta)
      return ICOLORBAR_PRIMARY;
  }

  return 0;
}

static int iColorbarSetNumPartsAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToInt(value, &ih->data->num_parts))
    IupUpdate(ih);
  return 0;
}

static char* iColorbarGetNumPartsAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->num_parts);
}

static int iColorbarSetPrimaryCellAttrib(Ihandle* ih, const char* value)
{
  int new_val;
  if (iupStrToInt(value, &new_val))
  {
    if (new_val > 0 && new_val < ih->data->num_cells)
    { 
      ih->data->fgcolor_idx = new_val;
      IupUpdate(ih);
    }
  }
  return 0;
}

static char* iColorbarGetPrimaryCellAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->fgcolor_idx);
}

static int iColorbarSetSecondaryCellAttrib(Ihandle* ih, const char* value)
{
  int new_val;
  if (iupStrToInt(value, &new_val))
  {
    if (new_val > 0 && new_val < ih->data->num_cells)
    { 
      ih->data->bgcolor_idx = new_val;
      IupUpdate(ih);
    }
  }

  return 0;
}

static char* iColorbarGetSecondaryCellAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->bgcolor_idx);
}

static int iColorbarSetNumCellsAttrib(Ihandle* ih, const char* value)
{
  int new_val;
  if (iupStrToInt(value, &new_val))
  {
    if (new_val > 0 && new_val <= 256)
    { 
      ih->data->num_cells = new_val;

      if (ih->data->fgcolor_idx >= ih->data->num_cells)
        ih->data->fgcolor_idx = ih->data->num_cells - 1;

      if (ih->data->bgcolor_idx >= ih->data->num_cells)
        ih->data->bgcolor_idx = ih->data->num_cells - 1;

      IupUpdate(ih);
    }
  }
  return 0;
}

static char* iColorbarGetNumCellsAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->num_cells);
}

static int iColorbarSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "HORIZONTAL"))
    ih->data->vertical = 0;
  else
    ih->data->vertical = 1;

  IupUpdate(ih);
  return 0;
}

static char* iColorbarGetOrientationAttrib(Ihandle* ih)
{
  if (ih->data->vertical) 
    return "VERTICAL";
  else 
    return "HORIZONTAL";
}

static int iColorbarSetSquaredAttrib(Ihandle* ih, const char* value)
{
  ih->data->squared = iupStrBoolean(value);
  IupUpdate(ih);
  return 0;
}

static char* iColorbarGetSquaredAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->squared);
}

static int iColorbarSetFocusSelectAttrib(Ihandle* ih, const char* value)
{
  ih->data->focus_select = iupStrBoolean(value);
  IupUpdate(ih);
  return 0;
}

static char* iColorbarGetFocusSelectAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->focus_select);
}

static int iColorbarSetShadowedAttrib(Ihandle* ih, const char* value)
{
  ih->data->shadowed = iupStrBoolean(value);
  IupUpdate(ih);
  return 0;
}

static char* iColorbarGetShadowedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->shadowed);
}

static int iColorbarSetFlatAttrib(Ihandle* ih, const char* value)
{
  ih->data->shadowed = !iupStrBoolean(value);
  IupUpdate(ih);
  return 0;
}

static char* iColorbarGetFlatAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(!ih->data->shadowed);
}

static int iColorbarSetShowSecondaryAttrib(Ihandle* ih, const char* value)
{
  ih->data->show_secondary = iupStrBoolean(value);
  IupUpdate(ih);
  return 0;
}

static int iColorbarSetFlatColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->flatcolor = iupDrawStrToColor(value, ih->data->flatcolor);
  IupUpdate(ih);
  return 1;
}

static char* iColorbarGetShowSecondaryAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->show_secondary);
}

static int iColorbarSetShowPreviewAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->preview_size = -1;  /* reset to automatic */
  else
    ih->data->preview_size = 0;
  
  IupUpdate(ih);
  return 1;
}

static int iColorbarSetPreviewSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->data->preview_size = -1;
    IupUpdate(ih);
  }
  else if (iupStrToInt(value, &ih->data->preview_size))
    IupUpdate(ih);
  return 0;
}

static char* iColorbarGetPreviewSizeAttrib(Ihandle* ih)
{
  if (ih->data->preview_size == -1)  /* automatic */
    return NULL;
  else 
    return iupStrReturnInt(ih->data->preview_size);
}

static int iColorbarSetCellAttrib(Ihandle* ih, int id, const char* value)
{
  if (id >= 0 || id < ih->data->num_cells)
  { 
    ih->data->colors[id] = iupDrawStrToColor(value, ih->data->colors[id]);
    IupUpdate(ih);
  }

  return 0;
}

static char* iColorbarGetCellAttrib(Ihandle* ih, int id)
{
  long color;
  
  if (id < 0 || id >= ih->data->num_cells)
    return NULL;

  color = ih->data->colors[id];
  return iupStrReturnRGB(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color));
}

static int iColorbarSetTransparencyAttrib(Ihandle* ih, const char* value)
{
  if (value == NULL)
    ih->data->transparency = ICOLORBAR_NO_COLOR;
  else
    ih->data->transparency = iupDrawStrToColor(value, ih->data->transparency);

  IupUpdate(ih);
  return 1;
}

static int iColorbarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = iupBaseNativeParentGetBgColorAttrib(ih);

  ih->data->bgcolor = iupDrawStrToColor(value, ih->data->bgcolor);

  iupDrawCalcShadows(ih->data->bgcolor, &ih->data->light_shadow, &ih->data->mid_shadow, &ih->data->dark_shadow);

  if (!iupdrvIsActive(ih))
    ih->data->light_shadow = ih->data->mid_shadow;
  
  IupUpdate(ih);
  return 1;
}

static int iColorbarSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);

  iupDrawCalcShadows(ih->data->bgcolor, &ih->data->light_shadow, &ih->data->mid_shadow, &ih->data->dark_shadow);

  if (!iupdrvIsActive(ih))
    ih->data->light_shadow = ih->data->mid_shadow;

  IupUpdate(ih);
  return 0;   /* do not store value in hash table */
}

static int iColorbarRedraw_CB(Ihandle* ih)
{
  IupDrawBegin(ih);

  IupDrawParentBackground(ih);

  iColorbarRenderPreview(ih);
  iColorbarRenderCells(ih);

  if (ih->data->has_focus)
    iColorbarDrawFocusCell(ih);

  IupDrawEnd(ih);

  return IUP_DEFAULT;
}

static int iColorbarResize_CB(Ihandle* ih, int w, int h)
{
  ih->data->w = w;
  ih->data->h = h;
  return IUP_DEFAULT;
}

static int iColorbarFocus_CB(Ihandle* ih, int focus)
{
  ih->data->has_focus = focus;
  IupUpdate(ih);
  return IUP_DEFAULT;
}

static void iColorbarCallExtentedCb(Ihandle* ih, int idx)
{
  IFni extended_cb = (IFni)IupGetCallback(ih, "EXTENDED_CB");
  if (!extended_cb)
    return;

  if (extended_cb(ih, idx) == IUP_IGNORE)
    return;

  IupUpdate(ih);
}
    
static void iColorbarCallSelectCb(Ihandle* ih, int idx, int type)
{ 
  IFnii select_cb;

  if (type == ICOLORBAR_SECONDARY && !ih->data->show_secondary)
    return;

  select_cb = (IFnii)IupGetCallback(ih, "SELECT_CB");
  if (select_cb && select_cb(ih, idx, type) == IUP_IGNORE)
    return;

  if (type == ICOLORBAR_PRIMARY)
    ih->data->fgcolor_idx = idx;
  else
    ih->data->bgcolor_idx = idx;

  IupUpdate(ih);
}

static void iColorbarCallCellCb(Ihandle* ih, int idx)
{
  char* ret;
  sIFni cell_cb = (sIFni)IupGetCallback(ih, "CELL_CB");
  if (!cell_cb)
    return;

  ret = cell_cb(ih, idx);  /* the application can change the color */
  if (ret) 
  {
    ih->data->colors[idx] = iupDrawStrToColor(ret, ih->data->colors[idx]);
    IupUpdate(ih);
  }
}

static int iColorbarKeyPress_CB(Ihandle* ih, int c, int press)
{
  int old_focus_cell;

  if (c != K_LEFT && c != K_UP && c != K_RIGHT && c != K_DOWN &&
      c != K_HOME && c != K_END &&
      c != K_SP && c != K_sCR && c != K_sSP && c != K_cSP)
    return IUP_DEFAULT;

  if (!press || !ih->data->has_focus)
    return IUP_DEFAULT;

  old_focus_cell = ih->data->focus_cell;

  switch(c)
  {
  case K_LEFT:
    if (ih->data->vertical)
    {
      int cells_per_line = ih->data->num_cells / ih->data->num_parts;
      if (ih->data->focus_cell > cells_per_line)
        ih->data->focus_cell -= cells_per_line;
    }
    else
    {
      if (ih->data->focus_cell > 0)
        ih->data->focus_cell--;
    }
    break;
  case K_DOWN:
    if (ih->data->vertical)
    {
      if (ih->data->focus_cell > 0)
        ih->data->focus_cell--;
    }
    else
    {
      int cells_per_line = ih->data->num_cells / ih->data->num_parts;
      if (ih->data->focus_cell + cells_per_line < ih->data->num_cells - 1)
        ih->data->focus_cell += cells_per_line;
    }
    break;
  case K_RIGHT:
    if (ih->data->vertical)
    {
      int cells_per_line = ih->data->num_cells / ih->data->num_parts;
      if (ih->data->focus_cell+cells_per_line < ih->data->num_cells-1)
        ih->data->focus_cell += cells_per_line;
    }
    else
    {
      if (ih->data->focus_cell < ih->data->num_cells-1)
        ih->data->focus_cell++;
    }
    break;
  case K_UP:
    if (ih->data->vertical)
    {
      if (ih->data->focus_cell < ih->data->num_cells-1)
        ih->data->focus_cell++;
    }
    else
    {
      int cells_per_line = ih->data->num_cells / ih->data->num_parts;
      if (ih->data->focus_cell > cells_per_line)
        ih->data->focus_cell -= cells_per_line;
    }
    break;
  case K_HOME:
    ih->data->focus_cell = 0;
    break;
  case K_END:
    ih->data->focus_cell = ih->data->num_cells-1;
    break;
  case K_sCR:
    iColorbarCallCellCb(ih, ih->data->focus_cell);
    return IUP_DEFAULT;
  case K_SP:
    iColorbarCallSelectCb(ih, ih->data->focus_cell, ICOLORBAR_PRIMARY);
    return IUP_DEFAULT;
  case K_cSP:
    iColorbarCallSelectCb(ih, ih->data->focus_cell, ICOLORBAR_SECONDARY);
    return IUP_DEFAULT;
  case K_sSP:
    iColorbarCallExtentedCb(ih, ih->data->focus_cell);
    return IUP_DEFAULT;
  }

  if (old_focus_cell != ih->data->focus_cell && ih->data->focus_select)
    iColorbarCallSelectCb(ih, ih->data->focus_cell, ICOLORBAR_PRIMARY);

  IupUpdate(ih);
  return IUP_IGNORE;  /* to avoid arrow keys being processed by the system */
}

static int iColorbarButton_CB(Ihandle* ih, int b, int m, int x, int y, char* r)
{
  int idx;

  if (m == 0)
    return IUP_DEFAULT;

  if (b == IUP_BUTTON1 && iup_isdouble(r)) 
  { 
    idx = iColorbarGetIndexColor(ih, x, y); 
    if (idx < 0  || idx >= ih->data->num_cells) 
    {
      int ret = iColorbarCheckPreview(ih, x, y);
      if (ret)
      {
        if (ret == 1)
        {
          IFnii switch_cb = (IFnii)IupGetCallback(ih, "SWITCH_CB");

          if (!ih->data->show_secondary)
            return IUP_DEFAULT;

          if (switch_cb && switch_cb(ih, ih->data->fgcolor_idx, ih->data->bgcolor_idx) == IUP_IGNORE) 
            return IUP_DEFAULT;

          /* the application allow to switch the indices */
          idx = ih->data->fgcolor_idx;
          ih->data->fgcolor_idx = ih->data->bgcolor_idx;
          ih->data->bgcolor_idx = idx;

          IupUpdate(ih);
        }
        else
        {
          if (ret == ICOLORBAR_PRIMARY)
            idx = ih->data->fgcolor_idx;
          else
            idx = ih->data->bgcolor_idx;

          iColorbarCallCellCb(ih, idx);
        }
      }

      return IUP_DEFAULT;
    }

    if (ih->data->focus_cell != idx)
    {
      ih->data->focus_cell = idx;

      if (ih->data->focus_select)
        iColorbarCallSelectCb(ih, idx, ICOLORBAR_PRIMARY);
    }

    iColorbarCallCellCb(ih, idx);
  }
  else if (b == IUP_BUTTON1)
  { 
    idx = iColorbarGetIndexColor(ih, x, y);
    if (idx < 0  || idx >= ih->data->num_cells)
      return IUP_DEFAULT;

    if (ih->data->focus_cell != idx)
    {
      ih->data->focus_cell = idx;

      iColorbarCallSelectCb(ih, idx, ICOLORBAR_PRIMARY);
    }
  }
  else if (b == IUP_BUTTON3 && iup_isshift(r)) 
  { 
    idx = iColorbarGetIndexColor(ih, x, y); 
    if (idx < 0  || idx >= ih->data->num_cells)
      return IUP_DEFAULT;

    iColorbarCallExtentedCb(ih, idx);
  }
  else if (b == IUP_BUTTON3) 
  { 
    idx = iColorbarGetIndexColor(ih, x, y); 
    if (idx < 0  || idx >= ih->data->num_cells)
      return IUP_DEFAULT;

    iColorbarCallSelectCb(ih, idx, ICOLORBAR_SECONDARY);
  }

  return IUP_DEFAULT;
}


/****************************************************************************/


static int iColorbarCreateMethod(Ihandle* ih, void **params)
{
  int i;
  (void)params;

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* default values */
  ih->data->num_cells = 16;
  ih->data->num_parts = 1;
  ih->data->vertical  = 1;
  ih->data->squared   = 1;
  ih->data->shadowed  = 1;
  ih->data->focus_cell = 0;
  ih->data->preview_size = -1;  /* automatic */
  ih->data->fgcolor_idx  = 0;   /* black */
  ih->data->bgcolor_idx  = 15;  /* white */
  ih->data->transparency = ICOLORBAR_NO_COLOR;
  ih->data->light_shadow = iupDrawColor(255, 255, 255, 255);
  ih->data->mid_shadow = iupDrawColor(192, 192, 192, 255);
  ih->data->dark_shadow = iupDrawColor(128, 128, 128, 255);
  ih->data->flatcolor = iupDrawColor(0, 0, 0, 255);

  /* Initialization of the color array */
  for (i = 0; i < ICOLORBAR_DEFAULT_NUM_CELLS; i++)
  {
    ih->data->colors[i] = iupDrawColor((unsigned char)default_colors[i].r,
                                       (unsigned char)default_colors[i].g,
                                       (unsigned char)default_colors[i].b, 
                                       255);
  }

  /* IupCanvas callbacks */
  IupSetCallback(ih, "RESIZE_CB", (Icallback)iColorbarResize_CB);
  IupSetCallback(ih, "ACTION",    (Icallback)iColorbarRedraw_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iColorbarButton_CB);
  IupSetCallback(ih, "FOCUS_CB", (Icallback)iColorbarFocus_CB);
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iColorbarKeyPress_CB);

  return IUP_NOERROR;
}

Iclass* iupColorbarNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "colorbar";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iupColorbarNewClass;
  ic->Create  = iColorbarCreateMethod;

  /* Do not need to set base attributes because they are inherited from IupCanvas */

  /* IupColorbar Callbacks */
  iupClassRegisterCallback(ic, "CELL_CB",     "i=s");
  iupClassRegisterCallback(ic, "SWITCH_CB",   "ii");
  iupClassRegisterCallback(ic, "SELECT_CB",   "ii");
  iupClassRegisterCallback(ic, "EXTENDED_CB", "i");

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupColorbar only */
  iupClassRegisterAttributeId(ic, "CELL", iColorbarGetCellAttrib, iColorbarSetCellAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iColorbarGetNumCellsAttrib, NULL, IUPAF_SAMEASSYSTEM, "16", IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUM_CELLS", iColorbarGetNumCellsAttrib, iColorbarSetNumCellsAttrib, IUPAF_SAMEASSYSTEM, "16", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUM_PARTS", iColorbarGetNumPartsAttrib, iColorbarSetNumPartsAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEW_SIZE", iColorbarGetPreviewSizeAttrib, iColorbarSetPreviewSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRIMARY_CELL", iColorbarGetPrimaryCellAttrib, iColorbarSetPrimaryCellAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SECONDARY_CELL", iColorbarGetSecondaryCellAttrib, iColorbarSetSecondaryCellAttrib, IUPAF_SAMEASSYSTEM, "15", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSSELECT", iColorbarGetFocusSelectAttrib, iColorbarSetFocusSelectAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ORIENTATION", iColorbarGetOrientationAttrib, iColorbarSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "VERTICAL", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TRANSPARENCY", NULL, iColorbarSetTransparencyAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SHOW_PREVIEW", NULL, iColorbarSetShowPreviewAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SHOW_SECONDARY", iColorbarGetShowSecondaryAttrib, iColorbarSetShowSecondaryAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SQUARED", iColorbarGetSquaredAttrib, iColorbarSetSquaredAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SHADOWED", iColorbarGetShadowedAttrib, iColorbarSetShadowedAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", iColorbarGetFlatAttrib, iColorbarSetFlatAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLATCOLOR", NULL, iColorbarSetFlatColorAttrib, IUPAF_SAMEASSYSTEM, "0 0 0", IUPAF_NOT_MAPPED);

  /* Overwrite IupCanvas Attributes */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iColorbarSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iColorbarSetBgColorAttrib, NULL, "255 255 255", IUPAF_NO_INHERIT);    /* overwrite canvas implementation, set a system default to force a new default */

  return ic;
}

IUP_API Ihandle* IupColorbar(void)
{
  return IupCreate("colorbar");
}

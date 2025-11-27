/** \file
 * \brief iupmatrix control
 * draw functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iupdraw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_image.h"

#include "iup_controls.h"

#include "iupmat_def.h"
#include "iupmat_cd.h"
#include "iupmat_draw.h"
#include "iupmat_aux.h"
#include "iupmat_getset.h"
#include "iupmat_mark.h"


/***************************************************************************/
/* IupDraw Helper Functions                                                */
/***************************************************************************/

/* Helper function to parse color string "R G B" into RGB components */
static void iMatrixParseColor(const char* color_str, unsigned char* r, unsigned char* g, unsigned char* b)
{
  if (!color_str)
  {
    *r = 255; *g = 255; *b = 255;
    return;
  }
  iupStrToRGB(color_str, r, g, b);
}

/* Helper function to set DRAWCOLOR attribute from RGB components */
static void iMatrixSetDrawColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  char color_str[32];
  sprintf(color_str, "%d %d %d", r, g, b);
  IupSetStrAttribute(ih, "DRAWCOLOR", color_str);
}

/* Helper function to set DRAWCOLOR attribute from color string */
static void iMatrixSetDrawColorStr(Ihandle* ih, const char* color_str)
{
  IupSetStrAttribute(ih, "DRAWCOLOR", color_str);
}

/* Helper function to set DRAWCOLOR from long RGB value */
static void iMatrixSetDrawColorLong(Ihandle* ih, long color)
{
  unsigned char r = (unsigned char)((color >> 16) & 0xFF);
  unsigned char g = (unsigned char)((color >> 8) & 0xFF);
  unsigned char b = (unsigned char)(color & 0xFF);
  iMatrixSetDrawColor(ih, r, g, b);
}

#define IMAT_FEEDBACK_SIZE 16

static unsigned char imatrix_toggleoff_alpha[IMAT_FEEDBACK_SIZE * IMAT_FEEDBACK_SIZE] =
{
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

static unsigned char imatrix_toggleon_alpha[IMAT_FEEDBACK_SIZE * IMAT_FEEDBACK_SIZE] =
{
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 89, 252, 89, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 84, 239, 106, 239, 84, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 84, 239, 60, 0, 60, 239, 84, 0, 0, 0, 0, 0, 255,
  255, 0, 84, 239, 60, 0, 0, 0, 60, 239, 84, 0, 0, 0, 0, 255,
  255, 16, 227, 60, 0, 0, 0, 0, 0, 60, 239, 84, 0, 0, 0, 255,
  255, 0, 8, 0, 0, 0, 0, 0, 0, 0, 60, 239, 84, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60, 239, 84, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60, 227, 16, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

static unsigned char imatrix_dropdown_alpha[IMAT_FEEDBACK_SIZE * IMAT_FEEDBACK_SIZE] =
{
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 128, 128, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 000, 128, 255, 255, 128, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 128, 255, 255, 255, 255, 128, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 128, 255, 255, 255, 255, 255, 255, 128, 000, 000, 000, 000,
  000, 000, 000, 128, 255, 255, 255, 255, 255, 255, 255, 255, 128, 000, 000, 000,
  000, 000, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 128, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 000, 000,
  000, 000, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
  000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000
};

static unsigned char imatrix_sortup_alpha[IMAT_FEEDBACK_SIZE * IMAT_FEEDBACK_SIZE] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 32, 0,
  0, 0, 32, 239, 60, 0, 0, 0, 0, 0, 0, 0, 60, 239, 32, 0,
  0, 0, 0, 84, 239, 60, 0, 0, 0, 0, 0, 60, 239, 84, 0, 0,
  0, 0, 0, 0, 84, 239, 60, 0, 0, 0, 60, 239, 84, 0, 0, 0,
  0, 0, 8, 0, 0, 84, 239, 60, 0, 60, 239, 84, 0, 0, 8, 0,
  0, 0, 32, 32, 0, 0, 84, 239, 106, 239, 84, 0, 0, 32, 32, 0,
  0, 0, 32, 239, 60, 0, 0, 89, 252, 89, 0, 0, 60, 239, 32, 0,
  0, 0, 0, 84, 239, 60, 0, 0, 31, 0, 0, 60, 239, 84, 0, 0,
  0, 0, 0, 0, 84, 239, 60, 0, 0, 0, 60, 239, 84, 0, 0, 0,
  0, 0, 0, 0, 0, 84, 239, 60, 0, 60, 239, 84, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 84, 239, 106, 239, 84, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 89, 252, 89, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static unsigned char imatrix_sortdown_alpha[IMAT_FEEDBACK_SIZE * IMAT_FEEDBACK_SIZE] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 89, 252, 89, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 84, 239, 106, 239, 84, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 84, 239, 60, 0, 60, 239, 84, 0, 0, 0, 0,
  0, 0, 0, 0, 84, 239, 60, 0, 0, 0, 60, 239, 84, 0, 0, 0,
  0, 0, 0, 84, 239, 60, 0, 0, 31, 0, 0, 60, 239, 84, 0, 0,
  0, 0, 32, 239, 60, 0, 0, 89, 252, 89, 0, 0, 60, 239, 32, 0,
  0, 0, 32, 32, 0, 0, 84, 239, 106, 239, 84, 0, 0, 32, 32, 0,
  0, 0, 8, 0, 0, 84, 239, 60, 0, 60, 239, 84, 0, 0, 8, 0,
  0, 0, 0, 0, 84, 239, 60, 0, 0, 0, 60, 239, 84, 0, 0, 0,
  0, 0, 0, 84, 239, 60, 0, 0, 0, 0, 0, 60, 239, 84, 0, 0,
  0, 0, 32, 239, 60, 0, 0, 0, 0, 0, 0, 0, 60, 239, 32, 0,
  0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 32, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/**************************************************************************/
/*  Private functions                                                     */
/**************************************************************************/

static void iMatrixDrawSetCellClipping(Ihandle* ih, int x1, int x2, int y1, int y2)
{
  /* IupDraw: manage clipping manually, intersect with existing clip */
  int saved_x1 = ih->data->clip_x1;
  int saved_y1 = ih->data->clip_y1;
  int saved_x2 = ih->data->clip_x2;
  int saved_y2 = ih->data->clip_y2;

  /* Normalize coordinates */
  if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
  if (y1 > y2) { int tmp = y1; y1 = y2; y2 = tmp; }

  /* Intersect with existing clip if active */
  if (saved_x1 != 0 || saved_y1 != 0 || saved_x2 != 0 || saved_y2 != 0)
  {
    if (x1 < saved_x1) x1 = saved_x1;
    if (x2 > saved_x2) x2 = saved_x2;
    if (y1 < saved_y1) y1 = saved_y1;
    if (y2 > saved_y2) y2 = saved_y2;
    if (x1 > x2) x2 = x1;
    if (y1 > y2) y2 = y1;
  }

  /* Set the intersected clip, but DON'T modify ih->data->clip_* here!
   * That contains the GLOBAL clip area and must not be corrupted by cell-level clipping.
   * If we overwrite it, the next cell will use THIS cell's clip as the parent, causing progressive shrinkage! */
  IupDrawSetClipRect(ih, x1, y1, x2, y2);
}

static void iMatrixDrawResetCellClipping(Ihandle* ih)
{
  /* Restore previous clip area */
  if (ih->data->clip_x1 != 0 || ih->data->clip_y1 != 0 ||
      ih->data->clip_x2 != 0 || ih->data->clip_y2 != 0)
  {
    IupDrawSetClipRect(ih, ih->data->clip_x1, ih->data->clip_y1,
                       ih->data->clip_x2, ih->data->clip_y2);
  }
  else
  {
    IupDrawResetClip(ih);
  }
}

static int iMatrixDrawCallDrawCB(Ihandle* ih, int lin, int col, int x1, int x2, int y1, int y2, IFniiiiii draw_cb)
{
  int ret;

  iMatrixDrawSetCellClipping(ih, x1, x2, y1, y2);

  /* IupDraw uses top-down coordinates natively, no Y inversion needed */
  ret = draw_cb(ih, lin, col, x1, x2, y1, y2);

  iMatrixDrawResetCellClipping(ih);

  if (ret == IUP_DEFAULT)
    return 0;

  return 1;
}

/* Set IupDraw foreground color for drawing a cell with its FOREGROUND COLOR. */
static void iMatrixDrawSetFgColor(Ihandle* ih, int lin, int col, int marked, int active)
{
  unsigned char r = 0, g = 0, b = 0;
  iupMatrixGetFgRGB(ih, lin, col, &r, &g, &b, marked, active);
  iMatrixSetDrawColor(ih, r, g, b);
  ih->data->fgcolor_r = r;
  ih->data->fgcolor_g = g;
  ih->data->fgcolor_b = b;
}

/* Set IupDraw foreground color for drawing a cell with its BACKGROUND COLOR. */
static void iMatrixDrawSetBgColor(Ihandle* ih, int lin, int col, int marked, int active)
{
  unsigned char r = 255, g = 255, b = 255;
  iupMatrixGetBgRGB(ih, lin, col, &r, &g, &b, marked, active);
  iMatrixSetDrawColor(ih, r, g, b);
  ih->data->bgcolor_r = r;
  ih->data->bgcolor_g = g;
  ih->data->bgcolor_b = b;
}

static void iMatrixDrawSetTypeColor(Ihandle* ih, const char* color, int marked, int active)
{
  unsigned char r = 0, g = 0, b = 0;
  iupMatrixGetTypeRGB(ih, color, &r, &g, &b, marked, active);
  iMatrixSetDrawColor(ih, r, g, b);
}

static int iMatrixDrawFrameVertLineTitleHighlight(Ihandle* ih, int lin, int col, int x, int y1, int y2, long framecolor)
{
  if (ih->data->flat)
    return 1;

  if (col > 0)
  {
    int transp = iupMatrixGetFrameVertColor(ih, lin, col - 1, &framecolor, 0); /* framecolor is ignored here */
    if (transp)
      return 1;
  }

  iMatrixSetDrawColorStr(ih, "255 255 255");  /* White */
  iupMATRIX_LINE(ih, x, y1, x, y2);

  return 0;
}

static int iMatrixDrawFrameHorizLineTitleHighlight(Ihandle* ih, int lin, int col, int x1, int x2, int y, long framecolor)
{
  if (ih->data->flat)
    return 1;

  if (lin > 0)
  {
    int transp = iupMatrixGetFrameHorizColor(ih, lin - 1, col, &framecolor, 0); /* framecolor is ignored here */
    if (transp)
      return 1;
  }

  iMatrixSetDrawColorStr(ih, "255 255 255");  /* White */
  iupMATRIX_LINE(ih, x1, y, x2, y);

  return 0;
}

static int iMatrixDrawFrameHorizLineCell(Ihandle* ih, int lin, int col, int x1, int x2, int y, long framecolor)
{
  int transp = iupMatrixGetFrameHorizColor(ih, lin, col, &framecolor, 0);
  if (transp)
    return 1;

  /* Convert CD color to RGB */
  unsigned char r = (unsigned char)((framecolor >> 16) & 0xFF);
  unsigned char g = (unsigned char)((framecolor >> 8) & 0xFF);
  unsigned char b = (unsigned char)(framecolor & 0xFF);
  iMatrixSetDrawColor(ih, r, g, b);
  iupMATRIX_LINE(ih, x1, y, x2, y);   /* horizontal line */
  return 0;
}

static int iMatrixDrawFrameHorizLineTitle(Ihandle* ih, int col, int x1, int x2, int y, long framecolor)
{
  int transp = iupMatrixGetFrameHorizColor(ih, 0, col, &framecolor, 1);
  if (transp)
    return 1;

  /* Convert CD color to RGB */
  unsigned char r = (unsigned char)((framecolor >> 16) & 0xFF);
  unsigned char g = (unsigned char)((framecolor >> 8) & 0xFF);
  unsigned char b = (unsigned char)(framecolor & 0xFF);
  iMatrixSetDrawColor(ih, r, g, b);
  iupMATRIX_LINE(ih, x1, y, x2, y);   /* horizontal line */
  return 0;
}

static int iMatrixDrawFrameVertLineTitle(Ihandle* ih, int lin, int x, int y1, int y2, long framecolor)
{
  int transp = iupMatrixGetFrameVertColor(ih, lin, 0, &framecolor, 1);
  if (transp)
    return 1;

  /* Convert CD color to RGB */
  unsigned char r = (unsigned char)((framecolor >> 16) & 0xFF);
  unsigned char g = (unsigned char)((framecolor >> 8) & 0xFF);
  unsigned char b = (unsigned char)(framecolor & 0xFF);
  iMatrixSetDrawColor(ih, r, g, b);
  iupMATRIX_LINE(ih, x, y1, x, y2);    /* vertical line */
  return 0;
}

static int iMatrixDrawFrameVertLineCell(Ihandle* ih, int lin, int col, int x, int y1, int y2, long framecolor)
{
  int transp = iupMatrixGetFrameVertColor(ih, lin, col, &framecolor, 0);
  if (transp)
    return 1;

  /* Convert CD color to RGB */
  unsigned char r = (unsigned char)((framecolor >> 16) & 0xFF);
  unsigned char g = (unsigned char)((framecolor >> 8) & 0xFF);
  unsigned char b = (unsigned char)(framecolor & 0xFF);
  iMatrixSetDrawColor(ih, r, g, b);
  iupMATRIX_LINE(ih, x, y1, x, y2);    /* vertical line */
  return 0;
}

static void iMatrixDrawFrameRectTitle(Ihandle* ih, int lin, int col, int x1, int x2, int y1, int y2, long framecolor, int framehighlight)
{
  /* avoid drawing over the frame of the next cell */
  x2 -= IMAT_FRAME_W / 2;
  y2 -= IMAT_FRAME_H / 2;


  /********************* VERTICAL *************************/


  /* right vertical line */
  iMatrixDrawFrameVertLineCell(ih, lin, col, x2, y1, y2, framecolor);
  if (col == 0)
  {
    /* left vertical line */
    iMatrixDrawFrameVertLineTitle(ih, lin, x1, y1, y2, framecolor);
    x1++;
  }
  else if (col == 1 && ih->data->columns.dt[0].size == 0)
  {
    /* If does not have line titles then draw the left vertical line */
    iMatrixDrawFrameVertLineCell(ih, lin, col - 1, x1, y1, y2, framecolor);
    x1++;
  }

  /* Titles have a bright vertical line near the frame, at left */
  if (framehighlight)
    iMatrixDrawFrameVertLineTitleHighlight(ih, lin, col, x1, y1 + 1, y2 - 1, framecolor);


  /********************* HORIZONTAL *************************/


  /* bottom horizontal line */
  iMatrixDrawFrameHorizLineCell(ih, lin, col, x1, x2, y2, framecolor);
  if (lin == 0)
  {
    /* top horizontal line */
    iMatrixDrawFrameHorizLineTitle(ih, col, x1, x2, y1, framecolor);
    y1++;
  }
  else if (lin == 1 && ih->data->lines.dt[0].size == 0)
  {
    /* If does not have column titles then draw the top horizontal line */
    iMatrixDrawFrameHorizLineCell(ih, lin - 1, col, x1, x2 - 1, y1, framecolor);
    y1++;
  }

  /* Titles have a bright horizontal line near the frame, at top */
  if (framehighlight)
    iMatrixDrawFrameHorizLineTitleHighlight(ih, lin, col, x1, x2 - 1, y1, framecolor);
}

static void iMatrixDrawFrameRectCell(Ihandle* ih, int lin, int col, int x1, int x2, int y1, int y2, long framecolor)
{
  int transp;

  if (col == 1 && ih->data->columns.dt[0].size == 0)
  {
    /* If does not have line titles then draw the >> left line << of the cell frame */
    iMatrixDrawFrameVertLineCell(ih, lin, col - 1, x1, y1, y2 - 1, framecolor);
  }

  if (lin == 1 && ih->data->lines.dt[0].size == 0)
  {
    /* If does not have column titles then draw the >> top line << of the cell frame */
    iMatrixDrawFrameHorizLineCell(ih, lin - 1, col, x1, x2 - 1, y1, framecolor);
  }

  /* bottom line */
  transp = iMatrixDrawFrameHorizLineCell(ih, lin, col, x1, x2 - 1, y2 - 1, framecolor);

  /* right line */
  iMatrixDrawFrameVertLineCell(ih, lin, col, x2 - 1, y1, transp ? y2 - 1 : y2 - 2, framecolor);
}

static void iMatrixDrawFeedbackImage(Ihandle* ih, int x1, int x2, int y1, int y2, int lin, int col, int active, int marked, const char*name, unsigned char* alpha)
{
  int x, y;
  Ihandle* image = IupImageGetHandle(name);
  if (image)
  {
    int image_width = IupGetInt(image, "WIDTH");
    int image_height = IupGetInt(image, "HEIGHT");
    char bgcolor_str[32];
    unsigned char r = 255, g = 255, b = 255;
    iupMatrixGetBgRGB(ih, lin, col, &r, &g, &b, marked, active);
    sprintf(bgcolor_str, "%d %d %d", r, g, b);

    y = (y2 + y1 - image_height) / 2;
    x = (x2 + x1 - image_width) / 2;

    /* IupDraw uses top-down coordinates, no Y inversion needed */
    /* Note: IupDrawImage doesn't support make_inactive or bgcolor like CD did */
    IupDrawImage(ih, (char*)name, x, y, -1, -1);
  }
  else if (alpha)
  {
    /* Render built-in feedback icon from alpha channel data */
    unsigned char fg_r, fg_g, fg_b;
    unsigned char bg_r = 255, bg_g = 255, bg_b = 255;
    int i, j, idx;

    iupMatrixGetFgRGB(ih, lin, col, &fg_r, &fg_g, &fg_b, marked, active);
    iupMatrixGetBgRGB(ih, lin, col, &bg_r, &bg_g, &bg_b, marked, active);

    y = (y2 + y1 - IMAT_FEEDBACK_SIZE) / 2;
    x = (x2 + x1 - IMAT_FEEDBACK_SIZE) / 2;

    /* Draw icon pixel by pixel, blending foreground with background using alpha */
    for (j = 0; j < IMAT_FEEDBACK_SIZE; j++)
    {
      for (i = 0; i < IMAT_FEEDBACK_SIZE; i++)
      {
        idx = j * IMAT_FEEDBACK_SIZE + i;
        if (alpha[idx] > 0)
        {
          /* Alpha blend: result = (fg * alpha + bg * (255 - alpha)) / 255 */
          unsigned char a = alpha[idx];
          unsigned char r = (fg_r * a + bg_r * (255 - a)) / 255;
          unsigned char g = (fg_g * a + bg_g * (255 - a)) / 255;
          unsigned char b = (fg_b * a + bg_b * (255 - a)) / 255;

          iMatrixSetDrawColor(ih, r, g, b);
          IupSetAttribute(ih, "DRAWSTYLE", "FILL");
          IupDrawRectangle(ih, x + i, y + j, x + i + 1, y + j + 1);
        }
      }
    }
  }
}

static int iMatrixDrawSortSign(Ihandle* ih, int x2, int y1, int y2, int col, int active)
{
  int x1;

  char* sort = iupAttribGetId(ih, "SORTSIGN", col);
  if (!sort || iupStrEqualNoCase(sort, "NO"))
    return 0;

  /* feedback area */
  iupMatrixDrawSetDropFeedbackArea(&x1, &y1, &x2, &y2);

  if (iupStrEqualNoCase(sort, "DOWN"))
    iMatrixDrawFeedbackImage(ih, x1, x2, y1, y2, 0, col, active, 0, iupAttribGet(ih, "SORTIMAGEDOWN"), imatrix_sortdown_alpha);
  else
    iMatrixDrawFeedbackImage(ih, x1, x2, y1, y2, 0, col, active, 0, iupAttribGet(ih, "SORTIMAGEUP"), imatrix_sortup_alpha);

  return 1;
}

static void iMatrixDrawDropdownButton(Ihandle* ih, int x2, int y1, int y2, int lin, int col, int marked, int active)
{
  int x1;

  /* feedback area */
  iupMatrixDrawSetDropFeedbackArea(&x1, &y1, &x2, &y2);

  iMatrixDrawFeedbackImage(ih, x1, x2, y1, y2, lin, col, active, marked, iupAttribGet(ih, "DROPIMAGE"), imatrix_dropdown_alpha);
}

static void iMatrixDrawToggle(Ihandle* ih, int x1, int x2, int y1, int y2, int lin, int col, int marked, int active, int toggle_centered)
{
  int togglevalue = 0;

  /* toggle area */
  iupMatrixDrawSetToggleFeedbackArea(toggle_centered, &x1, &y1, &x2, &y2);

  if (toggle_centered)
  {
    char* value = iupMatrixGetValueDisplay(ih, lin, col);
    togglevalue = iupStrBoolean(value);
  }
  else
    togglevalue = iupAttribGetIntId2(ih, "TOGGLEVALUE", lin, col);

  /* toggle check */
  if (togglevalue)
    iMatrixDrawFeedbackImage(ih, x1, x2, y1, y2, lin, col, active, marked, iupAttribGet(ih, "TOGGLEIMAGEON"), imatrix_toggleon_alpha);
  else
    iMatrixDrawFeedbackImage(ih, x1, x2, y1, y2, lin, col, active, marked, iupAttribGet(ih, "TOGGLEIMAGEOFF"), imatrix_toggleoff_alpha);
}

static void iMatrixDrawBackground(Ihandle* ih, int x1, int x2, int y1, int y2, int marked, int active, int lin, int col)
{
  /* avoid drawing over the frame of the next cell */
  x2 -= IMAT_FRAME_W / 2;
  y2 -= IMAT_FRAME_H / 2;

  iMatrixDrawSetBgColor(ih, lin, col, marked, active);
  iupMATRIX_BOX(ih, x1, x2, y1, y2);
}

static void iMatrixDrawText(Ihandle* ih, int x1, int x2, int y1, int y2, int col_alignment, int lin_alignment, int marked, int active, int lin, int col, const char* text)
{
  int text_alignment;
  int charheight, x, y, hidden_text_marks = 0;

  iupdrvFontGetCharSize(ih, NULL, &charheight);

  if (lin == 0 || ih->data->hidden_text_marks)
  {
    int text_w;
    iupdrvFontGetMultiLineStringSize(ih, text, &text_w, NULL);
    if (text_w > x2 - x1 + 1 - IMAT_PADDING_W - IMAT_FRAME_W)
    {
      if (lin == 0)
        col_alignment = IMAT_ALIGN_START;

      if (ih->data->hidden_text_marks)
        hidden_text_marks = 1;
    }
  }

  /* Set the color used to draw the text */
  iMatrixDrawSetFgColor(ih, lin, col, marked, active);

  /* Set the font for drawing text */
  IupSetAttribute(ih, "DRAWFONT", iupMatrixGetFont(ih, lin, col));

  /* Set the clip area to the cell region informed, the text maybe greater than the cell */
  if (hidden_text_marks)
  {
    int crop = iupdrvFontGetStringWidth(ih, "...") + 2;
    iMatrixDrawSetCellClipping(ih, x1, x2 - crop, y1, y2);
  }
  else
  {
    iMatrixDrawSetCellClipping(ih, x1, x2, y1, y2);
  }

  /* Create an space between text and cell frame */
  x1 += IMAT_PADDING_W / 2;       x2 -= IMAT_PADDING_W / 2;
  y1 += IMAT_PADDING_H / 2;       y2 -= IMAT_PADDING_H / 2;

  iupMatrixGetCellAlign(ih, lin, col, &col_alignment, &lin_alignment);

  /* IupDraw: Use top-left positioning and let IupDrawText handle alignment via bounding box */
  if (lin_alignment == IMAT_ALIGN_CENTER)
  {
    y = iupROUND((y1 + y2) / 2.0);

    if (col_alignment == IMAT_ALIGN_CENTER)
    {
      x = iupROUND((x1 + x2) / 2.0);
      text_alignment = IUP_ALIGN_ACENTER;  /* Center both H and V */
    }
    else if (col_alignment == IMAT_ALIGN_START)
    {
      x = x1;
      text_alignment = IUP_ALIGN_ALEFT;  /* Left, center V */
    }
    else  /* RIGHT */
    {
      x = x2;
      text_alignment = IUP_ALIGN_ARIGHT;  /* Right, center V */
    }
  }
  else if (lin_alignment == IMAT_ALIGN_START)
  {
    y = y1;

    if (col_alignment == IMAT_ALIGN_CENTER)
    {
      x = iupROUND((x1 + x2) / 2.0);
      text_alignment = IUP_ALIGN_ACENTER | IUP_ALIGN_ATOP;  /* Center H, top V */
    }
    else if (col_alignment == IMAT_ALIGN_START)
    {
      x = x1;
      text_alignment = IUP_ALIGN_ALEFT | IUP_ALIGN_ATOP;  /* Left-top */
    }
    else  /* RIGHT */
    {
      x = x2;
      text_alignment = IUP_ALIGN_ARIGHT | IUP_ALIGN_ATOP;  /* Right-top */
    }
  }
  else /* lin_alignment == IMAT_ALIGN_END */
  {
    y = y2;

    if (col_alignment == IMAT_ALIGN_CENTER)
    {
      x = iupROUND((x1 + x2) / 2.0);
      text_alignment = IUP_ALIGN_ACENTER | IUP_ALIGN_ABOTTOM;  /* Center H, bottom V */
    }
    else if (col_alignment == IMAT_ALIGN_START)
    {
      x = x1;
      text_alignment = IUP_ALIGN_ALEFT | IUP_ALIGN_ABOTTOM;  /* Left-bottom */
    }
    else  /* RIGHT */
    {
      x = x2;
      text_alignment = IUP_ALIGN_ARIGHT | IUP_ALIGN_ABOTTOM;  /* Right-bottom */
    }
  }

  /* Get actual text size to properly center it within the cell, avoiding clipping */
  int text_w, text_h;
  iupdrvFontGetMultiLineStringSize(ih, text, &text_w, &text_h);

  /* Adjust position based on alignment and actual text size */
  if (col_alignment == IMAT_ALIGN_CENTER)
  {
    x = x1 + (x2 - x1 - text_w) / 2;
  }
  else if (col_alignment == IMAT_ALIGN_END)
  {
    x = x2 - text_w;
  }
  /* else START alignment, x = x1 already set */

  if (lin_alignment == IMAT_ALIGN_CENTER)
  {
    y = y1 + (y2 - y1 - text_h) / 2;
  }
  else if (lin_alignment == IMAT_ALIGN_END)
  {
    y = y2 - text_h;
  }
  /* else START alignment, y = y1 already set */

  /* Draw text at exact position without bounding box - this prevents clipping */
  IupDrawText(ih, text, -1, x, y, 0, 0);

  iMatrixDrawResetCellClipping(ih);

  if (hidden_text_marks)
  {
    IupSetInt(ih, "DRAWTEXTALIGNMENT", IUP_ALIGN_ARIGHT);
    y = (int)((y1 + y2) / 2.0 - 0.5);
    x = x2 + IMAT_PADDING_W / 2;
    iupMATRIX_TEXT(ih, x, y, "...");
  }
}

static void iMatrixDrawColor(Ihandle* ih, int x1, int x2, int y1, int y2, int marked, int active, const char* color, long framecolor)
{
  x1 += IMAT_PADDING_W / 2 + IMAT_FRAME_H / 2;
  x2 -= IMAT_PADDING_W / 2 + IMAT_FRAME_W / 2;
  y1 += IMAT_PADDING_H / 2 + IMAT_FRAME_H / 2;
  y2 -= IMAT_PADDING_H / 2 + IMAT_FRAME_H / 2;

  if (!iupAttribGetBoolean(ih, "TYPECOLORINACTIVE"))
    active = 1; /* draw as active */

  /* Fill the box with the color */
  iMatrixDrawSetTypeColor(ih, color, marked, active);
  iupMATRIX_BOX(ih, x1, x2, y1, y2);

  /* Draw the frame */
  iMatrixSetDrawColorLong(ih, framecolor);
  iupMATRIX_RECT(ih, x1, x2, y1, y2);
}

static void iMatrixDrawFill(Ihandle* ih, int x1, int x2, int y1, int y2, int marked, int active, int lin, int col, const char* value, long framecolor)
{
  int empty, fill = 0;

  iupStrToInt(value, &fill);
  if (fill < 0) fill = 0;
  if (fill > 100) fill = 100;

  /* Create an space between text and cell frame */
  x1 += IMAT_PADDING_W / 2;       x2 -= IMAT_PADDING_W / 2;
  y1 += IMAT_PADDING_H / 2;       y2 -= IMAT_PADDING_H / 2;

  empty = ((x2 - x1)*(100 - fill)) / 100;

  /* Fill the box with the color */
  iMatrixDrawSetFgColor(ih, lin, col, marked, active);
  iupMATRIX_BOX(ih, x1, x2 - empty, y1, y2);

  if (ih->data->show_fill_value)
  {
    int y = (int)((y1 + y2) / 2.0 - 0.5);
    int empty1 = ((x2 - x1)*fill) / 100;
    char text[50];
    sprintf(text, "%d%%", fill);
    IupSetAttribute(ih, "DRAWFONT", iupMatrixGetFont(ih, lin, col));
    IupSetInt(ih, "DRAWTEXTALIGNMENT", IUP_ALIGN_ACENTER);

    iMatrixDrawSetCellClipping(ih, x1 + empty1, x2, y1, y2);
    iupMATRIX_TEXT_CENTERED(ih, x1 + empty1, x2, y1, y2, text);
    iMatrixDrawResetCellClipping(ih);

    iMatrixDrawSetBgColor(ih, lin, col, marked, active);
    iMatrixDrawSetCellClipping(ih, x1, x2 - empty, y1, y2);
    iupMATRIX_TEXT_CENTERED(ih, x1, x2 - empty, y1, y2, text);
    iMatrixDrawResetCellClipping(ih);
  }


  /* Draw the frame */
  iMatrixSetDrawColorLong(ih, framecolor);
  iupMATRIX_RECT(ih, x1, x2, y1, y2);
}

static void iMatrixDrawImage(Ihandle* ih, int x1, int x2, int y1, int y2, int col_alignment, int lin_alignment, int marked, int active, int lin, int col, const char* name)
{
  Ihandle* image;

  iMatrixDrawSetCellClipping(ih, x1, x2, y1, y2);

  /* Create an space between image and cell frame */
  x1 += IMAT_PADDING_W / 2;       x2 -= IMAT_PADDING_W / 2;
  y1 += IMAT_PADDING_H / 2;       y2 -= IMAT_PADDING_H / 2;

  image = IupImageGetHandle(name);
  if (image)
  {
    char bgcolor_str[32];
    int x, y;
    int image_width = IupGetInt(image, "WIDTH");
    int image_height = IupGetInt(image, "HEIGHT");
    unsigned char r = 255, g = 255, b = 255;
    iupMatrixGetBgRGB(ih, lin, col, &r, &g, &b, marked, active);
    sprintf(bgcolor_str, "%d %d %d", r, g, b);

    if (lin_alignment == IMAT_ALIGN_CENTER)
      y = (y2 + y1 - image_height) / 2;
    else if (lin_alignment == IMAT_ALIGN_START)
      y = y1;
    else  /* BOTTOM */
      y = y2 - image_height;

    if (col_alignment == IMAT_ALIGN_CENTER)
      x = (x2 + x1 - image_width) / 2;
    else if (col_alignment == IMAT_ALIGN_START)
      x = x1;
    else  /* RIGHT */
      x = x2 - image_width;

    /* IupDraw: no Y inversion needed, image drawn directly */
    /* Note: IupDrawImage doesn't support make_inactive or bgcolor like CD did */
    IupDrawImage(ih, (char*)name, x, y, -1, -1);
  }

  iMatrixDrawResetCellClipping(ih);
}

/* Put the cell contents in the screen, using the specified color and Alignment.
   -> y1, y2 : vertical limits of the cell
   -> x1, x2 : horizontal limits of the complete cell
   -> col_alignment : Alignment type (horizontal) assigned to the text. The options are:
   [IMAT_ALIGN_CENTER,IMAT_ALIGN_START,IMAT_ALIGN_END]
   -> marked : mark state
   -> lin, col - cell coordinates */
static void iMatrixDrawCellValue(Ihandle* ih, int x1, int x2, int y1, int y2, int col_alignment, int lin_alignment, int marked, int active, int lin, int col, IFniiiiii draw_cb, long framecolor)
{
  char *value;

  /* avoid drawing over the frame of the next cell */
  x2 -= IMAT_FRAME_W / 2;
  y2 -= IMAT_FRAME_H / 2;

  /* avoid drawing over the frame of the cell */
  x2 -= IMAT_FRAME_W / 2;
  y2 -= IMAT_FRAME_H / 2;

  if (lin == 0 || col == 0)
  {
    /* avoid drawing over the frame of the cell */
    x1 += IMAT_FRAME_W / 2;
    y1 += IMAT_FRAME_H / 2;

    if (col == 0) x1 += IMAT_FRAME_W / 2;
    if (lin == 0) y1 += IMAT_FRAME_H / 2;
  }
  else if ((col == 1 && ih->data->columns.dt[0].size == 0) || (lin == 1 && ih->data->lines.dt[0].size == 0))
  {
    /* avoid drawing over the frame of the cell */
    x1 += IMAT_FRAME_W / 2;
    y1 += IMAT_FRAME_H / 2;
  }

  if (draw_cb && !iMatrixDrawCallDrawCB(ih, lin, col, x1, x2, y1, y2, draw_cb))
  {
    return;
  }

  value = iupMatrixGetValueDisplay(ih, lin, col);

  /* Put the text */
  if (value && *value)
  {
    int type = iupMatrixGetType(ih, lin, col);
    if (type == IMAT_TYPE_TEXT)
    {
      iMatrixDrawText(ih, x1, x2, y1, y2, col_alignment, lin_alignment, marked, active, lin, col, value);
    }
    else if (type == IMAT_TYPE_COLOR)
      iMatrixDrawColor(ih, x1, x2, y1, y2, marked, active, value, framecolor);
    else if (type == IMAT_TYPE_FILL)
      iMatrixDrawFill(ih, x1, x2, y1, y2, marked, active, lin, col, value, framecolor);
    else if (type == IMAT_TYPE_IMAGE)
      iMatrixDrawImage(ih, x1, x2, y1, y2, col_alignment, lin_alignment, marked, active, lin, col, value);
  }
}

static void iMatrixDrawTitleCorner(Ihandle* ih)
{
  if (ih->data->lines.dt[0].size && ih->data->columns.dt[0].size)
  {
    const char* framecolor_str = iupAttribGetStr(ih, "FRAMECOLOR");
    int active = iupdrvIsActive(ih);
    IFniiiiii draw_cb = (IFniiiiii)IupGetCallback(ih, "DRAW_CB");
    int col_alignment = iupMatrixGetColAlignmentLin0(ih);
    int lin_alignment = iupMatrixGetLinAlignment(ih, 0);
    int framehighlight = iupAttribGetInt(ih, "FRAMETITLEHIGHLIGHT");
    long framecolor;
    unsigned char r, g, b;

    /* Convert framecolor_str to long RGB value */
    iupStrToRGB(framecolor_str, &r, &g, &b);
    framecolor = ((long)r << 16) | ((long)g << 8) | (long)b;

    iMatrixDrawBackground(ih, 0, ih->data->columns.dt[0].size, 0, ih->data->lines.dt[0].size, 0, active, 0, 0);

    iMatrixDrawFrameRectTitle(ih, 0, 0, 0, ih->data->columns.dt[0].size, 0, ih->data->lines.dt[0].size, framecolor, framehighlight);

    iMatrixDrawCellValue(ih, 0, ih->data->columns.dt[0].size, 0, ih->data->lines.dt[0].size, col_alignment, lin_alignment, 0, active, 0, 0, draw_cb, framecolor);
  }
}

static void iMatrixDrawFocus(Ihandle* ih)
{
  int x1, y1, x2, y2, dx, dy;

  if (iupAttribGetBoolean(ih, "HIDEFOCUS"))
    return;

  /* there are no cells that can get the focus */
  if (ih->data->columns.num <= 1 || ih->data->lines.num <= 1)
    return;

  if (!iupMatrixAuxIsCellVisible(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell))
    return;

  iupMatrixGetVisibleCellDim(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell, &x1, &y1, &dx, &dy);

  x2 = x1 + dx - 1;
  y2 = y1 + dy - 1;

  if (ih->data->noscroll_as_title && (ih->data->columns.focus_cell < ih->data->columns.num_noscroll || ih->data->lines.focus_cell < ih->data->lines.num_noscroll))
  {
    x1++;
    y1++;
  }
  else
  {
    if (ih->data->columns.focus_cell == 1 && ih->data->columns.dt[0].size == 0)
      x1++;
    if (ih->data->lines.focus_cell == 1 && ih->data->lines.dt[0].size == 0)
      y1++;
  }

  /* IupDraw: no Y inversion needed */
  IupDrawFocusRect(ih, x1, y1, x2, y2);
}

static void iMatrixDrawColRes(Ihandle* ih)
{
  iMatrixSetDrawColor(ih, (ih->data->colres_color >> 16) & 0xFF, (ih->data->colres_color >> 8) & 0xFF, ih->data->colres_color & 0xFF);
  IupDrawLine(ih, ih->data->colres_x, ih->data->colres_y1,
                                    ih->data->colres_x, ih->data->colres_y2);
}


/**************************************************************************/
/* Exported functions                                                     */
/**************************************************************************/

void iupMatrixDrawSetDropFeedbackArea(int *x1, int *y1, int *x2, int *y2)
{
  *x2 -= IMAT_PADDING_W / 2 + IMAT_FRAME_W / 2;
  *x1 = *x2 - IMAT_FEEDBACK_SIZE - IMAT_PADDING_W / 2;

  *y1 += IMAT_PADDING_H / 2 + IMAT_FRAME_H / 2;
  *y2 -= IMAT_PADDING_H / 2 + IMAT_FRAME_H / 2;
}

void iupMatrixDrawSetToggleFeedbackArea(int toggle_centered, int *x1, int *y1, int *x2, int *y2)
{
  if (toggle_centered)
  {
    *x1 = (*x2 + *x1) / 2 - IMAT_FEEDBACK_SIZE / 2;
    *x2 = *x1 + IMAT_FEEDBACK_SIZE;
  }
  else
  {
    *x2 -= IMAT_PADDING_W / 2 + IMAT_FRAME_W / 2;
    *x1 = *x2 - IMAT_FEEDBACK_SIZE - IMAT_PADDING_W / 2;
  }

  *y1 += IMAT_PADDING_H / 2 + IMAT_FRAME_H / 2;
  *y2 -= IMAT_PADDING_H / 2 + IMAT_FRAME_H / 2;
}

static int iMatrixAdjustVisibleColToMergedCells(Ihandle *ih, int *col1, int lin1, int lin2)
{
  int lin, adjusted = 0, merged;
  int startCol;
  int new_col1 = *col1;

  /* check merged cells at col1 */
  for (lin = lin1; lin <= lin2; lin++)
  {
    merged = iupMatrixGetMerged(ih, lin, *col1);
    if (merged)
    {
      iupMatrixGetMergedRect(ih, merged, NULL, NULL, &startCol, NULL);

      if (startCol < new_col1)
      {
        new_col1 = startCol;
        adjusted = 1;
      }
    }
  }

  if (adjusted)
    *col1 = new_col1;

  return adjusted;
}

static int iMatrixAdjustVisibleLinToMergedCells(Ihandle *ih, int *lin1, int col1, int col2)
{
  int col, adjusted = 0, merged;
  int startLin;
  int new_lin1 = *lin1;

  /* check merged cells at lin1 */
  for (col = col1; col <= col2; col++)
  {
    merged = iupMatrixGetMerged(ih, *lin1, col);
    if (merged)
    {
      iupMatrixGetMergedRect(ih, merged, &startLin, NULL, NULL, NULL);

      if (startLin < new_lin1)
      {
        new_lin1 = startLin;
        adjusted = 1;
      }
    }
  }

  if (adjusted)
    *lin1 = new_lin1;

  return adjusted;
}

/* Draw the line titles, visible, between lin and lastlin, include it.
   Line titles marked will be draw with the appropriate feedback.
   -> lin1 - First line to have its title drawn
   -> lin2 - Last line to have its title drawn */
static void iMatrixDrawTitleLines(Ihandle* ih, int lin1, int lin2)
{
  int x1, y1, x2, y2, first_lin, adjust_merged_lin = 0;
  int lin, col_alignment, active, framehighlight;
  long framecolor;
  IFniiiiii draw_cb;

  /* here col==0 always */

  if (!ih->data->columns.dt[0].size)
    return;

  if (ih->data->merge_info_count)
    adjust_merged_lin = iMatrixAdjustVisibleLinToMergedCells(ih, &lin1, 0, 0);

  if (ih->data->lines.num_noscroll > 1 && lin1 == 1 && lin2 == ih->data->lines.num_noscroll - 1)
  {
    first_lin = 0;
    y1 = 0;
  }
  else
  {
    if (lin1 > ih->data->lines.last ||
        lin2 < ih->data->lines.first)
        return;

    if (!adjust_merged_lin && lin1 < ih->data->lines.first)
      lin1 = ih->data->lines.first;
    if (lin2 > ih->data->lines.last)
      lin2 = ih->data->lines.last;

    first_lin = ih->data->lines.first;
    y1 = 0;
    for (lin = 0; lin < ih->data->lines.num_noscroll; lin++)
      y1 += ih->data->lines.dt[lin].size;
  }

  /* Start the position of the line title */
  x1 = 0;
  x2 = ih->data->columns.dt[0].size;

  iupMATRIX_CLIPAREA(ih, x1, x2, y1, iupMatrixGetHeight(ih) - 1);
  /* Clip enabled */;

  /* Find the initial position of the first line */
  if (first_lin == ih->data->lines.first)
    y1 -= ih->data->lines.first_offset;
  for (lin = first_lin; lin < lin1; lin++)
    y1 += ih->data->lines.dt[lin].size;
  if (adjust_merged_lin)
  {
    for (lin = first_lin; lin > lin1; lin--)
      y1 -= ih->data->lines.dt[lin].size;
  }

  const char* framecolor_str = iupAttribGetStr(ih, "FRAMECOLOR");
  framehighlight = iupAttribGetInt(ih, "FRAMETITLEHIGHLIGHT");
  active = iupdrvIsActive(ih);
  draw_cb = (IFniiiiii)IupGetCallback(ih, "DRAW_CB");

  col_alignment = iupMatrixGetColAlignment(ih, 0);

  /* Draw the titles */
  for (lin = lin1; lin <= lin2; lin++)
  {
    int merged;

    /* If it is a hidden line (size = 0), don't draw the title */
    if (ih->data->lines.dt[lin].size == 0)
      continue;

    merged = 0;
    if (ih->data->merge_info_count)
      merged = iupMatrixGetMerged(ih, lin, 0);

    y2 = y1 + ih->data->lines.dt[lin].size;

    if (merged)
    {
      int startLin, endLin, startCol, endCol;
      iupMatrixGetMergedRect(ih, merged, &startLin, &endLin, &startCol, &endCol);

      if (lin == startLin && 0 == startCol)  /* merged start */
      {
        int i;
        for (i = startLin + 1; i <= endLin; i++)
          y2 += ih->data->lines.dt[i].size;
      }
      else
        continue;  /* ignore internal merged */
    }

    /* If it doesn't have title, the loop just calculate the final position */
    if (ih->data->columns.dt[0].size)
    {
      int marked = iupMatrixLineIsMarked(ih, lin);
      int lin_alignment = iupMatrixGetLinAlignment(ih, lin);

      iMatrixDrawBackground(ih, x1, x2, y1, y2, marked, active, lin, 0);

      iMatrixDrawFrameRectTitle(ih, lin, 0, x1, x2, y1, y2, framecolor, framehighlight);

      iMatrixDrawCellValue(ih, x1, x2, y1, y2, col_alignment, lin_alignment, marked, active, lin, 0, draw_cb, framecolor);
    }

    y1 = y2;
  }

  /* Clip disabled */;
}

/* Draw the column titles, visible, between col and lastcol, include it.
   Column titles marked will be draw with the appropriate feedback.
   -> col1 - First column to have its title drawn
   -> col2 - Last column to have its title drawn */
static void iMatrixDrawTitleColumns(Ihandle* ih, int col1, int col2)
{
  int x1, y1, x2, y2, first_col, adjust_merged_col = 0;
  int col, active, col_alignment, lin_alignment, framehighlight;
  long framecolor;
  IFniiiiii draw_cb;

  /* here lin==0 always */

  if (!ih->data->lines.dt[0].size)
    return;

  if (ih->data->merge_info_count)
    adjust_merged_col = iMatrixAdjustVisibleColToMergedCells(ih, &col1, 0, 0);

  if (ih->data->columns.num_noscroll > 1 && col1 == 1 && col2 == ih->data->columns.num_noscroll - 1)
  {
    first_col = 0;
    x1 = 0;
  }
  else
  {
    if (col1 > ih->data->columns.last ||
        col2 < ih->data->columns.first)
        return;

    if (!adjust_merged_col && col1 < ih->data->columns.first)
      col1 = ih->data->columns.first;
    if (col2 > ih->data->columns.last)
      col2 = ih->data->columns.last;

    first_col = ih->data->columns.first;
    x1 = 0;
    for (col = 0; col < ih->data->columns.num_noscroll; col++)
      x1 += ih->data->columns.dt[col].size;
  }

  /* Start the position of the first column title */
  y1 = 0;
  y2 = ih->data->lines.dt[0].size;

  iupMATRIX_CLIPAREA(ih, x1, iupMatrixGetWidth(ih) - 1, y1, y2);
  /* Clip enabled */;

  /* Find the initial position of the first column */
  if (first_col == ih->data->columns.first)
    x1 -= ih->data->columns.first_offset;
  for (col = first_col; col < col1; col++)
    x1 += ih->data->columns.dt[col].size;
  if (adjust_merged_col)
  {
    for (col = first_col; col > col1; col--)
      x1 -= ih->data->columns.dt[col].size;
  }

  const char* framecolor_str = iupAttribGetStr(ih, "FRAMECOLOR");
  framehighlight = iupAttribGetInt(ih, "FRAMETITLEHIGHLIGHT");
  active = iupdrvIsActive(ih);
  draw_cb = (IFniiiiii)IupGetCallback(ih, "DRAW_CB");
  col_alignment = iupMatrixGetColAlignmentLin0(ih);
  lin_alignment = iupMatrixGetLinAlignment(ih, 0);

  /* Draw the titles */
  for (col = col1; col <= col2; col++)
  {
    int merged;

    /* If it is hidden column (size = 0), do not draw the title */
    if (ih->data->columns.dt[col].size == 0)
      continue;

    merged = 0;
    if (ih->data->merge_info_count)
      merged = iupMatrixGetMerged(ih, 0, col);

    x2 = x1 + ih->data->columns.dt[col].size;

    if (merged)
    {
      int startLin, endLin, startCol, endCol;
      iupMatrixGetMergedRect(ih, merged, &startLin, &endLin, &startCol, &endCol);

      if (0 == startLin && col == startCol) /* merged start */
      {
        int i;
        for (i = startCol + 1; i <= endCol; i++)
          x2 += ih->data->columns.dt[i].size;
      }
      else
        continue;  /* ignore internal merged */
    }

    /* If it doesn't have title, the loop just calculate the final position */
    if (ih->data->lines.dt[0].size)
    {
      int sort = 0;
      int marked = iupMatrixColumnIsMarked(ih, col);

      iMatrixDrawBackground(ih, x1, x2, y1, y2, marked, active, 0, col);

      iMatrixDrawFrameRectTitle(ih, 0, col, x1, x2, y1, y2, framecolor, framehighlight);

      if (iMatrixDrawSortSign(ih, x2, y1, y2, col, active))
        sort = IMAT_PADDING_W / 2 + IMAT_FEEDBACK_SIZE + IMAT_PADDING_W / 2; /* same space is used by the sort sign */

      iMatrixDrawCellValue(ih, x1, x2 - sort, y1, y2, col_alignment, lin_alignment, marked, active, 0, col, draw_cb, framecolor);
    }

    x1 = x2;
  }

  /* Clip disabled */;
}

/* Redraw a block of cells of the matrix. Handle marked cells, change
   automatically the background color of them.
   - lin1, col1 : cell coordinates that mark the left top corner of the area to be redrawn
   - lin2, col2 : cell coordinates that mark the right bottom corner of the area to be redrawn */
static void iMatrixDrawCells(Ihandle* ih, int lin1, int col1, int lin2, int col2)
{
  int x1, y1, x2, y2, old_x2, old_y1, old_y2, toggle_centered;
  int col_alignment, lin, col, active, first_col, first_lin;
  int i, adjust_merged_col = 0, adjust_merged_lin = 0;
  long framecolor, framehighlight;
  IFnii mark_cb;
  IFnii dropcheck_cb;
  IFniiiiii draw_cb;

  if (ih->data->merge_info_count)
  {
    adjust_merged_lin = iMatrixAdjustVisibleLinToMergedCells(ih, &lin1, col1, col2);
    adjust_merged_col = iMatrixAdjustVisibleColToMergedCells(ih, &col1, lin1, lin2);
  }

  x2 = iupMatrixGetWidth(ih) - 1;
  y2 = iupMatrixGetHeight(ih) - 1;

  old_x2 = x2;
  old_y1 = 0;
  old_y2 = y2;

  if (ih->data->lines.num <= 1 ||
      ih->data->columns.num <= 1)
  {
      return;
  }

  if (ih->data->columns.num_noscroll > 1 && col1 == 1 && col2 == ih->data->columns.num_noscroll - 1)
  {
    first_col = 0;
    x1 = 0;
  }
  else
  {
    if (col1 > ih->data->columns.last ||
        col2 < ih->data->columns.first)
      return;

    if (!adjust_merged_col && col1 < ih->data->columns.first)
      col1 = ih->data->columns.first;
    if (col2 > ih->data->columns.last)
      col2 = ih->data->columns.last;

    first_col = ih->data->columns.first;
    x1 = 0;
    for (col = 0; col< ih->data->columns.num_noscroll; col++)
      x1 += ih->data->columns.dt[col].size;
  }

  if (ih->data->lines.num_noscroll>1 && lin1 == 1 && lin2 == ih->data->lines.num_noscroll - 1)
  {
    first_lin = 0;
    y1 = 0;
  }
  else
  {
    if (lin1 > ih->data->lines.last ||
        lin2 < ih->data->lines.first)
      return;

    if (!adjust_merged_lin && lin1 < ih->data->lines.first)
      lin1 = ih->data->lines.first;
    if (lin2 > ih->data->lines.last)
      lin2 = ih->data->lines.last;

    first_lin = ih->data->lines.first;
    y1 = 0;
    for (lin = 0; lin < ih->data->lines.num_noscroll; lin++)
      y1 += ih->data->lines.dt[lin].size;
  }

  iupMATRIX_CLIPAREA(ih, x1, x2, y1, y2);
  /* Save clip rect values in data structure AND before they get modified */
  ih->data->clip_x1 = x1;
  ih->data->clip_y1 = y1;
  ih->data->clip_x2 = x2;
  ih->data->clip_y2 = y2;
  int saved_clip_x1 = x1, saved_clip_y1 = y1, saved_clip_x2 = x2, saved_clip_y2 = y2;
  IupDrawResetClip(ih);  /* Disable clipping for background drawing */

  /* Find the initial position of the first column */
  if (first_col == ih->data->columns.first)
    x1 -= ih->data->columns.first_offset;
  for (col = first_col; col < col1; col++)
    x1 += ih->data->columns.dt[col].size;
  if (adjust_merged_col)
  {
    for (col = first_col-1; col >= col1; col--)
      x1 -= ih->data->columns.dt[col].size;
  }

  /* Find the initial position of the first line */
  if (first_lin == ih->data->lines.first)
    y1 -= ih->data->lines.first_offset;
  for (lin = first_lin; lin < lin1; lin++)
    y1 += ih->data->lines.dt[lin].size;
  if (adjust_merged_lin)
  {
    for (lin = first_lin-1; lin >= lin1; lin--)
      y1 -= ih->data->lines.dt[lin].size;
  }

  /* NOTE: Scroll offset is already handled by first_offset mechanism above.
   * Do NOT subtract POSX/POSY here - that would be a double-offset! */

  /* Find the final position of the last column */
  x2 = x1;
  for (; col <= col2; col++)
    x2 += ih->data->columns.dt[col].size;

  /* Find the final position of the last line */
  y2 = y1;
  for (; lin <= lin2; lin++)
    y2 += ih->data->lines.dt[lin].size;

  if ((col2 == ih->data->columns.num - 1) && (old_x2 > x2))
  {
    const char* emptyarea_color_str =(ih->data->bgcolor_parent);
    iMatrixSetDrawColorStr(ih, emptyarea_color_str);

    /* If it was drawn until the last column and remains space in the right of it,
       then delete this area with the the background color. */
    iupMATRIX_BOX(ih, x2, old_x2, old_y1, old_y2);
  }

  if ((lin2 == ih->data->lines.num - 1) && (old_y2 > y2))
  {
    const char* emptyarea_color_str =(ih->data->bgcolor_parent);
    iMatrixSetDrawColorStr(ih, emptyarea_color_str);

    /* If it was drawn until the last line visible and remains space below it,
       then delete this area with the the background color. */
    iupMATRIX_BOX(ih, 0, old_x2, y2, old_y2);
  }

  /* after the background */
  IupDrawSetClipRect(ih, saved_clip_x1, saved_clip_y1, saved_clip_x2, saved_clip_y2);  /* Re-enable clipping */
  /* Note: ih->data->clip_x1/y1/x2/y2 already set above */

  /***** Draw the cell values and frame */
  old_y1 = y1;
  const char* framecolor_str = iupAttribGetStr(ih, "FRAMECOLOR");
  framehighlight = iupAttribGetInt(ih, "FRAMETITLEHIGHLIGHT");
  active = iupdrvIsActive(ih);

  mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");
  dropcheck_cb = (IFnii)IupGetCallback(ih, "DROPCHECK_CB");
  draw_cb = (IFniiiiii)IupGetCallback(ih, "DRAW_CB");
  toggle_centered = iupAttribGetBoolean(ih, "TOGGLECENTERED");

  for (col = col1; col <= col2; col++)  /* For all the columns in the region */
  {
    int last_x2, last_y2;

    if (ih->data->columns.dt[col].size == 0)
      continue;

    col_alignment = iupMatrixGetColAlignment(ih, col);

    x2 = x1 + ih->data->columns.dt[col].size;
    last_x2 = x2;

    for (lin = lin1; lin <= lin2; lin++)     /* For all lines in the region */
    {
      int drop = 0;
      int marked = 0;
      int lin_alignment;
      int merged;

      if (ih->data->lines.dt[lin].size == 0)
        continue;

      merged = 0;
      if (ih->data->merge_info_count)
        merged = iupMatrixGetMerged(ih, lin, col);

      y2 = y1 + ih->data->lines.dt[lin].size;
      last_y2 = y2;

      if (merged)
      {
        int startLin, endLin, startCol, endCol;
        iupMatrixGetMergedRect(ih, merged, &startLin, &endLin, &startCol, &endCol);

        if (lin == startLin && col == startCol)  /* merged start */
        {
          for (i = startLin + 1; i <= endLin; i++)
            y2 += ih->data->lines.dt[i].size;
          for (i = startCol + 1; i <= endCol; i++)
            x2 += ih->data->columns.dt[i].size;
        }
        else
        {
          x2 = last_x2;
          y1 = last_y2;
          continue;   /* ignore internal merged */
        }
      }

      lin_alignment = iupMatrixGetLinAlignment(ih, lin);

      /* If the cell is marked, then draw it with attenuation color */
      marked = iupMatrixGetMark(ih, lin, col, mark_cb);

      if (ih->data->noscroll_as_title && (lin < ih->data->lines.num_noscroll || col < ih->data->columns.num_noscroll))
      {
        iMatrixDrawBackground(ih, x1, x2, y1, y2, marked, active, lin, 0);
        iMatrixDrawFrameRectTitle(ih, lin, col, x1, x2, y1, y2, framecolor, framehighlight);
      }
      else
      {
        iMatrixDrawBackground(ih, x1, x2, y1, y2, marked, active, lin, col);

        iMatrixDrawFrameRectCell(ih, lin, col, x1, x2, y1, y2, framecolor);

        if (dropcheck_cb)
        {
          int ret = dropcheck_cb(ih, lin, col);
          if (ret == IUP_DEFAULT)
          {
            iMatrixDrawDropdownButton(ih, x2, y1, y2, lin, col, marked, active);

            drop = IMAT_PADDING_W / 2 + IMAT_FEEDBACK_SIZE + IMAT_PADDING_W / 2;
          }
          else if (ret == IUP_CONTINUE)
          {
            iMatrixDrawToggle(ih, x1, x2, y1, y2, lin, col, marked, active, toggle_centered);

            if (toggle_centered)
            {
              y1 = last_y2;
              continue; /* do not draw the cell contents */
            }

            drop = IMAT_PADDING_W / 2 + IMAT_FEEDBACK_SIZE + IMAT_PADDING_W / 2;
          }
        }
      }

      /* draw the cell contents */
      iMatrixDrawCellValue(ih, x1, x2 - drop, y1, y2, col_alignment, lin_alignment, marked, active, lin, col, draw_cb, framecolor);

      x2 = last_x2;
      y1 = last_y2;
    }

    x1 = last_x2;
    y1 = old_y1;  /* must reset also y */
  }

  IupDrawResetClip(ih);  /* Disable clipping at end */
  ih->data->clip_x1 = 0;
  ih->data->clip_y1 = 0;
  ih->data->clip_x2 = 0;
  ih->data->clip_y2 = 0;
}

static void iMatrixDrawMatrix(Ihandle* ih)
{
  int canvas_w, canvas_h;

  iupMatrixPrepareDrawData(ih);

  /* If last is uninitialized (first draw before calcsize), initialize it */
  if (ih->data->lines.last == 0 && ih->data->lines.num > 0)
  {
    iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_LIN);
    iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_COL);
  }

  /* Get current canvas size dynamically for IupDraw */
  IupDrawGetSize(ih, &canvas_w, &canvas_h);

  /* Clear the entire canvas background first (before drawing cells) */
  iMatrixParseColor(ih->data->bgcolor_parent, &ih->data->bgcolor_r, &ih->data->bgcolor_g, &ih->data->bgcolor_b);
  iMatrixSetDrawColor(ih, ih->data->bgcolor_r, ih->data->bgcolor_g, ih->data->bgcolor_b);
  IupSetAttribute(ih, "DRAWSTYLE", "FILL");
  IupDrawRectangle(ih, 0, 0, canvas_w, canvas_h);

  /* Draw the corner between line and column titles, if necessary */
  iMatrixDrawTitleCorner(ih);

  /* If there are columns, then draw their titles */
  if (ih->data->columns.num_noscroll > 1)
    iMatrixDrawTitleColumns(ih, 1, ih->data->columns.num_noscroll - 1);
  iMatrixDrawTitleColumns(ih, ih->data->columns.first, ih->data->columns.last);

  /* If there are lines, then draw their titles */
  if (ih->data->lines.num_noscroll > 1)
    iMatrixDrawTitleLines(ih, 1, ih->data->lines.num_noscroll - 1);
  iMatrixDrawTitleLines(ih, ih->data->lines.first, ih->data->lines.last);

  /* If there are ordinary cells, then draw them */
  if (ih->data->columns.num_noscroll > 1 && ih->data->lines.num_noscroll > 1) {
    iMatrixDrawCells(ih, 1, 1, ih->data->lines.num_noscroll - 1, ih->data->columns.num_noscroll - 1);
  }
  if (ih->data->columns.num_noscroll > 1) {
    iMatrixDrawCells(ih, ih->data->lines.first, 1, ih->data->lines.last, ih->data->columns.num_noscroll - 1);
  }
  if (ih->data->lines.num_noscroll > 1) {
    iMatrixDrawCells(ih, 1, ih->data->columns.first, ih->data->lines.num_noscroll - 1, ih->data->columns.last);
  }
  iMatrixDrawCells(ih, ih->data->lines.first, ih->data->columns.first, ih->data->lines.last, ih->data->columns.last);

  if (iupAttribGetBoolean(ih, "FRAMEBORDER"))
  {
    const char* framecolor_str = iupAttribGetStr(ih, "FRAMECOLOR");
    iMatrixSetDrawColorStr(ih, framecolor_str);

    /* if vertical scrollbar is visible */
    if (!iupAttribGetBoolean(ih, "YHIDDEN"))
    {
      int posy = IupGetInt(ih, "POSY");
      int dy = IupGetInt(ih, "DY");
      int ymax = IupGetInt(ih, "YMAX");
      int height = iupMatrixGetHeight(ih);

      int width = iupMatrixGetWidth(ih);
      if (width > ih->data->columns.total_size)
        width = ih->data->columns.total_size;

      /* if scrollbar at top, top line is not necessary */
      if (posy > 0)
        iupMATRIX_LINE(ih, 0, 0, width - 1, 0);  /* top horizontal line */

      /* if scrollbar at bottom, bottom line is not necessary */
      if (posy < ymax - dy)
        iupMATRIX_LINE(ih, 0, height - 1, width - 1, height - 1);  /* bottom horizontal line */
    }

    /* if horizontal scrollbar is visible */
    if (!iupAttribGetBoolean(ih, "XHIDDEN"))
    {
      int posx = IupGetInt(ih, "POSX");
      int dx = IupGetInt(ih, "DX");
      int xmax = IupGetInt(ih, "XMAX");
      int width = iupMatrixGetWidth(ih);

      int height = iupMatrixGetHeight(ih);
      if (height > ih->data->lines.total_size)
        height = ih->data->lines.total_size;

      /* if scrollbar at left, left line is not necessary */
      if (posx > 0)
        iupMATRIX_LINE(ih, 0, 0, 0, height - 1);  /* left vertical line */

      /* if scrollbar at right, right line is not necessary */
      if (posx < xmax - dx)
        iupMATRIX_LINE(ih, width - 1, 0, width - 1, height - 1);  /* right vertical line */
    }
  }
}

void iupMatrixDrawCells(Ihandle* ih, int lin1, int col1, int lin2, int col2)
{
  iMatrixDrawCells(ih, lin1, col1, lin2, col2);
}

void iupMatrixDrawTitleColumns(Ihandle* ih, int col1, int col2)
{
  iMatrixDrawTitleColumns(ih, col1, col2);
}

void iupMatrixDrawTitleLines(Ihandle* ih, int lin1, int lin2)
{
  iMatrixDrawTitleLines(ih, lin1, lin2);
}

void iupMatrixDraw(Ihandle* ih, int update)
{
  /* For IupDraw system: Just clear the flag, actual drawing happens in iupMatrixDrawCB (ACTION callback) */
  ih->data->need_redraw = 0;

  if (update)
    iupMatrixDrawUpdate(ih);
}

void iupMatrixDrawUpdate(Ihandle* ih)
{
  iupdrvRedrawNow(ih);
}

/*
static int iMatrixDrawHasFlatScrollBar(Ihandle* ih)
{
  char* value = iupAttribGetStr(ih, "FLATSCROLLBAR");
  if (value && !iupStrEqualNoCase(value, "NO"))
    return 1;
  else
    return 0;
}
*/

void iupMatrixDrawCB(Ihandle* ih)
{
  /* called only from the ACTION callback */
  if (ih->data->need_calcsize)
  {
    iupMatrixAuxCalcSizes(ih);  /* does not use IupDraw, can be done before Begin */
    /* Note: scrollbar resize early return is disabled - was not working reliably in original code */
  }

  /* Check CLIPRECT and scroll pos before drawing */
  char* cliprect = iupAttribGet(ih, "CLIPRECT");
  int posx = IupGetInt(ih, "POSX");
  int posy = IupGetInt(ih, "POSY");
  int dx = IupGetInt(ih, "DX");
  int dy = IupGetInt(ih, "DY");
  int xmax = IupGetInt(ih, "XMAX");
  int ymax = IupGetInt(ih, "YMAX");

  /* IupDraw: All drawing must be between Begin/End calls */
  IupDrawBegin(ih);

  iMatrixDrawMatrix(ih);

  ih->data->need_redraw = 0;

  if (ih->data->has_focus)
    iMatrixDrawFocus(ih);

  if (ih->data->colres_feedback)
    iMatrixDrawColRes(ih);

  IupDrawEnd(ih);

  /* Do NOT call IupUpdate here - we're already IN the ACTION callback!
   * IupUpdate would trigger another ACTION callback = infinite loop */

  if (!ih->data->edit_hide_onfocus && ih->data->editing)
    IupUpdate(ih->data->datah);
}

int iupMatrixDrawSetRedrawAttrib(Ihandle* ih, const char* value)
{
  IupRedraw(ih, 0);  /* redraw now */
  (void)value;
  return 0;
}

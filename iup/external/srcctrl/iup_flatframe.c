/** \file
 * \brief FlatFrame Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_register.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_image.h"



static void iFlatFrameGetTitleSize(Ihandle* ih, int frame, int *width, int *height)
{
  int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TITLEIMAGEPOSITION"));
  int spacing = iupAttribGetInt(ih, "TITLEIMAGESPACING");
  char* imagename = iupAttribGet(ih, "TITLEIMAGE");
  char* title = iupAttribGet(ih, "TITLE");
  double text_orientation = iupAttribGetDouble(ih, "TITLETEXTORIENTATION");
  int horiz_padding = 0, vert_padding = 0;
  IupGetIntInt(ih, "TITLEPADDING", &horiz_padding, &vert_padding);

  iupFlatDrawGetIconSize(ih, img_position, spacing, horiz_padding, vert_padding, imagename, title, width, height, text_orientation);

  if (frame != 2 && *height != 0 && iupAttribGetBoolean(ih, "TITLELINE"))
    *height += iupAttribGetInt(ih, "TITLELINEWIDTH");
}

static int iFlatFrameGetFrame(Ihandle* ih)
{
  char* value = iupAttribGetStr(ih, "FRAME");
  if (iupStrBoolean(value))
    return 1;
  else if (iupStrEqualNoCase(value, "CROSSTITLE"))
    return 2;
  else
    return 0;
}

static int iFlatFrameRedraw_CB(Ihandle* ih)
{
  char* backcolor = iupAttribGet(ih, "BGCOLOR");  /* don't get with default value, if NULL will use from parent */
  int frame_width = iupAttribGetInt(ih, "FRAMEWIDTH");
  int frame = iFlatFrameGetFrame(ih);
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);
  int text_flags = iupDrawGetTextFlags(ih, "TITLETEXTALIGNMENT", "TITLETEXTWRAP", "TITLETEXTELLIPSIS");
  double text_orientation = iupAttribGetDouble(ih, "TITLETEXTORIENTATION");
  int active = IupGetInt(ih, "ACTIVE");
  int title_w, title_h;

  if (!backcolor)
    backcolor = iupBaseNativeParentGetBgColorAttrib(ih);

  /* draw background */
  iupFlatDrawBox(dc, 0, ih->currentwidth - 1,
                 0, ih->currentheight - 1, backcolor, NULL, 1);  /* background is always active */

  iFlatFrameGetTitleSize(ih, frame, &title_w, &title_h);

  /* draw border - can still be disabled setting frame_width=0 */
  if (frame != 0)
  {
    char* frame_color = iupAttribGetStr(ih, "FRAMECOLOR");
    int frame_top = 0;

    if (frame == 2 && title_h)
      frame_top = frame_width + title_h/2;

    iupFlatDrawBorder(dc, 0,         ih->currentwidth - 1,
                          frame_top, ih->currentheight - 1,
                          frame_width, frame_color, NULL, active);
  }
  else
    frame_width = 0;

  if (title_h)
  {
    char *titleimage = iupAttribGet(ih, "TITLEIMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    char* titlecolor = iupAttribGetStr(ih, "TITLECOLOR");
    char* titlebgcolor = iupAttribGetStr(ih, "TITLEBGCOLOR");
    int title_alignment = iupFlatGetHorizontalAlignment(iupAttribGetStr(ih, "TITLEALIGNMENT"));
    int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TITLEIMAGEPOSITION"));
    int spacing = iupAttribGetInt(ih, "TITLEIMAGESPACING");
    int horiz_padding, vert_padding;
    int make_inactive = 0, x_off = 0;

    int title_line = 0;
    if (frame != 2 && iupAttribGetBoolean(ih, "TITLELINE"))
      title_line = iupAttribGetInt(ih, "TITLELINEWIDTH");

    if (!active && titleimage)
    {
      char* titleimage_inactive = iupAttribGet(ih, "TITLEIMAGEINACTIVE");
      if (!titleimage_inactive)
        make_inactive = 1;
    }
    
    IupGetIntInt(ih, "TITLEPADDING", &horiz_padding, &vert_padding);

    /* draw title background */
    if (frame == 2)
    {
      title_alignment = IUP_ALIGN_ALEFT;
      x_off = 6;
      iupFlatDrawBox(dc, frame_width + x_off - 2, frame_width + x_off + title_w + 2,
                         frame_width, frame_width + title_h - 1 - title_line, backcolor, NULL, 1); /* background is always active */
    }
    else if (titlebgcolor)
      iupFlatDrawBox(dc, frame_width, ih->currentwidth - 1 - frame_width,
                         frame_width, frame_width + title_h - 1 - title_line, titlebgcolor, NULL, 1); /* background is always active */

    if (frame != 2 && iupAttribGetBoolean(ih, "TITLELINE"))
    {
      int i;
      char* title_line_color = iupAttribGetStr(ih, "TITLELINECOLOR");
      long color = iupDrawStrToColor(title_line_color, 0);

      /* don't use DRAWLINEWIDTH so we can control spacing */
      for (i = 0; i < title_line; i++)
        iupdrvDrawLine(dc, frame_width, frame_width + title_h - 1 - i,
                           ih->currentwidth - 1 - frame_width, frame_width + title_h - 1 - i,
                           color, IUP_DRAW_STROKE, 1);
    }

    iupFlatDrawIcon(ih, dc, frame_width + x_off, frame_width,
                    ih->currentwidth - 2 * frame_width, title_h - title_line,
                    img_position, spacing, title_alignment, IUP_ALIGN_ATOP, horiz_padding, vert_padding,
                    titleimage, make_inactive, title, text_flags, text_orientation, titlecolor, backcolor, active);
  }

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}


/*****************************************************************************************/


static char* iFlatFrameGetDecorSizeAttrib(Ihandle* ih)
{
  int height = 0;
  int width = 0;
  int title_w, title_h;
  int frame = iFlatFrameGetFrame(ih);

  if (frame != 0)
  {
    int frame_width = iupAttribGetInt(ih, "FRAMEWIDTH");
    int frame_space = iupAttribGetInt(ih, "FRAMESPACE");

    width = 2 * frame_width + 2 * frame_space;
    height = width;
  }

  iFlatFrameGetTitleSize(ih, frame, &title_w, &title_h);
  height += title_h;

  return iupStrReturnIntInt(width, height, 'x');
}

static char* iFlatFrameGetDecorOffsetAttrib(Ihandle* ih)
{
  int dx = 0;
  int dy = 0;
  int title_w, title_h;
  int frame = iFlatFrameGetFrame(ih);

  if (frame != 0)
  {
    int frame_width = iupAttribGetInt(ih, "FRAMEWIDTH");
    int frame_space = iupAttribGetInt(ih, "FRAMESPACE");

    dx = frame_width + frame_space;
    dy = dx;
  }

  iFlatFrameGetTitleSize(ih, frame, &title_w, &title_h);
  dy += title_h;

  return iupStrReturnIntInt(dx, dy, 'x');
}

static int iFlatFrameCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  IupSetCallback(ih, "ACTION", (Icallback)iFlatFrameRedraw_CB);
  return IUP_NOERROR;
}

static int iFlatFrameSetAttribPostRedraw(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
}


/******************************************************************************/


IUP_API Ihandle* IupFlatFrame(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("flatframe", children);
}

Iclass* iupFlatFrameNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("backgroundbox"));

  ic->name = "flatframe";
  ic->cons = "FlatFrame";
  ic->format = "h"; /* one Ihandle* */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDMANY+1;   /* 1 child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupFlatFrameNewClass;
  ic->Create = iFlatFrameCreateMethod;

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupFlatSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* replace IupBackgroundBox behavior */
  iupClassRegisterAttribute(ic, "DECORATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DECORSIZE", iFlatFrameGetDecorSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DECOROFFSET", iFlatFrameGetDecorOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* FlatFrame */
  iupClassRegisterAttribute(ic, "TITLE", NULL, iFlatFrameSetAttribPostRedraw, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEBGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLELINE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLELINECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLELINEWIDTH", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGEPOSITION", NULL, NULL, IUPAF_SAMEASSYSTEM, "LEFT",  IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGESPACING", NULL, NULL, IUPAF_SAMEASSYSTEM, "2", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEALIGNMENT", NULL, NULL, "ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEPADDING", NULL, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLETEXTALIGNMENT", NULL, NULL, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLETEXTWRAP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLETEXTELLIPSIS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLETEXTCLIP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLETEXTORIENTATION", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMEWIDTH", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMESPACE", NULL, NULL, IUPAF_SAMEASSYSTEM, "2", IUPAF_NO_INHERIT);

  return ic;
}

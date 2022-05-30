/** \file
 * \brief FlatButton Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_key.h"


/* from IupRadio implementation */
IUP_SDK_API Ihandle *iupRadioFindToggleParent(Ihandle* ih_toggle);


struct _IcontrolData 
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  /* attributes */
  int horiz_padding, vert_padding;  /* button margin */
  int spacing, img_position;        /* used when both text and image are displayed */
  int horiz_alignment, vert_alignment;  
  int border_width;

  /* aux */
  int has_focus,
    highlighted,
    pressed,
    inside_action;
};


/****************************************************************/


static int iFlatButtonRedraw_CB(Ihandle* ih)
{
  char *image = iupAttribGet(ih, "IMAGE");
  char* title = iupAttribGet(ih, "TITLE");
  int active = IupGetInt(ih, "ACTIVE");  /* native implementation */
  int selected = iupAttribGetInt(ih, "VALUE");
  char* fgcolor = iupAttribGetStr(ih, "FGCOLOR");
  char* bgcolor = iupAttribGet(ih, "BGCOLOR");  /* don't get with default value, if NULL will use from parent */
  char* bgimage = iupAttribGet(ih, "BACKIMAGE");
  char* fgimage = iupAttribGet(ih, "FRONTIMAGE");
  int text_flags = iupDrawGetTextFlags(ih, "TEXTALIGNMENT", "TEXTWRAP", "TEXTELLIPSIS");
  double text_orientation = iupAttribGetDouble(ih, "TEXTORIENTATION");
  const char* draw_image;
  int border_width = ih->data->border_width;
  int draw_border = iupAttribGetBoolean(ih, "SHOWBORDER");
  int image_pressed;
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);
  int make_inactive = 0;
  int focus_feedback = iupAttribGetBoolean(ih, "FOCUSFEEDBACK");

  iupDrawParentBackground(dc, ih);

  if (!bgcolor)
  {
    if (draw_border)
      bgcolor = iupFlatGetDarkerBgColor(ih);
    else
      bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  }

  if ((ih->data->pressed && ih->data->highlighted) || (selected && !ih->data->highlighted))
  {
    char* presscolor = iupAttribGetStr(ih, "PSCOLOR");
    if (presscolor)
      bgcolor = presscolor;

    presscolor = iupAttribGetStr(ih, "TEXTPSCOLOR");
    if (presscolor)
      fgcolor = presscolor;

    draw_border = 1;
  }
  else if (ih->data->highlighted)
  {
    char* hlcolor = iupAttribGetStr(ih, "HLCOLOR");
    if (hlcolor)
      bgcolor = hlcolor;

    hlcolor = iupAttribGetStr(ih, "TEXTHLCOLOR");
    if (hlcolor)
      fgcolor = hlcolor;

    draw_border = 1;
  }

  /* draw border - can still be removed by setting border_width=0 */
  if (draw_border)
  {
    char* bordercolor = iupAttribGetStr(ih, "BORDERCOLOR");

    if ((ih->data->pressed && ih->data->highlighted) || (selected && !ih->data->highlighted))
    {
      char* presscolor = iupAttribGetStr(ih, "BORDERPSCOLOR");
      if (presscolor)
        bordercolor = presscolor;
    }
    else if (ih->data->highlighted)
    {
      char* hlcolor = iupAttribGetStr(ih, "BORDERHLCOLOR");
      if (hlcolor)
        bordercolor = hlcolor;
    }

    iupFlatDrawBorder(dc, 0, ih->currentwidth - 1, 
                          0, ih->currentheight - 1, 
                          border_width, bordercolor, bgcolor, active);
  }

  /* simulate pressed when selected and has images (but colors and borders are not included) */
  image_pressed = ih->data->pressed && ih->data->highlighted;
  if (selected && !ih->data->pressed && (bgimage || image))
    image_pressed = 1;

  /* draw background */
  if (bgimage)
  {
    int backimage_zoom = iupAttribGetBoolean(ih, "BACKIMAGEZOOM");
    draw_image = iupFlatGetImageName(ih, "BACKIMAGE", bgimage, image_pressed, ih->data->highlighted, 1, &make_inactive);
    if (backimage_zoom)
      iupdrvDrawImage(dc, draw_image, make_inactive, bgcolor, border_width, border_width, ih->currentwidth - border_width, ih->currentheight - border_width);
    else
      iupdrvDrawImage(dc, draw_image, make_inactive, bgcolor, border_width, border_width, -1, -1);
  }
  else
    iupFlatDrawBox(dc, border_width, ih->currentwidth - 1 - border_width,
                       border_width, ih->currentheight - 1 - border_width,
                       bgcolor, NULL, 1);  /* background is always active */

  /* reserve space for focus feedback (after background draw) */
  if (iupAttribGetBoolean(ih, "CANFOCUS") && focus_feedback)
    border_width++;

  /* draw icon */
  draw_image = iupFlatGetImageName(ih, "IMAGE", image, image_pressed, ih->data->highlighted, active, &make_inactive);
  iupFlatDrawIcon(ih, dc, border_width, border_width,
                          ih->currentwidth - 2 * border_width, ih->currentheight - 2 * border_width,
                          ih->data->img_position, ih->data->spacing, ih->data->horiz_alignment, ih->data->vert_alignment, ih->data->horiz_padding, ih->data->vert_padding,
                          draw_image, make_inactive, title, text_flags, text_orientation, fgcolor, bgcolor, active);

  if (fgimage)
  {
    draw_image = iupFlatGetImageName(ih, "FRONTIMAGE", fgimage, image_pressed, ih->data->highlighted, active, &make_inactive);
    iupdrvDrawImage(dc, draw_image, make_inactive, bgcolor, border_width, border_width, -1, -1);
  }
  else if (!image && !title)  /* color only button */
  {
    int space = border_width + 2;
    iupFlatDrawBorder(dc, space, ih->currentwidth - 1 - space,
                              space, ih->currentheight - 1 - space,
                              1, "0 0 0", bgcolor, active);
    space++;
    iupFlatDrawBox(dc, space, ih->currentwidth - 1 - space,
                           space, ih->currentheight - 1 - space,
                           fgcolor, bgcolor, active);
  }


  if (ih->data->has_focus && focus_feedback)
  {
    border_width--;
    iupdrvDrawFocusRect(dc, border_width, border_width, ih->currentwidth - 1 - border_width, ih->currentheight - 1 - border_width);
  }

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static void iFlatButtonNotify(Ihandle* ih, int is_toggle)
{
  Icallback cb = IupGetCallback(ih, "FLAT_ACTION");
  if (cb)
  {
    /* to avoid double calls when a dialog is displayed */
    if (!ih->data->inside_action)
    {
      int ret;

      ih->data->inside_action = 1;

      ret = cb(ih);
      if (ret == IUP_CLOSE)
        IupExitLoop();

      if (ret != IUP_IGNORE && iupObjectCheck(ih))
        ih->data->inside_action = 0;
    }
  }

  if (is_toggle)
  {
    if (iupObjectCheck(ih))
      iupBaseCallValueChangedCb(ih);
  }
}

static int iFlatButtonButton_CB(Ihandle* ih, int button, int pressed, int x, int y, char* status)
{
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "FLAT_BUTTON_CB");
  if (cb)
  {
    if (cb(ih, button, pressed, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (button == IUP_BUTTON1)
  {
    if (iupAttribGetBoolean(ih, "TOGGLE"))
    {
      Ihandle* radio = iupRadioFindToggleParent(ih);
      Ihandle* last_tg = NULL;

      if (!pressed && ih->data->highlighted)  /* released inside the button area */
      {
        int selected = iupAttribGetInt(ih, "VALUE");
        if (selected)  /* was ON */
        {
          if (!radio)
            iupAttribSet(ih, "VALUE", "OFF");
          else
            last_tg = ih;  /* to avoid the callback call */
        }
        else  /* was OFF */
        {
          if (radio)
          {
            last_tg = (Ihandle*)iupAttribGet(radio, "_IUP_FLATBUTTON_LASTRADIO");
            if (iupObjectCheck(last_tg) && last_tg != ih)
            {
              iupAttribSet(last_tg, "VALUE", "OFF");
              iupdrvRedrawNow(last_tg);
            }
            else
              last_tg = NULL;

            iupAttribSet(radio, "_IUP_FLATBUTTON_LASTRADIO", (char*)ih);
          }

          iupAttribSet(ih, "VALUE", "ON");
        }
      }

      ih->data->pressed = pressed;
      iupdrvRedrawNow(ih);

      if (!pressed && ih->data->highlighted)  /* released inside the button area */
      {
        if (last_tg && ih != last_tg)
          iFlatButtonNotify(last_tg, 1);

        if (!radio || ih != last_tg)
          iFlatButtonNotify(ih, 1);
      }
    }
    else
    {
      ih->data->pressed = pressed;
      iupdrvRedrawNow(ih);

      if (!pressed && ih->data->highlighted)  /* released inside the button area */
        iFlatButtonNotify(ih, 0);
    }
  }

  return IUP_DEFAULT;
}

static int iFlatButtonActivate_CB(Ihandle* ih)
{
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

  iFlatButtonButton_CB(ih, IUP_BUTTON1, 1, 0, 0, status);

  iupdrvSleep(100);

  iFlatButtonButton_CB(ih, IUP_BUTTON1, 0, 0, 0, status);

  return IUP_DEFAULT;
}

static int iFlatButtonFocus_CB(Ihandle* ih, int focus)
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

static int iFlatButtonMotion_CB(Ihandle* ih, int x, int y, char* status)
{
  int redraw = 0;
  IFniis cb = (IFniis)IupGetCallback(ih, "FLAT_MOTION_CB");
  if (cb)
  {
    if (cb(ih, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (iup_isbutton1(status))
  {
    /* handle when mouse is pressed and moved to/from inside the canvas */
    if (x < 0 || x > ih->currentwidth - 1 ||
        y < 0 || y > ih->currentheight - 1)
    {
      if (ih->data->highlighted)
      {
        redraw = 1;
        ih->data->highlighted = 0;
      }
    }
    else
    {
      if (!ih->data->highlighted)
      {
        redraw = 1;
        ih->data->highlighted = 1;
      }
    }
  }

  if (redraw)
    iupdrvRedrawNow(ih);

  return IUP_DEFAULT;
}

static int iFlatButtonEnterWindow_CB(Ihandle* ih)
{
  IFn cb = (IFn)IupGetCallback(ih, "FLAT_ENTERWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  ih->data->highlighted = 1;
  iupdrvRedrawNow(ih);

  return IUP_DEFAULT;
}

static int iFlatButtonLeaveWindow_CB(Ihandle* ih)
{
  IFn cb = (IFn)IupGetCallback(ih, "FLAT_LEAVEWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  ih->data->highlighted = 0;
  iupdrvRedrawNow(ih);

  return IUP_DEFAULT;
}


/***********************************************************************************************/


static int iFlatButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char value1[30], value2[30];

  iupStrToStrStr(value, value1, value2, ':');

 ih->data->horiz_alignment = iupFlatGetHorizontalAlignment(value1);
 ih->data->vert_alignment = iupFlatGetVerticalAlignment(value2);

  if (ih->handle)
    iupdrvRedrawNow(ih);

  return 1;
}

static char* iFlatButtonGetAlignmentAttrib(Ihandle *ih)
{
  char* horiz_align2str[3] = {"ALEFT", "ACENTER", "ARIGHT"};
  char* vert_align2str[3] = {"ATOP", "ACENTER", "ABOTTOM"};
  return iupStrReturnStrf("%s:%s", horiz_align2str[ih->data->horiz_alignment], vert_align2str[ih->data->vert_alignment]);
}

static int iFlatButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
    iupdrvRedrawNow(ih);
  return 0;
}

static int iFlatButtonSetAttribPostRedraw(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
}

static char* iFlatButtonGetBgColorAttrib(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "BGCOLOR");
  if (!value)
    return iupBaseNativeParentGetBgColorAttrib(ih);
  else
    return value;
}

static char* iFlatButtonGetPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

static int iFlatButtonSetImagePositionAttrib(Ihandle* ih, const char* value)
{
  ih->data->img_position = iupFlatGetImagePosition(value);

  if (ih->handle)
    iupdrvRedrawNow(ih);

  return 0;
}

static char* iFlatButtonGetImagePositionAttrib(Ihandle *ih)
{
  char* img_pos2str[4] = {"LEFT", "RIGHT", "TOP", "BOTTOM"};
  return iupStrReturnStr(img_pos2str[ih->data->img_position]);
}

static int iFlatButtonSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);
  if (ih->handle)
    iupdrvRedrawNow(ih);
  return 0;
}

static char* iFlatButtonGetSpacingAttrib(Ihandle *ih)
{
  return iupStrReturnInt(ih->data->spacing);
}

static int iFlatButtonSetBorderWidthAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->border_width);
  if (ih->handle)
    iupdrvRedrawNow(ih);
  return 0;
}

static char* iFlatButtonGetBorderWidthAttrib(Ihandle *ih)
{
  return iupStrReturnInt(ih->data->border_width);
}

static int iFlatButtonSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "TOGGLE"))
  {
    Ihandle* radio = iupRadioFindToggleParent(ih);
    if (radio)
    {
      /* can only set Radio to ON */
      if (iupStrEqualNoCase(value, "TOGGLE") || iupStrBoolean(value))
      {
        Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUP_FLATBUTTON_LASTRADIO");
        if (iupObjectCheck(last_tg) && last_tg != ih)
        {
          iupAttribSet(last_tg, "VALUE", "OFF");
          if (last_tg->handle)
            iupdrvRedrawNow(last_tg);
        }

        iupAttribSet(radio, "_IUP_FLATBUTTON_LASTRADIO", (char*)ih);
      }
      else
        return 0;
    }
    else
    {
      if (iupStrEqualNoCase(value, "TOGGLE"))
      {
        int oldcheck = iupAttribGetBoolean(ih, "VALUE");
        if (oldcheck)
          iupAttribSet(ih, "VALUE", "OFF");
        else
          iupAttribSet(ih, "VALUE", "ON");

        if (ih->handle)
          iupdrvRedrawNow(ih);

        return 0;
      }
    }

    if (ih->handle)
      iupdrvPostRedraw(ih);

    return 1;
  }
  else
    return 0;
}

static char* iFlatButtonGetRadioAttrib(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "TOGGLE"))
  {
    Ihandle* radio = iupRadioFindToggleParent(ih);
    return iupStrReturnBoolean(radio != NULL);
  }
  else
    return NULL;
}

static char* iFlatButtonGetHighlightedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->highlighted);
}

static char* iFlatButtonGetPressedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->pressed);
}

static char* iFlatButtonGetHasFocusAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->has_focus);
}


/*****************************************************************************************/


static int iFlatButtonCreateMethod(Ihandle* ih, void** params)
{
  if (params && params[0])
  {
    iupAttribSetStr(ih, "TITLE", (char*)(params[0]));
  }
  
  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* change the IupCanvas default values */
  iupAttribSet(ih, "BORDER", "NO");
  ih->expand = IUP_EXPAND_NONE;

  /* non zero default values */
  ih->data->spacing = 2;
  ih->data->border_width = 1;
  ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
  ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  /* initial values - don't use default so they can be set to NULL */
  iupAttribSet(ih, "HLCOLOR", IUP_FLAT_HIGHCOLOR);
  iupAttribSet(ih, "PSCOLOR", IUP_FLAT_PRESSCOLOR);

  /* internal callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iFlatButtonRedraw_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iFlatButtonButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)iFlatButtonMotion_CB);
  IupSetCallback(ih, "FOCUS_CB", (Icallback)iFlatButtonFocus_CB);
  IupSetCallback(ih, "LEAVEWINDOW_CB", iFlatButtonLeaveWindow_CB);
  IupSetCallback(ih, "ENTERWINDOW_CB", iFlatButtonEnterWindow_CB);
  IupSetCallback(ih, "K_CR", (Icallback)iFlatButtonActivate_CB);
  IupSetCallback(ih, "K_SP", (Icallback)iFlatButtonActivate_CB);

  return IUP_NOERROR;
}

static int iFlatButtonMapMethod(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "TOGGLE"))
  {
    Ihandle* radio = iupRadioFindToggleParent(ih);
    if (radio)
    {
      if (!iupAttribGet(radio, "_IUP_FLATBUTTON_LASTRADIO"))
      {
        /* this is the first toggle in the radio, and then set it with VALUE=ON */
        iupAttribSet(ih, "VALUE", "ON");
      }

      /* make sure it has at least one name */
      if (!iupAttribGetHandleName(ih))
        iupAttribSetHandleName(ih);
    }
  }
  return IUP_NOERROR;
}

static void iFlatButtonComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int fit2backimage = iupAttribGetBoolean(ih, "FITTOBACKIMAGE");
  char* bgimage = iupAttribGet(ih, "BACKIMAGE");
  if (fit2backimage && bgimage)
    iupImageGetInfo(bgimage, w, h, NULL);
  else
  {
    char* imagename = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    double text_orientation = iupAttribGetDouble(ih, "TEXTORIENTATION");

    iupFlatDrawGetIconSize(ih, ih->data->img_position, ih->data->spacing, ih->data->horiz_padding, ih->data->vert_padding, imagename, title, w, h, text_orientation);
  }

  *w += 2 * ih->data->border_width;
  *h += 2 * ih->data->border_width;

  (void)children_expand; /* unset if not a container */
}


/******************************************************************************/


Iclass* iupFlatButtonNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "flatbutton";
  ic->cons = "FlatButton";
  ic->format = "s"; /* one string */
  ic->format_attr = "TITLE";
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupFlatButtonNewClass;
  ic->Create = iFlatButtonCreateMethod;
  ic->ComputeNaturalSize = iFlatButtonComputeNaturalSizeMethod;
  ic->Map = iFlatButtonMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "FLAT_ACTION", "");
  iupClassRegisterCallback(ic, "FLAT_BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "FLAT_MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "FLAT_FOCUS_CB", "i");
  iupClassRegisterCallback(ic, "FLAT_ENTERWINDOW_CB", "ii");
  iupClassRegisterCallback(ic, "FLAT_LEAVEWINDOW_CB", "");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupFlatSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", NULL, iFlatButtonSetAttribPostRedraw, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* IupFlatButton */
  iupClassRegisterAttribute(ic, "VALUE", NULL, iFlatButtonSetValueAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RADIO", iFlatButtonGetRadioAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", iFlatButtonGetAlignmentAttrib, iFlatButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iFlatButtonGetPaddingAttrib, iFlatButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CPADDING", iupBaseGetCPaddingAttrib, iupBaseSetCPaddingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SPACING", iFlatButtonGetSpacingAttrib, iFlatButtonSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "2", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CSPACING", iupBaseGetCSpacingAttrib, iupBaseSetCSpacingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "IGNORERADIO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIGHLIGHTED", iFlatButtonGetHighlightedAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRESSED", iFlatButtonGetPressedAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HASFOCUS", iFlatButtonGetHasFocusAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWBORDER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSFEEDBACK", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BORDERCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, IUP_FLAT_BORDERCOLOR, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "BORDERPSCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "BORDERHLCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "BORDERWIDTH", iFlatButtonGetBorderWidthAttrib, iFlatButtonSetBorderWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, "DLGFGCOLOR", NULL, IUPAF_NOT_MAPPED);  /* force the new default value */
  iupClassRegisterAttribute(ic, "BGCOLOR", iFlatButtonGetBgColorAttrib, iFlatButtonSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE | IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "HLCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "PSCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "TEXTHLCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "TEXTPSCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */

  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", iFlatButtonGetImagePositionAttrib, iFlatButtonSetImagePositionAttrib, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTALIGNMENT", NULL, NULL, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTWRAP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTELLIPSIS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTCLIP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTORIENTATION", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKIMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITTOBACKIMAGE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FRONTIMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRONTIMAGEPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRONTIMAGEHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRONTIMAGEINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupFlatButton(const char* title)
{
  void *params[2];
  params[0] = (void*)title;
  params[1] = NULL;
  return IupCreatev("flatbutton", params);
}

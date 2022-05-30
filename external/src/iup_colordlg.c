/** \file
 * \brief IupColorDlg pre-defined dialog control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"
#include "iupcontrols.h"

#include "iupdraw.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_strmessage.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_image.h"
#include "iup_colorhsi.h"
#include "iup_childtree.h"

       
#define COLORTABLE_MAX 20

static const char* default_colortable_cells[COLORTABLE_MAX] =
{
  "0 0 0", "64 64 64", "128 128 128", "144 144 144", "0 128 128", "128 0 128", "128 128 0", "128 0 0", "0 128 0", "0 0 128",
  "255 255 255", "240 240 240", "224 224 224", "192 192 192", "0 255 255", "255 0 255", "255 255 0", "255 0 0", "0 255 0", "0 0 255"
};

typedef struct _IcolorDlgData
{
  int status;

  long previous_color, 
       color; /* same as (red,green,blue,alpha) */

  double hue, saturation, intensity;
  unsigned char red, green, blue, alpha;

  Ihandle *red_txt, *green_txt, *blue_txt, *alpha_txt;
  Ihandle *hue_txt, *intensity_txt, *saturation_txt;
  Ihandle *color_browser, *color_cnv, *colorhex_txt;
  Ihandle *colortable_cbar, *alpha_val;
  Ihandle *help_bt;
} IcolorDlgData;


static void iColorDlgHSI2RGB(IcolorDlgData* colordlg_data)
{
  iupColorHSI2RGB(colordlg_data->hue, colordlg_data->saturation, colordlg_data->intensity,
                  &colordlg_data->red, &colordlg_data->green, &colordlg_data->blue);
}

static void iColorDlgRGB2HSI(IcolorDlgData* colordlg_data)
{
  iupColorRGB2HSI(colordlg_data->red, colordlg_data->green, colordlg_data->blue,
                  &(colordlg_data->hue), &(colordlg_data->saturation), &(colordlg_data->intensity));
}

static void iColorDlgHex_TXT_Update(IcolorDlgData* colordlg_data)
{
  IupSetfAttribute(colordlg_data->colorhex_txt, "VALUE", "#%02X%02X%02X", (int)colordlg_data->red, (int)colordlg_data->green, (int)colordlg_data->blue);
}

/*************************************************\
* Updates text fields with the current HSI values *
\*************************************************/
static void iColorDlgHSI_TXT_Update(IcolorDlgData* colordlg_data)
{
  IupSetInt(colordlg_data->hue_txt, "VALUE", iupROUND(colordlg_data->hue));
  IupSetInt(colordlg_data->saturation_txt, "VALUE", iupRound(colordlg_data->saturation * 100));
  IupSetInt(colordlg_data->intensity_txt, "VALUE", iupRound(colordlg_data->intensity * 100));
}

/*************************************************\
* Updates text fields with the current RGB values *
\*************************************************/
static void iColorDlgRGB_TXT_Update(IcolorDlgData* colordlg_data)
{
  IupSetInt(colordlg_data->red_txt, "VALUE", (int) colordlg_data->red);
  IupSetInt(colordlg_data->green_txt, "VALUE", (int) colordlg_data->green);
  IupSetInt(colordlg_data->blue_txt, "VALUE", (int) colordlg_data->blue);
}

static void iColorDlgBrowserRGB_Update(IcolorDlgData* colordlg_data)
{
  IupSetfAttribute(colordlg_data->color_browser, "RGB", "%d %d %d", colordlg_data->red, colordlg_data->green, colordlg_data->blue);
}

static void iColorDlgBrowserHSI_Update(IcolorDlgData* colordlg_data)
{
  IupSetfAttribute(colordlg_data->color_browser, "HSI", IUP_DOUBLE2STR" "IUP_DOUBLE2STR" "IUP_DOUBLE2STR, colordlg_data->hue, colordlg_data->saturation, colordlg_data->intensity);
}

/*****************************************\
* Sets the RGB color in the Color Canvas *
\*****************************************/
static void iColorDlgColor_Update(IcolorDlgData* colordlg_data)
{
  colordlg_data->color = iupDrawColor(colordlg_data->red, colordlg_data->green, colordlg_data->blue, colordlg_data->alpha);
  IupUpdate(colordlg_data->color_cnv);

  {
    Ihandle* ih = IupGetDialog(colordlg_data->color_browser);
    Icallback cb = IupGetCallback(ih, "COLORUPDATE_CB");
    if (cb) cb(ih);
  }
}

static void iColorDlgHSIChanged(IcolorDlgData* colordlg_data) 
{
  iColorDlgHSI2RGB(colordlg_data);
  iColorDlgBrowserHSI_Update(colordlg_data);
  iColorDlgHex_TXT_Update(colordlg_data);
  iColorDlgRGB_TXT_Update(colordlg_data);
  iColorDlgColor_Update(colordlg_data);
}

static void iColorDlgRGBChanged(IcolorDlgData* colordlg_data) 
{
  iColorDlgRGB2HSI(colordlg_data);
  iColorDlgBrowserRGB_Update(colordlg_data);
  iColorDlgHex_TXT_Update(colordlg_data);
  iColorDlgHSI_TXT_Update(colordlg_data);
  iColorDlgColor_Update(colordlg_data);
}

/***********************************************\
* Initializes the default values to the element *
\***********************************************/
static void iColorDlgInit_Defaults(IcolorDlgData* colordlg_data)
{
  Ihandle* box;
  int i;

  IupSetAttribute(colordlg_data->color_browser, "RGB", "0 0 0");

  IupSetAttribute(colordlg_data->red_txt,   "VALUE", "0");
  IupSetAttribute(colordlg_data->green_txt, "VALUE", "0");
  IupSetAttribute(colordlg_data->blue_txt,  "VALUE", "0");

  IupSetAttribute(colordlg_data->hue_txt,        "VALUE", "0");
  IupSetAttribute(colordlg_data->saturation_txt, "VALUE", "0");
  IupSetAttribute(colordlg_data->intensity_txt,  "VALUE", "0");
  
  IupSetAttribute(colordlg_data->colorhex_txt, "VALUE", "#000000");

  colordlg_data->alpha = 255;
  IupSetAttribute(colordlg_data->alpha_val, "VALUE", "255");
  IupSetAttribute(colordlg_data->alpha_txt, "VALUE", "255");

  box = IupGetParent(colordlg_data->alpha_val);
  IupSetAttribute(box, "FLOATING", "YES");
  IupSetAttribute(box, "VISIBLE", "NO");

  box = IupGetParent(colordlg_data->colortable_cbar);
  IupSetAttribute(box, "FLOATING", "YES");
  IupSetAttribute(box, "VISIBLE", "NO");

  box = IupGetParent(colordlg_data->colorhex_txt);
  IupSetAttribute(box, "FLOATING", "YES");
  IupSetAttribute(box, "VISIBLE", "NO");

  for (i = 0; i < COLORTABLE_MAX; i++)
    IupSetAttributeId(colordlg_data->colortable_cbar, "CELL", i, default_colortable_cells[i]);
}


/**************************************************************************************************************/
/*                                 Internal Callbacks                                                         */
/**************************************************************************************************************/


static int iColorDlgButtonOK_CB(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  colordlg_data->status = 1;
  return IUP_CLOSE;
}

static int iColorDlgButtonCancel_CB(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  colordlg_data->status = 0;
  return IUP_CLOSE;
}

static int iColorDlgButtonHelp_CB(Ihandle* ih)
{
  Icallback cb = IupGetCallback(IupGetDialog(ih), "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
  {
    IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
    colordlg_data->status = 0;
    return IUP_CLOSE;
  }
  return IUP_DEFAULT;
}

static void iColorDrawTransparentRectangle(Ihandle* color_cnv, int xmin, int ymin, int xmax, int ymax, long color)
{
  if (iupDrawAlpha(color) == 255)
  {
    iupDrawSetColor(color_cnv, "DRAWCOLOR", color);
    IupDrawRectangle(color_cnv, xmin, ymin, xmax, ymax);
  }
  else
  {
    int w = xmax - xmin + 1;
    int h = ymax - ymin + 1;
    Ihandle* image = IupImageRGBA(w,h,NULL);
    unsigned char *colors = (unsigned char*)iupAttribGet(image, "WID");
    int x, y;
    unsigned char red = iupDrawRed(color);
    unsigned char green = iupDrawGreen(color);
    unsigned char blue = iupDrawBlue(color);
    unsigned char alpha = iupDrawAlpha(color);

    for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
      {
        colors[0] = red;
        colors[1] = green;
        colors[2] = blue;
        colors[3] = alpha;

        colors += 4;
      }
    }

    iupAttribSetHandleName(image);
    IupDrawImage(color_cnv, iupAttribGetHandleName(image), xmin, ymin, -1, -1);

    IupDestroy(image);
  }
}

static int iColorDlgColorCnvAction_CB(Ihandle* color_cnv)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(color_cnv, "_IUP_GC_DATA");

  int x, y, w, h, width, height, box_size = 10;

  IupDrawBegin(color_cnv);

  IupDrawGetSize(color_cnv, &width, &height);

  iupDrawSetColor(color_cnv, "DRAWCOLOR", iupDrawColor(255, 255, 255, 255));
  iupAttribSet(color_cnv, "DRAWSTYLE", "FILL");
  IupDrawRectangle(color_cnv, 0, 0, width - 1, height - 1);

  w = (width + box_size - 1) / box_size;
  h = (height + box_size - 1) / box_size;

  iupDrawSetColor(color_cnv, "DRAWCOLOR", iupDrawColor(192, 192, 192, 255));

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      if (((x % 2) && (y % 2)) || (((x + 1) % 2) && ((y + 1) % 2)))
      {
        int xmin = x*box_size;
        int xmax = xmin + box_size;
        int ymin = y*box_size;
        int ymax = ymin + box_size;

        IupDrawRectangle(color_cnv, xmin, ymin, xmax, ymax);
      }
    }
  }

  iColorDrawTransparentRectangle(color_cnv, 0, 0, width / 2, height - 1, colordlg_data->previous_color);

  iColorDrawTransparentRectangle(color_cnv, width / 2 + 1, 0, width - 1, height - 1, colordlg_data->color);

  IupDrawEnd(color_cnv);

  return IUP_DEFAULT;
}

static int iColorDlgRedAction_CB(Ihandle* ih, int c, char *value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int vi;

  if (iupStrToInt(value, &vi))
  {
    colordlg_data->red = (unsigned char)vi;
    iColorDlgRGBChanged(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgRedSpin_CB(Ihandle* ih, int vi) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->red = (unsigned char)vi;
  iColorDlgRGBChanged(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgGreenAction_CB(Ihandle* ih, int c, char *value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int vi;

  if (iupStrToInt(value, &vi))
  {
    colordlg_data->green = (unsigned char)vi;
    iColorDlgRGBChanged(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgGreenSpin_CB(Ihandle* ih, int vi) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->green = (unsigned char)vi;
  iColorDlgRGBChanged(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgBlueAction_CB(Ihandle* ih, int c, char *value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int vi;

  if (iupStrToInt(value, &vi))
  {
    colordlg_data->blue = (unsigned char)vi;
    iColorDlgRGBChanged(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgBlueSpin_CB(Ihandle* ih, int vi) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->blue = (unsigned char)vi;
  iColorDlgRGBChanged(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgHueAction_CB(Ihandle* ih, int c, char *value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int vi;

  if (iupStrToInt(value, &vi))
  {
    colordlg_data->hue = (double)vi;
    iColorDlgHSIChanged(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgHueSpin_CB(Ihandle* ih, int vi) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->hue = (double)vi;
  iColorDlgHSIChanged(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgSaturationAction_CB(Ihandle* ih, int c, char *value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int vi;

  if (iupStrToInt(value, &vi))
  {
    colordlg_data->saturation = (double)vi/100.0;
    iColorDlgHSIChanged(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgSaturationSpin_CB(Ihandle* ih, int vi) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->saturation = (double)vi/100.0;
  iColorDlgHSIChanged(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgIntensityAction_CB(Ihandle* ih, int c, char *value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int vi;

  if (iupStrToInt(value, &vi))
  {
    colordlg_data->intensity = (double)vi/100.0;
    iColorDlgHSIChanged(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgIntensitySpin_CB(Ihandle* ih, int vi) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->intensity = (double)vi/100.0;
  iColorDlgHSIChanged(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgHexAction_CB(Ihandle* ih, int c, char* value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  if (iupStrToRGB(value, &(colordlg_data->red), &(colordlg_data->green), &(colordlg_data->blue)))
  {
    iColorDlgRGB2HSI(colordlg_data);
    iColorDlgBrowserRGB_Update(colordlg_data);
    iColorDlgHSI_TXT_Update(colordlg_data);
    iColorDlgRGB_TXT_Update(colordlg_data);
    iColorDlgColor_Update(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgColorSelDrag_CB(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->red   = r;
  colordlg_data->green = g;
  colordlg_data->blue  = b;

  iColorDlgRGB2HSI(colordlg_data);
  iColorDlgHex_TXT_Update(colordlg_data);
  iColorDlgHSI_TXT_Update(colordlg_data);
  iColorDlgRGB_TXT_Update(colordlg_data);

  iColorDlgColor_Update(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgAlphaVal_CB(Ihandle* ih, double val)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->alpha = (unsigned char)val;
  IupSetInt(colordlg_data->alpha_txt, "VALUE", (int)colordlg_data->alpha);

  iColorDlgColor_Update(colordlg_data);

  return IUP_DEFAULT;  
}

static int iColorDlgAlphaAction_CB(Ihandle* ih, int c, char* value) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int vi;

  if (iupStrToInt(value, &vi))
  {
    colordlg_data->alpha = (unsigned char)vi;
    IupSetInt(colordlg_data->alpha_val, "VALUE", (int)colordlg_data->alpha);

    iColorDlgColor_Update(colordlg_data);
  }

  (void)c;
  return IUP_DEFAULT;
}

static int iColorDlgAlphaSpin_CB(Ihandle* ih, int vi) 
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  colordlg_data->alpha = (unsigned char)vi;
  IupSetInt(colordlg_data->alpha_val, "VALUE", (int)colordlg_data->alpha);

  iColorDlgColor_Update(colordlg_data);

  return IUP_DEFAULT;
}

static int iColorDlgColorTableSelect_CB(Ihandle* ih, int cell, int type)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  iupStrToRGB(IupGetAttributeId(ih, "CELL", cell), &colordlg_data->red, &colordlg_data->green, &colordlg_data->blue);

  iColorDlgRGB_TXT_Update(colordlg_data);
  iColorDlgRGBChanged(colordlg_data);

  (void)type;
  return IUP_DEFAULT;
}

static int iColorDlgColorCnvButton_CB(Ihandle* ih, int b, int press, int x, int y)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int width;
  (void)y;

  if (b != IUP_BUTTON1 || !press)
    return IUP_DEFAULT;

  width = colordlg_data->color_cnv->currentwidth;

  if (x < width/2)
  {
    /* reset color to previous */
    colordlg_data->red = iupDrawRed(colordlg_data->previous_color);
    colordlg_data->green = iupDrawGreen(colordlg_data->previous_color);
    colordlg_data->blue = iupDrawBlue(colordlg_data->previous_color);
    colordlg_data->alpha = iupDrawAlpha(colordlg_data->previous_color);

    IupSetInt(colordlg_data->alpha_txt, "VALUE", (int)colordlg_data->alpha);
    IupSetInt(colordlg_data->alpha_val, "VALUE", (int)colordlg_data->alpha);

    iColorDlgRGB_TXT_Update(colordlg_data);
    iColorDlgRGBChanged(colordlg_data);
  }

  return IUP_DEFAULT;
}


/**************************************************************************************************************/
/*                                     Attributes                                                             */
/**************************************************************************************************************/


static char* iColorDlgGetStatusAttrib(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  if (colordlg_data->status) 
    return "1";
  else 
    return NULL;
}

static int iColorDlgSetShowHelpAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  IupSetAttribute(colordlg_data->help_bt, "VISIBLE", iupStrBoolean(value)? "YES": "NO");
  return 1;
}

static int iColorDlgSetShowHexAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  /* valid only before map */
  if (ih->handle)
    return 1;

  if (iupStrBoolean(value))
  {
    Ihandle* box = IupGetParent(colordlg_data->colorhex_txt);
    IupSetAttribute(box, "FLOATING", NULL);
    IupSetAttribute(box, "VISIBLE", "YES");
  }

  return 1;
}

static int iColorDlgSetShowColorTableAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  /* valid only before map */
  if (ih->handle)
    return 1;

  if (iupStrBoolean(value))
  {
    Ihandle* box = IupGetParent(colordlg_data->colortable_cbar);
    IupSetAttribute(box, "FLOATING", NULL);
    IupSetAttribute(box, "VISIBLE", "YES");
  }

  return 1;
}

static int iColorDlgSetShowAlphaAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  /* valid only before map */
  if (ih->handle)
    return 1;

  if (iupStrBoolean(value))
  {
    Ihandle* box = IupGetParent(colordlg_data->alpha_val);
    IupSetAttribute(box, "FLOATING", NULL);
    IupSetAttribute(box, "VISIBLE", "YES");
  }

  return 1;
}

static int iColorDlgSetAlphaAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int alpha;
  if (iupStrToInt(value, &alpha))
  {
    colordlg_data->alpha = (unsigned char)alpha;
    IupSetInt(colordlg_data->alpha_txt, "VALUE", (int)colordlg_data->alpha);
    IupSetInt(colordlg_data->alpha_val, "VALUE", (int)colordlg_data->alpha);

    colordlg_data->previous_color = iupDrawColor(iupDrawRed(colordlg_data->previous_color), iupDrawGreen(colordlg_data->previous_color), iupDrawBlue(colordlg_data->previous_color), colordlg_data->alpha);

    iColorDlgColor_Update(colordlg_data);

    if (!ih->handle)  /* do it only before map */
      IupSetAttribute(ih, "SHOWALPHA", "YES");
  }
 
  return 1;
}

static char* iColorDlgGetAlphaAttrib(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  return iupStrReturnInt((int)colordlg_data->alpha);
}

static int iStrToRGBA(const char *str, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
  unsigned int ri = 0, gi = 0, bi = 0, ai = 0, ret;
  if (!str) return 0;
  if (str[0]=='#')
  {
    str++;
    ret = sscanf(str, "%2X%2X%2X%2X", &ri, &gi, &bi, &ai);
  }
  else
    ret = sscanf(str, "%u %u %u %u", &ri, &gi, &bi, &ai);

  if (ret < 3) return 0;
  if (ri > 255 || gi > 255 || bi > 255 || ai > 255) return 0;
  *r = (unsigned char)ri;
  *g = (unsigned char)gi;
  *b = (unsigned char)bi;
  if (ret == 4)
  {
    *a = (unsigned char)ai;
    return 4;
  }
  else
    return 3;
}

static int iColorDlgSetValueAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int ret = iStrToRGBA(value, &colordlg_data->red, &colordlg_data->green, &colordlg_data->blue, &colordlg_data->alpha);
  if (!ret)
    return 0;
  
  colordlg_data->previous_color = iupDrawColor(colordlg_data->red, colordlg_data->green, colordlg_data->blue, colordlg_data->alpha);

  if (ret == 4)
  {
    IupSetInt(colordlg_data->alpha_txt, "VALUE", (int)colordlg_data->alpha);
    IupSetInt(colordlg_data->alpha_val, "VALUE", (int)colordlg_data->alpha);

    if (!ih->handle)  /* do it only before map */
      IupSetAttribute(ih, "SHOWALPHA", "YES");
  }

  iColorDlgRGB_TXT_Update(colordlg_data);
  iColorDlgRGBChanged(colordlg_data);

  return 0;
}

static char* iColorDlgGetValueAttrib(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  if (iupAttribGetBoolean(ih, "SHOWALPHA"))
    return iupStrReturnStrf("%d %d %d %d", (int)colordlg_data->red, (int)colordlg_data->green, (int)colordlg_data->blue, (int)colordlg_data->alpha);
  else
    return iupStrReturnRGB(colordlg_data->red, colordlg_data->green, colordlg_data->blue);
}

static int iupStrToHSI_Int(const char *str, int *h, int *s, int *i)
{
  int fh, fs, fi;
  if (!str) return 0;
  if (sscanf(str, "%d %d %d", &fh, &fs, &fi) != 3) return 0;
  if (fh > 359 || fs > 100 || fi > 100) return 0;
  if (fh < 0 || fs < 0 || fi < 0) return 0;
  *h = fh;
  *s = fs;
  *i = fi;
  return 1;
}

static int iColorDlgSetValueHSIAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  int hue, saturation, intensity;

  if (!iupStrToHSI_Int(value, &hue, &saturation, &intensity))
    return 0;
  
  colordlg_data->hue = (double)hue;
  colordlg_data->saturation = (double)saturation/100.0;
  colordlg_data->intensity = (double)intensity/100.0;

  iColorDlgHSI2RGB(colordlg_data);
  colordlg_data->previous_color = iupDrawColor(colordlg_data->red, colordlg_data->green, colordlg_data->blue, colordlg_data->alpha);
 
  iColorDlgHSIChanged(colordlg_data);
  return 0;
}

static char* iColorDlgGetValueHSIAttrib(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  return iupStrReturnStrf("%d %d %d",(int)colordlg_data->hue, (int)(colordlg_data->saturation*100), (int)(colordlg_data->intensity*100));
}

static int iColorDlgSetValueHexAttrib(Ihandle* ih, const char* value)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  if (!iupStrToRGB(value, &(colordlg_data->red), &(colordlg_data->green), &(colordlg_data->blue)))
    return 0;

  colordlg_data->previous_color = iupDrawColor(colordlg_data->red, colordlg_data->green, colordlg_data->blue, colordlg_data->alpha);

  iColorDlgRGB2HSI(colordlg_data);
  iColorDlgBrowserRGB_Update(colordlg_data);
  iColorDlgHSI_TXT_Update(colordlg_data);
  iColorDlgRGB_TXT_Update(colordlg_data);
  iColorDlgColor_Update(colordlg_data);
  return 0;
}

static char* iColorDlgGetValueHexAttrib(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  return iupStrReturnStrf("#%02X%02X%02X", (int)colordlg_data->red, (int)colordlg_data->green, (int)colordlg_data->blue);
}

static char* iColorDlgGetColorTableAttrib(Ihandle* ih)
{
  int i, inc, off = 0;
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  char* color_str;
  char* str = iupStrGetMemory(COLORTABLE_MAX * 3 * 20);
  for (i = 0; i < COLORTABLE_MAX; i++)
  {
    color_str = IupGetAttributeId(colordlg_data->colortable_cbar, "CELL", i);
    inc = (int)strlen(color_str);
    memcpy(str+off, color_str, inc);
    memcpy(str+off+inc, ";", 1);
    off += inc+1;
  }
  str[off-1] = 0; /* remove last separator */
  return str;
}

static int iColorDlgSetColorTableAttrib(Ihandle* ih, const char* value)
{
  int i = 0;
  unsigned char r, g, b;
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  if (!ih->handle)    /* do it only before map */
    iColorDlgSetShowColorTableAttrib(ih, "YES");

  while (value && *value && i < COLORTABLE_MAX)
  {
    if (iupStrToRGB(value, &r, &g, &b))
      IupSetRGBId(colordlg_data->colortable_cbar, "CELL", i, r, g, b);

    value = strchr(value, ';');
    if (value) value++;
    i++;
  }

  return 1;
}


/**************************************************************************************************************/
/*                                     Methods                                                                */
/**************************************************************************************************************/

static int iColorDlgMapMethod(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");

  if (!IupGetCallback(ih, "HELP_CB"))
    IupSetAttribute(colordlg_data->help_bt, "VISIBLE", "NO");

  return IUP_NOERROR;
}

static void iColorDlgDestroyMethod(Ihandle* ih)
{
  IcolorDlgData* colordlg_data = (IcolorDlgData*)iupAttribGetInherit(ih, "_IUP_GC_DATA");
  free(colordlg_data);
}

static int iColorDlgCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *ok_bt, *cancel_bt;
  Ihandle *rgb_vb, *hsi_vb, *clr_vb;
  Ihandle *lin1, *lin2, *col1, *col2;

  IcolorDlgData* colordlg_data = (IcolorDlgData*)malloc(sizeof(IcolorDlgData));
  memset(colordlg_data, 0, sizeof(IcolorDlgData));
  iupAttribSet(ih, "_IUP_GC_DATA", (char*)colordlg_data);

  /* ======================================================================= */
  /* BUTTONS   ============================================================= */
  /* ======================================================================= */
  ok_bt = IupButton("_@IUP_OK", NULL);                      /* Ok Button */
  IupSetStrAttribute(ok_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback (ok_bt, "ACTION", (Icallback)iColorDlgButtonOK_CB);
  IupSetAttributeHandle(ih, "DEFAULTENTER", ok_bt);

  cancel_bt = IupButton("_@IUP_CANCEL", NULL);          /* Cancel Button */
  IupSetStrAttribute(cancel_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback (cancel_bt, "ACTION", (Icallback)iColorDlgButtonCancel_CB);
  IupSetAttributeHandle(ih, "DEFAULTESC", cancel_bt);

  colordlg_data->help_bt = IupButton("_@IUP_HELP", NULL);            /* Help Button */
  IupSetStrAttribute(colordlg_data->help_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback (colordlg_data->help_bt, "ACTION", (Icallback)iColorDlgButtonHelp_CB);

  /* ======================================================================= */
  /* COLOR   =============================================================== */
  /* ======================================================================= */
  colordlg_data->color_browser = IupColorBrowser();
  IupSetAttribute(colordlg_data->color_browser, "EXPAND", "YES");  
  IupSetCallback(colordlg_data->color_browser, "DRAG_CB",   (Icallback)iColorDlgColorSelDrag_CB);
  IupSetCallback(colordlg_data->color_browser, "CHANGE_CB", (Icallback)iColorDlgColorSelDrag_CB);

  colordlg_data->color_cnv = IupCanvas(NULL);  /* Canvas of the color */
  IupSetAttribute(colordlg_data->color_cnv, "SIZE", "x12");
  IupSetAttribute(colordlg_data->color_cnv, "CANFOCUS", "NO");
  IupSetAttribute(colordlg_data->color_cnv, "EXPAND", "HORIZONTAL");
  IupSetCallback (colordlg_data->color_cnv, "ACTION", (Icallback)iColorDlgColorCnvAction_CB);
  IupSetCallback (colordlg_data->color_cnv, "BUTTON_CB", (Icallback)iColorDlgColorCnvButton_CB);

  colordlg_data->colorhex_txt = IupText(NULL);      /* Hex of the color */
  IupSetAttribute(colordlg_data->colorhex_txt, "VISIBLECOLUMNS", "7");
  IupSetCallback (colordlg_data->colorhex_txt, "ACTION", (Icallback)iColorDlgHexAction_CB);
  IupSetAttribute(colordlg_data->colorhex_txt, "MASK", "#[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]");

  /* ======================================================================= */
  /* ALPHA TRANSPARENCY   ================================================== */
  /* ======================================================================= */
  colordlg_data->alpha_val = IupVal("HORIZONTAL");
  IupSetAttribute(colordlg_data->alpha_val, "EXPAND", "HORIZONTAL");
  IupSetAttribute(colordlg_data->alpha_val, "MIN", "0");
  IupSetAttribute(colordlg_data->alpha_val, "MAX", "255");
  IupSetAttribute(colordlg_data->alpha_val, "VALUE", "255");
  IupSetAttribute(colordlg_data->alpha_val, "SIZE", "80x12");
  IupSetCallback (colordlg_data->alpha_val, "MOUSEMOVE_CB", (Icallback)iColorDlgAlphaVal_CB);
  IupSetCallback (colordlg_data->alpha_val, "BUTTON_PRESS_CB", (Icallback)iColorDlgAlphaVal_CB);
  IupSetCallback (colordlg_data->alpha_val, "BUTTON_RELEASE_CB", (Icallback)iColorDlgAlphaVal_CB);

  colordlg_data->alpha_txt = IupText(NULL);                        /* Alpha value */
  IupSetAttribute(colordlg_data->alpha_txt, "VISIBLECOLUMNS", "3");
  IupSetAttribute(colordlg_data->alpha_txt, "SPIN", "YES");
  IupSetAttribute(colordlg_data->alpha_txt, "SPINMIN", "0");
  IupSetAttribute(colordlg_data->alpha_txt, "SPINMAX", "255");
  IupSetAttribute(colordlg_data->alpha_txt, "SPININC", "1");
  IupSetCallback (colordlg_data->alpha_txt, "ACTION", (Icallback)iColorDlgAlphaAction_CB);
  IupSetCallback (colordlg_data->alpha_txt, "SPIN_CB", (Icallback)iColorDlgAlphaSpin_CB);
  IupSetAttribute(colordlg_data->alpha_txt, "MASKINT", "0:255");

  /* ======================================================================= */
  /* COLOR TABLE   ========================================================= */
  /* ======================================================================= */
  colordlg_data->colortable_cbar = IupColorbar();
  IupSetAttribute(colordlg_data->colortable_cbar, "ORIENTATION", "HORIZONTAL");
  IupSetAttribute(colordlg_data->colortable_cbar, "NUM_PARTS", "2");  
  IupSetInt(colordlg_data->colortable_cbar, "NUM_CELLS", COLORTABLE_MAX);
  IupSetAttribute(colordlg_data->colortable_cbar, "SHOW_PREVIEW", "NO");
  IupSetAttribute(colordlg_data->colortable_cbar, "SIZE", "138x22");
  IupSetAttribute(colordlg_data->colortable_cbar, "SQUARED", "NO");
  IupSetCallback (colordlg_data->colortable_cbar, "SELECT_CB",   (Icallback)iColorDlgColorTableSelect_CB);

  /* ======================================================================= */
  /* RGB TEXT FIELDS   ===================================================== */
  /* ======================================================================= */
  colordlg_data->red_txt = IupText(NULL);                            /* Red value */
  IupSetAttribute(colordlg_data->red_txt, "VISIBLECOLUMNS", "3");
  IupSetAttribute(colordlg_data->red_txt, "SPIN", "YES");
  IupSetAttribute(colordlg_data->red_txt, "SPINMIN", "0");
  IupSetAttribute(colordlg_data->red_txt, "SPINMAX", "255");
  IupSetAttribute(colordlg_data->red_txt, "SPININC", "1");
  IupSetCallback (colordlg_data->red_txt, "ACTION", (Icallback)iColorDlgRedAction_CB);
  IupSetCallback (colordlg_data->red_txt, "SPIN_CB", (Icallback)iColorDlgRedSpin_CB);
  IupSetAttribute(colordlg_data->red_txt, "MASKINT", "0:255");

  colordlg_data->green_txt = IupText(NULL);                        /* Green value */
  IupSetAttribute(colordlg_data->green_txt, "VISIBLECOLUMNS", "3");
  IupSetAttribute(colordlg_data->green_txt, "SPIN", "YES");
  IupSetAttribute(colordlg_data->green_txt, "SPINMIN", "0");
  IupSetAttribute(colordlg_data->green_txt, "SPINMAX", "255");
  IupSetAttribute(colordlg_data->green_txt, "SPININC", "1");
  IupSetCallback (colordlg_data->green_txt, "ACTION", (Icallback)iColorDlgGreenAction_CB);
  IupSetCallback (colordlg_data->green_txt, "SPIN_CB", (Icallback)iColorDlgGreenSpin_CB);
  IupSetAttribute(colordlg_data->green_txt, "MASKINT", "0:255");

  colordlg_data->blue_txt = IupText(NULL);                          /* Blue value */
  IupSetAttribute(colordlg_data->blue_txt, "VISIBLECOLUMNS", "3");
  IupSetAttribute(colordlg_data->blue_txt, "SPIN", "YES");
  IupSetAttribute(colordlg_data->blue_txt, "SPINMIN", "0");
  IupSetAttribute(colordlg_data->blue_txt, "SPINMAX", "255");
  IupSetAttribute(colordlg_data->blue_txt, "SPININC", "1");
  IupSetCallback (colordlg_data->blue_txt, "ACTION", (Icallback)iColorDlgBlueAction_CB);
  IupSetCallback (colordlg_data->blue_txt, "SPIN_CB", (Icallback)iColorDlgBlueSpin_CB);
  IupSetAttribute(colordlg_data->blue_txt, "MASKINT", "0:255");

  /* ======================================================================= */
  /* HSI TEXT FIELDS   ===================================================== */
  /* ======================================================================= */
  colordlg_data->hue_txt = IupText(NULL);                            /* Hue value */
  IupSetAttribute(colordlg_data->hue_txt, "VISIBLECOLUMNS", "3");
  IupSetAttribute(colordlg_data->hue_txt, "SPIN", "YES");
  IupSetAttribute(colordlg_data->hue_txt, "SPINMIN", "0");
  IupSetAttribute(colordlg_data->hue_txt, "SPINMAX", "359");
  IupSetAttribute(colordlg_data->hue_txt, "SPINWRAP", "YES");
  IupSetAttribute(colordlg_data->hue_txt, "SPININC", "1");
  IupSetCallback(colordlg_data->hue_txt, "ACTION", (Icallback)iColorDlgHueAction_CB);
  IupSetCallback(colordlg_data->hue_txt, "SPIN_CB", (Icallback)iColorDlgHueSpin_CB);
  IupSetAttribute(colordlg_data->hue_txt, "MASKINT", "0:359");

  colordlg_data->saturation_txt = IupText(NULL);              /* Saturation value */
  IupSetAttribute(colordlg_data->saturation_txt, "VISIBLECOLUMNS", "3");
  IupSetAttribute(colordlg_data->saturation_txt, "SPIN", "YES");
  IupSetAttribute(colordlg_data->saturation_txt, "SPINMIN", "0");
  IupSetAttribute(colordlg_data->saturation_txt, "SPINMAX", "100");
  IupSetAttribute(colordlg_data->saturation_txt, "SPININC", "1");
  IupSetCallback(colordlg_data->saturation_txt, "ACTION", (Icallback)iColorDlgSaturationAction_CB);
  IupSetCallback(colordlg_data->saturation_txt, "SPIN_CB", (Icallback)iColorDlgSaturationSpin_CB);
  IupSetAttribute(colordlg_data->saturation_txt, "MASKINT", "0:100");

  colordlg_data->intensity_txt = IupText(NULL);                /* Intensity value */
  IupSetAttribute(colordlg_data->intensity_txt, "VISIBLECOLUMNS", "3");
  IupSetAttribute(colordlg_data->intensity_txt, "SPIN", "YES");
  IupSetAttribute(colordlg_data->intensity_txt, "SPINMIN", "0");
  IupSetAttribute(colordlg_data->intensity_txt, "SPINMAX", "100");
  IupSetAttribute(colordlg_data->intensity_txt, "SPININC", "1");
  IupSetCallback(colordlg_data->intensity_txt, "ACTION", (Icallback)iColorDlgIntensityAction_CB);
  IupSetCallback(colordlg_data->intensity_txt, "SPIN_CB", (Icallback)iColorDlgIntensitySpin_CB);
  IupSetAttribute(colordlg_data->intensity_txt, "MASKINT", "0:100");

  /* =================== */
  /* 1st line = Controls */
  /* =================== */

  col1 = IupVbox(colordlg_data->color_browser, IupSetAttributes(IupHbox(colordlg_data->color_cnv, NULL), "MARGIN=30x0"),NULL);

  hsi_vb = IupVbox(IupSetAttributes(IupHbox(IupLabel("_@IUP_HUE"), 
                                            colordlg_data->hue_txt, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   IupSetAttributes(IupHbox(IupLabel("_@IUP_SATURATION"), 
                                            colordlg_data->saturation_txt, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   IupSetAttributes(IupHbox(IupLabel("_@IUP_INTENSITY"), 
                                            colordlg_data->intensity_txt, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   NULL);
  IupSetAttribute(hsi_vb, "GAP", "5");
  
  rgb_vb = IupVbox(IupSetAttributes(IupHbox(IupLabel("_@IUP_RED"), 
                                            colordlg_data->red_txt, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   IupSetAttributes(IupHbox(IupLabel("_@IUP_GREEN"), 
                                            colordlg_data->green_txt, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   IupSetAttributes(IupHbox(IupLabel("_@IUP_BLUE"), 
                                            colordlg_data->blue_txt, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   NULL);
  IupSetAttribute(rgb_vb, "GAP", "5");
  
  clr_vb = IupVbox(IupSetAttributes(IupHbox(IupLabel("_@IUP_OPACITY"), 
                                            colordlg_data->alpha_txt, colordlg_data->alpha_val, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   IupSetAttributes(IupHbox(IupLabel("He&xa:"), 
                                            colordlg_data->colorhex_txt, 
                                            NULL), "ALIGNMENT=ACENTER"),
                   IupSetAttributes(IupVbox(IupLabel("_@IUP_PALETTE"), 
                                            colordlg_data->colortable_cbar,
                                            NULL), "GAP=3"),
                   NULL);
  IupSetAttribute(clr_vb, "GAP", "5");
  IupSetAttribute(clr_vb, "EXPAND", "YES");

  IupDestroy(IupSetAttributes(IupNormalizer(IupGetChild(IupGetChild(hsi_vb, 0), 0),  /* Hue Label */
                                            IupGetChild(IupGetChild(hsi_vb, 1), 0),  /* Saturation Label */
                                            IupGetChild(IupGetChild(hsi_vb, 2), 0),  /* Intensity Label */
                                            IupGetChild(IupGetChild(clr_vb, 0), 0),  /* Opacity Label */
                                            IupGetChild(IupGetChild(clr_vb, 1), 0),  /* Hexa Label */
                                            NULL), "NORMALIZE=HORIZONTAL"));

  IupDestroy(IupSetAttributes(IupNormalizer(IupGetChild(IupGetChild(rgb_vb, 0), 0),  /* Red Label */
                                            IupGetChild(IupGetChild(rgb_vb, 1), 0),  /* Green Label */
                                            IupGetChild(IupGetChild(rgb_vb, 2), 0),  /* Blue Label */
                                            NULL), "NORMALIZE=HORIZONTAL"));

  col2 = IupVbox(IupSetAttributes(IupHbox(hsi_vb, IupFill(), rgb_vb, NULL), "EXPAND=YES"), 
                 IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"), 
                 clr_vb,
                 NULL);
  IupSetAttributes(col2, "EXPAND=NO, GAP=10");

  lin1 = IupHbox(col1, col2, NULL);
  IupSetAttribute(lin1, "GAP", "10");
  IupSetAttribute(lin1, "MARGIN", "0x0");

  /* ================== */
  /* 2nd line = Buttons */
  /* ================== */

  lin2 = IupHbox(IupFill(), ok_bt, cancel_bt, colordlg_data->help_bt, NULL);
  IupSetAttribute(lin2, "GAP", "5");
  IupSetAttribute(lin2, "MARGIN", "0x0");
  IupSetAttribute(lin2, "NORMALIZESIZE", "HORIZONTAL");

  /* Do not use IupAppend because we set childtype=IUP_CHILDNONE */
  iupChildTreeAppend(ih, IupSetAttributes(IupVbox(lin1, IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"), lin2, NULL), "MARGIN=10x10, GAP=10"));

  IupRefresh(ih);

  if (colordlg_data->color_browser->currentwidth < colordlg_data->color_browser->currentheight)
  {
    IupSetStrf(colordlg_data->color_browser, "RASTERSIZE", "%dx%d", colordlg_data->color_browser->currentheight, colordlg_data->color_browser->currentheight);
    IupSetAttribute(ih, "RASTERSIZE", NULL);
  }

  iColorDlgInit_Defaults(colordlg_data);

  (void)params;
  return IUP_NOERROR;
}

Iclass* iupColorDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->New = iupColorDlgNewClass;
  ic->Create = iColorDlgCreateMethod;
  ic->Destroy = iColorDlgDestroyMethod;
  ic->Map = iColorDlgMapMethod;

  ic->name = "colordlg";
  ic->cons = "ColorDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;
  ic->childtype = IUP_CHILDNONE;

  iupClassRegisterCallback(ic, "COLORUPDATE_CB", "");

  iupClassRegisterAttribute(ic, "COLORTABLE", iColorDlgGetColorTableAttrib, iColorDlgSetColorTableAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", iColorDlgGetStatusAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iColorDlgGetValueAttrib, iColorDlgSetValueAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALPHA", iColorDlgGetAlphaAttrib, iColorDlgSetAlphaAttrib, IUPAF_SAMEASSYSTEM, "255", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUEHSI", iColorDlgGetValueHSIAttrib, iColorDlgSetValueHSIAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUEHEX", iColorDlgGetValueHexAttrib, iColorDlgSetValueHexAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWALPHA", NULL, iColorDlgSetShowAlphaAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCOLORTABLE", NULL, iColorDlgSetShowColorTableAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWHEX", NULL, iColorDlgSetShowHexAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWHELP", NULL, iColorDlgSetShowHelpAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupColorDlg(void)
{
  return IupCreate("colordlg");
}

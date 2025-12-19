/** \file
 * \brief Motif Open/Close
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <stdint.h>    

#include <Xm/Xm.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_globalattrib.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


/* global variables */
Widget   iupmot_appshell = 0;
Display* iupmot_display = 0;
int      iupmot_screen = 0;
Visual*  iupmot_visual = 0;
XtAppContext iupmot_appcontext = 0;

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return iupmot_display;
}

void iupmotSetGlobalColorAttrib(Widget w, const char* xmname, const char* name)
{
  unsigned char r, g, b;
  Pixel color;

  XtVaGetValues(w, xmname, &color, NULL);
  iupmotColorGetRGB(color, &r, &g, &b);

  iupGlobalSetDefaultColorAttrib(name, r, g, b);
}

/* Set IUP global color from X resource database, with fallback to default */
static void iupmotSetGlobalColorFromXrm(const char* resource_name, const char* resource_class, const char* iup_name,
                                         unsigned char def_r, unsigned char def_g, unsigned char def_b)
{
  XrmDatabase db;
  XrmValue value;
  char* type = NULL;

  db = XrmGetDatabase(iupmot_display);
  if (db && XrmGetResource(db, resource_name, resource_class, &type, &value))
  {
    XColor xcolor;
    Colormap colormap = DefaultColormap(iupmot_display, iupmot_screen);

    if (XParseColor(iupmot_display, colormap, value.addr, &xcolor))
    {
      unsigned char r = xcolor.red >> 8;
      unsigned char g = xcolor.green >> 8;
      unsigned char b = xcolor.blue >> 8;
      iupGlobalSetDefaultColorAttrib(iup_name, r, g, b);
      return;
    }
  }

  /* Fallback to default values */
  iupGlobalSetDefaultColorAttrib(iup_name, def_r, def_g, def_b);
}

int iupdrvOpen(int *argc, char ***argv)
{
  IupSetGlobal("DRIVER", "Motif");

  setlocale(LC_ALL, "");

  XtToolkitInitialize();

  iupmot_appcontext = XtCreateApplicationContext();

  iupmot_display = XtOpenDisplay(iupmot_appcontext, NULL, NULL, "Iup", NULL, 0, argc, *argv);   
  if (!iupmot_display)
  {
    fprintf (stderr, "IUP error: cannot open display.\n");
    return IUP_ERROR;
  }

  iupmot_appshell = XtAppCreateShell(NULL, "Iup", sessionShellWidgetClass, iupmot_display, NULL, 0);
  if (!iupmot_appshell)
  {
    fprintf(stderr, "IUP error: cannot create shell.\n");
    return IUP_ERROR;
  }
  IupSetGlobal("APPSHELL", (char*)iupmot_appshell);

  IupStoreGlobal("SYSTEMLANGUAGE", setlocale(LC_ALL, NULL));

  iupmot_screen  = XDefaultScreen(iupmot_display);

  IupSetGlobal("XDISPLAY", (char*)iupmot_display);
  IupSetGlobal("XSCREEN", (char*)(intptr_t)iupmot_screen);

  /* screen depth can be 8bpp, but canvas can be 24bpp */
  {
    XVisualInfo vinfo;
    if (XMatchVisualInfo(iupmot_display, iupmot_screen, 24, TrueColor, &vinfo) ||
        XMatchVisualInfo(iupmot_display, iupmot_screen, 16, TrueColor, &vinfo))
    {
      iupmot_visual = vinfo.visual;
      IupSetGlobal("TRUECOLORCANVAS", "YES");
    }
    else
    {
      iupmot_visual = DefaultVisual(iupmot_display, iupmot_screen);
      IupSetGlobal("TRUECOLORCANVAS", "NO");
    }
  }

  /* driver system version */
  {
    int major = xmUseVersion/1000;
    int minor = xmUseVersion - major * 1000;
    IupSetfAttribute(NULL, "MOTIFVERSION", "%d.%d", major, minor);
    IupSetInt(NULL, "MOTIFNUMBER", (XmVERSION * 1000 + XmREVISION * 100 + XmUPDATE_LEVEL));

    IupSetGlobal("XSERVERVENDOR", ServerVendor(iupmot_display));
    IupSetInt(NULL, "XVENDORRELEASE", VendorRelease(iupmot_display));
  }

  iupmotColorInit();

  /* Set default colors from X resources, with fallbacks.
   * These colors are read from the X resource database (e.g., .Xresources)
   * allowing users to theme Motif applications via standard X mechanisms. */
  {
    /* Dialog colors - use *.background and *.foreground */
    iupmotSetGlobalColorFromXrm("*background", "*Background", "DLGBGCOLOR", 192, 192, 192);
    iupmotSetGlobalColorFromXrm("*foreground", "*Foreground", "DLGFGCOLOR", 0, 0, 0);

    /* Text widget colors - also use *.background and *.foreground for consistency */
    iupmotSetGlobalColorFromXrm("*background", "*Background", "TXTBGCOLOR", 255, 255, 255);
    iupmotSetGlobalColorFromXrm("*foreground", "*Foreground", "TXTFGCOLOR", 0, 0, 0);
    iupmotSetGlobalColorFromXrm("*highlightColor", "*HighlightColor", "TXTHLCOLOR", 128, 128, 128);

    /* Link color - typically blue, but can be themed */
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
  }

  /* enable alternative DnD icons as default */
  XtVaSetValues(XmGetXmDisplay(iupmot_display), XmNenableDragIcon, True, NULL);

  if (getenv("IUP_DEBUG"))
    XSynchronize(iupmot_display, 1);

  return IUP_NOERROR;
}

int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPNAME_INTERNAL", value);
  appname_set = 1;
  return 1;
}

void iupdrvClose(void)
{
  iupmotColorFinish();
  iupmotTipsFinish();
  iupmotStrRelease();

  if (iupmot_appshell)
    XtDestroyWidget(iupmot_appshell);

  if (iupmot_appcontext)
    XtDestroyApplicationContext(iupmot_appcontext);
}

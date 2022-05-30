/** \file
 * \brief Motif Open/Close
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>    
#include <stdio.h>    
#include <locale.h>
#include <string.h>    

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

int iupdrvOpen(int *argc, char ***argv)
{
  IupSetGlobal("DRIVER", "Motif");

  /* XtSetLanguageProc(NULL, NULL, NULL); 
     Removed to avoid invalid locale in modern Linux that set LANG=en_US.UTF-8 */

  /* We do NOT use XtVaOpenApplication because it crashes when using internal dummy argc and argv.
     iupmot_appshell = XtVaOpenApplication(&iupmot_appcontext, "Iup", NULL, 0, argc, *argv, NULL,
                                           sessionShellWidgetClass, NULL); */

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
  IupSetGlobal("XSCREEN", (char*)iupmot_screen);

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

  /* dialog background color */
  {
    iupmotSetGlobalColorAttrib(iupmot_appshell, XmNbackground, "DLGBGCOLOR");
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", 0, 0, 0);
    IupSetGlobal("_IUP_RESET_DLGBGCOLOR", "YES");  /* will update the DLGFGCOLOR when the first dialog is mapped */

    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", 0, 0, 0);
    iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", 128, 128, 128);
    IupSetGlobal("_IUP_RESET_TXTCOLORS", "YES");   /* will update the TXTCOLORS when the first text or list is mapped */

    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
  }

  /* enable alternative DnD icons as default */
  XtVaSetValues(XmGetXmDisplay(iupmot_display), XmNenableDragIcon, True, NULL);

  if (getenv("IUP_DEBUG"))
    XSynchronize(iupmot_display, 1);

  return IUP_NOERROR;
}

void iupdrvClose(void)
{ 
  iupmotColorFinish();
  iupmotTipsFinish();

  if (iupmot_appshell)
    XtDestroyWidget(iupmot_appshell);

  if (iupmot_appcontext)
    XtDestroyApplicationContext(iupmot_appcontext);
}

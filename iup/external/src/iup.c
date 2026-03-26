/* See Copyright Notice in "iup.h" */

/** \mainpage IUP Internal SDK
 *
 * \section codestd Code Standards
 *
 * \subsection func Function Names
 *  - IupFunc - User API, implemented in the core
 *  - iupFunc - Internal Core API, used in the core or driver
 *  - iupxxxFunc - Driver-specific internal API
 *  - iupdrvFunc - Driver API, implemented in driver, used in core or driver
 *  - xxxFunc - Driver local functions
 *
 * \subsection fil File Names
 *  - iupyyy.h - public headers
 *  - iup_yyy.h/c - core
 *  - iupxxx_yyy.h/c - driver
 *
 * \subsection def Defines
 *  - IUP_XXX - global enumerations
 *  - IXXX_YYY - local enumerations
 *  - iupXXX - macros
 */

/** \defgroup util Utilities */
/** \defgroup cpi Control SDK */
/** \defgroup ctrl Controls
 * \ingroup cpi */

#include <stdlib.h>

#include "iup.h"

/* This appears only here to avoid changing the iup.h header for bug fixes */
#define IUP_VERSION_FIX ""
#define IUP_VERSION_FIX_NUMBER 0
/* #define IUP_VERSION_FIX_DATE "AAAA/MM/DD" */

const char iup_ident[] = 
  "$IUP: " IUP_VERSION IUP_VERSION_FIX " " IUP_COPYRIGHT " $\n"
  "$URL: www.tecgraf.puc-rio.br/iup $\n";

/* Using this, if you look for the string TECVER, you will find also the library version. */
const char *iup_tecver = "TECVERID.str:Iup:LIB:" IUP_VERSION IUP_VERSION_FIX;

IUP_API char* IupVersion(void)
{
  (void)iup_tecver;
  (void)iup_ident;
  return IUP_VERSION IUP_VERSION_FIX;
}

IUP_API char* IupVersionDate(void)
{
#ifdef IUP_VERSION_FIX_DATE
  return IUP_VERSION_FIX_DATE;
#else
  return IUP_VERSION_DATE;
#endif
}
 
IUP_API int IupVersionNumber(void)
{
  return IUP_VERSION_NUMBER+IUP_VERSION_FIX_NUMBER;
}

static int ok_action(Ihandle* ih)
{
  (void)ih;
  return IUP_CLOSE;
}

IUP_API void IupVersionShow(void)
{
  Ihandle* dlg, *info, *ok;
  char* value;

  info = IupText(NULL);
  IupSetAttribute(info, "MULTILINE", "Yes");
  IupSetAttribute(info, "EXPAND", "Yes");
  IupSetAttribute(info, "READONLY", "Yes");
  IupSetAttribute(info, "VISIBLELINES", "10");
  IupSetAttribute(info, "VISIBLECOLUMNS", "30");

  ok = IupButton("_@IUP_OK", NULL);
  IupSetStrAttribute(ok, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(ok, "ACTION", (Icallback)ok_action);

  dlg = IupDialog(IupVbox(info, ok, NULL));

  IupSetAttribute(dlg,"TITLE","IUP Version");
  IupSetAttribute(dlg,"DIALOGFRAME","YES");
  IupSetAttribute(dlg,"DIALOGHINT","YES");
  IupSetAttribute(dlg, "RESIZE", "YES");
  IupSetAttribute(dlg, "GAP", "10");
  IupSetAttribute(dlg,"MARGIN","10x10");
  IupSetAttribute(IupGetChild(dlg, 0), "ALIGNMENT", "ACENTER");
  IupSetAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
  IupSetAttribute(dlg, "ICON", IupGetGlobal("ICON"));
  IupSetAttributeHandle(dlg, "DEFAULTESC", ok);

  IupMap(dlg);

  IupSetAttribute(info, "APPEND", IUP_NAME);
  IupSetAttribute(info, "APPEND", IUP_COPYRIGHT);
  IupSetAttribute(info, "APPEND", "");

  IupSetStrf(info, "APPEND", "IUP %s - %s", IupVersion(), IupVersionDate());
  IupSetStrf(info, "APPEND", "   Driver: %s", IupGetGlobal("DRIVER"));
  IupSetStrf(info, "APPEND", "   System: %s", IupGetGlobal("SYSTEM"));
  IupSetStrf(info, "APPEND", "   System Version: %s", IupGetGlobal("SYSTEMVERSION"));

  value = IupGetGlobal("MOTIFVERSION");
  if (value) IupSetStrf(info, "APPEND", "   Motif Version: %s", value);

  value = IupGetGlobal("GTKVERSION");
  if (value) IupSetStrf(info, "APPEND", "   GTK Version: %s", value);

  IupSetStrf(info, "APPEND", "   Screen Size: %s", IupGetGlobal("SCREENSIZE"));
  IupSetStrf(info, "APPEND", "   Screen DPI: %d", IupGetInt(NULL, "SCREENDPI"));
  IupSetStrf(info, "APPEND", "   Default Font: %s", IupGetGlobal("DEFAULTFONT"));

  IupSetAttribute(info, "APPEND", "");
  IupSetAttribute(info, "APPEND", "Iup Libraries Open:");

  if (IupGetGlobal("_IUP_CONTROLS_OPEN"))
    IupSetAttribute(info, "APPEND", "   IupControlsOpen");

  if (IupGetGlobal("_IUP_GLCANVAS_OPEN"))
  {
    IupSetAttribute(info, "APPEND", "   IupGLCanvasOpen");

    if (IupGetGlobal("GL_VERSION"))
    {
      IupSetStrf(info, "APPEND", "      OpenGL Vendor: %s", IupGetGlobal("GL_VENDOR"));
      IupSetStrf(info, "APPEND", "      OpenGL Renderer: %s", IupGetGlobal("GL_RENDERER"));
      IupSetStrf(info, "APPEND", "      OpenGL Version: %s", IupGetGlobal("GL_VERSION"));
    }
  }

  if (IupGetGlobal("_IUP_GLCONTROLS_OPEN"))
    IupSetAttribute(info, "APPEND", "   IupGLControlsOpen");

  if (IupGetGlobal("_IUP_SCINTILLA_OPEN"))
  {
    IupSetAttribute(info, "APPEND", "   IupScintillaOpen");
    IupSetStrf(info, "APPEND", "      Scintilla %s", IupGetGlobal("SCINTILLA_VERSION"));
  }

  if (IupGetGlobal("_IUP_WEBBROWSER_OPEN"))
    IupSetAttribute(info, "APPEND", "   IupWebBrowserOpen");

  if (IupGetGlobal("_IUP_PLOT_OPEN"))
    IupSetAttribute(info, "APPEND", "   IupPlotOpen");

  if (IupGetGlobal("_IUP_MGLPLOT_OPEN"))
    IupSetAttribute(info, "APPEND", "   IupMglPlotOpen");

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);
  IupDestroy(dlg);
}

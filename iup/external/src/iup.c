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
#include <stdio.h>
#include <stdarg.h>

#include "iup.h"

/* This appears only here to avoid changing the iup.h header for bug fixes */
#define IUP_VERSION_FIX ""
#define IUP_VERSION_FIX_NUMBER 0
/* #define IUP_VERSION_FIX_DATE "AAAA/MM/DD" */

const char iup_ident[] =
  "$IUP: " IUP_VERSION IUP_VERSION_FIX " " IUP_COPYRIGHT " $\n"
  "$URL: www.tecgraf.puc-rio.br/iup $\n";

/* Using this, if you look for the string TECVER, you will find also the library version. */
const char* iup_tecver = "TECVERID.str:Iup:LIB:" IUP_VERSION IUP_VERSION_FIX;

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

typedef struct _IversionText
{
  char buffer[4096];
  int len, lines, columns;
} IversionText;

static void iVersionAppend(IversionText* text, const char* format, ...)
{
  int count, size;
  va_list arglist;

  if (text->lines && text->len < (int)sizeof(text->buffer) - 1)
    text->buffer[text->len++] = '\n';

  size = (int)sizeof(text->buffer) - text->len;
  if (size <= 1)
    return;

  va_start(arglist, format);
  count = vsnprintf(text->buffer + text->len, size, format, arglist);
  va_end(arglist);

  if (count < 0)
    count = 0;
  if (count >= size)
    count = size - 1;

  text->len += count;
  text->buffer[text->len] = 0;
  text->lines++;
  if (count > text->columns)
    text->columns = count;
}

static void iVersionAppendGlobal(IversionText* text, const char* title, const char* name)
{
  char* value = IupGetGlobal(name);
  if (value)
    iVersionAppend(text, "%s: %s", title, value);
}

IUP_API void IupVersionShow(void)
{
  Ihandle* dlg, *info, *ok;
  IversionText text;

  text.len = 0;
  text.lines = 0;
  text.columns = 0;
  text.buffer[0] = 0;

  iVersionAppend(&text, "%s", IUP_NAME);
  iVersionAppend(&text, "%s", IUP_COPYRIGHT);
  iVersionAppend(&text, "%s", "");

  iVersionAppend(&text, "IUP %s - %s", IupVersion(), IupVersionDate());
  iVersionAppendGlobal(&text, "   Driver", "DRIVER");
  iVersionAppendGlobal(&text, "   System", "SYSTEM");
  iVersionAppendGlobal(&text, "   System Version", "SYSTEMVERSION");
  iVersionAppendGlobal(&text, "   Motif Version", "MOTIFVERSION");
  iVersionAppendGlobal(&text, "   GTK Version", "GTKVERSION");
  iVersionAppendGlobal(&text, "   Qt Version", "QTVERSION");
  iVersionAppendGlobal(&text, "   EFL Version", "EFLVERSION");
  iVersionAppendGlobal(&text, "   FLTK Version", "FLTKVERSION");
  iVersionAppendGlobal(&text, "   GNUstep Version", "GNUSTEPVERSION");
  iVersionAppendGlobal(&text, "   WinUI Version", "WINUIVERSION");
  iVersionAppendGlobal(&text, "   Screen Size", "SCREENSIZE");
  iVersionAppend(&text, "   Screen DPI: %d", IupGetInt(NULL, "SCREENDPI"));
  iVersionAppendGlobal(&text, "   Default Font", "DEFAULTFONT");

  if (IupGetGlobal("_IUP_CONTROLS_OPEN") || IupGetGlobal("_IUP_GLCANVAS_OPEN") || IupGetGlobal("_IUP_WEBBROWSER_OPEN") ||
      IupGetGlobal("_IUP_PLOT_OPEN") || IupGetGlobal("_IUP_MEDIA_OPEN"))
  {
    iVersionAppend(&text, "%s", "");
    iVersionAppend(&text, "Iup Libraries Open:");

    if (IupGetGlobal("_IUP_CONTROLS_OPEN"))
      iVersionAppend(&text, "   IupControlsOpen");

    if (IupGetGlobal("_IUP_GLCANVAS_OPEN"))
    {
      iVersionAppend(&text, "   IupGLCanvasOpen");

      if (IupGetGlobal("GL_VERSION"))
      {
        iVersionAppendGlobal(&text, "      OpenGL Vendor", "GL_VENDOR");
        iVersionAppendGlobal(&text, "      OpenGL Renderer", "GL_RENDERER");
        iVersionAppendGlobal(&text, "      OpenGL Version", "GL_VERSION");
      }
    }

    if (IupGetGlobal("_IUP_WEBBROWSER_OPEN"))
      iVersionAppend(&text, "   IupWebBrowserOpen");

    if (IupGetGlobal("_IUP_PLOT_OPEN"))
      iVersionAppend(&text, "   IupPlotOpen");

    if (IupGetGlobal("_IUP_MEDIA_OPEN"))
      iVersionAppend(&text, "   IupMediaOpen");
  }

  info = IupText();
  IupSetAttribute(info, "MULTILINE", "Yes");
  IupSetAttribute(info, "EXPAND", "Yes");
  IupSetAttribute(info, "READONLY", "Yes");
  IupSetInt(info, "VISIBLELINES", text.lines);
  IupSetInt(info, "VISIBLECOLUMNS", text.columns);
  IupSetStrAttribute(info, "VALUE", text.buffer);

  ok = IupButton("_@IUP_OK");
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

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);
  IupDestroy(dlg);
}

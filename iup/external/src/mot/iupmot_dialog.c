/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/BulletinB.h>
#include <Xm/MwmUtil.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


Atom iupmot_wm_deletewindow = 0;  /* used also by IupMessageDlg */

static int motDialogSetBgColorAttrib(Ihandle* ih, const char* value);

/****************************************************************
                     Utilities
****************************************************************/

void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  XSetTransientForHint(iupmot_display, XtWindow(ih->handle), XtWindow(parent));
}

int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih) || ih->data->show_state == IUP_MINIMIZE;
}

void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  Dimension width, height;
  int border=0, caption=0, menu;
  if (!handle)
    handle = ih->handle;

  XtVaGetValues(handle, XmNwidth, &width, XmNheight, &height, NULL);

  if (ih)
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  if (w) *w = width + 2*border;
  if (h) *h = height + 2*border + caption;  /* menu is inside the dialog_manager */
}

void iupmotDialogSetVisual(Ihandle* ih, void* visual)
{
  Ihandle *dialog = IupGetDialog(ih);
  XtVaSetValues(dialog->handle, XmNvisual, visual, NULL);
}

void iupmotDialogResetVisual(Ihandle* ih)
{
  Ihandle *dialog = IupGetDialog(ih);
  XtVaSetValues(dialog->handle, XmNvisual, iupmot_visual, NULL);
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  if (visible)
  {
    XtMapWidget(ih->handle);
    XRaiseWindow(iupmot_display, XtWindow(ih->handle));
    while (!iupdrvDialogIsVisible(ih)); /* waits until window get mapped */
  }
  else
  {
    /* if iupdrvIsVisible reports hidden, then it should be minimized */ 
    if (!iupdrvIsVisible(ih))  /* can NOT hide a minimized window, so map it first. */
    {
      XtMapWidget(ih->handle);
      XRaiseWindow(iupmot_display, XtWindow(ih->handle));
      while (!iupdrvDialogIsVisible(ih)); /* waits until window get mapped */
    }

    XtUnmapWidget(ih->handle);
    while (iupdrvDialogIsVisible(ih)); /* waits until window gets unmapped */
  }
}

void iupdrvDialogGetPosition(Ihandle *ih, InativeHandle* handle, int *x, int *y)
{
  Position cur_x, cur_y;
  if (!handle)
    handle = ih->handle;
  XtVaGetValues(handle, XmNx, &cur_x, 
                        XmNy, &cur_y, 
                        NULL);

  if (ih)
  {
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);
    /* compensate the decoration */
    cur_x -= (Position)border;
    cur_y -= (Position)(border+caption);
  }

  if (x) *x = cur_x;
  if (y) *y = cur_y;
}

void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  /* no need to compensate decoration when setting */
  XtVaSetValues(ih->handle,
    XmNx, (XtArgVal)x,
    XmNy, (XtArgVal)y,
    NULL);
}

static int motDialogGetMenuSize(Ihandle* ih)
{
  if (ih->data->menu)
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    return 0;
}

static int motDialogGetWindowDecor(Ihandle* ih, int *border, int *caption)
{
  /* Try to get the size of the window decoration. */
  /* Use the dialog_manager instead of the handle, so it can use the client offset. */
  Widget dialog_manager = (Widget)iupAttribGet(ih, "_IUPMOT_DLGCONTAINER");
  XWindowAttributes wa;
  wa.x = 0; wa.y = 0;
  XGetWindowAttributes(iupmot_display, XtWindow(dialog_manager), &wa);
  if (wa.x > 0 && wa.y > 0 && wa.y >= wa.x)
  {
    *border = wa.x;
    *caption = wa.y - *border;
    return 1;
  }

  *border = 0;
  *caption = 0;

  return 0;
}

void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
  static int native_border = 0;
  static int native_caption = 0;

  int has_titlebar = iupAttribGetBoolean(ih, "RESIZE")  || /* GTK and Motif only */
                     iupAttribGetBoolean(ih, "MAXBOX")  ||
                     iupAttribGetBoolean(ih, "MINBOX")  ||
                     iupAttribGetBoolean(ih, "MENUBOX") || 
                     iupAttribGet(ih, "TITLE");

  int has_border = has_titlebar ||
                   iupAttribGetBoolean(ih, "RESIZE") ||
                   iupAttribGetBoolean(ih, "BORDER");

  *menu = motDialogGetMenuSize(ih);

  if (ih->handle && iupdrvDialogIsVisible(ih))
  {
    int win_border, win_caption;
    if (motDialogGetWindowDecor(ih, &win_border, &win_caption))
    {
      *border = 0;
      if (has_border)
        *border = win_border;

      *caption = 0;
      if (has_titlebar)
        *caption = win_caption;

      if (!native_border && *border)
        native_border = win_border;

      if (!native_caption && *caption)
        native_caption = win_caption;

      return;
    }
  }

  /* I could not set the size of the window including the decorations when the dialog is hidden */
  /* So we have to estimate the size of borders and caption when the dialog is hidden           */

  *border = 0;
  if (has_border)
  {
    if (native_border)
      *border = native_border;
    else
      *border = 5;
  }

  *caption = 0;
  if (has_titlebar)
  {
    if (native_caption)
      *caption = native_caption;
    else
      *caption = 20;
  }
}

static int motDialogQueryWMspecSupport(Atom feature)
{
  static Atom netsuppport = 0;
  Atom type;
  Atom *atoms;
  int format;
  unsigned long after, natoms, i;

  if (!netsuppport)
    netsuppport = XmInternAtom(iupmot_display, "_NET_SUPPORTED", False);

  /* get all the features */
  XGetWindowProperty(iupmot_display, RootWindow(iupmot_display, iupmot_screen),
                     netsuppport, 0, LONG_MAX, False, XA_ATOM, &type, &format, &natoms,
                     &after, (unsigned char **)&atoms);
  if (type != XA_ATOM || atoms == NULL)
  {
    if (atoms) XFree(atoms);
    return 0;
  }

  /* Lookup the feature we want */
  for (i = 0; i < natoms; i++)
  {
    if (atoms[i] == feature)
    {
      XFree(atoms);
      return 1;
    }
  }

  XFree(atoms);
  return 0;
}

static void motDialogSetWindowManagerStyle(Ihandle* ih)
{
  MwmHints hints;
  static Atom xwmhint = 0;
  if (!xwmhint)
    xwmhint = XmInternAtom(iupmot_display, "_MOTIF_WM_HINTS", False);

  hints.flags = (MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS);
  hints.functions = 0;
  hints.decorations = 0;
  hints.input_mode = 0;
  hints.status = 0;

  if (iupAttribGet(ih, "TITLE")) {
    hints.functions   |= MWM_FUNC_MOVE;
    hints.decorations |= MWM_DECOR_TITLE;
  }

  if (iupAttribGetBoolean(ih, "MENUBOX")) {
    hints.functions   |= MWM_FUNC_CLOSE;
    hints.decorations |= MWM_DECOR_MENU;
  }

  if (iupAttribGetBoolean(ih, "MINBOX")) {
    hints.functions   |= MWM_FUNC_MINIMIZE;
    hints.decorations |= MWM_DECOR_MINIMIZE;
  }

  if (iupAttribGetBoolean(ih, "MAXBOX")) {
    hints.functions   |= MWM_FUNC_MAXIMIZE;
    hints.decorations |= MWM_DECOR_MAXIMIZE;
  }

  if (iupAttribGetBoolean(ih, "RESIZE")) {
    hints.functions   |= MWM_FUNC_RESIZE;
    hints.decorations |= MWM_DECOR_RESIZEH;
  }

  if (iupAttribGetBoolean(ih, "BORDER"))
    hints.decorations |= MWM_DECOR_BORDER;

  XChangeProperty(iupmot_display, XtWindow(ih->handle),
      xwmhint, xwmhint,
      32, PropModeReplace,
      (const unsigned char *) &hints,
      PROP_MOTIF_WM_HINTS_ELEMENTS);
}

static void motDialogChangeWMState(Ihandle* ih, Atom state1, Atom state2, int operation)
{
  static Atom wmstate = 0;
  if (!wmstate)
    wmstate = XmInternAtom(iupmot_display, "_NET_WM_STATE", False);

  if (iupdrvDialogIsVisible(ih))
  {
    XEvent evt;
    evt.type = ClientMessage;
    evt.xclient.type = ClientMessage;
    evt.xclient.serial = 0;
    evt.xclient.send_event = True;
    evt.xclient.display = iupmot_display;
    evt.xclient.window = XtWindow(ih->handle);
    evt.xclient.message_type = wmstate;
    evt.xclient.format = 32;
    evt.xclient.data.l[0] = operation;
    evt.xclient.data.l[1] = state1;
    evt.xclient.data.l[2] = state2;
    evt.xclient.data.l[3] = 0;
    evt.xclient.data.l[4] = 0;
    
    XSendEvent(iupmot_display, RootWindow(iupmot_display, iupmot_screen), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &evt);
  }
  else
  {
    if (operation)
    {
      if (state1 && state2)
      {
        Atom atoms[2];
        atoms[0] = state1;
        atoms[1] = state2;

        XChangeProperty(iupmot_display, XtWindow(ih->handle),
            wmstate, XA_ATOM,
            32, PropModeReplace,
            (const unsigned char *)&atoms, 2);
      }
      else
      {
        XChangeProperty(iupmot_display, XtWindow(ih->handle),
            wmstate, XA_ATOM,
            32, PropModeReplace,
            (const unsigned char *)&state1, 1);
      }
    }
    else
    {
      /* TODO: This is not working. The property is not correctly removed. */
      /* XDeleteProperty(iupmot_display, XtWindow(ih->handle), wmstate); */

      /* Maybe the right way to do it is to retrieve all the atoms               */
      /* and change again with all atoms except the one to remove                */
    }
  }
}

static int motDialogSetFullScreen(Ihandle* ih, int fullscreen)
{
  static int support_fullscreen = -1;  /* WARNING: The WM can be changed dinamically */

  static Atom xwmfs = 0;
  if (!xwmfs)
    xwmfs = XmInternAtom(iupmot_display, "_NET_WM_STATE_FULLSCREEN", False);

  if (support_fullscreen == -1)
    support_fullscreen = motDialogQueryWMspecSupport(xwmfs);

  if (support_fullscreen)
  {
    motDialogChangeWMState(ih, xwmfs, 0, fullscreen);
    return 1;
  }

  return 0;
}

int iupdrvDialogSetPlacement(Ihandle* ih)
{
  char* placement;
  int old_state = ih->data->show_state;
  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
    return 1;
  
  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupDialogCustomFrameRestore(ih))
    {
      ih->data->show_state = IUP_RESTORE;
      return 1;
    }

    return 0;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    iupDialogCustomFrameMaximize(ih);
    iupAttribSet(ih, "PLACEMENT", NULL); /* reset to NORMAL */
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    if (iupdrvDialogIsVisible(ih))
      XIconifyWindow(iupmot_display, XtWindow(ih->handle), iupmot_screen);
    else
    {
      /* TODO: This is not working, so force a minimize after visible.  */
      /*XWMHints wm_hints;                                               */
      /*wm_hints.flags = StateHint;                                      */
      /*wm_hints.initial_state = IconicState;                            */
      /*XSetWMHints(iupmot_display, XtWindow(ih->handle), &wm_hints);  */

      XtMapWidget(ih->handle);
      XIconifyWindow(iupmot_display, XtWindow(ih->handle), iupmot_screen);
    }
    ih->data->show_state = IUP_MINIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    static Atom maxatoms[2] = {0, 0};
    if (!(maxatoms[0]))
    {
      maxatoms[0] = XmInternAtom(iupmot_display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
      maxatoms[1] = XmInternAtom(iupmot_display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    }

    motDialogChangeWMState(ih, maxatoms[0], maxatoms[1], 1);
    ih->data->show_state = IUP_MAXIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    /* position the decoration outside the screen */
    x = -(border);
    y = -(border+caption+menu);

    /* the dialog client area will cover the task bar */
    iupdrvGetFullSize(&width, &height);

    height += menu; /* the menu is included in the client area size in Motif. */

    /* set the new size and position */
    /* The resize event will update the layout */
    XtVaSetValues(ih->handle,
      XmNx, (XtArgVal)x,  /* outside border */
      XmNy, (XtArgVal)y,
      XmNwidth, (XtArgVal)width,  /* client size */
      XmNheight, (XtArgVal)height,
      NULL);

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL); /* reset to NORMAL */

  return 1;
}


/****************************************************************************
                                   Attributes
****************************************************************************/


static int motDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  int decorwidth = 0, decorheight = 0;
  int min_w = 1, min_h = 1;          /* MINSIZE default value */
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  /* The minmax size restricts the client area */
  iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

  if (min_w > decorwidth)
    XtVaSetValues(ih->handle, XmNminWidth, min_w-decorwidth, NULL);
  if (min_h > decorheight)
    XtVaSetValues(ih->handle, XmNminHeight, min_h-decorheight, NULL);  

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int motDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  int decorwidth = 0, decorheight = 0;
  int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  /* The minmax size restricts the client area */
  iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

  if (max_w > decorwidth)
    XtVaSetValues(ih->handle, XmNmaxWidth, max_w-decorwidth, NULL);  
  if (max_h > decorheight)
    XtVaSetValues(ih->handle, XmNmaxHeight, max_h-decorheight, NULL);  

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static int motDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = "";

  iupmotSetTitle(ih->handle, value);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return 0;

  return 1;
}

static char* motDialogGetClientSizeAttrib(Ihandle *ih)
{
  if (ih->handle)
  {
    Dimension width, height;
    Widget dialog_manager = (Widget)iupAttribGet(ih, "_IUPMOT_DLGCONTAINER");
    XtVaGetValues(dialog_manager, XmNwidth, &width, XmNheight, &height, NULL);

    /* remove the menu because it is placed inside the client area */
    height -= (Dimension)motDialogGetMenuSize(ih);

    return iupStrReturnIntInt((int)width, (int)height, 'x');
  }
  else
    return iupDialogGetClientSizeAttrib(ih);
}

static char* motDialogGetClientOffsetAttrib(Ihandle *ih)
{
  /* remove the menu because it is placed inside the client area */
  return iupStrReturnIntInt(0, -motDialogGetMenuSize(ih), 'x');
}

static int motDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
  {
    Widget dialog_manager = (Widget)iupAttribGet(ih, "_IUPMOT_DLGCONTAINER");
    XtVaSetValues(dialog_manager, XmNbackground, color, NULL);
    XtVaSetValues(dialog_manager, XmNbackgroundPixmap, XmUNSPECIFIED_PIXMAP, NULL);
    return 1;
  }
  return 0; 
}

static int motDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (motDialogSetBgColorAttrib(ih, value))
    return 1;
  else                                     
  {
    Pixmap pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
    if (pixmap)
    {
      Widget dialog_manager = (Widget)iupAttribGet(ih, "_IUPMOT_DLGCONTAINER");
      XtVaSetValues(dialog_manager, XmNbackgroundPixmap, pixmap, NULL);
      return 1;
    }
  }
  return 0; 
}

static int motDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{                       
  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPMOT_FS_STYLE"))
    {
      int visible = iupdrvDialogIsVisible(ih);
      if (visible)
        iupAttribSet(ih, "_IUPMOT_FS_STYLE", "VISIBLE");
      else
        iupAttribSet(ih, "_IUPMOT_FS_STYLE", "HIDDEN");

      /* save the previous decoration attributes */
      /* during fullscreen these attributes can be consulted by the application */
      iupAttribSetStr(ih, "_IUPMOT_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribSetStr(ih, "_IUPMOT_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribSetStr(ih, "_IUPMOT_FS_MENUBOX",iupAttribGet(ih, "MENUBOX"));
      iupAttribSetStr(ih, "_IUPMOT_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribSetStr(ih, "_IUPMOT_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribSetStr(ih, "_IUPMOT_FS_TITLE",  iupAttribGet(ih, "TITLE"));

      /* remove the decorations attributes */
      iupAttribSet(ih, "MAXBOX", "NO");
      iupAttribSet(ih, "MINBOX", "NO");
      iupAttribSet(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL);  /* must use IupSetAttribute to update the native implementation */
      iupAttribSet(ih, "RESIZE", "NO");
      iupAttribSet(ih, "BORDER", "NO");

      /* use WM fullscreen support */
      if (!motDialogSetFullScreen(ih, 1))
      {
        /* or configure fullscreen manually */
        int decor;
        int width, height;

        /* hide before changing decorations */
        if (visible)
        {
          iupAttribSet(ih, "_IUPMOT_SHOW_STATE", NULL);  /* To avoid a SHOW_CB notification */
          XtUnmapWidget(ih->handle);
        }

        /* save the previous position and size */
        iupAttribSetStr(ih, "_IUPMOT_FS_X", IupGetAttribute(ih, "X"));  /* must use IupGetAttribute to check from the native implementation */
        iupAttribSetStr(ih, "_IUPMOT_FS_Y", IupGetAttribute(ih, "Y"));
        iupAttribSetStr(ih, "_IUPMOT_FS_SIZE", IupGetAttribute(ih, "RASTERSIZE"));

        /* save the previous decoration */
        XtVaGetValues(ih->handle, XmNmwmDecorations, &decor, NULL);
        iupAttribSet(ih, "_IUPMOT_FS_DECOR", (char*)(long)decor);

        /* remove the decorations */
        XtVaSetValues(ih->handle, XmNmwmDecorations, (XtArgVal)0, NULL);
        motDialogSetWindowManagerStyle(ih);

        /* get full screen size */
        iupdrvGetFullSize(&width, &height);

        /* set position and size */
        XtVaSetValues(ih->handle, XmNwidth,  (XtArgVal)width,  /* client size */
                                  XmNheight, (XtArgVal)height, 
                                  XmNx,      (XtArgVal)0,
                                  XmNy,      (XtArgVal)0,
                                  NULL);

        /* layout will be updated in motDialogConfigureNotify */
        if (visible)
          XtMapWidget(ih->handle);
      }
    }
  }
  else
  {
    char* fs_style = iupAttribGet(ih, "_IUPMOT_FS_STYLE");
    if (fs_style)
    {
      /* can only switch back from full screen if window was visible */
      /* when fullscreen was set */
      if (iupStrEqualNoCase(fs_style, "VISIBLE"))
      {
        iupAttribSet(ih, "_IUPMOT_FS_STYLE", NULL);

        /* restore the decorations attributes */
        iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPMOT_FS_MAXBOX"));
        iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPMOT_FS_MINBOX"));
        iupAttribSetStr(ih, "MENUBOX",iupAttribGet(ih, "_IUPMOT_FS_MENUBOX"));
        IupSetAttribute(ih, "TITLE",  iupAttribGet(ih, "_IUPMOT_FS_TITLE"));   /* must use IupSetAttribute to update the native implementation */
        iupAttribSetStr(ih, "RESIZE", iupAttribGet(ih, "_IUPMOT_FS_RESIZE"));
        iupAttribSetStr(ih, "BORDER", iupAttribGet(ih, "_IUPMOT_FS_BORDER"));

        if (!motDialogSetFullScreen(ih, 0))
        {
          int border, caption, menu, x, y;
          int visible = iupdrvDialogIsVisible(ih);
          if (visible)
            XtUnmapWidget(ih->handle);

          /* restore the decorations */
          XtVaSetValues(ih->handle, XmNmwmDecorations, (XtArgVal)(int)(long)iupAttribGet(ih, "_IUPMOT_FS_DECOR"), NULL);
          motDialogSetWindowManagerStyle(ih);

          /* the dialog decoration will not be considered yet in the next XtVaSetValues */
          /* so compensate the decoration when restoring the position and size */
          iupdrvDialogGetDecoration(ih, &border, &caption, &menu);
          x = iupAttribGetInt(ih, "_IUPMOT_FS_X") - (border);
          y = iupAttribGetInt(ih, "_IUPMOT_FS_Y") - (border+caption+menu);

          /* restore position and size */
          XtVaSetValues(ih->handle, 
                      XmNx,      (XtArgVal)x, 
                      XmNy,      (XtArgVal)y, 
                      XmNwidth,  (XtArgVal)(IupGetInt(ih, "_IUPMOT_FS_SIZE") - (2*border)), 
                      XmNheight, (XtArgVal)(IupGetInt2(ih, "_IUPMOT_FS_SIZE") - (2*border+caption)), 
                      NULL);

          /* layout will be updated in motDialogConfigureNotify */
          if (visible)
            XtMapWidget(ih->handle);

          /* remove auxiliar attributes */
          iupAttribSet(ih, "_IUPMOT_FS_X", NULL);
          iupAttribSet(ih, "_IUPMOT_FS_Y", NULL);
          iupAttribSet(ih, "_IUPMOT_FS_SIZE", NULL);
          iupAttribSet(ih, "_IUPMOT_FS_DECOR", NULL);
        }

        /* remove auxiliar attributes */
        iupAttribSet(ih, "_IUPMOT_FS_MAXBOX", NULL);
        iupAttribSet(ih, "_IUPMOT_FS_MINBOX", NULL);
        iupAttribSet(ih, "_IUPMOT_FS_MENUBOX",NULL);
        iupAttribSet(ih, "_IUPMOT_FS_RESIZE", NULL);
        iupAttribSet(ih, "_IUPMOT_FS_BORDER", NULL);
        iupAttribSet(ih, "_IUPMOT_FS_TITLE",  NULL);
      }
    }
  }
  return 1;
}

static int motDialogSetIconAttrib(Ihandle* ih, const char *value)
{
  if (!value)
    XtVaSetValues(ih->handle, XmNiconPixmap, NULL, NULL);
  else
  {
    Pixmap icon = (Pixmap)iupImageGetIcon(value);
    Pixmap icon_mask = iupmotImageGetMask(value);
    if (icon)
      XtVaSetValues(ih->handle, XmNiconPixmap, icon, NULL);
    if (icon_mask)
      XtVaSetValues(ih->handle, XmNiconMask, icon, NULL);
  }
  return 1;
}

/****************************************************************
                     Callbacks and Events
****************************************************************/


static void motDialogCBclose(Widget w, XtPointer client_data, XtPointer call_data)
{
  Icallback cb;
  Ihandle *ih = (Ihandle*)client_data;
  if (!ih) return;
  (void)call_data;
  (void)w;

  /* even when ACTIVE=NO the dialog gets this event */
  if (!iupdrvIsActive(ih))
    return;

  cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return;
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih); /* default: close the window */
}

static void motDialogConfigureNotify(Widget w, XEvent *evt, String* s, Cardinal *card)
{
  IFnii cb;
  int border, caption, menu;
  XConfigureEvent *cevent = (XConfigureEvent *)evt;
  Ihandle* ih;
  (void)s;
  (void)card;

  XtVaGetValues(w, XmNuserData, &ih, NULL);
  if (!ih) return;

  if (ih->data->menu && ih->data->menu->handle)
    XtVaSetValues(ih->data->menu->handle, XmNwidth, (XtArgVal)(cevent->width), NULL);

  if (ih->data->ignore_resize) return; 

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  /* update dialog size */
  ih->currentwidth = cevent->width + 2*border;
  ih->currentheight = cevent->height + 2*border + caption; /* menu is inside the dialog_manager */

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (!cb || cb(ih, cevent->width, cevent->height - menu)!=IUP_IGNORE)  /* width and height here are for the client area */
  {
    ih->data->ignore_resize = 1;
    IupRefresh(ih);
    ih->data->ignore_resize = 0;
  }
}

static void motDialogCBStructureNotifyEvent(Widget w, XtPointer data, XEvent *evt, Boolean *cont)
{
  Ihandle *ih = (Ihandle*)data;
  int state = -1;
  (void)cont;
  (void)w;

  switch(evt->type)
  {
    case MapNotify:
    {
      if (ih->data->show_state == IUP_MINIMIZE) /* it is a RESTORE. */
        state = IUP_RESTORE;
      break;
    }
    case UnmapNotify:
    {
      if (ih->data->show_state != IUP_HIDE) /* it is a MINIMIZE. */
        state = IUP_MINIMIZE;
      break;
    }
  }

  if (state < 0)
    return;

  if (ih->data->show_state != state)
  {
    IFni cb;
    ih->data->show_state = state;

    cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    if (cb && cb(ih, state) == IUP_CLOSE) 
      IupExitLoop();
  }
}

static void motDialogDestroyCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  /* If the IUP dialog was not destroyed, destroy it here. */
  if (iupObjectCheck(ih))
    IupDestroy(ih);

  /* this callback is useful to destroy children dialogs when the parent is destroyed. */
  /* The application is responsible for destroying the children before this happen.     */
  (void)w;
  (void)call_data;
}


/****************************************************************
                     Idialog
****************************************************************/

/* replace the common dialog SetChildrenPosition method because of 
   the menu that it is inside the dialog. */
static void motDialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    /* Native container, position is reset */
    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    y += motDialogGetMenuSize(ih);

    /* Child coordinates are relative to client left-top corner. */
    iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static void* motDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUPMOT_DLGCONTAINER");
}

static int motDialogMapMethod(Ihandle* ih)
{
  Widget dialog_manager;
  InativeHandle* parent;
  int mwm_decor = 0;
  int num_args = 0;
  int has_titlebar = 0;
  Arg args[20];

  /****************************/
  /* Create the dialog shell  */
  /****************************/

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    iupDialogCustomFrameSimulateCheckCallbacks(ih);

  if (iupAttribGet(ih, "TITLE"))
    has_titlebar = 1;
  if (iupAttribGetBoolean(ih, "RESIZE"))
  {
    mwm_decor |= MWM_DECOR_RESIZEH;
    mwm_decor |= MWM_DECOR_BORDER;  /* has_border */
  }
  else
    iupAttribSet(ih, "MAXBOX", "NO");
  if (iupAttribGetBoolean(ih, "MENUBOX"))
  {
    mwm_decor |= MWM_DECOR_MENU;
    has_titlebar = 1;
  }
  if (iupAttribGetBoolean(ih, "MAXBOX"))
  {
    mwm_decor |= MWM_DECOR_MAXIMIZE;
    has_titlebar = 1;
  }
  if (iupAttribGetBoolean(ih, "MINBOX"))
  {
    mwm_decor |= MWM_DECOR_MINIMIZE;
    has_titlebar = 1;
  }
  if (has_titlebar)
    mwm_decor |= MWM_DECOR_TITLE;
  if (iupAttribGetBoolean(ih, "BORDER") || has_titlebar)   
    mwm_decor |= MWM_DECOR_BORDER;  /* has_border */

  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* so XtRealizeWidget will not show the dialog */
  iupMOT_SETARG(args, num_args, XmNdeleteResponse, XmDO_NOTHING);
  iupMOT_SETARG(args, num_args, XmNallowShellResize, True); /* Used so the BulletinBoard can control the shell size */
  iupMOT_SETARG(args, num_args, XmNtitle, "");
  iupMOT_SETARG(args, num_args, XmNvisual, iupmot_visual);
  
  if (iupmotColorMap()) 
    iupMOT_SETARG(args, num_args, XmNcolormap, iupmotColorMap());

  if (mwm_decor != 0x7E) 
    iupMOT_SETARG(args, num_args, XmNmwmDecorations, mwm_decor);

  if (iupAttribGetBoolean(ih, "SAVEUNDER"))
    iupMOT_SETARG(args, num_args, XmNsaveUnder, True);

  parent = iupDialogGetNativeParent(ih);
  if (parent)
    ih->handle = XtCreatePopupShell(NULL, topLevelShellWidgetClass, (Widget)parent, args, num_args);
  else
    ih->handle = XtAppCreateShell(NULL, "dialog", topLevelShellWidgetClass, iupmot_display, args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  XmAddWMProtocolCallback(ih->handle, iupmot_wm_deletewindow, motDialogCBclose, (XtPointer)ih);

  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent,      (XtPointer)ih);
  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, StructureNotifyMask, False, (XtEventHandler)motDialogCBStructureNotifyEvent, (XtPointer)ih);

  XtAddCallback(ih->handle, XmNdestroyCallback, (XtCallbackProc)motDialogDestroyCallback, (XtPointer)ih);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
  {
    XtAddEventHandler(ih->handle, ButtonPressMask | ButtonReleaseMask, False, (XtEventHandler)iupmotButtonPressReleaseEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);
  }

  /*****************************/
  /* Create the dialog manager */
  /*****************************/

  dialog_manager = XtVaCreateManagedWidget(
              "dialog_manager",
              xmBulletinBoardWidgetClass,
              ih->handle,
              XmNmarginWidth, 0,
              XmNmarginHeight, 0,
              XmNwidth, 100,     /* set this to avoid size calculation problems */
              XmNheight, 100,
              XmNborderWidth, 0,
              XmNshadowThickness, 0,
              XmNnoResize, iupAttribGetBoolean(ih, "RESIZE")? False: True,
              XmNresizePolicy, XmRESIZE_NONE, /* no automatic resize of children */
              XmNuserData, ih, /* used only in motDialogConfigureNotify                   */
              XmNnavigationType, XmTAB_GROUP,
              NULL);

  iupAttribSet(ih, "_IUPMOT_DLGCONTAINER", (char*)dialog_manager);

  XtOverrideTranslations(dialog_manager, XtParseTranslationTable("<Configure>: iupDialogConfigure()"));
  XtAddCallback(dialog_manager, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);
  XtAddEventHandler(dialog_manager, KeyPressMask, False,(XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);

  /* initialize the widget */
  XtRealizeWidget(ih->handle);

  /* child dialogs must be always on top of the parent */
  if (parent)
    XSetTransientForHint(iupmot_display, XtWindow(ih->handle), XtWindow(parent));

  if (mwm_decor != 0x7E)  /* some decoration was changed */
    motDialogSetWindowManagerStyle(ih);

  /* Ignore VISIBLE before mapping */
  iupAttribSet(ih, "VISIBLE", NULL);

  if (IupGetGlobal("_IUP_RESET_DLGBGCOLOR"))
  {
    iupmotSetGlobalColorAttrib(dialog_manager, XmNbackground, "DLGBGCOLOR");
    iupmotSetGlobalColorAttrib(dialog_manager, XmNforeground, "DLGFGCOLOR");
    IupSetGlobal("_IUP_RESET_DLGBGCOLOR", NULL);
  }

  if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
    XtAddEventHandler(dialog_manager, PointerMotionMask, False, (XtEventHandler)iupmotDummyPointerMotionEvent, NULL);

  return IUP_NOERROR;
}

static void motDialogUnMapMethod(Ihandle* ih)
{
  Widget dialog_manager;
  if (ih->data->menu) 
  {
    ih->data->menu->handle = NULL; /* the dialog will destroy the native menu */
    IupDestroy(ih->data->menu);  
    ih->data->menu = NULL;
  }

  dialog_manager = (Widget)iupAttribGet(ih, "_IUPMOT_DLGCONTAINER");
  XtVaSetValues(dialog_manager, XmNuserData, NULL, NULL);

  XtRemoveEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtRemoveEventHandler(ih->handle, KeyPressMask, False, (XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);
  XtRemoveEventHandler(ih->handle, StructureNotifyMask, False, (XtEventHandler)motDialogCBStructureNotifyEvent, (XtPointer)ih);
  XtRemoveCallback(ih->handle, XmNdestroyCallback, (XtCallbackProc)motDialogDestroyCallback, (XtPointer)ih);

  XtRemoveEventHandler(dialog_manager, KeyPressMask, False, (XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);
  XtRemoveCallback(dialog_manager, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);
  
  iupdrvBaseUnMapMethod(ih);
}

static void motDialogLayoutUpdateMethod(Ihandle *ih)
{
  int border, caption, menu;

  if (ih->data->ignore_resize ||
      iupAttribGet(ih, "_IUPMOT_FS_STYLE"))
    return;

  /* for dialogs the position is not updated here */
  ih->data->ignore_resize = 1;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    int width = ih->currentwidth - 2*border;
    int height = ih->currentheight - 2*border - caption;
    XtVaSetValues(ih->handle,
      XmNwidth, width,  /* client size */
      XmNheight, height,
      XmNminWidth, width, 
      XmNminHeight, height, 
      XmNmaxWidth, width, 
      XmNmaxHeight, height, 
      NULL);
  }
  else
  {
    XtVaSetValues(ih->handle,
      XmNwidth, (XtArgVal)(ih->currentwidth - 2*border),     /* excluding the border */
      XmNheight, (XtArgVal)(ih->currentheight - 2*border - caption),
      NULL);
  }

  ih->data->ignore_resize = 0;
}

/***************************************************************/

void iupdrvDialogInitClass(Iclass* ic)
{
  /* Driver Dependent Class methods */
  ic->Map = motDialogMapMethod;
  ic->UnMap = motDialogUnMapMethod;
  ic->LayoutUpdate = motDialogLayoutUpdateMethod;
  ic->SetChildrenPosition = motDialogSetChildrenPositionMethod;
  ic->GetInnerNativeContainerHandle = motDialogGetInnerNativeContainerHandleMethod;

  if (!iupmot_wm_deletewindow)
  {
    /* Set up a translation table that captures "Resize" events
      (also called ConfigureNotify or Configure events). */
    XtActionsRec rec = {"iupDialogConfigure", motDialogConfigureNotify};
    XtAppAddActions(iupmot_appcontext, &rec, 1);

    /* register atom to intercept the close button in the window frame */
    iupmot_wm_deletewindow = XmInternAtom(iupmot_display, "WM_DELETE_WINDOW", False);
  }

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motDialogSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", motDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);  /* dialog is the only not read-only */
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", motDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", NULL, motDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupDialog only */
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, motDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ICON", NULL, motDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, motDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, motDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, motDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);

  /* IupDialog X Only */
  iupClassRegisterAttribute(ic, "XWINDOW", iupmotGetXWindowAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_STRING);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "OPACITY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

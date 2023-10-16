/** \file
 * \brief Menu Resources
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_dlglist.h"
#include "iup_focus.h"
#include "iup_menu.h"
#include "iup_drv.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"
#include "iupwin_str.h"


Ihandle* iupwinMenuGetHandle(HMENU hMenu)
{
  MENUINFO menuinfo;
  menuinfo.cbSize = sizeof(MENUINFO);
  menuinfo.fMask = MIM_MENUDATA;
  if (GetMenuInfo(hMenu, &menuinfo))
    return (Ihandle*)menuinfo.dwMenuData;
  else
    return NULL;
}

Ihandle* iupwinMenuGetItemHandle(HMENU hMenu, int menuId)
{
  MENUITEMINFO menuiteminfo;
  menuiteminfo.cbSize = sizeof(MENUITEMINFO);
  menuiteminfo.fMask = MIIM_DATA;

  if (GetMenuItemInfo(hMenu, menuId, FALSE, &menuiteminfo))
    return (Ihandle*)menuiteminfo.dwItemData;
  else
    return NULL;
}

IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  (void)ih;
  return GetSystemMetrics(SM_CYMENU);
}

static void winMenuUpdateBar(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih) && ih->parent->handle)
    DrawMenuBar(ih->parent->handle);
  else if (ih->parent)
  {
    ih = ih->parent; /* check also for children of a menu bar */
    if (iupMenuIsMenuBar(ih) && ih->parent->handle)
      DrawMenuBar(ih->parent->handle);
  }
}

static void winMenuGetLastPos(Ihandle* ih, int *last_pos, int *pos)
{
  Ihandle* child;
  *last_pos=0;
  *pos=0;
  for (child=ih->parent->firstchild; child; child=child->brother)
  {
    if (child == ih)
      *pos = *last_pos;
    (*last_pos)++;
  }
}

static void winItemCheckToggle(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    int last_pos, pos;
    winMenuGetLastPos(ih, &last_pos, &pos);
    CheckMenuRadioItem((HMENU)ih->handle, 0, last_pos, pos, MF_BYPOSITION);

    winMenuUpdateBar(ih);
  }
  else if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
  {
    if (GetMenuState((HMENU)ih->handle, (UINT)ih->serial, MF_BYCOMMAND) & MF_CHECKED)
      CheckMenuItem((HMENU)ih->handle, (UINT)ih->serial, MF_UNCHECKED|MF_BYCOMMAND);
    else
      CheckMenuItem((HMENU)ih->handle, (UINT)ih->serial, MF_CHECKED|MF_BYCOMMAND);

    winMenuUpdateBar(ih);
  }
}

void iupwinMenuDialogProc(Ihandle* ih_dialog, UINT msg, WPARAM wp, LPARAM lp)
{
  /* called only from winDialogBaseProc */

  switch (msg)
  {
  case WM_INITMENUPOPUP:
    {
      HMENU hMenu = (HMENU)wp;
      Ihandle *ih = iupwinMenuGetHandle(hMenu);
      if (ih)
      {
        Icallback cb = (Icallback)IupGetCallback(ih, "OPEN_CB");
        if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "OPEN_CB");  /* check also in the Submenu */
        if (cb) cb(ih);
      }
      break;
    }
  case WM_UNINITMENUPOPUP:
    {
      HMENU hMenu = (HMENU)wp;
      Ihandle *ih = iupwinMenuGetHandle(hMenu);
      if (ih)
      {
        Icallback cb = (Icallback)IupGetCallback(ih, "MENUCLOSE_CB");
        if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "MENUCLOSE_CB");  /* check also in the Submenu */
        if (cb) cb(ih);
      }
      break;
    }
  case WM_MENUSELECT:
    {
      HMENU hMenu = (HMENU)lp;
      Ihandle *ih;

      if (!lp)
        break;

      if ((HIWORD(wp) & MF_POPUP) || (HIWORD(wp) & MF_SYSMENU)) /* drop-down ih or submenu. */
      {
        UINT menuindex = LOWORD(wp);
        HMENU hsubmenu = GetSubMenu(hMenu, menuindex);
        ih = iupwinMenuGetHandle(hsubmenu);  /* returns the handle of a IupMenu */
        if (ih) ih = ih->parent;  /* gets the submenu */
      }
      else /* ih item */
      {
        UINT menuID = LOWORD(wp);
        ih = iupwinMenuGetItemHandle(hMenu, menuID);
      }

      if (ih)
      {
        Icallback cb = IupGetCallback(ih, "HIGHLIGHT_CB");
        if (cb) cb(ih);
      }
      break;
    }
  case WM_MENUCOMMAND:
    {
      int menuId = GetMenuItemID((HMENU)lp, (int)wp);
      Icallback cb;
      Ihandle* ih;
        
      if (menuId >= IUP_MDI_FIRSTCHILD)
        break;
        
      ih  = iupwinMenuGetItemHandle((HMENU)lp, menuId);
      if (!ih)
        break;

      winItemCheckToggle(ih);

      cb = IupGetCallback(ih, "ACTION");
      if (cb && cb(ih) == IUP_CLOSE)
        IupExitLoop();

      break;
    }
  case WM_ENTERMENULOOP:
    {
      /* Simulate WM_KILLFOCUS when the menu interaction is started */
      Ihandle* lastfocus = (Ihandle*)iupAttribGet(ih_dialog, "_IUPWIN_LASTFOCUS");
      if (iupObjectCheck(lastfocus)) iupCallKillFocusCb(lastfocus);
      break;
    }
  case WM_EXITMENULOOP:
    {
      /* Simulate WM_GETFOCUS when the menu interaction is stopped */
      Ihandle* lastfocus = (Ihandle*)iupAttribGet(ih_dialog, "_IUPWIN_LASTFOCUS");
      if (iupObjectCheck(lastfocus)) 
        iupCallGetFocusCb(lastfocus);
      break;
    }
  }
}

static int iwinMenuGetPopupAlign(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "POPUPALIGN");
  if (value)
  {
    int horiz_alignment, vert_alignment;
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, value2, ':');

    horiz_alignment = TPM_LEFTALIGN;
    if (iupStrEqualNoCase(value1, "ARIGHT"))
      horiz_alignment = TPM_RIGHTALIGN;
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      horiz_alignment = TPM_CENTERALIGN;

    vert_alignment = TPM_TOPALIGN;
    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      vert_alignment = TPM_BOTTOMALIGN;
    else if (iupStrEqualNoCase(value2, "ACENTER"))
      vert_alignment = TPM_VCENTERALIGN;

    return horiz_alignment | vert_alignment;
  }

  return TPM_LEFTALIGN;
}

int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  HWND hWndActive = GetActiveWindow();
  int tray_menu = 0;
  int menuId;
  int align;

  if (!hWndActive)
  {
    /* search for a valid handle */
    Ihandle* dlg = iupDlgListFirst();
    while (dlg)
    {
      if (dlg->handle)
      {
        hWndActive = dlg->handle;  /* found a valid handle */

        /* if not a "TRAY" dialog, keep searching, because TRAY is a special case */
        if (iupAttribGetBoolean(dlg, "TRAY")) 
          break;
      }
      dlg = iupDlgListNext();
    } 
  }

  /* Necessary to avoid tray dialogs to lock popup menus (they get sticky after the 1st time) */
  if (hWndActive)
  {
    Ihandle* dlg = iupwinHandleGet(hWndActive);
    if (iupObjectCheck(dlg) && iupAttribGetBoolean(dlg, "TRAY"))
    {
      /* To display a context menu for a notification icon, 
         the current window must be the foreground window. */
      SetForegroundWindow(hWndActive);
      tray_menu = 1;
    }
  }

  align = iwinMenuGetPopupAlign(ih);

  /* stop processing here. messages will not go to the message loop */
  menuId = TrackPopupMenu((HMENU)ih->handle, align |TPM_RIGHTBUTTON|TPM_RETURNCMD, x, y, 0, hWndActive, NULL);

  if (tray_menu)
  {
    /* You must force a task switch to the application that 
       called TrackPopupMenu at some time in the near future. 
       This is done by posting a benign message to the window. */
    PostMessage(hWndActive, WM_NULL, 0, 0);
  }

  if (menuId)
  {
    Icallback cb;
    Ihandle* ih_item = iupwinMenuGetItemHandle((HMENU)ih->handle, menuId);
    if (!ih_item) return IUP_NOERROR;

    winItemCheckToggle(ih_item);

    cb = IupGetCallback(ih_item, "ACTION");
    if (cb && cb(ih_item) == IUP_CLOSE)
      IupExitLoop();
  }

  return IUP_NOERROR;
}


/*******************************************************************************************/


static int winMenuSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  /* must use IupGetAttribute to use inheritance */
  if (iupStrToRGB(value, &r, &g, &b))
  {
    MENUINFO menuinfo;
    menuinfo.cbSize = sizeof(MENUINFO);
    menuinfo.fMask = MIM_BACKGROUND;
    menuinfo.hbrBack = iupwinBrushGet(RGB(r,g,b)); 
    SetMenuInfo((HMENU)ih->handle, &menuinfo);
    winMenuUpdateBar(ih);
  }
  return 1;
}

static int winSubmenuAddToParent(Ihandle* ih)
{
  int pos;

  pos = IupGetChildPos(ih->parent, ih);
  ih->serial = iupMenuGetChildId(ih);

  {
    MENUITEMINFO menuiteminfo;

    menuiteminfo.cbSize = sizeof(MENUITEMINFO); 
    menuiteminfo.fMask = MIIM_ID|MIIM_DATA|MIIM_SUBMENU|MIIM_STRING; 
    menuiteminfo.dwTypeData = TEXT(""); /* must set or it will be not possible to update */
    menuiteminfo.cch = 0;
    menuiteminfo.wID = (UINT)ih->serial;
    menuiteminfo.dwItemData = (ULONG_PTR)ih; 
    menuiteminfo.hSubMenu = (HMENU)ih->firstchild->handle;  /* this is why the submenu is created only here with the child menu handle */

    if (!InsertMenuItem((HMENU)ih->parent->handle, pos, TRUE, &menuiteminfo))
      return IUP_ERROR;
  }

  /* Notice that "handle" here is the HMENU of the parent menu,
  and "serial" identifies the submenu */

  ih->handle = ih->parent->handle; /* gets the HMENU of the parent */

  /* must update attributes since submenu is actually created after it is mapped */
  iupAttribUpdate(ih); 
  iupAttribUpdateFromParent(ih);

  winMenuUpdateBar(ih);

  return IUP_NOERROR;
}

static void winMenuChildUnMapMethod(Ihandle* ih)
{
  RemoveMenu((HMENU)ih->handle, (UINT)ih->serial, MF_BYCOMMAND);
}

static void winMenuUnMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    SetMenu(ih->parent->handle, NULL);
    ih->parent = NULL;
  }

  if (!iupMenuIsMenuBar(ih) && ih->parent)
  {
    /* parent is a submenu, it is destroyed here */
    RemoveMenu((HMENU)ih->parent->handle, (UINT)ih->parent->serial, MF_BYCOMMAND);
  }

  DestroyMenu((HMENU)ih->handle);   /* DestroyMenu is recursive */
}

static int winMenuMapMethod(Ihandle* ih)
{
  MENUINFO menuinfo;

  if (iupMenuIsMenuBar(ih))
  {
    /* top level menu used for MENU attribute in IupDialog (a menu bar) */

    ih->handle = (InativeHandle*)CreateMenu();
    if (!ih->handle)
      return IUP_ERROR;

    SetMenu(ih->parent->handle, (HMENU)ih->handle);
  }
  else
  {
    if (ih->parent)
    {
      /* parent is a submenu, it is created here */

      ih->handle = (InativeHandle*)CreatePopupMenu();
      if (!ih->handle)
        return IUP_ERROR;

      if (winSubmenuAddToParent(ih->parent) == IUP_ERROR)
      {
        DestroyMenu((HMENU)ih->handle);
        return IUP_ERROR;
      }
    }
    else  
    {
      /* top level menu used for IupPopup */

      ih->handle = (InativeHandle*)CreatePopupMenu();
      if (!ih->handle)
        return IUP_ERROR;

      iupAttribSet(ih, "_IUPWIN_POPUP_MENU", "1");
    }
  }

  menuinfo.cbSize = sizeof(MENUINFO);
  menuinfo.fMask = MIM_MENUDATA;
  menuinfo.dwMenuData = (ULONG_PTR)ih;

  if (!iupAttribGetInherit(ih, "_IUPWIN_POPUP_MENU"))   /* check in the top level menu using inheritance */
  {
    menuinfo.fMask |= MIM_STYLE;
    menuinfo.dwStyle = MNS_NOTIFYBYPOS;
  }

  SetMenuInfo((HMENU)ih->handle, &menuinfo);

  winMenuUpdateBar(ih);

  return IUP_NOERROR;
}

void iupdrvMenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winMenuMapMethod;
  ic->UnMap = winMenuUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winMenuSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "MENUBGCOLOR", IUPAF_DEFAULT);
}


/*******************************************************************************************/


static int winItemSetImageAttrib(Ihandle* ih, const char* value)
{
  HBITMAP hBitmapUnchecked, hBitmapChecked;
  char* impress;

  /* check if the submenu handle was created in winSubmenuAddToParent */
  if (ih->handle == (InativeHandle*)-1) 
    return 1;

  hBitmapUnchecked = iupImageGetImage(value, ih, 0, NULL);

  impress = iupAttribGet(ih, "IMPRESS");
  if (impress)
    hBitmapChecked = iupImageGetImage(impress, ih, 0, NULL);
  else
    hBitmapChecked = hBitmapUnchecked;

  SetMenuItemBitmaps((HMENU)ih->handle, (UINT)ih->serial, MF_BYCOMMAND, hBitmapUnchecked, hBitmapChecked);

  winMenuUpdateBar(ih);

  return 1;
}

static int winItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  HBITMAP hBitmapUnchecked, hBitmapChecked;

  char *image = iupAttribGet(ih, "IMPRESS");
  hBitmapUnchecked = iupImageGetImage(image, ih, 0, NULL);

  if (value)
    hBitmapChecked = iupImageGetImage(value, ih, 0, NULL);
  else
    hBitmapChecked = hBitmapUnchecked;

  SetMenuItemBitmaps((HMENU)ih->handle, (UINT)ih->serial, MF_BYCOMMAND, hBitmapUnchecked, hBitmapChecked);

  winMenuUpdateBar(ih);

  return 1;
}

static int winItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  char *str;

  /* check if the submenu handle was created in winSubmenuAddToParent */
  if (ih->handle == (InativeHandle*)-1)
    return 1;

  if (!value)
  {
    str = "     ";
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  {
    TCHAR* tstr = iupwinStrToSystem(str);
    int len = lstrlen(tstr);
    MENUITEMINFO menuiteminfo;
    menuiteminfo.cbSize = sizeof(MENUITEMINFO); 
    menuiteminfo.fMask = MIIM_TYPE;
    menuiteminfo.fType = MFT_STRING;
    menuiteminfo.dwTypeData = tstr;
    menuiteminfo.cch = len;

    SetMenuItemInfo((HMENU)ih->handle, (UINT)ih->serial, FALSE, &menuiteminfo);
  }

  if (str != value) free(str);

  winMenuUpdateBar(ih);

  return 1;
}

static int winItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  HBITMAP hBitmap;

  /* check if the submenu handle was created in winSubmenuAddToParent */
  if (ih->handle == (InativeHandle*)-1)
    return 1;

  hBitmap = iupImageGetImage(value, ih, 0, NULL);

  {
    MENUITEMINFO menuiteminfo;
    menuiteminfo.cbSize = sizeof(MENUITEMINFO); 
    menuiteminfo.fMask = MIIM_BITMAP; 
    menuiteminfo.hbmpItem = hBitmap;
    SetMenuItemInfo((HMENU)ih->handle, (UINT)ih->serial, FALSE, &menuiteminfo);
  }

  winMenuUpdateBar(ih);

  return 1;
}

static int winItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* check if the submenu handle was created in winSubmenuAddToParent */
  if (ih->handle == (InativeHandle*)-1)
    return 1;

  if (iupStrBoolean(value))
    EnableMenuItem((HMENU)ih->handle, (UINT)ih->serial, MF_ENABLED|MF_BYCOMMAND);
  else
    EnableMenuItem((HMENU)ih->handle, (UINT)ih->serial, MF_GRAYED|MF_BYCOMMAND);

  winMenuUpdateBar(ih);

  return 0;
}

static char* winItemGetActiveAttrib(Ihandle* ih)
{
  /* check if the submenu handle was created in winSubmenuAddToParent */
  if (ih->handle == (InativeHandle*)-1)
    return NULL;

  return iupStrReturnBoolean(!(GetMenuState((HMENU)ih->handle, (UINT)ih->serial, MF_BYCOMMAND) & MF_GRAYED));
}

static int winItemSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    int last_pos, pos;
    winMenuGetLastPos(ih, &last_pos, &pos);
    CheckMenuRadioItem((HMENU)ih->handle, 0, last_pos, pos, MF_BYPOSITION);
  }
  else
  {
    if (iupStrBoolean(value))
      CheckMenuItem((HMENU)ih->handle, (UINT)ih->serial, MF_CHECKED|MF_BYCOMMAND);
    else
      CheckMenuItem((HMENU)ih->handle, (UINT)ih->serial, MF_UNCHECKED|MF_BYCOMMAND);
  }

  winMenuUpdateBar(ih);

  return 0;
}

static char* winItemGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnChecked(GetMenuState((HMENU)ih->handle, (UINT)ih->serial, MF_BYCOMMAND) & MF_CHECKED);
}

static int winItemMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent || !IsMenu((HMENU)ih->parent->handle))
    return IUP_ERROR;

  pos = IupGetChildPos(ih->parent, ih);
  ih->serial = iupMenuGetChildId(ih);

  {
    MENUITEMINFO menuiteminfo;
    menuiteminfo.cbSize = sizeof(MENUITEMINFO); 
    menuiteminfo.fMask = MIIM_ID|MIIM_DATA|MIIM_STRING; 
    menuiteminfo.dwTypeData = TEXT(""); /* must set or it will be not possible to update */
    menuiteminfo.cch = 0;
    menuiteminfo.wID = (UINT)ih->serial;
    menuiteminfo.dwItemData = (ULONG_PTR)ih; 

    if (!InsertMenuItem((HMENU)ih->parent->handle, pos, TRUE, &menuiteminfo))
      return IUP_ERROR;
  }

  /* Notice that "handle" here is the HMENU of the parent menu,
  and "serial" identifies the menu item */

  ih->handle = ih->parent->handle; /* gets the HMENU of the parent */
  winMenuUpdateBar(ih);

  return IUP_NOERROR;
}

void iupdrvItemInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winItemMapMethod;
  ic->UnMap = winMenuChildUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", winItemGetActiveAttrib, winItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "MENUBGCOLOR", IUPAF_DEFAULT);  /* used by IupImage */

  /* IupItem only */
  iupClassRegisterAttribute(ic, "VALUE", winItemGetValueAttrib, winItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, winItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, winItemSetTitleImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, winItemSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* necessary because transparent background does not work when not using visual styles */
  if (!iupwin_comctl32ver6)  /* Used by iupdrvImageCreateImage */
    iupClassRegisterAttribute(ic, "FLAT_ALPHA", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}


/*******************************************************************************************/


static int winSubmenuMapMethod(Ihandle* ih)
{
  if (!ih->parent || !IsMenu((HMENU)ih->parent->handle))
    return IUP_ERROR;

  /* will map as void here, but later when the "child" menu is mapped 
     the submenu handle receives the "parent" menu handle in winSubmenuAddToParent,
     because the submenu needs the child menu to be created in the native system. */
  return iupBaseTypeVoidMapMethod(ih);
}

void iupdrvSubmenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winSubmenuMapMethod;

  /* IupSubmenu only */
  iupClassRegisterAttribute(ic, "ACTIVE", winItemGetActiveAttrib, winItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, winItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "MENUBGCOLOR", IUPAF_DEFAULT);  /* used by IupImage */

  /* necessary because transparent background does not work when not using visual styles */
  if (!iupwin_comctl32ver6)  /* Used by iupdrvImageCreateImage */
    iupClassRegisterAttribute(ic, "FLAT_ALPHA", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}


/*******************************************************************************************/


static int winSeparatorMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent || !IsMenu((HMENU)ih->parent->handle))
    return IUP_ERROR;

  pos = IupGetChildPos(ih->parent, ih);
  ih->serial = iupMenuGetChildId(ih);

  {
    MENUITEMINFO menuiteminfo;
    menuiteminfo.cbSize = sizeof(MENUITEMINFO); 
    menuiteminfo.fMask = MIIM_FTYPE|MIIM_ID|MIIM_DATA; 
    menuiteminfo.fType = MFT_SEPARATOR; 
    menuiteminfo.wID = (UINT)ih->serial;
    menuiteminfo.dwItemData = (ULONG_PTR)ih; 

    if (!InsertMenuItem((HMENU)ih->parent->handle, pos, TRUE, &menuiteminfo))
      return IUP_ERROR;
  }

  /* Notice that "handle" here is the HMENU of the parent menu,
  and "serial" identifies the menu separator */

  ih->handle = ih->parent->handle; /* gets the HMENU of the parent */
  winMenuUpdateBar(ih);

  return IUP_NOERROR;
}

void iupdrvSeparatorInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winSeparatorMapMethod;
  ic->UnMap = winMenuChildUnMapMethod;
}

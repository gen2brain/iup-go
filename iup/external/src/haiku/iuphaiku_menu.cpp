/** \file
 * \brief Haiku Menu Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Application.h>
#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <SeparatorItem.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_menu.h"
}

#include "iuphaiku_drv.h"


/* BMessage what + field for IupItem dispatch (constant in drv.h). */
static const char* kIupMenuItemField = "ih";

/* Hop off menu_tracking / BMenuWindow loopers onto the dialog looper. */
static void haikuMenuPostCallback(Ihandle* ih, const char* cb_name)
{
  if (!ih) return;
  Ihandle* dlg = IupGetDialog(ih);
  if (!dlg || !dlg->handle) return;
  BMessage m(IUPHAIKU_MENU_CB_MSG);
  m.AddPointer("ih", ih);
  m.AddString("cb", cb_name);
  BMessenger((BWindow*)dlg->handle).SendMessage(&m);
}

static BMenu* haikuMenuParentBMenu(Ihandle* ih)
{
  if (!ih || !ih->parent || !ih->parent->handle)
    return NULL;
  return (BMenu*)ih->parent->handle;
}

static BWindow* haikuMenuOwningWindow(Ihandle* ih)
{
  /* Walk up to the dialog Ihandle (TYPEDIALOG holds BWindow*). */
  Ihandle* dlg = IupGetDialog(ih);
  if (!dlg || !dlg->handle)
    return NULL;
  return (BWindow*)dlg->handle;
}

static char* haikuStrippedMnemonic(const char* title)
{
  if (!title) return NULL;
  if (!strchr(title, '&')) return iupStrDup(title);
  return iupStrProcessMnemonic(title, NULL, 0);
}

/* "Label\tShortcut": label gets the mnemonic pass, shortcut goes to SetShortcut. */
static char* haikuItemSplitTitle(const char* value, const char** shortcut)
{
  *shortcut = NULL;
  if (!value) return iupStrDup("");
  const char* sep = strchr(value, '\t');
  if (!sep) return haikuStrippedMnemonic(value);
  *shortcut = sep + 1;
  int len = (int)(sep - value);
  char* label = (char*)malloc(len + 1);
  memcpy(label, value, len);
  label[len] = '\0';
  char* stripped = haikuStrippedMnemonic(label);
  free(label);
  return stripped;
}

/* Parse "Ctrl+S" / "Shift+Alt+F1" / "Cmd+Q" / "S" into BeOS char + modifier mask.
 * Returns 0 if the trailing token can't fit BMenuItem::SetShortcut (only printable chars are supported by that API). */
static int haikuMenuItemParseShortcut(const char* str, char* out_ch, uint32* out_mods)
{
  if (!*str) return 0;
  uint32 mods = 0;
  char* tmp = iupStrDup(str);
  char* tok = tmp;
  char ch = 0;

  while (tok && *tok)
  {
    char* plus = strchr(tok, '+');
    if (plus) *plus = '\0';

    while (*tok == ' ' || *tok == '\t') tok++;

    if      (iupStrEqualNoCase(tok, "Ctrl"))     mods |= B_CONTROL_KEY;
    else if (iupStrEqualNoCase(tok, "Shift"))    mods |= B_SHIFT_KEY;
    else if (iupStrEqualNoCase(tok, "Alt"))      mods |= B_OPTION_KEY;
    else if (iupStrEqualNoCase(tok, "Cmd") ||
             iupStrEqualNoCase(tok, "Sys") ||
             iupStrEqualNoCase(tok, "Meta"))     mods |= B_COMMAND_KEY;
    else if (tok[0] && !tok[1])                  ch = tok[0];

    tok = plus ? plus + 1 : NULL;
  }
  free(tmp);

  if (!ch) return 0;
  *out_ch = ch;
  *out_mods = mods;
  return 1;
}

static void haikuItemApplyKey(BMenuItem* item, const char* value)
{
  if (!item) return;
  if (!value || !*value) { item->SetShortcut(0, 0); return; }
  char ch; uint32 mods;
  if (haikuMenuItemParseShortcut(value, &ch, &mods))
    item->SetShortcut(ch, mods);
}


class IupHaikuMenu : public BMenu
{
public:
  explicit IupHaikuMenu(const char* name) : BMenu(name), fIhandle(NULL) {}

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

  void AttachedToWindow() override
  {
    BMenu::AttachedToWindow();
    if (fIhandle) haikuMenuPostCallback(fIhandle, "MENUOPEN_CB");
  }

  void DetachedFromWindow() override
  {
    if (fIhandle) haikuMenuPostCallback(fIhandle, "MENUCLOSE_CB");
    BMenu::DetachedFromWindow();
  }

private:
  Ihandle* fIhandle;
};

/* Custom BMenuItem subclass: per-item icon + HIGHLIGHT_CB hook. */

class IupHaikuMenuItem : public BMenuItem
{
public:
  IupHaikuMenuItem(Ihandle* ih, const char* label, BMessage* msg)
    : BMenuItem(label, msg), fIhandle(ih), fIcon(NULL) {}

  /* Submenu superitem flavour: BMenuItem(BMenu*) is the canonical "this item
   * opens that submenu" constructor. */
  IupHaikuMenuItem(Ihandle* ih, BMenu* submenu)
    : BMenuItem(submenu, NULL), fIhandle(ih), fIcon(NULL) {}

  void SetIcon(BBitmap* icon) { fIcon = icon; }

protected:
  void GetContentSize(float* w, float* h) override
  {
    BMenuItem::GetContentSize(w, h);
    if (fIcon)
    {
      float iw = fIcon->Bounds().Width() + 1;
      float ih = fIcon->Bounds().Height() + 1;
      *w += iw + 6;
      if (ih > *h) *h = ih;
    }
  }

  void DrawContent() override
  {
    BMenu* menu = Menu();
    if (fIcon && menu)
    {
      BPoint loc = ContentLocation();
      menu->SetDrawingMode(B_OP_ALPHA);
      menu->DrawBitmap(fIcon, loc);
      menu->SetDrawingMode(B_OP_COPY);
      menu->MovePenTo(loc.x + fIcon->Bounds().Width() + 1 + 6, menu->PenLocation().y);
    }
    BMenuItem::DrawContent();
  }

  void Highlight(bool highlight) override
  {
    BMenuItem::Highlight(highlight);
    if (fIhandle && highlight) haikuMenuPostCallback(fIhandle, "HIGHLIGHT_CB");
  }

private:
  Ihandle* fIhandle;
  BBitmap* fIcon;
};


static BBitmap* haikuItemBitmapByName(Ihandle* ih, const char* name)
{
  return (name && *name) ? (BBitmap*)iupImageGetImage(name, ih, 0, NULL) : NULL;
}

static void haikuItemRefreshIcon(IupHaikuMenuItem* item, Ihandle* ih)
{
  if (!item) return;
  const char* name = NULL;
  if (iupAttribGetBoolean(ih, "VALUE"))
    name = iupAttribGet(ih, "IMPRESS");
  if (!name)
    name = iupAttribGet(ih, "IMAGE");
  item->SetIcon(haikuItemBitmapByName(ih, name));
  if (item->Menu()) item->Menu()->InvalidateLayout();
}

/* IupItem */

static int haikuItemMapMethod(Ihandle* ih)
{
  BMenu* parent = haikuMenuParentBMenu(ih);
  if (!parent) return IUP_ERROR;

  const char* shortcut = NULL;
  char* label = haikuItemSplitTitle(iupAttribGet(ih, "TITLE"), &shortcut);

  BMessage* msg = new BMessage(IUPHAIKU_MENU_ITEM_MSG);
  msg->AddPointer(kIupMenuItemField, ih);

  IupHaikuMenuItem* item = new IupHaikuMenuItem(ih, label ? label : "", msg);
  if (label) free(label);

  BWindow* win = haikuMenuOwningWindow(ih);
  if (win) item->SetTarget(BMessenger(win));
  else if (be_app) item->SetTarget(BMessenger(be_app));

  if (iupStrBoolean(iupAttribGet(ih, "VALUE")) &&
      !iupAttribGetBoolean(ih, "HIDEMARK"))
    item->SetMarked(true);

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    item->SetEnabled(false);

  haikuItemRefreshIcon(item, ih);

  char* key = iupAttribGet(ih, "KEY");
  if (key) haikuItemApplyKey(item, key);
  else if (shortcut) haikuItemApplyKey(item, shortcut);

  parent->AddItem(item);
  ih->handle = (InativeHandle*)item;
  return IUP_NOERROR;
}

/* No LooperLockGuard on item->Menu()->Window(): cross-window inversion with menu_tracking. */

static int haikuItemSetKeyAttrib(Ihandle* ih, const char* value)
{
  BMenuItem* item = (BMenuItem*)ih->handle;
  if (!item) return 1;
  haikuItemApplyKey(item, value);
  return 1;
}

static int haikuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  BMenuItem* item = (BMenuItem*)ih->handle;
  if (!item) return 1;
  const char* shortcut = NULL;
  char* label = haikuItemSplitTitle(value, &shortcut);
  item->SetLabel(label ? label : "");
  if (label) free(label);
  /* KEY wins; otherwise the title's "Label\tShortcut" tail sets (or clears) the binding. */
  if (!iupAttribGet(ih, "KEY"))
    haikuItemApplyKey(item, shortcut);
  return 1;
}

static int haikuItemSetValueAttrib(Ihandle* ih, const char* value)
{
  IupHaikuMenuItem* item = dynamic_cast<IupHaikuMenuItem*>((BMenuItem*)ih->handle);
  if (!item) return 1;
  iupAttribSetStr(ih, "VALUE", value);
  if (iupAttribGetBoolean(ih, "HIDEMARK"))
    item->SetMarked(false);
  else
    item->SetMarked(iupStrBoolean(value));
  haikuItemRefreshIcon(item, ih);
  return 1;
}

static int haikuItemSetImageAttrib(Ihandle* ih, const char* value)
{
  IupHaikuMenuItem* item = dynamic_cast<IupHaikuMenuItem*>((BMenuItem*)ih->handle);
  if (!item) return 1;
  iupAttribSetStr(ih, "IMAGE", value);
  haikuItemRefreshIcon(item, ih);
  return 1;
}

static int haikuItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  IupHaikuMenuItem* item = dynamic_cast<IupHaikuMenuItem*>((BMenuItem*)ih->handle);
  if (!item) return 1;
  iupAttribSetStr(ih, "IMPRESS", value);
  haikuItemRefreshIcon(item, ih);
  return 1;
}

static int haikuItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  BMenuItem* item = (BMenuItem*)ih->handle;
  if (item) item->SetEnabled(iupStrBoolean(value));
  return iupBaseSetActiveAttrib(ih, value);
}

/* IupSeparator */

static int haikuSeparatorMapMethod(Ihandle* ih)
{
  BMenu* parent = haikuMenuParentBMenu(ih);
  if (!parent) return IUP_ERROR;

  BSeparatorItem* sep = new BSeparatorItem();
  parent->AddItem(sep);
  ih->handle = (InativeHandle*)sep;
  return IUP_NOERROR;
}

/* IupSubmenu */

static int haikuSubmenuMapMethod(Ihandle* ih)
{
  BMenu* parent = haikuMenuParentBMenu(ih);
  if (!parent) return IUP_ERROR;

  char* title = iupAttribGet(ih, "TITLE");
  char* stripped = haikuStrippedMnemonic(title);

  IupHaikuMenu* submenu = new IupHaikuMenu(stripped ? stripped : "");
  if (stripped) free(stripped);

  /* Use our custom-item flavor as the superitem so per-submenu IMAGE/
   * TITLEIMAGE support comes along for free. */
  IupHaikuMenuItem* super = new IupHaikuMenuItem(ih, submenu);
  char* image = iupAttribGet(ih, "IMAGE");
  if (image) super->SetIcon(haikuItemBitmapByName(ih, image));

  parent->AddItem(super);
  ih->handle = (InativeHandle*)submenu;
  iupAttribSet(ih, "_IUPHAIKU_SUBMENU_SUPER", (char*)super);

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    submenu->SetEnabled(false);

  return IUP_NOERROR;
}

static int haikuSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  BMenu* m = (BMenu*)ih->handle;
  if (!m) return 1;
  char* stripped = haikuStrippedMnemonic(value);
  /* BMenu::SetName isn't exposed; reach through the superitem instead. */
  BMenuItem* super = m->Superitem();
  if (super) super->SetLabel(stripped ? stripped : "");
  if (stripped) free(stripped);
  return 1;
}

static int haikuSubmenuSetActiveAttrib(Ihandle* ih, const char* value)
{
  BMenu* m = (BMenu*)ih->handle;
  if (m) m->SetEnabled(iupStrBoolean(value));
  return iupBaseSetActiveAttrib(ih, value);
}

static int haikuSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  IupHaikuMenuItem* super = (IupHaikuMenuItem*)iupAttribGet(ih, "_IUPHAIKU_SUBMENU_SUPER");
  if (!super) return 1;
  super->SetIcon(haikuItemBitmapByName(ih, value));
  if (super->Menu()) super->Menu()->InvalidateLayout();
  return 1;
}

/* IupMenu */

static int haikuMenuMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    BWindow* win = (BWindow*)ih->parent->handle;
    if (!win) return IUP_ERROR;

    int menu_h = iupdrvMenuGetMenuBarSize(ih);
    BMenuBar* menubar;
    {
      LooperLockGuard guard(win);
      BRect b = win->Bounds();
      menubar = new BMenuBar(BRect(0, 0, b.Width(), (float)(menu_h - 1)),
                             "iup_menubar",
                             B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
                             B_ITEMS_IN_ROW,
                             false);
      win->AddChild(menubar);

      /* B_FOLLOW_ALL_SIDES on the root view preserves menu_h top offset on resize. */
      BView* root = iuphaikuDialogRootView(win);
      if (root)
      {
        root->MoveTo(0, (float)menu_h);
        root->ResizeTo(b.Width(), b.Height() - menu_h);
      }
    }
    ih->handle = (InativeHandle*)menubar;
    iupAttribSet(ih->parent, "_IUP_DIALOG_HASMENU", "1");
    return IUP_NOERROR;
  }

  /* Menu under Submenu: alias the parent submenu's BMenu so children
   * (Items / Separators / further Submenus) attach to the same node. */
  if (ih->parent && ih->parent->handle &&
      ih->parent->iclass && iupStrEqual(ih->parent->iclass->name, "submenu"))
  {
    ih->handle = ih->parent->handle;
    /* Bind the IupMenu Ihandle so MENUOPEN_CB / MENUCLOSE_CB find their target. */
    if (IupHaikuMenu* m = dynamic_cast<IupHaikuMenu*>((BMenu*)ih->handle))
      m->SetIhandle(ih);
    if (iupAttribGetBoolean(ih, "RADIO"))
      ((BMenu*)ih->handle)->SetRadioMode(true);
    return IUP_NOERROR;
  }

  /* Standalone Menu (used by IupPopup). */
  BPopUpMenu* popup = new BPopUpMenu("iup_popup", false, false);
  if (iupAttribGetBoolean(ih, "RADIO"))
    popup->SetRadioMode(true);
  ih->handle = (InativeHandle*)popup;
  return IUP_NOERROR;
}

static int haikuMenuSetRadioAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  if (iupMenuIsMenuBar(ih)) return 0;  /* meaningless for menubar */
  BMenu* m = (BMenu*)ih->handle;
  m->SetRadioMode(iupStrBoolean(value) ? true : false);
  return 1;
}

static void haikuMenuUnMapMethod(Ihandle* ih)
{
  /* The aliased submenu case must NOT delete - the real owner is the parent. */
  if (ih->parent && ih->parent->handle == ih->handle)
  {
    ih->handle = NULL;
    return;
  }

  if (iupMenuIsMenuBar(ih))
  {
    BMenuBar* mb = (BMenuBar*)ih->handle;
    if (mb)
    {
      BWindow* win = mb->Window();
      if (win)
      {
        LooperLockGuard guard(win);
        mb->RemoveSelf();
        BView* root = iuphaikuDialogRootView(win);
        if (root)
        {
          BRect b = win->Bounds();
          root->MoveTo(0, 0);
          root->ResizeTo(b.Width(), b.Height());
        }
      }
    }
    if (ih->parent) iupAttribSet(ih->parent, "_IUP_DIALOG_HASMENU", NULL);
  }
  else
  {
    /* Standalone popup. */
    delete (BPopUpMenu*)ih->handle;
  }
  ih->handle = NULL;
}

static void haikuItemUnMapMethod(Ihandle* ih)
{
  /* Item is owned by parent BMenu - just clear our pointer. */
  ih->handle = NULL;
}

/* iupdrvMenuPopup: show standalone menu at screen coords. */
extern "C" IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  BPopUpMenu* popup = (BPopUpMenu*)ih->handle;
  if (!popup) return IUP_ERROR;

  /* autoInvoke=false: we dispatch ACTION synchronously to avoid racing with iup.Destroy after IupPopup. */
  BMenuItem* sel = popup->Go(BPoint((float)x, (float)y), false, false, false);
  if (!sel) return IUP_NOERROR;

  BMessage* m = sel->Message();
  if (!m || m->what != IUPHAIKU_MENU_ITEM_MSG) return IUP_NOERROR;

  Ihandle* item_ih = NULL;
  m->FindPointer(kIupMenuItemField, (void**)&item_ih);
  if (!item_ih || !iupObjectCheck(item_ih)) return IUP_NOERROR;

  if (iupAttribGetBoolean(item_ih, "AUTOTOGGLE"))
  {
    int v = iupStrBoolean(iupAttribGet(item_ih, "VALUE"));
    IupSetAttribute(item_ih, "VALUE", v ? "OFF" : "ON");
  }
  Icallback cb = (Icallback)IupGetCallback(item_ih, "ACTION");
  if (cb && cb(item_ih) == IUP_CLOSE) IupExitLoop();
  return IUP_NOERROR;
}

extern "C" IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, NULL, &ch);
  return 4 + ch + 4;
}

/* Class registration */

extern "C" IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = haikuMenuMapMethod;
  ic->UnMap = haikuMenuUnMapMethod;

  /* Inherited by IupSubmenu / IupItem; queried by iupdrvMenuGetMenuBarSize. */
  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "RADIO", NULL, haikuMenuSetRadioAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}

extern "C" IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
  ic->Map = haikuItemMapMethod;
  ic->UnMap = haikuItemUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, haikuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", NULL, haikuItemSetValueAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "KEY", NULL, haikuItemSetKeyAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, haikuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, haikuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMPRESS", NULL, haikuItemSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}

extern "C" IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
  ic->Map = haikuSeparatorMapMethod;
  ic->UnMap = haikuItemUnMapMethod;
}

extern "C" IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = haikuSubmenuMapMethod;
  ic->UnMap = haikuItemUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, haikuSubmenuSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuSubmenuSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, haikuSubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, haikuSubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}

/* IupConfigRecent: dynamic BMenuItems posting IUPHAIKU_MENU_RECENT_MSG */

extern "C" IUP_SDK_API int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

extern "C" IUP_SDK_API int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  if (!menu || !menu->handle) return -1;
  BMenu* bm = (BMenu*)menu->handle;

  int max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  if (count > max_recent) count = max_recent;
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  int prev = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");
  BWindow* win = haikuMenuOwningWindow(menu);
  BMessenger target = win ? BMessenger(win) : (be_app ? BMessenger(be_app) : BMessenger());

  LooperLockGuard guard(bm->Window());

  for (int i = 0; i < prev; i++)
  {
    char an[32];
    snprintf(an, sizeof(an), "_IUP_RECENT_ITEM%d", i);
    BMenuItem* item = (BMenuItem*)iupAttribGet(menu, an);
    if (item) { bm->RemoveItem(item); delete item; }
    iupAttribSet(menu, an, NULL);
    snprintf(an, sizeof(an), "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, an, NULL);
  }

  for (int i = 0; i < count; i++)
  {
    BMessage* msg = new BMessage(IUPHAIKU_MENU_RECENT_MSG);
    msg->AddPointer("menu", menu);
    msg->AddInt32("index", i);
    BMenuItem* item = new BMenuItem(filenames[i], msg);
    if (target.IsValid()) item->SetTarget(target);
    bm->AddItem(item);

    char an[32];
    snprintf(an, sizeof(an), "_IUP_RECENT_ITEM%d", i);
    iupAttribSet(menu, an, (char*)item);
    snprintf(an, sizeof(an), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, an, filenames[i]);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}

IUP_DRV_API void iuphaikuRecentDispatch(Ihandle* menu, int index)
{
  if (!menu || !iupObjectCheck(menu) || index < 0) return;
  Icallback cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  Ihandle* config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");
  if (!cb || !config) return;
  char an[32];
  snprintf(an, sizeof(an), "_IUP_RECENT_FILE%d", index);
  char* filename = iupAttribGet(menu, an);
  if (!filename) return;
  IupSetStrAttribute(config, "RECENTFILENAME", filename);
  IupSetStrAttribute(config, "TITLE", filename);
  config->parent = menu;
  cb(config);
  config->parent = NULL;
  IupSetAttribute(config, "RECENTFILENAME", NULL);
  IupSetAttribute(config, "TITLE", NULL);
}

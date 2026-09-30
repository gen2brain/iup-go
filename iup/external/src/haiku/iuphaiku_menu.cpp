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
#include "iupkey.h"
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
  BMessenger(reinterpret_cast<BWindow*>(dlg->handle)).SendMessage(&m);
}

static BMenu* haikuMenuParentBMenu(Ihandle* ih)
{
  if (!ih || !ih->parent || !ih->parent->handle)
    return nullptr;
  return reinterpret_cast<BMenu*>(ih->parent->handle);
}

static BWindow* haikuMenuOwningWindow(Ihandle* ih)
{
  Ihandle* dlg = IupGetDialog(ih);
  if (!dlg || !dlg->handle)
    return nullptr;
  return reinterpret_cast<BWindow*>(dlg->handle);
}

static char* haikuMenuLabel(Ihandle* ih, const char* title, char* trigger)
{
  if (!title) title = "";
  char* processed = iupMenuProcessTitle(ih, title);
  *trigger = 0;
  char* label = iupStrProcessMnemonic(processed, trigger, -1);
  if (label == processed) label = iupStrDup(processed);
  if (processed != title) free(processed);
  return label;
}

static char* haikuItemSplitTitle(Ihandle* ih, const char* value, char* trigger)
{
  if (!value) value = "";
  const char* sep = strchr(value, '\t');
  if (!sep) return haikuMenuLabel(ih, value, trigger);
  int len = static_cast<int>(sep - value);
  char* title = static_cast<char*>(malloc(len + 1));
  memcpy(title, value, len);
  title[len] = '\0';
  char* label = haikuMenuLabel(ih, title, trigger);
  free(title);
  return label;
}

static void haikuItemApplyKey(Ihandle* ih, BMenuItem* item, const char* title)
{
  int code = IupGetDialog(ih) ? iupMenuGetAccel(title) : 0;
  int base = iup_XkeyBase(code);
  uint32 mods = 0;
  char ch = 0;

  if (!item) return;

  switch (base)
  {
  case 0: break;
  case K_ESC:  ch = B_ESCAPE; break;
  case K_TAB:  ch = B_TAB; break;
  case K_SP:   ch = B_SPACE; break;
  case K_CR:   ch = B_ENTER; break;
  case K_BS:   ch = B_BACKSPACE; break;
  case K_DEL:  ch = B_DELETE; break;
  case K_INS:  ch = B_INSERT; break;
  case K_HOME: ch = B_HOME; break;
  case K_END:  ch = B_END; break;
  case K_PGUP: ch = B_PAGE_UP; break;
  case K_PGDN: ch = B_PAGE_DOWN; break;
  case K_LEFT: ch = B_LEFT_ARROW; break;
  case K_RIGHT: ch = B_RIGHT_ARROW; break;
  case K_UP:   ch = B_UP_ARROW; break;
  case K_DOWN: ch = B_DOWN_ARROW; break;
  default:
    if (base > K_SP && base < 127)
      ch = static_cast<char>(iup_tolower(base));
    break;
  }

  if (!ch)
  {
    item->SetShortcut(0, 0);
    return;
  }

  if (iup_isShiftXkey(code)) mods |= B_SHIFT_KEY;
  if (iup_isAltXkey(code)) mods |= B_CONTROL_KEY;
  if (iup_isSysXkey(code)) mods |= B_OPTION_KEY;
  item->SetShortcut(ch, mods);
}


static void haikuMenuApplyColors(BMenu* menu)
{
  if (iuphaikuColorForced())
  {
    menu->SetViewColor(iuphaikuColor(B_MENU_BACKGROUND_COLOR));
    menu->SetLowColor(iuphaikuColor(B_MENU_BACKGROUND_COLOR));
    menu->SetHighColor(iuphaikuColor(B_MENU_ITEM_TEXT_COLOR));
  }
  else
  {
    menu->SetViewUIColor(B_MENU_BACKGROUND_COLOR);
    menu->SetLowUIColor(B_MENU_BACKGROUND_COLOR);
    menu->SetHighUIColor(B_MENU_ITEM_TEXT_COLOR);
  }
  menu->Invalidate();
}

class IupHaikuMenuBar : public BMenuBar
{
public:
  explicit IupHaikuMenuBar(BRect frame)
    : BMenuBar(frame, "iup_menubar", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_ITEMS_IN_ROW, false) {}

  void AttachedToWindow() override
  {
    BMenuBar::AttachedToWindow();
    haikuMenuApplyColors(this);
  }
};

IUP_DRV_API void iuphaikuMenuBarUpdateColors(Ihandle* ih)
{
  auto* menubar = reinterpret_cast<BMenuBar*>(ih->handle);
  if (!menubar || !menubar->Window())
    return;
  LooperLockGuard guard(menubar->Window());
  haikuMenuApplyColors(menubar);
}

class IupHaikuMenu : public BMenu
{
public:
  explicit IupHaikuMenu(const char* name) : BMenu(name), fIhandle(nullptr) {}

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

  void AttachedToWindow() override
  {
    BMenu::AttachedToWindow();
    haikuMenuApplyColors(this);
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

class IupHaikuMenuItem : public BMenuItem
{
public:
  IupHaikuMenuItem(Ihandle* ih, const char* label, BMessage* msg)
    : BMenuItem(label, msg), fIhandle(ih), fIcon(nullptr) {}

  IupHaikuMenuItem(Ihandle* ih, BMenu* submenu)
    : BMenuItem(submenu, nullptr), fIhandle(ih), fIcon(nullptr) {}

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
    if (menu && iuphaikuColorForced() && IsEnabled())
      menu->SetHighColor(iuphaikuColor(IsSelected() ? B_MENU_SELECTED_ITEM_TEXT_COLOR : B_MENU_ITEM_TEXT_COLOR));
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
  return (name && *name) ? static_cast<BBitmap*>(iupImageGetImage(name, ih, 0, nullptr)) : nullptr;
}

static void haikuItemRefreshIcon(IupHaikuMenuItem* item, Ihandle* ih)
{
  if (!item) return;
  const char* name = nullptr;
  if (iupAttribGetBoolean(ih, "VALUE"))
    name = iupAttribGet(ih, "IMPRESS");
  if (!name)
    name = iupAttribGet(ih, "IMAGE");
  item->SetIcon(haikuItemBitmapByName(ih, name));
  if (item->Menu()) item->Menu()->InvalidateLayout();
}

static int haikuItemMapMethod(Ihandle* ih)
{
  BMenu* parent = haikuMenuParentBMenu(ih);
  if (!parent) return IUP_ERROR;

  char trigger;
  char* label = haikuItemSplitTitle(ih, iupAttribGet(ih, "TITLE"), &trigger);

  auto* msg = new BMessage(IUPHAIKU_MENU_ITEM_MSG);
  msg->AddPointer(kIupMenuItemField, ih);

  auto* item = new IupHaikuMenuItem(ih, label, msg);
  free(label);
  if (trigger) item->SetTrigger(trigger);

  BWindow* win = haikuMenuOwningWindow(ih);
  if (win) item->SetTarget(BMessenger(win));
  else if (be_app) item->SetTarget(BMessenger(be_app));

  if (iupStrBoolean(iupAttribGet(ih, "VALUE")) &&
      !iupAttribGetBoolean(ih, "HIDEMARK"))
    item->SetMarked(true);

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    item->SetEnabled(false);

  haikuItemRefreshIcon(item, ih);

  haikuItemApplyKey(ih, item, iupAttribGet(ih, "TITLE"));

  parent->AddItem(item);
  ih->handle = reinterpret_cast<InativeHandle*>(item);
  return IUP_NOERROR;
}

/* No LooperLockGuard on item->Menu()->Window(): cross-window inversion with menu_tracking. */

static int haikuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* item = reinterpret_cast<BMenuItem*>(ih->handle);
  if (!item) return 1;
  char trigger;
  char* label = haikuItemSplitTitle(ih, value, &trigger);
  item->SetLabel(label);
  free(label);
  item->SetTrigger(trigger);
  haikuItemApplyKey(ih, item, value);
  return 1;
}

static int haikuItemSetValueAttrib(Ihandle* ih, const char* value)
{
  auto* item = dynamic_cast<IupHaikuMenuItem*>(reinterpret_cast<BMenuItem*>(ih->handle));
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
  auto* item = dynamic_cast<IupHaikuMenuItem*>(reinterpret_cast<BMenuItem*>(ih->handle));
  if (!item) return 1;
  iupAttribSetStr(ih, "IMAGE", value);
  haikuItemRefreshIcon(item, ih);
  return 1;
}

static int haikuItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  auto* item = dynamic_cast<IupHaikuMenuItem*>(reinterpret_cast<BMenuItem*>(ih->handle));
  if (!item) return 1;
  iupAttribSetStr(ih, "IMPRESS", value);
  haikuItemRefreshIcon(item, ih);
  return 1;
}

static int haikuItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  auto* item = reinterpret_cast<BMenuItem*>(ih->handle);
  if (item) item->SetEnabled(iupStrBoolean(value));
  return iupBaseSetActiveAttrib(ih, value);
}

static int haikuSeparatorMapMethod(Ihandle* ih)
{
  BMenu* parent = haikuMenuParentBMenu(ih);
  if (!parent) return IUP_ERROR;

  auto* sep = new BSeparatorItem();
  parent->AddItem(sep);
  ih->handle = reinterpret_cast<InativeHandle*>(sep);
  return IUP_NOERROR;
}

static int haikuSubmenuMapMethod(Ihandle* ih)
{
  BMenu* parent = haikuMenuParentBMenu(ih);
  if (!parent) return IUP_ERROR;

  char trigger;
  char* label = haikuMenuLabel(ih, iupAttribGet(ih, "TITLE"), &trigger);

  auto* submenu = new IupHaikuMenu(label);
  free(label);

  auto* super = new IupHaikuMenuItem(ih, submenu);
  if (trigger) super->SetTrigger(trigger);
  char* image = iupAttribGet(ih, "IMAGE");
  if (image) super->SetIcon(haikuItemBitmapByName(ih, image));

  parent->AddItem(super);
  ih->handle = reinterpret_cast<InativeHandle*>(submenu);
  iupAttribSet(ih, "_IUPHAIKU_SUBMENU_SUPER", reinterpret_cast<char*>(super));

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    submenu->SetEnabled(false);

  return IUP_NOERROR;
}

static int haikuSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* m = reinterpret_cast<BMenu*>(ih->handle);
  if (!m) return 1;
  char trigger;
  char* label = haikuMenuLabel(ih, value, &trigger);
  /* BMenu::SetName isn't exposed; reach through the superitem instead. */
  BMenuItem* super = m->Superitem();
  if (super)
  {
    super->SetLabel(label);
    super->SetTrigger(trigger);
  }
  free(label);
  return 1;
}

static int haikuSubmenuSetActiveAttrib(Ihandle* ih, const char* value)
{
  auto* m = reinterpret_cast<BMenu*>(ih->handle);
  if (m) m->SetEnabled(iupStrBoolean(value));
  return iupBaseSetActiveAttrib(ih, value);
}

static int haikuSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  auto* super = reinterpret_cast<IupHaikuMenuItem*>(iupAttribGet(ih, "_IUPHAIKU_SUBMENU_SUPER"));
  if (!super) return 1;
  super->SetIcon(haikuItemBitmapByName(ih, value));
  if (super->Menu()) super->Menu()->InvalidateLayout();
  return 1;
}

static int haikuMenuMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    auto* win = reinterpret_cast<BWindow*>(ih->parent->handle);
    if (!win) return IUP_ERROR;

    int menu_h = iupdrvMenuGetMenuBarSize(ih);
    BMenuBar* menubar;
    {
      LooperLockGuard guard(win);
      BRect b = win->Bounds();
      menubar = new IupHaikuMenuBar(BRect(0, 0, b.Width(), static_cast<float>(menu_h - 1)));
      win->AddChild(menubar);

      /* B_FOLLOW_ALL_SIDES on the root view preserves menu_h top offset on resize. */
      BView* root = iuphaikuDialogRootView(win);
      if (root)
      {
        root->MoveTo(0, static_cast<float>(menu_h));
        root->ResizeTo(b.Width(), b.Height() - menu_h);
      }
    }
    ih->handle = reinterpret_cast<InativeHandle*>(menubar);
    iupAttribSet(ih->parent, "_IUP_DIALOG_HASMENU", "1");
    return IUP_NOERROR;
  }

  /* Menu under Submenu aliases the parent submenu's BMenu so children attach to the same node. */
  if (ih->parent && ih->parent->handle &&
      ih->parent->iclass && iupStrEqual(ih->parent->iclass->name, "submenu"))
  {
    ih->handle = ih->parent->handle;
    if (auto* m = dynamic_cast<IupHaikuMenu*>(reinterpret_cast<BMenu*>(ih->handle)))
      m->SetIhandle(ih);
    if (iupAttribGetBoolean(ih, "RADIO"))
      (reinterpret_cast<BMenu*>(ih->handle))->SetRadioMode(true);
    return IUP_NOERROR;
  }

  auto* popup = new BPopUpMenu("iup_popup", false, false);
  if (iupAttribGetBoolean(ih, "RADIO"))
    popup->SetRadioMode(true);
  ih->handle = reinterpret_cast<InativeHandle*>(popup);
  return IUP_NOERROR;
}

static int haikuMenuSetRadioAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  if (iupMenuIsMenuBar(ih)) return 0;
  auto* m = reinterpret_cast<BMenu*>(ih->handle);
  m->SetRadioMode(iupStrBoolean(value) ? true : false);
  return 1;
}

static void haikuMenuUnMapMethod(Ihandle* ih)
{
  /* The aliased submenu case must NOT delete - the real owner is the parent. */
  if (ih->parent && ih->parent->handle == ih->handle)
  {
    ih->handle = nullptr;
    return;
  }

  if (iupMenuIsMenuBar(ih))
  {
    auto* mb = reinterpret_cast<BMenuBar*>(ih->handle);
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
      delete mb;
    }
    if (ih->parent) iupAttribSet(ih->parent, "_IUP_DIALOG_HASMENU", nullptr);
    ih->parent = nullptr;
  }
  else
  {
    delete reinterpret_cast<BPopUpMenu*>(ih->handle);
  }
  ih->handle = nullptr;
}

static void haikuItemUnMapMethod(Ihandle* ih)
{
  /* Item is owned by parent BMenu - just clear our pointer. */
  ih->handle = nullptr;
}

extern "C" IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  auto* popup = reinterpret_cast<BPopUpMenu*>(ih->handle);
  if (!popup) return IUP_ERROR;

  /* autoInvoke=false: ACTION dispatches synchronously to avoid racing IupDestroy after IupPopup. */
  BMenuItem* sel = popup->Go(BPoint(static_cast<float>(x), static_cast<float>(y)), false, false, false);
  if (!sel) return IUP_NOERROR;

  BMessage* m = sel->Message();
  if (!m || m->what != IUPHAIKU_MENU_ITEM_MSG) return IUP_NOERROR;

  Ihandle* item_ih = nullptr;
  m->FindPointer(kIupMenuItemField, reinterpret_cast<void**>(&item_ih));
  if (!item_ih || !iupObjectCheck(item_ih)) return IUP_NOERROR;

  if (iupAttribGetBoolean(item_ih, "AUTOTOGGLE"))
  {
    int v = iupStrBoolean(iupAttribGet(item_ih, "VALUE"));
    IupSetAttribute(item_ih, "VALUE", v ? "OFF" : "ON");
  }
  auto cb = static_cast<Icallback>(IupGetCallback(item_ih, "ACTION"));
  if (cb && cb(item_ih) == IUP_CLOSE) IupExitLoop();
  return IUP_NOERROR;
}

extern "C" IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, nullptr, &ch);
  return 4 + ch + 4;
}

extern "C" IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = haikuMenuMapMethod;
  ic->UnMap = haikuMenuUnMapMethod;

  /* Inherited by IupSubmenu / IupItem; queried by iupdrvMenuGetMenuBarSize. */
  iupClassRegisterAttribute(ic, "FONT", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "RADIO", nullptr, haikuMenuSetRadioAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
}

extern "C" IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
  ic->Map = haikuItemMapMethod;
  ic->UnMap = haikuItemUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", nullptr, haikuItemSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", nullptr, haikuItemSetValueAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "IMAGE", nullptr, haikuItemSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, haikuItemSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDEMARK", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMPRESS", nullptr, haikuItemSetImpressAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
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

  iupClassRegisterAttribute(ic, "TITLE", nullptr, haikuSubmenuSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuSubmenuSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "IMAGE", nullptr, haikuSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, haikuSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}

extern "C" IUP_SDK_API int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", reinterpret_cast<char*>(recent_cb));
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

extern "C" IUP_SDK_API int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  if (!menu || !menu->handle) return -1;
  auto* bm = reinterpret_cast<BMenu*>(menu->handle);

  int max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  if (count > max_recent) count = max_recent;
  iupAttribSet(menu, "_IUP_RECENT_CB", reinterpret_cast<char*>(recent_cb));

  int prev = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");
  BWindow* win = haikuMenuOwningWindow(menu);
  BMessenger target = win ? BMessenger(win) : (be_app ? BMessenger(be_app) : BMessenger());

  LooperLockGuard guard(bm->Window());

  for (int i = 0; i < prev; i++)
  {
    char an[32];
    snprintf(an, sizeof(an), "_IUP_RECENT_ITEM%d", i);
    auto* item = reinterpret_cast<BMenuItem*>(iupAttribGet(menu, an));
    if (item) { bm->RemoveItem(item); delete item; }
    iupAttribSet(menu, an, nullptr);
    snprintf(an, sizeof(an), "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, an, nullptr);
  }

  for (int i = 0; i < count; i++)
  {
    auto* msg = new BMessage(IUPHAIKU_MENU_RECENT_MSG);
    msg->AddPointer("menu", menu);
    msg->AddInt32("index", i);
    auto* item = new BMenuItem(filenames[i], msg);
    if (target.IsValid()) item->SetTarget(target);
    bm->AddItem(item);

    char an[32];
    snprintf(an, sizeof(an), "_IUP_RECENT_ITEM%d", i);
    iupAttribSet(menu, an, reinterpret_cast<char*>(item));
    snprintf(an, sizeof(an), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, an, filenames[i]);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}

IUP_DRV_API void iuphaikuRecentDispatch(Ihandle* menu, int index)
{
  if (!menu || !iupObjectCheck(menu) || index < 0) return;
  auto cb = reinterpret_cast<Icallback>(iupAttribGet(menu, "_IUP_RECENT_CB"));
  auto* config = reinterpret_cast<Ihandle*>(iupAttribGet(menu, "_IUP_CONFIG"));
  if (!cb || !config) return;
  char an[32];
  snprintf(an, sizeof(an), "_IUP_RECENT_FILE%d", index);
  char* filename = iupAttribGet(menu, an);
  if (!filename) return;
  IupSetStrAttribute(config, "RECENTFILENAME", filename);
  IupSetStrAttribute(config, "TITLE", filename);
  config->parent = menu;
  cb(config);
  config->parent = nullptr;
  IupSetAttribute(config, "RECENTFILENAME", nullptr);
  IupSetAttribute(config, "TITLE", nullptr);
}

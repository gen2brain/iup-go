/** \file
 * \brief Haiku Button Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <InterfaceDefs.h>
#include <Looper.h>
#include <Message.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_drv.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


#define IUPHAIKU_BUTTON_INVOKE_MSG 'iupB'


static char* haikuStrippedMnemonic(const char* title)
{
  if (!title) return nullptr;
  if (!strchr(title, '&')) return iupStrDup(title);
  return iupStrProcessMnemonic(title, nullptr, 0);
}


class IupHaikuImageButton : public BView
{
public:
  explicit IupHaikuImageButton(Ihandle* ih)
    : BView(BRect(0, 0, 0, 0), "iup_image_button", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fIhandle(ih), fPressed(false), fEnabled(true)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void Draw(BRect /*update*/) override
  {
    BBitmap* bm = currentBitmap();
    BRect bounds = Bounds();

    if (!bm && fIhandle && !iupAttribGet(fIhandle, "TITLE") && iupAttribGet(fIhandle, "BGCOLOR"))
    {
      SetHighColor(tint_color(ViewColor(), fPressed ? B_DARKEN_2_TINT : B_DARKEN_1_TINT));
      StrokeRect(bounds);
      return;
    }

    int img_w = 0, img_h = 0;
    if (bm) { img_w = static_cast<int>(bm->Bounds().Width() + 1); img_h = static_cast<int>(bm->Bounds().Height() + 1); }

    char* title = fIhandle ? iupAttribGet(fIhandle, "TITLE") : nullptr;
    char* stripped = haikuStrippedMnemonic(title);
    const char* text = stripped ? stripped : "";
    int has_text = text && *text;

    int spacing = bm && has_text ? 2 : 0;
    int text_w = has_text ? static_cast<int>(ceilf(StringWidth(text))) : 0;
    font_height fh; GetFontHeight(&fh);
    int text_h = has_text ? static_cast<int>(ceilf(fh.ascent + fh.descent)) : 0;

    int total_w = img_w + spacing + text_w;
    int total_h = (img_h > text_h) ? img_h : text_h;
    int origin_x = static_cast<int>((bounds.Width() + 1 - total_w) / 2);
    int origin_y = static_cast<int>((bounds.Height() + 1 - total_h) / 2);
    if (origin_x < 0) origin_x = 0;
    if (origin_y < 0) origin_y = 0;

    if (bm)
    {
      SetDrawingMode(B_OP_ALPHA);
      DrawBitmap(bm, BPoint(origin_x, origin_y + (total_h - img_h) / 2));
    }
    if (has_text)
    {
      SetDrawingMode(B_OP_OVER);
      SetHighColor(fEnabled ? iuphaikuColor(B_PANEL_TEXT_COLOR)
                            : tint_color(iuphaikuColor(B_PANEL_TEXT_COLOR), B_DISABLED_LABEL_TINT));
      float text_x = origin_x + img_w + spacing;
      float text_y = origin_y + (total_h - text_h) / 2 + fh.ascent;
      DrawString(text, BPoint(text_x, text_y));
    }
    if (stripped) free(stripped);
  }

  void MouseDown(BPoint where) override
  {
    if (!fIhandle || !fEnabled) return;
    if (iupAttribGetBoolean(fIhandle, "CANFOCUS")) MakeFocus(true);

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : nullptr;
    int32 buttons = 0, mods = 0, clicks = 1;
    if (msg)
    {
      msg->FindInt32("buttons", &buttons);
      msg->FindInt32("modifiers", &mods);
      msg->FindInt32("clicks", &clicks);
    }

    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
    fPressed = true;
    Invalidate();

    int btn = 0;
    if      (buttons & B_PRIMARY_MOUSE_BUTTON)   btn = IUP_BUTTON1;
    else if (buttons & B_SECONDARY_MOUSE_BUTTON) btn = IUP_BUTTON3;
    else if (buttons & B_TERTIARY_MOUSE_BUTTON)  btn = IUP_BUTTON2;

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus(static_cast<unsigned>(mods), static_cast<unsigned>(buttons), 0, status, clicks == 2 ? 1 : 0);

    auto cb = reinterpret_cast<IFniiiis>(IupGetCallback(fIhandle, "BUTTON_CB"));
    if (cb) cb(fIhandle, btn, 1, static_cast<int>(where.x), static_cast<int>(where.y), status);
  }

  void MouseUp(BPoint where) override
  {
    if (!fIhandle) return;

    bool wasPressed = fPressed;
    fPressed = false;
    Invalidate();

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : nullptr;
    int32 mods = 0;
    if (msg) msg->FindInt32("modifiers", &mods);

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus(static_cast<unsigned>(mods), 0, 0, status, 0);

    auto cb = reinterpret_cast<IFniiiis>(IupGetCallback(fIhandle, "BUTTON_CB"));
    if (cb) cb(fIhandle, IUP_BUTTON1, 0, static_cast<int>(where.x), static_cast<int>(where.y), status);

    if (wasPressed && fEnabled && Bounds().Contains(where))
    {
      auto acb = static_cast<Icallback>(IupGetCallback(fIhandle, "ACTION"));
      if (acb && acb(fIhandle) == IUP_CLOSE) IupExitLoop();
    }
  }

  void SetEnabled(bool enabled)
  {
    if (fEnabled == enabled) return;
    fEnabled = enabled;
    Invalidate();
  }

  /* borrowed pointers, IUP image cache owns the bitmaps */
  void SetBitmaps(BBitmap* normal, BBitmap* press, BBitmap* inactive)
  {
    fNormal = normal; fPress = press; fInactive = inactive;
    Invalidate();
  }

  void MakeFocus(bool focus = true) override
  {
    BView::MakeFocus(focus);
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focus ? 1 : 0);
  }

private:
  BBitmap* currentBitmap() const
  {
    if (!fEnabled && fInactive) return fInactive;
    if (fPressed && fPress) return fPress;
    return fNormal;
  }

  Ihandle* fIhandle;
  BBitmap* fNormal = nullptr;
  BBitmap* fPress = nullptr;
  BBitmap* fInactive = nullptr;
  bool fPressed;
  bool fEnabled;
};


class IupHaikuButton : public BButton
{
public:
  IupHaikuButton(Ihandle* ih, const char* label)
    : BButton(BRect(0, 0, 0, 0), "iup_button", label,
              new BMessage(IUPHAIKU_BUTTON_INVOKE_MSG), B_FOLLOW_NONE),
      fIhandle(ih), fInside(false)
  {
    SetExplicitMinSize(BSize(0, 0));
  }

  void AttachedToWindow() override
  {
    BButton::AttachedToWindow();
    SetTarget(this);
  }

  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  {
    BButton::MouseMoved(where, transit, drag);
    if (transit == B_ENTERED_VIEW) fInside = true;
    else if (transit == B_EXITED_VIEW) fInside = false;
  }

  void Draw(BRect updateRect) override
  {
    int pos = fIhandle ? fIhandle->data->img_position : IUP_IMGPOS_LEFT;
    const BBitmap* icon = IconBitmap((Value() == B_CONTROL_OFF ? B_INACTIVE_ICON_BITMAP : B_ACTIVE_ICON_BITMAP)
                                     | (IsEnabled() ? 0 : B_DISABLED_ICON_BITMAP));
    const char* label = Label();
    if (pos == IUP_IMGPOS_LEFT || !icon || !label || !*label)
    {
      BButton::Draw(updateRect);
      return;
    }

    BRect rect(Bounds());
    rgb_color background = ViewColor();
    rgb_color text = ui_color(B_CONTROL_TEXT_COLOR);
    rgb_color base = ui_color(B_CONTROL_BACKGROUND_COLOR);

    uint32 flags = be_control_look->Flags(this);
    if (IsDefault()) flags |= BControlLook::B_DEFAULT_BUTTON;
    if (IsFlat() && !IsTracking()) flags |= BControlLook::B_FLAT;
    if (fInside) flags |= BControlLook::B_HOVER;

    be_control_look->DrawButtonFrame(this, rect, updateRect, base, background, flags);
    be_control_look->DrawButtonBackground(this, rect, updateRect, base, flags);

    float spacing = fIhandle->data->spacing;
    float icon_w = icon->Bounds().Width() + 1;
    float icon_h = icon->Bounds().Height() + 1;
    float text_w = ceilf(StringWidth(label));
    font_height fh; GetFontHeight(&fh);
    float text_h = ceilf(fh.ascent + fh.descent);

    BPoint icon_at;
    BRect text_rect;
    if (pos == IUP_IMGPOS_RIGHT)
    {
      float x = rect.left + floorf((rect.Width() + 1 - (icon_w + spacing + text_w)) / 2);
      text_rect.Set(x, rect.top, x + text_w - 1, rect.bottom);
      icon_at.Set(x + text_w + spacing, rect.top + floorf((rect.Height() + 1 - icon_h) / 2));
    }
    else
    {
      float y = rect.top + floorf((rect.Height() + 1 - (icon_h + spacing + text_h)) / 2);
      float icon_y = pos == IUP_IMGPOS_TOP ? y : y + text_h + spacing;
      float text_y = pos == IUP_IMGPOS_TOP ? y + icon_h + spacing : y;
      icon_at.Set(rect.left + floorf((rect.Width() + 1 - icon_w) / 2), icon_y);
      text_rect.Set(rect.left, text_y, rect.right, text_y + text_h - 1);
    }

    SetDrawingMode(B_OP_ALPHA);
    DrawBitmap(icon, icon_at);
    SetDrawingMode(B_OP_COPY);
    be_control_look->DrawLabel(this, label, text_rect, updateRect, base, flags,
                               BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE), &text);
  }

  void MakeFocus(bool focus = true) override
  {
    BButton::MakeFocus(focus);
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focus ? 1 : 0);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg->what == IUPHAIKU_BUTTON_INVOKE_MSG && fIhandle)
    {
      if (Window()) Window()->UpdateIfNeeded();
      auto cb = static_cast<Icallback>(IupGetCallback(fIhandle, "ACTION"));
      if (cb && cb(fIhandle) == IUP_CLOSE) IupExitLoop();
      return;
    }
    BButton::MessageReceived(msg);
  }

  void MouseDown(BPoint where) override
  {
    if (fIhandle && IsEnabled() && iupAttribGetBoolean(fIhandle, "CANFOCUS"))
      MakeFocus(true);
    BButton::MouseDown(where);
    if (!fIhandle) return;

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : nullptr;
    int32 buttons = 0, mods = 0, clicks = 1;
    if (msg)
    {
      msg->FindInt32("buttons", &buttons);
      msg->FindInt32("modifiers", &mods);
      msg->FindInt32("clicks", &clicks);
    }

    int btn = 0;
    if      (buttons & B_PRIMARY_MOUSE_BUTTON)   btn = IUP_BUTTON1;
    else if (buttons & B_SECONDARY_MOUSE_BUTTON) btn = IUP_BUTTON3;
    else if (buttons & B_TERTIARY_MOUSE_BUTTON)  btn = IUP_BUTTON2;

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus(static_cast<unsigned>(mods), static_cast<unsigned>(buttons), 0, status, clicks == 2 ? 1 : 0);

    auto cb = reinterpret_cast<IFniiiis>(IupGetCallback(fIhandle, "BUTTON_CB"));
    if (cb) cb(fIhandle, btn, 1, static_cast<int>(where.x), static_cast<int>(where.y), status);
  }

  void MouseUp(BPoint where) override
  {
    BButton::MouseUp(where);
    if (!fIhandle) return;

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : nullptr;
    int32 mods = 0;
    if (msg) msg->FindInt32("modifiers", &mods);

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus(static_cast<unsigned>(mods), 0, 0, status, 0);

    auto cb = reinterpret_cast<IFniiiis>(IupGetCallback(fIhandle, "BUTTON_CB"));
    if (cb) cb(fIhandle, IUP_BUTTON1, 0, static_cast<int>(where.x), static_cast<int>(where.y), status);
  }

private:
  Ihandle* fIhandle;
  bool fInside;
};


static int haikuButtonIsColorSwatch(Ihandle* ih)
{
  if (ih->data->type & IUP_BUTTON_IMAGE) return 0;
  char* title = iupAttribGet(ih, "TITLE");
  if (title && *title) return 0;
  return iupAttribGet(ih, "BGCOLOR") != nullptr;
}

static int haikuButtonIsChromeless(Ihandle* ih)
{
  if (haikuButtonIsColorSwatch(ih)) return 1;
  if (!(ih->data->type & IUP_BUTTON_IMAGE)) return 0;
  if (!iupAttribGet(ih, "IMPRESS")) return 0;
  if (iupAttribGetBoolean(ih, "IMPRESSBORDER")) return 0;
  return 1;
}

static void haikuImageButtonRefresh(Ihandle* ih)
{
  auto* btn = dynamic_cast<IupHaikuImageButton*>(reinterpret_cast<BView*>(ih->handle));
  if (!btn) return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  char* image = iupAttribGet(ih, "IMAGE");
  char* press = iupAttribGet(ih, "IMPRESS");
  char* inactive = iupAttribGet(ih, "IMINACTIVE");

  BBitmap* bmImage    = image    ? static_cast<BBitmap*>(iupImageGetImage(image,    ih, 0, bgcolor)) : nullptr;
  BBitmap* bmPress    = press    ? static_cast<BBitmap*>(iupImageGetImage(press,    ih, 0, bgcolor)) : nullptr;
  BBitmap* bmInactive = inactive ? static_cast<BBitmap*>(iupImageGetImage(inactive, ih, 0, bgcolor))
                                 : (image ? static_cast<BBitmap*>(iupImageGetImage(image, ih, 1, bgcolor)) : nullptr);

  LooperLockGuard guard(btn->Looper());
  btn->SetBitmaps(bmImage, bmPress, bmInactive);
}

static void haikuButtonApplyImage(Ihandle* ih, const char* name, int make_inactive)
{
  if (haikuButtonIsChromeless(ih)) { haikuImageButtonRefresh(ih); return; }

  auto* button = dynamic_cast<BButton*>(reinterpret_cast<BView*>(ih->handle));
  if (!button || !name) return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  auto* bm = static_cast<BBitmap*>(iupImageGetImage(name, ih, make_inactive, bgcolor));
  if (!bm) return;

  LooperLockGuard guard(button->Looper());
  button->SetIcon(nullptr, 0);
  button->SetIcon(bm, 0);
}


static int haikuButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* button = dynamic_cast<BButton*>(reinterpret_cast<BView*>(ih->handle));
  if (!button) return 1;

  int mn = iupStrFindMnemonic(value);
  if (mn) iupKeySetMnemonic(ih, mn, 0);

  char* stripped = haikuStrippedMnemonic(value);
  LooperLockGuard guard(button->Looper());
  button->SetLabel(stripped ? stripped : "");
  if (stripped) free(stripped);
  return 1;
}

static int haikuButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  auto* view = reinterpret_cast<BView*>(ih->handle);
  if (view)
  {
    bool enable = iupStrBoolean(value) ? true : false;
    LooperLockGuard guard(view->Looper());
    if (auto* b = dynamic_cast<BButton*>(view)) b->SetEnabled(enable);
    else if (auto* ib = dynamic_cast<IupHaikuImageButton*>(view)) ib->SetEnabled(enable);
  }

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
    {
      if (iupStrBoolean(value))
        haikuButtonApplyImage(ih, image, 0);
      else
      {
        char* iminactive = iupAttribGet(ih, "IMINACTIVE");
        if (iminactive)
          haikuButtonApplyImage(ih, iminactive, 0);
        else
          haikuButtonApplyImage(ih, image, 1);
      }
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int haikuButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->type & IUP_BUTTON_IMAGE) || !value)
    return 0;

  if (iupdrvIsActive(ih))
    haikuButtonApplyImage(ih, value, 0);
  else
  {
    char* iminactive = iupAttribGet(ih, "IMINACTIVE");
    if (iminactive)
      haikuButtonApplyImage(ih, iminactive, 0);
    else
      haikuButtonApplyImage(ih, value, 1);
  }
  return 1;
}

static int haikuButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->type & IUP_BUTTON_IMAGE))
    return 0;

  if (!iupdrvIsActive(ih))
  {
    if (value)
      haikuButtonApplyImage(ih, value, 0);
    else
    {
      char* image = iupAttribGet(ih, "IMAGE");
      if (image) haikuButtonApplyImage(ih, image, 1);
    }
  }
  return 1;
}

static int haikuButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (haikuButtonIsChromeless(ih))
    haikuImageButtonRefresh(ih);
  return 1;
}

static int haikuButtonSetShowAsDefaultAttrib(Ihandle* ih, const char* value)
{
  auto* button = dynamic_cast<BButton*>(reinterpret_cast<BView*>(ih->handle));
  if (!button) return 1;

  LooperLockGuard guard(button->Looper());
  button->MakeDefault(iupStrBoolean(value) ? true : false);
  return 1;
}

static int haikuButtonSetCanFocusAttrib(Ihandle* ih, const char* value)
{
  auto* view = reinterpret_cast<BView*>(ih->handle);
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());
  iuphaikuSetCanFocus(view, iupStrBoolean(value));
  return 1;
}


static int haikuButtonMapMethod(Ihandle* ih)
{
  if (iupAttribGet(ih, "IMAGE"))
  {
    ih->data->type = IUP_BUTTON_IMAGE;
    char* title = iupAttribGet(ih, "TITLE");
    if (title && *title != 0)
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
  {
    ih->data->type = IUP_BUTTON_TEXT;
  }

  if (haikuButtonIsChromeless(ih))
  {
    auto* btn = new IupHaikuImageButton(ih);
    ih->handle = reinterpret_cast<InativeHandle*>(btn);
    iuphaikuAddToParent(ih);
    haikuImageButtonRefresh(ih);
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    char* stripped = haikuStrippedMnemonic(title);

    auto* button = new IupHaikuButton(ih, stripped ? stripped : "");
    if (stripped) free(stripped);

    ih->handle = reinterpret_cast<InativeHandle*>(button);

    iuphaikuAddToParent(ih);
    iuphaikuUpdateWidgetFont(ih, button);

    if (ih->data->type & IUP_BUTTON_IMAGE)
    {
      char* image = iupAttribGet(ih, "IMAGE");
      if (image) haikuButtonApplyImage(ih, image, 0);
    }

    if (iupAttribGetBoolean(ih, "FLAT"))
      button->SetFlat(true);

    if (iupAttribGetBoolean(ih, "SHOWASDEFAULT"))
      haikuButtonSetShowAsDefaultAttrib(ih, "YES");
  }

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    haikuButtonSetCanFocusAttrib(ih, "NO");

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int* x, int* y)
{
  if (ih && haikuButtonIsColorSwatch(ih))
  {
    /* replace (not add): core seeded h with charheight for empty title */
    if (x) *x = 24;
    if (y) *y = 24;
    return;
  }

  if (!be_control_look) { if (x) *x += 24; if (y) *y += 12; return; }

  int is_default = ih && iupAttribGetBoolean(ih, "SHOWASDEFAULT");
  uint32 flags = is_default ? BControlLook::B_DEFAULT_BUTTON : 0;
  int has_user_padding = ih && (ih->data->horiz_padding > 0 || ih->data->vert_padding > 0);

  float left, top, right, bottom;
  if (has_user_padding)
    be_control_look->GetFrameInsets(BControlLook::B_BUTTON_FRAME, flags, left, top, right, bottom);
  else
    be_control_look->GetInsets(BControlLook::B_BUTTON_FRAME, BControlLook::B_BUTTON_BACKGROUND, flags, left, top, right, bottom);

  if (has_user_padding)
  {
    if (x) *x += static_cast<int>(left + right + 0.5f);
    if (y) *y += static_cast<int>(top + bottom + 0.5f);
    return;
  }

  float spacing = be_control_look->DefaultLabelSpacing();
  float insets_w = left + right + spacing - 1;
  int has_label = ih && (ih->data->type & IUP_BUTTON_TEXT);
  float chrome_w = has_label ? fmaxf(insets_w, ceilf(spacing * 3.3f)) : insets_w;
  if (ih && (ih->data->type & IUP_BUTTON_IMAGE) && has_label &&
      (ih->data->img_position == IUP_IMGPOS_LEFT || ih->data->img_position == IUP_IMGPOS_RIGHT))
    chrome_w += spacing;
  if (x)
  {
    int total = *x + static_cast<int>(chrome_w + 0.5f);
    int minw = has_label ? static_cast<int>(spacing * 12.5f + 0.5f) : static_cast<int>(spacing + 0.5f);
    *x = total > minw ? total : minw;
  }
  if (y) *y += static_cast<int>(top + bottom + spacing + 0.5f);
}

extern "C" IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = haikuButtonMapMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", nullptr, haikuButtonSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", nullptr, haikuButtonSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", nullptr, haikuButtonSetImInactiveAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", nullptr, haikuButtonSetImPressAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", nullptr, nullptr, "ACENTER:ACENTER", nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, nullptr, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED);

  iupClassRegisterAttribute(ic, "SHOWASDEFAULT", nullptr, haikuButtonSetShowAsDefaultAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANFOCUS", nullptr, haikuButtonSetCanFocusAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}

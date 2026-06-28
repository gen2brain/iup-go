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
  if (!title) return NULL;
  if (!strchr(title, '&')) return iupStrDup(title);
  return iupStrProcessMnemonic(title, NULL, 0);
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
    if (bm) { img_w = (int)(bm->Bounds().Width() + 1); img_h = (int)(bm->Bounds().Height() + 1); }

    char* title = fIhandle ? iupAttribGet(fIhandle, "TITLE") : NULL;
    char* stripped = haikuStrippedMnemonic(title);
    const char* text = stripped ? stripped : "";
    int has_text = text && *text;

    int spacing = bm && has_text ? 2 : 0;
    int text_w = has_text ? (int)ceilf(StringWidth(text)) : 0;
    font_height fh; GetFontHeight(&fh);
    int text_h = has_text ? (int)ceilf(fh.ascent + fh.descent) : 0;

    int total_w = img_w + spacing + text_w;
    int total_h = (img_h > text_h) ? img_h : text_h;
    int origin_x = (int)((bounds.Width() + 1 - total_w) / 2);
    int origin_y = (int)((bounds.Height() + 1 - total_h) / 2);
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
      SetHighColor(fEnabled ? ui_color(B_PANEL_TEXT_COLOR)
                            : tint_color(ui_color(B_PANEL_TEXT_COLOR), B_DISABLED_LABEL_TINT));
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

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : NULL;
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
    iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, clicks == 2 ? 1 : 0);

    IFniiiis cb = (IFniiiis)IupGetCallback(fIhandle, "BUTTON_CB");
    if (cb) cb(fIhandle, btn, 1, (int)where.x, (int)where.y, status);
  }

  void MouseUp(BPoint where) override
  {
    if (!fIhandle) return;

    bool wasPressed = fPressed;
    fPressed = false;
    Invalidate();

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : NULL;
    int32 mods = 0;
    if (msg) msg->FindInt32("modifiers", &mods);

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus((unsigned)mods, 0, 0, status, 0);

    IFniiiis cb = (IFniiiis)IupGetCallback(fIhandle, "BUTTON_CB");
    if (cb) cb(fIhandle, IUP_BUTTON1, 0, (int)where.x, (int)where.y, status);

    if (wasPressed && fEnabled && Bounds().Contains(where))
    {
      Icallback acb = (Icallback)IupGetCallback(fIhandle, "ACTION");
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
  BBitmap* fNormal = NULL;
  BBitmap* fPress = NULL;
  BBitmap* fInactive = NULL;
  bool fPressed;
  bool fEnabled;
};


class IupHaikuButton : public BButton
{
public:
  IupHaikuButton(Ihandle* ih, const char* label)
    : BButton(BRect(0, 0, 0, 0), "iup_button", label,
              new BMessage(IUPHAIKU_BUTTON_INVOKE_MSG), B_FOLLOW_NONE),
      fIhandle(ih)
  {
    SetExplicitMinSize(BSize(0, 0));
  }

  void AttachedToWindow() override
  {
    BButton::AttachedToWindow();
    SetTarget(this);
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
      Icallback cb = (Icallback)IupGetCallback(fIhandle, "ACTION");
      if (cb && cb(fIhandle) == IUP_CLOSE) IupExitLoop();
      return;
    }
    BButton::MessageReceived(msg);
  }

  void MouseDown(BPoint where) override
  {
    BButton::MouseDown(where);
    if (!fIhandle) return;

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : NULL;
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
    iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, clicks == 2 ? 1 : 0);

    IFniiiis cb = (IFniiiis)IupGetCallback(fIhandle, "BUTTON_CB");
    if (cb) cb(fIhandle, btn, 1, (int)where.x, (int)where.y, status);
  }

  void MouseUp(BPoint where) override
  {
    BButton::MouseUp(where);
    if (!fIhandle) return;

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : NULL;
    int32 mods = 0;
    if (msg) msg->FindInt32("modifiers", &mods);

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus((unsigned)mods, 0, 0, status, 0);

    IFniiiis cb = (IFniiiis)IupGetCallback(fIhandle, "BUTTON_CB");
    if (cb) cb(fIhandle, IUP_BUTTON1, 0, (int)where.x, (int)where.y, status);
  }

private:
  Ihandle* fIhandle;
};


static int haikuButtonIsColorSwatch(Ihandle* ih)
{
  if (ih->data->type & IUP_BUTTON_IMAGE) return 0;
  char* title = iupAttribGet(ih, "TITLE");
  if (title && *title) return 0;
  return iupAttribGet(ih, "BGCOLOR") != NULL;
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
  IupHaikuImageButton* btn = dynamic_cast<IupHaikuImageButton*>((BView*)ih->handle);
  if (!btn) return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  char* image = iupAttribGet(ih, "IMAGE");
  char* press = iupAttribGet(ih, "IMPRESS");
  char* inactive = iupAttribGet(ih, "IMINACTIVE");

  BBitmap* bmImage    = image    ? (BBitmap*)iupImageGetImage(image,    ih, 0, bgcolor) : NULL;
  BBitmap* bmPress    = press    ? (BBitmap*)iupImageGetImage(press,    ih, 0, bgcolor) : NULL;
  BBitmap* bmInactive = inactive ? (BBitmap*)iupImageGetImage(inactive, ih, 0, bgcolor)
                                 : (image ? (BBitmap*)iupImageGetImage(image, ih, 1, bgcolor) : NULL);

  LooperLockGuard guard(btn->Looper());
  btn->SetBitmaps(bmImage, bmPress, bmInactive);
}

static void haikuButtonApplyImage(Ihandle* ih, const char* name, int make_inactive)
{
  if (haikuButtonIsChromeless(ih)) { haikuImageButtonRefresh(ih); return; }

  BButton* button = dynamic_cast<BButton*>((BView*)ih->handle);
  if (!button || !name) return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  BBitmap* bm = (BBitmap*)iupImageGetImage(name, ih, make_inactive, bgcolor);
  if (!bm) return;

  LooperLockGuard guard(button->Looper());
  button->SetIcon(bm, 0);
}


static int haikuButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  BButton* button = dynamic_cast<BButton*>((BView*)ih->handle);
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
  BView* view = (BView*)ih->handle;
  if (view)
  {
    bool enable = iupStrBoolean(value) ? true : false;
    LooperLockGuard guard(view->Looper());
    if (BButton* b = dynamic_cast<BButton*>(view)) b->SetEnabled(enable);
    else if (IupHaikuImageButton* ib = dynamic_cast<IupHaikuImageButton*>(view)) ib->SetEnabled(enable);
  }

  /* swap to IMINACTIVE / auto-greyed IMAGE when going inactive */
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
  BButton* button = dynamic_cast<BButton*>((BView*)ih->handle);
  if (!button) return 1;

  LooperLockGuard guard(button->Looper());
  button->MakeDefault(iupStrBoolean(value) ? true : false);
  return 1;
}

static int haikuButtonSetCanFocusAttrib(Ihandle* ih, const char* value)
{
  BView* view = (BView*)ih->handle;
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
    IupHaikuImageButton* btn = new IupHaikuImageButton(ih);
    ih->handle = (InativeHandle*)btn;
    iuphaikuAddToParent(ih);
    haikuImageButtonRefresh(ih);
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    char* stripped = haikuStrippedMnemonic(title);

    IupHaikuButton* button = new IupHaikuButton(ih, stripped ? stripped : "");
    if (stripped) free(stripped);

    ih->handle = (InativeHandle*)button;

    iuphaikuAddToParent(ih);
    iuphaikuUpdateWidgetFont(ih, button);

    if (ih->data->type & IUP_BUTTON_IMAGE)
    {
      char* image = iupAttribGet(ih, "IMAGE");
      if (image) haikuButtonApplyImage(ih, image, 0);
    }

    if (iupAttribGetBoolean(ih, "SHOWASDEFAULT"))
      haikuButtonSetShowAsDefaultAttrib(ih, "YES");
  }

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    haikuButtonSetCanFocusAttrib(ih, "NO");

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
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
    if (x) *x += (int)(left + right + 0.5f);
    if (y) *y += (int)(top + bottom + 0.5f);
    return;
  }

  float spacing = be_control_look->DefaultLabelSpacing();
  float insets_w = left + right + spacing - 1;
  int has_label = ih && (ih->data->type & IUP_BUTTON_TEXT);
  float chrome_w = has_label ? fmaxf(insets_w, ceilf(spacing * 3.3f)) : insets_w;
  if (ih && (ih->data->type & IUP_BUTTON_IMAGE) && has_label)
    chrome_w += spacing;
  if (x)
  {
    int total = *x + (int)(chrome_w + 0.5f);
    int minw = has_label ? (int)(spacing * 12.5f + 0.5f) : (int)(spacing + 0.5f);
    *x = total > minw ? total : minw;
  }
  if (y) *y += (int)(top + bottom + spacing + 0.5f);
}

extern "C" IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = haikuButtonMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, haikuButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, haikuButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, haikuButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, haikuButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, NULL, "ACENTER:ACENTER", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);

  iupClassRegisterAttribute(ic, "SHOWASDEFAULT", NULL, haikuButtonSetShowAsDefaultAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANFOCUS", NULL, haikuButtonSetCanFocusAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}

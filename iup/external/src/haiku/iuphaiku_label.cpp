/** \file
 * \brief Haiku Label Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <Looper.h>
#include <String.h>
#include <StringList.h>
#include <StringView.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_label.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


static void haikuLabelFireMouseCb(Ihandle* ih, BView* view, BPoint where, int pressed)
{
  if (!ih) return;
  if (!iupdrvIsActive(ih)) return;
  BMessage* msg = view->Looper() ? view->Looper()->CurrentMessage() : NULL;
  int32 buttons = 0, mods = 0, clicks = 1;
  if (msg)
  {
    msg->FindInt32("buttons", &buttons);
    msg->FindInt32("modifiers", &mods);
    msg->FindInt32("clicks", &clicks);
  }
  int btn = IUP_BUTTON1;
  if      (buttons & B_SECONDARY_MOUSE_BUTTON) btn = IUP_BUTTON3;
  else if (buttons & B_TERTIARY_MOUSE_BUTTON)  btn = IUP_BUTTON2;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, clicks == 2 ? 1 : 0);
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb) cb(ih, btn, pressed, (int)where.x, (int)where.y, status);
}

static void haikuLabelFireTransitCb(Ihandle* ih, uint32 transit)
{
  if (!ih) return;
  if (!iupdrvIsActive(ih)) return;
  if (transit == B_ENTERED_VIEW)
  {
    IFn cb = IupGetCallback(ih, "ENTERWINDOW_CB");
    if (cb) cb(ih);
  }
  else if (transit == B_EXITED_VIEW)
  {
    IFn cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
    if (cb) cb(ih);
  }
}

static void haikuLabelFireMotionCb(Ihandle* ih, BView* view, BPoint where)
{
  if (!ih) return;
  if (!iupdrvIsActive(ih)) return;
  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (!cb) return;
  BMessage* msg = view->Looper() ? view->Looper()->CurrentMessage() : NULL;
  int32 buttons = 0, mods = 0;
  if (msg)
  {
    msg->FindInt32("buttons", &buttons);
    msg->FindInt32("modifiers", &mods);
  }
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, 0);
  cb(ih, (int)where.x, (int)where.y, status);
}

class IupHaikuLabelString : public BStringView
{
public:
  IupHaikuLabelString(Ihandle* ih, const char* text)
    : BStringView(BRect(0, 0, 0, 0), "iup_label", text ? text : "", B_FOLLOW_NONE),
      fIhandle(ih) {}
  void MouseDown(BPoint where) override
  { BStringView::MouseDown(where); haikuLabelFireMouseCb(fIhandle, this, where, 1); }
  void MouseUp(BPoint where) override
  { BStringView::MouseUp(where); haikuLabelFireMouseCb(fIhandle, this, where, 0); }
  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  { BStringView::MouseMoved(where, transit, drag);
    haikuLabelFireTransitCb(fIhandle, transit);
    haikuLabelFireMotionCb(fIhandle, this, where); }
  /* BStringView always bottom-anchors and ignores vertical alignment, so lay out the lines ourselves. */
  void Draw(BRect /*update*/) override
  {
    bool over_gl = false;
    if (fIhandle && iupAttribGet(fIhandle, "_IUPHAIKU_GLTRANSPARENT"))
    {
      Ihandle* np = iupChildTreeGetNativeParent(fIhandle);
      BBitmap* bmp = np ? (BBitmap*)iupAttribGet(np, "_IUPHAIKU_GLBITMAP") : NULL;
      if (bmp)
      {
        BRect frame = Frame();
        BRect bounds = Bounds();
        BRect src(frame.left, frame.top, frame.left + bounds.Width(), frame.top + bounds.Height());
        SetDrawingMode(B_OP_COPY);
        DrawBitmap(bmp, src, bounds);
        SetDrawingMode(B_OP_OVER);
        over_gl = true;
      }
    }

    const char* full = Text();
    if (!full || !*full)
      return;

    rgb_color hc = HighColor();
    bool inactive = (fIhandle && !iupdrvIsActive(fIhandle));
    if (inactive)
    {
      rgb_color bg = ui_color(B_PANEL_BACKGROUND_COLOR);
      rgb_color dim = { (uint8)((hc.red + bg.red) / 2), (uint8)((hc.green + bg.green) / 2), (uint8)((hc.blue + bg.blue) / 2), 255 };
      SetHighColor(dim);
    }
    if (!over_gl && LowUIColor() == B_NO_COLOR)
      SetLowColor(ViewColor());

    font_height fh;
    GetFontHeight(&fh);
    float lineHeight = ceilf(fh.ascent + fh.descent + fh.leading);
    BRect bounds = Bounds();

    BStringList lines;
    BString(full).Split("\n", false, lines);
    int n = lines.CountStrings();
    float blockHeight = (n - 1) * lineHeight + ceilf(fh.ascent + fh.descent);

    int valign = fIhandle ? fIhandle->data->vert_alignment : IUP_ALIGN_ACENTER;
    float blockTop;
    if (valign == IUP_ALIGN_ATOP)
      blockTop = bounds.top;
    else if (valign == IUP_ALIGN_ABOTTOM)
      blockTop = bounds.bottom - blockHeight;
    else
      blockTop = bounds.top + (bounds.Height() - blockHeight) / 2.0f;

    alignment halign = Alignment();
    uint32 trunc = Truncation();

    for (int i = 0; i < n; i++)
    {
      BString line = lines.StringAt(i);
      float width = StringWidth(line.String());
      if (trunc != B_NO_TRUNCATION && width > bounds.Width())
      {
        TruncateString(&line, trunc, bounds.Width());
        width = StringWidth(line.String());
      }

      float y = blockTop + ceilf(fh.ascent) + i * lineHeight;
      float x;
      if (halign == B_ALIGN_RIGHT)
        x = bounds.Width() - width;
      else if (halign == B_ALIGN_CENTER)
        x = (bounds.Width() - width) / 2.0f;
      else
        x = 0.0f;

      DrawString(line.String(), BPoint(x, y));
    }

    if (inactive)
      SetHighColor(hc);
  }
private:
  Ihandle* fIhandle;
};

/* Image label: custom BView that blits a BBitmap (weak ref into IUP image cache). */

class IupHaikuLabelImage : public BView
{
public:
  explicit IupHaikuLabelImage(Ihandle* ih)
    : BView(BRect(0, 0, 0, 0), "iup_label_img", B_FOLLOW_NONE, B_WILL_DRAW),
      fIhandle(ih), fBitmap(NULL)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void SetBitmap(BBitmap* bm) { fBitmap = bm; Invalidate(); }

  void Draw(BRect /*update*/) override
  {
    if (fBitmap)
    {
      SetDrawingMode(B_OP_ALPHA);
      DrawBitmap(fBitmap, BPoint(0, 0));
    }
  }

  void MouseDown(BPoint where) override
  { BView::MouseDown(where); haikuLabelFireMouseCb(fIhandle, this, where, 1); }
  void MouseUp(BPoint where) override
  { BView::MouseUp(where); haikuLabelFireMouseCb(fIhandle, this, where, 0); }
  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  { BView::MouseMoved(where, transit, drag);
    haikuLabelFireTransitCb(fIhandle, transit);
    haikuLabelFireMotionCb(fIhandle, this, where); }

private:
  Ihandle* fIhandle;
  BBitmap* fBitmap;
};

/* Separator: 2px etched line (dark + light tint). */

class IupHaikuLabelSeparator : public BView
{
public:
  explicit IupHaikuLabelSeparator(bool horizontal)
    : BView(BRect(0, 0, 0, 0), "iup_label_sep", B_FOLLOW_NONE, B_WILL_DRAW),
      fHorizontal(horizontal)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void Draw(BRect /*update*/) override
  {
    BRect b = Bounds();
    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    rgb_color dark = tint_color(base, B_DARKEN_2_TINT);
    rgb_color light = tint_color(base, B_LIGHTEN_2_TINT);

    if (fHorizontal)
    {
      float y = b.top + (b.Height() / 2.0f);
      SetHighColor(dark);
      StrokeLine(BPoint(b.left, y), BPoint(b.right, y));
      SetHighColor(light);
      StrokeLine(BPoint(b.left, y + 1), BPoint(b.right, y + 1));
    }
    else
    {
      float x = b.left + (b.Width() / 2.0f);
      SetHighColor(dark);
      StrokeLine(BPoint(x, b.top), BPoint(x, b.bottom));
      SetHighColor(light);
      StrokeLine(BPoint(x + 1, b.top), BPoint(x + 1, b.bottom));
    }
  }

private:
  bool fHorizontal;
};

/* Helpers */

static char* haikuStrippedMnemonic(const char* title)
{
  if (!title) return NULL;
  if (!strchr(title, '&')) return iupStrDup(title);
  return iupStrProcessMnemonic(title, NULL, 0);
}

static alignment haikuParseAlignment(const char* value)
{
  if (iupStrEqualNoCase(value, "ARIGHT"))  return B_ALIGN_RIGHT;
  if (iupStrEqualNoCase(value, "ACENTER")) return B_ALIGN_CENTER;
  return B_ALIGN_LEFT;
}

static void haikuLabelApplyImage(Ihandle* ih, const char* name, int make_inactive)
{
  IupHaikuLabelImage* view = (IupHaikuLabelImage*)ih->handle;
  if (!view || !name) return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  BBitmap* bm = (BBitmap*)iupImageGetImage(name, ih, make_inactive, bgcolor);

  LooperLockGuard guard(view->Looper());
  view->SetBitmap(bm);
}

/* Attribute Setters */

static int haikuLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_TEXT)
    return 1;

  BStringView* view = (BStringView*)ih->handle;
  if (!view) return 1;

  int mn = iupStrFindMnemonic(value);
  if (mn) iupKeySetMnemonic(ih, mn, 0);

  char* stripped = haikuStrippedMnemonic(value);
  LooperLockGuard guard(view->Looper());
  view->SetText(stripped ? stripped : "");
  if (stripped) free(stripped);
  return 1;
}

static int haikuLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_TEXT)
    return 1;

  BStringView* view = (BStringView*)ih->handle;
  if (!view) return 1;

  /* IUP "HALIGN:VALIGN"; horizontal via SetAlignment, vertical handled in Draw */
  char halign[20] = {0};
  char valign[20] = {0};
  iupStrToStrStr(value, halign, sizeof(halign), valign, sizeof(valign), ':');

  if (iupStrEqualNoCase(valign, "ATOP"))
    ih->data->vert_alignment = IUP_ALIGN_ATOP;
  else if (iupStrEqualNoCase(valign, "ABOTTOM"))
    ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
  else
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  LooperLockGuard guard(view->Looper());
  view->SetAlignment(haikuParseAlignment(halign));
  view->Invalidate();
  return 1;
}

static int haikuLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_TEXT)
    return 0;

  BStringView* view = (BStringView*)ih->handle;
  if (!view) return 1;

  LooperLockGuard guard(view->Looper());
  view->SetTruncation(iupStrBoolean(value) ? B_TRUNCATE_END : B_NO_TRUNCATION);
  return 1;
}

static int haikuLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* BStringView does not dim visually; iupBaseSetActiveAttrib gates input via iupdrvIsActive */

  /* For image labels, swap to IMINACTIVE / generated grayscale on inactive. */
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
    {
      if (iupStrBoolean(value))
        haikuLabelApplyImage(ih, image, 0);
      else
      {
        char* iminactive = iupAttribGet(ih, "IMINACTIVE");
        if (iminactive)
          haikuLabelApplyImage(ih, iminactive, 0);
        else
          haikuLabelApplyImage(ih, image, 1);
      }
    }
  }
  return iupBaseSetActiveAttrib(ih, value);
}

static int haikuLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_IMAGE || !value)
    return 0;

  if (iupdrvIsActive(ih))
    haikuLabelApplyImage(ih, value, 0);
  else
  {
    char* iminactive = iupAttribGet(ih, "IMINACTIVE");
    if (iminactive)
      haikuLabelApplyImage(ih, iminactive, 0);
    else
      haikuLabelApplyImage(ih, value, 1);
  }
  return 1;
}

static int haikuLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_IMAGE)
    return 0;
  if (!iupdrvIsActive(ih))
  {
    if (value)
      haikuLabelApplyImage(ih, value, 0);
    else
    {
      char* image = iupAttribGet(ih, "IMAGE");
      if (image) haikuLabelApplyImage(ih, image, 1);
    }
  }
  return 1;
}

/* Map */

static int haikuLabelMapMethod(Ihandle* ih)
{
  char* sep = iupAttribGet(ih, "SEPARATOR");
  if (sep)
    ih->data->type = iupStrEqualNoCase(sep, "HORIZONTAL") ? IUP_LABEL_SEP_HORIZ : IUP_LABEL_SEP_VERT;
  else if (iupAttribGet(ih, "IMAGE"))
    ih->data->type = IUP_LABEL_IMAGE;
  else
    ih->data->type = IUP_LABEL_TEXT;

  BView* native = NULL;

  if (ih->data->type == IUP_LABEL_SEP_HORIZ)
  {
    native = new IupHaikuLabelSeparator(true);
  }
  else if (ih->data->type == IUP_LABEL_SEP_VERT)
  {
    native = new IupHaikuLabelSeparator(false);
  }
  else if (ih->data->type == IUP_LABEL_IMAGE)
  {
    native = new IupHaikuLabelImage(ih);
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    char* stripped = haikuStrippedMnemonic(title);
    IupHaikuLabelString* sv = new IupHaikuLabelString(ih, stripped ? stripped : "");
    if (stripped) free(stripped);

    char* align = iupAttribGet(ih, "ALIGNMENT");
    if (align)
    {
      char halign[20] = {0};
      char valign[20] = {0};
      iupStrToStrStr(align, halign, sizeof(halign), valign, sizeof(valign), ':');
      sv->SetAlignment(haikuParseAlignment(halign));
    }

    native = sv;
  }

  ih->handle = (InativeHandle*)native;
  iuphaikuAddToParent(ih);

  {
    Ihandle* native_parent = iupChildTreeGetNativeParent(ih);
    if (native_parent && IupClassMatch(native_parent, "glbackgroundbox") && !iupAttribGet(ih, "BGCOLOR"))
    {
      LooperLockGuard guard(native->Looper());
      native->SetViewColor(B_TRANSPARENT_COLOR);
      iupAttribSet(ih, "_IUPHAIKU_GLTRANSPARENT", "1");
    }
  }

  if (ih->data->type == IUP_LABEL_TEXT)
    iuphaikuUpdateWidgetFont(ih, native);
  else if (ih->data->type == IUP_LABEL_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image) haikuLabelApplyImage(ih, image, 0);
  }

  return IUP_NOERROR;
}

/* Driver hooks */

extern "C" IUP_SDK_API void iupdrvLabelAddExtraPadding(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  (void)x;
  (void)y;
}

extern "C" IUP_SDK_API void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = haikuLabelMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", NULL, haikuLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, haikuLabelSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, haikuLabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, haikuLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, haikuLabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SELECTABLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

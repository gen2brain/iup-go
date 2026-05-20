/** \file
 * \brief Haiku Text Implementation
 *
 * Single-line: BTextControl. Multi-line: BTextView in BScrollView.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <Bitmap.h>
#include <Button.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <Font.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Rect.h>
#include <Region.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_array.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_mask.h"
#include "iup_text.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


#define IUPHAIKU_TEXT_MODIFIED_MSG 'IupT'

static void haikuTextFireCaretCb(Ihandle* ih, BTextView* tv);

/* B_KEY_DOWN filter on the inner BTextView for ACTION char-by-char dispatch
 * (abort / replace key via iupEditCallActionCb). */
class IupHaikuTextKeyFilter : public BMessageFilter
{
public:
  explicit IupHaikuTextKeyFilter(Ihandle* ih)
    : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN),
      fIhandle(ih) {}

  filter_result Filter(BMessage* msg, BHandler** target) override
  {
    if (!fIhandle || !fIhandle->handle || fIhandle->data->disable_callbacks)
      return B_DISPATCH_MESSAGE;

    const char* bytes = NULL;
    ssize_t byte_count = 0;
    if (msg->FindData("bytes", B_STRING_TYPE, (const void**)&bytes, &byte_count) != B_OK || !bytes)
      return B_DISPATCH_MESSAGE;

    BTextView* tv = NULL;
    if (target && *target) tv = dynamic_cast<BTextView*>(*target);
    if (!tv) return B_DISPATCH_MESSAGE;

    /* K_ANY dispatch: walks up the IUP parent chain so dialog-level handlers
       (e.g. autocomplete K_DOWN to shift focus) see the key first. */
    int32 raw_char = 0, mods = 0;
    msg->FindInt32("raw_char", &raw_char);
    msg->FindInt32("modifiers", &mods);
    int code = iuphaikuKeyDecode((unsigned char)bytes[0], raw_char, (unsigned)mods);
    if (code)
    {
      int r = iupKeyCallKeyCb(fIhandle, code);
      if (r == IUP_CLOSE) { IupExitLoop(); return B_SKIP_MESSAGE; }
      if (r == IUP_IGNORE) return B_SKIP_MESSAGE;
    }

    IFnis action_cb = (IFnis)IupGetCallback(fIhandle, "ACTION");
    if (!action_cb && !fIhandle->data->mask && fIhandle->data->nc <= 0)
      return B_DISPATCH_MESSAGE;

    /* "bytes" is stored null-terminated, FindData reports size+1 */
    int char_len = (byte_count > 0 && bytes[byte_count - 1] == 0) ? (int)byte_count - 1 : (int)byte_count;

    /* Control keys reach our hooks via Delete/Select, not InsertText. */
    if (char_len == 1 && (unsigned char)bytes[0] < B_SPACE && bytes[0] != '\n' && bytes[0] != '\t')
      return B_DISPATCH_MESSAGE;

    int32 sel_s = 0, sel_e = 0;
    tv->GetSelection(&sel_s, &sel_e);

    char tmp[8] = {0};
    int copy = char_len < 7 ? char_len : 7;
    memcpy(tmp, bytes, copy);

    const char* filter = iupAttribGet(fIhandle, "FILTER");
    if (filter)
    {
      if (iupStrEqualNoCase(filter, "NUMBER"))
      {
        for (int i = 0; i < copy; i++)
          if (tmp[i] < '0' || tmp[i] > '9')
            return B_SKIP_MESSAGE;
      }
      else if (iupStrEqualNoCase(filter, "UPPERCASE") || iupStrEqualNoCase(filter, "LOWERCASE"))
      {
        bool to_upper = iupStrEqualNoCase(filter, "UPPERCASE");
        bool changed = false;
        for (int i = 0; i < copy; i++)
        {
          char c = (char)(to_upper ? iup_toupper(tmp[i]) : iup_tolower(tmp[i]));
          if (c != tmp[i]) { tmp[i] = c; changed = true; }
        }
        if (changed)
        {
          msg->ReplaceData("bytes", B_STRING_TYPE, tmp, copy + 1);
          if (copy == 1)
          {
            int8 raw = (int8)tmp[0];
            if (msg->HasInt8("byte")) msg->ReplaceInt8("byte", raw);
            else                       msg->AddInt8("byte", raw);
          }
        }
      }
    }

    int ret = iupEditCallActionCb(fIhandle, action_cb, tmp, sel_s, sel_e,
                                  fIhandle->data->mask,
                                  fIhandle->data->nc, 0, 1);
    if (ret == 0) return B_SKIP_MESSAGE;
    if (ret != -1 && char_len == 1)
    {
      char rep[2] = { (char)ret, 0 };
      msg->ReplaceData("bytes", B_STRING_TYPE, rep, 2);
      int8 raw = (int8)rep[0];
      if (msg->HasInt8("byte")) msg->ReplaceInt8("byte", raw);
      else                           msg->AddInt8("byte", raw);
    }
    return B_DISPATCH_MESSAGE;
  }

private:
  Ihandle* fIhandle;
};


class IupHaikuTextControl : public BTextControl
{
public:
  explicit IupHaikuTextControl(Ihandle* ih)
    : BTextControl(BRect(0, 0, 0, 0), "iup_text", NULL, "",
                   NULL, B_FOLLOW_NONE),
      fIhandle(ih), fKeyFilter(NULL), fMute(false)
  {
    BTextControl::SetDivider(0);
    SetFlags(Flags() | B_DRAW_ON_CHILDREN);
  }

  ~IupHaikuTextControl() override
  {
    if (fKeyFilter && TextView())
      TextView()->RemoveFilter(fKeyFilter);
    delete fKeyFilter;
  }

  void AttachedToWindow() override
  {
    BTextControl::AttachedToWindow();
    SetTarget(this);
    SetModificationMessage(new BMessage(IUPHAIKU_TEXT_MODIFIED_MSG));

    if (fIhandle && !fKeyFilter && TextView())
    {
      fKeyFilter = new IupHaikuTextKeyFilter(fIhandle);
      TextView()->AddFilter(fKeyFilter);
    }
  }

  /* SetDivider resets fText's frame; SetInsets + SetTextRect drive _ResetTextRect to re-center H and V. */
  void FrameResized(float w, float h) override
  {
    BTextControl::FrameResized(w, h);
    SetDivider(0);
    BTextView* tv = TextView();
    if (!tv || !tv->Bounds().IsValid()) return;

    float h_inset = floorf(be_control_look->DefaultLabelSpacing() / 2.0f);
    float line_h = tv->LineHeight(0);
    float v_inset = (tv->Bounds().Height() + 1 - line_h) / 2;
    if (v_inset < 1) v_inset = 1;
    tv->SetInsets(h_inset, v_inset, h_inset, v_inset);
    tv->SetTextRect(tv->Bounds());
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TEXT_MODIFIED_MSG && fIhandle)
    {
      Icallback cb = IupGetCallback(fIhandle, "VALUECHANGED_CB");
      if (cb)
      {
        int ret = cb(fIhandle);
        if (ret == IUP_CLOSE) IupExitLoop();
      }
      if (TextView()) haikuTextFireCaretCb(fIhandle, TextView());
      return;
    }
    BTextControl::MessageReceived(msg);
  }

  void DrawAfterChildren(BRect updateRect) override
  {
    if (!fIhandle) return;
    const char* cue = iupAttribGet(fIhandle, "CUEBANNER");
    if (!cue || !cue[0]) return;
    if (TextLength() != 0) return;
    BTextView* tv = TextView();
    if (!tv || tv->IsFocus()) return;

    BRect tf = tv->Frame();
    if (!tf.Intersects(updateRect)) return;

    BFont font;
    rgb_color text_color;
    tv->GetFontAndColor(0, &font, &text_color);
    SetFont(&font);
    SetHighColor(mix_color(text_color, tv->ViewColor(), 128));
    SetLowColor(B_TRANSPARENT_COLOR);

    font_height fh;
    font.GetHeight(&fh);
    BPoint p(tf.left + ceilf(be_control_look->DefaultLabelSpacing() / 2.0f),
             tf.top + ceilf((tf.Height() + 1 - fh.ascent - fh.descent) / 2.0f) + fh.ascent);
    DrawString(cue, p);
  }

  /* Clear modification message around programmatic SetText so the queued
   * InvokeNotify doesn't fire VALUECHANGED_CB for non-user edits. */
  void MuteBegin()
  {
    if (fMute) return;
    fMute = true;
    SetModificationMessage(NULL);
  }
  void MuteEnd()
  {
    if (!fMute) return;
    fMute = false;
    SetModificationMessage(new BMessage(IUPHAIKU_TEXT_MODIFIED_MSG));
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
  IupHaikuTextKeyFilter* fKeyFilter;
  bool fMute;
};


class IupHaikuTextView : public BTextView
{
public:
  struct BgRange { int32 start; int32 end; rgb_color color; };
  struct ImageRange { int32 offset; BBitmap* bm; int w; int h; };

  explicit IupHaikuTextView(Ihandle* ih)
    : BTextView(BRect(0, 0, 100, 100), "iup_textview", BRect(2, 2, 98, 98),
                B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS),
      fIhandle(ih), fSuppress(false), fOverwrite(false),
      fLinkCursor(B_CURSOR_ID_FOLLOW_LINK)
  {
  }

  void FrameResized(float w, float h) override
  {
    BTextView::FrameResized(w, h);
    BRect b = Bounds();
    BRect tr(b.left + 2, b.top + 2, b.right - 2, b.bottom - 2);
    if (tr.IsValid() && tr != TextRect()) SetTextRect(tr);
  }

  void Draw(BRect updateRect) override
  {
    BTextView::Draw(updateRect);
    if (!fBgRanges.empty())
    {
      PushState();
      SetDrawingMode(B_OP_ALPHA);
      SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
      for (size_t i = 0; i < fBgRanges.size(); ++i)
      {
        const BgRange& bg = fBgRanges[i];
        if (bg.start >= bg.end) continue;
        BRegion reg;
        GetTextRegion(bg.start, bg.end, &reg);
        rgb_color c = bg.color;
        c.alpha = 110;
        SetHighColor(c);
        for (int32 j = 0; j < reg.CountRects(); ++j)
        {
          BRect r = reg.RectAt(j);
          if (r.Intersects(updateRect)) FillRect(r);
        }
      }
      PopState();
    }
    if (!fImageRanges.empty())
    {
      PushState();
      SetDrawingMode(B_OP_ALPHA);
      SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
      for (size_t i = 0; i < fImageRanges.size(); ++i)
      {
        const ImageRange& ir = fImageRanges[i];
        if (!ir.bm) continue;
        float lh = 0;
        BPoint p = PointAt(ir.offset, &lh);
        BRect dst(p.x, p.y, p.x + ir.w - 1, p.y + ir.h - 1);
        if (dst.Intersects(updateRect)) DrawBitmap(ir.bm, dst);
      }
      PopState();
    }
  }

  void AddBgRange(int32 start, int32 end, rgb_color color)
  {
    if (start >= end) return;
    fBgRanges.push_back({start, end, color});
    Invalidate();
  }

  void ClearBgRanges(int32 from, int32 to)
  {
    if (from <= 0 && to >= TextLength())
    {
      fBgRanges.clear();
    }
    else
    {
      std::vector<BgRange> kept;
      for (size_t i = 0; i < fBgRanges.size(); ++i)
      {
        const BgRange& r = fBgRanges[i];
        if (r.end <= from || r.start >= to) kept.push_back(r);
      }
      fBgRanges.swap(kept);
    }
    Invalidate();
  }

  void AddImageRange(int32 offset, BBitmap* bm, int w, int h)
  {
    if (!bm) return;
    fImageRanges.push_back({offset, bm, w, h});
    Invalidate();
  }

  void ClearImageRanges(int32 from, int32 to)
  {
    if (from <= 0 && to >= TextLength())
    {
      fImageRanges.clear();
    }
    else
    {
      std::vector<ImageRange> kept;
      for (size_t i = 0; i < fImageRanges.size(); ++i)
      {
        const ImageRange& r = fImageRanges[i];
        if (r.offset < from || r.offset >= to) kept.push_back(r);
      }
      fImageRanges.swap(kept);
    }
    Invalidate();
  }

  void ShiftImageRanges(int32 from, int32 delta)
  {
    for (size_t i = 0; i < fImageRanges.size(); ++i)
      if (fImageRanges[i].offset >= from) fImageRanges[i].offset += delta;
  }

protected:
  void InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs) override
  {
    if (fSuppress || !fIhandle || fIhandle->data->disable_callbacks)
    {
      BTextView::InsertText(text, length, offset, runs);
      ShiftImageRanges(offset + 1, length);
      return;
    }

    if (fOverwrite && length == 1 && text && text[0] != '\n')
    {
      int32 tlen = TextLength();
      if (offset < tlen && ByteAt(offset) != '\n')
      {
        ClearImageRanges(offset, offset + 1);
        BTextView::DeleteText(offset, offset + 1);
        ShiftImageRanges(offset + 1, -1);
      }
    }

    const char* filter = iupAttribGet(fIhandle, "FILTER");
    char* filter_buf = NULL;
    if (filter && text && length > 0)
    {
      if (iupStrEqualNoCase(filter, "NUMBER"))
      {
        for (int32 i = 0; i < length; i++)
          if (text[i] < '0' || text[i] > '9') return;
      }
      else if (iupStrEqualNoCase(filter, "UPPERCASE") || iupStrEqualNoCase(filter, "LOWERCASE"))
      {
        bool to_upper = iupStrEqualNoCase(filter, "UPPERCASE");
        filter_buf = (char*)malloc(length);
        if (filter_buf)
        {
          for (int32 i = 0; i < length; i++)
            filter_buf[i] = (char)(to_upper ? iup_toupper(text[i]) : iup_tolower(text[i]));
          text = filter_buf;
        }
      }
    }

    IFnis action_cb = (IFnis)IupGetCallback(fIhandle, "ACTION");
    if (action_cb || fIhandle->data->mask || fIhandle->data->nc > 0)
    {
      int32 sel_s = 0, sel_e = 0;
      GetSelection(&sel_s, &sel_e);
      int start = (offset == sel_s) ? sel_s : offset;
      int end   = (offset == sel_s) ? sel_e : offset;

      char* tmp = (char*)malloc(length + 1);
      memcpy(tmp, text, length);
      tmp[length] = '\0';

      int ret = iupEditCallActionCb(fIhandle, action_cb, tmp, start, end,
                                    (void*)fIhandle->data->mask,
                                    fIhandle->data->nc, 0, 1);
      free(tmp);

      if (ret == 0) { free(filter_buf); return; }
      if (ret != -1 && length == 1)
      {
        char rep = (char)ret;
        BTextView::InsertText(&rep, 1, offset, runs);
        ShiftImageRanges(offset + 1, 1);
        fireValueChanged();
        free(filter_buf);
        return;
      }
    }

    BTextView::InsertText(text, length, offset, runs);
    ShiftImageRanges(offset + 1, length);
    fireValueChanged();
    free(filter_buf);
  }

  void DeleteText(int32 fromOffset, int32 toOffset) override
  {
    if (fSuppress || !fIhandle || fIhandle->data->disable_callbacks)
    {
      ClearImageRanges(fromOffset, toOffset);
      BTextView::DeleteText(fromOffset, toOffset);
      ShiftImageRanges(toOffset, fromOffset - toOffset);
      return;
    }

    IFnis action_cb = (IFnis)IupGetCallback(fIhandle, "ACTION");
    if (action_cb || fIhandle->data->mask)
    {
      int ret = iupEditCallActionCb(fIhandle, action_cb, NULL,
                                    fromOffset, toOffset,
                                    fIhandle->data->mask,
                                    fIhandle->data->nc, 0, 1);
      if (ret == 0) return;
    }

    ClearImageRanges(fromOffset, toOffset);
    BTextView::DeleteText(fromOffset, toOffset);
    ShiftImageRanges(toOffset, fromOffset - toOffset);
    fireValueChanged();
  }

public:
  void Select(int32 startOffset, int32 endOffset) override
  {
    BTextView::Select(startOffset, endOffset);
    if (!fSuppress && fIhandle) haikuTextFireCaretCb(fIhandle, this);
  }

  bool LinkAt(BPoint where) const
  {
    if (!fIhandle) return false;
    int count = iupAttribGetInt(fIhandle, "_IUPHAIKU_LINK_COUNT");
    if (count <= 0) return false;
    int32 pos = this->OffsetAt(where);
    for (int i = 0; i < count; ++i)
    {
      char key[64];
      snprintf(key, sizeof(key), "_IUPHAIKU_LINK_RANGE_%d", i);
      int s = 0, e = 0;
      if (iupStrToIntInt(iupAttribGet(fIhandle, key), &s, &e, ':') == 2 &&
          pos >= s && pos < e)
        return true;
    }
    return false;
  }

  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  {
    BTextView::MouseMoved(where, transit, drag);
    if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW) return;
    SetViewCursor(LinkAt(where) ? &fLinkCursor : B_CURSOR_I_BEAM, true);
  }

  void MouseDown(BPoint where) override
  {
    if (fIhandle && !fIhandle->data->disable_callbacks)
    {
      int count = iupAttribGetInt(fIhandle, "_IUPHAIKU_LINK_COUNT");
      if (count > 0)
      {
        int32 pos = OffsetAt(where);
        for (int i = 0; i < count; ++i)
        {
          char key[64];
          snprintf(key, sizeof(key), "_IUPHAIKU_LINK_RANGE_%d", i);
          int s = 0, e = 0;
          if (iupStrToIntInt(iupAttribGet(fIhandle, key), &s, &e, ':') == 2 &&
              pos >= s && pos < e)
          {
            snprintf(key, sizeof(key), "_IUPHAIKU_LINK_URL_%d", i);
            char* url = iupAttribGet(fIhandle, key);
            if (url)
            {
              IFns cb = (IFns)IupGetCallback(fIhandle, "LINK_CB");
              if (cb)
              {
                int ret = cb(fIhandle, url);
                if (ret == IUP_CLOSE) IupExitLoop();
                if (ret == IUP_DEFAULT) IupHelp(url);
              }
              else
              {
                IupHelp(url);
              }
              return;
            }
          }
        }
      }
    }
    BTextView::MouseDown(where);
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }
  void SetSuppress(bool s) { fSuppress = s; }
  void SetOverwrite(bool o) { fOverwrite = o; }

private:
  void fireValueChanged()
  {
    if (!fIhandle || fIhandle->data->disable_callbacks) return;
    Icallback cb = IupGetCallback(fIhandle, "VALUECHANGED_CB");
    if (cb)
    {
      int ret = cb(fIhandle);
      if (ret == IUP_CLOSE) IupExitLoop();
    }
  }

  Ihandle* fIhandle;
  bool fSuppress;
  bool fOverwrite;
  BCursor fLinkCursor;
  std::vector<BgRange> fBgRanges;
  std::vector<ImageRange> fImageRanges;
};


static void haikuTextFireCaretCb(Ihandle* ih, BTextView* tv)
{
  if (!ih || !tv) return;
  if (ih->data->disable_callbacks) return;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  int32 ln = tv->LineAt(s);
  int32 ls = tv->OffsetAt(ln);
  int lin = ln + 1;
  int col = s - ls + 1;
  int pos = s;

  /* Suppress duplicates within the same caret position. */
  int last = iupAttribGetInt(ih, "_IUPHAIKU_LAST_CARET");
  if (last == pos + 1) return;
  iupAttribSetInt(ih, "_IUPHAIKU_LAST_CARET", pos + 1);

  cb(ih, lin, col, pos);
}

#define IUPHAIKU_SPIN_UP_MSG   'IupU'
#define IUPHAIKU_SPIN_DOWN_MSG 'IupD'

class IupHaikuSpinWrap : public BView
{
public:
  IupHaikuSpinWrap(Ihandle* ih, IupHaikuTextControl* tc)
    : BView(BRect(0, 0, 0, 0), "iup_spin_wrap", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fIhandle(ih), fText(tc),
      fMin(0), fMax(100), fStep(1), fValue(0), fWrap(false)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    fUp   = new BButton(BRect(0,0,0,0), "iup_spin_up",   "+",
                        new BMessage(IUPHAIKU_SPIN_UP_MSG),   B_FOLLOW_NONE);
    fDown = new BButton(BRect(0,0,0,0), "iup_spin_down", "\xe2\x88\x92",  /* − */
                        new BMessage(IUPHAIKU_SPIN_DOWN_MSG), B_FOLLOW_NONE);
    AddChild(fText);
    AddChild(fUp);
    AddChild(fDown);
  }

  IupHaikuTextControl* TextControl() const { return fText; }
  void SetMin(int32 v)  { fMin = v; }
  void SetMax(int32 v)  { fMax = v; }
  void SetStep(int32 v) { fStep = v > 0 ? v : 1; }
  void SetWrap(bool w)  { fWrap = w; }
  int32 Value() const { return fValue; }
  void SetSpinValue(int32 v) { fValue = v; }

  bool IsSpinAuto() const
  {
    return !fIhandle || iupAttribGetBoolean(fIhandle, "SPINAUTO");
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; if (fText) fText->SetIhandle(ih); }

  void AttachedToWindow() override
  {
    BView::AttachedToWindow();
    fUp->SetTarget(this);
    fDown->SetTarget(this);
  }

  void FrameResized(float w, float h) override
  {
    BView::FrameResized(w, h);
    Layout();
  }

  void Layout()
  {
    BRect b = Bounds();
    float btn_w = 22;
    fText->MoveTo(0, 0);
    fText->ResizeTo(b.Width() - btn_w - 2, b.Height());
    float half = (b.Height() - 1) / 2;
    fUp->MoveTo(b.Width() - btn_w, 0);
    fUp->ResizeTo(btn_w, half);
    fDown->MoveTo(b.Width() - btn_w, half + 1);
    fDown->ResizeTo(btn_w, b.Height() - half - 1);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_SPIN_UP_MSG)   { Adjust(+1); return; }
    if (msg && msg->what == IUPHAIKU_SPIN_DOWN_MSG) { Adjust(-1); return; }
    BView::MessageReceived(msg);
  }

private:
  void Adjust(int dir)
  {
    int32 nw = fValue + dir * fStep;
    if (nw > fMax) nw = fWrap ? fMin : fMax;
    if (nw < fMin) nw = fWrap ? fMax : fMin;
    fValue = nw;
    if (IsSpinAuto())
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "%d", (int)nw);
      fText->MuteBegin();
      fText->SetText(buf);
      fText->MuteEnd();
    }
    if (fIhandle)
    {
      IFni cb = (IFni)IupGetCallback(fIhandle, "SPIN_CB");
      if (cb) cb(fIhandle, (int)nw);
    }
  }

  Ihandle* fIhandle;
  IupHaikuTextControl* fText;
  BButton* fUp;
  BButton* fDown;
  int32 fMin, fMax, fStep, fValue;
  bool fWrap;
};

static IupHaikuSpinWrap* haikuTextGetSpinWrap(Ihandle* ih)
{
  if (!ih || !ih->handle || ih->data->is_multiline) return NULL;
  return dynamic_cast<IupHaikuSpinWrap*>((BView*)ih->handle);
}

static BTextControl* haikuTextGetTextControl(Ihandle* ih)
{
  if (!ih || !ih->handle || ih->data->is_multiline) return NULL;
  if (IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih))
    return sw->TextControl();
  return (BTextControl*)ih->handle;
}

static BTextView* haikuTextGetEditor(Ihandle* ih)
{
  if (!ih || !ih->handle) return NULL;
  if (ih->data->is_multiline)
    return (BTextView*)iupAttribGet(ih, "_IUPHAIKU_TEXT_INNER");
  BTextControl* tc = haikuTextGetTextControl(ih);
  return tc ? tc->TextView() : NULL;
}

static BLooper* haikuTextGetLooper(Ihandle* ih)
{
  if (!ih || !ih->handle) return NULL;
  return ((BView*)ih->handle)->Looper();
}

static IupHaikuTextView* haikuTextGetMultilineView(Ihandle* ih)
{
  if (!ih || !ih->data->is_multiline) return NULL;
  return (IupHaikuTextView*)iupAttribGet(ih, "_IUPHAIKU_TEXT_INNER");
}

static int haikuTextParseRange(BTextView* tv, const char* sel, bool one_based, int32* start, int32* end)
{
  if (!tv || !sel) return 0;
  int s = 0, e = 0;
  if (sscanf(sel, "%d:%d", &s, &e) != 2) return 0;
  if (one_based) { s -= 1; e -= 1; }
  if (s < 0) s = 0;
  if (e < s) e = s;
  *start = s;
  *end = e;
  return 1;
}

static void haikuTextSetTextSuppressed(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    IupHaikuTextView* tv = haikuTextGetMultilineView(ih);
    if (tv)
    {
      tv->SetSuppress(true);
      tv->SetText(value ? value : "");
      tv->SetSuppress(false);
    }
  }
  else
  {
    if (IupHaikuTextControl* tc = dynamic_cast<IupHaikuTextControl*>(haikuTextGetTextControl(ih)))
    {
      tc->MuteBegin();
      tc->SetText(value ? value : "");
      tc->MuteEnd();
    }
  }
}

static int haikuTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  haikuTextSetTextSuppressed(ih, value);
  return 0;
}

static char* haikuTextGetValueAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  if (ih->data->is_multiline)
  {
    BTextView* tv = haikuTextGetEditor(ih);
    if (!tv) return NULL;
    int32 len = tv->TextLength();
    char* buf = iupStrGetMemory(len + 1);
    tv->GetText(0, len, buf);
    buf[len] = '\0';
    return buf;
  }
  BTextControl* tc = haikuTextGetTextControl(ih);
  return tc ? iupStrReturnStr(tc->Text()) : NULL;
}

static int haikuTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 1;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  tv->MakeEditable(!iupStrBoolean(value));
  return 1;
}

static char* haikuTextGetReadOnlyAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  return iupStrReturnBoolean(!tv->IsEditable());
}

static int haikuTextSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    LooperLockGuard guard(haikuTextGetLooper(ih));
    if (ih->data->is_multiline)
    {
      BTextView* tv = haikuTextGetEditor(ih);
      if (tv) tv->MakeEditable(iupStrBoolean(value) && !iupAttribGetBoolean(ih, "READONLY"));
    }
    else
    {
      if (BTextControl* tc = haikuTextGetTextControl(ih))
        tc->SetEnabled(iupStrBoolean(value));
    }
  }
  return iupBaseSetActiveAttrib(ih, value);
}

static int haikuTextSetNCAttrib(Ihandle* ih, const char* value)
{
  int nc;
  if (!iupStrToInt(value, &nc)) return 0;
  ih->data->nc = nc;

  if (ih->handle && nc > 0 && !ih->data->is_multiline)
  {
    BTextControl* tc = haikuTextGetTextControl(ih);
    if (tc)
    {
      LooperLockGuard guard(tc->Looper());
      tc->TextView()->SetMaxBytes(nc);
    }
  }
  /* Multi-line NC is enforced inside our InsertText override. */
  return 0;
}

static int haikuTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    int32 s = 0, e = 0;
    tv->GetSelection(&s, &e);
    tv->Select(e, e);
    return 0;
  }
  if (iupStrEqualNoCase(value, "ALL"))
  {
    tv->SelectAll();
    return 0;
  }
  int32 s = 0, e = 0;
  if (haikuTextParseRange(tv, value, true /*1-based*/, &s, &e))
    tv->Select(s, e);
  return 0;
}

static int haikuTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    int32 s = 0, e = 0;
    tv->GetSelection(&s, &e);
    tv->Select(e, e);
    return 0;
  }
  if (iupStrEqualNoCase(value, "ALL"))
  {
    tv->SelectAll();
    return 0;
  }
  int32 s = 0, e = 0;
  if (haikuTextParseRange(tv, value, false /*0-based*/, &s, &e))
    tv->Select(s, e);
  return 0;
}

static char* haikuTextGetSelectionAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  if (s == e) return NULL;
  return iupStrReturnStrf("%d:%d", s + 1, e + 1);
}

static char* haikuTextGetSelectionPosAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  if (s == e) return NULL;
  return iupStrReturnStrf("%d:%d", (int)s, (int)e);
}

static char* haikuTextGetSelectedTextAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  if (s == e) return NULL;
  int len = e - s;
  char* buf = iupStrGetMemory(len + 1);
  tv->GetText(s, len, buf);
  buf[len] = '\0';
  return buf;
}

static int haikuTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 0;
  int pos = 0;
  if (!iupStrToInt(value, &pos)) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  if (pos < 0) pos = 0;
  if (pos > tv->TextLength()) pos = tv->TextLength();
  tv->Select(pos, pos);
  tv->ScrollToOffset(pos);
  return 0;
}

static char* haikuTextGetCaretPosAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  return iupStrReturnInt((int)s);
}

static int haikuTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
  {
    int col = 1;
    iupStrToInt(value, &col);
    col--;
    if (col < 0) col = 0;
    return haikuTextSetCaretPosAttrib(ih, iupStrReturnInt(col));
  }
  int lin = 1, col = 1;
  if (iupStrToIntInt(value, &lin, &col, ',') != 2) return 0;
  int pos = 0;
  iupdrvTextConvertLinColToPos(ih, lin, col, &pos);
  return haikuTextSetCaretPosAttrib(ih, iupStrReturnInt(pos));
}

static char* haikuTextGetCaretAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  if (!ih->data->is_multiline) return iupStrReturnInt((int)s + 1);
  int lin = 1, col = 1;
  iupdrvTextConvertPosToLinCol(ih, s, &lin, &col);
  return iupStrReturnIntInt(lin, col, ',');
}

static int haikuTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));

  if (ih->data->is_multiline)
  {
    IupHaikuTextView* tv = haikuTextGetMultilineView(ih);
    if (!tv) return 0;
    int32 end = tv->TextLength();
    const char* prefix = (ih->data->append_newline && end > 0) ? "\n" : "";
    char* full = (char*)malloc(strlen(prefix) + strlen(value) + 1);
    strcpy(full, prefix);
    strcat(full, value);
    ih->data->disable_callbacks = 1;
    tv->Insert(end, full, strlen(full));
    ih->data->disable_callbacks = 0;
    free(full);
    tv->ScrollToOffset(tv->TextLength());
  }
  else
  {
    BTextControl* tc = haikuTextGetTextControl(ih);
    if (!tc) return 0;
    int32 len = tc->TextLength();
    char* full = (char*)malloc(len + strlen(value) + 1);
    strcpy(full, tc->Text());
    strcat(full, value);
    if (IupHaikuTextControl* itc = dynamic_cast<IupHaikuTextControl*>(tc))
    {
      itc->MuteBegin();
      tc->SetText(full);
      itc->MuteEnd();
    }
    free(full);
  }
  return 0;
}

static int haikuTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  if (s != e) tv->Delete(s, e);
  tv->Insert(s, value, strlen(value));
  tv->Select(s + (int32)strlen(value), s + (int32)strlen(value));
  return 0;
}

static int haikuTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  /* Multi-line: "lin,col" 1-based. Single-line: integer pos. */
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    if (iupStrToIntInt(value, &lin, &col, ',') < 1) return 0;
    int pos = 0;
    iupdrvTextConvertLinColToPos(ih, lin, col, &pos);
    tv->ScrollToOffset(pos);
  }
  else
  {
    int pos = 0;
    if (!iupStrToInt(value, &pos)) return 0;
    tv->ScrollToOffset(pos);
  }
  return 0;
}

static int haikuTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 0;
  int pos = 0;
  if (!iupStrToInt(value, &pos)) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  tv->ScrollToOffset(pos);
  return 0;
}

static int haikuTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv || !value) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  if      (iupStrEqualNoCase(value, "COPY"))  tv->Copy(be_clipboard);
  else if (iupStrEqualNoCase(value, "CUT"))   tv->Cut(be_clipboard);
  else if (iupStrEqualNoCase(value, "PASTE")) tv->Paste(be_clipboard);
  else if (iupStrEqualNoCase(value, "CLEAR")) tv->Clear();
  else if (iupStrEqualNoCase(value, "UNDO") || iupStrEqualNoCase(value, "REDO"))
  {
    /* BTextView::Undo toggles direction; UndoState(isRedo) reports which way the next call goes */
    bool is_redo = false;
    undo_state st = tv->UndoState(&is_redo);
    if (st == B_UNDO_UNAVAILABLE) return 0;
    bool want_redo = iupStrEqualNoCase(value, "REDO") ? true : false;
    if (is_redo == want_redo) tv->Undo(be_clipboard);
  }
  return 0;
}

static int haikuTextSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline) return 0;
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 1;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  tv->SetWordWrap(iupStrBoolean(value));
  return 1;
}

static int haikuTextSetPasswordAttrib(Ihandle* ih, const char* value)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 1;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  tv->HideTyping(iupStrBoolean(value) ? true : false);
  return 1;
}

static int haikuTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->is_multiline)
    return 0;
  IupHaikuTextControl* tc = (IupHaikuTextControl*)ih->handle;
  if (tc)
  {
    LooperLockGuard guard(haikuTextGetLooper(ih));
    tc->Invalidate();
  }
  return 1;
}

static int haikuTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline) return 0;
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 1;
  int n;
  if (!iupStrToInt(value, &n)) return 0;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  BFont font;
  tv->GetFont(&font);
  tv->SetTabWidth(font.StringWidth(" ") * (float)n);
  return 1;
}

static int haikuTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTextView* tv = haikuTextGetMultilineView(ih);
  if (!tv) return 0;
  tv->SetOverwrite(iupStrBoolean(value) ? true : false);
  return 1;
}

static int haikuTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  alignment a = B_ALIGN_LEFT;
  if      (iupStrEqualNoCase(value, "ARIGHT"))  a = B_ALIGN_RIGHT;
  else if (iupStrEqualNoCase(value, "ACENTER")) a = B_ALIGN_CENTER;
  LooperLockGuard guard(haikuTextGetLooper(ih));
  if (ih->data->is_multiline)
  {
    BTextView* tv = haikuTextGetEditor(ih);
    if (tv) tv->SetAlignment(a);
  }
  else
  {
    BTextControl* tc = haikuTextGetTextControl(ih);
    if (tc) tc->SetAlignment(B_ALIGN_LEFT, a);
  }
  return 1;
}

/* BTextView has no per-range hooks for ALIGNMENT / INDENT / RISE. */
static void haikuTextApplyFormatTagFont(BFont& bfont, Ihandle* tag, uint32* mode)
{
  *mode = 0;

  /* FONT (full IUP font string takes precedence). */
  char* font_value = iupAttribGet(tag, "FONT");
  if (font_value)
  {
    char typeface[1024] = {0};
    int size = 0, bold = 0, italic = 0, underline = 0, strikeout = 0;
    if (iupGetFontInfo(font_value, typeface, &size, &bold, &italic, &underline, &strikeout))
    {
      if (typeface[0])
      {
        int32 family_count = count_font_families();
        for (int32 i = 0; i < family_count; ++i)
        {
          font_family family;
          uint32 flags = 0;
          if (get_font_family(i, &family, &flags) == B_OK &&
              strcasecmp(family, typeface) == 0)
          {
            font_style style;
            strcpy(style, "Regular");
            bfont.SetFamilyAndStyle(family, style);
            *mode |= B_FONT_FAMILY_AND_STYLE;
            break;
          }
        }
      }
      if (size > 0) { bfont.SetSize((float)size); *mode |= B_FONT_SIZE; }
      uint16 face = 0;
      if (bold) face |= B_BOLD_FACE;
      if (italic) face |= B_ITALIC_FACE;
      if (underline) face |= B_UNDERSCORE_FACE;
      if (strikeout) face |= B_STRIKEOUT_FACE;
      if (face) { bfont.SetFace(face); *mode |= B_FONT_FACE; }
    }
  }

  /* FONTFACE: family name only, preserve current size/face. */
  char* fontface = iupAttribGet(tag, "FONTFACE");
  if (fontface)
  {
    int32 family_count = count_font_families();
    for (int32 i = 0; i < family_count; ++i)
    {
      font_family family;
      uint32 flags = 0;
      if (get_font_family(i, &family, &flags) == B_OK &&
          strcasecmp(family, fontface) == 0)
      {
        font_style style;
        bfont.GetFamilyAndStyle(NULL, &style);
        bfont.SetFamilyAndStyle(family, style);
        *mode |= B_FONT_FAMILY_AND_STYLE;
        break;
      }
    }
  }

  /* Granular per-attribute overrides. */
  char* fontsize = iupAttribGet(tag, "FONTSIZE");
  if (fontsize)
  {
    int sz = 0;
    if (iupStrToInt(fontsize, &sz) && sz > 0)
    {
      bfont.SetSize((float)sz);
      *mode |= B_FONT_SIZE;
    }
  }

  char* fontscale = iupAttribGet(tag, "FONTSCALE");
  if (fontscale)
  {
    double scale = 1.0;
    if      (iupStrEqualNoCase(fontscale, "XX-LARGE")) scale = 2.0;
    else if (iupStrEqualNoCase(fontscale, "X-LARGE"))  scale = 1.5;
    else if (iupStrEqualNoCase(fontscale, "LARGE"))    scale = 1.25;
    else if (iupStrEqualNoCase(fontscale, "MEDIUM"))   scale = 1.0;
    else if (iupStrEqualNoCase(fontscale, "SMALL"))    scale = 0.83;
    else if (iupStrEqualNoCase(fontscale, "X-SMALL"))  scale = 0.69;
    else if (iupStrEqualNoCase(fontscale, "XX-SMALL")) scale = 0.58;
    else iupStrToDouble(fontscale, &scale);
    bfont.SetSize((float)(bfont.Size() * scale));
    *mode |= B_FONT_SIZE;
  }

  uint16 face = bfont.Face();
  bool face_changed = false;

  char* weight = iupAttribGet(tag, "WEIGHT");
  if (weight)
  {
    if (iupStrEqualNoCase(weight, "BOLD") || iupStrEqualNoCase(weight, "EXTRABOLD"))
      face |= B_BOLD_FACE;
    else
      face &= ~B_BOLD_FACE;
    face_changed = true;
  }

  char* italic = iupAttribGet(tag, "ITALIC");
  if (italic)
  {
    if (iupStrBoolean(italic)) face |= B_ITALIC_FACE; else face &= ~B_ITALIC_FACE;
    face_changed = true;
  }

  char* underline = iupAttribGet(tag, "UNDERLINE");
  if (underline)
  {
    if (iupStrEqualNoCase(underline, "NONE")) face &= ~B_UNDERSCORE_FACE;
    else                                       face |= B_UNDERSCORE_FACE;
    face_changed = true;
  }

  char* strikeout = iupAttribGet(tag, "STRIKEOUT");
  if (strikeout)
  {
    if (iupStrBoolean(strikeout)) face |= B_STRIKEOUT_FACE; else face &= ~B_STRIKEOUT_FACE;
    face_changed = true;
  }

  if (face_changed) { bfont.SetFace(face); *mode |= B_FONT_FACE; }
}

extern "C" IUP_SDK_API void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* tag, int /*bulk*/)
{
  if (!ih->data->is_multiline) return;
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv || !tag) return;

  LooperLockGuard guard(haikuTextGetLooper(ih));

  int32 start = 0, end = 0;
  char* sel = iupAttribGet(tag, "SELECTION");
  char* sel_pos = iupAttribGet(tag, "SELECTIONPOS");
  if (sel)
  {
    /* SELECTION format on tag: "lin1,col1:lin2,col2" or "start:end" linear. */
    int l1 = 1, c1 = 1, l2 = 1, c2 = 1;
    if (sscanf(sel, "%d,%d:%d,%d", &l1, &c1, &l2, &c2) == 4)
    {
      int p1 = 0, p2 = 0;
      iupdrvTextConvertLinColToPos(ih, l1, c1, &p1);
      iupdrvTextConvertLinColToPos(ih, l2, c2, &p2);
      start = p1; end = p2;
    }
    else if (haikuTextParseRange(tv, sel, true, &start, &end)) { /* range parsed */ }
    else return;
  }
  else if (sel_pos)
  {
    if (!haikuTextParseRange(tv, sel_pos, false, &start, &end)) return;
  }
  else
  {
    int32 s = 0, e = 0;
    tv->GetSelection(&s, &e);
    start = s; end = e;
  }

  /* IMAGE replaces the range with a U+FFFC placeholder; BBitmap drawn over it. */
  char* image_name = iupAttribGet(tag, "IMAGE");
  if (image_name)
  {
    BBitmap* bm = (BBitmap*)iupImageGetImage(image_name, ih, 0, NULL);
    if (!bm) return;
    int img_w = (int)(bm->Bounds().Width() + 1);
    int img_h = (int)(bm->Bounds().Height() + 1);
    int nw = 0, nh = 0;
    char* a = iupAttribGet(tag, "WIDTH");  if (a) iupStrToInt(a, &nw);
    a = iupAttribGet(tag, "HEIGHT");       if (a) iupStrToInt(a, &nh);
    if (nw > 0) img_w = nw;
    if (nh > 0) img_h = nh;

    IupHaikuTextView* itv = (IupHaikuTextView*)tv;
    itv->SetSuppress(true);
    if (end > start) tv->Delete(start, end);
    static const char ph[3] = { (char)0xEF, (char)0xBF, (char)0xBC };
    tv->Insert(start, ph, 3);
    BFont ifont(be_plain_font);
    ifont.SetSize((float)img_h);
    rgb_color clear = { 0, 0, 0, 0 };
    tv->SetFontAndColor(start, start + 3, &ifont, B_FONT_ALL, &clear);
    itv->AddImageRange(start, bm, img_w, img_h);
    itv->SetSuppress(false);
    return;
  }

  if (start == end) return;

  BFont bfont;
  rgb_color color;
  tv->GetFontAndColor(start, &bfont, &color);

  uint32 mode = 0;
  haikuTextApplyFormatTagFont(bfont, tag, &mode);

  bool color_changed = false;
  char* fg = iupAttribGet(tag, "FGCOLOR");
  if (fg)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(fg, &r, &g, &b))
    {
      color = (rgb_color){ r, g, b, 255 };
      color_changed = true;
    }
  }

  /* Record LINK range + URL; click hit-test is in IupHaikuTextView::MouseDown. */
  char* link_url = iupAttribGet(tag, "LINK");
  if (link_url && start != end)
  {
    int idx = iupAttribGetInt(ih, "_IUPHAIKU_LINK_COUNT");
    char key[64];
    snprintf(key, sizeof(key), "_IUPHAIKU_LINK_URL_%d", idx);
    iupAttribSetStr(ih, key, link_url);
    snprintf(key, sizeof(key), "_IUPHAIKU_LINK_RANGE_%d", idx);
    iupAttribSetStrf(ih, key, "%d:%d", (int)start, (int)end);
    iupAttribSetInt(ih, "_IUPHAIKU_LINK_COUNT", idx + 1);

    uint16 face = bfont.Face();
    face |= B_UNDERSCORE_FACE;
    bfont.SetFace(face);
    mode |= B_FONT_FACE;
    color = (rgb_color){ 0, 0, 255, 255 };
    color_changed = true;
  }

  if (mode == 0) mode = B_FONT_ALL;
  tv->SetFontAndColor(start, end, &bfont, mode, color_changed ? &color : NULL);

  /* Per-range BGCOLOR: overlay rect painted in IupHaikuTextView::Draw. */
  char* bg = iupAttribGet(tag, "BGCOLOR");
  if (bg)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(bg, &r, &g, &b))
    {
      rgb_color bgcol = (rgb_color){ r, g, b, 255 };
      ((IupHaikuTextView*)tv)->AddBgRange(start, end, bgcol);
    }
  }
}

extern "C" IUP_SDK_API void* iupdrvTextAddFormatTagStartBulk(Ihandle* /*ih*/) { return NULL; }
extern "C" IUP_SDK_API void  iupdrvTextAddFormatTagStopBulk (Ihandle* /*ih*/, void* /*state*/) {}

static int haikuTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline) return 0;
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return 0;

  LooperLockGuard guard(haikuTextGetLooper(ih));
  int32 s = 0, e = tv->TextLength();
  if (!iupStrEqualNoCase(value, "ALL"))
    tv->GetSelection(&s, &e);

  /* Reset to system default font + view's standard text color. */
  BFont base_font(be_plain_font);
  rgb_color base_col = ui_color(B_DOCUMENT_TEXT_COLOR);
  tv->SetFontAndColor(s, e, &base_font, B_FONT_ALL, &base_col);
  ((IupHaikuTextView*)tv)->ClearBgRanges(s, e);
  ((IupHaikuTextView*)tv)->ClearImageRanges(s, e);
  return 0;
}

static int haikuTextMapMethod(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupHaikuTextView* tv = new IupHaikuTextView(ih);
    bool has_h = (ih->data->sb & IUP_SB_HORIZ) != 0;
    bool has_v = (ih->data->sb & IUP_SB_VERT) != 0;
    BScrollView* sv = new BScrollView("iup_text_scroll", tv, B_FOLLOW_NONE,
                                      B_WILL_DRAW | B_FRAME_EVENTS, has_h, has_v);
    ih->handle = (InativeHandle*)sv;
    iupAttribSet(ih, "_IUPHAIKU_TEXT_INNER", (char*)tv);

    iuphaikuAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "WORDWRAP"))    tv->SetWordWrap(false);
    if (iupAttribGetBoolean(ih, "OVERWRITE"))    tv->SetOverwrite(true);
    if (ih->data->has_formatting)                tv->SetStylable(true);
    char* value = iupAttribGet(ih, "VALUE");
    if (value) { tv->SetSuppress(true); tv->SetText(value); tv->SetSuppress(false); }
    if (iupAttribGetBoolean(ih, "READONLY"))     tv->MakeEditable(false);
    char* align = iupAttribGet(ih, "ALIGNMENT");
    if (align) haikuTextSetAlignmentAttrib(ih, align);
  }
  else
  {
    IupHaikuTextControl* tc = new IupHaikuTextControl(ih);
    bool spin = iupAttribGetBoolean(ih, "SPIN");
    if (spin)
    {
      IupHaikuSpinWrap* sw = new IupHaikuSpinWrap(ih, tc);
      sw->SetMin(iupAttribGetInt(ih, "SPINMIN"));
      sw->SetMax(iupAttribGet(ih, "SPINMAX") ? iupAttribGetInt(ih, "SPINMAX") : 100);
      sw->SetStep(iupAttribGet(ih, "SPININC") ? iupAttribGetInt(ih, "SPININC") : 1);
      sw->SetWrap(iupAttribGetBoolean(ih, "SPINWRAP"));
      ih->handle = (InativeHandle*)sw;
    }
    else
    {
      ih->handle = (InativeHandle*)tc;
    }
    iupAttribSet(ih, "_IUPHAIKU_TEXT_INNER", (char*)tc->TextView());

    iuphaikuAddToParent(ih);

    char* value = iupAttribGet(ih, "VALUE");
    if (value)
    {
      tc->MuteBegin();
      tc->SetText(value);
      tc->MuteEnd();
    }
    if (ih->data->nc > 0) tc->TextView()->SetMaxBytes(ih->data->nc);
    if (iupAttribGetBoolean(ih, "READONLY")) tc->TextView()->MakeEditable(false);
    char* align = iupAttribGet(ih, "ALIGNMENT");
    if (align) haikuTextSetAlignmentAttrib(ih, align);
  }

  iuphaikuUpdateWidgetFont(ih, (BView*)ih->handle);

  if (ih->data->is_multiline && ih->data->formattags)
    iupTextUpdateFormatTags(ih);

  return IUP_NOERROR;
}

static void haikuTextUnMapMethod(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupHaikuTextView* tv = (IupHaikuTextView*)iupAttribGet(ih, "_IUPHAIKU_TEXT_INNER");
    if (tv) tv->SetIhandle(NULL);
  }
  else
  {
    if (IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih))
    {
      sw->SetIhandle(NULL);
    }
    else
    {
      IupHaikuTextControl* tc = (IupHaikuTextControl*)ih->handle;
      if (tc) tc->SetIhandle(NULL);
    }
  }
  iupAttribSet(ih, "_IUPHAIKU_TEXT_INNER", NULL);
  iupdrvBaseUnMapMethod(ih);
}

/* BTextControl chrome is unconditional, so AddBorders and AddExtraPadding agree. */
static void haikuTextProbeChrome(int* w, int* h)
{
  static int probe_w = -1, probe_h = -1;
  if (probe_w < 0)
  {
    BTextControl probe(BRect(0, 0, 0, 0), "iup_probe", NULL, NULL, NULL);
    float pw = 0, ph = 0;
    probe.GetPreferredSize(&pw, &ph);
    font_height fh;
    probe.GetFontHeight(&fh);
    int charh = (int)ceilf(fh.ascent + fh.descent + fh.leading);
    if (charh < 1) charh = 1;
    probe_h = (int)ceilf(ph) - charh;
    if (probe_h < 10) probe_h = 10;
    probe_w = 8;
  }
  if (w) *w += probe_w;
  if (h) *h += probe_h;
}

extern "C" IUP_SDK_API void iupdrvTextAddBorders(Ihandle* ih, int *w, int *h)
{
  if (ih && ih->data && ih->data->is_multiline)
  {
    if (w) *w += 6;
    if (h) *h += 6;
    return;
  }
  haikuTextProbeChrome(w, h);
}

extern "C" IUP_SDK_API void iupdrvTextAddExtraPadding(Ihandle* ih, int* w, int* h)
{
  /* IUP core calls this on BORDER=NO; BTextControl still has chrome on Haiku. */
  if (ih && ih->data && ih->data->is_multiline) return;
  haikuTextProbeChrome(w, h);
}
extern "C" IUP_SDK_API void iupdrvTextAddSpin(Ihandle* /*ih*/, int* w, int /*h*/)
{
  if (w) *w += 24;
}

static char* haikuTextGetLineValueAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  if (!ih->data->is_multiline)
  {
    BTextControl* tc = haikuTextGetTextControl(ih);
    return tc ? iupStrReturnStr(tc->Text()) : NULL;
  }
  int32 s = 0, e = 0;
  tv->GetSelection(&s, &e);
  int32 ln = tv->LineAt(s);
  int32 line_start = tv->OffsetAt(ln);
  int32 line_end   = tv->OffsetAt(ln + 1);
  int32 len = line_end - line_start;
  /* Trim trailing newline from intermediate lines. */
  if (len > 0)
  {
    char last = tv->ByteAt(line_end - 1);
    if (last == '\n' || last == '\r') len--;
  }
  char* buf = iupStrGetMemory(len + 1);
  tv->GetText(line_start, len, buf);
  buf[len] = '\0';
  return buf;
}

static char* haikuTextGetCountAttrib(Ihandle* ih)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  return iupStrReturnInt((int)tv->TextLength());
}

static char* haikuTextGetLineCountAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline) return iupStrReturnInt(1);
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) return NULL;
  return iupStrReturnInt(tv->CountLines());
}

static int haikuTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle && ih->data->is_multiline)
  {
    BTextView* tv = haikuTextGetEditor(ih);
    if (tv)
    {
      LooperLockGuard guard(haikuTextGetLooper(ih));
      float pad_x = (float)(ih->data->horiz_padding + 2);
      float pad_y = (float)(ih->data->vert_padding + 2);
      tv->SetInsets(pad_x, pad_y, pad_x, pad_y);
    }
  }
  return 0;
}

static int haikuTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  if (IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih))
  {
    int n = 0;
    if (iupStrToInt(value, &n)) sw->SetMin(n);
  }
  return 1;
}

static int haikuTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  if (IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih))
  {
    int n = 0;
    if (iupStrToInt(value, &n)) sw->SetMax(n);
  }
  return 1;
}

static int haikuTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  if (IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih))
  {
    int n = 0;
    if (iupStrToInt(value, &n)) sw->SetStep(n);
  }
  return 1;
}

static int haikuTextSetSpinWrapAttrib(Ihandle* ih, const char* value)
{
  if (IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih))
    sw->SetWrap(iupStrBoolean(value) != 0);
  return 1;
}

static int haikuTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih);
  if (!sw || !value) return 1;
  int n = 0;
  if (!iupStrToInt(value, &n)) return 1;
  sw->SetSpinValue(n);
  if (!sw->IsSpinAuto()) return 1;  /* SPINAUTO=NO: caller manages text via SPIN_CB */
  IupHaikuTextControl* tc = sw->TextControl();
  if (!tc) return 1;
  LooperLockGuard guard(tc->Looper());
  tc->MuteBegin();
  tc->SetText(value);
  tc->MuteEnd();
  return 1;
}

static char* haikuTextGetSpinValueAttrib(Ihandle* ih)
{
  IupHaikuSpinWrap* sw = haikuTextGetSpinWrap(ih);
  if (sw && !sw->IsSpinAuto()) return iupStrReturnInt(sw->Value());
  BTextControl* tc = haikuTextGetTextControl(ih);
  return tc ? iupStrReturnStr(tc->Text()) : NULL;
}

extern "C" IUP_SDK_API void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) { if (pos) *pos = 0; return; }
  LooperLockGuard guard(haikuTextGetLooper(ih));

  int32 line_start = tv->OffsetAt(lin - 1);
  int32 line_end   = tv->OffsetAt(lin);
  int32 line_len   = line_end - line_start;
  if (col - 1 > line_len) col = line_len + 1;
  if (pos) *pos = line_start + col - 1;
}

extern "C" IUP_SDK_API void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  BTextView* tv = haikuTextGetEditor(ih);
  if (!tv) { if (lin) *lin = 1; if (col) *col = 1; return; }
  LooperLockGuard guard(haikuTextGetLooper(ih));
  int32 ln = tv->LineAt(pos);
  int32 ls = tv->OffsetAt(ln);
  if (lin) *lin = ln + 1;
  if (col) *col = pos - ls + 1;
}

extern "C" IUP_SDK_API void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = haikuTextMapMethod;
  ic->UnMap = haikuTextUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuTextSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", haikuTextGetValueAttrib, haikuTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", haikuTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", haikuTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", haikuTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", haikuTextGetReadOnlyAttrib, haikuTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, haikuTextSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, haikuTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "SELECTION", haikuTextGetSelectionAttrib, haikuTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", haikuTextGetSelectionPosAttrib, haikuTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", haikuTextGetSelectedTextAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CARET", haikuTextGetCaretAttrib, haikuTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", haikuTextGetCaretPosAttrib, haikuTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "APPEND", NULL, haikuTextSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, haikuTextSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, haikuTextSetClipboardAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, haikuTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, haikuTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, haikuTextSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, haikuTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, haikuTextSetAlignmentAttrib, "ALEFT", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", NULL, haikuTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PASSWORD", NULL, haikuTextSetPasswordAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, haikuTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, haikuTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SPIN", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, haikuTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, haikuTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, haikuTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINWRAP", NULL, haikuTextSetSpinWrapAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", haikuTextGetSpinValueAttrib, haikuTextSetSpinValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}

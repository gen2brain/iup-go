/** \file
 * \brief Haiku Table (BColumnListView)
 *
 * BColumnListView is in private/interface/ColumnListView.h, linked via
 * libcolumnlistview.a. Same library that backs HaikuDepot's package list.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

#include <Bitmap.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ControlLook.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <String.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_table.h"
#include "iup_key.h"
}

#include "iuphaiku_drv.h"


class IupHaikuTableField : public BStringField
{
public:
  IupHaikuTableField(const char* str, BBitmap* icon = NULL)
    : BStringField(str ? str : ""),
      fIcon(icon),
      fFg{0, 0, 0, 0}, fBg{0, 0, 0, 0}, fHasFg(false),
      fHasBg(false), fHasFont(false), fFont(*be_plain_font),
      fAlignment(B_ALIGN_LEFT), fRow(NULL)
  {}

  BBitmap* Icon() const { return fIcon; }
  void SetIcon(BBitmap* b) { fIcon = b; }

  BRow* Row() const { return fRow; }
  void SetRow(BRow* r) { fRow = r; }

private:
  BBitmap* fIcon;
  rgb_color fFg, fBg;
  bool fHasFg, fHasBg, fHasFont;
  BFont fFont;
  alignment fAlignment;
  BRow* fRow;
};


static BBitmap* haikuTableBitmapByName(Ihandle* ih, const char* name)
{
  return (name && *name) ? (BBitmap*)iupImageGetImage(name, ih, 0, NULL) : NULL;
}

/* Look up an id2 attribute with priority: per-cell (L:C) > per-column (0:C) > per-row (L:0). */
static const char* haikuTableLookupId2(Ihandle* ih, const char* name, int lin, int col)
{
  const char* v = iupAttribGetId2(ih, name, lin, col);
  if (!v) v = iupAttribGetId2(ih, name, 0, col);
  if (!v) v = iupAttribGetId2(ih, name, lin, 0);
  return v;
}

static float haikuTableRowHeight(BColumnListView* tv)
{
  BFont font;
  if (tv) tv->GetFont(B_FONT_ROW, &font);
  else font = *be_plain_font;
  font_height fh;
  font.GetHeight(&fh);
  float h = ceilf(fh.ascent + fh.descent + fh.leading + 6);
  return h < 18.0f ? 18.0f : h;
}

/* No-op operator delete keeps the shared sentinel alive through ~BRow's `delete field` loop. */
class IupHaikuVirtualField : public BField
{
public:
  void operator delete(void*) {}
  void operator delete(void*, std::size_t) {}
};

static IupHaikuVirtualField g_virtual_field;

class IupHaikuTableView;
static void haikuTableAutoSizeColumns(Ihandle* ih, IupHaikuTableView* tv);
static void haikuTableCheckColumnMove(Ihandle* ih, IupHaikuTableView* tv);
static void haikuTableUserSort(Ihandle* ih, IupHaikuTableView* tv, int col);

#define IUPHAIKU_TABLE_AUTOSIZE 'IupA'
#define IUPHAIKU_TABLE_COLMOVE 'IupM'
#define IUPHAIKU_TABLE_SCROLL 'IupS'
#define IUPHAIKU_TABLE_SORTCLICK 'IupO'

class IupHaikuTableEditor : public BTextView
{
public:
  IupHaikuTableEditor(BRect r, IupHaikuTableView* tv, int lin, int col, const char* val);
  void KeyDown(const char* bytes, int32 numBytes) override;
  void MakeFocus(bool focused = true) override;

  IupHaikuTableView* fTv;
  int fLin, fCol;
  bool fEnded;

protected:
  void InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs) override;
  void DeleteText(int32 fromOffset, int32 toOffset) override;
};


class IupHaikuTableView : public BColumnListView
{
public:
  explicit IupHaikuTableView(Ihandle* ih)
    : BColumnListView(BRect(0, 0, 0, 0), "iup_table",
                      B_FOLLOW_NONE, B_SUPPORTS_LAYOUT | B_DRAW_ON_CHILDREN | B_NAVIGABLE,
                      B_FANCY_BORDER, true),
      fIhandle(ih)
  {
    SetColor(B_COLOR_ROW_DIVIDER, GridLineColor());
    /* Opaque bg under the frame; CLV's _Init leaves the view transparent. */
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  Ihandle* GetIhandle() const { return fIhandle; }
  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

  bool IsVirtual() const { return fIsVirtual; }
  void SetVirtual(bool v) { fIsVirtual = v; }

  static rgb_color GridLineColor()
  {
    return tint_color(iuphaikuColor(B_LIST_BACKGROUND_COLOR), B_DARKEN_1_TINT);
  }

  void ApplyColors()
  {
    SetColor(B_COLOR_BACKGROUND, iuphaikuColor(B_LIST_BACKGROUND_COLOR));
    SetColor(B_COLOR_TEXT, iuphaikuColor(B_LIST_ITEM_TEXT_COLOR));
    SetColor(B_COLOR_ROW_DIVIDER, GridLineColor());
    SetColor(B_COLOR_SELECTION, iuphaikuColor(B_LIST_SELECTED_BACKGROUND_COLOR));
    SetColor(B_COLOR_SELECTION_TEXT, iuphaikuColor(B_LIST_SELECTED_ITEM_TEXT_COLOR));
    SetColor(B_COLOR_NON_FOCUS_SELECTION, iuphaikuColor(B_LIST_SELECTED_BACKGROUND_COLOR));
    SetColor(B_COLOR_HEADER_BACKGROUND, iuphaikuColor(B_CONTROL_BACKGROUND_COLOR));
    SetColor(B_COLOR_HEADER_TEXT, iuphaikuColor(B_PANEL_TEXT_COLOR));
    SetViewColor(iuphaikuColor(B_PANEL_BACKGROUND_COLOR));
    SetLowColor(iuphaikuColor(B_PANEL_BACKGROUND_COLOR));
    Invalidate();
  }

  void SetSortFilter(BMessageFilter* f) { fSortFilter = f; }
  BMessageFilter* SortFilter() const { return fSortFilter; }

  int FocusCol() const { return fFocusCol; }
  void SetFocusCol(int c) { if (c > 0) fFocusCol = c; }

  IupHaikuTableEditor* Editor() const { return fEditor; }
  bool IsCellEditable(int lin, int col)
  {
    if (!fIhandle || lin < 1 || col < 1) return false;
    char name[32];
    snprintf(name, sizeof(name), "EDITABLE%d", col);
    const char* v = iupAttribGet(fIhandle, name);
    if (!v) v = iupAttribGet(fIhandle, "EDITABLE");
    return v && iupStrBoolean(v);
  }
  void StartEdit(int lin, int col);
  void EndEdit(bool apply);

  void ClearScrollBarFocusHighlight()
  {
    /* CLV highlights scrollbar borders on focus; the focused-cell rect is enough. */
    if (BScrollBar* sb = dynamic_cast<BScrollBar*>(FindView("vertical_scroll_bar")))   sb->SetBorderHighlighted(false);
    if (BScrollBar* sb = dynamic_cast<BScrollBar*>(FindView("horizontal_scroll_bar"))) sb->SetBorderHighlighted(false);
  }

  void MakeFocus(bool state = true) override
  {
    BColumnListView::MakeFocus(state);
    ClearScrollBarFocusHighlight();
    if (fTrail && !fTrail->IsHidden(fTrail)) fTrail->Invalidate();
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, state ? 1 : 0);
  }

  void SelectionChanged() override
  {
    BColumnListView::SelectionChanged();
    if (fTrail && !fTrail->IsHidden(fTrail)) fTrail->Invalidate();
    if (!fIhandle) return;
    if (iupAttribGet(fIhandle, "_IUPTABLE_IGNORE_SELECTION_CB")) return;

    iupTableCallMultiSelectionCb(fIhandle);

    BRow* row = CurrentSelection();
    if (!row) return;
    int lin = IndexOf(row);
    if (lin < 0) return;

    IFnii cb = (IFnii)IupGetCallback(fIhandle, "ENTERITEM_CB");
    if (cb) cb(fIhandle, lin + 1, fFocusCol);
  }

protected:
  void WindowActivated(bool active) override
  {
    BColumnListView::WindowActivated(active);
    ClearScrollBarFocusHighlight();
    if (fTrail && !fTrail->IsHidden(fTrail)) fTrail->Invalidate();
  }

  /* Mirror CLV::Draw but never set B_FOCUSED so the frame stays neutral. */
  void Draw(BRect updateRect) override
  {
    if (!be_control_look) return;
    BRect rect = Bounds();
    rgb_color base = iuphaikuColor(B_PANEL_BACKGROUND_COLOR);

    BScrollBar* vsb = dynamic_cast<BScrollBar*>(FindView("vertical_scroll_bar"));
    BScrollBar* hsb = dynamic_cast<BScrollBar*>(FindView("horizontal_scroll_bar"));
    BRect vsbFrame, hsbFrame;
    if (vsb && !vsb->IsHidden(vsb)) vsbFrame = vsb->Frame();
    if (hsb && !hsb->IsHidden(hsb)) hsbFrame = hsb->Frame();

    be_control_look->DrawScrollViewFrame(this, rect, updateRect,
      vsbFrame, hsbFrame, base, B_FANCY_BORDER, 0);
  }

  void KeyDown(const char* bytes, int32 numBytes) override
  {
    if (fIhandle && !iupdrvIsActive(fIhandle)) return;
    if (!fIhandle || numBytes < 1) { BColumnListView::KeyDown(bytes, numBytes); return; }

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : NULL;
    int32 raw_char = 0, mods = 0, raw_key = 0;
    if (msg) { msg->FindInt32("raw_char", &raw_char); msg->FindInt32("key", &raw_key); msg->FindInt32("modifiers", &mods); }

    int code = iuphaikuKeyDecode((int)(unsigned char)bytes[0], (int)raw_char, (int)raw_key, (unsigned)mods);
    if (code)
    {
      int ret = (mods & B_COMMAND_KEY) ? IUP_DEFAULT : iupKeyCallKeyCb(fIhandle, code);
      if (ret == IUP_CLOSE) IupExitLoop();
      if (ret == IUP_IGNORE) return;
    }

    if (bytes[0] == B_LEFT_ARROW || bytes[0] == B_RIGHT_ARROW)
    {
      int nc = (int)CountColumns();
      int delta = (bytes[0] == B_LEFT_ARROW) ? -1 : 1;
      int new_col = fFocusCol + delta;
      if (new_col >= 1 && new_col <= nc)
      {
        fFocusCol = new_col;
        BRow* r = FocusRow();
        if (r)
        {
          IFnii cb = (IFnii)IupGetCallback(fIhandle, "ENTERITEM_CB");
          if (cb) cb(fIhandle, (int)IndexOf(r) + 1, fFocusCol);
        }
        if (BView* o = ScrollView()) o->Invalidate();
      }
      return;
    }

    bool is_f2 = (bytes[0] == B_FUNCTION_KEY && raw_char == B_F2_KEY);
    if (bytes[0] == B_RETURN || is_f2)
    {
      BRow* r = FocusRow();
      if (r) StartEdit((int)IndexOf(r) + 1, fFocusCol);
      return;
    }

    BColumnListView::KeyDown(bytes, numBytes);
  }

  /* BWindow dispatches Cmd+C/V/X as B_COPY/B_PASTE/B_CUT to the focused view. */
  void MessageReceived(BMessage* msg) override
  {
    if (!fIhandle) { BColumnListView::MessageReceived(msg); return; }

    if (msg->what == IUPHAIKU_TABLE_SCROLL)
    {
      if (fPendingScrollLin > 0)
        ScrollToLine(fPendingScrollLin);
      return;
    }

    if (msg->what == IUPHAIKU_TABLE_COLMOVE)
    {
      haikuTableCheckColumnMove(fIhandle, this);
      return;
    }

    if (msg->what == IUPHAIKU_TABLE_SORTCLICK)
    {
      haikuTableUserSort(fIhandle, this, msg->GetInt32("col", 0));
      return;
    }

    if (msg->what == IUPHAIKU_TABLE_AUTOSIZE)
    {
      delete fAutoSizeRunner;
      fAutoSizeRunner = NULL;
      haikuTableAutoSizeColumns(fIhandle, this);
      StretchLastColumn();
      SetMeasuredWithRows();
      Invalidate();
      return;
    }

    BRow* r = FocusRow();
    int lin = r ? (int)IndexOf(r) + 1 : 0;

    switch (msg->what)
    {
      case B_COPY:
        if (lin > 0)
        {
          char* val = iupdrvTableGetCellValue(fIhandle, lin, fFocusCol);
          if (val) IupSetGlobal("CLIPBOARD", val);
        }
        return;

      case B_CUT:
        if (lin > 0 && IsCellEditable(lin, fFocusCol))
        {
          char* val = iupdrvTableGetCellValue(fIhandle, lin, fFocusCol);
          if (val) IupSetGlobal("CLIPBOARD", val);
          char* old = iupdrvTableGetCellValue(fIhandle, lin, fFocusCol);
          BString old_buf = old ? old : "";
          iupdrvTableSetCellValue(fIhandle, lin, fFocusCol, "");
          if (!old_buf.IsEmpty())
          {
            IFnii vc = (IFnii)IupGetCallback(fIhandle, "VALUECHANGED_CB");
            if (vc) vc(fIhandle, lin, fFocusCol);
          }
        }
        return;

      case B_PASTE:
        if (lin > 0 && IsCellEditable(lin, fFocusCol))
        {
          const char* val = IupGetGlobal("CLIPBOARD");
          if (val)
          {
            char* old = iupdrvTableGetCellValue(fIhandle, lin, fFocusCol);
            BString old_buf = old ? old : "";
            iupdrvTableSetCellValue(fIhandle, lin, fFocusCol, val);
            if (old_buf != val)
            {
              IFnii vc = (IFnii)IupGetCallback(fIhandle, "VALUECHANGED_CB");
              if (vc) vc(fIhandle, lin, fFocusCol);
            }
          }
        }
        return;
    }
    BColumnListView::MessageReceived(msg);
  }

  /* Re-extend the outline view at the end of every layout pass when a scrollbar is hidden. */
  void DoLayout() override
  {
    BColumnListView::DoLayout();
    UpdateScrollBarVisibility();
  }

public:
  void MouseMoved(BPoint /*where*/, uint32 transit, const BMessage* /*drag*/) override
  {
    if (!fIhandle) return;
    if (transit == B_ENTERED_VIEW)
    {
      IFn cb = (IFn)IupGetCallback(fIhandle, "ENTERWINDOW_CB");
      if (cb) cb(fIhandle);
    }
    else if (transit == B_EXITED_VIEW)
    {
      IFn cb = (IFn)IupGetCallback(fIhandle, "LEAVEWINDOW_CB");
      if (cb) cb(fIhandle);
    }
  }

  void ItemInvoked() override
  {
    BRow* r = FocusRow();
    if (r) StartEdit((int)IndexOf(r) + 1, fFocusCol);
  }

  void SetDragSourceRow(int r) { fDragSourceRow = r; }
  int DragSourceRow() const { return fDragSourceRow; }
  void SetRowDragStartY(int y) { fDragStartY = y; }
  int RowDragStartY() const { return fDragStartY; }
  void SetRowDragging(bool d) { fRowDragging = d; }
  bool RowDragging() const { return fRowDragging; }
  void SetDragTargetRow(int t) { fDragTargetRow = t; }
  int DragTargetRow() const { return fDragTargetRow; }

  int InsertBeforeAtOutlineY(float oy)
  {
    int n = (int)CountRows(NULL);
    float y = 0.0f;
    for (int i = 0; i < n; i++)
    {
      const BRow* row = RowAt(i, NULL);
      if (!row) continue;
      float rh = row->Height();
      if (oy < y + rh / 2.0f) return i;
      y += rh + 1.0f;
    }
    return n;
  }

  void CommitRowMove(int source, int drop)
  {
    int is_ctrl = 0;
    if (source < 0 || drop < 0)
      return;
    if (iupTableCallDragDropCb(fIhandle, source, drop, &is_ctrl) != IUP_CONTINUE)
      return;

    int n = (int)CountRows(NULL);
    int target = (drop > source) ? drop - 1 : drop;
    if (target >= n) target = n - 1;
    if (target < 0) target = 0;
    if (target == source) return;

    BRow* row = RowAt(source, NULL);
    if (!row) return;
    RemoveRow(row);
    SuspendSortColumn();
    AddRow(row, target, NULL);
    RestoreSortColumn();
    iupTableMoveLinAttribs(fIhandle, source + 1, target + 1);
    SetFocusRow(row, true);
    Invalidate();
  }

  void FrameResized(float w, float h) override
  {
    BColumnListView::FrameResized(w, h);
    StretchLastColumn();
    UpdateScrollBarVisibility();
    RepositionTrail();
  }

  void DrawAfterChildren(BRect /*updateRect*/) override
  {
    if (!fIhandle) return;
    if (fPendingScrollLin > 0 && Looper())
      Looper()->PostMessage(IUPHAIKU_TABLE_SCROLL, this);
    BView* outline = ScrollView();
    if (!outline) return;

    BRect of = outline->Frame();
    float scroll_y = outline->Bounds().top;

    bool showgrid = iupAttribGetBoolean(fIhandle, "SHOWGRID");
    bool alt = iupAttribGetBoolean(fIhandle, "ALTERNATECOLOR");
    rgb_color divider = GridLineColor();
    rgb_color base = Color(B_COLOR_BACKGROUND);

    /* CLV advances fieldLeftEdge by Width()+1 per column, so include +1 per visible column. */
    float total_w = 15.0f;
    int nc = (int)CountColumns();
    for (int i = 0; i < nc; i++)
    {
      BColumn* c = ColumnAt(i);
      if (c && c->IsVisible()) total_w += c->Width() + 1.0f;
    }
    float trail_left = of.left + total_w;
    BRow* focus = FocusRow();
    if (!focus) focus = CurrentSelection();

    int n = (int)CountRows(NULL);
    float line = -scroll_y;
    for (int i = 0; i < n; i++)
    {
      const BRow* row = RowAt(i, NULL);
      if (!row) { line += 0; continue; }
      float rh = row->Height();
      float row_top = of.top + line;
      float row_bot = of.top + line + rh;
      if (row_top > of.bottom) break;

      if (trail_left < of.right && row_bot >= of.top && !row->IsSelected())
      {
        rgb_color bg = base;
        unsigned char r, g, b;
        /* STRETCHLAST: paint trail with last cell's bg so it merges with that column. */
        bool stretch_last = fIhandle->data->stretch_last;
        const char* cs = NULL;
        if (stretch_last)
          cs = haikuTableLookupId2(fIhandle, "BGCOLOR", i + 1, nc);
        if (!cs && alt)
          cs = ((i + 1) % 2 == 0) ? iupAttribGet(fIhandle, "EVENROWCOLOR")
                                  : iupAttribGet(fIhandle, "ODDROWCOLOR");
        if (cs && iupStrToRGB(cs, &r, &g, &b))
          { bg.red = r; bg.green = g; bg.blue = b; bg.alpha = 255; }
        SetLowColor(bg);
        FillRect(BRect(trail_left, row_top, of.right, row_bot), B_SOLID_LOW);
      }

      if (showgrid && row_bot >= of.top && row_bot <= of.bottom)
      {
        const BRow* next = (i + 1 < n) ? RowAt(i + 1, NULL) : NULL;
        if (!next || next != focus)
        {
          SetHighColor(divider);
          StrokeLine(BPoint(of.left, row_bot), BPoint(of.right, row_bot));
        }
      }
      line += rh + 1.0f;
    }

    /* Fill the trail below the last row; CLV leaves it as panel (gray) bg. */
    if (trail_left < of.right && of.top + line < of.bottom)
    {
      SetLowColor(base);
      FillRect(BRect(trail_left, of.top + line, of.right, of.bottom), B_SOLID_LOW);
    }

    if (fRowDragging && fDragTargetRow >= 0 &&
        fDragTargetRow != fDragSourceRow && fDragTargetRow != fDragSourceRow + 1)
    {
      float ly = of.top - scroll_y;
      for (int i = 0; i < n && i < fDragTargetRow; i++)
      {
        const BRow* r = RowAt(i, NULL);
        if (r) ly += r->Height() + 1.0f;
      }
      if (ly >= of.top && ly <= of.bottom)
      {
        rgb_color blue = { 0, 120, 215, 255 };
        SetHighColor(blue);
        FillRect(BRect(of.left, ly - 1, of.right, ly + 1));
      }
    }
  }

  void UpdateScrollBarVisibility()
  {
    BView* hsb = FindView("horizontal_scroll_bar");
    BView* vsb = FindView("vertical_scroll_bar");
    BView* outline = ScrollView();
    if (!hsb || !vsb || !outline) return;

    float total_w = 23.0f;  /* CLV's _VirtualWidth: kLeftMargin + kRightMargin. */
    int n = (int)CountColumns();
    for (int i = 0; i < n; i++)
    {
      BColumn* c = ColumnAt(i);
      if (c && c->IsVisible()) total_w += c->Width();
    }
    float total_h = 0.0f;
    int nr = (int)CountRows(NULL);
    for (int i = 0; i < nr; i++)
    {
      const BRow* row = RowAt(i, NULL);
      if (row) total_h += row->Height() + 1.0f;
    }

    float hsb_h = be_control_look ? be_control_look->GetScrollBarWidth(B_HORIZONTAL) : 14.0f;
    float vsb_w = be_control_look ? be_control_look->GetScrollBarWidth(B_VERTICAL)   : 14.0f;

    /* IsHidden(self) ignores window visibility; only an explicit hide counts. */
    bool h_hidden = hsb->IsHidden(hsb);
    bool v_hidden = vsb->IsHidden(vsb);

    float avail_w = Bounds().Width() - 4.0f - (v_hidden ? 0.0f : vsb_w);
    float avail_h = Bounds().Height() - 4.0f - iupdrvTableGetHeaderHeight(fIhandle ? fIhandle : NULL)
                                              - (h_hidden ? 0.0f : hsb_h);

    bool need_h = total_w > avail_w;
    bool need_v = total_h > avail_h;

    AdjustScrollBar(hsb, outline, !need_h, h_hidden, /*horizontal*/true, hsb_h);
    AdjustScrollBar(vsb, outline, !need_v, v_hidden, /*horizontal*/false, vsb_w);

    bool h_now_hidden = hsb->IsHidden(hsb);
    bool v_now_hidden = vsb->IsHidden(vsb);
    float target_bottom = Bounds().Height() - 2.0f;
    float target_right  = Bounds().Width()  - 2.0f;

    /* CLV's frame paint leaves a stray patch in the corner a hidden scrollbar vacated. */
    if (h_now_hidden && !v_now_hidden)
    {
      BRect r = vsb->Frame();
      if (r.bottom < target_bottom)
        vsb->ResizeTo(r.Width(), r.Height() + (target_bottom - r.bottom));
    }
    if (v_now_hidden && !h_now_hidden)
    {
      BRect r = hsb->Frame();
      if (r.right < target_right)
        hsb->ResizeTo(r.Width() + (target_right - r.right), r.Height());
    }
  }

  void AdjustScrollBar(BView* sb, BView* outline, bool hide, bool currently_hidden,
                       bool horizontal, float thickness)
  {
    if (hide && !currently_hidden)
    {
      BRect sb_frame = sb->Frame();
      BRect r = outline->Frame();
      sb->Hide();
      if (horizontal) outline->ResizeTo(r.Width(), r.Height() + thickness);
      else            outline->ResizeTo(r.Width() + thickness, r.Height());
      Invalidate(sb_frame);
    }
    else if (!hide && currently_hidden)
    {
      BRect r = outline->Frame();
      sb->Show();
      if (horizontal) outline->ResizeTo(r.Width(), r.Height() - thickness);
      else            outline->ResizeTo(r.Width() - thickness, r.Height());
    }
    else if (hide && currently_hidden)
    {
      /* DoLayout reset outline assuming the scrollbar is showing; re-extend. */
      BRect r = outline->Frame();
      BRect b = Bounds();
      if (horizontal)
      {
        if (r.bottom < b.bottom - 2.0f)
          outline->ResizeTo(r.Width(), r.Height() + thickness);
      }
      else
      {
        if (r.right < b.right - 2.0f)
          outline->ResizeTo(r.Width() + thickness, r.Height());
      }
    }
  }

  void DrawLatch(BView* view, BRect frame, LatchType type, BRow* row) override
  {
    if (!view || !row)
    {
      BColumnListView::DrawLatch(view, frame, type, row);
      return;
    }
    if (!row->IsSelected())
    {
      rgb_color bg = Color(B_COLOR_BACKGROUND);
      if (fIhandle && iupAttribGetBoolean(fIhandle, "ALTERNATECOLOR"))
      {
        int lin = (int)IndexOf(row) + 1;
        const char* cs = (lin % 2 == 0) ? iupAttribGet(fIhandle, "EVENROWCOLOR")
                                        : iupAttribGet(fIhandle, "ODDROWCOLOR");
        unsigned char r, g, b;
        if (cs && iupStrToRGB(cs, &r, &g, &b)) { bg.red = r; bg.green = g; bg.blue = b; bg.alpha = 255; }
      }
      view->SetLowColor(bg);
      view->FillRect(frame, B_SOLID_LOW);
    }
    BColumnListView::DrawLatch(view, frame, type, row);

    if (fIhandle && iupAttribGetBoolean(fIhandle, "SHOWGRID"))
    {
      view->SetHighColor(GridLineColor());
      view->StrokeLine(BPoint(frame.right - 1, frame.top),
                       BPoint(frame.right - 1, frame.bottom));
      view->StrokeLine(BPoint(frame.left, frame.bottom),
                       BPoint(frame.right, frame.bottom));
    }
  }

  /* STRETCHLAST: last column consumes horizontal slack unless pinned by WIDTH/RASTERWIDTH. */
  void StretchLastColumn()
  {
    if (!fIhandle || !fIhandle->data->stretch_last) return;
    int n = (int)CountColumns();
    if (n == 0) return;
    char name[32];
    snprintf(name, sizeof(name), "WIDTH%d", n);
    if (iupAttribGet(fIhandle, name)) return;
    snprintf(name, sizeof(name), "RASTERWIDTH%d", n);
    if (iupAttribGet(fIhandle, name)) return;
    BColumn* last = ColumnAt(n - 1);
    if (!last) return;

    float sum = 15.0f;
    for (int i = 0; i < n - 1; i++)
    {
      BColumn* c = ColumnAt(i);
      if (c) sum += c->Width();
    }
    float vsb = be_control_look ? be_control_look->GetScrollBarWidth(B_VERTICAL) : 14.0f;
    /* border 4 + kRightMargin 8 keeps _VirtualWidth <= fVisibleRect.Width(). */
    float avail = Bounds().Width() - sum - vsb - 12.0f;
    if (avail < last->MinWidth()) avail = last->MinWidth();
    if (avail != last->Width()) { last->SetWidth(avail); Invalidate(); }
    RepositionTrail();
  }

  void ScrollToLine(int lin)
  {
    BView* outline = ScrollView();
    BRow* r = RowAt(lin - 1, NULL);
    if (!r) return;
    if (!outline || !Window() || Window()->IsHidden())
    {
      fPendingScrollLin = lin;
      return;
    }
    fPendingScrollLin = 0;
    ScrollTo(r);
  }

  bool NeedsAutoSize() const { return fNeedsAutoSize; }
  void ClearNeedsAutoSize() { fNeedsAutoSize = false; }
  void ScheduleAutoSize()
  {
    if (fAutoSizeRunner) return;
    BMessage tick(IUPHAIKU_TABLE_AUTOSIZE);
    fAutoSizeRunner = new BMessageRunner(BMessenger(this), &tick, 50000, 1);
  }
  void CancelAutoSize() { delete fAutoSizeRunner; fAutoSizeRunner = NULL; }
  bool MeasuredWithRows() const { return fMeasuredWithRows; }
  void SetMeasuredWithRows() { fMeasuredWithRows = true; }

  void SetTrailView(BView* v) { fTrail = v; }
  void SuspendSortColumn() { ClearSortColumns(); }
  void RestoreSortColumn()
  {
    int col = iupAttribGetInt(fIhandle, "_IUPHAIKU_SORTCOL");
    if (col > 0 && ColumnAt(col - 1))
      SetSortColumn(ColumnAt(col - 1), false, iupAttribGetInt(fIhandle, "_IUPHAIKU_SORTASC") != 0);
  }
  int HeaderPressCol() const { return fHeaderPressCol; }
  BPoint HeaderPressPoint() const { return fHeaderPressPt; }
  void SetHeaderPress(int col, BPoint pt) { fHeaderPressCol = col; fHeaderPressPt = pt; }
  void RepositionTrail();

private:
  Ihandle* fIhandle;
  bool fIsVirtual = false;
  bool fNeedsAutoSize = true;
  bool fMeasuredWithRows = false;
  BMessageRunner* fAutoSizeRunner = NULL;
  int fFocusCol = 1;
  BMessageFilter* fSortFilter = NULL;
  int fPendingScrollLin = 0;
  IupHaikuTableEditor* fEditor = NULL;
  BView* fTrail = NULL;
  int fDragSourceRow = -1;
  int fDragTargetRow = -1;
  bool fRowDragging = false;
  int fDragStartY = 0;
  int fHeaderPressCol = 0;
  BPoint fHeaderPressPt;
};


class IupHaikuTrailDividers : public BView
{
public:
  explicit IupHaikuTrailDividers(IupHaikuTableView* tv)
    : BView(BRect(0, 0, 0, 0), "iup_trail", B_FOLLOW_NONE, B_WILL_DRAW),
      fTv(tv)
  {
    BView::SetViewColor(B_TRANSPARENT_COLOR);
  }

  void Draw(BRect updateRect) override
  {
    if (!fTv) return;
    Ihandle* ih = fTv->GetIhandle();
    if (!ih) return;

    float rh = haikuTableRowHeight(fTv);
    float pitch = rh + 1.0f;
    if (pitch <= 0.0f) return;

    bool showgrid = iupAttribGetBoolean(ih, "SHOWGRID");
    bool alt = iupAttribGetBoolean(ih, "ALTERNATECOLOR");
    rgb_color base = fTv->Color(B_COLOR_BACKGROUND);
    rgb_color div  = fTv->GridLineColor();
    rgb_color foc  = fTv->Color(B_COLOR_ROW_DIVIDER);
    bool tv_focused = fTv->IsFocus() && fTv->Window() && fTv->Window()->IsActive();

    BRow* focus = fTv->FocusRow();
    if (!focus) focus = fTv->CurrentSelection();

    int n = fTv->CountRows(NULL);
    int first = (int)(updateRect.top / pitch);
    int last  = (int)(updateRect.bottom / pitch) + 1;
    if (first < 0) first = 0;
    if (last > n) last = n;

    for (int i = first; i < last; i++)
    {
      const BRow* row = fTv->RowAt(i, NULL);
      if (!row) continue;
      float row_top = i * pitch;
      float row_bot = row_top + rh;

      rgb_color bg;
      if (row->IsSelected())
      {
        bool active = fTv->Window() && fTv->Window()->IsActive();
        bg = fTv->Color(active ? B_COLOR_SELECTION : B_COLOR_NON_FOCUS_SELECTION);
        if (i & 1) bg = tint_color(bg, bg.IsLight() ? 1.04f : 0.90f);
      }
      else
      {
        bg = base;
        if (alt)
        {
          const char* cs = ((i + 1) % 2 == 0) ? iupAttribGet(ih, "EVENROWCOLOR")
                                              : iupAttribGet(ih, "ODDROWCOLOR");
          unsigned char r, g, b;
          if (cs && iupStrToRGB(cs, &r, &g, &b))
            { bg.red = r; bg.green = g; bg.blue = b; bg.alpha = 255; }
        }
      }
      SetLowColor(bg);
      FillRect(BRect(updateRect.left, row_top, updateRect.right, row_bot), B_SOLID_LOW);

      if (focus == row && tv_focused)
      {
        SetHighColor(foc);
        StrokeLine(BPoint(updateRect.left, row_top), BPoint(updateRect.right, row_top));
        StrokeLine(BPoint(updateRect.left, row_bot), BPoint(updateRect.right, row_bot));
      }
      else if (showgrid)
      {
        SetHighColor(div);
        StrokeLine(BPoint(updateRect.left, row_bot), BPoint(updateRect.right, row_bot));
      }
    }
  }

private:
  IupHaikuTableView* fTv;
};


void IupHaikuTableView::RepositionTrail()
{
  if (!fTrail) return;
  BView* outline = ScrollView();
  if (!outline) return;
  float total_w = 15.0f;
  int n = CountColumns();
  for (int i = 0; i < n; i++)
  {
    BColumn* c = ColumnAt(i);
    if (c && c->IsVisible()) total_w += c->Width() + 1.0f;
  }
  float right = outline->Bounds().right;
  bool stretch_last = fIhandle && fIhandle->data->stretch_last;
  bool active = !stretch_last && total_w < right;
  if (!active) { if (!fTrail->IsHidden(fTrail)) fTrail->Hide(); return; }
  if (fTrail->IsHidden(fTrail)) fTrail->Show();
  BRect cur = fTrail->Frame();
  if (cur.left != total_w) fTrail->MoveTo(total_w, 0);
  if (cur.Width() != right - total_w) fTrail->ResizeTo(right - total_w, 1e9f);
}


IupHaikuTableEditor::IupHaikuTableEditor(BRect r, IupHaikuTableView* tv, int lin, int col, const char* val)
  : BTextView(r, "iup_cell_edit", BRect(2.0f, 2.0f, r.Width() - 2.0f, r.Height() - 2.0f),
              B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE),
    fTv(tv), fLin(lin), fCol(col), fEnded(false)
{
  SetText(val ? val : "");
  SetWordWrap(false);
  SelectAll();
}

void IupHaikuTableEditor::KeyDown(const char* bytes, int32 numBytes)
{
  if (!fEnded && numBytes >= 1)
  {
    if (bytes[0] == B_ESCAPE) { if (fTv) fTv->EndEdit(false); return; }
    if (bytes[0] == B_RETURN) { if (fTv) fTv->EndEdit(true);  return; }
  }
  BTextView::KeyDown(bytes, numBytes);
}

void IupHaikuTableEditor::MakeFocus(bool focused)
{
  BTextView::MakeFocus(focused);
  if (!focused && !fEnded && fTv) fTv->EndEdit(true);
}

void IupHaikuTableEditor::InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs)
{
  BTextView::InsertText(text, length, offset, runs);
  if (!fEnded && fTv && fTv->GetIhandle())
  {
    IFniis cb = (IFniis)IupGetCallback(fTv->GetIhandle(), "EDITION_CB");
    if (cb) cb(fTv->GetIhandle(), fLin, fCol, (char*)Text());
  }
}

void IupHaikuTableEditor::DeleteText(int32 fromOffset, int32 toOffset)
{
  BTextView::DeleteText(fromOffset, toOffset);
  if (!fEnded && fTv && fTv->GetIhandle())
  {
    IFniis cb = (IFniis)IupGetCallback(fTv->GetIhandle(), "EDITION_CB");
    if (cb) cb(fTv->GetIhandle(), fLin, fCol, (char*)Text());
  }
}

void IupHaikuTableView::StartEdit(int lin, int col)
{
  if (!fIhandle || !IsCellEditable(lin, col)) return;
  if (fEditor) EndEdit(true);

  IFnii beg = (IFnii)IupGetCallback(fIhandle, "EDITBEGIN_CB");
  if (beg && beg(fIhandle, lin, col) == IUP_IGNORE) return;

  BRow* row = RowAt(lin - 1, NULL);
  BColumn* column = ColumnAt(col - 1);
  if (!row || !column) return;

  BRect cell_rect = GetFieldRect(row, column);
  BView* outline = ScrollView();
  if (!outline) return;

  const char* value = NULL;
  if (fIsVirtual)
  {
    sIFnii vcb = (sIFnii)IupGetCallback(fIhandle, "VALUE_CB");
    if (vcb) value = vcb(fIhandle, lin, col);
  }
  else
  {
    BField* f = row->GetField(col - 1);
    IupHaikuTableField* tf = dynamic_cast<IupHaikuTableField*>(f);
    if (tf) value = tf->String();
  }

  fEditor = new IupHaikuTableEditor(cell_rect, this, lin, col, value);
  outline->AddChild(fEditor);
  fEditor->MakeFocus(true);
}

void IupHaikuTableView::EndEdit(bool apply)
{
  if (!fEditor || fEditor->fEnded) return;
  fEditor->fEnded = true;

  Ihandle* ih = fIhandle;
  int lin = fEditor->fLin;
  int col = fEditor->fCol;
  BString text(fEditor->Text());

  IFniisi end = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  int veto = (end && apply) ? end(ih, lin, col, (char*)text.String(), 1) : IUP_DEFAULT;
  if (!apply && end) end(ih, lin, col, (char*)text.String(), 0);
  /* Callback may IupDestroy the table, freeing `this`. */
  if (!iupObjectCheck(ih)) return;

  bool save = apply && veto != IUP_IGNORE;
  if (save)
  {
    char* old = iupdrvTableGetCellValue(ih, lin, col);
    BString old_buf = old ? old : "";
    iupdrvTableSetCellValue(ih, lin, col, text.String());
    if (old_buf != text)
    {
      IFnii vc = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (vc) vc(ih, lin, col);
      if (!iupObjectCheck(ih)) return;
    }
  }

  fEditor->RemoveSelf();
  delete fEditor;
  fEditor = NULL;
  MakeFocus(true);
}

/* IUP CLICK_CB status string (10 chars, see iup.h iup_isshift/iscontrol/...). */
static void haikuTableFillStatus(const BMessage* msg, char status[11])
{
  for (int i = 0; i < 10; i++) status[i] = ' ';
  status[10] = '\0';
  if (!msg) return;
  int32 mods = 0, buttons = 0, clicks = 1;
  msg->FindInt32("modifiers", &mods);
  msg->FindInt32("buttons", &buttons);
  msg->FindInt32("clicks", &clicks);
  if (mods & B_SHIFT_KEY)               status[0] = 'S';
  if (mods & B_CONTROL_KEY)             status[1] = 'C';
  if (buttons & B_PRIMARY_MOUSE_BUTTON) status[2] = '1';
  if (buttons & B_TERTIARY_MOUSE_BUTTON)  status[3] = '2';
  if (buttons & B_SECONDARY_MOUSE_BUTTON) status[4] = '3';
  if (clicks >= 2)                      status[5] = 'D';
  if (mods & B_COMMAND_KEY)             status[6] = 'A';
}

/* MOUSE_MOVED keeps the trail overlay aligned while CLV does its offscreen column resize. */

class IupHaikuTableSortFilter : public BMessageFilter
{
public:
  explicit IupHaikuTableSortFilter(IupHaikuTableView* tv)
    : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE), fTv(tv) {}

  filter_result Filter(BMessage* msg, BHandler** target) override
  {
    if (!fTv || !target || !*target) return B_DISPATCH_MESSAGE;

    BView* tgt = dynamic_cast<BView*>(*target);
    if (!tgt) return B_DISPATCH_MESSAGE;

    bool inside = false;
    for (BView* p = tgt; p; p = p->Parent())
      if (p == fTv) { inside = true; break; }
    if (!inside) return B_DISPATCH_MESSAGE;

    if (msg->WasDropped() && fTv->GetIhandle())
    {
      if (iuphaikuHandleDropFiles(fTv->GetIhandle(), fTv, msg)) return B_SKIP_MESSAGE;
      if (iuphaikuDnDMessageReceived(fTv->GetIhandle(), fTv, msg)) return B_SKIP_MESSAGE;
    }

    if (msg->what != B_MOUSE_DOWN && msg->what != B_MOUSE_UP &&
        msg->what != B_MOUSE_MOVED) return B_DISPATCH_MESSAGE;

    if (!iupdrvIsActive(fTv->GetIhandle()))
      return B_SKIP_MESSAGE;

    if (msg->what == B_MOUSE_MOVED)
    {
      fTv->RepositionTrail();
      if (fTv->GetIhandle()->data->show_dragdrop && fTv->DragSourceRow() >= 0)
      {
        BPoint where;
        BView* outline = fTv->ScrollView();
        if (outline && msg->FindPoint("be:view_where", &where) == B_OK)
        {
          BPoint ov_pt = outline->ConvertFromScreen(tgt->ConvertToScreen(where));
          if (!fTv->RowDragging() && abs((int)ov_pt.y - fTv->RowDragStartY()) > 4)
            fTv->SetRowDragging(true);
          if (fTv->RowDragging())
          {
            int t = fTv->InsertBeforeAtOutlineY(ov_pt.y);
            if (t != fTv->DragTargetRow())
            {
              fTv->SetDragTargetRow(t);
              fTv->Invalidate();
            }
          }
        }
      }
      return B_DISPATCH_MESSAGE;
    }

    if (msg->what == B_MOUSE_UP)
    {
      if (fTv->HeaderPressCol() > 0)
      {
        BPoint up;
        int col = fTv->HeaderPressCol();
        BPoint press = fTv->HeaderPressPoint();
        fTv->SetHeaderPress(0, BPoint());
        if (msg->FindPoint("be:view_where", &up) == B_OK && fTv->Looper())
        {
          BPoint delta = tgt->ConvertToScreen(up) - press;
          if (fabsf(delta.x) <= 10.0f && fabsf(delta.y) <= 10.0f)
          {
            BMessage sort(IUPHAIKU_TABLE_SORTCLICK);
            sort.AddInt32("col", col);
            fTv->Looper()->PostMessage(&sort, fTv);
          }
        }
      }
      if (fTv->GetIhandle()->data->show_dragdrop && fTv->DragSourceRow() >= 0)
      {
        int source = fTv->DragSourceRow();
        int target = fTv->DragTargetRow();
        bool was_dragging = fTv->RowDragging();
        fTv->SetDragSourceRow(-1);
        fTv->SetRowDragging(false);
        fTv->SetDragTargetRow(-1);
        if (was_dragging && target >= 0)
          fTv->CommitRowMove(source, target);
      }
      if (fTv->GetIhandle()->data->allow_reorder && fTv->Looper())
        fTv->Looper()->PostMessage(IUPHAIKU_TABLE_COLMOVE, fTv);
      fTv->RepositionTrail();
      fTv->UpdateScrollBarVisibility();
      if (BView* outline = fTv->ScrollView()) outline->Invalidate();
      fTv->Invalidate();
      return B_DISPATCH_MESSAGE;
    }

    BPoint where;
    if (msg->FindPoint("be:view_where", &where) != B_OK) return B_DISPATCH_MESSAGE;
    BPoint clv_pt = fTv->ConvertFromScreen(tgt->ConvertToScreen(where));

    Ihandle* ih = fTv->GetIhandle();
    int hh = iupdrvTableGetHeaderHeight(ih);
    BColumn* col = fTv->ColumnAt(clv_pt);

    if (clv_pt.y >= 0 && clv_pt.y <= hh + 4)
    {
      int32 buttons = B_PRIMARY_MOUSE_BUTTON;
      msg->FindInt32("buttons", &buttons);
      if (msg->what == B_MOUSE_DOWN && col && ih->data->sortable && (buttons & B_PRIMARY_MOUSE_BUTTON) &&
          fTv->ColumnAt(clv_pt - BPoint(5, 0)) == col && fTv->ColumnAt(clv_pt + BPoint(5, 0)) == col)
        fTv->SetHeaderPress((int)col->LogicalFieldNum() + 1, tgt->ConvertToScreen(where));
      return B_DISPATCH_MESSAGE;
    }

    if (col) fTv->SetFocusCol(col->LogicalFieldNum() + 1);

    BView* outline = fTv->ScrollView();
    if (outline)
    {
      BPoint ov_pt = outline->ConvertFromScreen(tgt->ConvertToScreen(where));
      int nr = fTv->CountRows(NULL);
      float y = 0.0f;
      int hit_lin = 0;
      for (int i = 0; i < nr; i++)
      {
        const BRow* row = fTv->RowAt(i, NULL);
        if (!row) continue;
        float rh = row->Height();
        if (ov_pt.y >= y && ov_pt.y <= y + rh) { hit_lin = i + 1; break; }
        y += rh + 1.0f;
      }
      if (hit_lin > 0 && ih->data->show_dragdrop)
      {
        fTv->SetDragSourceRow(hit_lin - 1);
        fTv->SetRowDragStartY((int)ov_pt.y);
        fTv->SetRowDragging(false);
        fTv->SetDragTargetRow(-1);
      }

      if (hit_lin > 0 && col)
      {
        IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
        if (cb)
        {
          char status[11];
          haikuTableFillStatus(msg, status);
          cb(ih, hit_lin, col->LogicalFieldNum() + 1, status);
        }
      }
    }
    return B_DISPATCH_MESSAGE;
  }

private:
  IupHaikuTableView* fTv;
};


static rgb_color haikuTableDimColor(rgb_color c)
{
  rgb_color bg = iuphaikuColor(B_PANEL_BACKGROUND_COLOR);
  rgb_color d = { (uint8)((c.red + bg.red) / 2), (uint8)((c.green + bg.green) / 2), (uint8)((c.blue + bg.blue) / 2), 255 };
  return d;
}

class IupHaikuTableColumn : public BTitledColumn
{
public:
  IupHaikuTableColumn(Ihandle* ih, const char* title, float width, alignment align)
    : BTitledColumn(title ? title : "", width, 40.0f, 4096.0f, align), fIhandle(ih) {}

  void IconDrawSize(BBitmap* icon, float row_h, float* w, float* h) const
  {
    float img_w = icon->Bounds().Width() + 1.0f;
    float img_h = icon->Bounds().Height() + 1.0f;
    bool fit = !fIhandle || fIhandle->data->fit_image;
    if (fit && img_h > row_h - 2.0f && img_h > 0)
    {
      float s = (row_h - 2.0f) / img_h;
      *w = img_w * s;
      *h = img_h * s;
    }
    else { *w = img_w; *h = img_h; }
  }

  void DrawField(BField* field, BRect rect, BView* parent) override
  {
    /* Column resize draws via an offscreen BufferView whose Parent() is NULL. */
    IupHaikuTableView* tv = dynamic_cast<IupHaikuTableView*>(parent->Parent());
    if (!tv && fIhandle) tv = (IupHaikuTableView*)fIhandle->handle;

    IupHaikuTableField* f = dynamic_cast<IupHaikuTableField*>(field);
    const BRow* row = NULL;
    if (tv && f && f->Row()) row = f->Row();
    else if (tv && parent->Parent()) row = tv->RowAt(BPoint(rect.left, rect.top + 1));

    bool selected = row && row->IsSelected();
    bool virt = tv && tv->IsVirtual() && fIhandle && row;
    if (virt) f = NULL;
    if (!virt && !f) { BTitledColumn::DrawField(field, rect, parent); return; }

    int lin = row ? tv->IndexOf(const_cast<BRow*>(row)) + 1 : 0;
    int col = LogicalFieldNum() + 1;

    const char* virt_str = NULL;
    BBitmap* virt_icon = NULL;
    if (virt)
    {
      sIFnii vcb = (sIFnii)IupGetCallback(fIhandle, "VALUE_CB");
      if (vcb) virt_str = vcb(fIhandle, lin, col);
      char* iname = iupTableGetCellImageCb(fIhandle, lin, col);
      if (iname) virt_icon = haikuTableBitmapByName(fIhandle, iname);
    }

    /* Bg priority: selection > BGCOLOR (cell/col/row) > alternating > base. */
    if (!selected)
    {
      bool drew = false;
      unsigned char r, g, b;
      const char* bg = fIhandle ? haikuTableLookupId2(fIhandle, "BGCOLOR", lin, col) : NULL;
      if (bg && iupStrToRGB(bg, &r, &g, &b))
      {
        rgb_color c = { r, g, b, 255 };
        parent->SetLowColor(c);
        parent->FillRect(rect, B_SOLID_LOW);
        drew = true;
      }
      else if (row && fIhandle && iupAttribGetBoolean(fIhandle, "ALTERNATECOLOR"))
      {
        const char* cs = (lin % 2 == 0) ? iupAttribGet(fIhandle, "EVENROWCOLOR")
                                        : iupAttribGet(fIhandle, "ODDROWCOLOR");
        if (cs && iupStrToRGB(cs, &r, &g, &b))
        {
          rgb_color c = { r, g, b, 255 };
          parent->SetLowColor(c);
          parent->FillRect(rect, B_SOLID_LOW);
          drew = true;
        }
      }
      if (!drew && tv)
      {
        parent->SetLowColor(tv->Color(B_COLOR_BACKGROUND));
        parent->FillRect(rect, B_SOLID_LOW);
      }
    }

    BRect cell = rect;
    cell.left += 4;
    cell.right -= 4;

    BBitmap* draw_icon = virt ? virt_icon : f->Icon();
    if (draw_icon)
    {
      float draw_w, draw_h;
      IconDrawSize(draw_icon, cell.Height() + 1.0f, &draw_w, &draw_h);
      float iy = cell.top + ((cell.Height() - draw_h) / 2);
      parent->SetDrawingMode(B_OP_ALPHA);
      parent->DrawBitmap(draw_icon,
                         BRect(cell.left, iy, cell.left + draw_w - 1, iy + draw_h - 1));
      parent->SetDrawingMode(B_OP_COPY);
      cell.left += draw_w + 6;
    }

    if (fIhandle)
    {
      unsigned char r, g, b;
      const char* fg = haikuTableLookupId2(fIhandle, "FGCOLOR", lin, col);
      if (fg && iupStrToRGB(fg, &r, &g, &b))
      {
        rgb_color c = { r, g, b, 255 };
        parent->SetHighColor(c);
      }
      const char* fn = haikuTableLookupId2(fIhandle, "FONT", lin, col);
      if (fn)
      {
        BFont* bf = iuphaikuGetBFont(fn);
        if (bf) parent->SetFont(bf);
      }
    }

    if (fIhandle && !iupdrvIsActive(fIhandle))
      parent->SetHighColor(haikuTableDimColor(parent->HighColor()));

    DrawString(virt ? (virt_str ? virt_str : "") : f->String(), parent, cell);

    if (tv && fIhandle && iupAttribGetBoolean(fIhandle, "SHOWGRID"))
    {
      parent->SetHighColor(tv->GridLineColor());
      bool is_last = (col == tv->CountColumns());
      bool stretch_last = fIhandle->data->stretch_last;
      if (!(is_last && stretch_last))
        parent->StrokeLine(BPoint(rect.right, rect.top), BPoint(rect.right, rect.bottom));
      parent->StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.right, rect.bottom));
    }

    if (tv && fIhandle && row && iupAttribGetBoolean(fIhandle, "FOCUSRECT"))
    {
      BRow* fr = tv->FocusRow();
      if (!fr) fr = tv->CurrentSelection();
      if (fr == row && col == tv->FocusCol())
      {
        parent->SetHighColor(selected ? iuphaikuColor(B_LIST_SELECTED_ITEM_TEXT_COLOR)
                                      : ui_color(B_KEYBOARD_NAVIGATION_COLOR));
        parent->StrokeRect(BRect(rect.left + 1, rect.top + 1, rect.right - 1, rect.bottom - 1));
      }
    }
  }

  void DrawTitle(BRect rect, BView* parent) override
  {
    if (fIhandle && !iupdrvIsActive(fIhandle))
      parent->SetHighColor(haikuTableDimColor(parent->HighColor()));
    BTitledColumn::DrawTitle(rect, parent);
  }

  int CompareFields(BField* a, BField* b) override
  {
    (void)a;
    (void)b;
    return 0;
  }

  /* Per-cell natural width; CLV maxes across rows, factors in the title, clamps to MinWidth. */
  float GetPreferredWidth(BField* field, BView* parent) const override
  {
    IupHaikuTableField* f = dynamic_cast<IupHaikuTableField*>(field);
    if (!f || !parent) return Width();
    BFont font;
    parent->GetFont(&font);
    const char* s = f->String();
    float text_w = (s && *s) ? font.StringWidth(s) : 0.0f;
    float icon_w = 0.0f;
    if (f->Icon())
    {
      float dh;
      IconDrawSize(f->Icon(), haikuTableRowHeight(dynamic_cast<BColumnListView*>(parent->Parent())), &icon_w, &dh);
    }
    float spacing = icon_w > 0 ? 6.0f : 0.0f;
    return text_w + icon_w + spacing + 12.0f;
  }

  bool AcceptsField(const BField* field) const override
  {
    return dynamic_cast<const IupHaikuTableField*>(field) != NULL
        || dynamic_cast<const IupHaikuVirtualField*>(field) != NULL;
  }

private:
  Ihandle* fIhandle;
};

/* All helpers below take IUP's 1-based lin/col. */

static IupHaikuTableView* haikuTableGetView(Ihandle* ih)
{
  if (!ih || !ih->handle) return NULL;
  return (IupHaikuTableView*)ih->handle;
}

/* WIDTHn is dropped from the hash after map; track explicit width with a flag. */
static bool haikuTableColHasExplicitWidth(Ihandle* ih, int col)
{
  if (iupAttribGetId(ih, "_IUPHAIKU_EXPLICITWIDTH", col))
    return true;
  if (iupAttribGetIntId(ih, "RASTERWIDTH", col) > 0) return true;
  if (iupAttribGetIntId(ih, "WIDTH",       col) > 0) return true;
  return false;
}

/* One-shot column auto-size: title (header font) + first 100 rows of content. */
static void haikuTableAutoSizeColumn(Ihandle* ih, IupHaikuTableView* tv, int i)
{
  IupHaikuTableColumn* c = (IupHaikuTableColumn*)tv->ColumnAt(i);
  if (!c) return;

  int num_lin = tv->CountRows(NULL);
  int max_rows = num_lin > 100 ? 100 : num_lin;
  bool sortable = iupAttribGetBoolean(ih, "SORTABLE");

  BFont header_font;
  tv->GetFont(B_FONT_HEADER, &header_font);
  BFont row_font;
  tv->GetFont(B_FONT_ROW, &row_font);

  BString name;
  c->GetColumnName(&name);
  float w = header_font.StringWidth(name.String()) + 16.0f;
  if (sortable) w += 11.0f;

  sIFnii vcb = tv->IsVirtual() ? (sIFnii)IupGetCallback(ih, "VALUE_CB") : NULL;
  int col = c->LogicalFieldNum() + 1;

  for (int lin = 0; lin < max_rows; lin++)
  {
    const char* s;
    bool icon;
    if (tv->IsVirtual())
    {
      s = vcb ? vcb(ih, lin + 1, col) : NULL;
      icon = iupTableGetCellImageCb(ih, lin + 1, col) != NULL;
    }
    else
    {
      BRow* row = tv->RowAt(lin, NULL);
      if (!row) continue;
      IupHaikuTableField* f = (IupHaikuTableField*)row->GetField(i);
      if (!f) continue;
      s = f->String();
      icon = f->Icon() != NULL;
    }
    float cw = (s && *s) ? row_font.StringWidth(s) : 0.0f;
    if (icon) cw += 22.0f;
    cw += 16.0f;
    if (cw > w) w = cw;
  }

  if (w < 50.0f) w = 50.0f;
  if (w > c->MaxWidth()) w = c->MaxWidth();
  c->SetWidth(w);
}

static void haikuTableAutoSizeColumns(Ihandle* ih, IupHaikuTableView* tv)
{
  int num_col = tv->CountColumns();
  for (int i = 0; i < num_col; i++)
  {
    if (!haikuTableColHasExplicitWidth(ih, i + 1))
      haikuTableAutoSizeColumn(ih, tv, i);
  }
}

static void haikuTableMoveColumn(Ihandle* ih, IupHaikuTableView* tv, int from_col, int to_col, int attribs)
{
  int n = (int)tv->CountColumns();
  std::vector<BString> titles(n);
  std::vector<float> widths(n);
  std::vector<alignment> aligns(n);
  int c;

  for (c = 0; c < n; c++)
  {
    BColumn* col = tv->ColumnAt(c);
    col->GetColumnName(&titles[c]);
    widths[c] = col->Width();
    aligns[c] = col->Alignment();
  }

  if (!tv->IsVirtual())
  {
    int nr = (int)tv->CountRows(NULL);
    std::vector<BString> texts(n);
    std::vector<BBitmap*> icons(n);
    for (int i = 0; i < nr; i++)
    {
      BRow* r = tv->RowAt(i, NULL);
      if (!r) continue;
      for (c = 0; c < n; c++)
      {
        IupHaikuTableField* f = dynamic_cast<IupHaikuTableField*>(r->GetField(c));
        texts[c] = f ? f->String() : "";
        icons[c] = f ? f->Icon() : NULL;
      }
      for (c = 0; c < n; c++)
      {
        IupHaikuTableField* f = dynamic_cast<IupHaikuTableField*>(r->GetField(iupTableMoveColPos(c + 1, from_col, to_col) - 1));
        if (!f) continue;
        f->SetString(texts[c].String());
        f->SetIcon(icons[c]);
      }
    }
  }

  {
    int step = (from_col < to_col) ? 1 : -1;
    char* saved = iupStrDup(iupAttribGetId(ih, "_IUPHAIKU_EXPLICITWIDTH", from_col));
    for (c = from_col; c != to_col; c += step)
      iupAttribSetStrId(ih, "_IUPHAIKU_EXPLICITWIDTH", c, iupAttribGetId(ih, "_IUPHAIKU_EXPLICITWIDTH", c + step));
    iupAttribSetStrId(ih, "_IUPHAIKU_EXPLICITWIDTH", to_col, saved);
    if (saved) free(saved);
  }

  if (attribs)
    iupTableMoveColAttribs(ih, from_col, to_col);

  for (c = 0; c < n; c++)
  {
    IupHaikuTableColumn* col = (IupHaikuTableColumn*)tv->ColumnAt(iupTableMoveColPos(c + 1, from_col, to_col) - 1);
    col->SetTitle(titles[c].String());
    col->SetWidth(widths[c]);
    col->SetAlignment(aligns[c]);
  }

  int sort_col = iupAttribGetInt(ih, "_IUPHAIKU_SORTCOL");
  if (sort_col > 0)
  {
    sort_col = iupTableMoveColPos(sort_col, from_col, to_col);
    iupAttribSetInt(ih, "_IUPHAIKU_SORTCOL", sort_col);
    IupHaikuTableColumn* col = (IupHaikuTableColumn*)tv->ColumnAt(sort_col - 1);
    tv->ClearSortColumns();
    tv->SetSortColumn(col, false, iupAttribGetInt(ih, "_IUPHAIKU_SORTASC") != 0);
  }

  if (tv->FocusCol() > 0)
    tv->SetFocusCol(iupTableMoveColPos(tv->FocusCol(), from_col, to_col));

  if (ih->data->stretch_last && (from_col == n || to_col == n))
  {
    int moved = iupTableMoveColPos(n, from_col, to_col);
    if (!haikuTableColHasExplicitWidth(ih, moved))
      haikuTableAutoSizeColumn(ih, tv, moved - 1);
    tv->StretchLastColumn();
  }
}

static void haikuTableCheckColumnMove(Ihandle* ih, IupHaikuTableView* tv)
{
  int n = (int)tv->CountColumns();
  int a = -1, b = -1, from, to, i;

  for (i = 0; i < n; i++)
  {
    BColumn* col = tv->ColumnAt(i);
    if (col && col->LogicalFieldNum() != i)
    {
      if (a < 0) a = i;
      b = i;
    }
  }
  if (a < 0)
    return;

  if (tv->ColumnAt(b)->LogicalFieldNum() == a) { from = a; to = b; }
  else if (tv->ColumnAt(a)->LogicalFieldNum() == b) { from = b; to = a; }
  else return;

  tv->MoveColumn(tv->ColumnAt(to), from);

  IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  int ret = cb ? cb(ih, from + 1, to + 1) : IUP_DEFAULT;
  if (ret != IUP_IGNORE)
    haikuTableMoveColumn(ih, tv, from + 1, to + 1, 1);

  tv->Invalidate();
  if (BView* outline = tv->ScrollView()) outline->Invalidate();

  if (ret == IUP_CLOSE)
    IupExitLoop();
}

static IupHaikuTableColumn* haikuTableGetColumn(IupHaikuTableView* tv, int col)
{
  return tv ? (IupHaikuTableColumn*)tv->ColumnAt(col - 1) : NULL;
}

static void haikuTableUserSort(Ihandle* ih, IupHaikuTableView* tv, int col)
{
  IupHaikuTableColumn* column = haikuTableGetColumn(tv, col);
  int prev_col = iupAttribGetInt(ih, "_IUPHAIKU_SORTCOL");
  int prev_asc = iupAttribGetInt(ih, "_IUPHAIKU_SORTASC");
  int ascending = (prev_col == col) ? !prev_asc : 1;
  IFni cb = (IFni)IupGetCallback(ih, "SORT_CB");

  if (!column || !ih->data->sortable)
    return;

  if (cb && cb(ih, col) == IUP_IGNORE)
  {
    tv->ClearSortColumns();
    if (prev_col > 0 && haikuTableGetColumn(tv, prev_col))
      tv->SetSortColumn(haikuTableGetColumn(tv, prev_col), false, prev_asc != 0);
    tv->Invalidate();
    return;
  }

  iupAttribSetInt(ih, "_IUPHAIKU_SORTCOL", col);
  iupAttribSetInt(ih, "_IUPHAIKU_SORTASC", ascending);
  tv->SetSortColumn(column, false, ascending != 0);

  int n = (int)tv->CountRows(NULL);
  if (tv->IsVirtual() || n < 2)
  {
    tv->Invalidate();
    return;
  }

  tv->ClearSortColumns();

  std::vector<BRow*> rows(n);
  std::vector<int> order(n);
  for (int i = 0; i < n; i++)
  {
    rows[i] = tv->RowAt(i, NULL);
    order[i] = i + 1;
  }

  std::stable_sort(order.begin(), order.end(), [&rows, col, ascending](int a, int b) {
    IupHaikuTableField* fa = dynamic_cast<IupHaikuTableField*>(rows[a - 1]->GetField(col - 1));
    IupHaikuTableField* fb = dynamic_cast<IupHaikuTableField*>(rows[b - 1]->GetField(col - 1));
    int cmp = iupStrCompare(fa ? fa->String() : "", fb ? fb->String() : "", 0, 1);
    return ascending ? cmp < 0 : cmp > 0;
  });

  BRow* focus_row = tv->FocusRow();
  std::vector<BRow*> selected;
  for (BRow* row = tv->CurrentSelection(); row; row = tv->CurrentSelection(row))
    selected.push_back(row);

  BList old_rows, new_rows;
  for (int i = 0; i < n; i++)
  {
    old_rows.AddItem(rows[i]);
    new_rows.AddItem(rows[order[i] - 1]);
  }

  char* ignore = iupAttribGet(ih, "_IUPTABLE_IGNORE_SELECTION_CB");
  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", "1");

  tv->RemoveRows(&old_rows);
  tv->AddRows(&new_rows, -1, NULL);

  for (BRow* row : selected)
    tv->AddToSelection(row);
  if (focus_row)
    tv->SetFocusRow(focus_row, false);

  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", ignore);

  tv->SetSortColumn(column, false, ascending != 0);
  iupTableSortLinAttribs(ih, order.data());
  tv->Invalidate();
  if (BView* outline = tv->ScrollView())
    outline->Invalidate();
}

static BRow* haikuTableGetRow(IupHaikuTableView* tv, int lin)
{
  return tv ? tv->RowAt(lin - 1, NULL) : NULL;
}

static IupHaikuTableField* haikuTableGetField(IupHaikuTableView* tv, int lin, int col)
{
  BRow* r = haikuTableGetRow(tv, lin);
  if (!r) return NULL;
  return (IupHaikuTableField*)r->GetField(col - 1);
}

static IupHaikuTableField* haikuTableEnsureField(IupHaikuTableView* tv, int lin, int col)
{
  BRow* r = haikuTableGetRow(tv, lin);
  if (!r) return NULL;
  IupHaikuTableField* f = (IupHaikuTableField*)r->GetField(col - 1);
  if (!f)
  {
    f = new IupHaikuTableField("");
    f->SetRow(r);
    r->SetField(f, col - 1);
  }
  return f;
}

static int haikuTableFollowPos(int cur, int pos, int delta, int count)
{
  if (cur <= 0)
    return cur;
  if (cur >= pos && !(delta < 0 && cur == pos))
    cur += delta;
  if (cur > count)
    cur = count;
  return cur;
}

static int haikuTableFocusLin(IupHaikuTableView* tv)
{
  BRow* r = tv->FocusRow();
  return r ? (int)tv->IndexOf(r) + 1 : 0;
}

static void haikuTableRestoreFocusLin(IupHaikuTableView* tv, int focus_lin, int pos, int delta, int num_lin)
{
  int lin = haikuTableFollowPos(focus_lin, pos, delta, num_lin);
  if (lin > 0 && haikuTableFocusLin(tv) != lin)
    tv->SetFocusRow(lin - 1, false);
}

extern "C" IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());

  ih->data->num_lin = num_lin;

  /* the cells are filled after this returns, so measure the columns later */
  if (num_lin > 0 && !tv->MeasuredWithRows())
    tv->ScheduleAutoSize();

  int cur = tv->CountRows(NULL);
  int focus_lin = haikuTableFocusLin(tv);
  std::vector<int> selected;
  BView* outline = tv->ScrollView();
  float top = outline ? outline->Bounds().top : 0.0f;
  bool rebuilt = false;

  if (tv->IsVirtual() && num_lin < cur)
  {
    for (BRow* row = tv->CurrentSelection(); row; row = tv->CurrentSelection(row))
    {
      int lin = (int)tv->IndexOf(row);
      if (lin < num_lin)
        selected.push_back(lin);
    }
    if (BRow* focus_row = tv->FocusRow())
    {
      tv->RemoveRow(focus_row);
      delete focus_row;
    }
    tv->Clear();
    cur = 0;
    rebuilt = true;
  }

  if (num_lin > cur)
  {
    int n_cols = tv->CountColumns();
    float row_h = haikuTableRowHeight(tv);
    bool virt = tv->IsVirtual();
    /* Batch insert; one AddRow() per row is too slow for large tables. */
    BList batch;
    for (int i = cur; i < num_lin; ++i)
    {
      BRow* r = new BRow(row_h);
      if (virt)
        for (int c = 0; c < n_cols; ++c) r->SetField(&g_virtual_field, c);
      else
        for (int c = 0; c < n_cols; ++c)
        {
          IupHaikuTableField* f = new IupHaikuTableField("");
          f->SetRow(r);
          r->SetField(f, c);
        }
      batch.AddItem(r);
    }
    tv->SuspendSortColumn();
    tv->AddRows(&batch, -1, NULL);
    tv->RestoreSortColumn();
  }
  else if (num_lin < cur)
  {
    for (int i = cur - 1; i >= num_lin; --i)
    {
      BRow* r = tv->RowAt(i, NULL);
      if (r) { tv->RemoveRow(r); delete r; }
    }
  }

  if (rebuilt)
  {
    for (int lin : selected)
      tv->AddToSelection(tv->RowAt(lin, NULL));
    if (outline)
    {
      float max_top = num_lin * (haikuTableRowHeight(tv) + 1.0f) - outline->Bounds().Height();
      outline->ScrollTo(outline->Bounds().left, top < max_top ? top : (max_top > 0.0f ? max_top : 0.0f));
    }
  }

  haikuTableRestoreFocusLin(tv, focus_lin, num_lin + 1, 0, num_lin);
}

extern "C" IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());

  ih->data->num_col = num_col;

  int cur = tv->CountColumns();
  if (num_col > cur)
  {
    for (int i = cur; i < num_col; ++i)
    {
      IupHaikuTableColumn* c = new IupHaikuTableColumn(ih, "", 100.0f, B_ALIGN_LEFT);
      tv->AddColumn(c, i);
    }

    int nr = (int)tv->CountRows(NULL);
    for (int r_i = 0; r_i < nr; r_i++)
    {
      BRow* r = tv->RowAt(r_i, NULL);
      if (!r) continue;
      for (int i = cur; i < num_col; ++i)
      {
        if (tv->IsVirtual())
          r->SetField(&g_virtual_field, i);
        else
        {
          IupHaikuTableField* f = new IupHaikuTableField("");
          f->SetRow(r);
          r->SetField(f, i);
        }
      }
    }
  }
  else if (num_col < cur)
  {
    for (int i = cur - 1; i >= num_col; --i)
    {
      BColumn* c = tv->ColumnAt(i);
      if (c) { tv->RemoveColumn(c); delete c; }
    }
  }
  tv->SetFocusCol(haikuTableFollowPos(tv->FocusCol(), num_col + 1, 0, num_col));
  tv->RepositionTrail();
}

extern "C" IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  int n_cols = tv->CountColumns();
  BRow* r = new BRow(haikuTableRowHeight(tv));
  if (tv->IsVirtual())
    for (int c = 0; c < n_cols; ++c) r->SetField(&g_virtual_field, c);
  else
    for (int c = 0; c < n_cols; ++c)
    {
      IupHaikuTableField* f = new IupHaikuTableField("");
      f->SetRow(r);
      r->SetField(f, c);
    }
  tv->SuspendSortColumn();
  tv->AddRow(r, pos - 1, NULL);
  tv->RestoreSortColumn();
  ih->data->num_lin++;
}

extern "C" IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  int focus_lin = haikuTableFocusLin(tv);
  BRow* r = tv->RowAt(pos - 1, NULL);
  if (r) { tv->RemoveRow(r); delete r; ih->data->num_lin--; }
  haikuTableRestoreFocusLin(tv, focus_lin, pos, -1, ih->data->num_lin);
}

extern "C" IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  int focus_col = tv->FocusCol();
  iupdrvTableSetNumCol(ih, ih->data->num_col + 1);
  LooperLockGuard guard(tv->Looper());
  if (pos < ih->data->num_col)
    haikuTableMoveColumn(ih, tv, ih->data->num_col, pos, 0);
  tv->SetFocusCol(haikuTableFollowPos(focus_col, pos, 1, ih->data->num_col));
}

extern "C" IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv || ih->data->num_col < 1) return;
  int focus_col = tv->FocusCol();
  if (pos < ih->data->num_col)
  {
    LooperLockGuard guard(tv->Looper());
    haikuTableMoveColumn(ih, tv, pos, ih->data->num_col, 0);
  }
  iupdrvTableSetNumCol(ih, ih->data->num_col - 1);
  LooperLockGuard guard(tv->Looper());
  tv->SetFocusCol(haikuTableFollowPos(focus_col, pos, -1, ih->data->num_col));
}

extern "C" IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  if (tv->IsVirtual()) return;
  LooperLockGuard guard(tv->Looper());
  IupHaikuTableField* f = haikuTableEnsureField(tv, lin, col);
  if (!f) return;
  f->SetString(value ? value : "");
  BRow* r = haikuTableGetRow(tv, lin);
  if (r) tv->UpdateRow(r);
}

extern "C" IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return NULL;
  if (tv->IsVirtual())
  {
    sIFnii vcb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    return vcb ? iupStrReturnStr(vcb(ih, lin, col)) : NULL;
  }
  IupHaikuTableField* f = haikuTableGetField(tv, lin, col);
  if (!f) return NULL;
  return iupStrReturnStr(f->String());
}

extern "C" IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  if (tv->IsVirtual()) return;
  LooperLockGuard guard(tv->Looper());
  IupHaikuTableField* f = haikuTableEnsureField(tv, lin, col);
  if (!f) return;
  f->SetIcon(haikuTableBitmapByName(ih, image));
  BRow* r = haikuTableGetRow(tv, lin);
  if (r) tv->UpdateRow(r);
}

extern "C" IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  IupHaikuTableColumn* c = haikuTableGetColumn(tv, col);
  if (!tv || !c) return;
  LooperLockGuard guard(tv->Looper());
  c->SetTitle(title ? title : "");
  tv->Invalidate();
}

extern "C" IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  IupHaikuTableColumn* c = haikuTableGetColumn(tv, col);
  if (!tv || !c) return NULL;
  BString s;
  c->GetColumnName(&s);
  return iupStrReturnStr(s.String());
}

extern "C" IUP_SDK_API void iupdrvTableSetSortSign(Ihandle* ih, int col, int sign)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  IupHaikuTableColumn* c = haikuTableGetColumn(tv, col);
  if (!tv || !c) return;

  iupAttribSetInt(ih, "_IUPHAIKU_SORTCOL", sign ? col : 0);
  iupAttribSetInt(ih, "_IUPHAIKU_SORTASC", sign > 0);

  LooperLockGuard guard(tv->Looper());
  tv->ClearSortColumns();

  if (sign != 0)
    tv->SetSortColumn(c, false, sign > 0);

  tv->Invalidate();
}

extern "C" IUP_SDK_API int iupdrvTableGetSortSign(Ihandle* ih, int col)
{
  if (iupAttribGetInt(ih, "_IUPHAIKU_SORTCOL") != col)
    return 0;

  return iupAttribGetInt(ih, "_IUPHAIKU_SORTASC") ? 1 : -1;
}

extern "C" IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  IupHaikuTableColumn* c = haikuTableGetColumn(tv, col);
  if (!tv || !c) return;
  iupAttribSetId(ih, "_IUPHAIKU_EXPLICITWIDTH", col, "1");
  LooperLockGuard guard(tv->Looper());
  c->SetWidth((float)width);
  tv->RepositionTrail();
  tv->Invalidate();
}

extern "C" IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  IupHaikuTableColumn* c = haikuTableGetColumn(tv, col);
  if (!tv || !c) return 80;
  return (int)c->Width();
}

extern "C" IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  bool select = !iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTIONMODE"), "NONE");
  if (col > 0) tv->SetFocusCol(col);
  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", "1");
  if (select) tv->DeselectAll();
  tv->SetFocusRow(lin - 1, select);
  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", NULL);
  tv->ScrollToLine(lin);
  if (BView* outline = tv->ScrollView()) outline->Invalidate();
}

extern "C" IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (lin) *lin = 0;
  if (col) *col = 0;
  if (!tv) return;
  BRow* r = tv->FocusRow();
  if (r && lin) *lin = tv->IndexOf(r) + 1;
  if (col) *col = tv->FocusCol();
}

extern "C" IUP_SDK_API int iupdrvTableIsLinSelected(Ihandle* ih, int lin)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return 0;
  LooperLockGuard guard(tv->Looper());
  BRow* row = haikuTableGetRow(tv, lin);
  return (row && row->IsSelected()) ? 1 : 0;
}

extern "C" IUP_SDK_API void iupdrvTableSelectLin(Ihandle* ih, int lin, int select)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  BRow* row = haikuTableGetRow(tv, lin);
  if (!row) return;

  if (select)
    tv->AddToSelection(row);
  else
    tv->Deselect(row);
}

extern "C" IUP_SDK_API int* iupdrvTableGetSelectedLins(Ihandle* ih, int* count)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  *count = 0;
  if (!tv) return NULL;

  LooperLockGuard guard(tv->Looper());

  int total = 0;
  for (BRow* row = tv->CurrentSelection(); row; row = tv->CurrentSelection(row))
    total++;

  if (total == 0) return NULL;

  int* lins = (int*)malloc(sizeof(int) * total);
  int i = 0;

  for (BRow* row = tv->CurrentSelection(); row && i < total; row = tv->CurrentSelection(row))
  {
    int lin = tv->IndexOf(row);
    if (lin >= 0) lins[i++] = lin + 1;
  }

  *count = i;
  return lins;
}

extern "C" IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int /*col*/)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  tv->ScrollToLine(lin);
}

extern "C" IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  tv->Refresh();
  tv->Invalidate();
}

extern "C" IUP_SDK_API void iupdrvTableUpdateCellStyle(Ihandle* ih, int /*lin*/, int /*col*/)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  tv->Invalidate();
}

extern "C" IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  /* Commit before redraw; the core setter only stores after returning. */
  iupAttribSetStr(ih, "SHOWGRID", show ? "YES" : "NO");
  iupdrvTableRedraw(ih);
}

extern "C" IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* /*ih*/)
{
  return 1;
}

extern "C" IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  /* GetFont needs the looper locked; natural-size may run unlocked. */
  LooperLockGuard guard(tv ? tv->Looper() : NULL);
  /* CLV accumulates Height() + 1 per row in fItemsHeight; return the pitch. */
  return (int)(haikuTableRowHeight(tv) + 0.5f) + 1;
}

extern "C" IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return 20;
  LooperLockGuard guard(tv->Looper());
  BFont font;
  tv->GetFont(B_FONT_HEADER, &font);
  int hh = (int)ceilf(font.Size() * 1.4f);
  if (hh < 16) hh = 16;
  return hh;
}

extern "C" IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int border = 2 * 2;
  int sb = iupdrvGetScrollbarSize();
  const int latch = 15;
  if (w) *w += border + sb + latch;
  if (h)
  {
    /* FANCY_BORDER (4) + 1 px gap between header and outline + 1 inclusive-bounds. */
    *h += 6;
    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
      *h += sb;
  }
}

static void haikuTableApplyColumnFlags(Ihandle* ih)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return;
  uint32 flags = 0;
  if (ih->data->user_resize)   flags |= B_ALLOW_COLUMN_RESIZE;
  if (ih->data->allow_reorder) flags |= B_ALLOW_COLUMN_MOVE;
  tv->SetColumnFlags((column_flags)flags);
}

static void haikuTableLayoutUpdateMethod(Ihandle* ih)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) { iupdrvBaseLayoutUpdateMethod(ih); return; }

  if (tv->NeedsAutoSize())
  {
    LooperLockGuard guard(tv->Looper());
    haikuTableAutoSizeColumns(ih, tv);
    tv->StretchLastColumn();
    tv->ClearNeedsAutoSize();
    if (ih->data->num_lin > 0)
      tv->SetMeasuredWithRows();
  }

  iuphaikuSetPosSize(tv, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

IUP_DRV_API void iuphaikuTableUpdateColors(Ihandle* ih)
{
  IupHaikuTableView* tv = (IupHaikuTableView*)ih->handle;
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  tv->ApplyColors();
}

static int haikuTableMapMethod(Ihandle* ih)
{
  IupHaikuTableView* tv = new IupHaikuTableView(ih);
  ih->handle = (InativeHandle*)tv;
  iuphaikuAddToParent(ih);

  tv->SetVirtual(iupAttribGetBoolean(ih, "VIRTUALMODE"));

  iupdrvTableSetNumCol(ih, ih->data->num_col);
  iupdrvTableSetNumLin(ih, ih->data->num_lin);

  LooperLockGuard guard(tv->Looper());
  tv->ApplyColors();
  tv->SetSortingEnabled(ih->data->sortable ? true : false);
  {
    /* CLV defaults to MULTIPLE; IUP's default is SINGLE. */
    const char* selmode = iupAttribGet(ih, "SELECTIONMODE");
    tv->SetSelectionMode(selmode && iupStrEqualNoCase(selmode, "MULTIPLE")
                         ? B_MULTIPLE_SELECTION_LIST : B_SINGLE_SELECTION_LIST);
  }
  haikuTableApplyColumnFlags(ih);
  tv->StretchLastColumn();
  tv->UpdateScrollBarVisibility();

  if (BView* outline = tv->ScrollView())
  {
    IupHaikuTrailDividers* trail = new IupHaikuTrailDividers(tv);
    outline->AddChild(trail);
    tv->SetTrailView(trail);
    tv->RepositionTrail();
  }

  if (BWindow* win = tv->Window())
  {
    IupHaikuTableSortFilter* sf = new IupHaikuTableSortFilter(tv);
    win->AddCommonFilter(sf);
    tv->SetSortFilter(sf);
  }

  return IUP_NOERROR;
}

static void haikuTableUnMapMethod(Ihandle* ih)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (tv)
  {
    LooperLockGuard guard(tv->Looper());

    if (BWindow* win = tv->Window())
    {
      BMessageFilter* sf = tv->SortFilter();
      if (sf) { win->RemoveCommonFilter(sf); delete sf; tv->SetSortFilter(NULL); }
    }

    if (tv->Editor()) tv->EndEdit(false);

    tv->CancelAutoSize();
    tv->SetIhandle(NULL);

    /* BColumnListView::Clear() does NOT delete rows; RemoveColumn / RemoveRow leave items to the caller. */
    while (tv->CountRows(NULL) > 0)
    {
      BRow* r = const_cast<BRow*>(tv->RowAt(0, NULL));
      if (!r) break;
      tv->RemoveRow(r);
      delete r;
    }
    while (tv->CountColumns() > 0)
    {
      BColumn* c = tv->ColumnAt(0);
      if (!c) break;
      tv->RemoveColumn(c);
      delete c;
    }
  }
  iupdrvBaseUnMapMethod(ih);
}

static int haikuTableSetAlignmentIdAttrib(Ihandle* ih, int col, const char* value)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  IupHaikuTableColumn* c = haikuTableGetColumn(tv, col);
  if (!tv || !c) return 1;
  alignment a = B_ALIGN_LEFT;
  if      (iupStrEqualNoCase(value, "ARIGHT"))  a = B_ALIGN_RIGHT;
  else if (iupStrEqualNoCase(value, "ACENTER")) a = B_ALIGN_CENTER;
  LooperLockGuard guard(tv->Looper());
  c->SetAlignment(a);
  tv->Invalidate();
  return 1;
}

static int haikuTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  ih->data->sortable = iupStrBoolean(value) ? 1 : 0;
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (tv)
  {
    LooperLockGuard guard(tv->Looper());
    if (!ih->data->sortable)
    {
      tv->ClearSortColumns();
      iupAttribSet(ih, "_IUPHAIKU_SORTCOL", NULL);
      iupAttribSet(ih, "_IUPHAIKU_SORTASC", NULL);
    }
    tv->SetSortingEnabled(ih->data->sortable ? true : false);
    tv->Invalidate();
  }
  return 1;
}

static int haikuTableSetSelectionModeAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (!tv) return 1;
  LooperLockGuard guard(tv->Looper());
  list_view_type t = iupStrEqualNoCase(value, "MULTIPLE")
                        ? B_MULTIPLE_SELECTION_LIST : B_SINGLE_SELECTION_LIST;
  tv->SetSelectionMode(t);
  return 1;
}

static int haikuTableSetStretchLastAttrib(Ihandle* ih, const char* value)
{
  ih->data->stretch_last = iupStrBoolean(value) ? 1 : 0;
  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (tv)
  {
    LooperLockGuard guard(tv->Looper());
    tv->StretchLastColumn();
  }
  return 1;
}

static int haikuTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  ih->data->user_resize = iupStrBoolean(value) ? 1 : 0;
  if (haikuTableGetView(ih)) haikuTableApplyColumnFlags(ih);
  return 1;
}

static int haikuTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  ih->data->allow_reorder = iupStrBoolean(value) ? 1 : 0;
  if (haikuTableGetView(ih)) haikuTableApplyColumnFlags(ih);
  return 1;
}

static int haikuTableSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);

  IupHaikuTableView* tv = haikuTableGetView(ih);
  if (tv)
  {
    LooperLockGuard guard(tv->Looper());
    if (BView* outline = tv->ScrollView()) outline->Invalidate();
    tv->Invalidate();
  }
  return 1;
}

extern "C" IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = haikuTableMapMethod;
  ic->UnMap = haikuTableUnMapMethod;
  ic->LayoutUpdate = haikuTableLayoutUpdateMethod;

  iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuTableSetActiveAttrib);

  iupClassRegisterAttributeId(ic, "ALIGNMENT", NULL, haikuTableSetAlignmentIdAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SORTABLE", NULL, haikuTableSetSortableAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "USERRESIZE", NULL, haikuTableSetUserResizeAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, haikuTableSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONMODE", NULL, haikuTableSetSelectionModeAttrib, IUPAF_SAMEASSYSTEM, "SINGLE", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STRETCHLAST", NULL, haikuTableSetStretchLastAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
}

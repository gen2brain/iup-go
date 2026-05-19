/** \file
 * \brief Haiku Tabs Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Bitmap.h>
#include <CardLayout.h>
#include <ControlLook.h>
#include <List.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_tabs.h"
#include "iup_drvfont.h"
#include "iup_image.h"
}

#include "iuphaiku_drv.h"


/* Compositing tab: icon (TABIMAGE) + label + close box (SHOWCLOSE). */
class IupHaikuTab : public BTab
{
public:
  static constexpr int kIconPad   = 4;  /* gap between icon and label */
  static constexpr int kClosePad  = 4;  /* gap between label and close box */
  static constexpr int kCloseSize = 12; /* close box side */

  IupHaikuTab() : fIcon(NULL), fIconW(0), fIconH(0), fShowClose(false), fCloseHot(false) {}

  /* bm is a weak ref into the IUP image cache; draw_w/draw_h are the size to render at */
  void SetIcon(BBitmap* bm, int draw_w, int draw_h)
  {
    fIcon = bm;
    if (bm && (draw_w <= 0 || draw_h <= 0))
    {
      draw_w = (int)(bm->Bounds().Width() + 1);
      draw_h = (int)(bm->Bounds().Height() + 1);
    }
    fIconW = bm ? draw_w : 0;
    fIconH = bm ? draw_h : 0;
  }
  void SetShowClose(bool v) { fShowClose = v; }
  bool ShowClose() const { return fShowClose; }
  void SetCloseHot(bool v) { fCloseHot = v; }
  float IconWidth() const { return fIcon ? (float)fIconW : 0.0f; }
  float IconHeight() const { return fIcon ? (float)fIconH : 0.0f; }
  float ExtraWidth() const
  {
    float extra = 0.0f;
    if (fIcon)      extra += IconWidth() + kIconPad;
    if (fShowClose) extra += kCloseSize + kClosePad;
    return extra;
  }

  BRect IconRect(BRect frame) const
  {
    if (!fIcon) return BRect();
    float spacing = be_control_look ? be_control_look->DefaultLabelSpacing() : 4.0f;
    float iw = (float)fIconW;
    float ih = (float)fIconH;
    float cy = frame.top + (frame.Height() - ih) / 2.0f;
    float cx = frame.left + spacing;
    return BRect(cx, cy, cx + iw - 1, cy + ih - 1);
  }

  BRect CloseRect(BRect frame) const
  {
    if (!fShowClose) return BRect();
    float spacing = be_control_look ? be_control_look->DefaultLabelSpacing() : 4.0f;
    float cy = frame.top + (frame.Height() - kCloseSize) / 2.0f;
    float cx = frame.right - spacing - kCloseSize + 1;
    return BRect(cx, cy, cx + kCloseSize - 1, cy + kCloseSize - 1);
  }

  /* Painting; only TOP/BOTTOM strips composite icon+close. LEFT/RIGHT falls back to BTab. */
  void DrawLabel(BView* owner, BRect frame) override
  {
    BTabView* tv = dynamic_cast<BTabView*>(owner);
    BTabView::tab_side side = tv ? tv->TabSide() : BTabView::kTopSide;
    bool horizontal = (side == BTabView::kTopSide || side == BTabView::kBottomSide);
    if (!horizontal || (!fIcon && !fShowClose))
    {
      BTab::DrawLabel(owner, frame);
      return;
    }

    BRect icon = IconRect(frame);
    BRect close = CloseRect(frame);

    BRect label = frame;
    if (fIcon)    label.left  = icon.right + kIconPad;
    if (fShowClose) label.right = close.left - kClosePad;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    rgb_color textcolor = ui_color(B_PANEL_TEXT_COLOR);
    uint32 flags = IsEnabled() ? 0 : BControlLook::B_DISABLED;

    if (fIcon)
    {
      owner->SetDrawingMode(B_OP_ALPHA);
      owner->DrawBitmap(fIcon, fIcon->Bounds(), icon, B_FILTER_BITMAP_BILINEAR);
      owner->SetDrawingMode(B_OP_COPY);
    }

    if (be_control_look)
      be_control_look->DrawLabel(owner, Label(), label, label, base, flags,
                                 BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER),
                                 &textcolor);

    if (fShowClose)
    {
      rgb_color cross = textcolor;
      if (fCloseHot)
      {
        owner->SetHighColor(tint_color(base, B_DARKEN_2_TINT));
        owner->FillRect(close);
        cross = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
      }
      owner->SetHighColor(cross);
      float inset = 3.0f;
      owner->StrokeLine(BPoint(close.left + inset,  close.top + inset),
                        BPoint(close.right - inset, close.bottom - inset));
      owner->StrokeLine(BPoint(close.left + inset,  close.bottom - inset),
                        BPoint(close.right - inset, close.top + inset));
    }
  }

private:
  BBitmap* fIcon;
  int fIconW;
  int fIconH;
  bool fShowClose;
  bool fCloseHot;
};


class IupHaikuTabView : public BTabView
{
public:
  explicit IupHaikuTabView(Ihandle* ih)
    : BTabView(BRect(0, 0, 0, 0), "iup_tabs", B_WIDTH_FROM_LABEL),
      fIhandle(ih), fHotCloseTab(-1), fHotTipTab(-1),
      fDragSource(-1), fDragTarget(-1), fDragging(false),
      fSuppressSelectCallbacks(false)
  {
  }

  /* BTabView indices skip hidden tabs; convert to IUP child positions. */
  int IupPosFromBTab(int bp) const
  {
    if (!fIhandle || bp < 0) return -1;
    int seen = 0, i = 0;
    for (Ihandle* c = fIhandle->firstchild; c; c = c->brother, i++)
    {
      if (iupAttribGet(c, "_IUPHAIKU_HIDDEN_TAB")) continue;
      if (seen == bp) return i;
      seen++;
    }
    return -1;
  }

  void Select(int32 tab) override
  {
    int32 prev = Selection();
    /* Freshly-attached pages keep their initial (0,0,0,0) frame; pre-size them. */
    BTab* btab = TabAt(tab);
    if (btab && btab->View() && ContainerView())
    {
      BRect cb = ContainerView()->Bounds();
      btab->View()->MoveTo(0, 0);
      btab->View()->ResizeTo(cb.Width(), cb.Height());
    }

    BTabView::Select(tab);
    if (fSuppressSelectCallbacks) return;
    if (!fIhandle || prev == tab) return;

    int iup_new = IupPosFromBTab(tab);
    int iup_old = IupPosFromBTab(prev);

    IFnnn cb = (IFnnn)IupGetCallback(fIhandle, "TABCHANGE_CB");
    if (cb)
    {
      Ihandle* new_child = (iup_new >= 0) ? IupGetChild(fIhandle, iup_new) : NULL;
      Ihandle* old_child = (iup_old >= 0) ? IupGetChild(fIhandle, iup_old) : NULL;
      cb(fIhandle, new_child, old_child);
    }
    IFnii cb2 = (IFnii)IupGetCallback(fIhandle, "TABCHANGEPOS_CB");
    if (cb2) cb2(fIhandle, iup_new, iup_old);
  }

  /* Close-box hit-test runs before BTabView's tab-switch lookup. */
  void MouseDown(BPoint where) override
  {
    int btab_idx = HitCloseAt(where);
    if (btab_idx >= 0 && fIhandle && iupObjectCheck(fIhandle))
    {
      int iup_pos = IupPosFromBTab(btab_idx);
      if (iup_pos < 0) return;
      IFni cb = (IFni)IupGetCallback(fIhandle, "TABCLOSE_CB");
      int ret = cb ? cb(fIhandle, iup_pos) : IUP_DEFAULT;
      if (ret == IUP_IGNORE) return;

      Ihandle* child = IupGetChild(fIhandle, iup_pos);
      if (ret == IUP_CONTINUE && child) IupDestroy(child);
      else if (child)
      {
        /* IUP_DEFAULT: detach the BTab, stash for later re-show. */
        BTab* tab = RemoveTab(btab_idx);
        if (tab)
        {
          iupAttribSet(child, "_IUPHAIKU_HIDDEN_TAB", (char*)tab);
          iupAttribSet(child, "TABVISIBLE", "NO");
        }
      }
      Invalidate();
      return;
    }

    if (fIhandle && iupAttribGetBoolean(fIhandle, "ALLOWREORDER"))
    {
      for (int32 i = 0; i < CountTabs(); i++)
      {
        if (TabFrame(i).Contains(where))
        {
          fDragSource = i;
          fDragTarget = i;
          fDragStart = where;
          fDragging = false;
          SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
          break;
        }
      }
    }

    BTabView::MouseDown(where);
  }

  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  {
    int idx = HitCloseAt(where);
    if (idx != fHotCloseTab)
    {
      IupHaikuTab* old = (fHotCloseTab >= 0) ? dynamic_cast<IupHaikuTab*>(TabAt(fHotCloseTab)) : NULL;
      if (old) { old->SetCloseHot(false); Invalidate(TabFrame(fHotCloseTab)); }
      IupHaikuTab* nw = (idx >= 0) ? dynamic_cast<IupHaikuTab*>(TabAt(idx)) : NULL;
      if (nw)  { nw->SetCloseHot(true);  Invalidate(TabFrame(idx)); }
      fHotCloseTab = idx;
    }

    int tip_idx = -1;
    for (int32 i = 0; i < CountTabs(); i++)
      if (TabFrame(i).Contains(where)) { tip_idx = i; break; }
    if (tip_idx != fHotTipTab)
    {
      fHotTipTab = tip_idx;
      const char* tip = NULL;
      if (tip_idx >= 0 && fIhandle)
      {
        int iup_pos = IupPosFromBTab(tip_idx);
        Ihandle* ch = (iup_pos >= 0) ? IupGetChild(fIhandle, iup_pos) : NULL;
        if (ch) tip = iupAttribGet(ch, "TABTIP");
        if (!tip && iup_pos >= 0) tip = iupAttribGetId(fIhandle, "TABTIP", iup_pos);
      }
      SetToolTip(tip);
    }

    if (fDragSource >= 0)
    {
      float dx = where.x - fDragStart.x;
      float dy = where.y - fDragStart.y;
      if (!fDragging && (dx * dx + dy * dy) >= 25.0f) fDragging = true;
      if (fDragging)
      {
        int new_target = -1;
        for (int32 i = 0; i < CountTabs(); i++)
          if (TabFrame(i).Contains(where)) { new_target = i; break; }
        if (new_target >= 0 && new_target != fDragTarget)
        {
          fDragTarget = new_target;
          Invalidate();
        }
      }
    }

    BTabView::MouseMoved(where, transit, drag);
  }

  void MouseUp(BPoint where) override
  {
    int src = fDragSource, tgt = fDragTarget;
    bool was = fDragging;
    fDragSource = -1; fDragTarget = -1; fDragging = false;

    if (was && src >= 0 && tgt >= 0 && src != tgt)
      ReorderBTab(src, tgt);
    if (was) Invalidate();

    BTabView::MouseUp(where);
  }

  /* Draw the reorder indicator (vertical line) on top of the strip. */
  void Draw(BRect updateRect) override
  {
    BTabView::Draw(updateRect);
    if (fDragging && fDragTarget >= 0)
    {
      BRect t = TabFrame(fDragTarget);
      float x = (fDragSource < fDragTarget) ? t.right : t.left;
      SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
      StrokeLine(BPoint(x, t.top), BPoint(x, t.bottom));
      StrokeLine(BPoint(x + 1, t.top), BPoint(x + 1, t.bottom));
    }
  }

  void ReorderBTab(int src_btab_pos, int tgt_btab_pos)
  {
    if (!fIhandle || src_btab_pos == tgt_btab_pos) return;
    int iup_src = IupPosFromBTab(src_btab_pos);
    int iup_tgt = IupPosFromBTab(tgt_btab_pos);
    if (iup_src < 0 || iup_tgt < 0) return;

    IFnii cb = (IFnii)IupGetCallback(fIhandle, "REORDER_CB");
    if (cb && cb(fIhandle, iup_src, iup_tgt) == IUP_IGNORE) return;

    Ihandle* dragged = IupGetChild(fIhandle, iup_src);
    if (!dragged) return;

    /* BTabView has no insert-at-pos; RemoveTab dragged + tail from tgt, append, restore tail.
     * tgt is the FINAL index of the moved tab (Fl_Group::insert semantics, matches IUP order). */
    int prev_selection = (int)Selection();
    fSuppressSelectCallbacks = true;
    BTab* moving = RemoveTab(src_btab_pos);
    if (!moving) { fSuppressSelectCallbacks = false; return; }

    BList tail;
    for (int n = CountTabs() - tgt_btab_pos; n > 0; --n)
    {
      BTab* t = RemoveTab(tgt_btab_pos);
      if (t) tail.AddItem(t);
    }
    AddTab(moving->View(), moving);
    for (int32 i = 0; i < tail.CountItems(); i++)
    {
      BTab* t = (BTab*)tail.ItemAt(i);
      AddTab(t->View(), t);
    }
    fSuppressSelectCallbacks = false;

    Ihandle* ref_child = (iup_src < iup_tgt)
                          ? IupGetChild(fIhandle, iup_tgt + 1)
                          : IupGetChild(fIhandle, iup_tgt);
    iupAttribSet(fIhandle, "_IUPTABS_REORDERING", "1");
    IupReparent(dragged, fIhandle, ref_child);
    iupAttribSet(fIhandle, "_IUPTABS_REORDERING", NULL);

    /* Track the selection across the move: follows the dragged tab or shifts. */
    int new_selection = prev_selection;
    if (prev_selection == src_btab_pos) new_selection = tgt_btab_pos;
    else if (src_btab_pos < tgt_btab_pos && prev_selection > src_btab_pos && prev_selection <= tgt_btab_pos) new_selection--;
    else if (src_btab_pos > tgt_btab_pos && prev_selection >= tgt_btab_pos && prev_selection < src_btab_pos) new_selection++;
    Select(new_selection);
  }

  int HitCloseAt(BPoint where) const
  {
    for (int32 i = 0; i < CountTabs(); i++)
    {
      IupHaikuTab* t = dynamic_cast<IupHaikuTab*>(TabAt(i));
      if (!t || !t->ShowClose()) continue;
      BRect tf = this->TabFrame(i);
      if (t->CloseRect(tf).Contains(where)) return i;
    }
    return -1;
  }

  /* Same as BTabView::TabFrame B_WIDTH_FROM_LABEL, plus per-tab icon+close extras. */
  BRect TabFrame(int32 index) const override
  {
    if (index >= CountTabs() || index < 0) return BRect();
    const float pad = ceilf(be_control_look->DefaultLabelSpacing() * 3.3f);
    const float height = TabHeight();
    const float offset = BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING);
    const BRect bounds(Bounds());
    tab_side side = TabSide();

    float x = 0.0f;
    for (int32 i = 0; i <= index; i++)
    {
      IupHaikuTab* t = dynamic_cast<IupHaikuTab*>(TabAt(i));
      float extra = t ? t->ExtraWidth() : 0.0f;
      float w = StringWidth(TabAt(i)->Label()) + pad + extra;
      if (i == index)
      {
        switch (side)
        {
          case kTopSide:    return BRect(offset + x, 0.0f, offset + x + w, height);
          case kBottomSide: return BRect(offset + x, bounds.bottom - height, offset + x + w, bounds.bottom);
          case kLeftSide:   return BRect(0.0f, offset + x, height, offset + x + w);
          case kRightSide:  return BRect(bounds.right - height, offset + x, bounds.right, offset + x + w);
        }
      }
      x += w;
    }
    return BRect();
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
  int fHotCloseTab;
  int fHotTipTab;
  int fDragSource;
  int fDragTarget;
  BPoint fDragStart;
  bool fDragging;
  bool fSuppressSelectCallbacks;
};


static BBitmap* haikuTabsResolveImage(Ihandle* ih, const char* name)
{
  if (!name) return NULL;
  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  return (BBitmap*)iupImageGetImage(name, ih, 0, bgcolor);
}

static void haikuTabsAssignIcon(IupHaikuTab* tab, Ihandle* ih, BBitmap* bm)
{
  if (!bm)
  {
    tab->SetIcon(NULL, 0, 0);
    return;
  }
  int raw_w = (int)(bm->Bounds().Width() + 1);
  int raw_h = (int)(bm->Bounds().Height() + 1);
  int dst_w = raw_w, dst_h = raw_h;
  iupTabsScaleImageSize(ih, raw_w, raw_h, &dst_w, &dst_h);
  tab->SetIcon(bm, dst_w, dst_h);
}

/* Native BTabView strip height = font + 8; grow it to fit the tallest
   tab icon so scaled images don't clip vertically. */
static void haikuTabsUpdateStripHeight(IupHaikuTabView* tabs)
{
  if (!tabs) return;
  font_height fh;
  tabs->GetFontHeight(&fh);
  float natural = ceilf(fh.ascent + fh.descent + fh.leading + 8.0f);
  float wanted = natural;
  int n = tabs->CountTabs();
  for (int i = 0; i < n; i++)
  {
    IupHaikuTab* t = dynamic_cast<IupHaikuTab*>(tabs->TabAt(i));
    if (!t) continue;
    float h = t->IconHeight() + 8.0f;
    if (h > wanted) wanted = h;
  }
  if (wanted != tabs->TabHeight())
    tabs->SetTabHeight(wanted);
}

static void haikuTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING")) return;
  if (!ih->handle) return;

  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  int pos = IupGetChildPos(ih, child);

  BView* page = new BView(BRect(0, 0, 0, 0), "iup_tab_page", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
  page->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

  IupHaikuTab* tab = new IupHaikuTab();
  tab->SetView(page);

  char* title = iupAttribGetId(ih, "TABTITLE", pos);
  if (!title) title = iupAttribGet(child, "TABTITLE");
  if (title) tab->SetLabel(title);

  char* image = iupAttribGetId(ih, "TABIMAGE", pos);
  if (!image) image = iupAttribGet(child, "TABIMAGE");
  if (image) haikuTabsAssignIcon(tab, ih, haikuTabsResolveImage(ih, image));

  if (ih->data->show_close) tab->SetShowClose(true);

  LooperLockGuard guard(tabs->Looper());
  tabs->AddTab(page, tab);
  haikuTabsUpdateStripHeight(tabs);

  iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)page);
}

static void haikuTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING")) return;
  if (!ih->handle) return;

  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  LooperLockGuard guard(tabs->Looper());

  /* Hidden via TABCLOSE_CB IUP_DEFAULT: BTab was RemoveTab'd, just delete the stash. */
  BTab* hidden = (BTab*)iupAttribGet(child, "_IUPHAIKU_HIDDEN_TAB");
  if (hidden)
  {
    delete hidden;
    iupAttribSet(child, "_IUPHAIKU_HIDDEN_TAB", NULL);
    iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
    return;
  }

  /* `child` is already unlinked here; walk `pos` siblings forward to find its old slot. */
  int hidden_before = 0;
  int i = 0;
  for (Ihandle* c = ih->firstchild; c && i < pos; c = c->brother, i++)
    if (iupAttribGet(c, "_IUPHAIKU_HIDDEN_TAB")) hidden_before++;

  int btab_pos = pos - hidden_before;
  BTab* tab = tabs->RemoveTab(btab_pos);
  delete tab;

  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
}

/* Map */

static int haikuTabsMapMethod(Ihandle* ih)
{
  IupHaikuTabView* tabs = new IupHaikuTabView(ih);
  ih->handle = (InativeHandle*)tabs;
  iuphaikuAddToParent(ih);

  /* Keep every page attached so FOLLOW propagation works in unselected tabs. */
  if (tabs->ContainerView() && !tabs->ContainerView()->GetLayout())
    tabs->ContainerView()->SetLayout(new BCardLayout());

  BTabView::tab_side side;
  switch (ih->data->type)
  {
    case ITABS_BOTTOM: side = BTabView::kBottomSide; break;
    case ITABS_LEFT:   side = BTabView::kLeftSide;   break;
    case ITABS_RIGHT:  side = BTabView::kRightSide;  break;
    default:           side = BTabView::kTopSide;    break;
  }
  {
    LooperLockGuard guard(tabs->Looper());
    tabs->SetTabSide(side);
  }

  for (Ihandle* c = ih->firstchild; c; c = c->brother)
    haikuTabsChildAddedMethod(ih, c);

  return IUP_NOERROR;
}

static void haikuTabsUnMapMethod(Ihandle* ih)
{
  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  if (tabs) tabs->SetIhandle(NULL);
  iupdrvBaseUnMapMethod(ih);
}

/* Driver hooks */

extern "C" IUP_SDK_API int iupdrvTabsExtraDecor(Ihandle* /*ih*/) { return 0; }
extern "C" IUP_SDK_API int iupdrvTabsExtraMargin(void) { return 0; }
extern "C" IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* /*child*/, int /*pos*/) { return 1; }
extern "C" IUP_SDK_API int iupdrvTabsGetLineCountAttrib(Ihandle* /*ih*/) { return 1; }

extern "C" IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  if (!tabs) return;
  LooperLockGuard guard(tabs->Looper());
  tabs->Select(pos);
}

extern "C" IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  if (!tabs) return -1;
  return tabs->Selection();
}

extern "C" IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int text_w = 0, text_h = 0;
  if (tab_title)
  {
    text_w = iupdrvFontGetStringWidth(ih, tab_title);
    iupdrvFontGetCharSize(ih, NULL, &text_h);
  }
  if (tab_image)
  {
    int iw = 0, ih_ = 0;
    iupImageGetInfo(tab_image, &iw, &ih_, NULL);
    iupTabsScaleImageSize(ih, iw, ih_, &iw, &ih_);
    text_w += iw + (tab_title ? IupHaikuTab::kIconPad : 0);
    if (ih_ > text_h) text_h = ih_;
  }
  if (ih->data->show_close)
    text_w += IupHaikuTab::kCloseSize + IupHaikuTab::kClosePad;

  float spacing = be_control_look->DefaultLabelSpacing();
  int long_pad = (int)ceilf(spacing * 3.3f);    /* per-tab padding on the stack axis */
  int strip_pad = (int)ceilf(spacing * 1.3f);   /* fTabHeight extra over the font   */

  if (ih->data->type == ITABS_LEFT || ih->data->type == ITABS_RIGHT)
  {
    if (tab_width)  *tab_width  = text_h + strip_pad;
    if (tab_height) *tab_height = text_w + long_pad;
  }
  else
  {
    if (tab_width)  *tab_width  = text_w + long_pad;
    if (tab_height) *tab_height = text_h;
  }
}

static int haikuTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) return 0;  /* pre-Map only */
  if      (iupStrEqualNoCase(value, "BOTTOM")) ih->data->type = ITABS_BOTTOM;
  else if (iupStrEqualNoCase(value, "LEFT"))   ih->data->type = ITABS_LEFT;
  else if (iupStrEqualNoCase(value, "RIGHT"))  ih->data->type = ITABS_RIGHT;
  else                                          ih->data->type = ITABS_TOP;
  return 0;
}

static int haikuTabsSetTabOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) return 0;
  if (iupStrEqualNoCase(value, "VERTICAL")) ih->data->orientation = ITABS_VERTICAL;
  else                                      ih->data->orientation = ITABS_HORIZONTAL;
  return 0;
}

static int haikuTabsSetTabTitleAttribId(Ihandle* ih, int i, const char* value)
{
  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  if (!tabs) return 1;
  LooperLockGuard guard(tabs->Looper());
  BTab* tab = tabs->TabAt(i);
  if (tab) { tab->SetLabel(value ? value : ""); tabs->Invalidate(); }
  return 1;
}

static int haikuTabsSetTabImageAttribId(Ihandle* ih, int i, const char* value)
{
  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  if (!tabs) return 1;
  IupHaikuTab* tab = dynamic_cast<IupHaikuTab*>(tabs->TabAt(i));
  if (!tab) return 1;
  LooperLockGuard guard(tabs->Looper());
  haikuTabsAssignIcon(tab, ih, haikuTabsResolveImage(ih, value));
  haikuTabsUpdateStripHeight(tabs);
  tabs->Invalidate();
  return 1;
}

static char* haikuTabsGetShowCloseAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->show_close);
}

static int haikuTabsSetShowCloseAttrib(Ihandle* ih, const char* value)
{
  int v = iupStrBoolean(value);
  ih->data->show_close = v ? 1 : 0;

  IupHaikuTabView* tabs = (IupHaikuTabView*)ih->handle;
  if (!tabs) return 0;
  LooperLockGuard guard(tabs->Looper());
  for (int32 i = 0; i < tabs->CountTabs(); i++)
    if (IupHaikuTab* t = dynamic_cast<IupHaikuTab*>(tabs->TabAt(i)))
      t->SetShowClose(v != 0);
  tabs->Invalidate();
  return 0;
}

extern "C" IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = haikuTabsMapMethod;
  ic->UnMap = haikuTabsUnMapMethod;
  ic->ChildAdded = haikuTabsChildAddedMethod;
  ic->ChildRemoved = haikuTabsChildRemovedMethod;

  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");
  iupClassRegisterCallback(ic, "REORDER_CB", "ii");

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, haikuTabsSetTabTitleAttribId, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, haikuTabsSetTabImageAttribId, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTIP", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, haikuTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* LEFT/RIGHT is always rotated (BeOS); TOP/BOTTOM is always horizontal. */
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, haikuTabsSetTabOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCLOSE", haikuTabsGetShowCloseAttrib, haikuTabsSetShowCloseAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}

/** \file
 * \brief Haiku Tree (BOutlineListView)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <map>
#include <vector>

#include <Bitmap.h>
#include <ControlLook.h>
#include <Font.h>
#include <MimeType.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <ListItem.h>
#include <Looper.h>
#include <Message.h>
#include <OutlineListView.h>
#include <Point.h>
#include <Rect.h>
#include <ScrollView.h>
#include <String.h>
#include <TextView.h>
#include <View.h>

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
#include "iup_tree.h"
}

#include "iuphaiku_drv.h"


/* Custom BListItem holding the IUP per-node state. Each item knows its kind
 * (branch/leaf), its title, its optional images and per-node colors so DrawItem
 * can render without a hash lookup back to attributes. */
static int haikuTreeExtraItemHeight(BView* owner);

class IupHaikuTreeItem : public BListItem
{
public:
  IupHaikuTreeItem(uint32 level, int kind, const char* title)
    : BListItem(level, true), fKind(kind), fTitle(title ? title : ""),
      fImage(NULL), fImageExpanded(NULL),
      fHasFgColor(false), fHasBgColor(false), fHasFont(false), fFont(*be_plain_font),
      fShowToggle(0), fToggleValue(0), fToggleVisible(true), fFocused(false)
  {
    fFgColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
    fBgColor = ui_color(B_LIST_BACKGROUND_COLOR);
  }

  int Kind() const { return fKind; }

  const char* Title() const { return fTitle.String(); }
  void SetTitle(const char* t) { fTitle = t ? t : ""; }

  void SetImage(BBitmap* b) { fImage = b; }
  BBitmap* Image() const { return fImage; }
  void SetImageExpanded(BBitmap* b) { fImageExpanded = b; }
  BBitmap* ImageExpanded() const { return fImageExpanded; }

  void SetFgColor(rgb_color c) { fFgColor = c; fHasFgColor = true; }
  void ClearFgColor() { fHasFgColor = false; }
  bool HasFgColor() const { return fHasFgColor; }
  rgb_color FgColor() const { return fFgColor; }
  void SetBgColor(rgb_color c) { fBgColor = c; fHasBgColor = true; }
  bool HasBgColor() const { return fHasBgColor; }
  rgb_color BgColor() const { return fBgColor; }

  void SetFont(const BFont& f) { fFont = f; fHasFont = true; }
  bool HasFont() const { return fHasFont; }
  const BFont& Font() const { return fFont; }

  void SetShowToggle(int m) { fShowToggle = m; }
  int ShowToggle() const { return fShowToggle; }
  void SetToggleValue(int v) { fToggleValue = v; }
  int ToggleValue() const { return fToggleValue; }
  void SetToggleVisible(bool v) { fToggleVisible = v; }
  bool ToggleVisible() const { return fToggleVisible; }
  BRect ToggleBox() const { return fToggleBox; }

  void SetFocused(bool f) { fFocused = f; }
  bool IsFocused() const { return fFocused; }

  /* Width this item draws into, used for h-scrollbar autohide. */
  float DrawWidth(BView* owner) const
  {
    float left = OutlineLevel() * be_control_look->DefaultItemSpacing()
               + be_plain_font->Size();
    float w = 2;
    if (fShowToggle) w += ceilf(be_plain_font->Size() + 2) + 4;
    BBitmap* img = (fKind == ITREE_BRANCH && IsExpanded() && fImageExpanded)
                     ? fImageExpanded : fImage;
    if (img) w += img->Bounds().Width() + 1 + 4;
    BFont f;
    if (fHasFont) f = fFont; else owner->GetFont(&f);
    w += f.StringWidth(fTitle.String());
    return left + w + 4;
  }

  void DrawItem(BView* owner, BRect frame, bool /*complete*/) override
  {
    rgb_color bg = fHasBgColor ? fBgColor : ui_color(B_LIST_BACKGROUND_COLOR);
    rgb_color fg = fHasFgColor ? fFgColor : ui_color(B_LIST_ITEM_TEXT_COLOR);

    if (IsSelected())
    {
      bg = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
      fg = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
    }

    owner->SetLowColor(bg);
    owner->FillRect(frame, B_SOLID_LOW);

    BBitmap* img = (fKind == ITREE_BRANCH && IsExpanded() && fImageExpanded)
                     ? fImageExpanded : fImage;

    float x = frame.left + 2;
    if (fShowToggle)
    {
      float boxSize = ceilf(be_plain_font->Size() + 2);
      if (fToggleVisible)
      {
        float by = frame.top + ((frame.Height() - boxSize) / 2);
        BRect cb(x, by, x + boxSize, by + boxSize);
        fToggleBox = cb;
        uint32 flags = 0;
        if (fToggleValue == 1) flags |= BControlLook::B_ACTIVATED;
        else if (fToggleValue == -1) flags |= BControlLook::B_PARTIALLY_ACTIVATED;
        BRect drawRect = cb;
        be_control_look->DrawCheckBox(owner, drawRect, cb, bg, flags);
      }
      else
        fToggleBox = BRect();
      x += boxSize + 4;
    }
    if (img)
    {
      BRect ib = img->Bounds();
      float iy = frame.top + ((frame.Height() - ib.Height()) / 2);
      owner->SetDrawingMode(B_OP_ALPHA);
      owner->DrawBitmap(img, BPoint(x, iy));
      owner->SetDrawingMode(B_OP_COPY);
      x += ib.Width() + 4;
    }

    owner->SetHighColor(IsEnabled() ? fg : tint_color(fg, B_DISABLED_LABEL_TINT));
    if (fHasFont) owner->SetFont(&fFont);
    font_height fh;
    owner->GetFontHeight(&fh);
    float ty = frame.top + ((frame.Height() - (fh.ascent + fh.descent)) / 2) + fh.ascent;
    owner->DrawString(fTitle.String(), BPoint(x, ty));

    if (fFocused)
    {
      owner->SetHighColor(keyboard_navigation_color());
      BRect fr = frame;
      fr.InsetBy(1, 1);
      owner->StrokeRect(fr);
    }
  }

  void Update(BView* owner, const BFont* font) override
  {
    BListItem::Update(owner, fHasFont ? &fFont : font);
    if (fImage)
    {
      float ih = fImage->Bounds().Height() + 2;
      if (ih > Height()) SetHeight(ih);
    }
    int extra = haikuTreeExtraItemHeight(owner);
    if (extra > 0) SetHeight(Height() + extra);
  }

private:
  int fKind;
  BString fTitle;
  BBitmap* fImage;
  BBitmap* fImageExpanded;
  rgb_color fFgColor;
  rgb_color fBgColor;
  bool fHasFgColor;
  bool fHasBgColor;
  bool fHasFont;
  BFont fFont;
  int fShowToggle;
  int fToggleValue;
  bool fToggleVisible;
  bool fFocused;
  mutable BRect fToggleBox;
};

static void haikuTreeSetFocus(Ihandle* ih, int id);


/* BScrollView always shows both bars; hide them when content fits. */
class IupHaikuTreeScrollView : public BScrollView
{
public:
  IupHaikuTreeScrollView(const char* name, BView* target, uint32 resizingMode, uint32 flags, bool h, bool v, border_style border)
    : BScrollView(name, target, resizingMode, flags, h, v, border) {}

  void FrameResized(float w, float h) override
  {
    BScrollView::FrameResized(w, h);
    RelayoutChildren();
  }

  void RelayoutChildren()
  {
    BView* t = Target();
    if (!t) return;
    BScrollBar* vsb = ScrollBar(B_VERTICAL);
    BScrollBar* hsb = ScrollBar(B_HORIZONTAL);

    float vsb_w = vsb ? vsb->PreferredSize().Width()  : 0;
    float hsb_h = hsb ? hsb->PreferredSize().Height() : 0;

    BRect avail = Bounds();
    if (Border() != B_NO_BORDER) avail.InsetBy(1, 1);

    float content_h = 0, content_w = 0;
    if (BOutlineListView* olv = dynamic_cast<BOutlineListView*>(t))
    {
      /* BListView::ItemAt / CountItems return the visible-only flat list. */
      int n = olv->CountItems();
      for (int i = 0; i < n; ++i)
      {
        BListItem* it = olv->ItemAt(i);
        if (!it) continue;
        content_h += it->Height();
        IupHaikuTreeItem* tit = dynamic_cast<IupHaikuTreeItem*>(it);
        if (tit)
        {
          float w = tit->DrawWidth(t);
          if (w > content_w) content_w = w;
        }
      }
    }

    bool need_v = content_h > avail.Height() + 1;
    bool need_h = content_w > avail.Width() - (need_v ? vsb_w : 0) + 1;
    if (need_h) need_v = content_h > avail.Height() - hsb_h + 1;

    /* BView::Hide() is reference-counted; only toggle on actual state change. */
    if (hsb)
    {
      bool hidden = hsb->IsHidden();
      if (need_h && hidden) hsb->Show();
      else if (!need_h && !hidden) hsb->Hide();
    }
    if (vsb)
    {
      bool hidden = vsb->IsHidden();
      if (need_v && hidden) vsb->Show();
      else if (!need_v && !hidden) vsb->Hide();
    }

    BRect inner = avail;
    if (need_v) inner.right  -= vsb_w;
    if (need_h) inner.bottom -= hsb_h;
    t->MoveTo(inner.LeftTop()); t->ResizeTo(inner.Width(), inner.Height());

    if (hsb && need_h)
    {
      BRect r = inner;
      r.top = r.bottom + 1; r.bottom = r.top + hsb_h;
      if (Border() != B_NO_BORDER || need_v) r.right++;
      if (Border() != B_NO_BORDER) r.left--;
      hsb->MoveTo(r.LeftTop()); hsb->ResizeTo(r.Width(), r.Height());
    }
    if (vsb && need_v)
    {
      BRect r = inner;
      r.left = r.right + 1; r.right = r.left + vsb_w;
      if (Border() != B_NO_BORDER || need_h) r.bottom++;
      if (Border() != B_NO_BORDER) r.top--;
      vsb->MoveTo(r.LeftTop()); vsb->ResizeTo(r.Width(), r.Height());
    }
  }
};


class IupHaikuTreeView;

/* In-place rename editor. BTextView so Enter/Escape land in our KeyDown directly. */
class IupHaikuTreeEditor : public BTextView
{
public:
  IupHaikuTreeEditor(BRect r, IupHaikuTreeView* tv, int id, const char* val);
  void KeyDown(const char* bytes, int32 numBytes) override;
  void MakeFocus(bool focused = true) override;

  IupHaikuTreeView* fTv;
  int fId;
  bool fEnded;
};

class IupHaikuTreeView : public BOutlineListView
{
public:
  IupHaikuTreeView(Ihandle* ih)
    : BOutlineListView(BRect(0, 0, 0, 0), "iup_tree", B_SINGLE_SELECTION_LIST),
      fIhandle(ih), fEditor(NULL) {}

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }
  Ihandle* GetIhandle() const { return fIhandle; }

  void StartRename(int id);
  void EndRename(bool apply);

  BRect LatchRect(BRect itemRect, int32 level) const override
  {
    int indent = fIhandle ? iupAttribGetInt(fIhandle, "INDENTATION") : 0;
    if (indent <= 0)
      return BOutlineListView::LatchRect(itemRect, level);

    float latchWidth = be_plain_font->Size();
    float latchHeight = be_plain_font->Size();
    float heightOffset = itemRect.Height() / 2 - latchHeight / 2;
    return BRect(0, 0, latchWidth, latchHeight)
             .OffsetBySelf(itemRect.left, itemRect.top)
             .OffsetBySelf((float)(level * indent), heightOffset);
  }

  void SelectionChanged() override
  {
    BOutlineListView::SelectionChanged();
    if (!fIhandle) return;
    if (iupAttribGet(fIhandle, "_IUPTREE_IGNORE_SELECTION_CB")) return;
    if (!iupdrvIsActive(fIhandle))
    {
      iupAttribSet(fIhandle, "_IUPTREE_IGNORE_SELECTION_CB", (char*)"1");
      DeselectAll();
      iupAttribSet(fIhandle, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
      return;
    }

    int32 idx = CurrentSelection(0);
    if (idx < 0) return;
    BListItem* item = ItemAt(idx);
    if (!item) return;
    int id = iupTreeFindNodeId(fIhandle, (InodeHandle*)item);
    if (id < 0) return;

    haikuTreeSetFocus(fIhandle, id);

    IFnii cb = (IFnii)IupGetCallback(fIhandle, "SELECTION_CB");
    if (cb) cb(fIhandle, id, 1);
  }

protected:
  void ExpandOrCollapse(BListItem* super, bool expand) override
  {
    if (!fIhandle)
    {
      BOutlineListView::ExpandOrCollapse(super, expand);
      return;
    }

    int id = iupTreeFindNodeId(fIhandle, (InodeHandle*)super);
    Icallback cbgen = expand
                        ? IupGetCallback(fIhandle, "BRANCHOPEN_CB")
                        : IupGetCallback(fIhandle, "BRANCHCLOSE_CB");
    if (cbgen && id >= 0)
    {
      IFni cb = (IFni)cbgen;
      if (cb(fIhandle, id) == IUP_IGNORE) return;
    }

    BOutlineListView::ExpandOrCollapse(super, expand);

    /* Swap branch image when collapsed/expanded image differs. */
    IupHaikuTreeItem* it = dynamic_cast<IupHaikuTreeItem*>(super);
    if (it) InvalidateItem(IndexOf(super));
  }

  void DrawLatch(BRect itemRect, int32 level, bool collapsed, bool highlighted, bool misTracked) override
  {
    if (fIhandle && iupAttribGetBoolean(fIhandle, "HIDEBUTTONS"))
      return;
    BOutlineListView::DrawLatch(itemRect, level, collapsed, highlighted, misTracked);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (fIhandle && iuphaikuDnDMessageReceived(fIhandle, this, msg)) return;
    BOutlineListView::MessageReceived(msg);
  }

public:
  void MouseDown(BPoint where) override
  {
    if (!fIhandle) { BOutlineListView::MouseDown(where); return; }

    BMessage* msg = Looper() ? Looper()->CurrentMessage() : NULL;
    int32 buttons = 0, clicks = 1;
    if (msg)
    {
      msg->FindInt32("buttons", &buttons);
      msg->FindInt32("clicks", &clicks);
    }

    int32 idx = IndexOf(where);
    if (idx >= 0)
    {
      BListItem* item = ItemAt(idx);
      IupHaikuTreeItem* tit = dynamic_cast<IupHaikuTreeItem*>(item);
      int id = item ? iupTreeFindNodeId(fIhandle, (InodeHandle*)item) : -1;

      /* toggle checkbox click */
      if (tit && tit->ShowToggle() && tit->ToggleVisible() && id >= 0
          && !(buttons & B_SECONDARY_MOUSE_BUTTON) && tit->ToggleBox().Contains(where))
      {
        int newval = (tit->ToggleValue() == 1) ? 0 : 1;
        tit->SetToggleValue(newval);
        InvalidateItem(idx);
        IFnii cb = (IFnii)IupGetCallback(fIhandle, "TOGGLEVALUE_CB");
        if (cb) cb(fIhandle, id, newval);
        if (iupAttribGetBoolean(fIhandle, "MARKWHENTOGGLE"))
          Select(idx, false);
        return;
      }

      if (buttons & B_SECONDARY_MOUSE_BUTTON)
      {
        if (id >= 0)
        {
          IFni cb = (IFni)IupGetCallback(fIhandle, "RIGHTCLICK_CB");
          if (cb) cb(fIhandle, id);
        }
        /* If the callback popped a rename editor, don't steal its focus. */
        if (!fEditor) BOutlineListView::MouseDown(where);
        return;
      }

      if (clicks >= 2 && tit && id >= 0)
      {
        const char* cb_name = (tit->Kind() == ITREE_LEAF) ? "EXECUTELEAF_CB" : "EXECUTEBRANCH_CB";
        IFni cb = (IFni)IupGetCallback(fIhandle, cb_name);
        if (cb) cb(fIhandle, id);
      }
    }

    BOutlineListView::MouseDown(where);
  }

  bool InitiateDrag(BPoint where, int32 /*index*/, bool /*wasSelected*/) override
  {
    if (!fIhandle) return false;
    if (iupAttribGetBoolean(fIhandle, "DRAGSOURCE"))
      return iuphaikuDnDInitiateDrag(fIhandle, this, where);
    return false;
  }

  void KeyDown(const char* bytes, int32 numBytes) override
  {
    if (fIhandle && fIhandle->data->show_rename && numBytes == 1)
    {
      BMessage* msg = Looper() ? Looper()->CurrentMessage() : NULL;
      int32 raw = 0;
      if (msg) msg->FindInt32("raw_char", &raw);
      if ((unsigned char)bytes[0] == B_FUNCTION_KEY && raw == B_F2_KEY)
      {
        int32 sel = CurrentSelection(0);
        if (sel >= 0)
        {
          BListItem* item = ItemAt(sel);
          int id = item ? iupTreeFindNodeId(fIhandle, (InodeHandle*)item) : -1;
          if (id >= 0)
          {
            IFni cb = (IFni)IupGetCallback(fIhandle, "SHOWRENAME_CB");
            int ret = cb ? cb(fIhandle, id) : IUP_DEFAULT;
            if (ret != IUP_IGNORE) StartRename(id);
          }
        }
        return;
      }
    }
    BOutlineListView::KeyDown(bytes, numBytes);
  }

  void MakeFocus(bool state = true) override
  {
    BOutlineListView::MakeFocus(state);
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, state ? 1 : 0);
  }

private:
  Ihandle* fIhandle;
  IupHaikuTreeEditor* fEditor;
};


IupHaikuTreeEditor::IupHaikuTreeEditor(BRect r, IupHaikuTreeView* tv, int id, const char* val)
  : BTextView(r, "iup_tree_edit", BRect(2.0f, 2.0f, r.Width() - 2.0f, r.Height() - 2.0f),
              B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE),
    fTv(tv), fId(id), fEnded(false)
{
  SetText(val ? val : "");
  SetWordWrap(false);
  SelectAll();
}

void IupHaikuTreeEditor::KeyDown(const char* bytes, int32 numBytes)
{
  if (!fEnded && numBytes >= 1)
  {
    if (bytes[0] == B_ESCAPE) { if (fTv) fTv->EndRename(false); return; }
    if (bytes[0] == B_RETURN) { if (fTv) fTv->EndRename(true);  return; }
  }
  BTextView::KeyDown(bytes, numBytes);
}

void IupHaikuTreeEditor::MakeFocus(bool focused)
{
  BTextView::MakeFocus(focused);
  if (!focused && !fEnded && fTv) fTv->EndRename(true);
}

void IupHaikuTreeView::StartRename(int id)
{
  if (!fIhandle || id < 0) return;
  Ihandle* ih = fIhandle;
  if (fEditor) EndRename(true);
  /* RENAME_CB inside EndRename may have destroyed the tree, freeing `this`. */
  if (!iupObjectCheck(ih)) return;

  IupHaikuTreeItem* item = (IupHaikuTreeItem*)
    (id < ih->data->node_count ? ih->data->node_cache[id].node_handle : NULL);
  if (!item) return;

  int32 idx = IndexOf(item);
  if (idx < 0) return;
  BRect frame = ItemFrame(idx);
  /* ItemFrame is raw; BOutlineListView::DrawItem shifts left by LatchRect.right + our 2px + icon width. */
  frame.left = LatchRect(frame, item->OutlineLevel()).right + 2;
  BBitmap* img = item->Image();
  if (img) frame.left += img->Bounds().Width() + 1 + 4;
  fEditor = new IupHaikuTreeEditor(frame, this, id, item->Title());
  AddChild(fEditor);
  fEditor->MakeFocus(true);
}

void IupHaikuTreeView::EndRename(bool apply)
{
  if (!fEditor || fEditor->fEnded) return;
  fEditor->fEnded = true;

  Ihandle* ih = fIhandle;
  int id = fEditor->fId;
  BString text(fEditor->Text());

  bool save = apply && ih && id < ih->data->node_count &&
              ih->data->node_cache[id].node_handle;
  if (save)
  {
    IFnis cb = (IFnis)IupGetCallback(ih, "RENAME_CB");
    if (cb && cb(ih, id, (char*)text.String()) == IUP_IGNORE) save = false;
    /* Callback may have destroyed the tree or removed nodes; re-lookup item below. */
    if (!iupObjectCheck(ih)) return;
  }

  IupHaikuTreeItem* item = (ih && id < ih->data->node_count)
    ? (IupHaikuTreeItem*)ih->data->node_cache[id].node_handle : NULL;
  if (save && item)
  {
    item->SetTitle(text.String());
    InvalidateItem(IndexOf(item));
  }

  fEditor->RemoveSelf();
  delete fEditor;
  fEditor = NULL;
  MakeFocus(true);
}

static IupHaikuTreeView* haikuTreeGetView(Ihandle* ih)
{
  if (!ih || !ih->handle) return NULL;
  return (IupHaikuTreeView*)iupAttribGet(ih, "_IUPHAIKU_TREE_INNER");
}

static int haikuTreeExtraItemHeight(BView* owner)
{
  IupHaikuTreeView* tv = dynamic_cast<IupHaikuTreeView*>(owner);
  if (!tv) return 0;
  Ihandle* ih = tv->GetIhandle();
  return ih ? 2 * ih->data->spacing : 0;
}

static char* haikuTreeGetIndentationAttrib(Ihandle* ih)
{
  int indent = iupAttribGetInt(ih, "INDENTATION");
  if (indent <= 0)
    indent = (int)be_control_look->DefaultItemSpacing();
  return iupStrReturnInt(indent);
}

static int haikuTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (tv)
  {
    LooperLockGuard guard(tv->Looper());
    tv->Invalidate();
  }
  return 1; /* store; LatchRect reads it back */
}

static int haikuTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);
  if (ih->data->spacing < 0) ih->data->spacing = 0;

  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (tv)
  {
    LooperLockGuard guard(tv->Looper());
    BFont f; tv->GetFont(&f); tv->SetFont(&f); /* re-runs item Update to recompute heights */
    tv->Invalidate();
    return 0;
  }
  return 1; /* store until mapped */
}

static IupHaikuTreeItem* haikuTreeGetItem(Ihandle* ih, int id)
{
  return (IupHaikuTreeItem*)iupTreeGetNode(ih, id);
}

static void haikuTreeSetFocus(Ihandle* ih, int id)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  int old_id = iupAttribGetInt(ih, "_IUPHAIKU_FOCUS_ID");
  IupHaikuTreeItem* it;

  if (!tv) return;

  if (old_id != id && old_id >= 0 && (it = haikuTreeGetItem(ih, old_id)) != NULL)
  {
    it->SetFocused(false);
    int idx = tv->IndexOf(it);
    if (idx >= 0) tv->InvalidateItem(idx);
  }
  iupAttribSetInt(ih, "_IUPHAIKU_FOCUS_ID", id);
  if (id >= 0 && (it = haikuTreeGetItem(ih, id)) != NULL)
  {
    it->SetFocused(true);
    int idx = tv->IndexOf(it);
    if (idx >= 0) tv->InvalidateItem(idx);
  }
}

static int haikuTreeGetIdFromItem(Ihandle* ih, BListItem* item)
{
  if (!item) return -1;
  return iupTreeFindNodeId(ih, (InodeHandle*)item);
}

static int haikuTreeChildCountRec(IupHaikuTreeView* tv, BListItem* it)
{
  int c = tv->CountItemsUnder(it, true);
  int total = c;
  for (int i = 0; i < c; ++i)
  {
    BListItem* child = tv->ItemUnderAt(it, true, i);
    if (child) total += haikuTreeChildCountRec(tv, child);
  }
  return total;
}

static int haikuTreeFlatPosOfNextSibling(IupHaikuTreeView* tv, IupHaikuTreeItem* prev)
{
  /* Returns the FullList position immediately after prev's whole subtree. */
  int prev_idx = tv->FullListIndexOf(prev);
  if (prev_idx < 0) return tv->FullListCountItems();
  return prev_idx + 1 + haikuTreeChildCountRec(tv, prev);
}

static void haikuTreeRemoveAndDeleteSubtree(IupHaikuTreeView* tv, BListItem* root)
{
  /* RemoveItem cascade-deletes descendants; just the root needs explicit delete. */
  tv->RemoveItem(root);
  delete root;
}

static void haikuTreeRemoveAndDeleteAll(IupHaikuTreeView* tv)
{
  /* RemoveItem cascade-deletes descendants; collect roots only. */
  BList roots;
  int n = tv->FullListCountItems();
  for (int i = 0; i < n; ++i)
  {
    BListItem* it = tv->FullListItemAt(i);
    if (it && tv->Superitem(it) == NULL) roots.AddItem(it);
  }
  for (int i = roots.CountItems() - 1; i >= 0; --i)
  {
    BListItem* it = (BListItem*)roots.ItemAt(i);
    tv->RemoveItem(it);
    delete it;
  }
}

static BBitmap* haikuTreeBitmapByName(Ihandle* ih, const char* name)
{
  if (!name) return NULL;
  return (BBitmap*)iupImageGetImage(name, ih, 0, NULL);
}

/* Pulled once from the MIME database, then cached for the process lifetime. */
static BBitmap* haikuTreeSystemMimeIcon(const char* mime)
{
  BBitmap* bm = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);
  BMimeType type(mime);
  if (type.InitCheck() != B_OK || type.GetIcon(bm, B_MINI_ICON) != B_OK)
  {
    delete bm;
    return NULL;
  }
  return bm;
}

static BBitmap* haikuTreeStockLeafIcon()
{
  static BBitmap* cached = haikuTreeSystemMimeIcon(B_FILE_MIME_TYPE);
  return cached;
}

static BBitmap* haikuTreeStockFolderIcon()
{
  static BBitmap* cached = haikuTreeSystemMimeIcon("application/x-vnd.Be-directory");
  return cached;
}

extern "C" IUP_SDK_API void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return;

  IupHaikuTreeItem* prev = NULL;
  int kindPrev = -1;

  if (id == IUP_INVALID_ID && ih->data->node_count != 0)
    id = iupTreeFindNodeId(ih, iupdrvTreeGetFocusNode(ih));

  if (id >= 0 && id < ih->data->node_count)
  {
    prev = haikuTreeGetItem(ih, id);
    if (prev) kindPrev = prev->Kind();
  }

  LooperLockGuard guard(tv->Looper());

  IupHaikuTreeItem* it = NULL;
  if (prev)
  {
    if (add && kindPrev == ITREE_BRANCH)
    {
      it = new IupHaikuTreeItem(prev->OutlineLevel() + 1, kind, title);
      tv->AddItem(it, tv->FullListIndexOf(prev) + 1);
    }
    else
    {
      it = new IupHaikuTreeItem(prev->OutlineLevel(), kind, title);
      tv->AddItem(it, haikuTreeFlatPosOfNextSibling(tv, prev));
    }
  }
  else
  {
    it = new IupHaikuTreeItem(0, kind, title);
    tv->AddItem(it, 0);
  }

  it->SetShowToggle(ih->data->show_toggle);

  if (kind == ITREE_BRANCH)
  {
    it->SetExpanded(ih->data->add_expanded ? true : false);
    if (ih->data->add_expanded && ih->data->def_image_expanded)
      it->SetImageExpanded((BBitmap*)ih->data->def_image_expanded);
    if (ih->data->def_image_collapsed)
      it->SetImage((BBitmap*)ih->data->def_image_collapsed);
  }
  else if (ih->data->def_image_leaf)
  {
    it->SetImage((BBitmap*)ih->data->def_image_leaf);
  }

  iupTreeAddToCache(ih, add, kindPrev, (InodeHandle*)prev, (InodeHandle*)it);

  if (ih->data->node_count == 1 && ih->data->mark_mode == ITREE_MARK_SINGLE)
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    tv->Select(tv->IndexOf(it), false);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  tv->Invalidate();
  if (IupHaikuTreeScrollView* sv = dynamic_cast<IupHaikuTreeScrollView*>((BScrollView*)ih->handle))
    sv->RelayoutChildren();
}

extern "C" IUP_SDK_API InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  int focus_id = iupAttribGetInt(ih, "_IUPHAIKU_FOCUS_ID");
  if (focus_id >= 0 && focus_id < ih->data->node_count)
    return ih->data->node_cache[focus_id].node_handle;
  return (ih->data->node_count > 0) ? ih->data->node_cache[0].node_handle : NULL;
}

extern "C" IUP_SDK_API int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv || !node_handle) return 0;
  LooperLockGuard guard(tv->Looper());
  return haikuTreeChildCountRec(tv, (BListItem*)node_handle);
}

extern "C" IUP_SDK_API void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return;
  LooperLockGuard guard(tv->Looper());
  tv->SetListType(ih->data->mark_mode == ITREE_MARK_MULTIPLE ? B_MULTIPLE_SELECTION_LIST : B_SINGLE_SELECTION_LIST);
}

extern "C" IUP_SDK_API void iupdrvTreeAddBorders(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  int border = 2 * 2;
  int sb = iupdrvGetScrollbarSize();
  if (w) *w += border + sb;
  if (h) *h += border;
}

static IupHaikuTreeItem* haikuTreeCopyItem(IupHaikuTreeView* dst_tv, IupHaikuTreeItem* src, uint32 level, int flat_pos)
{
  IupHaikuTreeItem* nw = new IupHaikuTreeItem(level, src->Kind(), src->Title());
  nw->SetImage(src->Image());
  nw->SetImageExpanded(src->ImageExpanded());
  if (src->HasFgColor()) nw->SetFgColor(src->FgColor());
  if (src->HasBgColor()) nw->SetBgColor(src->BgColor());
  if (src->HasFont())    nw->SetFont(src->Font());
  if (src->Kind() == ITREE_BRANCH) nw->SetExpanded(src->IsExpanded());
  dst_tv->AddItem(nw, flat_pos);
  return nw;
}

static void haikuTreeCopyChildren(IupHaikuTreeView* src_tv, IupHaikuTreeView* dst_tv, Ihandle* dst, IupHaikuTreeItem* src_parent, IupHaikuTreeItem* dst_parent)
{
  int n = src_tv->CountItemsUnder(src_parent, true);
  for (int i = 0; i < n; ++i)
  {
    IupHaikuTreeItem* src_child = (IupHaikuTreeItem*)src_tv->ItemUnderAt(src_parent, true, i);
    if (!src_child) continue;
    int flat = dst_tv->FullListIndexOf(dst_parent) + 1
             + haikuTreeChildCountRec(dst_tv, dst_parent);
    IupHaikuTreeItem* nw = haikuTreeCopyItem(dst_tv, src_child, dst_parent->OutlineLevel() + 1, flat);
    dst->data->node_count++;
    haikuTreeCopyChildren(src_tv, dst_tv, dst, src_child, nw);
  }
}

extern "C" IUP_SDK_API void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* itemSrc, InodeHandle* itemDst)
{
  IupHaikuTreeView* src_tv = haikuTreeGetView(src);
  IupHaikuTreeView* dst_tv = haikuTreeGetView(dst);
  if (!src_tv || !dst_tv) return;

  IupHaikuTreeItem* src_item = (IupHaikuTreeItem*)itemSrc;
  IupHaikuTreeItem* dst_item = (IupHaikuTreeItem*)itemDst;
  if (!src_item || !dst_item) return;

  LooperLockGuard guard(dst_tv->Looper());

  int kind_dst = dst_item->Kind();
  IupHaikuTreeItem* nw = NULL;

  if (kind_dst == ITREE_BRANCH && dst_item->IsExpanded())
  {
    int flat = dst_tv->FullListIndexOf(dst_item) + 1;
    nw = haikuTreeCopyItem(dst_tv, src_item, dst_item->OutlineLevel() + 1, flat);
  }
  else
  {
    int flat = haikuTreeFlatPosOfNextSibling(dst_tv, dst_item);
    nw = haikuTreeCopyItem(dst_tv, src_item, dst_item->OutlineLevel(), flat);
  }

  iupTreeAddToCache(dst, kind_dst == ITREE_BRANCH && dst_item->IsExpanded() ? 1 : 0,
                    kind_dst, (InodeHandle*)dst_item, (InodeHandle*)nw);

  haikuTreeCopyChildren(src_tv, dst_tv, dst, src_item, nw);
}

static int haikuTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  void* bm = iupImageGetImage(value, ih, 0, NULL);
  if (!bm && iupStrEqualNoCase(value, "IMGLEAF")) bm = haikuTreeStockLeafIcon();
  ih->data->def_image_leaf = bm;
  return 1;
}

static int haikuTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  void* bm = iupImageGetImage(value, ih, 0, NULL);
  if (!bm && iupStrEqualNoCase(value, "IMGCOLLAPSED")) bm = haikuTreeStockFolderIcon();
  ih->data->def_image_collapsed = bm;
  return 1;
}

static int haikuTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  void* bm = iupImageGetImage(value, ih, 0, NULL);
  if (!bm && iupStrEqualNoCase(value, "IMGEXPANDED")) bm = haikuTreeStockFolderIcon();
  ih->data->def_image_expanded = bm;
  return 1;
}

static int haikuTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return -1;
  int idx = tv->IndexOf(BPoint(x, y));
  if (idx < 0) return -1;
  BListItem* it = tv->ItemAt(idx);
  if (!it) return -1;
  return iupTreeFindNodeId(ih, (InodeHandle*)it);
}

static int haikuTreeMapMethod(Ihandle* ih)
{
  IupHaikuTreeView* tv = new IupHaikuTreeView(ih);

  BScrollView* sv = new IupHaikuTreeScrollView("iup_tree_scroll", tv, B_FOLLOW_NONE,
                                               0, true, true, B_FANCY_BORDER);
  ih->handle = (InativeHandle*)sv;
  iupAttribSet(ih, "_IUPHAIKU_TREE_INNER", (char*)tv);

  iuphaikuAddToParent(ih);

  iupdrvTreeUpdateMarkMode(ih);

  haikuTreeSetImageLeafAttrib(ih, iupAttribGetStr(ih, "IMAGELEAF"));
  haikuTreeSetImageBranchCollapsedAttrib(ih, iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED"));
  haikuTreeSetImageBranchExpandedAttrib(ih, iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED"));

  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)haikuTreeConvertXYToPos);
  return IUP_NOERROR;
}

static void haikuTreeUnMapMethod(Ihandle* ih)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (tv)
  {
    LooperLockGuard guard(tv->Looper());
    tv->SetIhandle(NULL);
    haikuTreeRemoveAndDeleteAll(tv);
  }
  iupAttribSet(ih, "_IUPHAIKU_TREE_INNER", NULL);
  iupdrvBaseUnMapMethod(ih);
  ih->data->node_count = 0;
}

static int haikuTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 1;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  rgb_color c = { r, g, b, 255 };
  LooperLockGuard guard(tv->Looper());
  tv->SetViewColor(c);
  tv->Invalidate();
  return 1;
}

static int haikuTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 1;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  rgb_color c = { r, g, b, 255 };
  LooperLockGuard guard(tv->Looper());
  tv->SetHighColor(c);
  tv->Invalidate();
  return 1;
}

static char* haikuTreeGetTitleAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!it) return NULL;
  return iupStrReturnStr(it->Title());
}

static int haikuTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return 0;
  LooperLockGuard guard(tv->Looper());
  it->SetTitle(value);
  tv->InvalidateItem(tv->IndexOf(it));
  return 0;
}

static char* haikuTreeGetKindAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!it) return NULL;
  return (char*)(it->Kind() == ITREE_BRANCH ? "BRANCH" : "LEAF");
}

static char* haikuTreeGetDepthAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!it) return NULL;
  return iupStrReturnInt((int)it->OutlineLevel());
}

static char* haikuTreeGetParentAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return NULL;
  BListItem* p = tv->Superitem(it);
  if (!p) return NULL;
  return iupStrReturnInt(haikuTreeGetIdFromItem(ih, p));
}

static char* haikuTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return NULL;
  return iupStrReturnInt(tv->CountItemsUnder(it, true));
}

static int haikuTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it || it->Kind() != ITREE_BRANCH) return 0;
  LooperLockGuard guard(tv->Looper());
  if (iupStrEqualNoCase(value, "EXPANDED")) tv->Expand(it);
  else                                      tv->Collapse(it);
  return 0;
}

static char* haikuTreeGetStateAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!it || it->Kind() != ITREE_BRANCH) return NULL;
  return (char*)(it->IsExpanded() ? "EXPANDED" : "COLLAPSED");
}

static int haikuTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return 0;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) { it->ClearFgColor(); }
  else { rgb_color c = { r, g, b, 255 }; it->SetFgColor(c); }
  LooperLockGuard guard(tv->Looper());
  tv->InvalidateItem(tv->IndexOf(it));
  return 0;
}

static char* haikuTreeGetToggleValueAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!ih->data->show_toggle || !it) return NULL;
  int v = it->ToggleValue();
  if (v == 1) return (char*)"ON";
  if (v == -1) return (char*)"NOTDEF";
  return (char*)"OFF";
}

static int haikuTreeSetToggleValueAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!ih->data->show_toggle || !tv || !it) return 0;
  if (iupStrEqualNoCase(value, "ON")) it->SetToggleValue(1);
  else if (iupStrEqualNoCase(value, "NOTDEF") && ih->data->show_toggle == 2) it->SetToggleValue(-1);
  else it->SetToggleValue(0);
  LooperLockGuard guard(tv->Looper());
  tv->InvalidateItem(tv->IndexOf(it));
  return 0;
}

static char* haikuTreeGetToggleVisibleAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!it) return NULL;
  return (char*)(it->ToggleVisible() ? "YES" : "NO");
}

static int haikuTreeSetToggleVisibleAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return 0;
  it->SetToggleVisible(iupStrBoolean(value) ? true : false);
  LooperLockGuard guard(tv->Looper());
  tv->InvalidateItem(tv->IndexOf(it));
  return 0;
}

static char* haikuTreeGetRootCountAttrib(Ihandle* ih)
{
  int count = 0;
  for (int i = 0; i < ih->data->node_count; i++)
  {
    IupHaikuTreeItem* it = haikuTreeGetItem(ih, i);
    if (it && it->OutlineLevel() == 0) count++;
  }
  return iupStrReturnInt(count);
}

static char* haikuTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  char* str = iupStrGetMemory(ih->data->node_count + 1);
  for (int i = 0; i < ih->data->node_count; i++)
  {
    IupHaikuTreeItem* it = haikuTreeGetItem(ih, i);
    str[i] = (it && it->IsSelected()) ? '+' : '-';
  }
  str[ih->data->node_count] = 0;
  return str;
}

static int haikuTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv || !value || ih->data->mark_mode != ITREE_MARK_MULTIPLE) return 0;
  LooperLockGuard guard(tv->Looper());
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  int len = (int)strlen(value);
  for (int i = 0; i < ih->data->node_count && i < len; i++)
  {
    IupHaikuTreeItem* it = haikuTreeGetItem(ih, i);
    int idx = it ? tv->IndexOf(it) : -1;
    if (idx < 0) continue;
    if (value[i] == '+') tv->Select(idx, true);
    else                 tv->Deselect(idx);
  }
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  return 0;
}

static int haikuTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  int id;
  if (iupStrToInt(value, &id))
    iupAttribSetInt(ih, "_IUPTREE_MARKSTART", id);
  return 0;
}

static void haikuTreeSelectRange(IupHaikuTreeView* tv, Ihandle* ih, int id1, int id2)
{
  if (id1 > id2) { int t = id1; id1 = id2; id2 = t; }
  for (int i = id1; i <= id2; i++)
  {
    IupHaikuTreeItem* it = haikuTreeGetItem(ih, i);
    int idx = it ? tv->IndexOf(it) : -1;
    if (idx >= 0) tv->Select(idx, true);
  }
}

static int haikuTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv || ih->data->mark_mode != ITREE_MARK_MULTIPLE) return 0;
  LooperLockGuard guard(tv->Looper());
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  if (iupStrEqualNoCase(value, "CLEARALL"))
    tv->DeselectAll();
  else if (iupStrEqualNoCase(value, "MARKALL"))
    haikuTreeSelectRange(tv, ih, 0, ih->data->node_count - 1);
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      IupHaikuTreeItem* it = haikuTreeGetItem(ih, i);
      int idx = it ? tv->IndexOf(it) : -1;
      if (idx < 0) continue;
      if (it->IsSelected()) tv->Deselect(idx); else tv->Select(idx, true);
    }
  }
  else if (iupStrEqualNoCase(value, "BLOCK"))
  {
    int start = iupAttribGetInt(ih, "_IUPTREE_MARKSTART");
    int cur = tv->CurrentSelection(0);
    IupHaikuTreeItem* cit = cur >= 0 ? dynamic_cast<IupHaikuTreeItem*>(tv->ItemAt(cur)) : NULL;
    haikuTreeSelectRange(tv, ih, start, cit ? iupTreeFindNodeId(ih, (InodeHandle*)cit) : start);
  }
  else if (iupStrEqualPartial(value, "INVERT")) /* INVERTid, after INVERTALL */
  {
    int id = IUP_INVALID_ID;
    iupStrToInt(&value[strlen("INVERT")], &id);
    IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
    int idx = it ? tv->IndexOf(it) : -1;
    if (idx >= 0) { if (it->IsSelected()) tv->Deselect(idx); else tv->Select(idx, true); }
  }
  else /* start-end range */
  {
    int id1, id2;
    if (iupStrToIntInt(value, &id1, &id2, '-') == 2)
      haikuTreeSelectRange(tv, ih, id1, id2);
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  return 0;
}

static int haikuTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return 0;
  it->SetImage(haikuTreeBitmapByName(ih, value));
  LooperLockGuard guard(tv->Looper());
  tv->InvalidateItem(tv->IndexOf(it));
  return 1;
}

static int haikuTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return 0;
  it->SetImageExpanded(haikuTreeBitmapByName(ih, value));
  LooperLockGuard guard(tv->Looper());
  tv->InvalidateItem(tv->IndexOf(it));
  return 1;
}

static int haikuTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return 0;
  LooperLockGuard guard(tv->Looper());
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  int idx = tv->IndexOf(it);
  if (iupStrBoolean(value)) tv->Select(idx, true);
  else                       tv->Deselect(idx);
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  return 0;
}

static char* haikuTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!tv || !it) return NULL;
  return iupStrReturnBoolean(it->IsSelected() ? 1 : 0);
}

static int haikuTreeIsAncestor(IupHaikuTreeView* tv, IupHaikuTreeItem* item, IupHaikuTreeItem* ancestor)
{
  for (BListItem* p = tv->Superitem(item); p; p = tv->Superitem(p))
    if ((IupHaikuTreeItem*)p == ancestor)
      return 1;
  return 0;
}

static int haikuTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 0;

  IupHaikuTreeItem* src = haikuTreeGetItem(ih, id);
  IupHaikuTreeItem* dst = (IupHaikuTreeItem*)iupTreeGetNodeFromString(ih, value);
  if (!src || !dst || src == dst || haikuTreeIsAncestor(tv, dst, src))
    return 0;

  iupdrvTreeDragDropCopyNode(ih, ih, (InodeHandle*)src, (InodeHandle*)dst);
  return 0;
}

static int haikuTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 0;

  IupHaikuTreeItem* src = haikuTreeGetItem(ih, id);
  IupHaikuTreeItem* dst = (IupHaikuTreeItem*)iupTreeGetNodeFromString(ih, value);
  if (!src || !dst || src == dst || haikuTreeIsAncestor(tv, dst, src))
    return 0;

  LooperLockGuard guard(tv->Looper());

  int count = 1 + haikuTreeChildCountRec(tv, src);

  /* userdata by item, so it follows the moved objects (a move preserves userdata) */
  std::map<void*, void*> udata;
  for (int i = 0; i < ih->data->node_count; i++)
    udata[(void*)ih->data->node_cache[i].node_handle] = ih->data->node_cache[i].userdata;

  /* a subtree is contiguous in the full list */
  std::vector<IupHaikuTreeItem*> items(count);
  std::vector<uint32> levels(count);
  for (int k = 0; k < count; k++)
  {
    items[k] = (IupHaikuTreeItem*)tv->FullListItemAt(id + k);
    levels[k] = items[k]->OutlineLevel();
  }

  bool as_child = (dst->Kind() == ITREE_BRANCH && dst->IsExpanded());
  int delta = (int)(as_child ? dst->OutlineLevel() + 1 : dst->OutlineLevel()) - (int)levels[0];

  /* detach bottom-up: each item is a leaf when removed, so nothing is deleted */
  for (int k = count - 1; k >= 0; k--)
    tv->RemoveItem(items[k]);

  for (int k = 0; k < count; k++)
    items[k]->SetOutlineLevel((uint32)((int)levels[k] + delta));

  int dst_idx = tv->FullListIndexOf(dst);
  int ins = as_child ? dst_idx + 1 : dst_idx + 1 + haikuTreeChildCountRec(tv, dst);
  for (int k = 0; k < count; k++)
    tv->AddItem(items[k], ins + k);

  /* node_count is unchanged by a move; rebuild the cache from the new order */
  for (int i = 0; i < ih->data->node_count; i++)
  {
    BListItem* it = tv->FullListItemAt(i);
    ih->data->node_cache[i].node_handle = (InodeHandle*)it;
    ih->data->node_cache[i].userdata = udata[(void*)it];
  }

  tv->Invalidate();
  return 0;
}

static int haikuTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 0;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    LooperLockGuard guard(tv->Looper());
    int prev_count = ih->data->node_count;
    haikuTreeRemoveAndDeleteAll(tv);
    iupTreeDelFromCache(ih, 0, prev_count);
    return 0;
  }

  if (iupStrEqualNoCase(value, "MARKED"))
  {
    LooperLockGuard guard(tv->Looper());
    for (int i = tv->FullListCountItems() - 1; i >= 0; --i)
    {
      BListItem* fi = tv->FullListItemAt(i);
      if (fi && fi->IsSelected())
      {
        int sub = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)fi);
        int fid = iupTreeFindNodeId(ih, (InodeHandle*)fi);
        haikuTreeRemoveAndDeleteSubtree(tv, fi);
        if (fid >= 0) iupTreeDelFromCache(ih, fid, sub);
      }
    }
    return 0;
  }

  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!it) return 0;

  LooperLockGuard guard(tv->Looper());

  if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    int total = haikuTreeChildCountRec(tv, it);
    int n = tv->CountItemsUnder(it, true);
    for (int i = n - 1; i >= 0; --i)
    {
      BListItem* c = tv->ItemUnderAt(it, true, i);
      if (c) haikuTreeRemoveAndDeleteSubtree(tv, c);
    }
    iupTreeDelFromCache(ih, id + 1, total);
  }
  else /* SELECTED or default */
  {
    int total = 1 + haikuTreeChildCountRec(tv, it);
    haikuTreeRemoveAndDeleteSubtree(tv, it);
    iupTreeDelFromCache(ih, id, total);
  }

  return 0;
}

static int haikuTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 0;
  LooperLockGuard guard(tv->Looper());
  bool expand = iupStrBoolean(value) ? true : false;
  int n = tv->FullListCountItems();
  for (int i = 0; i < n; ++i)
  {
    IupHaikuTreeItem* it = (IupHaikuTreeItem*)tv->FullListItemAt(i);
    if (it && it->Kind() == ITREE_BRANCH)
    {
      if (expand) tv->Expand(it);
      else        tv->Collapse(it);
    }
  }
  return 0;
}

static char* haikuTreeGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->node_count);
}

static int haikuTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 0;
  int id = 0;
  if (!iupStrToInt(value, &id)) return 0;
  IupHaikuTreeItem* it = haikuTreeGetItem(ih, id);
  if (!it) return 0;
  LooperLockGuard guard(tv->Looper());
  int idx = tv->IndexOf(it);
  if (idx >= 0) tv->ScrollTo(idx);
  return 0;
}

static int haikuTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 0;
  LooperLockGuard guard(tv->Looper());

  int n = tv->CountItems();
  int cur_id = iupAttribGetInt(ih, "_IUPHAIKU_FOCUS_ID");
  IupHaikuTreeItem* cur = haikuTreeGetItem(ih, cur_id);
  int cur_idx = cur ? tv->IndexOf(cur) : -1;
  if (cur_idx < 0) cur_idx = 0;

  BListItem* item;
  if (iupStrEqualNoCase(value, "CLEAR"))
  {
    haikuTreeSetFocus(ih, -1);
    return 0;
  }
  else if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
    item = tv->ItemAt(0);
  else if (iupStrEqualNoCase(value, "LAST"))
    item = tv->ItemAt(n - 1);
  else if (iupStrEqualNoCase(value, "NEXT"))
    item = tv->ItemAt(cur_idx + 1 < n ? cur_idx + 1 : n - 1);
  else if (iupStrEqualNoCase(value, "PREVIOUS"))
    item = tv->ItemAt(cur_idx > 0 ? cur_idx - 1 : 0);
  else if (iupStrEqualNoCase(value, "PGDN"))
    item = tv->ItemAt(cur_idx + 10 < n ? cur_idx + 10 : n - 1);
  else if (iupStrEqualNoCase(value, "PGUP"))
    item = tv->ItemAt(cur_idx > 10 ? cur_idx - 10 : 0);
  else
  {
    int id = 0;
    if (!iupStrToInt(value, &id)) return 0;
    item = haikuTreeGetItem(ih, id);
  }

  if (!item) return 0;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    int idx = tv->IndexOf(item);
    if (idx >= 0) tv->Select(idx, false);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  haikuTreeSetFocus(ih, haikuTreeGetIdFromItem(ih, item));
  int idx = tv->IndexOf(item);
  if (idx >= 0) tv->ScrollTo(idx);
  return 0;
}

static char* haikuTreeGetValueAttrib(Ihandle* ih)
{
  int focus_id = iupAttribGetInt(ih, "_IUPHAIKU_FOCUS_ID");
  if (focus_id >= 0 && focus_id < ih->data->node_count)
    return iupStrReturnInt(focus_id);
  return iupStrReturnInt(ih->data->node_count > 0 ? 0 : -1);
}

static int haikuTreeSetRenameAttrib(Ihandle* ih, const char* /*value*/)
{
  IupHaikuTreeView* tv = haikuTreeGetView(ih);
  if (!tv) return 0;
  LooperLockGuard guard(tv->Looper());
  int32 idx = tv->CurrentSelection(0);
  if (idx < 0) return 0;
  BListItem* item = tv->ItemAt(idx);
  int id = item ? iupTreeFindNodeId(ih, (InodeHandle*)item) : -1;
  if (id >= 0) tv->StartRename(id);
  return 0;
}

extern "C" IUP_SDK_API void iupdrvTreeInitClass(Iclass* ic)
{
  ic->Map = haikuTreeMapMethod;
  ic->UnMap = haikuTreeUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, haikuTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, haikuTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* General */
  iupClassRegisterAttribute(ic, "COUNT", haikuTreeGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, haikuTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, haikuTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", haikuTreeGetIndentationAttrib, haikuTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, haikuTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  /* Images */
  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, haikuTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, haikuTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, haikuTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Per-node */
  iupClassRegisterAttributeId(ic, "STATE", haikuTreeGetStateAttrib, haikuTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", haikuTreeGetDepthAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", haikuTreeGetKindAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", haikuTreeGetParentAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", haikuTreeGetChildCountAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE", haikuTreeGetTitleAttrib, haikuTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", NULL, haikuTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, haikuTreeSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, haikuTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  /* Marks / selection */
  iupClassRegisterAttributeId(ic, "MARKED", haikuTreeGetMarkedAttrib, haikuTreeSetMarkedAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", haikuTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", NULL, haikuTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", NULL, haikuTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", NULL, haikuTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", haikuTreeGetMarkedNodesAttrib, haikuTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", haikuTreeGetToggleValueAttrib, haikuTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", haikuTreeGetToggleVisibleAttrib, haikuTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", haikuTreeGetValueAttrib, haikuTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Action */
  iupClassRegisterAttribute(ic, "ADDROOT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DELNODE", NULL, haikuTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, haikuTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, haikuTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Not yet supported on Haiku */
  iupClassRegisterAttribute(ic, "RUBBERBAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDELINES", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAME", NULL, haikuTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
}

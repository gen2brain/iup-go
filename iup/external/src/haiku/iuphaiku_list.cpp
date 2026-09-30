/** \file
 * \brief Haiku List Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cmath>
#include <cstddef>
#include <cstdlib>

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <ListItem.h>
#include <ListView.h>
#include <Looper.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_list.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


#define IUPHAIKU_LIST_DROP_MSG    'IupD'   /* dropdown item invoked */
#define IUPHAIKU_LIST_TEXT_MSG    'IupE'   /* text-control modification */
#define IUPHAIKU_LIST_OPENMENU    'IupO'   /* combobox chevron clicked */
#define IUPHAIKU_LIST_REORDER_MSG 'IupL'   /* SHOWDRAGDROP internal drag */

static void haikuListReorder(Ihandle* ih, int src, int drop, int is_ctrl);

class IupHaikuDropdownItem : public BMenuItem
{
public:
  IupHaikuDropdownItem(Ihandle* ih, const char* label, BMessage* msg, BBitmap* icon = nullptr)
    : BMenuItem(label, msg), fIhandle(ih), fIcon(icon) {}

  BBitmap* Icon() const { return fIcon; }
  void SetIcon(BBitmap* b) { fIcon = b; }

protected:
  void GetContentSize(float* w, float* h) override
  {
    BMenuItem::GetContentSize(w, h);
    if (!fIcon) return;

    float img_w = fIcon->Bounds().Width() + 1;
    float img_h = fIcon->Bounds().Height() + 1;
    float text_h = h ? *h : img_h;
    float draw_w = img_w, draw_h = img_h;
    bool fit = !fIhandle || fIhandle->data->fit_image;

    if (fit && img_h > text_h)
    {
      draw_w = img_w * (text_h / img_h);
      draw_h = text_h;
    }

    if (w) *w += draw_w + 6;
    if (h && draw_h > *h) *h = draw_h;
  }

  void DrawContent() override
  {
    BMenu* menu = Menu();
    if (!menu) { BMenuItem::DrawContent(); return; }

    if (fIcon)
    {
      BPoint pt = ContentLocation();
      float row_h = Frame().Height() + 1;
      float img_w = fIcon->Bounds().Width() + 1;
      float img_h = fIcon->Bounds().Height() + 1;
      float draw_w = img_w, draw_h = img_h;
      bool fit = !fIhandle || fIhandle->data->fit_image;

      if (fit && img_h > row_h)
      {
        draw_w = img_w * (row_h / img_h);
        draw_h = row_h;
      }

      float ix = pt.x;
      float iy = pt.y + (Frame().Height() - draw_h) / 2;
      menu->SetDrawingMode(B_OP_ALPHA);
      menu->DrawBitmap(fIcon, BRect(ix, iy, ix + draw_w - 1, iy + draw_h - 1));
      menu->SetDrawingMode(B_OP_COPY);
      menu->MovePenTo(pt.x + draw_w + 6, menu->PenLocation().y);
    }
    BMenuItem::DrawContent();
  }

private:
  Ihandle* fIhandle;
  BBitmap* fIcon;
};


class IupHaikuListItem : public BStringItem
{
public:
  IupHaikuListItem(Ihandle* ih, int pos, const char* text, BBitmap* icon = nullptr)
    : BStringItem(text), fIhandle(ih), fPos(pos), fIcon(icon) {}

  BBitmap* Icon() const { return fIcon; }
  void SetIcon(BBitmap* b) { fIcon = b; }
  void SetPos(int p) { fPos = p; }

  void DrawItem(BView* owner, BRect frame, bool complete) override
  {
    /* IUP positions are 1-based. */
    if (fIhandle && iupListIsVirtual(fIhandle))
    {
      const char* virt_text = iupListGetItemValueCb(fIhandle, fPos + 1);
      if (virt_text) SetText(virt_text);
      const char* virt_img = iupListGetItemImageCb(fIhandle, fPos + 1);
      if (virt_img) fIcon = static_cast<BBitmap*>(iupImageGetImage(virt_img, fIhandle, 0, nullptr));
    }

    /* Reserve uniform icon column (maximg_w from core) so all rows align. */
    int col_w = (fIhandle && fIhandle->data->show_image) ? fIhandle->data->maximg_w : 0;
    int sp = (fIhandle && fIhandle->data->spacing > 0) ? fIhandle->data->spacing : 0;
    frame.left += sp;

    if (fIcon)
    {
      float img_w = fIcon->Bounds().Width() + 1;
      float img_h = fIcon->Bounds().Height() + 1;
      float row_h = frame.Height() + 1;
      float draw_w = img_w, draw_h = img_h;
      bool fit = !fIhandle || fIhandle->data->fit_image;

      if (fit)
      {
        float sw = col_w > 0 ? col_w / img_w : 1.0f;
        float sh = row_h / img_h;
        float s = sw < sh ? sw : sh;
        if (s < 1.0f) { draw_w = img_w * s; draw_h = img_h * s; }
      }

      float ix = frame.left + 2 + (col_w > 0 ? (col_w - draw_w) / 2 : 0);
      float iy = frame.top + (row_h - draw_h) / 2;
      owner->SetDrawingMode(B_OP_ALPHA);
      owner->DrawBitmap(fIcon, BRect(ix, iy, ix + draw_w - 1, iy + draw_h - 1));
      owner->SetDrawingMode(B_OP_COPY);
    }

    BRect text_frame = frame;
    if (col_w > 0) text_frame.left += col_w + 6;

    rgb_color low = owner->LowColor();
    if (IsSelected())
    {
      owner->SetLowColor(iuphaikuColor(B_LIST_SELECTED_BACKGROUND_COLOR));
      owner->FillRect(text_frame, B_SOLID_LOW);
    }
    else if (complete)
    {
      owner->SetLowColor(owner->ViewColor());
      owner->FillRect(text_frame, B_SOLID_LOW);
    }
    else
      owner->SetLowColor(owner->ViewColor());

    if (Text())
    {
      owner->MovePenTo(text_frame.left + be_control_look->DefaultLabelSpacing(), text_frame.top + BaselineOffset());
      owner->DrawString(Text());
    }
    owner->SetLowColor(low);
  }

  void Update(BView* owner, const BFont* font) override
  {
    BStringItem::Update(owner, font);
    int col_w = (fIhandle && fIhandle->data->show_image) ? fIhandle->data->maximg_w : 0;
    if (col_w > 0) SetWidth(Width() + col_w + 6);
    if (fIhandle && fIhandle->data->spacing > 0)
    {
      SetHeight(Height() + 2 * fIhandle->data->spacing);
      SetWidth(Width() + 2 * fIhandle->data->spacing);
    }
  }

private:
  Ihandle* fIhandle;
  int fPos;
  BBitmap* fIcon;
};


class IupHaikuListView;

/* BScrollView reserves scrollbar width even when hidden. */
class IupHaikuListWrap : public BView
{
public:
  explicit IupHaikuListWrap(Ihandle* ih)
    : BView(BRect(0, 0, 0, 0), "iup_list_wrap", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fIhandle(ih), fList(nullptr), fSb(nullptr)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void Attach(IupHaikuListView* lv, BScrollBar* sb)
  {
    fList = reinterpret_cast<BView*>(lv);
    fSb = sb;
    AddChild(fList);
    AddChild(fSb);
  }

  void FrameResized(float w, float h) override
  {
    BView::FrameResized(w, h);
    RelayoutChildren();
  }

  void RelayoutChildren();

private:
  Ihandle* fIhandle;
  BView* fList;
  BScrollBar* fSb;
};


class IupHaikuListView : public BListView
{
public:
  IupHaikuListView(Ihandle* ih, list_view_type t)
    : BListView(BRect(0, 0, 0, 0), "iup_list", t, B_FOLLOW_ALL_SIDES),
      fIhandle(ih) {}

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

  void DrawItem(BListItem* item, BRect itemRect, bool complete) override
  {
    rgb_color text;
    unsigned char r, g, b;
    if (item->IsSelected())
      text = iuphaikuColor(B_LIST_SELECTED_ITEM_TEXT_COLOR);
    else if (fIhandle && iupStrToRGB(iupAttribGetStr(fIhandle, "FGCOLOR"), &r, &g, &b))
    {
      text.red = r; text.green = g; text.blue = b; text.alpha = 255;
    }
    else
      text = iuphaikuColor(B_LIST_ITEM_TEXT_COLOR);
    if (!item->IsEnabled())
      text = tint_color(text, B_DISABLED_LABEL_TINT);
    SetHighColor(text);
    item->DrawItem(this, itemRect, complete);
  }

  /* iupKeyCallKeyCb walks up to dialog K_ANY; IGNORE swallows the key */
  void KeyDown(const char* bytes, int32 numBytes) override
  {
    if (fIhandle && !iupdrvIsActive(fIhandle)) return;
    if (fIhandle && numBytes >= 1)
    {
      int32 raw_char = 0, mods = 0, raw_key = 0;
      if (BMessage* m = Window() ? Window()->CurrentMessage() : nullptr)
      {
        m->FindInt32("raw_char", &raw_char);
        m->FindInt32("key", &raw_key);
        m->FindInt32("modifiers", &mods);
      }
      int code = iuphaikuKeyDecode(static_cast<int>(static_cast<unsigned char>(bytes[0])), static_cast<int>(raw_char), static_cast<int>(raw_key), static_cast<unsigned>(mods));
      if (code)
      {
        int r = (mods & B_COMMAND_KEY) ? IUP_DEFAULT : iupKeyCallKeyCb(fIhandle, code);
        if (r == IUP_CLOSE) { IupExitLoop(); return; }
        if (r == IUP_IGNORE) return;
      }
    }
    BListView::KeyDown(bytes, numBytes);
  }

  void SelectionChanged() override
  {
    BListView::SelectionChanged();
    if (!fIhandle) return;
    if (iupAttribGet(fIhandle, "_IUPHAIKU_LIST_IGNORESELECT")) return;
    if (!iupdrvIsActive(fIhandle))
    {
      iupAttribSet(fIhandle, "_IUPHAIKU_LIST_IGNORESELECT", const_cast<char*>("1"));
      DeselectAll();
      iupAttribSet(fIhandle, "_IUPHAIKU_LIST_IGNORESELECT", nullptr);
      return;
    }

    if (fIhandle->data->is_multiple)
    {
      int n = CountItems();
      int* sel = static_cast<int*>(malloc(sizeof(int) * n));
      int sel_count = 0;
      for (int i = 0; i < n; ++i)
        if (IsItemSelected(i)) sel[sel_count++] = i;

      auto action = reinterpret_cast<IFnsii>(IupGetCallback(fIhandle, "ACTION"));
      IFns multi   = reinterpret_cast<IFns>(IupGetCallback(fIhandle, "MULTISELECT_CB"));
      if (action || multi)
        iupListMultipleCallActionCb(fIhandle, action, multi, sel, sel_count);
      free(sel);
    }
    else
    {
      int idx = CurrentSelection();
      if (idx < 0) return;

      if (fIhandle->data->has_editbox && !fIhandle->data->is_dropdown)
      {
        if (auto* it = dynamic_cast<BStringItem*>(ItemAt(idx)))
        {
          if (auto* e = reinterpret_cast<BTextControl*>(iupAttribGet(fIhandle, "_IUPHAIKU_LIST_EDIT")))
          {
            BMessage* saved = e->ModificationMessage();
            BMessage* clone = saved ? new BMessage(*saved) : nullptr;
            e->SetModificationMessage(nullptr);
            e->SetText(it->Text() ? it->Text() : "");
            e->SetModificationMessage(clone);
          }
        }
      }

      auto action = reinterpret_cast<IFnsii>(IupGetCallback(fIhandle, "ACTION"));
      if (action)
        iupListSingleCallActionCb(fIhandle, action, idx + 1);
    }

    Icallback vc = IupGetCallback(fIhandle, "VALUECHANGED_CB");
    if (vc) { int r = vc(fIhandle); if (r == IUP_CLOSE) IupExitLoop(); }
  }

  void MouseDown(BPoint where) override
  {
    if (fIhandle && !iupdrvIsActive(fIhandle)) return;
    BListView::MouseDown(where);
    if (!fIhandle) return;
    BMessage* msg = Looper() ? Looper()->CurrentMessage() : nullptr;
    int32 clicks = 1, buttons = 0, mods = 0;
    if (msg) { msg->FindInt32("clicks", &clicks); msg->FindInt32("buttons", &buttons); msg->FindInt32("modifiers", &mods); }

    int btn = IUP_BUTTON1;
    if      (buttons & B_SECONDARY_MOUSE_BUTTON) btn = IUP_BUTTON3;
    else if (buttons & B_TERTIARY_MOUSE_BUTTON)  btn = IUP_BUTTON2;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus(static_cast<unsigned>(mods), static_cast<unsigned>(buttons), 0, status, clicks == 2 ? 1 : 0);
    auto bcb = reinterpret_cast<IFniiiis>(IupGetCallback(fIhandle, "BUTTON_CB"));
    if (bcb) bcb(fIhandle, btn, 1, static_cast<int>(where.x), static_cast<int>(where.y), status);

    if (clicks >= 2)
    {
      int idx = CurrentSelection();
      if (idx >= 0)
      {
        auto cb = reinterpret_cast<IFnis>(IupGetCallback(fIhandle, "DBLCLICK_CB"));
        if (cb) iupListSingleCallDblClickCb(fIhandle, cb, idx + 1);
      }
    }
  }

  void MouseUp(BPoint where) override
  {
    BListView::MouseUp(where);
    if (!fIhandle) return;
    int32 mods = 0;
    if (BMessage* msg = Looper() ? Looper()->CurrentMessage() : nullptr) msg->FindInt32("modifiers", &mods);
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus(static_cast<unsigned>(mods), 0, 0, status, 0);
    auto bcb = reinterpret_cast<IFniiiis>(IupGetCallback(fIhandle, "BUTTON_CB"));
    if (bcb) bcb(fIhandle, IUP_BUTTON1, 0, static_cast<int>(where.x), static_cast<int>(where.y), status);
  }

  bool InitiateDrag(BPoint where, int32 index, bool /*wasSelected*/) override
  {
    if (!fIhandle) return false;
    if (fIhandle->data->show_dragdrop)
    {
      BMessage drag(IUPHAIKU_LIST_REORDER_MSG);
      drag.AddPointer("be:originator", fIhandle);
      drag.AddInt32("_iup_team", be_app ? be_app->Team() : 0);
      drag.AddInt32("source_index", index);
      BRect r = ItemFrame(index);
      DragMessage(&drag, r, this);
      return true;
    }
    if (iupAttribGetBoolean(fIhandle, "DRAGSOURCE"))
      return iuphaikuDnDInitiateDrag(fIhandle, this, where);
    return false;
  }

  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  {
    BListView::MouseMoved(where, transit, drag);
    if (!fIhandle) return;
    if (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW)
    {
      int32 buttons = 0, mods = 0;
      if (BMessage* cur = Looper() ? Looper()->CurrentMessage() : nullptr) { cur->FindInt32("buttons", &buttons); cur->FindInt32("modifiers", &mods); }
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iuphaikuButtonKeySetStatus(static_cast<unsigned>(mods), static_cast<unsigned>(buttons), 0, status, 0);
      auto mcb = reinterpret_cast<IFniis>(IupGetCallback(fIhandle, "MOTION_CB"));
      if (mcb) mcb(fIhandle, static_cast<int>(where.x), static_cast<int>(where.y), status);
    }
    if (!drag || !fIhandle->data->show_dragdrop) return;
    if (drag->what != IUPHAIKU_LIST_REORDER_MSG) return;
    int32 team = 0;
    drag->FindInt32("_iup_team", &team);
    if (!be_app || team != be_app->Team()) return;
    Ihandle* origin = nullptr;
    drag->FindPointer("be:originator", reinterpret_cast<void**>(&origin));
    if (origin != fIhandle) return;

    int32 mods = 0;
    if (BLooper* L = Looper())
      if (BMessage* cur = L->CurrentMessage()) cur->FindInt32("modifiers", &mods);
    BCursorID id = (mods & B_COMMAND_KEY) ? B_CURSOR_ID_COPY : B_CURSOR_ID_MOVE;
    int last = iupAttribGetInt(fIhandle, "_IUPHAIKU_LIST_REORDER_CURSOR");
    if (last == static_cast<int>(id)) return;
    auto* prev = reinterpret_cast<BCursor*>(iupAttribGet(fIhandle, "_IUPHAIKU_LIST_REORDER_BCURSOR"));
    auto* nw = new BCursor(id);
    SetViewCursor(nw, true);
    iupAttribSetInt(fIhandle, "_IUPHAIKU_LIST_REORDER_CURSOR", static_cast<int>(id));
    iupAttribSet(fIhandle, "_IUPHAIKU_LIST_REORDER_BCURSOR", reinterpret_cast<char*>(nw));
    delete prev;
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_LIST_REORDER_MSG && msg->WasDropped() && fIhandle && fIhandle->data->show_dragdrop)
    {
      int32 team = 0;
      msg->FindInt32("_iup_team", &team);
      Ihandle* src_ih = nullptr;
      if (be_app && team == be_app->Team())
        msg->FindPointer("be:originator", reinterpret_cast<void**>(&src_ih));
      if (src_ih == fIhandle)
      {
        int32 src_index = -1;
        msg->FindInt32("source_index", &src_index);
        BPoint dropPt = ConvertFromScreen(msg->DropPoint());
        int drop_index = IndexOf(dropPt);
        int is_ctrl = 0;
        if (iupListCallDragDropCb(fIhandle, static_cast<int>(src_index), drop_index, &is_ctrl) == IUP_CONTINUE)
          haikuListReorder(fIhandle, static_cast<int>(src_index), drop_index, is_ctrl);

        if (auto* c = reinterpret_cast<BCursor*>(iupAttribGet(fIhandle, "_IUPHAIKU_LIST_REORDER_BCURSOR")))
        {
          SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, true);
          delete c;
          iupAttribSet(fIhandle, "_IUPHAIKU_LIST_REORDER_BCURSOR", nullptr);
          iupAttribSet(fIhandle, "_IUPHAIKU_LIST_REORDER_CURSOR", nullptr);
        }
        return;
      }
    }
    if (fIhandle && iuphaikuHandleDropFiles(fIhandle, this, msg)) return;
    if (fIhandle && iuphaikuDnDMessageReceived(fIhandle, this, msg)) return;
    BListView::MessageReceived(msg);
  }

  void MakeFocus(bool focused = true) override
  {
    BListView::MakeFocus(focused);
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focused ? 1 : 0);
  }

private:
  Ihandle* fIhandle;
};


void IupHaikuListWrap::RelayoutChildren()
{
  if (!fList || !fSb) return;

  auto* lv = dynamic_cast<IupHaikuListView*>(fList);
  bool has_sb = !fIhandle || fIhandle->data->sb;
  bool autohide = fIhandle && iupAttribGetBoolean(fIhandle, "AUTOHIDE");
  float total_w = Bounds().Width();
  float total_h = Bounds().Height();
  float sb_w = fSb->PreferredSize().Width();

  bool need_sb = has_sb && !autohide;
  if (has_sb && autohide && lv)
  {
    float content_h = 0;
    int n = static_cast<int>(lv->CountItems());
    for (int i = 0; i < n; ++i)
      if (BListItem* it = lv->ItemAt(i)) content_h += it->Height();
    /* BRect::Height() is right-left (N-1 for N pixels). */
    need_sb = content_h > total_h + 1;
  }

  if (need_sb)
  {
    fList->MoveTo(0, 0);
    fList->ResizeTo(total_w - sb_w, total_h);
    fSb->MoveTo(total_w - sb_w + 1, 0);
    fSb->ResizeTo(sb_w - 1, total_h);
    if (fSb->IsHidden(fSb)) fSb->Show();
  }
  else
  {
    fList->MoveTo(0, 0);
    fList->ResizeTo(total_w, total_h);
    if (!fSb->IsHidden(fSb)) fSb->Hide();
  }
}


class IupHaikuListDropHandler : public BHandler
{
public:
  explicit IupHaikuListDropHandler(Ihandle* ih) : BHandler("iup_list_drop"), fIhandle(ih) {}

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_LIST_DROP_MSG && fIhandle)
    {
      int pos = -1;
      msg->FindInt32("pos", static_cast<int32*>(&pos));
      if (pos < 0) { BHandler::MessageReceived(msg); return; }

      auto* field = reinterpret_cast<BMenuField*>(fIhandle->handle);
      if (!fIhandle->data->has_editbox && field)
      {
        BMenu* m = field->Menu();
        if (m)
        {
          LooperLockGuard guard(field->Looper());
          for (int32 i = 0; i < m->CountItems(); ++i)
            m->ItemAt(i)->SetMarked(i == pos);
        }
      }

      auto action = reinterpret_cast<IFnsii>(IupGetCallback(fIhandle, "ACTION"));
      if (action) iupListSingleCallActionCb(fIhandle, action, pos + 1);

      Icallback vc = IupGetCallback(fIhandle, "VALUECHANGED_CB");
      if (vc) { int r = vc(fIhandle); if (r == IUP_CLOSE) IupExitLoop(); }
      return;
    }
    BHandler::MessageReceived(msg);
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
};


class IupHaikuListEditKeyFilter : public BMessageFilter
{
public:
  explicit IupHaikuListEditKeyFilter(Ihandle* ih)
    : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN), fIhandle(ih) {}

  filter_result Filter(BMessage* msg, BHandler** target) override
  {
    if (!fIhandle || !fIhandle->handle) return B_DISPATCH_MESSAGE;
    auto cb = reinterpret_cast<IFnis>(IupGetCallback(fIhandle, "EDIT_CB"));
    if (!cb && !fIhandle->data->mask && fIhandle->data->nc <= 0) return B_DISPATCH_MESSAGE;

    const char* bytes = nullptr;
    ssize_t byte_count = 0;
    if (msg->FindData("bytes", B_STRING_TYPE, reinterpret_cast<const void**>(&bytes), &byte_count) != B_OK || !bytes)
      return B_DISPATCH_MESSAGE;
    int char_len = (byte_count > 0 && bytes[byte_count - 1] == 0) ? static_cast<int>(byte_count) - 1 : static_cast<int>(byte_count);
    if (char_len == 1 && static_cast<unsigned char>(bytes[0]) < B_SPACE) return B_DISPATCH_MESSAGE;

    BTextView* tv = (target && *target) ? dynamic_cast<BTextView*>(*target) : nullptr;
    if (!tv) return B_DISPATCH_MESSAGE;
    int32 sel_s = 0, sel_e = 0;
    tv->GetSelection(&sel_s, &sel_e);

    char tmp[8] = {0};
    int copy = char_len < 7 ? char_len : 7;
    memcpy(tmp, bytes, copy);

    int ret = iupEditCallActionCb(fIhandle, cb, tmp, sel_s, sel_e, fIhandle->data->mask, fIhandle->data->nc, 0, 1);
    if (ret == 0) return B_SKIP_MESSAGE;
    if (ret != -1 && char_len == 1)
    {
      char rep[2] = { static_cast<char>(ret), 0 };
      msg->ReplaceData("bytes", B_STRING_TYPE, rep, 2);
      int8 raw = static_cast<int8>(rep[0]);
      if (msg->HasInt8("byte")) msg->ReplaceInt8("byte", raw); else msg->AddInt8("byte", raw);
    }
    return B_DISPATCH_MESSAGE;
  }

private:
  Ihandle* fIhandle;
};

class IupHaikuListEditCtrl : public BTextControl
{
public:
  explicit IupHaikuListEditCtrl(Ihandle* ih)
    : BTextControl(BRect(0, 0, 0, 0), "iup_list_edit", nullptr, "", nullptr, B_FOLLOW_NONE),
      fIhandle(ih), fKeyFilter(nullptr) { BTextControl::SetDivider(0); }

  ~IupHaikuListEditCtrl() override
  {
    if (fKeyFilter && TextView()) TextView()->RemoveFilter(fKeyFilter);
    delete fKeyFilter;
  }

  void AttachedToWindow() override
  {
    BTextControl::AttachedToWindow();
    SetTarget(this);
    SetModificationMessage(new BMessage(IUPHAIKU_LIST_TEXT_MSG));
    if (fIhandle && !fKeyFilter && TextView())
    {
      fKeyFilter = new IupHaikuListEditKeyFilter(fIhandle);
      TextView()->AddFilter(fKeyFilter);
    }
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_LIST_TEXT_MSG && fIhandle)
    {
      if (BTextView* tv = TextView())
      {
        auto ccb = reinterpret_cast<IFniii>(IupGetCallback(fIhandle, "CARET_CB"));
        if (ccb) { int32 s = 0, e = 0; tv->GetSelection(&s, &e); ccb(fIhandle, 1, static_cast<int>(s) + 1, static_cast<int>(s)); }
      }
      Icallback vc = IupGetCallback(fIhandle, "VALUECHANGED_CB");
      if (vc) { int r = vc(fIhandle); if (r == IUP_CLOSE) IupExitLoop(); }
      return;
    }
    BTextControl::MessageReceived(msg);
  }

  /* the inner BTextView holds the focus and invalidates on focus change */
  void Draw(BRect updateRect) override
  {
    BTextControl::Draw(updateRect);
    bool focused = TextView() && TextView()->IsFocus();
    if (focused != fLastFocus)
    {
      fLastFocus = focused;
      if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focused ? 1 : 0);
    }
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
  BMessageFilter* fKeyFilter;
  bool fLastFocus = false;
};


class IupHaikuListContainer : public BView
{
public:
  IupHaikuListContainer(Ihandle* ih, bool dropdown)
    : BView(BRect(0, 0, 0, 0), "iup_list_container", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fIhandle(ih), fDropdown(dropdown), fEdit(nullptr), fScroll(nullptr),
      fListView(nullptr), fChevron(nullptr), fPopUp(nullptr)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void SetEdit(IupHaikuListEditCtrl* e) { fEdit = e; }
  void SetScrollList(BScrollView* sv, IupHaikuListView* lv) { fScroll = sv; fListView = lv; }
  void SetChevron(BButton* btn, BPopUpMenu* menu) { fChevron = btn; fPopUp = menu; }
  void SetChevronEnabled(bool e) { if (fChevron) fChevron->SetEnabled(e); }

  void FrameResized(float w, float h) override
  {
    BView::FrameResized(w, h);
    Layout();
  }

  void Layout()
  {
    BRect b = Bounds();
    const float edit_h = 24.0f;

    if (fDropdown)
    {
      const float btn_w = 22.0f;
      if (fEdit)
        fEdit->MoveTo(0, 0), fEdit->ResizeTo(b.Width() - btn_w - 1, edit_h);
      if (fChevron)
        fChevron->MoveTo(b.Width() - btn_w + 1, 0), fChevron->ResizeTo(btn_w - 2, edit_h);
    }
    else
    {
      if (fEdit)
        fEdit->MoveTo(0, 0), fEdit->ResizeTo(b.Width(), edit_h);
      if (fScroll)
      {
        fScroll->MoveTo(0, edit_h + 2);
        fScroll->ResizeTo(b.Width(), b.Height() - edit_h - 2);
      }
    }
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_LIST_OPENMENU && fPopUp && fEdit)
    {
      if (!iupdrvIsActive(fIhandle)) return;
      BPoint where = fEdit->ConvertToScreen(BPoint(0, fEdit->Bounds().Height()));
      IFni dd = reinterpret_cast<IFni>(IupGetCallback(fIhandle, "DROPDOWN_CB"));
      if (dd) dd(fIhandle, 1);
      BMenuItem* picked = fPopUp->Go(where, false, false, false);
      if (dd) dd(fIhandle, 0);
      if (picked && fIhandle)
      {
        int pos = fPopUp->IndexOf(picked);
        const char* label = picked->Label();
        if (label) fEdit->SetText(label);

        auto action = reinterpret_cast<IFnsii>(IupGetCallback(fIhandle, "ACTION"));
        if (action) iupListSingleCallActionCb(fIhandle, action, pos + 1);
      }
      return;
    }
    BView::MessageReceived(msg);
  }

private:
  Ihandle* fIhandle;
  bool fDropdown;
  IupHaikuListEditCtrl* fEdit;
  BScrollView* fScroll;
  IupHaikuListView* fListView;
  BButton* fChevron;
  BPopUpMenu* fPopUp;
};


static IupHaikuListView* haikuListGetListView(Ihandle* ih)
{
  if (!ih || !ih->handle) return nullptr;
  return reinterpret_cast<IupHaikuListView*>(iupAttribGet(ih, "_IUPHAIKU_LIST_INNER"));
}

static IupHaikuListEditCtrl* haikuListGetEdit(Ihandle* ih)
{
  if (!ih || !ih->handle || !ih->data->has_editbox) return nullptr;
  return reinterpret_cast<IupHaikuListEditCtrl*>(iupAttribGet(ih, "_IUPHAIKU_LIST_EDIT"));
}

static BMenu* haikuListGetMenu(Ihandle* ih)
{
  if (!ih || !ih->handle) return nullptr;
  if (ih->data->has_editbox && ih->data->is_dropdown)
    return reinterpret_cast<BMenu*>(iupAttribGet(ih, "_IUPHAIKU_LIST_POPUP"));
  if (ih->data->is_dropdown)
    return (reinterpret_cast<BMenuField*>(ih->handle))->Menu();
  return nullptr;
}

static IupHaikuListDropHandler* haikuListGetDropHandler(Ihandle* ih)
{
  return reinterpret_cast<IupHaikuListDropHandler*>(iupAttribGet(ih, "_IUPHAIKU_LIST_DROPHANDLER"));
}

static int haikuListMenuSortPos(BMenu* m, const char* value)
{
  int n = m->CountItems();
  for (int i = 0; i < n; i++)
  {
    BMenuItem* it = m->ItemAt(i);
    const char* t = it ? it->Label() : "";
    if (iupStrCompare(t ? t : "", value, 0, 1) > 0) return i;
  }
  return n;
}

static int haikuListViewSortPos(BListView* lv, const char* value)
{
  int n = lv->CountItems();
  for (int i = 0; i < n; i++)
  {
    auto* it = dynamic_cast<BStringItem*>(lv->ItemAt(i));
    const char* t = it ? it->Text() : "";
    if (iupStrCompare(t ? t : "", value, 0, 1) > 0) return i;
  }
  return n;
}

extern "C" IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SORT"))
  {
    iupdrvListInsertItem(ih, 0, value);
    return;
  }

  BMenu* m = haikuListGetMenu(ih);
  IupHaikuListView* lv = haikuListGetListView(ih);

  if (m)
  {
    auto* msg = new BMessage(IUPHAIKU_LIST_DROP_MSG);
    msg->AddInt32("pos", m->CountItems());
    auto* item = new IupHaikuDropdownItem(ih, value ? value : "", msg);
    IupHaikuListDropHandler* h = haikuListGetDropHandler(ih);
    if (h) item->SetTarget(BMessenger(h));

    LooperLockGuard guard((reinterpret_cast<BView*>(ih->handle))->Looper());
    m->AddItem(item);
  }
  if (lv)
  {
    LooperLockGuard guard(lv->Looper());
    lv->AddItem(new IupHaikuListItem(ih, lv->CountItems(), value));
    if (auto* w = dynamic_cast<IupHaikuListWrap*>(reinterpret_cast<BView*>(ih->handle))) w->RelayoutChildren();
  }
}

extern "C" IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  BMenu* m = haikuListGetMenu(ih);
  IupHaikuListView* lv = haikuListGetListView(ih);

  if (iupAttribGetBoolean(ih, "SORT"))
    pos = m ? haikuListMenuSortPos(m, value) : (lv ? haikuListViewSortPos(lv, value) : pos);

  if (m)
  {
    auto* msg = new BMessage(IUPHAIKU_LIST_DROP_MSG);
    msg->AddInt32("pos", pos);
    auto* item = new IupHaikuDropdownItem(ih, value ? value : "", msg);
    IupHaikuListDropHandler* h = haikuListGetDropHandler(ih);
    if (h) item->SetTarget(BMessenger(h));

    LooperLockGuard guard((reinterpret_cast<BView*>(ih->handle))->Looper());
    m->AddItem(item, pos);
    for (int32 i = pos + 1; i < m->CountItems(); ++i)
    {
      BMessage* mm = m->ItemAt(i)->Message();
      if (mm) mm->ReplaceInt32("pos", i);
    }
  }
  if (lv)
  {
    LooperLockGuard guard(lv->Looper());
    lv->AddItem(new IupHaikuListItem(ih, pos, value), pos);
    for (int32 i = pos + 1; i < lv->CountItems(); ++i)
      if (auto* it = dynamic_cast<IupHaikuListItem*>(lv->ItemAt(i))) it->SetPos(i);
    if (auto* w = dynamic_cast<IupHaikuListWrap*>(reinterpret_cast<BView*>(ih->handle))) w->RelayoutChildren();
  }
}

extern "C" IUP_SDK_API void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  BMenu* m = haikuListGetMenu(ih);
  IupHaikuListView* lv = haikuListGetListView(ih);

  if (m)
  {
    LooperLockGuard guard((reinterpret_cast<BView*>(ih->handle))->Looper());
    delete m->RemoveItem(pos);
    for (int32 i = pos; i < m->CountItems(); ++i)
    {
      BMessage* mm = m->ItemAt(i)->Message();
      if (mm) mm->ReplaceInt32("pos", i);
    }
  }
  if (lv)
  {
    LooperLockGuard guard(lv->Looper());
    delete lv->RemoveItem(pos);
    for (int32 i = pos; i < lv->CountItems(); ++i)
      if (auto* it = dynamic_cast<IupHaikuListItem*>(lv->ItemAt(i))) it->SetPos(i);
    if (auto* w = dynamic_cast<IupHaikuListWrap*>(reinterpret_cast<BView*>(ih->handle))) w->RelayoutChildren();
  }
}

extern "C" IUP_SDK_API void iupdrvListRemoveAllItems(Ihandle* ih)
{
  BMenu* m = haikuListGetMenu(ih);
  IupHaikuListView* lv = haikuListGetListView(ih);

  if (m)
  {
    LooperLockGuard guard((reinterpret_cast<BView*>(ih->handle))->Looper());
    while (m->CountItems() > 0) delete m->RemoveItem(static_cast<int32>(0));
  }
  if (lv)
  {
    LooperLockGuard guard(lv->Looper());
    for (int32 i = lv->CountItems() - 1; i >= 0; --i) delete lv->RemoveItem(i);
    if (auto* w = dynamic_cast<IupHaikuListWrap*>(reinterpret_cast<BView*>(ih->handle))) w->RelayoutChildren();
  }
}

extern "C" IUP_SDK_API int iupdrvListGetCount(Ihandle* ih)
{
  BMenu* m = haikuListGetMenu(ih);
  if (m) return m->CountItems();
  IupHaikuListView* lv = haikuListGetListView(ih);
  if (lv) return lv->CountItems();
  return 0;
}

extern "C" IUP_SDK_API void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  if (BMenu* m = haikuListGetMenu(ih))
  {
    auto* mi = dynamic_cast<IupHaikuDropdownItem*>(m->ItemAt(id - 1));
    return mi ? mi->Icon() : nullptr;
  }
  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv) return nullptr;
  auto* it = dynamic_cast<IupHaikuListItem*>(lv->ItemAt(id - 1));
  return it ? it->Icon() : nullptr;
}

extern "C" IUP_SDK_API int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  /* id is 0-based per the cross-driver contract. */
  if (BMenu* m = haikuListGetMenu(ih))
  {
    LooperLockGuard guard((reinterpret_cast<BView*>(ih->handle))->Looper());
    auto* mi = dynamic_cast<IupHaikuDropdownItem*>(m->ItemAt(id));
    if (!mi) return 0;
    mi->SetIcon(static_cast<BBitmap*>(hImage));
    m->InvalidateLayout();
  }
  else
  {
    IupHaikuListView* lv = haikuListGetListView(ih);
    if (!lv) return 0;
    LooperLockGuard guard(lv->Looper());
    auto* it = dynamic_cast<IupHaikuListItem*>(lv->ItemAt(id));
    if (!it) return 0;
    it->SetIcon(static_cast<BBitmap*>(hImage));
    lv->InvalidateItem(id);
  }

  /* maximg_w drives DrawItem's text shift; stale on post-map icons. */
  if (hImage)
  {
    auto* bm = static_cast<BBitmap*>(hImage);
    int w = static_cast<int>(bm->Bounds().Width() + 1);
    int h = static_cast<int>(bm->Bounds().Height() + 1);
    if (w > ih->data->maximg_w)
    {
      ih->data->maximg_w = w;
      if (IupHaikuListView* lv = haikuListGetListView(ih))
      {
        LooperLockGuard guard(lv->Looper());
        lv->InvalidateLayout(true);
        lv->Invalidate();
      }
    }
    if (h > ih->data->maximg_h) ih->data->maximg_h = h;
  }
  return 1;
}

static char* haikuListGetIdValueAttrib(Ihandle* ih, int id)
{
  if (ih->data->is_virtual)
    return iupListGetItemValueCb(ih, id);

  if (BMenu* m = haikuListGetMenu(ih))
  {
    BMenuItem* it = m->ItemAt(id - 1);
    return it ? iupStrReturnStr(it->Label()) : nullptr;
  }
  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv) return nullptr;
  auto* it = dynamic_cast<IupHaikuListItem*>(lv->ItemAt(id - 1));
  return it ? iupStrReturnStr(it->Text()) : nullptr;
}

static char* haikuListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  return static_cast<char*>(iupdrvListGetImageHandle(ih, id));
}

static int haikuListSetImageAttribId(Ihandle* ih, int id, const char* value)
{
  if (!ih->data->show_image) return 0;
  auto* bm = static_cast<BBitmap*>(iupImageGetImage(value, ih, 0, nullptr));
  /* IupSetAttributeId is 1-based; iupdrvListSetImageHandle is 0-based. */
  iupdrvListSetImageHandle(ih, id - 1, bm);
  return 0;
}

extern "C" IUP_SDK_API void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv) return;
  LooperLockGuard guard(lv->Looper());
  int cur = lv->CountItems();
  if (count > cur)
  {
    BList batch;
    for (int i = cur; i < count; ++i)
      batch.AddItem(new IupHaikuListItem(ih, i, nullptr));
    lv->AddList(&batch);
  }
  else if (count < cur)
  {
    if (count > 0) lv->DeselectExcept(0, count - 1);
    else           lv->DeselectAll();

    BList tail;
    for (int i = count; i < cur; ++i)
      tail.AddItem(lv->ItemAt(i));
    lv->RemoveItems(count, cur - count);
    for (int i = 0; i < tail.CountItems(); ++i)
      delete static_cast<BListItem*>(tail.ItemAtFast(i));

    BRect b = lv->Bounds();
    lv->FrameResized(b.Width(), b.Height());
  }
  if (auto* w = dynamic_cast<IupHaikuListWrap*>(reinterpret_cast<BView*>(ih->handle))) w->RelayoutChildren();
  lv->Invalidate();
}

extern "C" IUP_SDK_API void iupdrvListAddBorders(Ihandle* ih, int* w, int* h)
{
  if (!ih) return;

  /* BStringItem needs 2x DefaultLabelSpacing (left inset + matching right margin). */
  int label_pad = be_control_look ? static_cast<int>(2 * be_control_look->DefaultLabelSpacing() + 0.5f) : 16;

  if (ih->data->is_dropdown && !ih->data->has_editbox)
  {
    int char_h = 0;
    iupdrvFontGetCharSize(ih, nullptr, &char_h);
    if (w) *w += 24;
    if (h && *h < char_h + 10) *h = char_h + 10;
    return;
  }

  if (ih->data->is_dropdown && ih->data->has_editbox)
  {
    if (w) *w += 24;
    if (h && *h < 24) *h = 24;
    return;
  }

  if (!ih->data->is_dropdown && ih->data->has_editbox)
  {
    if (w) *w += 4 + label_pad;
    if (h)
    {
      int char_h = 0;
      iupdrvFontGetCharSize(ih, nullptr, &char_h);
      int item_h = char_h;
      iupdrvListAddItemSpace(ih, &item_h);
      *h -= item_h;
      *h += 24 + 2;
    }
    return;
  }

  if (w) *w += label_pad;
}

extern "C" IUP_SDK_API void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
  /* ceil per font-component + 4 px matches the row height BListView uses. */
  if (!h) return;
  BFont* bf = ih ? iuphaikuGetBFont(iupGetFontValue(ih)) : nullptr;
  if (!bf) { *h += 4; return; }
  font_height fh; bf->GetHeight(&fh);
  int row = static_cast<int>(ceilf(fh.ascent)) + static_cast<int>(ceilf(fh.descent)) + static_cast<int>(ceilf(fh.leading)) + 4;
  int line = static_cast<int>(ceilf(fh.ascent + fh.descent + fh.leading));
  int diff = row - line;
  *h += diff < 4 ? 4 : diff;
}


static int haikuListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown && !ih->data->has_editbox)
  {
    BMenu* m = haikuListGetMenu(ih);
    if (!m) return 0;
    int pos = 0;
    if (!iupStrToInt(value, &pos)) return 0;
    LooperLockGuard guard((reinterpret_cast<BView*>(ih->handle))->Looper());
    for (int32 i = 0; i < m->CountItems(); ++i)
      m->ItemAt(i)->SetMarked(i == pos - 1);
    return 0;
  }

  if (ih->data->is_dropdown && ih->data->has_editbox)
  {
    IupHaikuListEditCtrl* e = haikuListGetEdit(ih);
    if (!e) return 0;
    LooperLockGuard guard(e->Looper());
    e->SetText(value ? value : "");
    return 0;
  }

  if (!ih->data->is_dropdown && ih->data->has_editbox)
  {
    IupHaikuListEditCtrl* e = haikuListGetEdit(ih);
    if (!e) return 0;
    LooperLockGuard guard(e->Looper());
    e->SetText(value ? value : "");
    return 0;
  }

  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv) return 0;
  LooperLockGuard guard(lv->Looper());

  iupAttribSet(ih, "_IUPHAIKU_LIST_IGNORESELECT", "1");
  if (!ih->data->is_multiple)
  {
    int pos = 0;
    if (!iupStrToInt(value, &pos)) { iupAttribSet(ih, "_IUPHAIKU_LIST_IGNORESELECT", nullptr); return 0; }
    if (pos == 0) lv->DeselectAll();
    else          lv->Select(pos - 1);
  }
  else
  {
    lv->DeselectAll();
    if (value)
    {
      int n = lv->CountItems();
      for (int i = 0; i < n && value[i]; ++i)
        if (value[i] == '+') lv->Select(i, true);
    }
  }
  iupAttribSet(ih, "_IUPHAIKU_LIST_IGNORESELECT", nullptr);
  return 0;
}

static int haikuListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupHaikuListView* lv = haikuListGetListView(ih);
  IupHaikuListEditCtrl* edit = haikuListGetEdit(ih);
  unsigned char r, g, b;
  if (!ih->handle || !iupStrToRGB(value, &r, &g, &b)) return 1;

  rgb_color c = { r, g, b, 255 };
  auto* container = reinterpret_cast<BView*>(ih->handle);
  LooperLockGuard guard(container->Looper());
  if (lv)
  {
    lv->SetViewColor(c);
    lv->SetLowColor(c);
    lv->Invalidate();
    container->SetViewColor(c);
    container->Invalidate();
  }
  if (edit)
  {
    edit->TextView()->SetViewColor(c);
    edit->TextView()->SetLowColor(c);
    edit->Invalidate();
  }
  return 1;
}

static int haikuListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  IupHaikuListView* lv = haikuListGetListView(ih);
  IupHaikuListEditCtrl* edit = haikuListGetEdit(ih);
  unsigned char r, g, b;
  if (!ih->handle || !iupStrToRGB(value, &r, &g, &b)) return 1;

  rgb_color c = { r, g, b, 255 };
  auto* container = reinterpret_cast<BView*>(ih->handle);
  LooperLockGuard guard(container->Looper());
  if (lv)
    lv->Invalidate();
  if (edit)
  {
    edit->TextView()->SetFontAndColor(0, edit->TextView()->TextLength(), nullptr, 0, &c);
    edit->Invalidate();
  }
  return 1;
}

static char* haikuListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->is_dropdown && !ih->data->has_editbox)
  {
    BMenu* m = haikuListGetMenu(ih);
    if (!m) return nullptr;
    BMenuItem* it = m->FindMarked();
    if (!it) return iupStrReturnInt(0);
    return iupStrReturnInt(m->IndexOf(it) + 1);
  }

  if (ih->data->has_editbox)
  {
    IupHaikuListEditCtrl* e = haikuListGetEdit(ih);
    if (!e) return nullptr;
    return iupStrReturnStr(e->Text());
  }

  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv) return nullptr;
  if (!ih->data->is_multiple)
    return iupStrReturnInt(lv->CurrentSelection() + 1);

  int n = lv->CountItems();
  char* buf = iupStrGetMemory(n + 1);
  for (int i = 0; i < n; ++i)
    buf[i] = lv->IsItemSelected(i) ? '+' : '-';
  buf[n] = 0;
  return buf;
}

static int haikuListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv) return 0;
  int pos = 0;
  if (!iupStrToInt(value, &pos) || pos < 1) return 0;
  LooperLockGuard guard(lv->Looper());
  lv->ScrollTo(pos - 1);
  return 0;
}

static int haikuListSetActiveAttrib(Ihandle* ih, const char* value)
{
  bool active = iupStrBoolean(value) ? true : false;

  IupHaikuListEditCtrl* e = haikuListGetEdit(ih);
  if (e)
  {
    LooperLockGuard guard(e->Looper());
    e->SetEnabled(active);
  }

  if (ih->data->is_dropdown && !ih->data->has_editbox && ih->handle)
  {
    auto* field = reinterpret_cast<BMenuField*>(ih->handle);
    LooperLockGuard guard(field->Looper());
    field->SetEnabled(active);
    if (BMenu* menu = field->Menu())
      menu->SetEnabled(active);
  }

  if (ih->data->is_dropdown && ih->data->has_editbox && ih->handle)
  {
    if (auto* cont = dynamic_cast<IupHaikuListContainer*>(reinterpret_cast<BView*>(ih->handle)))
    {
      LooperLockGuard guard(cont->Looper());
      cont->SetChevronEnabled(active);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static void haikuListReorder(Ihandle* ih, int src, int drop, int is_ctrl)
{
  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv || src < 0) return;

  const char* text = nullptr;
  if (BListItem* it = lv->ItemAt(src))
    text = (dynamic_cast<IupHaikuListItem*>(it)) ? dynamic_cast<IupHaikuListItem*>(it)->Text() : nullptr;
  if (!text) text = "";
  char* text_copy = iupStrDup(text);
  void* img = iupdrvListGetImageHandle(ih, src + 1);

  int count = lv->CountItems();
  int insert_at = drop;
  if (drop < 0 || drop >= count) insert_at = count;

  iupdrvListInsertItem(ih, insert_at, text_copy);
  if (img) iupdrvListSetImageHandle(ih, insert_at, img);

  int adj_src = src > insert_at ? src + 1 : src;
  int new_pos = insert_at;
  if (!is_ctrl)
  {
    iupdrvListRemoveItem(ih, adj_src);
    if (adj_src < insert_at) new_pos = insert_at - 1;
  }

  iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", new_pos + 1);
  IupSetInt(ih, "VALUE", new_pos + 1);
  free(text_copy);
}

static int haikuListConvertXYToPos(Ihandle* ih, int x, int y)
{
  IupHaikuListView* lv = haikuListGetListView(ih);
  if (!lv) return -1;
  int idx = static_cast<int>(lv->IndexOf(BPoint(x, y)));
  if (idx < 0) return -1;
  return idx + 1;
}

/* BMenuBar opens the popup ignoring SetEnabled, so drop the click when inactive */
class IupHaikuListMenuFilter : public BMessageFilter
{
public:
  explicit IupHaikuListMenuFilter(Ihandle* ih)
    : BMessageFilter(B_MOUSE_DOWN), fIhandle(ih) {}
  filter_result Filter(BMessage*, BHandler**) override
  {
    return iupdrvIsActive(fIhandle) ? B_DISPATCH_MESSAGE : B_SKIP_MESSAGE;
  }
private:
  Ihandle* fIhandle;
};

static int haikuListMapMethod(Ihandle* ih)
{
  if (ih->data->is_dropdown && be_app)
  {
    auto* h = new IupHaikuListDropHandler(ih);
    LooperLockGuard guard(be_app);
    be_app->AddHandler(h);
    iupAttribSet(ih, "_IUPHAIKU_LIST_DROPHANDLER", reinterpret_cast<char*>(h));
  }

  /* fixedSize=true so BMenuField's bar fills the field width on resize. */
  if (ih->data->is_dropdown && !ih->data->has_editbox)
  {
    auto* menu = new BPopUpMenu("");
    int char_h = 0;
    iupdrvFontGetCharSize(ih, nullptr, &char_h);
    int field_h = char_h + 10;
    auto* field = new BMenuField(BRect(0, 0, 99, field_h - 1), "iup_list", nullptr, menu, true, B_FOLLOW_NONE);
    ih->handle = reinterpret_cast<InativeHandle*>(field);
    iuphaikuAddToParent(ih);

    /* once attached every BView call needs the window lock, a map after Show runs on a live looper */
    LooperLockGuard guard(field->Looper());

    if (BMenuBar* mb = field->MenuBar())
      mb->AddFilter(new IupHaikuListMenuFilter(ih));
    iuphaikuUpdateWidgetFont(ih, field);
    iupListSetInitialItems(ih);
    return IUP_NOERROR;
  }

  if (ih->data->is_dropdown && ih->data->has_editbox)
  {
    auto* cont = new IupHaikuListContainer(ih, true);
    auto* edit = new IupHaikuListEditCtrl(ih);

    auto* open_msg = new BMessage(IUPHAIKU_LIST_OPENMENU);
    auto* chevron = new BButton(BRect(0, 0, 0, 0), "iup_list_chev", "v", open_msg, B_FOLLOW_NONE);
    auto* popup = new BPopUpMenu("iup_list_popup", false, false);

    cont->AddChild(edit);
    cont->AddChild(chevron);
    cont->SetEdit(edit);
    cont->SetChevron(chevron, popup);

    ih->handle = reinterpret_cast<InativeHandle*>(cont);
    iupAttribSet(ih, "_IUPHAIKU_LIST_EDIT", reinterpret_cast<char*>(edit));
    iupAttribSet(ih, "_IUPHAIKU_LIST_POPUP", reinterpret_cast<char*>(popup));

    iuphaikuAddToParent(ih);
    iuphaikuUpdateWidgetFont(ih, edit);

    {
      LooperLockGuard guard(cont->Looper());
      chevron->SetTarget(BMessenger(cont));
      cont->Layout();
    }
    iupListSetInitialItems(ih);
    return IUP_NOERROR;
  }

  if (!ih->data->is_dropdown && ih->data->has_editbox)
  {
    auto* cont = new IupHaikuListContainer(ih, false);
    auto* edit = new IupHaikuListEditCtrl(ih);

    list_view_type t = ih->data->is_multiple ? B_MULTIPLE_SELECTION_LIST : B_SINGLE_SELECTION_LIST;
    auto* lv = new IupHaikuListView(ih, t);
    auto* sv = new BScrollView("iup_list_scroll", lv, B_FOLLOW_NONE,
                                      B_WILL_DRAW | B_FRAME_EVENTS, false, true);

    cont->AddChild(edit);
    cont->AddChild(sv);
    cont->SetEdit(edit);
    cont->SetScrollList(sv, lv);

    ih->handle = reinterpret_cast<InativeHandle*>(cont);
    iupAttribSet(ih, "_IUPHAIKU_LIST_EDIT", reinterpret_cast<char*>(edit));
    iupAttribSet(ih, "_IUPHAIKU_LIST_INNER", reinterpret_cast<char*>(lv));

    iuphaikuAddToParent(ih);
    iuphaikuUpdateWidgetFont(ih, edit);
    iuphaikuUpdateWidgetFont(ih, lv);
    {
      LooperLockGuard guard(cont->Looper());
      cont->Layout();
    }
    iupListSetInitialItems(ih);
    return IUP_NOERROR;
  }

  list_view_type t = ih->data->is_multiple ? B_MULTIPLE_SELECTION_LIST : B_SINGLE_SELECTION_LIST;
  auto* lv = new IupHaikuListView(ih, t);
  auto* sb = new BScrollBar(BRect(0, 0, 14, 0), "_vsb_", lv, 0, 1000, B_VERTICAL);
  auto* wrap = new IupHaikuListWrap(ih);
  wrap->Attach(lv, sb);
  ih->handle = reinterpret_cast<InativeHandle*>(wrap);
  iupAttribSet(ih, "_IUPHAIKU_LIST_INNER", reinterpret_cast<char*>(lv));

  iuphaikuAddToParent(ih);
  iuphaikuUpdateWidgetFont(ih, lv);
  iupListSetInitialItems(ih);

  /* VIRTUALMODE: pre-Map ITEMCOUNT setter only stashed the count, realize placeholders now. */
  if (ih->data->is_virtual && ih->data->item_count > 0)
    iupdrvListSetItemCount(ih, ih->data->item_count);

  IupSetCallback(ih, "_IUP_XY2POS_CB", reinterpret_cast<Icallback>(haikuListConvertXYToPos));
  return IUP_NOERROR;
}

static void haikuListUnMapMethod(Ihandle* ih)
{
  auto* lv = reinterpret_cast<IupHaikuListView*>(iupAttribGet(ih, "_IUPHAIKU_LIST_INNER"));
  if (lv) lv->SetIhandle(nullptr);

  auto* e = reinterpret_cast<IupHaikuListEditCtrl*>(iupAttribGet(ih, "_IUPHAIKU_LIST_EDIT"));
  if (e) e->SetIhandle(nullptr);

  IupHaikuListDropHandler* h = haikuListGetDropHandler(ih);
  if (h && be_app)
  {
    h->SetIhandle(nullptr);
    {
      LooperLockGuard guard(be_app);
      be_app->RemoveHandler(h);
    }
    delete h;
  }

  /* DROPDOWN+EDITBOX owns the BPopUpMenu standalone (not auto-deleted by BView). */
  if (ih->data->is_dropdown && ih->data->has_editbox)
  {
    auto* popup = reinterpret_cast<BPopUpMenu*>(iupAttribGet(ih, "_IUPHAIKU_LIST_POPUP"));
    delete popup;
  }

  iupAttribSet(ih, "_IUPHAIKU_LIST_INNER", nullptr);
  iupAttribSet(ih, "_IUPHAIKU_LIST_EDIT", nullptr);
  iupAttribSet(ih, "_IUPHAIKU_LIST_POPUP", nullptr);
  iupAttribSet(ih, "_IUPHAIKU_LIST_DROPHANDLER", nullptr);

  /* Drop handler frees this on completion; destroy mid-drag skips that path. */
  if (auto* c = reinterpret_cast<BCursor*>(iupAttribGet(ih, "_IUPHAIKU_LIST_REORDER_BCURSOR")))
  {
    delete c;
    iupAttribSet(ih, "_IUPHAIKU_LIST_REORDER_BCURSOR", nullptr);
  }

  iupdrvBaseUnMapMethod(ih);
}

static int haikuListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToInt(value, &ih->data->spacing)) ih->data->spacing = 0;
  if (!ih->data->is_dropdown)
  {
    if (IupHaikuListView* lv = haikuListGetListView(ih))
    {
      LooperLockGuard guard(lv->Looper());
      lv->InvalidateLayout(true);
      lv->Invalidate();
    }
  }
  return 1;
}

extern "C" IUP_SDK_API void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = haikuListMapMethod;
  ic->UnMap = haikuListUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, haikuListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, haikuListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuListSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", haikuListGetValueAttrib, haikuListSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", nullptr, haikuListSetTopItemAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VISIBLEITEMS", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, haikuListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  /* BPopUpMenu auto-sizes to its widest item; not controllable */
  iupClassRegisterAttribute(ic, "DROPEXPAND", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  /* BMenuField has no programmatic open; a tracking BPopUpMenu has no clean dismiss */
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IDVALUE", haikuListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", nullptr, haikuListSetImageAttribId, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", haikuListGetImageNativeHandleAttribId, nullptr, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);
}

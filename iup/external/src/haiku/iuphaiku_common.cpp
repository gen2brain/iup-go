/** \file
 * \brief Haiku Driver Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <Control.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <MenuField.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <Rect.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_dialog.h"
#include "iup_dlglist.h"
#include "iup_image.h"
}

#include "iuphaiku_drv.h"


/* Native Container - plain BView, no BLayout (IUP positions children itself). */
IUP_DRV_API BView* iuphaikuNativeContainerNew(void)
{
  BView* container = new BView(BRect(0, 0, 0, 0), "iup_container",
                               B_FOLLOW_NONE, B_WILL_DRAW);
  container->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  return container;
}

IUP_DRV_API void iuphaikuNativeContainerAdd(BView* container, BView* widget)
{
  if (!container || !widget)
    return;

  LooperLockGuard guard(container->Looper());
  container->AddChild(widget);
}

/* Widget Management */

IUP_DRV_API void iuphaikuAddToParent(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  BView* widget = (BView*)ih->handle;
  BView* parent = (BView*)iupChildTreeGetNativeParentHandle(ih);
  if (!parent)
    return;

  LooperLockGuard guard(parent->Looper());
  parent->AddChild(widget);

  /* BView cursor isn't hierarchical; cascade dialog CURSOR onto each new descendant */
  for (Ihandle* a = ih->parent; a; a = a->parent)
  {
    if (a->iclass && a->iclass->nativetype == IUP_TYPEDIALOG)
    {
      if (BCursor* c = (BCursor*)iupAttribGet(a, "_IUPHAIKU_CURSOR"))
        widget->SetViewCursor(c, true);
      break;
    }
  }
}

IUP_DRV_API void iuphaikuSetPosSize(BView* widget, int x, int y, int width, int height)
{
  if (!widget)
    return;

  if (width  < 1) width  = 1;
  if (height < 1) height = 1;

  LooperLockGuard guard(widget->Looper());
  widget->MoveTo((float)x, (float)y);
  widget->ResizeTo((float)(width - 1), (float)(height - 1));
}

IUP_DRV_API BWindow* iuphaikuGetParentWindow(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  return (BWindow*)parent;
}

/* Stub Map - placeholder BView so AddToParent/Layout/UnMap work uniformly. */
IUP_DRV_API int iuphaikuStubMap(Ihandle* ih)
{
  BView* view = new BView(BRect(0, 0, 0, 0), "iup_stub", B_FOLLOW_NONE, B_WILL_DRAW);
  view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

  ih->handle = (InativeHandle*)view;
  iuphaikuAddToParent(ih);
  return IUP_NOERROR;
}

IUP_DRV_API void iuphaikuStubComputeNaturalSize(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)ih;
  (void)children_expand;
  if (w && *w == 0) *w = 1;
  if (h && *h == 0) *h = 1;
}

/* Coordinate Conversion */

extern "C" IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  if (!ih || !ih->handle) return;

  if (ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    BWindow* win = (BWindow*)ih->handle;
    LooperLockGuard guard(win);
    BRect frame = win->Frame();
    if (x) *x -= (int)frame.left;
    if (y) *y -= (int)frame.top;
    return;
  }

  BView* view = (BView*)ih->handle;
  LooperLockGuard guard(view->Looper());
  BPoint origin = view->ConvertToScreen(BPoint(0, 0));
  if (x) *x -= (int)origin.x;
  if (y) *y -= (int)origin.y;
}

extern "C" IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  if (!ih || !ih->handle) return;

  if (ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    BWindow* win = (BWindow*)ih->handle;
    LooperLockGuard guard(win);
    BRect frame = win->Frame();
    if (x) *x += (int)frame.left;
    if (y) *y += (int)frame.top;
    return;
  }

  BView* view = (BView*)ih->handle;
  LooperLockGuard guard(view->Looper());
  BPoint origin = view->ConvertToScreen(BPoint(0, 0));
  if (x) *x += (int)origin.x;
  if (y) *y += (int)origin.y;
}

/* Visibility / Active State */

extern "C" IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return 0;

  if (ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG)
    return iupdrvDialogIsVisible(ih);

  BView* view = (BView*)ih->handle;
  LooperLockGuard guard(view->Looper());
  return view->IsHidden() ? 0 : 1;
}

extern "C" IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  if (!ih || !ih->handle) return 1;
  if (ih->iclass && (ih->iclass->nativetype == IUP_TYPEVOID ||
                     ih->iclass->nativetype == IUP_TYPEMENU ||
                     ih->iclass->nativetype == IUP_TYPEDIALOG))
    return 1;

  BView* view = (BView*)ih->handle;
  BControl* ctrl = dynamic_cast<BControl*>(view);
  if (ctrl)
  {
    LooperLockGuard guard(view->Looper());
    return ctrl->IsEnabled() ? 1 : 0;
  }
  return iupAttribGet(ih, "_IUPHAIKU_INACTIVE") ? 0 : 1;
}

extern "C" IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int enable)
{
  if (!ih || !ih->handle) return;
  if (ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG) return;

  BView* view = (BView*)ih->handle;
  LooperLockGuard guard(view->Looper());
  /* IsHidden(view) checks this view's own counter; bare IsHidden() walks parents and reports hidden during Map. */
  if (enable) {
    if (view->IsHidden(view)) view->Show();
  } else {
    if (!view->IsHidden(view)) view->Hide();
  }
}

static void haikuSetActiveSelf(Ihandle* ih, int enable)
{
  if (!ih->handle) return;
  if (ih->iclass && (ih->iclass->nativetype == IUP_TYPEVOID ||
                     ih->iclass->nativetype == IUP_TYPEMENU ||
                     ih->iclass->nativetype == IUP_TYPEDIALOG))
    return;

  BView* view = (BView*)ih->handle;
  LooperLockGuard guard(view->Looper());

  BControl* ctrl = dynamic_cast<BControl*>(view);
  if (ctrl)
  {
    ctrl->SetEnabled(enable ? true : false);
    return;
  }

  BMenuField* field = dynamic_cast<BMenuField*>(view);
  if (field)
    field->SetEnabled(enable ? true : false);

  iupAttribSet(ih, "_IUPHAIKU_INACTIVE", enable ? NULL : (char*)"1");

  view->Invalidate();
}

/* BViews don't inherit the disabled state; cascade it, honoring each child's ACTIVE. */
extern "C" IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  if (!ih) return;

  haikuSetActiveSelf(ih, enable);

  for (Ihandle* child = ih->firstchild; child; child = child->brother)
  {
    char* a = iupAttribGet(child, "ACTIVE");
    int child_enable = enable && !(a && !iupStrBoolean(a));
    iupdrvSetActive(child, child_enable);
  }
}

extern "C" IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  if (!ih || !ih->handle) return;
  BView* view = (BView*)ih->handle;
  BControl* ctrl = dynamic_cast<BControl*>(view);
  if (!ctrl) return;
  LooperLockGuard guard(view->Looper());
  ctrl->Invoke();
}

extern "C" IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih)
{
  if (!ih || !ih->handle) return;
  if (ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG) return;

  BView* view = (BView*)ih->handle;
  BLooper* loop = view->Looper();
  if (!loop) return;

  LooperLockGuard guard(loop);
  view->Invalidate();
}

extern "C" IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih)
{
  if (!ih || !ih->handle) return;
  if (ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG) return;

  BView* view = (BView*)ih->handle;
  BWindow* win = view->Window();
  if (!win) return;

  LooperLockGuard guard(win);
  view->Invalidate();
  win->UpdateIfNeeded();
}

extern "C" IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  if (!ih || !ih->handle) return;

  BView* view = (BView*)ih->handle;
  BView* new_parent = (BView*)iupChildTreeGetNativeParentHandle(ih);
  BView* old_parent = view->Parent();

  if (!new_parent || new_parent == old_parent)
    return;

  if (old_parent)
  {
    LooperLockGuard guard(old_parent->Looper());
    view->RemoveSelf();
  }

  LooperLockGuard guard(new_parent->Looper());
  new_parent->AddChild(view);
}

extern "C" IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  if (be_control_look)
    return (int)(be_control_look->GetScrollBarWidth(B_VERTICAL) + 0.5f);
  return 14;
}

/* Walk parent's view tree top-down and return the deepest descendant whose
 * frame contains pt (which is given in parent's coord system). */
static BView* haikuViewAtPoint(BView* parent, BPoint pt)
{
  if (!parent) return NULL;
  for (int32 i = parent->CountChildren() - 1; i >= 0; --i)
  {
    BView* child = parent->ChildAt(i);
    if (!child || child->IsHidden(child)) continue;
    BRect frame = child->Frame();
    if (!frame.Contains(pt)) continue;
    BPoint child_pt = pt - frame.LeftTop();
    BView* leaf = haikuViewAtPoint(child, child_pt);
    return leaf ? leaf : child;
  }
  return NULL;
}

static BWindow* haikuWindowAtPoint(BPoint screen)
{
  for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    if (!dlg->handle) continue;
    BWindow* win = (BWindow*)dlg->handle;
    if (!win->IsHidden() && win->Frame().Contains(screen)) return win;
  }
  return NULL;
}

IUP_DRV_API void iuphaikuFireGlobalInputCB(BMessage* msg)
{
  switch (msg->what)
  {
    case B_MOUSE_DOWN:
    case B_MOUSE_UP:
    {
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (!cb) return;
      BPoint where;
      if (msg->FindPoint("screen_where", &where) != B_OK) return;
      int32 buttons = 0, mods = 0, clicks = 1;
      msg->FindInt32("buttons", &buttons);
      msg->FindInt32("modifiers", &mods);
      msg->FindInt32("clicks", &clicks);
      int btn = 0;
      if      (buttons & B_PRIMARY_MOUSE_BUTTON)   btn = IUP_BUTTON1;
      else if (buttons & B_SECONDARY_MOUSE_BUTTON) btn = IUP_BUTTON3;
      else if (buttons & B_TERTIARY_MOUSE_BUTTON)  btn = IUP_BUTTON2;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, clicks == 2 ? 1 : 0);
      cb(btn, msg->what == B_MOUSE_DOWN ? 1 : 0, (int)where.x, (int)where.y, status);
      break;
    }
    case B_MOUSE_MOVED:
    {
      IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
      if (!cb) return;
      BPoint where;
      if (msg->FindPoint("screen_where", &where) != B_OK) return;
      int32 buttons = 0, mods = 0;
      msg->FindInt32("buttons", &buttons);
      msg->FindInt32("modifiers", &mods);
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, 0);
      cb((int)where.x, (int)where.y, status);
      break;
    }
    case B_MOUSE_WHEEL_CHANGED:
    {
      IFfiis cb = (IFfiis)IupGetFunction("GLOBALWHEEL_CB");
      if (!cb) return;
      float delta = 0.0f;
      msg->FindFloat("be:wheel_delta_y", &delta);
      int32 mods = 0;
      msg->FindInt32("modifiers", &mods);
      BPoint screen(0, 0);
      get_mouse(&screen, NULL);
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iuphaikuButtonKeySetStatus((unsigned)mods, 0, 0, status, 0);
      cb(-delta, (int)screen.x, (int)screen.y, status);
      break;
    }
    case B_KEY_DOWN:
    case B_KEY_UP:
    {
      IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
      if (!cb) return;
      int32 byte_val = 0, raw_char = 0, mods = 0;
      msg->FindInt32("byte", &byte_val);
      msg->FindInt32("raw_char", &raw_char);
      msg->FindInt32("modifiers", &mods);
      int code = iuphaikuKeyDecode((int)byte_val, (int)raw_char, (unsigned)mods);
      if (code) cb(code, msg->what == B_KEY_DOWN ? 1 : 0);
      break;
    }
  }
}

extern "C" IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  Ihandle* focus = IupGetFocus();
  if (!focus || !focus->handle) return;
  BView* target = (BView*)focus->handle;
  if (target == (BView*)-1) return;

  unsigned int byte_val = 0, state = 0;
  iupdrvKeyEncode(key, &byte_val, &state);
  if (!byte_val) return;

  char buf[2] = { (char)byte_val, 0 };

  if (press & 0x01)
  {
    BMessage msg(B_KEY_DOWN);
    msg.AddInt64("when", system_time());
    msg.AddInt32("modifiers", (int32)state);
    msg.AddInt8("byte", (int8)byte_val);
    msg.AddString("bytes", buf);
    msg.AddInt32("raw_char", (int32)byte_val);
    msg.AddInt32("key", (int32)byte_val);
    BMessenger(target).SendMessage(&msg);
  }
  if (press & 0x02)
  {
    BMessage msg(B_KEY_UP);
    msg.AddInt64("when", system_time());
    msg.AddInt32("modifiers", (int32)state);
    msg.AddInt8("byte", (int8)byte_val);
    msg.AddString("bytes", buf);
    msg.AddInt32("raw_char", (int32)byte_val);
    msg.AddInt32("key", (int32)byte_val);
    BMessenger(target).SendMessage(&msg);
  }
}

extern "C" IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  iupdrvWarpPointer(x, y);

  BPoint screen(x, y);
  BWindow* win = haikuWindowAtPoint(screen);
  if (!win) return;

  if (!win->Lock()) return;

  BView* target = NULL;
  if (win->ChildAt(0))
  {
    BView* root = win->ChildAt(0);
    BPoint win_pt = screen - win->Frame().LeftTop();
    BPoint root_pt = win_pt - root->Frame().LeftTop();
    target = haikuViewAtPoint(root, root_pt);
    if (!target) target = root;
  }
  if (!target) { win->Unlock(); return; }

  /* Drop the lock before SendMessage; the dialog's looper needs it to drain
     the port and SendMessage under lock deadlocks if the port fills. */
  BPoint view_pt = target->ConvertFromScreen(screen);
  BMessenger msgr(target);
  win->Unlock();

  if (bt == 'W')
  {
    BMessage msg(B_MOUSE_WHEEL_CHANGED);
    msg.AddInt64("when", system_time());
    msg.AddFloat("be:wheel_delta_x", 0.0f);
    msg.AddFloat("be:wheel_delta_y", (float)-status);
    msgr.SendMessage(&msg);
    return;
  }

  uint32 buttons = 0;
  switch (bt)
  {
    case IUP_BUTTON1: buttons = B_PRIMARY_MOUSE_BUTTON;   break;
    case IUP_BUTTON2: buttons = B_TERTIARY_MOUSE_BUTTON;  break;
    case IUP_BUTTON3: buttons = B_SECONDARY_MOUSE_BUTTON; break;
    default: return;
  }

  BMessage msg;
  if (status == -1)
  {
    msg.what = B_MOUSE_MOVED;
  }
  else if (status == 0)
  {
    msg.what = B_MOUSE_UP;
    buttons = 0;
  }
  else
  {
    msg.what = B_MOUSE_DOWN;
  }

  msg.AddInt64("when", system_time());
  msg.AddPoint("where", view_pt);
  msg.AddPoint("be:view_where", view_pt);
  msg.AddInt32("buttons", (int32)buttons);
  msg.AddInt32("modifiers", (int32)modifiers());
  if (msg.what == B_MOUSE_DOWN)
    msg.AddInt32("clicks", (status == 2) ? 2 : 1);

  msgr.SendMessage(&msg);
}

extern "C" IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  static BMessenger sInputServer;
  if (!sInputServer.IsValid())
    sInputServer = BMessenger("application/x-vnd.Be-input_server", -1, NULL);
  BMessage cmd('Ismp');  /* IS_SET_MOUSE_POSITION */
  BMessage reply;
  cmd.AddPoint("where", BPoint(x, y));
  sInputServer.SendMessage(&cmd, &reply, 5000000LL, 5000000LL);
}

extern "C" IUP_SDK_API void iupdrvSleep(int time)
{
  snooze((bigtime_t)time * 1000);
}

extern "C" IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  (void)ih;
  (void)title;
}

extern "C" IUP_SDK_API void iupdrvSetAccessibleDescription(Ihandle *ih, const char* description)
{
  (void)ih;
  (void)description;
}

/* Base Class Methods */

extern "C" IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  if (!ih || !ih->handle)
    return;

  BView* view = (BView*)ih->handle;
  iuphaikuSetPosSize(view, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

extern "C" IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  BView* view = (BView*)ih->handle;
  BWindow* window = view->Window();

  if (window)
  {
    if (window->Lock())
    {
      view->RemoveSelf();
      window->Unlock();
    }
  }

  delete view;
  ih->handle = NULL;
}

extern "C" IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || !value)
    return 1;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 1;

  BView* view = (BView*)ih->handle;
  rgb_color color = { r, g, b, 255 };

  LooperLockGuard guard(view->Looper());
  view->SetViewColor(color);
  view->Invalidate();
  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || !value)
    return 1;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 1;

  BView* view = (BView*)ih->handle;
  rgb_color color = { r, g, b, 255 };

  LooperLockGuard guard(view->Looper());
  view->SetHighColor(color);
  view->Invalidate();
  return 1;
}

static bool haikuCursorIdForName(const char* name, BCursorID* out)
{
  static const struct { const char* iup; BCursorID hk; } table[] = {
    { "NONE",           B_CURSOR_ID_NO_CURSOR },
    { "NULL",           B_CURSOR_ID_NO_CURSOR },
    { "ARROW",          B_CURSOR_ID_SYSTEM_DEFAULT },
    { "BUSY",           B_CURSOR_ID_PROGRESS },
    { "WAIT",           B_CURSOR_ID_PROGRESS },
    { "APPSTARTING",    B_CURSOR_ID_PROGRESS },
    { "CROSS",          B_CURSOR_ID_CROSS_HAIR },
    { "PEN",            B_CURSOR_ID_CROSS_HAIR },
    { "HAND",           B_CURSOR_ID_FOLLOW_LINK },
    { "HELP",           B_CURSOR_ID_HELP },
    { "IUP",            B_CURSOR_ID_HELP },
    { "MOVE",           B_CURSOR_ID_MOVE },
    { "NO",             B_CURSOR_ID_NOT_ALLOWED },
    { "TEXT",           B_CURSOR_ID_I_BEAM },
    { "UPARROW",        B_CURSOR_ID_SYSTEM_DEFAULT },
    { "RESIZE_N",       B_CURSOR_ID_RESIZE_NORTH },
    { "RESIZE_S",       B_CURSOR_ID_RESIZE_SOUTH },
    { "RESIZE_E",       B_CURSOR_ID_RESIZE_EAST },
    { "RESIZE_W",       B_CURSOR_ID_RESIZE_WEST },
    { "RESIZE_NS",      B_CURSOR_ID_RESIZE_NORTH_SOUTH },
    { "RESIZE_WE",      B_CURSOR_ID_RESIZE_EAST_WEST },
    { "RESIZE_NE",      B_CURSOR_ID_RESIZE_NORTH_EAST },
    { "RESIZE_NW",      B_CURSOR_ID_RESIZE_NORTH_WEST },
    { "RESIZE_SE",      B_CURSOR_ID_RESIZE_SOUTH_EAST },
    { "RESIZE_SW",      B_CURSOR_ID_RESIZE_SOUTH_WEST },
    { "SPLITTER_HORIZ", B_CURSOR_ID_RESIZE_NORTH_SOUTH },
    { "SPLITTER_VERT",  B_CURSOR_ID_RESIZE_EAST_WEST },
  };
  for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
    if (iupStrEqualNoCase(name, table[i].iup)) { *out = table[i].hk; return true; }
  return false;
}

IUP_DRV_API BCursor* iuphaikuGetCursor(Ihandle* ih, const char* name, bool* owned)
{
  (void)ih;
  if (owned) *owned = false;
  if (!name) return NULL;
  BCursorID id;
  if (haikuCursorIdForName(name, &id))
  {
    if (owned) *owned = true;
    return new BCursor(id);
  }
  return (BCursor*)iupImageGetCursor(name);
}

static void haikuApplyCursorRecursive(BView* view, BCursor* cursor)
{
  if (!view) return;
  view->SetViewCursor(cursor, true);
  for (int32 i = 0; i < view->CountChildren(); ++i)
    haikuApplyCursorRecursive(view->ChildAt(i), cursor);
}

extern "C" IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle) return 1;
  bool is_dialog = ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG;
  BView* view = is_dialog ? iuphaikuDialogRootView((BWindow*)ih->handle) : (BView*)ih->handle;
  if (!view) return 1;

  BCursor* prev = (BCursor*)iupAttribGet(ih, "_IUPHAIKU_CURSOR");
  bool prev_owned = iupAttribGet(ih, "_IUPHAIKU_CURSOR_OWNED") != NULL;

  bool owned = false;
  BCursor* cursor = iuphaikuGetCursor(ih, value, &owned);
  if (!cursor) { cursor = new BCursor(B_CURSOR_ID_SYSTEM_DEFAULT); owned = true; }

  iupAttribSet(ih, "_IUPHAIKU_CURSOR", (char*)cursor);
  iupAttribSet(ih, "_IUPHAIKU_CURSOR_OWNED", owned ? "1" : NULL);

  LooperLockGuard guard(view->Looper());
  if (is_dialog)
    haikuApplyCursorRecursive(view, cursor);
  else
    view->SetViewCursor(cursor, true);
  if (prev_owned) delete prev;
  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle) return 1;
  BView* view = (ih->iclass && ih->iclass->nativetype == IUP_TYPEDIALOG) ? iuphaikuDialogRootView((BWindow*)ih->handle) : (BView*)ih->handle;
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());
  view->SetToolTip(value);
  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

extern "C" IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

extern "C" IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

/* Common Attribute Registration */

extern "C" IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  const char* font_id_name = iuphaikuGetNativeFontIdName();
  iupClassRegisterAttribute(ic, font_id_name, NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPICON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACCESSIBLETITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACCESSIBLEDESCRIPTION", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
}

/* Native Handle Access */

IUP_DRV_API char* iuphaikuGetNativeWindowHandleAttrib(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return NULL;
  return (char*)ih->handle;
}

IUP_DRV_API const char* iuphaikuGetNativeWindowHandleName(void)
{
  return "BWINDOW";
}

IUP_DRV_API const char* iuphaikuGetNativeFontIdName(void)
{
  return "BFONT";
}

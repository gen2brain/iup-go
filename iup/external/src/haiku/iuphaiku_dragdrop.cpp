/** \file
 * \brief Haiku Drag&Drop
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

#include <Application.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <InterfaceDefs.h>
#include <Looper.h>
#include <Message.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_key.h"
#include "iup_image.h"
}

#include "iuphaiku_drv.h"


/* Drag threshold in pixels (Haiku Tracker uses 5). */
static const float kDragThreshold = 5.0f;

#define IUPHAIKU_DD_END_REPLY 'IuDe'

static std::vector<std::string> haikuDnDParseTypes(const char* value)
{
  std::vector<std::string> result;
  if (!value || !*value) return result;
  const char* p = value;
  while (*p)
  {
    while (*p == ' ' || *p == '\t' || *p == ',') ++p;
    const char* start = p;
    while (*p && *p != ',') ++p;
    const char* end = p;
    while (end > start && (end[-1] == ' ' || end[-1] == '\t')) --end;
    if (end > start)
      result.emplace_back(start, end - start);
  }
  return result;
}

static bool haikuDnDFindCommonType(const std::vector<std::string>& srcTypes, const std::vector<std::string>& dstTypes, std::string* out)
{
  for (const auto& s : srcTypes)
    for (const auto& d : dstTypes)
      if (iupStrEqualNoCase(s.c_str(), d.c_str())) { *out = s; return true; }
  return false;
}

static BBitmap* haikuDnDLoadCursorBitmap(Ihandle* ih, const char* name)
{
  if (!name) return NULL;
  BBitmap* bm = (BBitmap*)iupImageGetImage(name, ih, 0, NULL);
  if (!bm) return NULL;
  /* iupImageGetImage caches; copy so the drag's delete doesn't free the cache entry. */
  return new BBitmap(bm);
}

static void haikuDnDClearTracking(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPHAIKU_DD_TRACK", NULL);
  iupAttribSet(ih, "_IUPHAIKU_DD_X", NULL);
  iupAttribSet(ih, "_IUPHAIKU_DD_Y", NULL);
}

void iuphaikuDnDMouseDown(Ihandle* ih, BPoint where, unsigned int buttons)
{
  if (!ih) return;
  if (!iupAttribGetBoolean(ih, "DRAGSOURCE")) return;
  if (!(buttons & B_PRIMARY_MOUSE_BUTTON)) return;
  iupAttribSetInt(ih, "_IUPHAIKU_DD_TRACK", 1);
  iupAttribSetInt(ih, "_IUPHAIKU_DD_X", (int)where.x);
  iupAttribSetInt(ih, "_IUPHAIKU_DD_Y", (int)where.y);
}

void iuphaikuDnDMouseUp(Ihandle* ih)
{
  if (!ih) return;
  haikuDnDClearTracking(ih);
}

bool iuphaikuDnDInitiateDrag(Ihandle* ih, BView* view, BPoint where)
{
  IFnii cbBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if (cbBegin && cbBegin(ih, (int)where.x, (int)where.y) == IUP_IGNORE)
    return false;

  std::vector<std::string> types = haikuDnDParseTypes(iupAttribGet(ih, "DRAGTYPES"));
  if (types.empty()) return false;

  BMessage drag(B_MIME_DATA);
  drag.AddPointer("be:originator", ih);
  drag.AddInt32("_iup_team", be_app ? be_app->Team() : 0);

  IFns cbSize = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
  IFnsVi cbData = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
  if (!cbSize || !cbData) return false;

  bool any = false;
  for (const std::string& t : types)
  {
    int size = cbSize(ih, (char*)t.c_str());
    if (size <= 0) continue;
    void* buf = malloc(size);
    if (!buf) continue;
    memset(buf, 0, size);
    cbData(ih, (char*)t.c_str(), buf, size);
    drag.AddData(t.c_str(), B_MIME_TYPE, buf, size);
    free(buf);
    any = true;
  }
  if (!any) return false;

  int drag_modes = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ? 0x3 : 0x1; /* bit0 copy, bit1 move */
  drag.AddInt32("be:actions", drag_modes);
  drag.AddString("_iup_dragtypes", iupAttribGet(ih, "DRAGTYPES"));

  /* Cursor bitmap: DRAGCURSOR (per spec). DragMessage takes ownership of the bitmap. */
  BBitmap* dragImg = haikuDnDLoadCursorBitmap(ih, iupAttribGet(ih, "DRAGCURSOR"));

  /* The reply messenger lets the target send back IUPHAIKU_DD_END_REPLY. */
  BHandler* replyTo = view;

  if (dragImg)
  {
    BPoint offset(dragImg->Bounds().Width() / 2, dragImg->Bounds().Height() / 2);
    view->DragMessage(&drag, dragImg, B_OP_BLEND, offset, replyTo);
  }
  else
  {
    BRect dragRect(where.x - 8, where.y - 8, where.x + 8, where.y + 8);
    view->DragMessage(&drag, dragRect, replyTo);
  }

  haikuDnDClearTracking(ih);
  return true;
}

static void haikuDnDApplyDragCursor(Ihandle* ih, BView* view, const BMessage* drag_msg, int32 mods)
{
  int32 team = 0;
  drag_msg->FindInt32("_iup_team", &team);
  if (!be_app || team != be_app->Team()) return;
  Ihandle* src_ih = NULL;
  drag_msg->FindPointer("be:originator", (void**)&src_ih);
  if (!src_ih || !iupObjectCheck(src_ih)) return;

  const char* name = (mods & B_CONTROL_KEY) ? iupAttribGet(src_ih, "DRAGCURSORCOPY") : NULL;
  if (!name) name = iupAttribGet(src_ih, "DRAGCURSOR");
  if (!name) return;

  const char* last = iupAttribGet(ih, "_IUPHAIKU_DD_CURSOR_NAME");
  if (last && iupStrEqual(last, name)) return;

  BCursor* prev = (BCursor*)iupAttribGet(ih, "_IUPHAIKU_DD_CURSOR");
  bool prev_owned = iupAttribGet(ih, "_IUPHAIKU_DD_CURSOR_OWNED") != NULL;

  bool owned = false;
  BCursor* c = iuphaikuGetCursor(ih, name, &owned);
  if (!c) return;

  view->SetViewCursor(c, true);
  iupAttribSetStr(ih, "_IUPHAIKU_DD_CURSOR_NAME", name);
  iupAttribSet(ih, "_IUPHAIKU_DD_CURSOR", (char*)c);
  iupAttribSet(ih, "_IUPHAIKU_DD_CURSOR_OWNED", owned ? "1" : NULL);
  if (prev_owned) delete prev;
}

static void haikuDnDClearDragCursor(Ihandle* ih)
{
  BCursor* prev = (BCursor*)iupAttribGet(ih, "_IUPHAIKU_DD_CURSOR");
  if (!prev) return;
  bool prev_owned = iupAttribGet(ih, "_IUPHAIKU_DD_CURSOR_OWNED") != NULL;

  /* Restore whatever the widget had set via CURSOR (or system default). */
  if (BView* view = (BView*)ih->handle)
  {
    BCursor* base = (BCursor*)iupAttribGet(ih, "_IUPHAIKU_CURSOR");
    LooperLockGuard guard(view->Looper());
    view->SetViewCursor(base ? base : B_CURSOR_SYSTEM_DEFAULT, true);
  }

  if (prev_owned) delete prev;
  iupAttribSet(ih, "_IUPHAIKU_DD_CURSOR_NAME", NULL);
  iupAttribSet(ih, "_IUPHAIKU_DD_CURSOR", NULL);
  iupAttribSet(ih, "_IUPHAIKU_DD_CURSOR_OWNED", NULL);
}

bool iuphaikuDnDMouseMoved(Ihandle* ih, BView* view, BPoint where, unsigned int transit, const BMessage* drag_msg)
{
  if (!ih || !view) return false;

  if (!drag_msg && iupAttribGet(ih, "_IUPHAIKU_DD_CURSOR"))
    haikuDnDClearDragCursor(ih);

  if (drag_msg && iupAttribGetBoolean(ih, "DROPTARGET"))
  {
    int32 mods = 0;
    const BMessage* cur = view->Looper() ? view->Looper()->CurrentMessage() : NULL;
    if (cur) cur->FindInt32("modifiers", &mods);

    if (transit == B_EXITED_VIEW)
      haikuDnDClearDragCursor(ih);
    else
      haikuDnDApplyDragCursor(ih, view, drag_msg, mods);

    IFniis cbMotion = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
    if (cbMotion)
    {
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iuphaikuButtonKeySetStatus((unsigned)mods, 0, 0, status, 0);
      cbMotion(ih, (int)where.x, (int)where.y, status);
    }
    return true;
  }

  if (!iupAttribGetInt(ih, "_IUPHAIKU_DD_TRACK")) return false;

  int sx = iupAttribGetInt(ih, "_IUPHAIKU_DD_X");
  int sy = iupAttribGetInt(ih, "_IUPHAIKU_DD_Y");
  float dx = where.x - sx;
  float dy = where.y - sy;
  if (dx * dx + dy * dy < kDragThreshold * kDragThreshold) return false;

  return iuphaikuDnDInitiateDrag(ih, view, where);
}

static int haikuDnDActionFromModifiers(Ihandle* ih, int32 mods)
{
  /* IUP semantics: CTRL forces copy, SHIFT forces move. Default follows DRAGSOURCEMOVE. */
  bool can_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE");
  if (mods & B_CONTROL_KEY) return 0;
  if (mods & B_SHIFT_KEY) return can_move ? 1 : 0;
  return can_move ? 1 : 0;
}

bool iuphaikuDnDMessageReceived(Ihandle* ih, BView* view, BMessage* msg)
{
  if (!ih || !msg) return false;

  if (msg->what == IUPHAIKU_DD_END_REPLY)
  {
    IFni cbEnd = (IFni)IupGetCallback(ih, "DRAGEND_CB");
    if (cbEnd)
    {
      int action = -1;
      msg->FindInt32("action", (int32*)&action);
      cbEnd(ih, action);
    }
    return true;
  }

  if (!msg->WasDropped()) return false;
  if (!iupAttribGetBoolean(ih, "DROPTARGET")) return false;

  haikuDnDClearDragCursor(ih);

  std::vector<std::string> srcTypes = haikuDnDParseTypes(msg->FindString("_iup_dragtypes"));
  /* Fallback: enumerate names actually present in the message as B_MIME_TYPE data. */
  if (srcTypes.empty())
  {
    char* name = NULL;
    uint32 type = 0;
    int32 count = 0;
    for (int32 i = 0; msg->GetInfo(B_MIME_TYPE, i, &name, &type, &count) == B_OK; ++i)
      if (name) srcTypes.emplace_back(name);
  }
  std::vector<std::string> dstTypes = haikuDnDParseTypes(iupAttribGet(ih, "DROPTYPES"));
  if (srcTypes.empty() || dstTypes.empty()) return false;

  std::string common;
  if (!haikuDnDFindCommonType(srcTypes, dstTypes, &common)) return false;

  const void* data = NULL;
  ssize_t size = 0;
  if (msg->FindData(common.c_str(), B_MIME_TYPE, &data, &size) != B_OK) return false;

  BPoint dropPt = msg->DropPoint();
  if (view->Looper())
    dropPt = view->ConvertFromScreen(dropPt);

  IFnsViii cbDrop = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  if (cbDrop)
    cbDrop(ih, (char*)common.c_str(), (void*)data, (int)size, (int)dropPt.x, (int)dropPt.y);

  /* Reply for DRAGEND_CB. */
  int32 mods = 0;
  msg->FindInt32("modifiers", &mods);
  int32 team = 0;
  msg->FindInt32("_iup_team", &team);
  Ihandle* src_ih = NULL;
  if (be_app && team == be_app->Team())
    msg->FindPointer("be:originator", (void**)&src_ih);
  if (src_ih && iupObjectCheck(src_ih))
  {
    BMessage reply(IUPHAIKU_DD_END_REPLY);
    reply.AddInt32("action", haikuDnDActionFromModifiers(src_ih, mods));
    msg->SendReply(&reply);
  }
  return true;
}

static int haikuDnDSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrBoolean(value)) haikuDnDClearTracking(ih);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DRAGBEGIN_CB",    "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB",     "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB",      "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB",     "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB",   "iis");
  iupClassRegisterCallback(ic, "DROPFILES_CB",    "siii");

  iupClassRegisterAttribute(ic, "DRAGTYPES",      NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",      NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE",     NULL, haikuDnDSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET",     NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR",     NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP",        NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

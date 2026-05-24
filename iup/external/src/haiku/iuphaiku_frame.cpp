/** \file
 * \brief Haiku Frame Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Box.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_frame.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


/* ih->handle = BBox*; inner BView stored in _IUPHAIKU_FRAME_INNER. */

static const uint32 kFrameFlags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP;

static border_style haikuFrameResolveBorder(Ihandle* ih)
{
  /* IUP convention: BORDER=NO removes the border; default is bordered. */
  char* border = iupAttribGet(ih, "BORDER");
  if (border && !iupStrBoolean(border))
    return B_NO_BORDER;
  return B_FANCY_BORDER;
}

/* Map */

static int haikuFrameMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  char* title = iupAttribGet(ih, "TITLE");
  if (title)
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
  else if (iupAttribGet(ih, "BGCOLOR"))
    iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");

  BBox* box = new BBox(BRect(0, 0, 99, 99), "iup_frame", B_FOLLOW_NONE,
                       kFrameFlags, haikuFrameResolveBorder(ih));
  if (title)
    box->SetLabel(title);

  BView* inner = new BView(box->InnerFrame(), "iup_frame_inner", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
  inner->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  box->AddChild(inner);

  ih->handle = (InativeHandle*)box;
  iupAttribSet(ih, "_IUPHAIKU_FRAME_INNER", (char*)inner);

  iuphaikuAddToParent(ih);
  iuphaikuUpdateWidgetFont(ih, box);

  return IUP_NOERROR;
}

static void haikuFrameUnMapMethod(Ihandle* ih)
{
  /* BBox auto-deletes the inner BView; clear the cached pointer. */
  iupAttribSet(ih, "_IUPHAIKU_FRAME_INNER", NULL);
  iupdrvBaseUnMapMethod(ih);
}

static void* haikuFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* /*child*/)
{
  return iupAttribGet(ih, "_IUPHAIKU_FRAME_INNER");
}

/* Attribute Setters */

static int haikuFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  BBox* box = (BBox*)ih->handle;
  if (value)
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");

  if (!box) return 1;

  LooperLockGuard guard(box->Looper());
  box->SetLabel(value ? value : "");
  return 1;
}

static int haikuFrameSetBorderAttrib(Ihandle* ih, const char* value)
{
  BBox* box = (BBox*)ih->handle;
  if (!box) return 1;
  LooperLockGuard guard(box->Looper());
  box->SetBorder(iupStrBoolean(value) ? B_FANCY_BORDER : B_NO_BORDER);
  return 1;
}

/* Driver hooks */

extern "C" IUP_SDK_API int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  /* Inner BView absorbs border + title offset. */
  (void)ih;
  return 0;
}

extern "C" IUP_SDK_API void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  if (x) *x = 0;
  if (y) *y = 0;
}

extern "C" IUP_SDK_API int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  if (!iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") && !iupAttribGet(ih, "TITLE"))
  {
    if (h) *h = 0;
    return 0;
  }

  BBox* box = (BBox*)ih->handle;
  if (box)
  {
    if (h) *h = (int)(box->TopBorderOffset() + 0.5f);
    return 1;
  }

  /* Pre-map estimate. */
  if (h) *h = 16;
  return 1;
}

extern "C" IUP_SDK_API int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  BBox temp(BRect(0, 0, 99, 99), "iup_frame_probe", B_FOLLOW_NONE, kFrameFlags, haikuFrameResolveBorder(ih));

  const char* title = iupAttribGet(ih, "TITLE");
  if (title && *title)
    temp.SetLabel(title);

  BRect inner = temp.InnerFrame();
  if (w) *w = 100 - (int)(inner.Width() + 1);
  if (h) *h = 100 - (int)(inner.Height() + 1);
  return 1;
}

static int haikuFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return 0;
  return iupdrvBaseSetBgColorAttrib(ih, value);
}

extern "C" IUP_SDK_API void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = haikuFrameMapMethod;
  ic->UnMap = haikuFrameUnMapMethod;
  ic->GetInnerNativeContainerHandle = haikuFrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TITLE", NULL, haikuFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, haikuFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BORDER", NULL, haikuFrameSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* SUNKEN: no native equivalent (BBox styles are FANCY/PLAIN/NONE). */
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

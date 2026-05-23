/** \file
 * \brief Haiku Driver - Common Function Declarations
 *
 * Minimum requirements: Haiku R1/beta4+, C++17.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPHAIKU_DRV_H
#define __IUPHAIKU_DRV_H

#include <Point.h>

#ifndef __IUP_OBJECT_H
#include "iup_object.h"
#endif

class BApplication;
class BWindow;
class BView;
class BLooper;
class BHandler;
class BMessage;
class BMessenger;
class BCursor;
class BFont;
struct rgb_color;

/* IupHaikuApp / IupHaikuWindow MessageReceived 'what' codes. */
#define IUPHAIKU_APP_DRAIN_MSG     'IuPM'  /* drain IupPostMessage queue */
#define IUPHAIKU_APP_IDLE_TICK     'IuIT'  /* idle-cb tick */
#define IUPHAIKU_APP_SHOW_WIN      'IuSW'  /* deferred Show during launch */
#define IUPHAIKU_MENU_ITEM_MSG     'IupM'  /* IupItem ACTION dispatch */
#define IUPHAIKU_MENU_RECENT_MSG   'IuRM'  /* IupConfigRecent item; "menu" Ihandle*, "index" int32 */
#define IUPHAIKU_MENU_CB_MSG       'IumC'  /* menu cb hop to dialog looper; "ih" ptr, "cb" string */
#define IUPHAIKU_POST_MSG          'IupP'  /* IupPostMessage hop; ih, s, i, d, p */
#define IUPHAIKU_TIMER_HOP_MSG     'IutH'  /* IupTimer ACTION_CB hop; "ih" ptr */
#define IUPHAIKU_THEME_CHANGED_MSG 'IuTC'  /* B_COLORS_UPDATED fanout */
#define IUPHAIKU_SI_MSG            'IuSI'  /* second-instance argv dispatch to COPYDATA_CB */
#define IUPHAIKU_MOVE_SETTLED      'IuMS'  /* debounced MOVE_CB after a window drag settles */

/* BApplication singleton (spawned in iupdrvOpen, reaped in iupdrvClose) */
IUP_DRV_API BApplication* iuphaikuGetApplication();

/* Widget management; ih->handle is BView* for controls, BWindow* for dialogs */
IUP_DRV_API void iuphaikuAddToParent(Ihandle* ih);
IUP_DRV_API void iuphaikuSetPosSize(BView* widget, int x, int y, int width, int height);

/* Plain BView container for Frame/Tabs/etc */
IUP_DRV_API BView* iuphaikuNativeContainerNew();
IUP_DRV_API void iuphaikuNativeContainerAdd(BView* container, BView* widget);

IUP_DRV_API BWindow* iuphaikuGetParentWindow(Ihandle* ih);

/* Inner content view of an IupHaikuWindow; NULL if not one. */
IUP_DRV_API BView* iuphaikuDialogRootView(BWindow* win);

IUP_DRV_API BFont* iuphaikuGetBFont(const char* value);
IUP_DRV_API void iuphaikuUpdateWidgetFont(Ihandle* ih, BView* widget);

IUP_DRV_API char* iuphaikuGetNativeWindowHandleAttrib(Ihandle* ih);
IUP_DRV_API const char* iuphaikuGetNativeWindowHandleName();
IUP_DRV_API const char* iuphaikuGetNativeFontIdName();

IUP_DRV_API int iuphaikuIsSystemDarkMode();
IUP_DRV_API void iuphaikuSetGlobalColors();

IUP_DRV_API void iuphaikuLoopCleanup();

/* Wake be_app to re-check the visible-dialog count. */
IUP_DRV_API void iuphaikuPostAppWake();

IUP_DRV_API void iuphaikuAppDrainPosts();
IUP_DRV_API void iuphaikuAppIdleTick();
IUP_DRV_API void iuphaikuAppStartIdleRunner();
IUP_DRV_API void iuphaikuAppStopIdleRunner();

IUP_DRV_API void iuphaikuRecentDispatch(Ihandle* menu, int index);

/* Key decode/encode helpers (used by widget KeyDown / MouseDown overrides).
 * Modifier / button masks are uint32_t to keep this header free of Haiku typedefs. */
IUP_DRV_API int  iuphaikuKeyDecode(int byte, int raw_char, unsigned int modifiers);
IUP_DRV_API void iuphaikuButtonKeySetStatus(unsigned int modifiers, unsigned int buttons, int button, char* status, int doubleclick);

/* Focus helpers. */
IUP_DRV_API void iuphaikuSetCanFocus(BView* widget, int can);
IUP_DRV_API void iuphaikuFocusInOutEvent(Ihandle* ih, int focus_in);

IUP_DRV_API int iuphaikuStubMap(Ihandle* ih);
IUP_DRV_API void iuphaikuStubComputeNaturalSize(Ihandle* ih, int *w, int *h, int *children_expand);

IUP_DRV_API void iuphaikuSingleInstanceDispatch(BMessage* msg);

IUP_DRV_API int iuphaikuLockLooper(BLooper* looper);
IUP_DRV_API void iuphaikuUnlockLooper(BLooper* looper);

IUP_DRV_API void iuphaikuFireGlobalInputCB(BMessage* msg);

/* `*owned` set true when the BCursor was new'd and the caller must delete. */
IUP_DRV_API BCursor* iuphaikuGetCursor(Ihandle* ih, const char* name, bool* owned);

class LooperLockGuard
{
public:
  explicit LooperLockGuard(BLooper* looper)
    : fLooper(looper), fLocked(false)
  {
    if (fLooper)
      fLocked = (iuphaikuLockLooper(fLooper) != 0);
  }
  ~LooperLockGuard()
  {
    if (fLocked && fLooper)
      iuphaikuUnlockLooper(fLooper);
  }
  LooperLockGuard(const LooperLockGuard&) = delete;
  LooperLockGuard& operator=(const LooperLockGuard&) = delete;

private:
  BLooper* fLooper;
  bool fLocked;
};

void iuphaikuDnDMouseDown(Ihandle* ih, BPoint where, unsigned int buttons);
void iuphaikuDnDMouseUp(Ihandle* ih);
bool iuphaikuDnDMouseMoved(Ihandle* ih, BView* view, BPoint where, unsigned int transit, const BMessage* drag_msg);
bool iuphaikuDnDMessageReceived(Ihandle* ih, BView* view, BMessage* msg);
bool iuphaikuDnDInitiateDrag(Ihandle* ih, BView* view, BPoint where);

/* Canvas event handlers, shared by IupCanvas (BView) and IupGLCanvas (BGLView). */
void iuphaikuCanvasOnDraw(Ihandle* ih, BView* view, BRect dirty);
void iuphaikuCanvasOnFrameResized(Ihandle* ih, BView* view, float new_w, float new_h);
void iuphaikuCanvasOnAttachedToWindow(Ihandle* ih, BView* view);
void iuphaikuCanvasOnMakeFocus(Ihandle* ih, BView* view, bool focused);
void iuphaikuCanvasOnMouseDown(Ihandle* ih, BView* view, BPoint where);
void iuphaikuCanvasOnMouseUp(Ihandle* ih, BView* view, BPoint where);
void iuphaikuCanvasOnMouseMoved(Ihandle* ih, BView* view, BPoint where, unsigned int transit, const BMessage* drag);
bool iuphaikuCanvasOnKeyDown(Ihandle* ih, BView* view, const char* bytes, int numBytes);
bool iuphaikuCanvasOnKeyUp(Ihandle* ih, BView* view, const char* bytes, int numBytes);
bool iuphaikuCanvasOnMessageReceived(Ihandle* ih, BView* view, BMessage* msg);

#endif

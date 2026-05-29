/** \file
 * \brief Windows Driver Touch management
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_drv.h"
#include "iup_str.h"
#include "iup_attrib.h"

#include "iupwin_drv.h"


#ifndef WM_TOUCH
#define WM_TOUCH                        0x0240
DECLARE_HANDLE(HTOUCHINPUT);
typedef struct tagTOUCHINPUT {
    LONG x; LONG y; HANDLE hSource; DWORD dwID; DWORD dwFlags; DWORD dwMask;
    DWORD dwTime; ULONG_PTR dwExtraInfo; DWORD cxContact; DWORD cyContact;
} TOUCHINPUT;
#define TOUCHEVENTF_MOVE            0x0001
#define TOUCHEVENTF_DOWN            0x0002
#define TOUCHEVENTF_UP              0x0004
#define TOUCHEVENTF_PRIMARY         0x0010
#endif

static int win_touch_loaded = 0;
static BOOL (WINAPI *winGetTouchInputInfo)(HTOUCHINPUT hTouchInput, UINT cInputs, TOUCHINPUT* pInputs, int cbSize) = NULL;
static BOOL (WINAPI *winCloseTouchInputHandle)(HTOUCHINPUT hTouchInput) = NULL;
static BOOL (WINAPI *winRegisterTouchWindow)(HWND hwnd, ULONG ulFlags) = NULL;
static BOOL (WINAPI *winUnregisterTouchWindow)(HWND hwnd) = NULL;
static BOOL (WINAPI *winIsTouchWindow)(HWND hwnd, PULONG pulFlags) = NULL;

IUP_DRV_API void iupwinTouchInit(void)
{
  HINSTANCE lib = LoadLibrary(TEXT("user32"));

  winGetTouchInputInfo = (BOOL (WINAPI *)(HTOUCHINPUT,UINT,TOUCHINPUT *,int))GetProcAddress(lib, "GetTouchInputInfo");
  winCloseTouchInputHandle = (BOOL (WINAPI *)(HTOUCHINPUT))GetProcAddress(lib, "CloseTouchInputHandle");
  winRegisterTouchWindow = (BOOL (WINAPI *)(HWND,ULONG))GetProcAddress(lib, "RegisterTouchWindow");
  winUnregisterTouchWindow = (BOOL (WINAPI *)(HWND))GetProcAddress(lib, "UnregisterTouchWindow");
  winIsTouchWindow = (BOOL (WINAPI *)(HWND,PULONG))GetProcAddress(lib, "IsTouchWindow");

  if (winIsTouchWindow)
    win_touch_loaded = 1;
  else
    win_touch_loaded = 0;
}

static int winSetTouchAttrib(Ihandle *ih, const char *value)
{
  if (win_touch_loaded)
  {
    if (iupStrBoolean(value))
      winRegisterTouchWindow(ih->handle, 0);
    else
      winUnregisterTouchWindow(ih->handle);
  }
  return 0;
}

static char* winGetTouchAttrib(Ihandle* ih)
{
  ULONG pulFlags = 0;
  return iupStrReturnBoolean (win_touch_loaded && winIsTouchWindow(ih->handle, &pulFlags));
}

static int winSetGestureAttrib(Ihandle *ih, const char *value)
{
  if (win_touch_loaded)
  {
    if (!iupStrBoolean(value))
    {
      GESTURECONFIG config;
      config.dwID = 0;
      config.dwWant = 0;
      config.dwBlock = GC_ALLGESTURES;
      SetGestureConfig(ih->handle, 0, 1, &config, sizeof(config));
    }
  }
  return 0;
}

/* WM_GESTURE and WM_TOUCH are mutually exclusive; GESTURE_CB needs TOUCH off. Gestures are on by default. */
IUP_DRV_API int iupwinGestureProcessInfo(Ihandle* ih, void* lp)
{
  GESTUREINFO gi;
  HGESTUREINFO hgi = (HGESTUREINFO)lp;
  IFniiiidd cb = (IFniiiidd)IupGetCallback(ih, "GESTURE_CB");
  int x, y, state;

  if (!cb)
    return 0;

  ZeroMemory(&gi, sizeof(GESTUREINFO));
  gi.cbSize = sizeof(GESTUREINFO);
  if (!GetGestureInfo(hgi, &gi))
    return 0;

  x = gi.ptsLocation.x;
  y = gi.ptsLocation.y;
  iupdrvScreenToClient(ih, &x, &y);

  if (gi.dwFlags & GF_BEGIN)    state = IUP_GESTURE_BEGIN;
  else if (gi.dwFlags & GF_END) state = IUP_GESTURE_END;
  else                          state = IUP_GESTURE_CHANGED;

  switch (gi.dwID)
  {
    case GID_ZOOM:
    {
      double dist = (double)(DWORD)gi.ullArguments;
      double scale;
      if (state == IUP_GESTURE_BEGIN) { iupAttribSetDouble(ih, "_IUPWIN_GESTURE_DIST0", dist); scale = 1.0; }
      else { double d0 = iupAttribGetDouble(ih, "_IUPWIN_GESTURE_DIST0"); scale = (d0 > 0.0)? dist / d0 : 1.0; }
      if (cb(ih, IUP_GESTURE_PINCH, state, x, y, scale, 0) == IUP_CLOSE) IupExitLoop();
      break;
    }
    case GID_ROTATE:
    {
      double ang = GID_ROTATE_ANGLE_FROM_ARGUMENT(gi.ullArguments) * 180.0 / 3.14159265358979;
      double deg;
      if (state == IUP_GESTURE_BEGIN) { iupAttribSetDouble(ih, "_IUPWIN_GESTURE_ANGLE0", ang); deg = 0; }
      else deg = ang - iupAttribGetDouble(ih, "_IUPWIN_GESTURE_ANGLE0");
      if (cb(ih, IUP_GESTURE_ROTATE, state, x, y, deg, 0) == IUP_CLOSE) IupExitLoop();
      break;
    }
    case GID_PAN:
    {
      double dx, dy;
      if (state == IUP_GESTURE_BEGIN) { iupAttribSetInt(ih, "_IUPWIN_GESTURE_PANX0", x); iupAttribSetInt(ih, "_IUPWIN_GESTURE_PANY0", y); dx = dy = 0; }
      else { dx = x - iupAttribGetInt(ih, "_IUPWIN_GESTURE_PANX0"); dy = y - iupAttribGetInt(ih, "_IUPWIN_GESTURE_PANY0"); }
      if (cb(ih, IUP_GESTURE_PAN, state, x, y, dx, dy) == IUP_CLOSE) IupExitLoop();
      break;
    }
    default:
      return 0;  /* GID_BEGIN/END/TWOFINGERTAP/PRESSANDTAP: let DefWindowProc close the handle */
  }

  CloseGestureInfoHandle(hgi);
  return 1;
}

IUP_DRV_API void iupwinTouchRegisterAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "TOUCH_CB", "iiis");
  iupClassRegisterCallback(ic, "MULTITOUCH_CB", "iIII");
  iupClassRegisterCallback(ic, "GESTURE_CB", "iiiidd");

  iupClassRegisterAttribute(ic, "TOUCH", winGetTouchAttrib, winSetTouchAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GESTURE", NULL, winSetGestureAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}

IUP_DRV_API void iupwinTouchProcessInput(Ihandle* ih, int count, void* lp)
{
  HTOUCHINPUT hTouchInput = (HTOUCHINPUT)lp;
  IFniIIII mcb = (IFniIIII)IupGetCallback(ih, "MULTITOUCH_CB");
  IFniiis cb = (IFniiis)IupGetCallback(ih, "TOUCH_CB");

  if (mcb || cb)
  {
    int *px=NULL, *py=NULL, *pid=NULL, *pstate=NULL;
    TOUCHINPUT* ti = malloc(count*sizeof(TOUCHINPUT));

    if (mcb)
    {
      px = malloc(sizeof(int)*(count));
      py = malloc(sizeof(int)*(count));
      pid = malloc(sizeof(int)*(count));
      pstate = malloc(sizeof(int)*(count));
    }

    if (winGetTouchInputInfo(hTouchInput, count, ti, sizeof(TOUCHINPUT)))
    {
      int i, x, y;
      for (i = 0; i < count; i++)
      {
        x = ti[i].x / 100;
        y = ti[i].y / 100;
        iupdrvScreenToClient(ih, &x, &y);

        if (ti[i].dwFlags & TOUCHEVENTF_DOWN ||
            ti[i].dwFlags & TOUCHEVENTF_MOVE ||
            ti[i].dwFlags & TOUCHEVENTF_UP)
        {
          char* state = (ti[i].dwFlags & TOUCHEVENTF_DOWN)? "DOWN": ((ti[i].dwFlags & TOUCHEVENTF_UP)? "UP": "MOVE");
          if (cb)
          {
            if (ti[i].dwFlags & TOUCHEVENTF_PRIMARY)
              state = (ti[i].dwFlags & TOUCHEVENTF_DOWN)? "DOWN-PRIMARY": ((ti[i].dwFlags & TOUCHEVENTF_UP)? "UP-PRIMARY": "MOVE-PRIMARY");

            if (cb(ih, ti[i].dwID, x, y, state)==IUP_CLOSE)
            {
              IupExitLoop();

              if (mcb)
              {
                free(px);
                free(py);
                free(pid);
                free(pstate);
              }

              winCloseTouchInputHandle(hTouchInput);
              free(ti);

              return;
            }
          }

          if (mcb)
          {
            px[i] = x;
            py[i] = y;
            pid[i] = ti[i].dwID;
            pstate[i] = state[0];
          }
        }
      }
    }

    if (mcb)
    {
      if (mcb(ih, count, pid, px, py, pstate)==IUP_CLOSE)
        IupExitLoop();

      free(px);
      free(py);
      free(pid);
      free(pstate);
    }

    winCloseTouchInputHandle(hTouchInput);
    free(ti);
  }
}

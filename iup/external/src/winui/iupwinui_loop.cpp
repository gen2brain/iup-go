/** \file
 * \brief WinUI Driver - Event Loop using Win32 message pump
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstdio>

#include "pch.h"

using namespace winrt;

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_drv.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_loop.h"
}

#include "iupwinui_drv.h"

static int winui_main_loop_level = 0;
static int winui_exit_loop = 0;
static IFidle winui_idle_cb = NULL;

extern "C" void iupwinuiLoopCleanup(void)
{
  winui_idle_cb = NULL;
  winui_exit_loop = 0;
  winui_main_loop_level = 0;
}

extern "C" void IupExitLoop(void)
{
  winui_exit_loop = 1;

  if (winui_main_loop_level <= 1)
    PostQuitMessage(0);
}

extern "C" int IupMainLoopLevel(void)
{
  return winui_main_loop_level;
}

static int winuiLoopProcessMessage(MSG* msg)
{
  if (msg->message == WM_QUIT)
    return IUP_CLOSE;

  if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)
  {
    int wincode = (int)msg->wParam;
    if (wincode != VK_SHIFT && wincode != VK_CONTROL && wincode != VK_MENU &&
        wincode != VK_LWIN && wincode != VK_RWIN)
    {
      int has_modifier = (GetKeyState(VK_CONTROL) & 0x8000) ||
                         (GetKeyState(VK_MENU) & 0x8000) ||
                         (GetKeyState(VK_LWIN) & 0x8000) ||
                         (GetKeyState(VK_RWIN) & 0x8000);
      if (has_modifier)
      {
        Ihandle* ih = IupGetFocus();
        if (!ih)
        {
          HWND active = GetActiveWindow();
          if (active)
          {
            Ihandle* dlg = (Ihandle*)GetWindowLongPtr(active, GWLP_USERDATA);
            if (dlg && iupObjectCheck(dlg))
              ih = dlg;
          }
        }
        if (ih)
        {
          if (!iupwinuiKeyEvent(ih, wincode, 1))
          {
            MSG flush;
            while (PeekMessage(&flush, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
            return IUP_DEFAULT;
          }

          int still_modified = (GetKeyState(VK_CONTROL) & 0x8000) ||
                               (GetKeyState(VK_MENU) & 0x8000) ||
                               (GetKeyState(VK_LWIN) & 0x8000) ||
                               (GetKeyState(VK_RWIN) & 0x8000);
          if (!still_modified)
          {
            MSG flush;
            while (PeekMessage(&flush, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
            return IUP_DEFAULT;
          }
        }
      }
    }
  }

  BOOL handled = iupwinuiContentPreTranslateMessage(msg);
  if (!handled)
  {
    TranslateMessage(msg);
    DispatchMessage(msg);
  }
  return IUP_DEFAULT;
}

extern "C" int IupMainLoop(void)
{
  static int has_done_entry = 0;

  winui_main_loop_level++;
  winui_exit_loop = 0;

  if (has_done_entry == 0)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  MSG msg;
  int ret;
  int return_code = IUP_NOERROR;

  do
  {
    if (winui_idle_cb)
    {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        if (winuiLoopProcessMessage(&msg) == IUP_CLOSE)
        {
          return_code = IUP_CLOSE;
          break;
        }
      }
      else
      {
        int idle_ret = winui_idle_cb();
        if (idle_ret == IUP_CLOSE)
        {
          return_code = IUP_CLOSE;
          break;
        }
        if (idle_ret == IUP_IGNORE)
          winui_idle_cb = NULL;
      }
      ret = 1;
    }
    else
    {
      ret = GetMessage(&msg, NULL, 0, 0);
      if (ret == -1)
        return_code = IUP_ERROR;
      if (ret == 0 || winuiLoopProcessMessage(&msg) == IUP_CLOSE)
      {
        return_code = IUP_NOERROR;
        break;
      }
    }
  } while (ret && !winui_exit_loop);

  winui_exit_loop = 0;
  winui_main_loop_level--;

  if (winui_main_loop_level == 0)
    iupLoopCallExitCb();

  return return_code;
}

extern "C" int IupLoopStepWait(void)
{
  MSG msg;
  int ret = GetMessage(&msg, NULL, 0, 0);
  if (ret == -1)
    return IUP_ERROR;
  if (ret == 0 || winuiLoopProcessMessage(&msg) == IUP_CLOSE)
    return IUP_CLOSE;
  return IUP_DEFAULT;
}

extern "C" int IupLoopStep(void)
{
  MSG msg;
  if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    return winuiLoopProcessMessage(&msg);
  else if (winui_idle_cb)
  {
    int ret = winui_idle_cb();
    if (ret == IUP_CLOSE)
      return IUP_CLOSE;
    if (ret == IUP_IGNORE)
      winui_idle_cb = NULL;
  }
  return IUP_DEFAULT;
}

extern "C" void IupFlush(void)
{
  int post_quit = 0;
  MSG msg;

  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    if (winuiLoopProcessMessage(&msg) == IUP_CLOSE)
    {
      post_quit = 1;
      break;
    }
  }

  if (post_quit && winui_main_loop_level > 0)
  {
    IupExitLoop();
  }
}

typedef struct {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  void* p;
} winuiPostMessageData;

extern "C" void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  winuiPostMessageData* data = (winuiPostMessageData*)malloc(sizeof(winuiPostMessageData));
  if (!data)
    return;

  data->ih = ih;
  data->s = iupStrDup(s);
  data->i = i;
  data->d = d;
  data->p = p;

  void* dq_ptr = iupwinuiGetDispatcherQueue();
  if (!dq_ptr)
  {
    if (data->s) free(data->s);
    free(data);
    return;
  }

  Windows::Foundation::IInspectable dq_obj{nullptr};
  winrt::copy_from_abi(dq_obj, dq_ptr);
  Microsoft::UI::Dispatching::DispatcherQueue dq = dq_obj.as<Microsoft::UI::Dispatching::DispatcherQueue>();

  dq.TryEnqueue([data]()
  {
    if (iupObjectCheck(data->ih))
    {
      IFnsidv cb = (IFnsidv)IupGetCallback(data->ih, "POSTMESSAGE_CB");
      if (cb)
        cb(data->ih, data->s, data->i, data->d, data->p);
    }
    if (data->s) free(data->s);
    free(data);
  });
}

extern "C" void iupdrvSetIdleFunction(Icallback func)
{
  winui_idle_cb = (IFidle)func;
}

extern "C" void iupdrvSleep(int time)
{
  Sleep(time);
}

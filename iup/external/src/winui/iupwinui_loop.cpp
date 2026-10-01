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
#include "iup_dlglist.h"
}

#include "iupwinui_drv.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

static int winui_main_loop_level = 0;
static int winui_exit_loop = 0;
static int winui_quit_pending = 0;
static IFidle winui_idle_cb = nullptr;

static void winuiFlushXamlLayout()
{
  Ihandle* ih;
  for (ih = iupDlgListFirst(); ih; ih = iupDlgListNext())
  {
    if (!ih->handle || !iupObjectCheck(ih))
      continue;
    if (!IupClassMatch(ih, "dialog"))
      continue;

    auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
    if (aux && aux->rootPanel)
      aux->rootPanel.UpdateLayout();
  }
}

IUP_DRV_API void iupwinuiLoopCleanup(void)
{
  winui_idle_cb = nullptr;
  winui_exit_loop = 0;
  winui_quit_pending = 0;
  winui_main_loop_level = 0;
}

extern "C" void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");
  if (winui_main_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    winui_exit_loop = 1;

    if (winui_main_loop_level <= 1 && !winui_quit_pending)
    {
      winui_quit_pending = 1;
      PostQuitMessage(0);
    }
  }
}

static int winuiLoopStepResult(int ret)
{
  if (winui_main_loop_level > 0)
  {
    if (ret == IUP_CLOSE)
      IupExitLoop();
    if (winui_exit_loop)
      return IUP_CLOSE;
  }
  return ret;
}

extern "C" int IupMainLoopLevel(void)
{
  return winui_main_loop_level;
}

static int winuiLoopProcessMessage(MSG* msg)
{
  int dispatched = 0;

  if (msg->message == WM_QUIT)
  {
    winui_quit_pending = 0;
    return IUP_CLOSE;
  }

  if (msg->wParam == VK_MENU)
  {
    if (msg->message == WM_SYSKEYDOWN || msg->message == WM_KEYDOWN)
      iupwinuiSetAccelCueAlt(1);
    else if (msg->message == WM_SYSKEYUP || msg->message == WM_KEYUP)
      iupwinuiSetAccelCueAlt(0);
  }

  if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)
  {
    int wincode = static_cast<int>(msg->wParam);
    if (wincode != VK_SHIFT && wincode != VK_CONTROL && wincode != VK_MENU &&
        wincode != VK_LWIN && wincode != VK_RWIN)
    {
      Ihandle* focus = IupGetFocus();
      Ihandle* dlg = focus ? IupGetDialog(focus) : nullptr;
      if (!dlg)
      {
        dlg = iupwinuiDialogFromHwnd(GetActiveWindow());
      }
      if (dlg)
      {
        int code = iupwinuiKeyDecode(wincode, (msg->lParam & 0x01000000)? 1: 0);
        if (code && iupwinuiMenuActivateAccel(dlg, code))
        {
          MSG flush;
          while (PeekMessage(&flush, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
          return IUP_DEFAULT;
        }
      }

      int has_modifier = (GetKeyState(VK_CONTROL) & 0x8000) ||
                         (GetKeyState(VK_MENU) & 0x8000) ||
                         (GetKeyState(VK_LWIN) & 0x8000) ||
                         (GetKeyState(VK_RWIN) & 0x8000);
      if (has_modifier)
      {
        Ihandle* ih = IupGetFocus();
        if (!ih)
          ih = iupwinuiDialogFromHwnd(GetActiveWindow());
        int alt_numpad_compose = ih &&
            (GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_CONTROL) & 0x8000) &&
            wincode >= VK_NUMPAD0 && wincode <= VK_NUMPAD9 &&
            IupGetCallback(ih, "TEXTINPUT_CB") != nullptr;
        if (ih && !alt_numpad_compose)
        {
          if (!iupwinuiKeyEvent(ih, wincode, (msg->lParam & 0x01000000)? 1: 0, 1))
          {
            MSG flush;
            while (PeekMessage(&flush, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
            return IUP_DEFAULT;
          }

          int still_modified = (GetKeyState(VK_CONTROL) & 0x8000) ||
                               (GetKeyState(VK_MENU) & 0x8000) ||
                               (GetKeyState(VK_LWIN) & 0x8000) ||
                               (GetKeyState(VK_RWIN) & 0x8000);
          if (!still_modified)
          {
            MSG flush;
            while (PeekMessage(&flush, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
            return IUP_DEFAULT;
          }

          dispatched = wincode;
        }
      }
    }
  }

  iupwinuiKeySetDispatched(dispatched);
  BOOL handled = iupwinuiContentPreTranslateMessage(msg);
  if (!handled)
  {
    TranslateMessage(msg);
    DispatchMessage(msg);
  }
  iupwinuiKeySetDispatched(0);
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
      if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
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
          winui_idle_cb = nullptr;
          return_code = IUP_CLOSE;
          break;
        }
        if (idle_ret == IUP_IGNORE)
          winui_idle_cb = nullptr;
      }
      ret = 1;
    }
    else
    {
      ret = GetMessage(&msg, nullptr, 0, 0);
      if (ret == -1)
        return_code = IUP_ERROR;
      if (ret == 0 || winuiLoopProcessMessage(&msg) == IUP_CLOSE)
      {
        if (ret == 0)
          winui_quit_pending = 0;
        return_code = IUP_NOERROR;
        break;
      }
    }
  } while (ret && !(winui_exit_loop && winui_main_loop_level > 1));

  winui_exit_loop = 0;
  winui_main_loop_level--;

  if (winui_main_loop_level == 0)
    iupLoopCallExitCb();

  return return_code;
}

extern "C" int IupLoopStepWait(void)
{
  MSG msg;
  int ret = GetMessage(&msg, nullptr, 0, 0);
  if (ret == -1)
    return IUP_ERROR;
  if (ret == 0 || winuiLoopProcessMessage(&msg) == IUP_CLOSE)
  {
    if (ret == 0)
      winui_quit_pending = 0;
    return winuiLoopStepResult(IUP_CLOSE);
  }
  return winuiLoopStepResult(IUP_DEFAULT);
}

extern "C" int IupLoopStep(void)
{
  MSG msg;
  if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    return winuiLoopStepResult(winuiLoopProcessMessage(&msg));
  else if (winui_idle_cb)
  {
    int ret = winui_idle_cb();
    if (ret == IUP_CLOSE)
    {
      winui_idle_cb = nullptr;
      return winuiLoopStepResult(IUP_CLOSE);
    }
    if (ret == IUP_IGNORE)
      winui_idle_cb = nullptr;
  }
  return winuiLoopStepResult(IUP_DEFAULT);
}

extern "C" void IupFlush(void)
{
  int post_quit = 0;
  int count = 0;
  MSG msg;

  while (count < 100 && PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    if (winuiLoopProcessMessage(&msg) == IUP_CLOSE)
    {
      post_quit = 1;
      break;
    }
    count++;
  }

  winuiFlushXamlLayout();

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
  auto* data = static_cast<winuiPostMessageData*>(malloc(sizeof(winuiPostMessageData)));
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
      auto cb = reinterpret_cast<IFnsidv>(IupGetCallback(data->ih, "POSTMESSAGE_CB"));
      if (cb)
      {
        if (cb(data->ih, data->s, data->i, data->d, data->p) == IUP_CLOSE)
          IupExitLoop();
      }
    }
    if (data->s) free(data->s);
    free(data);
  });
}

extern "C" IUP_SDK_API void iupdrvSetEntryFunction(Icallback func)
{
  (void)func;
}

extern "C" IUP_SDK_API void* iupdrvNativeScopeBegin(void)
{
  return nullptr;
}

extern "C" IUP_SDK_API void iupdrvNativeScopeEnd(void* scope)
{
  (void)scope;
}

extern "C" IUP_SDK_API void iupdrvSetIdleFunction(Icallback func)
{
  winui_idle_cb = reinterpret_cast<IFidle>(func);
}

extern "C" IUP_SDK_API void iupdrvSleep(int time)
{
  Sleep(time);
}

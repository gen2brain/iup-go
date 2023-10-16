/** \file
 * \brief Windows Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

#include <windows.h>

#include "iup.h"
#include "iupcbs.h"  

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_loop.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"

/* This just needs to be a random unique number not used by the OS */
#define IWIN_POSTMESSAGE_ID 0x4456

static IFidle win_idle_cb = NULL;
static int win_main_loop_level = 0;
static UINT win_quit_message = WM_QUIT;

void iupwinSetCustomQuitMessage(int enable)
{
  if (enable)
    win_quit_message = RegisterWindowMessage(TEXT("IUP_QUIT_MESSAGE"));
  else
    win_quit_message = WM_QUIT;
}

IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  win_idle_cb = (IFidle)f;
}

static int winLoopCallIdle(void)
{
  int ret = win_idle_cb();
  if (ret == IUP_CLOSE)
  {
    win_idle_cb = NULL;
    return IUP_CLOSE;
  }
  if (ret == IUP_IGNORE) 
    win_idle_cb = NULL;
  return ret;
}

IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");
  if (win_main_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    if (win_quit_message == WM_QUIT)
      PostQuitMessage(0);
    else
      PostMessage(NULL, win_quit_message, 0, 0L);
  }
}

static void winProcessPostMessage(LPARAM lParam);

IUP_DRV_API int iupwinPostMessageFilter(MSG* msg)
{
#ifndef USE_WINHOOKPOST
  if (msg->message == WM_APP && msg->wParam == IWIN_POSTMESSAGE_ID)
  {
    winProcessPostMessage(msg->lParam);
    return 1;
  }
#else
  if (CallMsgFilter(msg, IWIN_POSTMESSAGE_ID))
    return 1;
#endif
  return 0;
}

static int winLoopProcessMessage(MSG* msg)
{
  if (msg->message == win_quit_message)  /* IUP_CLOSE returned in a callback or IupHide in a popup dialog or all dialogs closed */
    return IUP_CLOSE;
  else
  {
    if (!iupwinPostMessageFilter(msg))
    {
      TranslateMessage(msg);
      DispatchMessage(msg);
    }
    return IUP_DEFAULT;
  }
}

IUP_API int IupMainLoopLevel(void)
{
  return win_main_loop_level;
}

IUP_API int IupMainLoop(void)
{
  MSG msg;
  int ret;
  int return_code = IUP_NOERROR;
  static int has_done_entry = 0;

  win_main_loop_level++;

  if (0 == has_done_entry)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  do
  {
    if (win_idle_cb)
    {
      ret = 1;
      if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      {
        if (winLoopProcessMessage(&msg) == IUP_CLOSE)
        {
          return_code = IUP_CLOSE;
          break;
        }
      }
      else
      {
        if (winLoopCallIdle() == IUP_CLOSE)
        {
          return_code = IUP_CLOSE;
          break;
        }
      }
    }
    else
    {
      ret = GetMessage(&msg, NULL, 0, 0);
      if (ret == -1) /* error */
      {
        return_code = IUP_ERROR;
        /* break; */ /* Very unlikely to occur and rarely, depending on the system, is aborting the application */
      }
      if (ret == 0 || /* WM_QUIT */
          winLoopProcessMessage(&msg) == IUP_CLOSE)  /* ret != 0 */
      {
        return_code = IUP_NOERROR;
        break;
      }
    }
  } while (ret);

  win_main_loop_level--;

  if (win_main_loop_level == 0)
    iupLoopCallExitCb();

  return return_code;
}

IUP_API int IupLoopStepWait(void)
{
  MSG msg;
  int ret = GetMessage(&msg, NULL, 0, 0);
  if (ret == -1) /* error */
    return IUP_ERROR;

  if (ret == 0 || /* WM_QUIT */
      winLoopProcessMessage(&msg) == IUP_CLOSE)  /* ret != 0 */
    return IUP_CLOSE;

  return IUP_DEFAULT;
}

IUP_API int IupLoopStep(void)
{
  MSG msg;
  if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    return winLoopProcessMessage(&msg);
  else if (win_idle_cb)
    return winLoopCallIdle();

  return IUP_DEFAULT;
}

IUP_API void IupFlush(void)
{
  int post_quit = 0;
  MSG msg;

  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    if (winLoopProcessMessage(&msg) == IUP_CLOSE)
    {
      post_quit = 1;
      break;
    }
  }

  /* re post the quit message if still inside MainLoop */
  if (post_quit && win_main_loop_level>0)
    IupExitLoop();
}


typedef struct {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  char* p;
} winPostMessageUserData;

IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  winPostMessageUserData* user_data = (winPostMessageUserData*)malloc(sizeof(winPostMessageUserData));
  user_data->ih = ih;
  user_data->s = iupStrDup(s);
  user_data->i = i;
  user_data->d = d;
  user_data->p = p;
  PostThreadMessage(iupwin_mainthreadid, WM_APP, (WPARAM)IWIN_POSTMESSAGE_ID, (LPARAM)user_data);
}

static void winProcessPostMessage(LPARAM lParam)
{
  winPostMessageUserData* user_data = (winPostMessageUserData*)lParam;
  Ihandle* ih = user_data->ih;
  IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if (cb)
    cb(ih, user_data->s, user_data->i, user_data->d, user_data->p);
  if (user_data->s) free(user_data->s);
  free(user_data);
}

#ifdef USE_WINHOOKPOST
/* Based on Raymond Chen's discussion of PostThreadMessage
https://blogs.msdn.microsoft.com/oldnewthing/20050428-00/?p=35753
*/
LRESULT CALLBACK iupwinPostMessageFilterProc(int code, WPARAM wParam, LPARAM lParam)
{
  MSG* msg = (MSG*)lParam;
  /* Interesting: Chen uses code >= 0 for a purpose.
	Usually, we get back IWIN_POSTMESSAGE_ID, but in the case where we could lose messages (right-click on the title bar)
	I get a different number. (2 for the right-click title bar)
	If we check explicitly only for code == IWIN_POSTMESSAGE_ID, then we skip this block in this case and lose messages.
	Unfortunately, we lose yet another identifier to recognize this is our message.
	However, Chen looks for WM_APP in psg->message which implies that is unique enough.
	If somehow we start getting other WM_APP messages that aren't ours, we may need
	to change things to set pmsg->wParam to another unique identifier so we can recognize this is our data.
	But for now, this seems to work and hopefully there can't be any other messages since we are the only
	ones that should be posting messages to our thread in an IUP app.
  */
  if (code >= 0)
  {
    if (msg->message == WM_APP && msg->wParam == IWIN_POSTMESSAGE_ID)
    {
      winProcessPostMessage(msg->lParam);
      return TRUE;
    }
  }

  return CallNextHookEx(iupwin_threadmsghook, code, wParam, lParam);
}
#endif

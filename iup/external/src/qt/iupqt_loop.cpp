/** \file
 * \brief Qt Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>

#include <QApplication>
#include <QTimer>
#include <QEventLoop>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_str.h"
#include "iup_object.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Idle Callback Management
 ****************************************************************************/

static IFidle qt_idle_cb = NULL;
static QTimer* qt_idle_timer = NULL;

static void qtIdleFunc(void)
{
  if (qt_idle_cb)
  {
    int ret = qt_idle_cb();

    if (ret == IUP_CLOSE)
    {
      qt_idle_cb = NULL;
      IupExitLoop();
      if (qt_idle_timer)
        qt_idle_timer->stop();
      return;
    }

    if (ret == IUP_IGNORE)
    {
      qt_idle_cb = NULL;
      if (qt_idle_timer)
        qt_idle_timer->stop();
      return;
    }
  }
  else
  {
    if (qt_idle_timer)
      qt_idle_timer->stop();
  }
}

extern "C" IUP_SDK_API void iupdrvSetEntryFunction(Icallback func)
{
  (void)func;
}

extern "C" IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  if (qt_idle_timer)
  {
    qt_idle_timer->stop();
    delete qt_idle_timer;
    qt_idle_timer = NULL;
  }

  qt_idle_cb = (IFidle)f;

  if (qt_idle_cb)
  {
    qt_idle_timer = new QTimer();
    qt_idle_timer->setInterval(0);
    qt_idle_timer->setSingleShot(false);

    QObject::connect(qt_idle_timer, &QTimer::timeout, qtIdleFunc);

    qt_idle_timer->start();
  }
}

/****************************************************************************
 * Main Loop Management
 ****************************************************************************/

static int qt_main_loop_level = 0;
static bool qt_loop_exit_flag[10] = {false}; /* Support up to 10 nested levels */


extern "C" IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");

  if (qt_main_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    if (qt_main_loop_level > 0 && qt_main_loop_level <= 10)
      qt_loop_exit_flag[qt_main_loop_level - 1] = true;
  }
}

extern "C" IUP_API int IupMainLoopLevel(void)
{
  return qt_main_loop_level;
}

extern "C" IUP_API int IupMainLoop(void)
{
  static int has_done_entry = 0;

  if (has_done_entry == 0)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  qt_main_loop_level++;
  int current_level = qt_main_loop_level - 1; /* 0-based index for array */

  if (current_level >= 10)
  {
    qt_main_loop_level--;
    return IUP_ERROR;
  }

  qt_loop_exit_flag[current_level] = false;

  QApplication* app = iupqtGetApplication();
  if (app)
  {
    while (!qt_loop_exit_flag[current_level] && !app->closingDown())
    {
      QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents | QEventLoop::AllEvents);

      if (qt_idle_cb)
      {
        int ret = qt_idle_cb();
        if (ret == IUP_CLOSE)
        {
          qt_idle_cb = NULL;
          IupExitLoop();
          break;
        }
        if (ret == IUP_IGNORE)
          qt_idle_cb = NULL;
      }
    }
  }

  qt_loop_exit_flag[current_level] = false;
  qt_main_loop_level--;

  if (qt_main_loop_level == 0)
    iupLoopCallExitCb();

  return IUP_NOERROR;
}

extern "C" IUP_API int IupLoopStepWait(void)
{
  QApplication* app = iupqtGetApplication();
  if (!app)
    return IUP_DEFAULT;

  QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

  if (app->closingDown())
    return IUP_CLOSE;

  return IUP_DEFAULT;
}

extern "C" IUP_API int IupLoopStep(void)
{
  QApplication* app = iupqtGetApplication();
  if (!app)
    return IUP_DEFAULT;

  QCoreApplication::processEvents(QEventLoop::AllEvents);

  if (qt_idle_cb)
  {
    int ret = qt_idle_cb();
    if (ret == IUP_CLOSE)
    {
      qt_idle_cb = NULL;
      return IUP_CLOSE;
    }
    if (ret == IUP_IGNORE)
      qt_idle_cb = NULL;
  }

  if (app->closingDown())
    return IUP_CLOSE;

  return IUP_DEFAULT;
}

extern "C" IUP_API void IupFlush(void)
{
  int count = 0;

  IFidle old_qt_idle_cb = NULL;
  if (qt_idle_cb)
  {
    old_qt_idle_cb = qt_idle_cb;
    iupdrvSetIdleFunction(NULL);
  }

  for (int i = 0; i < 10 && count < 100; i++, count++)
  {
    QCoreApplication::processEvents(QEventLoop::AllEvents);
  }

  if (old_qt_idle_cb)
    iupdrvSetIdleFunction((Icallback)old_qt_idle_cb);
}

/****************************************************************************
 * PostMessage Support
 ****************************************************************************/

typedef struct {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  void* p;
} qtPostMessageUserData;

static void qtPostMessageExecute(qtPostMessageUserData* user_data)
{
  if (!user_data)
  {
    return;
  }

  Ihandle* ih = user_data->ih;
  if (iupObjectCheck(ih))
  {
    IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
    if (cb)
    {
      if (cb(ih, user_data->s, user_data->i, user_data->d, user_data->p) == IUP_CLOSE)
        IupExitLoop();
    }
  }

  if (user_data->s)
    free(user_data->s);
  free(user_data);
}

extern "C" IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  qtPostMessageUserData* user_data = (qtPostMessageUserData*)malloc(sizeof(qtPostMessageUserData));
  user_data->ih = ih;
  user_data->s = iupStrDup(s);
  user_data->i = i;
  user_data->d = d;
  user_data->p = p;

  /* the QApplication context makes the post safe from another thread */
  QTimer::singleShot(0, QApplication::instance(), [user_data]() {
    qtPostMessageExecute(user_data);
  });
}

/****************************************************************************
 * Loop Cleanup
 ****************************************************************************/

IUP_DRV_API void iupqtLoopCleanup(void)
{
  if (qt_idle_timer)
  {
    qt_idle_timer->stop();
    delete qt_idle_timer;
    qt_idle_timer = NULL;
  }

  qt_idle_cb = NULL;
  qt_main_loop_level = 0;
}

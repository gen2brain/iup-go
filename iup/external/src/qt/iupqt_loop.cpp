/** \file
 * \brief Qt Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <QApplication>
#include <QTimer>
#include <QEventLoop>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_str.h"
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
    /* No callback, stop the timer */
    if (qt_idle_timer)
      qt_idle_timer->stop();
  }
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
    qt_idle_timer->setInterval(0);  /* Fire as soon as event loop is idle */
    qt_idle_timer->setSingleShot(false);  /* Repeat until stopped */

    /* Use lambda to avoid MOC - lambda captures are supported in Qt 5+ */
    QObject::connect(qt_idle_timer, &QTimer::timeout, qtIdleFunc);

    qt_idle_timer->start();
  }
}

/****************************************************************************
 * Main Loop Management
 ****************************************************************************/

/* Track main loop nesting level and exit flags */
static int qt_main_loop_level = 0;
static bool qt_loop_exit_flag[10] = {false}; /* Support up to 10 nested levels */


extern "C" IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");

  /* Exit if nested or EXITLOOP is not explicitly disabled */
  if (qt_main_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    /* Set exit flag for current loop level */
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

  /* Call entry callbacks once */
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

  /* Manually pump events */
  QApplication* app = iupqtGetApplication();
  if (app)
  {
    /* Process events in a loop until exit flag is set */
    while (!qt_loop_exit_flag[current_level] && !app->closingDown())
    {
      /* Wait for and process events */
      QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents | QEventLoop::AllEvents);

      /* Also call idle callback if set */
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

  /* Call exit callbacks when returning from top-level loop */
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

  /* Also call idle callback if looping manually */
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

  /* Temporarily disable idle callback to avoid interference */
  IFidle old_qt_idle_cb = NULL;
  if (qt_idle_cb)
  {
    old_qt_idle_cb = qt_idle_cb;
    iupdrvSetIdleFunction(NULL);
  }

  /* Process pending events (process several rounds to ensure all queued events are handled) */
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

/* User data structure for PostMessage */
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
  IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if (cb)
  {
    cb(ih, user_data->s, user_data->i, user_data->d, user_data->p);
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

  /* Use QTimer::singleShot with QApplication context to ensure cross-thread safety
   * QTimer::singleShot(0, ...) queues the lambda to run in the next event loop iteration */
  QTimer::singleShot(0, QApplication::instance(), [user_data]() {
    qtPostMessageExecute(user_data);
  });
}

/****************************************************************************
 * Loop Cleanup
 ****************************************************************************/

extern "C" void iupqtLoopCleanup(void)
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

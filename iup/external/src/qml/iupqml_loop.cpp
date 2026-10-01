/** \file
 * \brief Qt Quick Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>

#include <QGuiApplication>
#include <QTimer>
#include <QEventLoop>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_str.h"
#include "iup_object.h"
}

#include "iupqml_drv.h"


/****************************************************************************
 * Idle Callback Management
 ****************************************************************************/

static IFidle qml_idle_cb = nullptr;
static QTimer* qml_idle_timer = nullptr;

static void qmlIdleFunc()
{
  if (qml_idle_cb)
  {
    int ret = qml_idle_cb();

    if (ret == IUP_CLOSE)
    {
      qml_idle_cb = nullptr;
      IupExitLoop();
      if (qml_idle_timer)
        qml_idle_timer->stop();
      return;
    }

    if (ret == IUP_IGNORE)
    {
      qml_idle_cb = nullptr;
      if (qml_idle_timer)
        qml_idle_timer->stop();
      return;
    }
  }
  else
  {
    if (qml_idle_timer)
      qml_idle_timer->stop();
  }
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

extern "C" IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  if (qml_idle_timer)
  {
    qml_idle_timer->stop();
    delete qml_idle_timer;
    qml_idle_timer = nullptr;
  }

  qml_idle_cb = reinterpret_cast<IFidle>(f);

  if (qml_idle_cb)
  {
    qml_idle_timer = new QTimer();
    qml_idle_timer->setInterval(0);
    qml_idle_timer->setSingleShot(false);

    QObject::connect(qml_idle_timer, &QTimer::timeout, qmlIdleFunc);

    qml_idle_timer->start();
  }
}

/****************************************************************************
 * Main Loop Management
 ****************************************************************************/

static int qml_main_loop_level = 0;
static bool qml_loop_exit_flag[10] = {false}; /* Support up to 10 nested levels */
static bool qml_loop_exit_requested = false;

static bool qmlLoopExitPending()
{
  if (qml_main_loop_level > 0 && qml_main_loop_level <= 10)
    return qml_loop_exit_flag[qml_main_loop_level - 1];

  if (!qml_loop_exit_requested)
    return false;
  qml_loop_exit_requested = false;
  return true;
}


extern "C" IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");

  if (qml_main_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    if (qml_main_loop_level > 0 && qml_main_loop_level <= 10)
      qml_loop_exit_flag[qml_main_loop_level - 1] = true;
    else if (qml_main_loop_level == 0)
      qml_loop_exit_requested = true;
  }
}

extern "C" IUP_API int IupMainLoopLevel(void)
{
  return qml_main_loop_level;
}

extern "C" IUP_API int IupMainLoop(void)
{
  static int has_done_entry = 0;

  if (has_done_entry == 0)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  qml_main_loop_level++;
  int current_level = qml_main_loop_level - 1; /* 0-based index for array */

  if (current_level >= 10)
  {
    qml_main_loop_level--;
    return IUP_ERROR;
  }

  qml_loop_exit_flag[current_level] = false;
  qml_loop_exit_requested = false;

  QGuiApplication* app = iupqmlGetApplication();
  if (app)
  {
    while (!qml_loop_exit_flag[current_level] && !QCoreApplication::closingDown())
    {
      QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents | QEventLoop::AllEvents);
    }
  }

  qml_loop_exit_flag[current_level] = false;
  qml_main_loop_level--;

  if (qml_main_loop_level == 0)
    iupLoopCallExitCb();

  return IUP_NOERROR;
}

extern "C" IUP_API int IupLoopStepWait(void)
{
  QGuiApplication* app = iupqmlGetApplication();
  if (!app)
    return IUP_DEFAULT;

  QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

  if (QCoreApplication::closingDown() || qmlLoopExitPending())
    return IUP_CLOSE;

  return IUP_DEFAULT;
}

extern "C" IUP_API int IupLoopStep(void)
{
  QGuiApplication* app = iupqmlGetApplication();
  if (!app)
    return IUP_DEFAULT;

  QCoreApplication::processEvents(QEventLoop::AllEvents);

  if (QCoreApplication::closingDown() || qmlLoopExitPending())
    return IUP_CLOSE;

  return IUP_DEFAULT;
}

extern "C" IUP_API void IupFlush(void)
{
  int count = 0;

  IFidle old_qml_idle_cb = nullptr;
  if (qml_idle_cb)
  {
    old_qml_idle_cb = qml_idle_cb;
    iupdrvSetIdleFunction(nullptr);
  }

  for (int i = 0; i < 10 && count < 100; i++, count++)
  {
    QCoreApplication::processEvents(QEventLoop::AllEvents);
  }

  if (old_qml_idle_cb)
    iupdrvSetIdleFunction(reinterpret_cast<Icallback>(old_qml_idle_cb));
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
} qmlPostMessageUserData;

static void qmlPostMessageExecute(qmlPostMessageUserData* user_data)
{
  if (!user_data)
  {
    return;
  }

  Ihandle* ih = user_data->ih;
  if (iupObjectCheck(ih))
  {
    auto cb = reinterpret_cast<IFnsidv>(IupGetCallback(ih, "POSTMESSAGE_CB"));
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
  auto* user_data = static_cast<qmlPostMessageUserData*>(malloc(sizeof(qmlPostMessageUserData)));
  user_data->ih = ih;
  user_data->s = iupStrDup(s);
  user_data->i = i;
  user_data->d = d;
  user_data->p = p;

  QTimer::singleShot(0, QCoreApplication::instance(), [user_data]() {
    qmlPostMessageExecute(user_data);
  });
}

/****************************************************************************
 * Loop Cleanup
 ****************************************************************************/

IUP_DRV_API void iupqmlLoopCleanup()
{
  if (qml_idle_timer)
  {
    qml_idle_timer->stop();
    delete qml_idle_timer;
    qml_idle_timer = nullptr;
  }

  qml_idle_cb = nullptr;
  qml_main_loop_level = 0;
  qml_loop_exit_requested = false;
}

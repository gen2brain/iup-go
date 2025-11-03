/** \file
 * \brief Timer for the Qt Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <QTimer>
#include <QElapsedTimer>

#include <cstdio>
#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"
}

#include "iupqt_drv.h"


typedef struct _IupQtTimer
{
  QTimer* qtimer;
  QElapsedTimer* elapsed_timer;
  Ihandle* ih;
} IupQtTimer;

/****************************************************************************
 * Timer Callback
 ****************************************************************************/

static void qtTimerProc(IupQtTimer* timer_data)
{
  Ihandle* ih = timer_data->ih;
  Icallback cb;

  if (!iupObjectCheck(ih))   /* control could be destroyed before timer callback */
    return;

  cb = IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    qint64 elapsed = timer_data->elapsed_timer->elapsed();
    iupAttribSetInt(ih, "ELAPSEDTIME", (int)elapsed);

    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

/****************************************************************************
 * Timer Management Functions
 ****************************************************************************/

extern "C" void iupdrvTimerRun(Ihandle *ih)
{
  unsigned int time_ms;

  if (ih->serial > 0) /* timer already started */
    return;

  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    IupQtTimer* timer_data = new IupQtTimer();
    if (!timer_data)
      return;

    timer_data->ih = ih;
    timer_data->qtimer = new QTimer();
    timer_data->elapsed_timer = new QElapsedTimer();

    if (!timer_data->qtimer || !timer_data->elapsed_timer)
    {
      if (timer_data->qtimer) delete timer_data->qtimer;
      if (timer_data->elapsed_timer) delete timer_data->elapsed_timer;
      delete timer_data;
      return;
    }

    timer_data->qtimer->setInterval(time_ms);

    /* Set priority if requested - PRIORITY_HIGH uses PreciseTimer for better accuracy */
    if (iupAttribGetBoolean(ih, "PRIORITY_HIGH"))
      timer_data->qtimer->setTimerType(Qt::PreciseTimer);
    else
      timer_data->qtimer->setTimerType(Qt::CoarseTimer);

    /* Lambda captures timer_data and calls the callback function */
    QObject::connect(timer_data->qtimer, &QTimer::timeout, [timer_data]() {
      qtTimerProc(timer_data);
    });

    timer_data->elapsed_timer->start();
    timer_data->qtimer->start();

    /* Store pointer in serial field (cast to int) */
    ih->serial = 1; /* Mark as active */
    iupAttribSet(ih, "_IUP_QTTIMER", (char*)timer_data);
  }
}

extern "C" void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    IupQtTimer* timer_data = (IupQtTimer*)iupAttribGet(ih, "_IUP_QTTIMER");

    if (timer_data)
    {
      if (timer_data->qtimer)
      {
        timer_data->qtimer->stop();
        delete timer_data->qtimer;
      }

      /* Free elapsed timer (GTK leaks this, we don't!) */
      if (timer_data->elapsed_timer)
        delete timer_data->elapsed_timer;

      delete timer_data;

      iupAttribSet(ih, "_IUP_QTTIMER", nullptr);
    }

    ih->serial = -1;
  }
}

extern "C" void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}

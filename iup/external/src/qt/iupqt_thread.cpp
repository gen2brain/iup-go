/** \file
 * \brief Qt Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#include <QThread>
#include <QMutex>

extern "C" {
#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"
}

class IupQtThread : public QThread
{
public:
  IupQtThread(Ihandle* ih) : m_ih(ih) {}

protected:
  void run() override
  {
    Icallback cb = IupGetCallback(m_ih, "THREAD_CB");
    if (cb)
      cb(m_ih);
  }

private:
  Ihandle* m_ih;
};

extern "C" void* iupdrvThreadStart(Ihandle* ih)
{
  IupQtThread* thread = new IupQtThread(ih);

  const char* name = iupAttribGet(ih, "THREADNAME");
  if (name)
    thread->setObjectName(QString::fromUtf8(name));

  thread->start();
  return (void*)thread;
}

extern "C" void iupdrvThreadJoin(void* handle)
{
  ((IupQtThread*)handle)->wait();
}

extern "C" void iupdrvThreadYield(void)
{
  QThread::yieldCurrentThread();
}

extern "C" int iupdrvThreadIsCurrent(void* handle)
{
  return (IupQtThread*)handle == QThread::currentThread();
}

extern "C" void iupdrvThreadExit(int code)
{
  (void)code;
  QThread::currentThread()->quit();
}

extern "C" void iupdrvThreadDestroy(void* handle)
{
  if (handle)
  {
    IupQtThread* thread = (IupQtThread*)handle;
    if (thread->isRunning())
    {
      thread->wait();
    }
    delete thread;
  }
}

extern "C" void* iupdrvMutexCreate(void)
{
  return (void*)new QMutex();
}

extern "C" void iupdrvMutexLock(void* handle)
{
  ((QMutex*)handle)->lock();
}

extern "C" void iupdrvMutexUnlock(void* handle)
{
  ((QMutex*)handle)->unlock();
}

extern "C" void iupdrvMutexDestroy(void* handle)
{
  delete (QMutex*)handle;
}

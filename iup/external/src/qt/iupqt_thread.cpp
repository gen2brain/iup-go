/** \file
 * \brief Qt Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#include <QThread>
#include <QMutex>

extern "C" {
#include "iup.h"

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

extern "C" IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
  IupQtThread* thread = new IupQtThread(ih);

  const char* name = iupAttribGet(ih, "THREADNAME");
  if (name)
    thread->setObjectName(QString::fromUtf8(name));

  thread->start();
  return (void*)thread;
}

extern "C" IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  ((IupQtThread*)handle)->wait();
}

extern "C" IUP_SDK_API void iupdrvThreadYield(void)
{
  QThread::yieldCurrentThread();
}

extern "C" IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  return (IupQtThread*)handle == QThread::currentThread();
}

extern "C" IUP_SDK_API void iupdrvThreadExit(int code)
{
  (void)code;
  QThread::currentThread()->quit();
}

extern "C" IUP_SDK_API void iupdrvThreadDestroy(void* handle)
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

extern "C" IUP_SDK_API void* iupdrvMutexCreate(void)
{
  return (void*)new QMutex();
}

extern "C" IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  ((QMutex*)handle)->lock();
}

extern "C" IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  ((QMutex*)handle)->unlock();
}

extern "C" IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  delete (QMutex*)handle;
}

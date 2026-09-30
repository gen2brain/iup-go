/** \file
 * \brief Qt Quick Driver - Thread Support
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

class IupQmlThread : public QThread
{
public:
  IupQmlThread(Ihandle* ih) : m_ih(ih) {}

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
  auto* thread = new IupQmlThread(ih);

  const char* name = iupAttribGet(ih, "THREADNAME");
  if (name)
    thread->setObjectName(QString::fromUtf8(name));

  thread->start();
  return reinterpret_cast<void*>(thread);
}

extern "C" IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  (static_cast<IupQmlThread*>(handle))->wait();
}

extern "C" IUP_SDK_API void iupdrvThreadYield(void)
{
  QThread::yieldCurrentThread();
}

extern "C" IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  return static_cast<IupQmlThread*>(handle) == QThread::currentThread();
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
    auto* thread = static_cast<IupQmlThread*>(handle);
    if (thread->isRunning())
    {
      thread->wait();
    }
    delete thread;
  }
}

extern "C" IUP_SDK_API void* iupdrvMutexCreate(void)
{
  return reinterpret_cast<void*>(new QMutex());
}

extern "C" IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  (static_cast<QMutex*>(handle))->lock();
}

extern "C" IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  (static_cast<QMutex*>(handle))->unlock();
}

extern "C" IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  delete static_cast<QMutex*>(handle);
}

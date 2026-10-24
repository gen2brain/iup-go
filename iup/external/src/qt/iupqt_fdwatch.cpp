/** \file
 * \brief File Descriptor Watch for the Qt Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <QSocketNotifier>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"
}

#include "iupqt_drv.h"


class IupQtFdWatch : public QObject
{
public:
  IupQtFdWatch(Ihandle* ih, int fd, int mode)
    : fIhandle(ih), fRead(nullptr), fWrite(nullptr)
  {
    if (mode & IUP_FDWATCH_READ)
    {
      fRead = new QSocketNotifier(fd, QSocketNotifier::Read, this);
      QObject::connect(fRead, &QSocketNotifier::activated, this, [this]() { Fire(); });
    }
    if (mode & IUP_FDWATCH_WRITE)
    {
      fWrite = new QSocketNotifier(fd, QSocketNotifier::Write, this);
      QObject::connect(fWrite, &QSocketNotifier::activated, this, [this]() { Fire(); });
    }
  }

  void SetEnabled(bool enabled)
  {
    if (fRead) fRead->setEnabled(enabled);
    if (fWrite) fWrite->setEnabled(enabled);
  }

private:
  void Fire()
  {
    if (iupObjectCheck(fIhandle))
      iupFdWatchCallAction(fIhandle);
  }

  Ihandle* fIhandle;
  QSocketNotifier* fRead;
  QSocketNotifier* fWrite;
};

extern "C" IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPQT_FDWATCH"))
    return 1;

  auto* watch = new IupQtFdWatch(ih, iupFdWatchGetFd(ih), iupFdWatchGetMode(ih));
  iupAttribSet(ih, "_IUPQT_FDWATCH", reinterpret_cast<char*>(watch));
  return 1;
}

extern "C" IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  auto* watch = reinterpret_cast<IupQtFdWatch*>(iupAttribGet(ih, "_IUPQT_FDWATCH"));
  if (watch)
  {
    watch->SetEnabled(false);
    watch->deleteLater();
    iupAttribSet(ih, "_IUPQT_FDWATCH", nullptr);
  }
}

extern "C" IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}

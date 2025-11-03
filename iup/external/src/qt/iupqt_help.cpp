/** \file
 * \brief Qt Driver Help Support - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QDesktopServices>
#include <QUrl>
#include <QString>
#include <QWidget>
#include <QEvent>
#include <QProcess>
#include <QStringList>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Execute Program Asynchronously
 ****************************************************************************/

extern "C" IUP_API int IupExecute(const char* filename, const char* parameters)
{
  if (!filename || filename[0] == 0)
    return -1;

  QString program = QString::fromUtf8(filename);
  QStringList arguments;

  if (parameters && parameters[0] != 0)
  {
    QString params = QString::fromUtf8(parameters);
    arguments = params.split(' ', Qt::SkipEmptyParts);
  }

  qint64 pid;
  if (QProcess::startDetached(program, arguments, QString(), &pid))
    return 1;  /* Success */
  else
  {
    QProcess test;
    test.start(program, arguments);
    if (!test.waitForStarted(100))
    {
      QProcess::ProcessError error = test.error();
      if (error == QProcess::FailedToStart)
        return -2;  /* File not found or failed to start */
      else
        return -1;  /* Other error */
    }
    return -1;  /* Generic error */
  }
}

/****************************************************************************
 * Execute Program Synchronously with Wait
 ****************************************************************************/

extern "C" IUP_API int IupExecuteWait(const char* filename, const char* parameters)
{
  if (!filename || filename[0] == 0)
    return -1;

  QString program = QString::fromUtf8(filename);
  QStringList arguments;

  if (parameters && parameters[0] != 0)
  {
    QString params = QString::fromUtf8(parameters);
    arguments = params.split(' ', Qt::SkipEmptyParts);
  }

  int exitCode = QProcess::execute(program, arguments);

  if (exitCode == -2)
    return -2;  /* Process failed to start (file not found) */
  else if (exitCode == -1)
    return -1;  /* Process crashed */
  else
    return 1;  /* Success (exit code 0 or positive) */
}

/****************************************************************************
 * Open Help URL or File
 ****************************************************************************/

extern "C" IUP_API int IupHelp(const char* url)
{
  if (!url || url[0] == 0)
    return -1;

  QString urlString = QString::fromUtf8(url);
  QUrl qurl;

  if (urlString.startsWith("http://") || urlString.startsWith("https://") ||
      urlString.startsWith("file://") || urlString.startsWith("mailto:") ||
      urlString.startsWith("ftp://"))
  {
    qurl = QUrl(urlString);
  }
  else
  {
    qurl = QUrl::fromLocalFile(urlString);
  }

  if (QDesktopServices::openUrl(qurl))
    return 1;
  else
    return -1;
}

extern "C" int iupdrvHelp(const char* url)
{
  return IupHelp(url);
}

/****************************************************************************
 * Show Help for Widget (F1 key handler)
 ****************************************************************************/

extern "C" int iupqtShowHelp(QWidget* widget, Ihandle* ih)
{
  IFn help_cb;

  (void)widget;

  if (!ih)
    return 0;

  help_cb = (IFn)IupGetCallback(ih, "HELP_CB");
  if (help_cb)
  {
    if (help_cb(ih) == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }

  return 1;
}

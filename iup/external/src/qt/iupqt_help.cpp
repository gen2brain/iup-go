/** \file
 * \brief Qt Driver Help Support - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QDesktopServices>
#include <QUrl>
#include <QString>
#include <QProcess>
#include <QStringList>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iup_str.h"
}

static bool qtHelpSplitArguments(const char* filename, const char* parameters, QStringList& arguments)
{
  char** argv = iupStrSplitCommandLine(filename, parameters);
  if (!argv)
    return false;

  for (int i = 1; argv[i]; i++)
    arguments << QString::fromUtf8(argv[i]);

  free(argv);
  return true;
}

static bool qtHelpProgramExists(const QString& program)
{
  if (QDir::fromNativeSeparators(program).contains(QLatin1Char('/')))
    return QFileInfo::exists(program);
  return !QStandardPaths::findExecutable(program).isEmpty();
}

/****************************************************************************
 * Execute Program Asynchronously
 ****************************************************************************/

extern "C" IUP_API int IupExecute(const char* filename, const char* parameters)
{
  if (!filename || filename[0] == 0)
    return -1;

  QString program = QString::fromUtf8(filename);
  QStringList arguments;

  if (!qtHelpSplitArguments(filename, parameters, arguments))
    return -1;

  if (!qtHelpProgramExists(program))
    return -2;

  if (QProcess::startDetached(program, arguments))
    return 1;

  return -1;
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

  if (!qtHelpSplitArguments(filename, parameters, arguments))
    return -1;

  int exitCode = QProcess::execute(program, arguments);

  if (exitCode == -2)
    return -2;
  else if (exitCode == -1)
    return -1;
  else
    return 1;
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

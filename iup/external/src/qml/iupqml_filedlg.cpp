/** \file
 * \brief IupFileDlg pre-defined dialog - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickWindow>
#include <QEventLoop>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QUrl>
#include <QList>
#include <QVariant>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_array.h"
}

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__)
#include "unix/iupunix_portal.h"
#define IUPQML_FILEDLG_PORTAL
#endif

#include "iupqml_drv.h"


#define IUPQML_FILEMODE_OPEN  0
#define IUPQML_FILEMODE_OPENS 1
#define IUPQML_FILEMODE_SAVE  2

#define IUPQML_FILEOPT_DONTCONFIRMOVERWRITE 0x00000004

static int qmlIsFile(const QString& path)
{
  QFileInfo info(path);
  return info.isFile();
}

static int qmlIsDirectory(const QString& path)
{
  QFileInfo info(path);
  return info.isDir();
}

static char* qmlFileCheckExt(Ihandle* ih, const char* filename)
{
  char* ext = iupAttribGet(ih, "EXTDEFAULT");
  if (ext)
  {
    int len = static_cast<int>(strlen(filename));
    int ext_len = static_cast<int>(strlen(ext));

    if (len > ext_len && filename[len - ext_len - 1] == '.')
    {
      if (strcmp(filename + len - ext_len, ext) == 0)
        return const_cast<char*>(filename);
    }

    const char* dot = strrchr(filename, '.');
    const char* slash = strrchr(filename, '/');
    const char* backslash = strrchr(filename, '\\');

    if (!dot || (slash && dot < slash) || (backslash && dot < backslash))
    {
      int new_len = len + ext_len + 2;
      char* new_filename = static_cast<char*>(malloc(new_len));
      snprintf(new_filename, new_len, "%s.%s", filename, ext);
      return new_filename;
    }
  }

  return const_cast<char*>(filename);
}

static void qmlFileDlgGetMultipleFiles(Ihandle* ih, const QStringList& files)
{
  if (files.isEmpty())
    return;

  QFileInfo info(files[0]);
  QString dir = info.absolutePath();

  if (!dir.endsWith('/') && !dir.endsWith('\\'))
    dir += '/';

  QByteArray dirBytes = dir.toUtf8();
  iupAttribSetStr(ih, "DIRECTORY", dirBytes.constData());

  int dir_len = dirBytes.length();

  if (files.count() == 1)
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dirBytes.constData());

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    QByteArray fileBytes = files[0].toUtf8();
    iupAttribSetStrId(ih, "MULTIVALUE", 1, fileBytes.constData() + dir_len);
    iupAttribSetStr(ih, "VALUE", fileBytes.constData());

    iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
  }
  else
  {
    Iarray* names_array = iupArrayCreate(1024, sizeof(char));
    char* all_names;
    int cur_len, count = 0;

    int len = dir_len;
    if (dirBytes[len - 1] == '/' || dirBytes[len - 1] == '\\')
      len--;

    all_names = static_cast<char*>(iupArrayAdd(names_array, len + 1));
    memcpy(all_names, dirBytes.constData(), len);
    all_names[len] = '|';

    iupAttribSetStrId(ih, "MULTIVALUE", 0, dirBytes.constData());
    count++;

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    for (const QString& file : files)
    {
      QByteArray fileBytes = file.toUtf8();
      len = fileBytes.length() - dir_len;

      cur_len = iupArrayCount(names_array);

      all_names = static_cast<char*>(iupArrayAdd(names_array, len + 1));
      memcpy(all_names + cur_len, fileBytes.constData() + dir_len, len);
      all_names[cur_len + len] = '|';

      iupAttribSetStrId(ih, "MULTIVALUE", count, fileBytes.constData() + dir_len);
      count++;
    }

    iupAttribSetInt(ih, "MULTIVALUECOUNT", count);

    cur_len = iupArrayCount(names_array);
    all_names = static_cast<char*>(iupArrayInc(names_array));
    all_names[cur_len] = 0;

    iupAttribSetStr(ih, "VALUE", all_names);

    iupArrayDestroy(names_array);
  }
}

static QStringList qmlFileDlgSelectedFiles(QObject* dialog, int is_dir)
{
  QStringList files;

  if (is_dir)
  {
    QUrl url = dialog->property("selectedFolder").toUrl();
    if (url.isValid() && !url.isEmpty())
      files << url.toLocalFile();
    return files;
  }

  auto urls = dialog->property("selectedFiles").value<QList<QUrl> >();
  for (const QUrl& url : urls)
  {
    if (url.isValid() && !url.isEmpty())
      files << url.toLocalFile();
  }

  if (files.isEmpty())
  {
    QUrl url = dialog->property("selectedFile").toUrl();
    if (url.isValid() && !url.isEmpty())
      files << url.toLocalFile();
  }

  return files;
}

static void qmlFileDlgShowError(Ihandle* ih, const char* message)
{
  IupMessageError(IupGetDialog(ih), message);
}

static int qmlFileDlgPopup(Ihandle* ih, int x, int y)
{
  QQuickWindow* host;
  int host_owned;
  const char* value;
  int dialogtype;
  int is_dir = 0;
  auto file_cb = reinterpret_cast<IFnss>(IupGetCallback(ih, "FILE_CB"));
  QObject* dialog;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

#ifdef IUPQML_FILEDLG_PORTAL
  {
    int use_portal;
    value = iupAttribGet(ih, "PORTAL");
    if (value)
      use_portal = iupStrBoolean(value);
    else
      use_portal = IupGetGlobal("SANDBOX") != nullptr;

    if (use_portal && iupUnixPortalFileDialog(ih) == IUP_NOERROR)
      return IUP_NOERROR;
  }
#endif

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
    dialogtype = 1;
  else if (iupStrEqualNoCase(value, "DIR"))
  {
    dialogtype = 2;
    is_dir = 1;
    file_cb = nullptr;
  }
  else
    dialogtype = 0;

  if (is_dir)
    dialog = iupqmlCreateObject("import QtQuick.Dialogs\nFolderDialog { }");
  else
    dialog = iupqmlCreateObject("import QtQuick.Dialogs\nFileDialog { }");
  if (!dialog)
    return IUP_ERROR;

  value = iupAttribGet(ih, "TITLE");
  if (value)
    dialog->setProperty("title", QString::fromUtf8(value));

  host = iupqmlDialogHostWindow(ih, &host_owned);
  dialog->setProperty("parentWindow", QVariant::fromValue<QObject*>(host));
  dialog->setProperty("modality", static_cast<int>(Qt::ApplicationModal));

  if (!is_dir)
  {
    int mode = IUPQML_FILEMODE_OPEN;
    if (dialogtype == 1)
      mode = IUPQML_FILEMODE_SAVE;
    else if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
      mode = IUPQML_FILEMODE_OPENS;
    dialog->setProperty("fileMode", mode);

    if (dialogtype == 1 && iupAttribGetBoolean(ih, "NOOVERWRITEPROMPT"))
      dialog->setProperty("options", IUPQML_FILEOPT_DONTCONFIRMOVERWRITE);

    value = iupAttribGet(ih, "EXTDEFAULT");
    if (value)
      dialog->setProperty("defaultSuffix", QString::fromUtf8(value));
  }

  value = iupAttribGet(ih, "FILE");
  if (value && value[0] != 0 && (value[0] == '/' || value[1] == ':'))
  {
    char* dir = iupStrFileGetPath(value);
    int len = static_cast<int>(strlen(dir));
    iupAttribSetStr(ih, "DIRECTORY", dir);
    free(dir);

    iupAttribSetStr(ih, "FILE", value + len);
  }

  value = iupAttribGet(ih, "DIRECTORY");
  if (value)
    dialog->setProperty("currentFolder", QUrl::fromLocalFile(QString::fromUtf8(value)));

  if (!is_dir)
  {
    value = iupAttribGet(ih, "FILE");
    if (value)
    {
      QString file = QString::fromUtf8(value);
      QFileInfo info(file);
      if (info.isRelative())
      {
        const char* dir = iupAttribGet(ih, "DIRECTORY");
        file = QDir(dir ? QString::fromUtf8(dir) : QDir::currentPath()).filePath(file);
      }
      if (dialogtype == 1 || qmlIsFile(file))
      {
        dialog->setProperty("selectedFile", QUrl::fromLocalFile(file));
        dialog->setProperty("currentFile", QUrl::fromLocalFile(file));
      }
    }

    value = iupAttribGet(ih, "EXTFILTER");
    if (value)
    {
      char* filters_str = iupStrDup(value);
      int filter_count = iupStrReplace(filters_str, '|', 0) / 2;

      QStringList filterList;
      char* name = filters_str;

      int filter_index = iupAttribGetInt(ih, "FILTERUSED");
      if (!filter_index)
        filter_index = 1;

      for (int i = 0; i < filter_count && name[0]; i++)
      {
        char* pattern = name + strlen(name) + 1;

        iupStrReplace(pattern, ';', ' ');

        QString filter = QString::fromUtf8(name) + " (" + QString::fromUtf8(pattern) + ")";
        filterList << filter;

        name = pattern + strlen(pattern) + 1;
      }

      dialog->setProperty("nameFilters", filterList);

      if (filter_index > 0 && filter_index <= filterList.count())
      {
        auto* selected = dialog->property("selectedNameFilter").value<QObject*>();
        if (selected)
          selected->setProperty("index", filter_index - 1);
      }

      free(filters_str);
    }
    else
    {
      value = iupAttribGet(ih, "FILTER");
      if (value)
      {
        char* info = iupAttribGet(ih, "FILTERINFO");
        if (!info)
          info = const_cast<char*>(value);

        char* filters_str = iupStrDup(value);
        iupStrReplace(filters_str, ';', ' ');

        QStringList filterList;
        filterList << (QString::fromUtf8(info) + " (" + QString::fromUtf8(filters_str) + ")");
        dialog->setProperty("nameFilters", filterList);

        free(filters_str);
      }
    }
  }

  if (file_cb)
    file_cb(ih, nullptr, const_cast<char*>("INIT"));

  QObject* current_slot = nullptr;
  if (file_cb && !is_dir)
  {
    current_slot = iupqmlConnect(dialog, "currentFileChanged()", [ih, dialog, file_cb](void**) {
      QString path = dialog->property("currentFile").toUrl().toLocalFile();
      if (path.isEmpty())
        return;
      QByteArray bytes = path.toUtf8();
      if (qmlIsFile(path))
        file_cb(ih, const_cast<char*>(bytes.constData()), const_cast<char*>("SELECT"));
      else
        file_cb(ih, const_cast<char*>(bytes.constData()), const_cast<char*>("OTHER"));
    });
  }

  int accepted;
  for (;;)
  {
    QEventLoop loop;
    accepted = 0;

    QObject* accept_slot = iupqmlConnect(dialog, "accepted()", [&accepted](void**) {
      accepted = 1;
    });
    QObject* visible_slot = iupqmlConnect(dialog, "visibleChanged()", [dialog, &loop](void**) {
      if (!dialog->property("visible").toBool())
        loop.quit();
    });

    iupqmlCallMethod(dialog, "open");
    if (dialog->property("visible").toBool())
      loop.exec();

    delete accept_slot;
    delete visible_slot;

    if (!accepted)
      break;

    if (file_cb)
    {
      QStringList selectedFiles = qmlFileDlgSelectedFiles(dialog, is_dir);
      if (!selectedFiles.isEmpty())
      {
        QByteArray pathBytes = selectedFiles[0].toUtf8();
        int ret = file_cb(ih, const_cast<char*>(pathBytes.constData()), const_cast<char*>("OK"));
        if (ret == IUP_IGNORE)
          continue;
        if (ret == IUP_CONTINUE)
        {
          char* val = iupAttribGet(ih, "FILE");
          if (val && !is_dir)
            dialog->setProperty("selectedFile", QUrl::fromLocalFile(QString::fromUtf8(val)));
          continue;
        }
      }
    }

    break;
  }

  delete current_slot;

  if (file_cb)
    file_cb(ih, nullptr, const_cast<char*>("FINISH"));

  if (accepted)
  {
    QStringList selectedFiles = qmlFileDlgSelectedFiles(dialog, is_dir);

    if (selectedFiles.isEmpty())
    {
      iupAttribSet(ih, "VALUE", nullptr);
      iupAttribSet(ih, "STATUS", "-1");
      delete dialog;
      if (host_owned)
        delete host;
      return IUP_NOERROR;
    }

    value = iupAttribGet(ih, "EXTFILTER");
    if (value)
    {
      auto* selected = dialog->property("selectedNameFilter").value<QObject*>();
      if (selected)
        iupAttribSetInt(ih, "FILTERUSED", selected->property("index").toInt() + 1);
    }

    if (dialogtype == 0)
    {
      const QString& filename = selectedFiles[0];
      int file_exist = qmlIsFile(filename);
      int dir_exist = qmlIsDirectory(filename);

      if (dir_exist)
      {
        qmlFileDlgShowError(ih, IupGetLanguageString("IUP_FILEISDIR"));
        delete dialog;
        if (host_owned)
          delete host;
        iupAttribSet(ih, "VALUE", nullptr);
        iupAttribSet(ih, "STATUS", "-1");
        return IUP_NOERROR;
      }

      if (!file_exist && !iupAttribGetBoolean(ih, "MULTIPLEFILES"))
      {
        value = iupAttribGet(ih, "ALLOWNEW");
        if (!value)
          value = "NO";

        if (!iupStrBoolean(value))
        {
          qmlFileDlgShowError(ih, IupGetLanguageString("IUP_FILENOTEXIST"));
          delete dialog;
          if (host_owned)
            delete host;
          iupAttribSet(ih, "VALUE", nullptr);
          iupAttribSet(ih, "STATUS", "-1");
          return IUP_NOERROR;
        }
      }
    }

    if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && !is_dir)
    {
      qmlFileDlgGetMultipleFiles(ih, selectedFiles);
      iupAttribSet(ih, "FILEEXIST", "YES");
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      const QString& filename = selectedFiles[0];
      QByteArray filenameBytes = filename.toUtf8();

      char* final_filename = qmlFileCheckExt(ih, filenameBytes.constData());
      iupAttribSetStr(ih, "VALUE", final_filename);

      QString final_path = QString::fromUtf8(final_filename);

      if (final_filename != filenameBytes.constData())
        free(final_filename);

      QFileInfo info(final_path);
      QString dir = info.absolutePath();
      if (!dir.endsWith('/'))
        dir += '/';
      iupAttribSetStr(ih, "DIRECTORY", dir.toUtf8().constData());

      int file_exist = qmlIsFile(final_path);
      int dir_exist = qmlIsDirectory(final_path);

      if (dir_exist)
      {
        iupAttribSet(ih, "FILEEXIST", nullptr);
        iupAttribSet(ih, "STATUS", "0");
      }
      else if (file_exist)
      {
        iupAttribSet(ih, "FILEEXIST", "YES");
        iupAttribSet(ih, "STATUS", "0");
      }
      else
      {
        iupAttribSet(ih, "FILEEXIST", "NO");
        iupAttribSet(ih, "STATUS", "1");
      }
    }

    if (!is_dir && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
    {
      QString folder = dialog->property("currentFolder").toUrl().toLocalFile();
      if (!folder.isEmpty())
        QDir::setCurrent(folder);
    }
  }
  else
  {
    iupAttribSet(ih, "FILTERUSED", nullptr);
    iupAttribSet(ih, "VALUE", nullptr);
    iupAttribSet(ih, "FILEEXIST", nullptr);
    iupAttribSet(ih, "STATUS", "-1");
  }

  delete dialog;
  if (host_owned)
    delete host;

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = qmlFileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWPREVIEW", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWHIDDEN", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

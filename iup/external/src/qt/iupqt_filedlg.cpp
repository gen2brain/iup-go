/** \file
 * \brief IupFileDlg pre-defined dialog - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QUrl>
#include <QMessageBox>
#include <QPushButton>
#include <QDialog>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPainter>
#include <QPixmap>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_predialogs.h"
#include "iup_array.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Preview Canvas Widget
 ****************************************************************************/

class IupQtPreviewCanvas : public QWidget
{
public:
  IupQtPreviewCanvas(Ihandle* ih, QWidget* parent = nullptr)
    : QWidget(parent), m_ih(ih), m_buffer(nullptr)
  {
    setMinimumSize(100, 100);
    setAttribute(Qt::WA_OpaquePaintEvent);
  }

  ~IupQtPreviewCanvas()
  {
    if (m_buffer)
    {
      delete m_buffer;
      m_buffer = nullptr;
    }
  }

  void setCurrentFile(const QString& filename)
  {
    m_currentFile = filename;
    update();
  }

  QPixmap* buffer()
  {
    QSize widget_size = size();
    if (!m_buffer || m_buffer->size() != widget_size)
    {
      if (m_buffer)
        delete m_buffer;
      m_buffer = new QPixmap(widget_size);
      m_buffer->fill(Qt::white);
    }
    return m_buffer;
  }

protected:
  void paintEvent(QPaintEvent* event) override
  {
    Q_UNUSED(event);

    iupAttribSet(m_ih, "_IUPQT_PREVIEW_CANVAS", (char*)this);
    iupAttribSet(m_ih, "_IUPQT_PREVIEW_BUFFER", (char*)buffer());

    IFnss cb = (IFnss)IupGetCallback(m_ih, "FILE_CB");
    if (cb)
    {
      QFileInfo fi(m_currentFile);
      QByteArray fileBytes = m_currentFile.toUtf8();
      if (fi.isFile())
        cb(m_ih, (char*)fileBytes.constData(), (char*)"PAINT");
      else
        cb(m_ih, nullptr, (char*)"PAINT");
    }

    iupAttribSet(m_ih, "_IUPQT_PREVIEW_CANVAS", nullptr);
    iupAttribSet(m_ih, "_IUPQT_PREVIEW_BUFFER", nullptr);

    if (m_buffer)
    {
      QPainter painter(this);
      painter.drawPixmap(0, 0, *m_buffer);
    }
  }

private:
  Ihandle* m_ih;
  QString m_currentFile;
  QPixmap* m_buffer;
};

/****************************************************************************
 * Custom File Dialog with Preview
 ****************************************************************************/

class IupQtFileDialogWithPreview : public QDialog
{
public:
  IupQtFileDialogWithPreview(Ihandle* ih, QWidget* parent = nullptr)
    : QDialog(parent), m_ih(ih), m_result(QDialog::Rejected)
  {
    m_fileDialog = new QFileDialog(this);
    m_fileDialog->setOption(QFileDialog::DontUseNativeDialog, true);

    int preview_width = iupAttribGetInt(ih, "PREVIEWWIDTH");
    int preview_height = iupAttribGetInt(ih, "PREVIEWHEIGHT");
    if (preview_width <= 0) preview_width = 200;
    if (preview_height <= 0) preview_height = 150;

    m_previewCanvas = new IupQtPreviewCanvas(ih, this);
    m_previewCanvas->setFixedWidth(preview_width);
    m_previewCanvas->setMinimumHeight(preview_height);

    iupAttribSetInt(ih, "PREVIEWWIDTH", preview_width);
    iupAttribSetInt(ih, "PREVIEWHEIGHT", preview_height);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_fileDialog);
    splitter->addWidget(m_previewCanvas);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);
    setLayout(layout);

    connect(m_fileDialog, &QFileDialog::currentChanged, this, &IupQtFileDialogWithPreview::onCurrentChanged);
    connect(m_fileDialog, &QFileDialog::accepted, this, &IupQtFileDialogWithPreview::onAccepted);
    connect(m_fileDialog, &QFileDialog::rejected, this, &IupQtFileDialogWithPreview::onRejected);

    resize(800, 500);
  }

  QFileDialog* fileDialog() const { return m_fileDialog; }
  IupQtPreviewCanvas* previewCanvas() const { return m_previewCanvas; }
  int dialogResult() const { return m_result; }

private:
  void onCurrentChanged(const QString& path)
  {
    IFnss cb = (IFnss)IupGetCallback(m_ih, "FILE_CB");
    if (cb)
    {
      QFileInfo fi(path);
      QByteArray pathBytes = path.toUtf8();
      if (fi.isFile())
        cb(m_ih, (char*)pathBytes.constData(), (char*)"SELECT");
      else
        cb(m_ih, (char*)pathBytes.constData(), (char*)"OTHER");
    }
    m_previewCanvas->setCurrentFile(path);
  }

  void onAccepted()
  {
    m_result = QDialog::Accepted;
    accept();
  }

  void onRejected()
  {
    m_result = QDialog::Rejected;
    reject();
  }

  Ihandle* m_ih;
  QFileDialog* m_fileDialog;
  IupQtPreviewCanvas* m_previewCanvas;
  int m_result;
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static int qtIsFile(const QString& path)
{
  QFileInfo fileInfo(path);
  return fileInfo.isFile();
}

static int qtIsDirectory(const QString& path)
{
  QFileInfo fileInfo(path);
  return fileInfo.isDir();
}

static char* qtFileCheckExt(Ihandle* ih, const char* filename)
{
  char* ext = iupAttribGet(ih, "EXTDEFAULT");
  if (ext)
  {
    int len = (int)strlen(filename);
    int ext_len = (int)strlen(ext);

    /* Check if filename already has the extension */
    if (len > ext_len && filename[len - ext_len - 1] == '.')
    {
      if (strcmp(filename + len - ext_len, ext) == 0)
        return (char*)filename; /* already has extension */
    }

    /* Check if filename has any extension */
    const char* dot = strrchr(filename, '.');
    const char* slash = strrchr(filename, '/');
    const char* backslash = strrchr(filename, '\\');

    /* If no dot, or dot is before last separator, add extension */
    if (!dot || (slash && dot < slash) || (backslash && dot < backslash))
    {
      char* new_filename = (char*)malloc(len + ext_len + 2);
      strcpy(new_filename, filename);
      new_filename[len] = '.';
      strcpy(new_filename + len + 1, ext);
      return new_filename;
    }
  }

  return (char*)filename;
}

static void qtFileDlgGetMultipleFiles(Ihandle* ih, const QStringList& files)
{
  if (files.isEmpty())
    return;

  QString firstFile = files[0];
  QFileInfo fileInfo(firstFile);
  QString dir = fileInfo.absolutePath();

  if (!dir.endsWith('/') && !dir.endsWith('\\'))
    dir += '/';

  QByteArray dirBytes = dir.toUtf8();
  iupAttribSetStr(ih, "DIRECTORY", dirBytes.constData());

  int dir_len = dirBytes.length();

  /* Check if just one file is selected */
  if (files.count() == 1)
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dirBytes.constData());

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    QByteArray fileBytes = firstFile.toUtf8();
    iupAttribSetStrId(ih, "MULTIVALUE", 1, fileBytes.constData() + dir_len);
    iupAttribSetStr(ih, "VALUE", fileBytes.constData());

    iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
  }
  else
  {
    Iarray* names_array = iupArrayCreate(1024, sizeof(char));
    char* all_names;
    int cur_len, count = 0;

    /* Add directory to array */
    int len = dir_len;
    if (dirBytes[len - 1] == '/' || dirBytes[len - 1] == '\\')
      len--; /* remove last separator */

    all_names = (char*)iupArrayAdd(names_array, len + 1);
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

      all_names = (char*)iupArrayAdd(names_array, len + 1);
      memcpy(all_names + cur_len, fileBytes.constData() + dir_len, len);
      all_names[cur_len + len] = '|';

      iupAttribSetStrId(ih, "MULTIVALUE", count, fileBytes.constData() + dir_len);
      count++;
    }

    iupAttribSetInt(ih, "MULTIVALUECOUNT", count);

    cur_len = iupArrayCount(names_array);
    all_names = (char*)iupArrayInc(names_array);
    all_names[cur_len] = 0;

    iupAttribSetStr(ih, "VALUE", all_names);

    iupArrayDestroy(names_array);
  }
}

/****************************************************************************
 * File Dialog Popup
 ****************************************************************************/

static int qtFileDlgPopup(Ihandle* ih, int x, int y)
{
  QWidget* parent = (QWidget*)iupDialogGetNativeParent(ih);
  QFileDialog* dialog;
  QFileDialog::FileMode file_mode;
  QFileDialog::AcceptMode accept_mode;
  const char* value;
  int dialogtype;
  IupQtFileDialogWithPreview* preview_dialog = nullptr;
  IFnss file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  /* Check if preview mode is requested */
  if (iupAttribGetBoolean(ih, "SHOWPREVIEW") && file_cb)
  {
    preview_dialog = new IupQtFileDialogWithPreview(ih, parent);
    dialog = preview_dialog->fileDialog();

    file_cb(ih, nullptr, (char*)"INIT");
  }
  else
  {
    /* Create standard dialog */
    dialog = new QFileDialog(parent);

    if (!dialog)
      return IUP_ERROR;

    /* Handle PORTAL attribute:
       PORTAL=YES: Use native/portal dialog (default Qt behavior on Linux with XDG portal)
       PORTAL=NO: Force Qt's widget-based dialog (DontUseNativeDialog)
       When PORTAL is not set: use native/portal in sandbox, otherwise Qt decides based on platform */
    value = iupAttribGet(ih, "PORTAL");
    if (value)
    {
      if (!iupStrBoolean(value))
      {
        /* PORTAL=NO: Force Qt's built-in widget dialog */
        dialog->setOption(QFileDialog::DontUseNativeDialog, true);
      }
      /* PORTAL=YES: Let Qt use native/portal (default behavior, no action needed) */
    }
  }

  /* Determine dialog type */
  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
  {
    accept_mode = QFileDialog::AcceptSave;
    file_mode = QFileDialog::AnyFile;
    dialogtype = 1; /* SAVE */
  }
  else if (iupStrEqualNoCase(value, "DIR"))
  {
    accept_mode = QFileDialog::AcceptOpen;
    file_mode = QFileDialog::Directory;
    dialogtype = 2; /* DIR */
  }
  else /* OPEN */
  {
    accept_mode = QFileDialog::AcceptOpen;
    file_mode = QFileDialog::ExistingFile;
    dialogtype = 0; /* OPEN */
  }

  dialog->setAcceptMode(accept_mode);
  dialog->setFileMode(file_mode);

  value = iupAttribGet(ih, "TITLE");
  if (value)
    dialog->setWindowTitle(QString::fromUtf8(value));

  /* Show hidden files */
  if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
  {
    QFileDialog::Options options = dialog->options();
    options |= QFileDialog::HideNameFilterDetails;
    dialog->setOptions(options);
  }

  /* Multiple files */
  if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && accept_mode == QFileDialog::AcceptOpen)
    dialog->setFileMode(QFileDialog::ExistingFiles);

  /* Overwrite prompt */
  if (!iupAttribGetBoolean(ih, "NOOVERWRITEPROMPT") && accept_mode == QFileDialog::AcceptSave)
  {
    QFileDialog::Options options = dialog->options();
    options &= ~QFileDialog::DontConfirmOverwrite;
    dialog->setOptions(options);
  }

  /* Handle FILE attribute - check if it contains a path */
  value = iupAttribGet(ih, "FILE");
  if (value && value[0] != 0 && (value[0] == '/' || value[1] == ':'))
  {
    char* dir = iupStrFileGetPath(value);
    int len = (int)strlen(dir);
    iupAttribSetStr(ih, "DIRECTORY", dir);
    free(dir);

    iupAttribSetStr(ih, "FILE", value + len);
  }

  value = iupAttribGet(ih, "DIRECTORY");
  if (value)
    dialog->setDirectory(QString::fromUtf8(value));

  value = iupAttribGet(ih, "FILE");
  if (value)
  {
    if (accept_mode == QFileDialog::AcceptSave)
      dialog->selectFile(QString::fromUtf8(value));
    else
    {
      /* For open, only set if file exists */
      if (qtIsFile(QString::fromUtf8(value)))
        dialog->selectFile(QString::fromUtf8(value));
    }
  }

  value = iupAttribGet(ih, "EXTFILTER");
  if (value)
  {
    /* Parse EXTFILTER format: "Description1|Pattern1;Pattern2|Description2|Pattern3|..." */
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

      /* Convert semicolons to spaces for Qt format */
      iupStrReplace(pattern, ';', ' ');

      /* Build Qt filter string: "Description (*.ext1 *.ext2)" */
      QString filter = QString::fromUtf8(name) + " (" + QString::fromUtf8(pattern) + ")";
      filterList << filter;

      name = pattern + strlen(pattern) + 1;
    }

    dialog->setNameFilters(filterList);

    if (filter_index > 0 && filter_index <= filterList.count())
      dialog->selectNameFilter(filterList[filter_index - 1]);

    free(filters_str);
  }
  else
  {
    value = iupAttribGet(ih, "FILTER");
    if (value)
    {
      char* info = iupAttribGet(ih, "FILTERINFO");
      if (!info)
        info = (char*)value;

      char* filters_str = iupStrDup(value);
      iupStrReplace(filters_str, ';', ' ');

      QString filter = QString::fromUtf8(info) + " (" + QString::fromUtf8(filters_str) + ")";
      dialog->setNameFilter(filter);

      free(filters_str);
    }
  }

  /* Add Help button if HELP_CB exists (feature parity with Windows, GTK, Cocoa) */
  if (IupGetCallback(ih, "HELP_CB"))
  {
    QPushButton* help_button = dialog->findChild<QPushButton*>("qt_custom_help_button");
    if (!help_button)
    {
      help_button = new QPushButton(QString::fromUtf8(IupGetLanguageString("IUP_HELP") ? IupGetLanguageString("IUP_HELP") : "Help"));
      help_button->setObjectName("qt_custom_help_button");

      QObject::connect(help_button, &QPushButton::clicked, [ih, dialog]() {
        Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
        if (cb && cb(ih) == IUP_CLOSE)
          dialog->reject();
      });

      /* Add help button to dialog button box */
      QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
      if (!buttons.isEmpty())
      {
        QWidget* buttonBox = buttons[0]->parentWidget();
        if (buttonBox)
        {
          help_button->setParent(buttonBox);
          help_button->show();
        }
      }
    }
  }

  /* Position dialog */
  QDialog* exec_dialog = preview_dialog ? (QDialog*)preview_dialog : (QDialog*)dialog;
  ih->handle = (InativeHandle*)exec_dialog;
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;

  /* Set window title on preview dialog if needed */
  if (preview_dialog)
  {
    value = iupAttribGet(ih, "TITLE");
    if (value)
      preview_dialog->setWindowTitle(QString::fromUtf8(value));
  }

  /* Show dialog */
  int result;
  do
  {
    result = exec_dialog->exec();

    /* Handle HELP button response */
    if (result == QDialog::Rejected)
    {
      /* Check if help button was clicked (we handle this in the lambda above) */
      break;
    }

  } while (0);

  /* Call FINISH callback for preview mode */
  if (preview_dialog && file_cb)
  {
    file_cb(ih, nullptr, (char*)"FINISH");
  }

  /* Process result */
  if (result == QDialog::Accepted)
  {
    QStringList selectedFiles = dialog->selectedFiles();

    if (selectedFiles.isEmpty())
    {
      iupAttribSet(ih, "VALUE", NULL);
      iupAttribSet(ih, "STATUS", "-1");
      if (preview_dialog)
        delete preview_dialog;
      else
        delete dialog;
      return IUP_NOERROR;
    }

    /* Handle filter tracking */
    value = iupAttribGet(ih, "EXTFILTER");
    if (value)
    {
      QString selectedFilter = dialog->selectedNameFilter();
      QStringList filterList = dialog->nameFilters();

      for (int i = 0; i < filterList.count(); i++)
      {
        if (filterList[i] == selectedFilter)
        {
          iupAttribSetInt(ih, "FILTERUSED", i + 1);
          break;
        }
      }
    }

    /* Handle ALLOWNEW validation (feature parity with GTK) */
    if (dialogtype == 0) /* OPEN */
    {
      QString filename = selectedFiles[0];
      int file_exist = qtIsFile(filename);
      int dir_exist = qtIsDirectory(filename);

      if (dir_exist)
      {
        /* File is actually a directory */
        QMessageBox::critical(exec_dialog, QString::fromUtf8("Error"),
                            QString::fromUtf8("The selected path is a directory, not a file."));
        if (preview_dialog)
          delete preview_dialog;
        else
          delete dialog;
        iupAttribSet(ih, "VALUE", NULL);
        iupAttribSet(ih, "STATUS", "-1");
        return IUP_NOERROR;
      }

      if (!file_exist && !iupAttribGetBoolean(ih, "MULTIPLEFILES"))
      {
        /* Check ALLOWNEW */
        value = iupAttribGet(ih, "ALLOWNEW");
        if (!value)
          value = "NO"; /* Default for OPEN is NO */

        if (!iupStrBoolean(value))
        {
          QMessageBox::critical(exec_dialog, QString::fromUtf8("Error"),
                              QString::fromUtf8("The selected file does not exist."));
          if (preview_dialog)
            delete preview_dialog;
          else
            delete dialog;
          iupAttribSet(ih, "VALUE", NULL);
          iupAttribSet(ih, "STATUS", "-1");
          return IUP_NOERROR;
        }
      }
    }
    else if (dialogtype == 1) /* SAVE */
    {
      /* For SAVE, check ALLOWNEW */
      value = iupAttribGet(ih, "ALLOWNEW");
      if (!value)
        value = "YES"; /* Default for SAVE is YES */

      /* Qt already handles overwrite prompts if not disabled */
    }

    if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
    {
      qtFileDlgGetMultipleFiles(ih, selectedFiles);
      iupAttribSet(ih, "FILEEXIST", "YES");
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      QString filename = selectedFiles[0];
      QByteArray filenameBytes = filename.toUtf8();

      /* Check and add extension if needed */
      char* final_filename = qtFileCheckExt(ih, filenameBytes.constData());
      iupAttribSetStr(ih, "VALUE", final_filename);

      if (final_filename != filenameBytes.constData())
        free(final_filename);

      /* Store directory */
      QFileInfo fileInfo(filename);
      QByteArray dirBytes = fileInfo.absolutePath().toUtf8();
      iupAttribSetStr(ih, "DIRECTORY", dirBytes.constData());

      /* Check existence */
      int file_exist = qtIsFile(filename);
      int dir_exist = qtIsDirectory(filename);

      if (dir_exist)
      {
        iupAttribSet(ih, "FILEEXIST", NULL);
        iupAttribSet(ih, "STATUS", "0");
      }
      else
      {
        if (file_exist)
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
    }

    /* Change current directory if needed */
    if (file_mode != QFileDialog::Directory && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
    {
      QDir::setCurrent(dialog->directory().absolutePath());
    }
  }
  else
  {
    iupAttribSet(ih, "FILTERUSED", NULL);
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }

  if (preview_dialog)
    delete preview_dialog;
  else
    delete dialog;

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = qtFileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

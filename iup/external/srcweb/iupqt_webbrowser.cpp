/** \file
 * \brief WebBrowser Control - Qt Implementation using QtWebEngine
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWidget>
#include <QUrl>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageLayout>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QCoreApplication>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_webbrowser.h"


/* Forward declare Qt driver function we need */
/* This avoids including iupqt_drv.h and keeps web browser code independent */
void iupqtAddToParent(Ihandle* ih);
}

/****************************************************************************
 * Control Data Structure
 ****************************************************************************/

struct _IcontrolData
{
  int sb; /* scrollbar configuration */
};

/****************************************************************************
 * JavaScript Helper Functions
 ****************************************************************************/

/* Escape a string for safe inclusion in JavaScript code */
static char* qtWebBrowserEscapeJavaScript(const char* str)
{
  if (!str)
    return iupStrDup("null");

  size_t len = strlen(str);
  size_t escaped_len = len * 6 + 3;  /* Worst case: every char becomes \uXXXX */
  char* result = (char*)malloc(escaped_len);
  if (!result)
    return iupStrDup("null");

  char* p = result;
  *p++ = '"';

  for (const char* s = str; *s; s++)
  {
    switch (*s)
    {
      case '"':  *p++ = '\\'; *p++ = '"'; break;
      case '\\': *p++ = '\\'; *p++ = '\\'; break;
      case '\b': *p++ = '\\'; *p++ = 'b'; break;
      case '\f': *p++ = '\\'; *p++ = 'f'; break;
      case '\n': *p++ = '\\'; *p++ = 'n'; break;
      case '\r': *p++ = '\\'; *p++ = 'r'; break;
      case '\t': *p++ = '\\'; *p++ = 't'; break;
      default:
        if ((unsigned char)*s < 32)
        {
          sprintf(p, "\\u%04x", (unsigned char)*s);
          p += 6;
        }
        else
        {
          *p++ = *s;
        }
        break;
    }
  }

  *p++ = '"';
  *p = '\0';

  return result;
}

/* Structure for synchronous JavaScript execution */
struct JavaScriptResult
{
  QString result;
  bool completed;
  QMutex mutex;
  QEventLoop* loop;

  JavaScriptResult() : completed(false), loop(nullptr) {}
};

/* Run JavaScript asynchronously (fire and forget) */
static void qtWebBrowserRunJavaScript(Ihandle* ih, const char* format, ...)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return;

  char js[4096];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(js, sizeof(js), format, arglist);
  va_end(arglist);

  webview->page()->runJavaScript(QString::fromUtf8(js));
}

/* Run JavaScript synchronously and return the result */
static char* qtWebBrowserRunJavaScriptSync(Ihandle* ih, const char* format, ...)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  char js[4096];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(js, sizeof(js), format, arglist);
  va_end(arglist);

  JavaScriptResult js_result;
  js_result.loop = new QEventLoop();

  webview->page()->runJavaScript(QString::fromUtf8(js),
    [&js_result](const QVariant &result) {
      QMutexLocker locker(&js_result.mutex);
      js_result.result = result.toString();
      js_result.completed = true;
      if (js_result.loop)
        js_result.loop->quit();
    });

  /* Wait for JavaScript to complete (with timeout) - use manual event pumping
   * instead of exec() to avoid "event loop already running" errors */
  if (!js_result.completed)
  {
    QElapsedTimer timeout_timer;
    timeout_timer.start();

    while (!js_result.completed && timeout_timer.elapsed() < 5000)  /* 5 second timeout */
    {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);  /* Process events with 50ms timeout */
    }
  }

  delete js_result.loop;

  if (js_result.completed && !js_result.result.isEmpty())
    return iupStrReturnStr(js_result.result.toUtf8().constData());

  return nullptr;
}

/****************************************************************************
 * Document Initialization Script
 ****************************************************************************/

/* Initialize document with selection tracking and dirty flag support */
static void qtWebBrowserInitializeDocument(Ihandle* ih)
{
  /* No longer needed - initialization script is now injected automatically via QWebEngineScript */
  /* in qtWebBrowserMapMethod, and contentEditable is set in onLoadFinished */
  (void)ih;
}

/****************************************************************************
 * History Management
 ****************************************************************************/

static void qtWebBrowserUpdateHistory(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return;

  QWebEngineHistory* history = webview->page()->history();
  if (!history)
    return;

  bool can_go_back = history->canGoBack();
  bool can_go_forward = history->canGoForward();

  iupAttribSet(ih, "CANGOBACK", can_go_back ? "YES" : "NO");
  iupAttribSet(ih, "CANGOFORWARD", can_go_forward ? "YES" : "NO");
}

/****************************************************************************
 * Attribute Getters
 ****************************************************************************/

static char* qtWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  QWebEngineHistory* history = webview->page()->history();
  if (!history)
    return nullptr;

  QWebEngineHistoryItem item = history->itemAt(id);
  if (!item.isValid())
    return nullptr;

  return iupStrReturnStr(item.url().toString().toUtf8().constData());
}

static char* qtWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  QWebEngineHistory* history = webview->page()->history();
  if (!history)
    return iupStrReturnInt(0);

  return iupStrReturnInt(history->forwardItems(history->count()).count());
}

static char* qtWebBrowserGetBackCountAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  QWebEngineHistory* history = webview->page()->history();
  if (!history)
    return iupStrReturnInt(0);

  return iupStrReturnInt(history->backItems(history->count()).count());
}

static char* qtWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return iupStrReturnBoolean(0);

  return iupStrReturnBoolean(webview->page()->history()->canGoBack());
}

static char* qtWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return iupStrReturnBoolean(0);

  return iupStrReturnBoolean(webview->page()->history()->canGoForward());
}

/****************************************************************************
 * Core Attribute Setters
 ****************************************************************************/

static int qtWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview || !value)
    return 0;

  iupAttribSet(ih, "_IUPWEB_DIRTY", nullptr);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");

  webview->setHtml(QString::fromUtf8(value));

  /* No need to initialize document - script is injected automatically */

  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);
  return 0;
}

static char* qtWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  /* For editable content, use JavaScript to get the HTML */
  if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
  {
    return qtWebBrowserRunJavaScriptSync(ih,
      "'<html><head>' + document.head.innerHTML + '</head><body>' + document.body.innerHTML + '</body></html>';");
  }

  /* For non-editable content, we need to use toHtml() asynchronously */
  /* Store result in a temporary attribute */
  JavaScriptResult html_result;
  html_result.loop = new QEventLoop();

  webview->page()->toHtml([&html_result](const QString &html) {
    QMutexLocker locker(&html_result.mutex);
    html_result.result = html;
    html_result.completed = true;
    if (html_result.loop)
      html_result.loop->quit();
  });

  if (!html_result.completed)
  {
    QTimer::singleShot(5000, html_result.loop, &QEventLoop::quit);
    html_result.loop->exec();
  }

  delete html_result.loop;

  if (html_result.completed && !html_result.result.isEmpty())
    return iupStrReturnStr(html_result.result.toUtf8().constData());

  return nullptr;
}

static int qtWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview || !value)
    return 0;

  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
  webview->setUrl(QUrl::fromUserInput(QString::fromUtf8(value)));
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);

  return 0;
}

static char* qtWebBrowserGetValueAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  QUrl url = webview->url();
  if (url.isEmpty())
    return nullptr;

  return iupStrReturnStr(url.toString().toUtf8().constData());
}

static char* qtWebBrowserGetStatusAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  /* Check if we're currently loading */
  /* Qt5 doesn't have isLoading(), so we track via internal attribute */
  const char* loading = iupAttribGet(ih, "_IUPQT_WEB_LOADING");
  if (loading && iupStrEqualNoCase(loading, "YES"))
    return iupStrReturnStr("LOADING");

  return iupStrReturnStr("COMPLETED");
}

/****************************************************************************
 * Navigation Attribute Setters
 ****************************************************************************/

static int qtWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  int count = 1;
  if (value && *value)
    iupStrToInt(value, &count);

  QWebEngineHistory* history = webview->page()->history();
  if (history && history->canGoBack())
  {
    if (count == 1)
      history->back();
    else
      history->goToItem(history->itemAt(history->currentItemIndex() - count));
  }

  return 0;
}

static int qtWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  int count = 1;
  if (value && *value)
    iupStrToInt(value, &count);

  QWebEngineHistory* history = webview->page()->history();
  if (history && history->canGoForward())
  {
    if (count == 1)
      history->forward();
    else
      history->goToItem(history->itemAt(history->currentItemIndex() + count));
  }

  return 0;
}

static int qtWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->stop();
  return 0;
}

static int qtWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->reload();
  return 0;
}

static int qtWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview || !value)
    return 0;

  int count = 0;
  if (!iupStrToInt(value, &count))
    return 0;

  QWebEngineHistory* history = webview->page()->history();
  if (!history)
    return 0;

  int target_index = history->currentItemIndex() + count;
  if (target_index >= 0 && target_index < history->count())
    history->goToItem(history->itemAt(target_index));

  return 0;
}

/****************************************************************************
 * File Operations
 ****************************************************************************/

static int write_file(const char* filename, const char* str, int count)
{
  FILE* file = fopen(filename, "wb");
  if (!file)
    return 0;

  fwrite(str, 1, count, file);
  fclose(file);
  return 1;
}

static int qtWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  /* Get HTML content */
  char* html = qtWebBrowserGetHTMLAttrib(ih);
  if (!html)
    return 0;

  int success = write_file(value, html, strlen(html));
  return success;
}

static int qtWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview || !value)
    return 0;

  QFile file(QString::fromUtf8(value));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return 0;

  QTextStream in(&file);
  QString html = in.readAll();
  file.close();

  iupAttribSet(ih, "_IUPWEB_DIRTY", nullptr);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");

  webview->setHtml(html);

  /* No need to initialize document - script is injected automatically */

  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);
  return 1;
}

/****************************************************************************
 * Text Content Attributes
 ****************************************************************************/

static char* qtWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  return qtWebBrowserRunJavaScriptSync(ih, "document.body.innerText;");
}

static int qtWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  char* escaped = qtWebBrowserEscapeJavaScript(value);
  qtWebBrowserRunJavaScript(ih, "document.body.innerText = %s;", escaped);
  free(escaped);

  return 0;
}

/****************************************************************************
 * Zoom Attributes
 ****************************************************************************/

static int qtWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview || !value)
    return 0;

  int zoom_percent = 100;
  if (!iupStrToInt(value, &zoom_percent))
    return 0;

  qreal zoom_factor = (qreal)zoom_percent / 100.0;
  webview->setZoomFactor(zoom_factor);

  return 0;
}

static char* qtWebBrowserGetZoomAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  qreal zoom_factor = webview->zoomFactor();
  int zoom_percent = (int)(zoom_factor * 100.0);

  return iupStrReturnInt(zoom_percent);
}

/****************************************************************************
 * Print Attributes
 ****************************************************************************/

static int qtWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  QPrinter printer;
  QPrintDialog dialog(&printer, webview);

  if (dialog.exec() == QDialog::Accepted)
  {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    /* Qt6: Use printToPdf() instead of print() */
    webview->page()->printToPdf([](const QByteArray &) {
      /* Printing completed */
    });
#else
    /* Qt5: Use print() */
    webview->page()->print(&printer, [](bool success) {
      (void)success;
    });
#endif
  }

  return 0;
}

static int qtWebBrowserSetPrintPreviewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  QPrinter printer;
  QPrintPreviewDialog preview(&printer, webview);

  QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
    [webview](QPrinter *printer) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      /* Qt6: Use printToPdf() instead of print() */
      webview->page()->printToPdf([](const QByteArray &) {
        /* Printing completed */
      });
#else
      /* Qt5: Use print() */
      webview->page()->print(printer, [](bool success) {
        (void)success;
      });
#endif
    });

  preview.exec();
  return 0;
}

/****************************************************************************
 * Editing Mode Attributes
 ****************************************************************************/

static int qtWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  if (iupStrBoolean(value))
  {
    iupAttribSet(ih, "_IUPWEB_EDITABLE", "1");
    qtWebBrowserRunJavaScript(ih, "document.body.contentEditable = 'true';");
  }
  else
  {
    iupAttribSet(ih, "_IUPWEB_EDITABLE", nullptr);
    qtWebBrowserRunJavaScript(ih, "document.body.contentEditable = 'false';");
  }

  return 0;
}

static char* qtWebBrowserGetEditableAttrib(Ihandle* ih)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return nullptr;

  /* Query the actual state from the browser, like Windows does */
  char* result = qtWebBrowserRunJavaScriptSync(ih, "document.body.contentEditable == 'true';");
  if (result)
  {
    int val = strcmp(result, "true") == 0;
    free(result);
    return iupStrReturnBoolean(val);
  }
  return iupStrReturnBoolean(0);
}

/****************************************************************************
 * Clipboard Operations
 ****************************************************************************/

static int qtWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->page()->triggerAction(QWebEnginePage::Copy);
  return 0;
}

static int qtWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->page()->triggerAction(QWebEnginePage::Cut);
  return 0;
}

static int qtWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->page()->triggerAction(QWebEnginePage::Paste);
  return 0;
}

static int qtWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->page()->triggerAction(QWebEnginePage::SelectAll);
  return 0;
}

static int qtWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->page()->triggerAction(QWebEnginePage::Undo);
  return 0;
}

static int qtWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  webview->page()->triggerAction(QWebEnginePage::Redo);
  return 0;
}

/****************************************************************************
 * Rich Text Editor Commands
 ****************************************************************************/

static int qtWebBrowserExecCommandAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  qtWebBrowserRunJavaScript(ih,
    "if (window.iupRestoreSelection) window.iupRestoreSelection(); document.execCommand('%s', false, null);",
    value);
  return 0;
}

static int qtWebBrowserExecCommandWithParamAttrib(Ihandle* ih, const char* cmd, const char* param)
{
  if (!cmd)
    return 0;

  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  /* For large parameters (like data URLs), use QString directly to avoid buffer limits */
  char* escaped_param = qtWebBrowserEscapeJavaScript(param);
  QString js = QString(
    "if (window.iupRestoreSelection) window.iupRestoreSelection(); "
    "document.execCommand('%1', false, %2);")
    .arg(QString::fromUtf8(cmd))
    .arg(escaped_param ? QString::fromUtf8(escaped_param) : "null");

  if (escaped_param)
    free(escaped_param);

  webview->page()->runJavaScript(js);
  return 0;
}

static char* qtWebBrowserQueryCommandValue(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = qtWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandValue(%s);", escaped_cmd);
  free(escaped_cmd);
  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}

/****************************************************************************
 * Font Attributes
 ****************************************************************************/

static int qtWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  return qtWebBrowserExecCommandWithParamAttrib(ih, "fontName", value);
}

static char* qtWebBrowserGetFontNameAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  return window.getComputedStyle(node).fontFamily;"
    "})();";

  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static int qtWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  return qtWebBrowserExecCommandWithParamAttrib(ih, "fontSize", value);
}

static char* qtWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  var size = window.getComputedStyle(node).fontSize;"
    "  var px = parseFloat(size);"
    "  if (px <= 10) return '1';"
    "  else if (px <= 13) return '2';"
    "  else if (px <= 16) return '3';"
    "  else if (px <= 18) return '4';"
    "  else if (px <= 24) return '5';"
    "  else if (px <= 32) return '6';"
    "  else return '7';"
    "})();";

  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static int qtWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  return qtWebBrowserExecCommandWithParamAttrib(ih, "formatBlock", value);
}

static char* qtWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  while (node && node.nodeType !== 1) node = node.parentNode;"
    "  while (node && node.nodeName !== 'BODY' && node.nodeName !== 'HTML') {"
    "    var tag = node.nodeName.toUpperCase();"
    "    if (tag === 'H1' || tag === 'H2' || tag === 'H3' || tag === 'H4' || "
    "        tag === 'H5' || tag === 'H6' || tag === 'P' || tag === 'DIV' || "
    "        tag === 'PRE' || tag === 'BLOCKQUOTE') {"
    "      return tag;"
    "    }"
    "    node = node.parentNode;"
    "  }"
    "  return '';"
    "})();";

  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}


/****************************************************************************
 * Text Style Commands
 ****************************************************************************/

static int qtWebBrowserSetBoldAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "bold");
}

static int qtWebBrowserSetItalicAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "italic");
}

static int qtWebBrowserSetUnderlineAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "underline");
}

static int qtWebBrowserSetStrikethroughAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "strikeThrough");
}

/****************************************************************************
 * List Commands
 ****************************************************************************/

static int qtWebBrowserSetInsertOrderedListAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "insertOrderedList");
}

static int qtWebBrowserSetInsertUnorderedListAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "insertUnorderedList");
}

static int qtWebBrowserSetOutdentAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "outdent");
}

static int qtWebBrowserSetIndentAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qtWebBrowserExecCommandAttrib(ih, "indent");
}

/****************************************************************************
 * Color Commands
 ****************************************************************************/

static int qtWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    sprintf(rgb_color, "rgb(%d,%d,%d)", r, g, b);
    return qtWebBrowserExecCommandWithParamAttrib(ih, "foreColor", rgb_color);
  }
  return 0;
}

static int qtWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    sprintf(rgb_color, "rgb(%d,%d,%d)", r, g, b);
    return qtWebBrowserExecCommandWithParamAttrib(ih, "backColor", rgb_color);
  }
  return 0;
}

/****************************************************************************
 * Insert Commands
 ****************************************************************************/

static int qtWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  return qtWebBrowserExecCommandWithParamAttrib(ih, "insertHTML", value);
}

/* Forward declaration */
static int qtWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value);

static int qtWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  /* Read image file and convert to base64 data URL */
  QFile file(QString::fromUtf8(value));
  if (!file.open(QIODevice::ReadOnly))
    return 0;

  QByteArray data = file.readAll();
  file.close();

  QString base64 = data.toBase64();

  /* Determine MIME type from file extension */
  QString filename = QString::fromUtf8(value);
  QString mime = "image/png";
  if (filename.endsWith(".jpg", Qt::CaseInsensitive) || filename.endsWith(".jpeg", Qt::CaseInsensitive))
    mime = "image/jpeg";
  else if (filename.endsWith(".gif", Qt::CaseInsensitive))
    mime = "image/gif";
  else if (filename.endsWith(".svg", Qt::CaseInsensitive))
    mime = "image/svg+xml";
  else if (filename.endsWith(".webp", Qt::CaseInsensitive))
    mime = "image/webp";

  /* Create data URL and use INSERTIMAGE (like Windows does) */
  QString dataUrl = QString("data:%1;base64,%2").arg(mime).arg(base64);

  return qtWebBrowserSetInsertImageAttrib(ih, dataUrl.toUtf8().constData());
}

/****************************************************************************
 * Additional Editor Commands
 ****************************************************************************/

static int qtWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview)
    return 0;

  iupAttribSet(ih, "_IUPWEB_DIRTY", nullptr);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
  webview->setHtml("<html><body></body></html>");
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);

  /* No need to initialize document - script is injected automatically */

  return 0;
}

static int qtWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  return qtWebBrowserExecCommandWithParamAttrib(ih, "insertImage", value);
}

static int qtWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  return qtWebBrowserExecCommandWithParamAttrib(ih, "createLink", value);
}

static int qtWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  char* escaped = qtWebBrowserEscapeJavaScript(value);
  char js[1024];
  snprintf(js, sizeof(js),
    "if (window.iupRestoreSelection) window.iupRestoreSelection(); "
    "document.execCommand('insertText', false, %s);",
    escaped);
  qtWebBrowserRunJavaScript(ih, "%s", js);
  free(escaped);

  return 0;
}

static char* qtWebBrowserGetPasteAttrib(Ihandle* ih)
{
  /* Get clipboard content via JavaScript */
  return qtWebBrowserRunJavaScriptSync(ih,
    "(async function() {"
    "  try {"
    "    const text = await navigator.clipboard.readText();"
    "    return text;"
    "  } catch (err) {"
    "    return '';"
    "  }"
    "})();");
}

static char* qtWebBrowserGetForeColorAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  return window.getComputedStyle(node).color;"
    "})();";

  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* qtWebBrowserGetBackColorAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  return window.getComputedStyle(node).backgroundColor;"
    "})();";

  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}


/****************************************************************************
 * Command Query Attributes
 ****************************************************************************/

static char* qtWebBrowserQueryCommandState(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = qtWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandState(%s);", escaped_cmd);
  free(escaped_cmd);
  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* qtWebBrowserQueryCommandEnabled(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = qtWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandEnabled(%s);", escaped_cmd);
  free(escaped_cmd);
  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static const char* qtWebBrowserMapCommandName(const char* cmd)
{
  if (iupStrEqualNoCase(cmd, "FONTNAME"))
    return "fontName";
  else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
    return "fontSize";
  else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
    return "formatBlock";
  else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
    return "foreColor";
  else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
    return "backColor";

  return cmd;
}

static char* qtWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = qtWebBrowserMapCommandName(cmd);
    char* result = qtWebBrowserQueryCommandState(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnBoolean(0);
}

static char* qtWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = qtWebBrowserMapCommandName(cmd);
    char* result = qtWebBrowserQueryCommandEnabled(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnBoolean(0);
}

static char* qtWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = qtWebBrowserMapCommandName(cmd);
    char* result = qtWebBrowserQueryCommandValue(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnStr("");
}

static char* qtWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
  /* Command text is the same as command value for most commands */
  return qtWebBrowserGetCommandValueAttrib(ih);
}

/****************************************************************************
 * DOM Element Manipulation
 ****************************************************************************/

static char* qtWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attrib_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

  if (!element_id || !attrib_name)
    return nullptr;

  char* escaped_id = qtWebBrowserEscapeJavaScript(element_id);
  char* escaped_name = qtWebBrowserEscapeJavaScript(attrib_name);

  char js[512];
  snprintf(js, sizeof(js),
    "(function() {"
    "  var elem = document.getElementById(%s);"
    "  if (elem) return elem.getAttribute(%s) || '';"
    "  return '';"
    "})();",
    escaped_id, escaped_name);

  free(escaped_id);
  free(escaped_name);

  return qtWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static int qtWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attrib_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

  if (!element_id || !attrib_name)
    return 0;

  char* escaped_id = qtWebBrowserEscapeJavaScript(element_id);
  char* escaped_name = qtWebBrowserEscapeJavaScript(attrib_name);
  char* escaped_value = qtWebBrowserEscapeJavaScript(value);

  char js[1024];
  snprintf(js, sizeof(js),
    "(function() {"
    "  var elem = document.getElementById(%s);"
    "  if (elem) elem.setAttribute(%s, %s);"
    "})();",
    escaped_id, escaped_name, escaped_value ? escaped_value : "''");

  qtWebBrowserRunJavaScript(ih, "%s", js);

  free(escaped_id);
  free(escaped_name);
  if (escaped_value)
    free(escaped_value);

  return 0;
}

/****************************************************************************
 * Find Text
 ****************************************************************************/

static int qtWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
  QWebEngineView* webview = (QWebEngineView*)ih->handle;
  if (!webview || !value)
    return 0;

  webview->findText(QString::fromUtf8(value));
  return 0;
}

/****************************************************************************
 * Dirty Flag Support
 ****************************************************************************/

static char* qtWebBrowserGetDirtyAttrib(Ihandle* ih)
{
  /* Check if editing mode is enabled */
  if (!iupAttribGet(ih, "_IUPWEB_EDITABLE"))
    return iupStrReturnStr("NO");

  char* result = qtWebBrowserRunJavaScriptSync(ih, "window.iupGetDirtyFlag();");
  if (result && strcmp(result, "true") == 0)
    return iupStrReturnStr("YES");

  return iupStrReturnStr("NO");
}

/****************************************************************************
 * Custom QWebEngineView Class with Callbacks
 ****************************************************************************/

class IupQtWebBrowser : public QWebEngineView
{
public:
  Ihandle* ih;

  IupQtWebBrowser(Ihandle* ih_param) : QWebEngineView(), ih(ih_param)
  {
    /* Connect signals for callbacks */
    connect(this, &QWebEngineView::loadStarted, this, &IupQtWebBrowser::onLoadStarted);
    connect(this, &QWebEngineView::loadFinished, this, &IupQtWebBrowser::onLoadFinished);
    connect(this, &QWebEngineView::loadProgress, this, &IupQtWebBrowser::onLoadProgress);
    connect(this, &QWebEngineView::urlChanged, this, &IupQtWebBrowser::onUrlChanged);

    /* Connect page signals */
    connect(page(), &QWebEnginePage::windowCloseRequested, this, &IupQtWebBrowser::onWindowCloseRequested);
  }

private:
  void onLoadStarted()
  {
    if (!ih || iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
      return;

    /* Set loading flag for STATUS attribute */
    iupAttribSet(ih, "_IUPQT_WEB_LOADING", "YES");

    IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
    if (cb)
    {
      QUrl url = this->url();
      QString url_str = url.toString();

      int result = cb(ih, (char*)url_str.toUtf8().constData());
      if (result == IUP_IGNORE)
      {
        /* Stop navigation */
        this->stop();
      }
    }

    qtWebBrowserUpdateHistory(ih);
  }

  void onLoadFinished(bool ok)
  {
    if (!ih || iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
      return;

    /* Clear loading flag for STATUS attribute */
    iupAttribSet(ih, "_IUPQT_WEB_LOADING", nullptr);

    if (ok)
    {
      /* Always set contentEditable based on EDITABLE state (like Windows NavigationCompleted) */
      if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
        page()->runJavaScript("document.body.contentEditable = 'true';");
      else
        page()->runJavaScript("document.body.contentEditable = 'false';");

      IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
      if (cb)
      {
        QUrl url = this->url();
        QString url_str = url.toString();
        cb(ih, (char*)url_str.toUtf8().constData());
      }
    }
    else
    {
      IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
      if (cb)
      {
        QUrl url = this->url();
        QString url_str = url.toString();
        cb(ih, (char*)url_str.toUtf8().constData());
      }
    }

    qtWebBrowserUpdateHistory(ih);

    /* Call UPDATE_CB like Windows does */
    IFn update_cb = (IFn)IupGetCallback(ih, "UPDATE_CB");
    if (update_cb)
      update_cb(ih);
  }

  void onLoadProgress(int progress)
  {
    /* Can be used for progress reporting in future */
    (void)progress;
  }

  void onUrlChanged(const QUrl& url)
  {
    (void)url;
    qtWebBrowserUpdateHistory(ih);
  }

  void onWindowCloseRequested()
  {
    /* NEWWINDOW_CB equivalent - close request from JavaScript */
    IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
    if (cb)
    {
      QUrl url = this->url();
      QString url_str = url.toString();
      cb(ih, (char*)url_str.toUtf8().constData());
    }
  }
};

/****************************************************************************
 * Custom QWebEnginePage for new window handling
 ****************************************************************************/

class IupQtWebPage : public QWebEnginePage
{
public:
  Ihandle* ih;

  IupQtWebPage(QWebEngineProfile* profile, Ihandle* ih_param)
    : QWebEnginePage(profile), ih(ih_param)
  {
  }

protected:
  QWebEnginePage* createWindow(QWebEnginePage::WebWindowType type) override
  {
    (void)type;

    /* Call NEWWINDOW_CB callback */
    IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
    if (cb)
    {
      QUrl url = this->url();
      QString url_str = url.toString();

      int result = cb(ih, (char*)url_str.toUtf8().constData());
      if (result == IUP_IGNORE)
      {
        /* Don't open new window */
        return nullptr;
      }
    }

    /* Default: open in same window */
    return this;
  }
};

/****************************************************************************
 * Map and UnMap Methods
 ****************************************************************************/

static int qtWebBrowserMapMethod(Ihandle* ih)
{
  /* Create WebBrowser widget */
  IupQtWebBrowser* webview = new IupQtWebBrowser(ih);
  if (!webview)
    return IUP_ERROR;

  /* Create custom page for new window handling */
  IupQtWebPage* page = new IupQtWebPage(QWebEngineProfile::defaultProfile(), ih);
  webview->setPage(page);

  /* Configure settings */
  QWebEngineSettings* settings = page->settings();
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);

  /* Install initialization script that runs on every page load (like Windows AddScriptToExecuteOnDocumentCreated) */
  const char* init_script =
    "(function() {"
    "  var iupSavedRange = null;"
    "  var iupDirtyFlag = false;"
    "  document.addEventListener('selectionchange', function() {"
    "    var sel = window.getSelection();"
    "    if (sel.rangeCount > 0) {"
    "      iupSavedRange = sel.getRangeAt(0).cloneRange();"
    "    }"
    "  });"
    "  document.addEventListener('input', function(e) {"
    "    if (document.body.contentEditable == 'true') {"
    "      iupDirtyFlag = true;"
    "    }"
    "  });"
    "  window.iupRestoreSelection = function() {"
    "    if (document.body.contentEditable == 'true' && iupSavedRange) {"
    "      try {"
    "        var sel = window.getSelection();"
    "        sel.removeAllRanges();"
    "        sel.addRange(iupSavedRange);"
    "      } catch (e) {}"
    "    }"
    "  };"
    "  window.iupGetDirtyFlag = function() {"
    "    return iupDirtyFlag;"
    "  };"
    "  window.iupClearDirtyFlag = function() {"
    "    iupDirtyFlag = false;"
    "  };"
    "  document.body.style.overflow = 'auto';"
    "})();";

  QWebEngineScript script;
  script.setName("IupEditorInit");
  script.setSourceCode(QString::fromUtf8(init_script));
  script.setInjectionPoint(QWebEngineScript::DocumentReady);
  script.setRunsOnSubFrames(false);
  script.setWorldId(QWebEngineScript::MainWorld);
  page->scripts().insert(script);

  ih->handle = (InativeHandle*)webview;

  /* Add to parent container */
  iupqtAddToParent(ih);

  /* Set initial size policy */
  webview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  /* Load initial URL if specified */
  char* value = iupAttribGet(ih, "VALUE");
  if (value)
    qtWebBrowserSetValueAttrib(ih, value);

  return IUP_NOERROR;
}

static void qtWebBrowserUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    IupQtWebBrowser* webview = (IupQtWebBrowser*)ih->handle;

    /* Clear the ihandle pointer to prevent callbacks during destruction */
    webview->ih = nullptr;

    /* Delete the widget - Qt will automatically disconnect signals */
    delete webview;
    ih->handle = nullptr;
  }
}

/****************************************************************************
 * Create/Destroy Methods
 ****************************************************************************/

static int qtWebBrowserCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  /* Set default expand to fill available space like Canvas */
  ih->expand = IUP_EXPAND_BOTH;

  return IUP_NOERROR;
}

/****************************************************************************
 * Natural Size Calculation
 ****************************************************************************/

static void qtWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand;

  /* Get character size from font - WebBrowser has minimal natural size */
  /* This matches Windows and GTK implementations */
  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static void qtWebBrowserLayoutUpdateMethod(Ihandle* ih)
{
  /* Update base layout first */
  iupdrvBaseLayoutUpdateMethod(ih);

  /* Resize the QWebEngineView to fill the parent widget */
  if (ih->handle)
  {
    QWebEngineView* webview = (QWebEngineView*)ih->handle;

    /* Explicitly resize to match current size */
    webview->resize(ih->currentwidth, ih->currentheight);
  }
}

/****************************************************************************
 * Class Registration
 ****************************************************************************/

extern "C" Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(nullptr);

  ic->name = (char*)"webbrowser";
  ic->cons = (char*)"WebBrowser";
  ic->format = nullptr;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupWebBrowserNewClass;
  ic->Create = qtWebBrowserCreateMethod;

  ic->Map = qtWebBrowserMapMethod;
  ic->UnMap = qtWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = qtWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = qtWebBrowserLayoutUpdateMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");

  /* Common callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common attributes */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual attributes */
  iupBaseRegisterVisualAttrib(ic);

  /* Default EXPAND to YES - WebBrowser should fill available space like Canvas */
  iupClassRegisterReplaceAttribDef(ic, "EXPAND", IUPAF_SAMEASSYSTEM, "YES");

  /* WebBrowser specific attributes */

  /* Core navigation */
  iupClassRegisterAttribute(ic, "VALUE", qtWebBrowserGetValueAttrib, qtWebBrowserSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", nullptr, qtWebBrowserSetGoBackAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", nullptr, qtWebBrowserSetGoForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", nullptr, qtWebBrowserSetStopAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", nullptr, qtWebBrowserSetReloadAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", nullptr, qtWebBrowserSetBackForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* HTML content */
  iupClassRegisterAttribute(ic, "HTML", qtWebBrowserGetHTMLAttrib, qtWebBrowserSetHTMLAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", qtWebBrowserGetInnerTextAttrib, qtWebBrowserSetInnerTextAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* File operations */
  iupClassRegisterAttribute(ic, "OPENFILE", nullptr, qtWebBrowserSetOpenAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", nullptr, qtWebBrowserSetSaveAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Status and history */
  iupClassRegisterAttribute(ic, "STATUS", qtWebBrowserGetStatusAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", qtWebBrowserGetCanGoBackAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", qtWebBrowserGetCanGoForwardAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOUNT", qtWebBrowserGetBackCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", qtWebBrowserGetForwardCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", qtWebBrowserGetItemHistoryAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Zoom */
  iupClassRegisterAttribute(ic, "ZOOM", qtWebBrowserGetZoomAttrib, qtWebBrowserSetZoomAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Print */
  iupClassRegisterAttribute(ic, "PRINT", nullptr, qtWebBrowserSetPrintAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", nullptr, qtWebBrowserSetPrintPreviewAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Editing mode */
  iupClassRegisterAttribute(ic, "EDITABLE", qtWebBrowserGetEditableAttrib, qtWebBrowserSetEditableAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Clipboard operations */
  iupClassRegisterAttribute(ic, "COPY", nullptr, qtWebBrowserSetCopyAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", nullptr, qtWebBrowserSetCutAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", nullptr, qtWebBrowserSetPasteAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", nullptr, qtWebBrowserSetSelectAllAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", nullptr, qtWebBrowserSetUndoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", nullptr, qtWebBrowserSetRedoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Generic execCommand interface */
  iupClassRegisterAttribute(ic, "EXECCOMMAND", nullptr, qtWebBrowserExecCommandAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Font attributes */
  iupClassRegisterAttribute(ic, "FONTNAME", qtWebBrowserGetFontNameAttrib, qtWebBrowserSetFontNameAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", qtWebBrowserGetFontSizeAttrib, qtWebBrowserSetFontSizeAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", qtWebBrowserGetFormatBlockAttrib, qtWebBrowserSetFormatBlockAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Text style commands */
  iupClassRegisterAttribute(ic, "BOLD", nullptr, qtWebBrowserSetBoldAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ITALIC", nullptr, qtWebBrowserSetItalicAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDERLINE", nullptr, qtWebBrowserSetUnderlineAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STRIKETHROUGH", nullptr, qtWebBrowserSetStrikethroughAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* List commands */
  iupClassRegisterAttribute(ic, "INSERTORDEREDLIST", nullptr, qtWebBrowserSetInsertOrderedListAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTUNORDEREDLIST", nullptr, qtWebBrowserSetInsertUnorderedListAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENT", nullptr, qtWebBrowserSetIndentAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OUTDENT", nullptr, qtWebBrowserSetOutdentAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Color commands */
  iupClassRegisterAttribute(ic, "FORECOLOR", nullptr, qtWebBrowserSetForeColorAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", nullptr, qtWebBrowserSetBackColorAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Insert commands */
  iupClassRegisterAttribute(ic, "INSERTHTML", nullptr, qtWebBrowserSetInsertHtmlAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", nullptr, qtWebBrowserSetInsertImageFileAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Dirty flag */
  iupClassRegisterAttribute(ic, "DIRTY", qtWebBrowserGetDirtyAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* New document */
  iupClassRegisterAttribute(ic, "NEW", nullptr, qtWebBrowserSetNewAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Paste getter */
  iupClassRegisterAttribute(ic, "PASTE", qtWebBrowserGetPasteAttrib, qtWebBrowserSetPasteAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Insert commands */
  iupClassRegisterAttribute(ic, "INSERTIMAGE", nullptr, qtWebBrowserSetInsertImageAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", nullptr, qtWebBrowserSetCreateLinkAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", nullptr, qtWebBrowserSetInsertTextAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Color getters/setters */
  iupClassRegisterAttribute(ic, "FORECOLOR", qtWebBrowserGetForeColorAttrib, qtWebBrowserSetForeColorAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", qtWebBrowserGetBackColorAttrib, qtWebBrowserSetBackColorAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Command query attributes */
  iupClassRegisterAttribute(ic, "COMMAND", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", qtWebBrowserGetCommandStateAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", qtWebBrowserGetCommandEnabledAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", qtWebBrowserGetCommandTextAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", qtWebBrowserGetCommandValueAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* DOM manipulation attributes */
  iupClassRegisterAttribute(ic, "ELEMENT_ID", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", qtWebBrowserGetAttributeAttrib, qtWebBrowserSetAttributeAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Find text */
  iupClassRegisterAttribute(ic, "FIND", nullptr, qtWebBrowserSetFindAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}

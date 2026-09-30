/** \file
 * \brief WebBrowser Control - Qt Quick Implementation using QtWebEngine Quick
 *
 * See Copyright Notice in "iup.h"
 */

#include <QtGlobal>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include <QWebEngineScript>
#include <QQuickItem>
#include <QUrl>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QMetaObject>
#include <QHash>
#include <QSet>
#include <QVariant>

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
}

#include "iupqml_drv.h"

/* same deferred pre-application setup the QtWebEngineWidgets module does for itself */
static void qmlWebBrowserInitialize()
{
  QtWebEngineQuick::initialize();
}
Q_CONSTRUCTOR_FUNCTION(qmlWebBrowserInitialize)

struct IupQmlWebData
{
  QQuickItem* view = nullptr;
  int next_id = 0;
  QSet<int> waiting;
  QHash<int, QVariant> results;
};

static const char* qmlWebBrowserQml =
  "import QtQuick\n"
  "import QtWebEngine\n"
  "WebEngineView { id: web\n"
  "  property bool iupOpenNewWindow: true\n"
  "  signal iupLoading(int status, string url)\n"
  "  signal iupNewWindow(string url)\n"
  "  signal iupCloseRequested()\n"
  "  signal iupResult(int id, var result)\n"
  "  settings.javascriptEnabled: true\n"
  "  settings.localStorageEnabled: true\n"
  "  settings.pluginsEnabled: true\n"
  "  function iupRun(id, js) { runJavaScript(js, function(result) { web.iupResult(id, result) }) }\n"
  "  function iupGo(offset) { goBackOrForward(offset) }\n"
  "  function iupFind(text) { findText(text) }\n"
  "  function iupLoadHtml(html) { loadHtml(html) }\n"
  "  function iupAction(name) {\n"
  "    var actions = { Copy: WebEngineView.Copy, Cut: WebEngineView.Cut, Paste: WebEngineView.Paste, Undo: WebEngineView.Undo, Redo: WebEngineView.Redo, SelectAll: WebEngineView.SelectAll }\n"
  "    triggerWebAction(actions[name])\n"
  "  }\n"
  "  onLoadingChanged: (info) => web.iupLoading(info.status === WebEngineView.LoadStartedStatus ? 0 : info.status === WebEngineView.LoadSucceededStatus ? 1 : info.status === WebEngineView.LoadFailedStatus ? 2 : 3, info.url.toString())\n"
  "  onNewWindowRequested: (request) => { web.iupOpenNewWindow = true; web.iupNewWindow(request.requestedUrl.toString()); if (web.iupOpenNewWindow) request.openIn(web) }\n"
  "  onWindowCloseRequested: web.iupCloseRequested()\n"
  "}";

static const char* qmlWebBrowserInitScript =
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

static IupQmlWebData* qmlWebBrowserGetData(Ihandle* ih)
{
  return reinterpret_cast<IupQmlWebData*>(iupAttribGet(ih, "_IUPQML_WEB"));
}

static QWebEngineHistory* qmlWebBrowserHistory(Ihandle* ih)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return nullptr;
  return qobject_cast<QWebEngineHistory*>(qvariant_cast<QObject*>(data->view->property("history")));
}

/****************************************************************************
 * JavaScript Helper Functions
 ****************************************************************************/

static char* qmlWebBrowserEscapeJavaScript(const char* str)
{
  if (!str)
    return iupStrDup("null");

  size_t len = strlen(str);
  size_t escaped_len = len * 6 + 3;
  char* result = static_cast<char*>(malloc(escaped_len));
  if (!result)
    return iupStrDup("null");

  char* p = result;
  *p++ = '"';

  for (const char* s = str; *s; s++)
  {
    if (static_cast<unsigned char>(*s) == 0xE2 && static_cast<unsigned char>(*(s+1)) == 0x80 &&
        (static_cast<unsigned char>(*(s+2)) == 0xA8 || static_cast<unsigned char>(*(s+2)) == 0xA9))
    {
      snprintf(p, static_cast<size_t>((result + escaped_len) - p), "\\u202%c", static_cast<unsigned char>(*(s+2)) == 0xA8 ? '8' : '9');
      p += 6;
      s += 2;
      continue;
    }

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
        if (static_cast<unsigned char>(*s) < 32)
        {
          snprintf(p, static_cast<size_t>((result + escaped_len) - p), "\\u%04x", static_cast<unsigned char>(*s));
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

static void qmlWebBrowserRunJavaScript(Ihandle* ih, const char* format, ...)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return;

  char js[4096];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(js, sizeof(js), format, arglist);
  va_end(arglist);

  iupqmlCallMethod(data->view, "iupRun", -1, QString::fromUtf8(js));
}

static char* qmlWebBrowserExecJavaScriptSync(Ihandle* ih, const char* js)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return nullptr;

  int id = data->next_id++;
  data->waiting.insert(id);
  iupqmlCallMethod(data->view, "iupRun", id, QString::fromUtf8(js));

  QElapsedTimer timeout_timer;
  timeout_timer.start();

  while (!data->results.contains(id) && timeout_timer.elapsed() < 5000)
  {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    data = qmlWebBrowserGetData(ih);
    if (!data)
      return nullptr;
  }

  data->waiting.remove(id);
  if (!data->results.contains(id))
    return nullptr;

  QString result = data->results.take(id).toString();
  if (result.isEmpty())
    return nullptr;
  return iupStrReturnStr(result.toUtf8().constData());
}

static char* qmlWebBrowserRunJavaScriptSync(Ihandle* ih, const char* format, ...)
{
  char js[4096];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(js, sizeof(js), format, arglist);
  va_end(arglist);

  return qmlWebBrowserExecJavaScriptSync(ih, js);
}

/****************************************************************************
 * History Management
 ****************************************************************************/

static void qmlWebBrowserUpdateHistory(Ihandle* ih)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  if (!history)
    return;

  iupAttribSet(ih, "CANGOBACK", history->canGoBack() ? "YES" : "NO");
  iupAttribSet(ih, "CANGOFORWARD", history->canGoForward() ? "YES" : "NO");
}

/****************************************************************************
 * Attribute Getters
 ****************************************************************************/

static char* qmlWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  if (!history)
    return nullptr;

  QWebEngineHistoryItem item = history->itemAt(history->currentItemIndex() + id);
  if (!item.isValid())
    return nullptr;

  return iupStrReturnStr(item.url().toString().toUtf8().constData());
}

static char* qmlWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  if (!history)
    return iupStrReturnInt(0);

  return iupStrReturnInt(history->forwardItems(history->count()).count());
}

static char* qmlWebBrowserGetBackCountAttrib(Ihandle* ih)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  if (!history)
    return iupStrReturnInt(0);

  return iupStrReturnInt(history->backItems(history->count()).count());
}

static char* qmlWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  return iupStrReturnBoolean(history && history->canGoBack());
}

static char* qmlWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  return iupStrReturnBoolean(history && history->canGoForward());
}

/****************************************************************************
 * Core Attribute Setters
 ****************************************************************************/

static void qmlWebBrowserLoadHtml(Ihandle* ih, const QString& html)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return;

  iupAttribSet(ih, "_IUPWEB_DIRTY", nullptr);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
  iupqmlCallMethod(data->view, "iupLoadHtml", html);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);
}

static int qmlWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  qmlWebBrowserLoadHtml(ih, QString::fromUtf8(value));
  return 0;
}

static char* qmlWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  return qmlWebBrowserRunJavaScriptSync(ih, "new XMLSerializer().serializeToString(document);");
}

static int qmlWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data || !value)
    return 0;

  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
  data->view->setProperty("url", QUrl::fromUserInput(QString::fromUtf8(value)));
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);

  return 0;
}

static char* qmlWebBrowserGetValueAttrib(Ihandle* ih)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return nullptr;

  QUrl url = data->view->property("url").toUrl();
  if (url.isEmpty())
    return nullptr;

  return iupStrReturnStr(url.toString().toUtf8().constData());
}

static char* qmlWebBrowserGetStatusAttrib(Ihandle* ih)
{
  char* status = iupAttribGet(ih, "_IUPQML_WEB_STATUS");
  return status ? status : const_cast<char*>("COMPLETED");
}

/****************************************************************************
 * Navigation Attribute Setters
 ****************************************************************************/

static int qmlWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  if (!history)
    return 0;

  int count = 1;
  if (value && *value)
    iupStrToInt(value, &count);

  if (history->canGoBack())
  {
    if (count == 1)
      history->back();
    else
      history->goToItem(history->itemAt(history->currentItemIndex() - count));
  }

  return 0;
}

static int qmlWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  if (!history)
    return 0;

  int count = 1;
  if (value && *value)
    iupStrToInt(value, &count);

  if (history->canGoForward())
  {
    if (count == 1)
      history->forward();
    else
      history->goToItem(history->itemAt(history->currentItemIndex() + count));
  }

  return 0;
}

static int qmlWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (data)
    iupqmlCallMethod(data->view, "stop");
  return 0;
}

static int qmlWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (data)
    iupqmlCallMethod(data->view, "reload");
  return 0;
}

static int qmlWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  QWebEngineHistory* history = qmlWebBrowserHistory(ih);
  if (!history || !value)
    return 0;

  int count = 0;
  if (!iupStrToInt(value, &count))
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

static int qmlWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  char* html = qmlWebBrowserGetHTMLAttrib(ih);
  if (!html)
    return 0;

  return write_file(value, html, static_cast<int>(strlen(html)));
}

static int qmlWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  QFile file(QString::fromUtf8(value));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return 0;

  QTextStream in(&file);
  QString html = in.readAll();
  file.close();

  qmlWebBrowserLoadHtml(ih, html);
  return 1;
}

/****************************************************************************
 * Text Content Attributes
 ****************************************************************************/

static char* qmlWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  if (!element_id)
    return nullptr;

  char* escaped_id = qmlWebBrowserEscapeJavaScript(element_id);

  QString js = QString(
    "(function() {"
    "  var elem = document.getElementById(%1);"
    "  return elem ? elem.innerText : '';"
    "})();").arg(QString::fromUtf8(escaped_id));

  free(escaped_id);

  return qmlWebBrowserExecJavaScriptSync(ih, js.toUtf8().constData());
}

static int qmlWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");

  if (!data || !element_id)
    return 0;

  char* escaped_id = qmlWebBrowserEscapeJavaScript(element_id);
  char* escaped_value = qmlWebBrowserEscapeJavaScript(value ? value : "");

  QString js = QString(
    "(function() {"
    "  var elem = document.getElementById(%1);"
    "  if (elem) elem.innerText = %2;"
    "})();").arg(QString::fromUtf8(escaped_id), QString::fromUtf8(escaped_value));

  iupqmlCallMethod(data->view, "iupRun", -1, js);

  free(escaped_id);
  free(escaped_value);

  return 0;
}

static char* qmlWebBrowserGetJavascriptAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPWEB_JS_RESULT");
}

static int qmlWebBrowserSetJavascriptAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPWEB_JS_RESULT", nullptr);
  if (!value)
    return 0;

  char* result = qmlWebBrowserExecJavaScriptSync(ih, value);
  if (result)
    iupAttribSetStr(ih, "_IUPWEB_JS_RESULT", result);
  return 0;
}

/****************************************************************************
 * Zoom Attributes
 ****************************************************************************/

static int qmlWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data || !value)
    return 0;

  int zoom_percent = 100;
  if (!iupStrToInt(value, &zoom_percent))
    return 0;

  data->view->setProperty("zoomFactor", static_cast<double>(zoom_percent) / 100.0);
  return 0;
}

static char* qmlWebBrowserGetZoomAttrib(Ihandle* ih)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return nullptr;

  return iupStrReturnInt(static_cast<int>(data->view->property("zoomFactor").toDouble() * 100.0));
}

/****************************************************************************
 * Editing Mode Attributes
 ****************************************************************************/

static int qmlWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    iupAttribSet(ih, "_IUPWEB_EDITABLE", "1");
    qmlWebBrowserRunJavaScript(ih, "document.body.contentEditable = 'true';");
  }
  else
  {
    iupAttribSet(ih, "_IUPWEB_EDITABLE", nullptr);
    qmlWebBrowserRunJavaScript(ih, "document.body.contentEditable = 'false';");
  }

  return 0;
}

static char* qmlWebBrowserGetEditableAttrib(Ihandle* ih)
{
  char* result = qmlWebBrowserRunJavaScriptSync(ih, "document.body.contentEditable == 'true';");
  return iupStrReturnBoolean(result && strcmp(result, "true") == 0);
}

/****************************************************************************
 * Clipboard Operations
 ****************************************************************************/

static int qmlWebBrowserTriggerAction(Ihandle* ih, const char* name)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (data)
    iupqmlCallMethod(data->view, "iupAction", QString::fromUtf8(name));
  return 0;
}

static int qmlWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserTriggerAction(ih, "Copy");
}

static int qmlWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserTriggerAction(ih, "Cut");
}

static int qmlWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserTriggerAction(ih, "Paste");
}

static int qmlWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserTriggerAction(ih, "SelectAll");
}

static int qmlWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserTriggerAction(ih, "Undo");
}

static int qmlWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserTriggerAction(ih, "Redo");
}

/****************************************************************************
 * Rich Text Editor Commands
 ****************************************************************************/

static int qmlWebBrowserExecCommandAttrib(Ihandle* ih, const char* value)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data || !value)
    return 0;

  char* escaped = qmlWebBrowserEscapeJavaScript(value);

  QString js = QString(
    "if (window.iupRestoreSelection) window.iupRestoreSelection(); "
    "document.execCommand(%1, false, null);")
    .arg(QString::fromUtf8(escaped));

  free(escaped);

  iupqmlCallMethod(data->view, "iupRun", -1, js);
  return 0;
}

static int qmlWebBrowserExecCommandWithParamAttrib(Ihandle* ih, const char* cmd, const char* param)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return 0;

  char* escaped_param = qmlWebBrowserEscapeJavaScript(param);
  QString js = QString(
    "if (window.iupRestoreSelection) window.iupRestoreSelection(); "
    "document.execCommand('%1', false, %2);")
    .arg(QString::fromUtf8(cmd))
    .arg(QString::fromUtf8(escaped_param));

  free(escaped_param);

  iupqmlCallMethod(data->view, "iupRun", -1, js);
  return 0;
}

static char* qmlWebBrowserQueryCommandValue(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = qmlWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandValue(%s);", escaped_cmd);
  free(escaped_cmd);
  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

/****************************************************************************
 * Font Attributes
 ****************************************************************************/

static int qmlWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  return qmlWebBrowserExecCommandWithParamAttrib(ih, "fontName", value);
}

static char* qmlWebBrowserGetFontNameAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  return window.getComputedStyle(node).fontFamily;"
    "})();";

  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static int qmlWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  return qmlWebBrowserExecCommandWithParamAttrib(ih, "fontSize", value);
}

static char* qmlWebBrowserGetFontSizeAttrib(Ihandle* ih)
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

  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static int qmlWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  return qmlWebBrowserExecCommandWithParamAttrib(ih, "formatBlock", value);
}

static char* qmlWebBrowserGetFormatBlockAttrib(Ihandle* ih)
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

  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

/****************************************************************************
 * Text Style Commands
 ****************************************************************************/

static int qmlWebBrowserSetBoldAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "bold");
}

static int qmlWebBrowserSetItalicAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "italic");
}

static int qmlWebBrowserSetUnderlineAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "underline");
}

static int qmlWebBrowserSetStrikethroughAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "strikeThrough");
}

/****************************************************************************
 * List Commands
 ****************************************************************************/

static int qmlWebBrowserSetInsertOrderedListAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "insertOrderedList");
}

static int qmlWebBrowserSetInsertUnorderedListAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "insertUnorderedList");
}

static int qmlWebBrowserSetOutdentAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "outdent");
}

static int qmlWebBrowserSetIndentAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return qmlWebBrowserExecCommandAttrib(ih, "indent");
}

/****************************************************************************
 * Color Commands
 ****************************************************************************/

static int qmlWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    snprintf(rgb_color, sizeof(rgb_color), "rgb(%d,%d,%d)", r, g, b);
    return qmlWebBrowserExecCommandWithParamAttrib(ih, "foreColor", rgb_color);
  }
  return 0;
}

static int qmlWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    snprintf(rgb_color, sizeof(rgb_color), "rgb(%d,%d,%d)", r, g, b);
    return qmlWebBrowserExecCommandWithParamAttrib(ih, "backColor", rgb_color);
  }
  return 0;
}

/****************************************************************************
 * Insert Commands
 ****************************************************************************/

static int qmlWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  return qmlWebBrowserExecCommandWithParamAttrib(ih, "insertHTML", value);
}

static int qmlWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  return qmlWebBrowserExecCommandWithParamAttrib(ih, "insertImage", value);
}

static int qmlWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  QFile file(QString::fromUtf8(value));
  if (!file.open(QIODevice::ReadOnly))
    return 0;

  QByteArray data = file.readAll();
  file.close();

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

  QString dataUrl = QString("data:%1;base64,%2").arg(mime).arg(QString::fromLatin1(data.toBase64()));

  return qmlWebBrowserSetInsertImageAttrib(ih, dataUrl.toUtf8().constData());
}

/****************************************************************************
 * Additional Editor Commands
 ****************************************************************************/

static int qmlWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  qmlWebBrowserLoadHtml(ih, "<html><body></body></html>");
  return 0;
}

static int qmlWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  return qmlWebBrowserExecCommandWithParamAttrib(ih, "createLink", value);
}

static int qmlWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  return qmlWebBrowserExecCommandWithParamAttrib(ih, "insertText", value);
}

static char* qmlWebBrowserGetPasteAttrib(Ihandle* ih)
{
  return qmlWebBrowserRunJavaScriptSync(ih,
    "(async function() {"
    "  try {"
    "    const text = await navigator.clipboard.readText();"
    "    return text;"
    "  } catch (err) {"
    "    return '';"
    "  }"
    "})();");
}

static char* qmlWebBrowserGetForeColorAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  return window.getComputedStyle(node).color;"
    "})();";

  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* qmlWebBrowserGetBackColorAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  return window.getComputedStyle(node).backgroundColor;"
    "})();";

  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

/****************************************************************************
 * Command Query Attributes
 ****************************************************************************/

static char* qmlWebBrowserQueryCommandState(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = qmlWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandState(%s);", escaped_cmd);
  free(escaped_cmd);
  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* qmlWebBrowserQueryCommandEnabled(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = qmlWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandEnabled(%s);", escaped_cmd);
  free(escaped_cmd);
  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static const char* qmlWebBrowserMapCommandName(const char* cmd)
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

static char* qmlWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    char* result = qmlWebBrowserQueryCommandState(ih, qmlWebBrowserMapCommandName(cmd));
    if (result)
      return result;
  }
  return iupStrReturnBoolean(0);
}

static char* qmlWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    char* result = qmlWebBrowserQueryCommandEnabled(ih, qmlWebBrowserMapCommandName(cmd));
    if (result)
      return result;
  }
  return iupStrReturnBoolean(0);
}

static char* qmlWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    char* result = qmlWebBrowserQueryCommandValue(ih, qmlWebBrowserMapCommandName(cmd));
    if (result)
      return result;
  }
  return iupStrReturnStr("");
}

static char* qmlWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
  return qmlWebBrowserGetCommandValueAttrib(ih);
}

/****************************************************************************
 * DOM Element Manipulation
 ****************************************************************************/

static char* qmlWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attrib_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

  if (!element_id || !attrib_name)
    return nullptr;

  char* escaped_id = qmlWebBrowserEscapeJavaScript(element_id);
  char* escaped_name = qmlWebBrowserEscapeJavaScript(attrib_name);

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

  return qmlWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static int qmlWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attrib_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

  if (!element_id || !attrib_name)
    return 0;

  char* escaped_id = qmlWebBrowserEscapeJavaScript(element_id);
  char* escaped_name = qmlWebBrowserEscapeJavaScript(attrib_name);
  char* escaped_value = qmlWebBrowserEscapeJavaScript(value ? value : "");

  char js[1024];
  snprintf(js, sizeof(js),
    "(function() {"
    "  var elem = document.getElementById(%s);"
    "  if (elem) elem.setAttribute(%s, %s);"
    "})();",
    escaped_id, escaped_name, escaped_value);

  qmlWebBrowserRunJavaScript(ih, "%s", js);

  free(escaped_id);
  free(escaped_name);
  free(escaped_value);

  return 0;
}

/****************************************************************************
 * Find Text
 ****************************************************************************/

static int qmlWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data || !value)
    return 0;

  iupqmlCallMethod(data->view, "iupFind", QString::fromUtf8(value));
  return 0;
}

/****************************************************************************
 * Dirty Flag Support
 ****************************************************************************/

static char* qmlWebBrowserGetDirtyAttrib(Ihandle* ih)
{
  if (!iupAttribGet(ih, "_IUPWEB_EDITABLE"))
    return const_cast<char*>("NO");

  char* result = qmlWebBrowserRunJavaScriptSync(ih, "window.iupGetDirtyFlag();");
  return iupStrReturnBoolean(result && strcmp(result, "true") == 0);
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qmlWebBrowserLoading(Ihandle* ih, int status, const QString& url)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data || iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
    return;

  QByteArray url_str = url.toUtf8();

  if (status == 0)
  {
    iupAttribSet(ih, "_IUPQML_WEB_STATUS", "LOADING");

    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "NAVIGATE_CB"));
    if (cb && cb(ih, const_cast<char*>(url_str.constData())) == IUP_IGNORE)
      iupqmlCallMethod(data->view, "stop");

    qmlWebBrowserUpdateHistory(ih);
    return;
  }

  iupAttribSet(ih, "_IUPQML_WEB_STATUS", status == 2 ? "FAILED" : "COMPLETED");
  if (status == 3)
    return;

  if (status == 1)
  {
    qmlWebBrowserRunJavaScript(ih, "document.body.contentEditable = '%s';", iupAttribGet(ih, "_IUPWEB_EDITABLE") ? "true" : "false");

    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "COMPLETED_CB"));
    if (cb)
      cb(ih, const_cast<char*>(url_str.constData()));
  }
  else
  {
    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "ERROR_CB"));
    if (cb)
      cb(ih, const_cast<char*>(url_str.constData()));
  }

  qmlWebBrowserUpdateHistory(ih);

  IFn update_cb = static_cast<IFn>(IupGetCallback(ih, "UPDATE_CB"));
  if (update_cb)
    update_cb(ih);
}

static void qmlWebBrowserNewWindow(Ihandle* ih, const QString& url)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  if (!data)
    return;

  IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "NEWWINDOW_CB"));
  if (cb && cb(ih, const_cast<char*>(url.toUtf8().constData())) == IUP_IGNORE)
    data->view->setProperty("iupOpenNewWindow", false);
}

/****************************************************************************
 * Map and UnMap Methods
 ****************************************************************************/

static int qmlWebBrowserMapMethod(Ihandle* ih)
{
  QQuickItem* view = iupqmlCreateItem(qmlWebBrowserQml);
  if (!view)
    return IUP_ERROR;

  auto* data = new IupQmlWebData();
  data->view = view;
  data->next_id = 0;
  iupAttribSet(ih, "_IUPQML_WEB", reinterpret_cast<char*>(data));
  ih->handle = reinterpret_cast<InativeHandle*>(view);

  auto* scripts = qvariant_cast<QObject*>(view->property("userScripts"));
  if (scripts)
  {
    QWebEngineScript script;
    script.setName("IupEditorInit");
    script.setSourceCode(QString::fromUtf8(qmlWebBrowserInitScript));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setRunsOnSubFrames(false);
    script.setWorldId(QWebEngineScript::MainWorld);
    QMetaObject::invokeMethod(scripts, "insert", Q_ARG(QWebEngineScript, script));
  }

  iupqmlConnect(view, "iupLoading(int,QString)", [ih](void** args) {
    qmlWebBrowserLoading(ih, *static_cast<int*>(args[1]), *static_cast<QString*>(args[2]));
  });

  iupqmlConnect(view, "iupNewWindow(QString)", [ih](void** args) {
    qmlWebBrowserNewWindow(ih, *static_cast<QString*>(args[1]));
  });

  iupqmlConnect(view, "iupCloseRequested()", [ih](void**) {
    IupQmlWebData* d = qmlWebBrowserGetData(ih);
    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "NEWWINDOW_CB"));
    if (d && cb)
      cb(ih, const_cast<char*>(d->view->property("url").toUrl().toString().toUtf8().constData()));
  });

  iupqmlConnect(view, "iupResult(int,QVariant)", [ih](void** args) {
    IupQmlWebData* d = qmlWebBrowserGetData(ih);
    int id = *static_cast<int*>(args[1]);
    if (d && d->waiting.contains(id))
      d->results.insert(id, *static_cast<QVariant*>(args[2]));
  });

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, view);

  char* value = iupAttribGet(ih, "VALUE");
  if (value)
    qmlWebBrowserSetValueAttrib(ih, value);

  return IUP_NOERROR;
}

static void qmlWebBrowserUnMapMethod(Ihandle* ih)
{
  IupQmlWebData* data = qmlWebBrowserGetData(ih);
  iupAttribSet(ih, "_IUPQML_WEB", nullptr);
  delete data;

  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Create/Destroy Methods
 ****************************************************************************/

static int qmlWebBrowserCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  ih->expand = IUP_EXPAND_BOTH;

  return IUP_NOERROR;
}

/****************************************************************************
 * Natural Size Calculation
 ****************************************************************************/

static void qmlWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  (void)children_expand;
  iupdrvFontGetCharSize(ih, w, h);
}

/****************************************************************************
 * Class Registration
 ****************************************************************************/

extern "C" Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(nullptr);

  ic->name = const_cast<char*>("webbrowser");
  ic->cons = const_cast<char*>("WebBrowser");
  ic->format = nullptr;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupWebBrowserNewClass;
  ic->Create = qmlWebBrowserCreateMethod;

  ic->Map = qmlWebBrowserMapMethod;
  ic->UnMap = qmlWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = qmlWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  iupBaseRegisterCommonCallbacks(ic);

  iupBaseRegisterCommonAttrib(ic);

  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterReplaceAttribDef(ic, "EXPAND", IUPAF_SAMEASSYSTEM, "YES");

  iupClassRegisterAttribute(ic, "VALUE", qmlWebBrowserGetValueAttrib, qmlWebBrowserSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", nullptr, qmlWebBrowserSetGoBackAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", nullptr, qmlWebBrowserSetGoForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", nullptr, qmlWebBrowserSetStopAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", nullptr, qmlWebBrowserSetReloadAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", nullptr, qmlWebBrowserSetBackForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HTML", qmlWebBrowserGetHTMLAttrib, qmlWebBrowserSetHTMLAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", qmlWebBrowserGetInnerTextAttrib, qmlWebBrowserSetInnerTextAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "OPENFILE", nullptr, qmlWebBrowserSetOpenAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", nullptr, qmlWebBrowserSetSaveAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "STATUS", qmlWebBrowserGetStatusAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", qmlWebBrowserGetCanGoBackAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", qmlWebBrowserGetCanGoForwardAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOUNT", qmlWebBrowserGetBackCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", qmlWebBrowserGetForwardCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", qmlWebBrowserGetItemHistoryAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ZOOM", qmlWebBrowserGetZoomAttrib, qmlWebBrowserSetZoomAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PRINT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", qmlWebBrowserGetEditableAttrib, qmlWebBrowserSetEditableAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COPY", nullptr, qmlWebBrowserSetCopyAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", nullptr, qmlWebBrowserSetCutAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", qmlWebBrowserGetPasteAttrib, qmlWebBrowserSetPasteAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", nullptr, qmlWebBrowserSetSelectAllAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", nullptr, qmlWebBrowserSetUndoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", nullptr, qmlWebBrowserSetRedoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EXECCOMMAND", nullptr, qmlWebBrowserExecCommandAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME", qmlWebBrowserGetFontNameAttrib, qmlWebBrowserSetFontNameAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", qmlWebBrowserGetFontSizeAttrib, qmlWebBrowserSetFontSizeAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", qmlWebBrowserGetFormatBlockAttrib, qmlWebBrowserSetFormatBlockAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BOLD", nullptr, qmlWebBrowserSetBoldAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ITALIC", nullptr, qmlWebBrowserSetItalicAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDERLINE", nullptr, qmlWebBrowserSetUnderlineAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STRIKETHROUGH", nullptr, qmlWebBrowserSetStrikethroughAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "INSERTORDEREDLIST", nullptr, qmlWebBrowserSetInsertOrderedListAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTUNORDEREDLIST", nullptr, qmlWebBrowserSetInsertUnorderedListAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENT", nullptr, qmlWebBrowserSetIndentAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OUTDENT", nullptr, qmlWebBrowserSetOutdentAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FORECOLOR", qmlWebBrowserGetForeColorAttrib, qmlWebBrowserSetForeColorAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", qmlWebBrowserGetBackColorAttrib, qmlWebBrowserSetBackColorAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "INSERTHTML", nullptr, qmlWebBrowserSetInsertHtmlAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", nullptr, qmlWebBrowserSetInsertImageAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", nullptr, qmlWebBrowserSetInsertImageFileAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", nullptr, qmlWebBrowserSetCreateLinkAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", nullptr, qmlWebBrowserSetInsertTextAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", qmlWebBrowserGetDirtyAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", nullptr, qmlWebBrowserSetNewAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", qmlWebBrowserGetCommandStateAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", qmlWebBrowserGetCommandEnabledAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", qmlWebBrowserGetCommandTextAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", qmlWebBrowserGetCommandValueAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", qmlWebBrowserGetAttributeAttrib, qmlWebBrowserSetAttributeAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "JAVASCRIPT", qmlWebBrowserGetJavascriptAttrib, qmlWebBrowserSetJavascriptAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FIND", nullptr, qmlWebBrowserSetFindAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}

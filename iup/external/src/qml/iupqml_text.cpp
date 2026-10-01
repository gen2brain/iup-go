/** \file
 * \brief Text Control - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickTextDocument>
#include <QTextDocument>
#include <QTextList>
#include <QTextCharFormat>
#include <QTextOption>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QString>
#include <QUrl>
#include <QColor>
#include <QPixmap>
#include <QGuiApplication>
#include <QClipboard>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_mask.h"
#include "iup_text.h"
}

#include "iupqml_drv.h"


/****************************************************************************
 * Helpers
 ****************************************************************************/

static QQuickItem* qmlTextGetEdit(Ihandle* ih)
{
  return reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_TEXT_EDIT"));
}

static QObject* qmlTextGetSpin(Ihandle* ih)
{
  return reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_SPINBOX"));
}

static QTextDocument* qmlTextGetDocument(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit || !ih->data->is_multiline)
    return nullptr;

  auto* qdoc = qvariant_cast<QQuickTextDocument*>(edit->property("textDocument"));
  return qdoc ? qdoc->textDocument() : nullptr;
}

static QString qmlTextGetText(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit)
    return {};
  if (ih->data->is_multiline)
  {
    QTextDocument* doc = qmlTextGetDocument(ih);
    if (doc)
      return doc->toPlainText();
  }
  return edit->property("text").toString();
}

static void qmlTextSetText(Ihandle* ih, const QString& text)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit)
    return;

  ih->data->disable_callbacks = 1;
  edit->setProperty("text", text);
  ih->data->disable_callbacks = 0;
}

static int qmlTextGetCursor(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  return edit ? edit->property("cursorPosition").toInt() : 0;
}

static void qmlTextSetCursor(Ihandle* ih, int pos)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    edit->setProperty("cursorPosition", pos);
}

static void qmlTextGetSelectionRange(Ihandle* ih, int* start, int* end)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit)
  {
    *start = *end = 0;
    return;
  }
  *start = edit->property("selectionStart").toInt();
  *end = edit->property("selectionEnd").toInt();
}

static void qmlTextSelect(Ihandle* ih, int start, int end)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    QMetaObject::invokeMethod(edit, "select", Qt::DirectConnection, Q_ARG(int, start), Q_ARG(int, end));
}

static void qmlTextInsertAt(Ihandle* ih, int pos, const QString& text)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    QMetaObject::invokeMethod(edit, "insert", Qt::DirectConnection, Q_ARG(int, pos), Q_ARG(QString, text));
}

static void qmlTextRemove(Ihandle* ih, int start, int end)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    QMetaObject::invokeMethod(edit, "remove", Qt::DirectConnection, Q_ARG(int, start), Q_ARG(int, end));
}

static void qmlTextReplaceSelection(Ihandle* ih, const QString& text)
{
  int start, end;
  qmlTextGetSelectionRange(ih, &start, &end);
  ih->data->disable_callbacks = 1;
  if (end > start)
    qmlTextRemove(ih, start, end);
  qmlTextInsertAt(ih, start, text);
  qmlTextSetCursor(ih, start + text.length());
  ih->data->disable_callbacks = 0;
}

static QQuickItem* qmlTextGetFlickable(Ihandle* ih)
{
  auto* view = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!view || !ih->data->is_multiline)
    return nullptr;
  return iupqmlGetItemProperty(view, "contentItem");
}

static void qmlTextEnsureVisible(Ihandle* ih, int pos)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  QQuickItem* flick = qmlTextGetFlickable(ih);
  if (!edit)
    return;

  if (!flick)
  {
    if (!ih->data->is_multiline)
      QMetaObject::invokeMethod(edit, "ensureVisible", Qt::DirectConnection, Q_ARG(int, pos));
    return;
  }

  QRectF rect;
  QMetaObject::invokeMethod(edit, "positionToRectangle", Qt::DirectConnection, Q_RETURN_ARG(QRectF, rect), Q_ARG(int, pos));

  double y = flick->property("contentY").toDouble();
  double h = flick->height();
  if (rect.top() < y)
    flick->setProperty("contentY", rect.top());
  else if (rect.bottom() > y + h)
    flick->setProperty("contentY", rect.bottom() - h);

  double x = flick->property("contentX").toDouble();
  double w = flick->width();
  if (rect.left() < x)
    flick->setProperty("contentX", rect.left());
  else if (rect.right() > x + w)
    flick->setProperty("contentX", rect.right() - w);
}

/****************************************************************************
 * Size
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTextAddSpin(Ihandle* ih, int* w, int h)
{
  QQuickItem* spin = iupqmlTemplateItem(IUPQML_IMPORTS "SpinBox { from: 0; to: 100; editable: true }");
  int spin_min_width = 130;
  (void)h;
  (void)ih;

  if (spin)
  {
    spin_min_width = static_cast<int>(spin->implicitWidth());
    (*w) += static_cast<int>(iupqmlGetProperty(spin, "up.implicitIndicatorWidth").toDouble() +
                  iupqmlGetProperty(spin, "down.implicitIndicatorWidth").toDouble());
  }

  if (*w < spin_min_width)
    *w = spin_min_width;
}

static QQuickItem* qmlTextTemplate(Ihandle* ih)
{
  if (!iupAttribGetBoolean(ih, "_IUP_MULTILINE_TEXT"))
    return iupqmlTemplateItem(IUPQML_IMPORTS "TextField { text: \"X\" }");

  QQuickItem* view = iupqmlTemplateItem(IUPQML_IMPORTS "ScrollView { clip: true\n  TextArea { objectName: \"edit\"; text: \"X\" }\n}");
  return view ? view->findChild<QQuickItem*>("edit") : nullptr;
}

extern "C" IUP_SDK_API void iupdrvTextAddBorders(Ihandle* ih, int* w, int* h)
{
  QQuickItem* item = qmlTextTemplate(ih);

  if (item)
  {
    (*w) += static_cast<int>(item->property("leftPadding").toDouble() + item->property("rightPadding").toDouble());
    (*h) += static_cast<int>(item->property("topPadding").toDouble() + item->property("bottomPadding").toDouble());

    int min_h = static_cast<int>(item->property("implicitBackgroundHeight").toDouble());
    if (*h < min_h)
      *h = min_h;
  }
  else
  {
    (*w) += 8;
    (*h) += 8;
  }

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0)
  {
    int adjust = (iupdrvFontGetStringWidth(ih, "WWWWWWWWWW") -
                  iupdrvFontGetStringWidth(ih, "0000000000")) / 10;
    if (adjust > 0)
      (*w) -= visiblecolumns * adjust;
  }
}

extern "C" IUP_SDK_API void iupdrvTextAddExtraPadding(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  (void)w;
  (void)h;
}

/****************************************************************************
 * Position Conversion
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int* pos)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (doc)
  {
    lin--;
    col--;

    QTextBlock block = doc->findBlockByLineNumber(lin);
    if (block.isValid())
    {
      int blockLen = block.length() - 1;
      if (col > blockLen)
        col = blockLen;
      if (col < 0)
        col = 0;
      *pos = block.position() + col;
    }
    else
      *pos = 0;
  }
  else
    *pos = col - 1;
}

extern "C" IUP_SDK_API void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int* lin, int* col)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (doc)
  {
    QTextBlock block = doc->findBlock(pos);
    if (block.isValid())
    {
      *lin = block.blockNumber() + 1;
      *col = pos - block.position() + 1;
    }
    else
    {
      *lin = 1;
      *col = 1;
    }
  }
  else
  {
    *lin = 1;
    *col = pos + 1;
  }
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qmlTextApplyFilter(Ihandle* ih)
{
  const char* filter = iupAttribGet(ih, "FILTER");
  if (!filter || !ih->handle)
    return;

  bool is_number = iupStrEqualNoCase(filter, "NUMBER");
  bool is_upper = iupStrEqualNoCase(filter, "UPPERCASE");
  bool is_lower = iupStrEqualNoCase(filter, "LOWERCASE");
  if (!is_number && !is_upper && !is_lower)
    return;

  QString text = qmlTextGetText(ih);
  QString filtered;
  filtered.reserve(text.size());
  if (is_number)
  {
    for (QChar c : text)
      if (c.isDigit()) filtered.append(c);
  }
  else if (is_upper)
    filtered = text.toUpper();
  else
    filtered = text.toLower();

  if (filtered == text)
    return;

  int pos = qmlTextGetCursor(ih);
  qmlTextSetText(ih, filtered);
  qmlTextSetCursor(ih, qMin(pos, static_cast<int>(filtered.length())));
}

static void qmlTextValueChanged(Ihandle* ih)
{
  if (ih->data->disable_callbacks)
    return;

  qmlTextApplyFilter(ih);

  IFn cb = static_cast<IFn>(IupGetCallback(ih, "VALUECHANGED_CB"));
  if (cb)
    cb(ih);
}

static void qmlTextCursorPositionChanged(Ihandle* ih)
{
  auto cb = reinterpret_cast<IFniii>(IupGetCallback(ih, "CARET_CB"));
  if (cb)
  {
    int lin, col, pos = qmlTextGetCursor(ih);
    iupdrvTextConvertPosToLinCol(ih, pos, &lin, &col);
    cb(ih, lin, col, pos);
  }
}

static void qmlTextArbitrateHistory(Ihandle* ih, int redo)
{
  auto cb = reinterpret_cast<IFnis>(IupGetCallback(ih, "ACTION"));
  QQuickItem* edit = qmlTextGetEdit(ih);

  if (!edit || (!cb && !ih->data->mask && !ih->data->nc))
    return;

  QString value = qmlTextGetText(ih);
  if (iupEditCheckNewValue(ih, cb, value.toUtf8().constData(), ih->data->mask, ih->data->nc))
    return;

  ih->data->disable_callbacks = 1;
  QMetaObject::invokeMethod(edit, redo ? "undo" : "redo", Qt::DirectConnection);
  ih->data->disable_callbacks = 0;
}

static int qmlTextArbitrateClipboard(Ihandle* ih, int cut)
{
  auto cb = reinterpret_cast<IFnis>(IupGetCallback(ih, "ACTION"));
  int start, end, ret;

  if (!cb && !ih->data->mask && !ih->data->nc)
    return 1;

  qmlTextGetSelectionRange(ih, &start, &end);

  if (cut)
  {
    if (start == end)
      return 1;

    ret = iupEditCallActionCb(ih, cb, nullptr, start, end, ih->data->mask, ih->data->nc, 0, 1);
  }
  else
  {
    QString clip = QGuiApplication::clipboard()->text();
    if (clip.isEmpty())
      return 1;

    ret = iupEditCallActionCb(ih, cb, clip.toUtf8().constData(), start, end, ih->data->mask, ih->data->nc, 0, 1);
  }

  return ret != 0;
}

static void qmlTextLinkActivated(Ihandle* ih, const QString& link)
{
  QByteArray bytes = link.toUtf8();
  IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "LINK_CB"));
  if (cb)
  {
    int ret = cb(ih, const_cast<char*>(bytes.constData()));
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_DEFAULT)
      IupHelp(bytes.constData());
  }
  else
    IupHelp(bytes.constData());
}

class IupQmlTextKeyFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlTextKeyFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    (void)obj;
    if (event->type() != QEvent::KeyPress || !iupObjectCheck(ih))
      return false;

    auto* evt = static_cast<QKeyEvent*>(event);

    if (iupqmlKeyPressEvent(qmlTextGetEdit(ih), evt, ih))
      return true;

    if (evt->key() == Qt::Key_Insert && evt->modifiers() == Qt::NoModifier)
    {
      QQuickItem* edit = qmlTextGetEdit(ih);
      if (edit)
        edit->setProperty("overwriteMode", !edit->property("overwriteMode").toBool());
      return true;
    }

    auto cb = reinterpret_cast<IFnis>(IupGetCallback(ih, "ACTION"));
    if (!cb && !ih->data->mask && !ih->data->nc)
      return false;

    if (evt->matches(QKeySequence::Undo) || evt->matches(QKeySequence::Redo))
    {
      int redo = evt->matches(QKeySequence::Redo);
      QMetaObject::invokeMethod(qmlTextGetEdit(ih), redo ? "redo" : "undo", Qt::DirectConnection);
      qmlTextArbitrateHistory(ih, redo);
      return true;
    }

    if ((evt->matches(QKeySequence::Paste) || evt->matches(QKeySequence::Cut)) &&
        !qmlTextArbitrateClipboard(ih, evt->matches(QKeySequence::Cut)))
    {
      if (evt->matches(QKeySequence::Cut))
        QMetaObject::invokeMethod(qmlTextGetEdit(ih), "copy", Qt::DirectConnection);
      return true;
    }

    int start, end, ret;
    qmlTextGetSelectionRange(ih, &start, &end);

    if ((evt->key() == Qt::Key_Backspace || evt->key() == Qt::Key_Delete) && !(evt->modifiers() & (Qt::ControlModifier | Qt::AltModifier)))
    {
      int remove_dir = evt->key() == Qt::Key_Delete ? 1 : -1;
      int len = static_cast<int>(qmlTextGetText(ih).length());
      if (start == end && ((remove_dir == -1 && start == 0) || (remove_dir == 1 && start >= len)))
        return false;
      ret = iupEditCallActionCb(ih, cb, nullptr, start, end, ih->data->mask, ih->data->nc, remove_dir, 1);
      return ret == 0;
    }

    if (evt->key() == Qt::Key_Return || evt->key() == Qt::Key_Enter)
    {
      if (!ih->data->is_multiline)
        return false;
      ret = iupEditCallActionCb(ih, cb, "\n", start, end, ih->data->mask, ih->data->nc, 0, 1);
      return ret == 0;
    }

    QString text = evt->text();
    if (text.isEmpty() || !text.at(0).isPrint() || text.at(0).unicode() < 32)
      return false;

    ret = iupEditCallActionCb(ih, cb, text.toUtf8().constData(), start, end, ih->data->mask, ih->data->nc, 0, 1);
    if (ret == 0)
      return true;

    if (ret != -1)
    {
      qmlTextReplaceSelection(ih, QString(QChar(ret)));
      return true;
    }

    return false;
  }
};

/****************************************************************************
 * Attributes
 ****************************************************************************/

static int qmlTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";

  if (!ih->handle)
    return 0;

  QObject* spin = qmlTextGetSpin(ih);
  if (spin && spin->property("iupAuto").toBool())
  {
    int int_value = 0;
    if (value && *value)
      iupStrToInt(value, &int_value);
    ih->data->disable_callbacks = 1;
    spin->setProperty("value", int_value);
    ih->data->disable_callbacks = 0;
    return 0;
  }

  qmlTextSetText(ih, QString::fromUtf8(value));
  return 0;
}

static char* qmlTextGetValueAttrib(Ihandle* ih)
{
  QObject* spin = qmlTextGetSpin(ih);
  if (spin && spin->property("iupAuto").toBool())
    return iupStrReturnInt(spin->property("value").toInt());

  return iupStrReturnStr(qmlTextGetText(ih).toUtf8().constData());
}

static char* qmlTextGetLineValueAttrib(Ihandle* ih)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (doc)
  {
    QTextBlock block = doc->findBlock(qmlTextGetCursor(ih));
    return iupStrReturnStr(block.text().toUtf8().constData());
  }
  return qmlTextGetValueAttrib(ih);
}

static int qmlTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  qmlTextReplaceSelection(ih, QString::fromUtf8(value));
  return 0;
}

static char* qmlTextGetSelectedTextAttrib(Ihandle* ih)
{
  int start, end;
  qmlTextGetSelectionRange(ih, &start, &end);
  if (start == end)
    return nullptr;

  QQuickItem* edit = qmlTextGetEdit(ih);
  QString value = edit->property("selectedText").toString();
  return iupStrReturnStr(value.toUtf8().constData());
}

static int qmlTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start = 1, end = 1;
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    QMetaObject::invokeMethod(edit, "deselect", Qt::DirectConnection);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    QMetaObject::invokeMethod(edit, "selectAll", Qt::DirectConnection);
    return 0;
  }

  if (ih->data->is_multiline)
  {
    int lin1 = 1, col1 = 1, lin2 = 1, col2 = 1;
    if (!iupStrToLinColRange(value, &lin1, &col1, &lin2, &col2))
      return 0;
    iupdrvTextConvertLinColToPos(ih, lin1, col1, &start);
    iupdrvTextConvertLinColToPos(ih, lin2, col2, &end);
    qmlTextSelect(ih, start, end);
  }
  else
  {
    if (iupStrToIntInt(value, &start, &end, ':') != 2)
      return 0;
    if (start < 1) start = 1;
    if (end < 1) end = 1;
    qmlTextSelect(ih, start - 1, end - 1);
  }

  return 0;
}

static char* qmlTextGetSelectionAttrib(Ihandle* ih)
{
  int start, end;
  qmlTextGetSelectionRange(ih, &start, &end);
  if (start == end)
    return nullptr;

  if (ih->data->is_multiline)
  {
    int lin1, col1, lin2, col2;
    iupdrvTextConvertPosToLinCol(ih, start, &lin1, &col1);
    iupdrvTextConvertPosToLinCol(ih, end, &lin2, &col2);
    return iupStrReturnStrf("%d,%d:%d,%d", lin1, col1, lin2, col2);
  }

  return iupStrReturnStrf("%d:%d", start + 1, end + 1);
}

static int qmlTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start = 0, end = 0;
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    QMetaObject::invokeMethod(edit, "deselect", Qt::DirectConnection);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    QMetaObject::invokeMethod(edit, "selectAll", Qt::DirectConnection);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 0) start = 0;
  if (end < 0) end = 0;

  qmlTextSelect(ih, start, end);
  return 0;
}

static char* qmlTextGetSelectionPosAttrib(Ihandle* ih)
{
  int start, end;
  qmlTextGetSelectionRange(ih, &start, &end);
  if (start == end)
    return nullptr;
  return iupStrReturnStrf("%d:%d", start, end);
}

static int qmlTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1, pos;
    iupStrToIntInt(value, &lin, &col, ',');
    iupdrvTextConvertLinColToPos(ih, lin, col, &pos);
    qmlTextSetCursor(ih, pos);
  }
  else
  {
    int pos = 1;
    iupStrToInt(value, &pos);
    pos--;
    if (pos < 0) pos = 0;
    qmlTextSetCursor(ih, pos);
  }

  return 0;
}

static char* qmlTextGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int lin, col;
    iupdrvTextConvertPosToLinCol(ih, qmlTextGetCursor(ih), &lin, &col);
    return iupStrReturnIntInt(lin, col, ',');
  }
  return iupStrReturnInt(qmlTextGetCursor(ih) + 1);
}

static int qmlTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;
  qmlTextSetCursor(ih, pos);
  return 0;
}

static char* qmlTextGetCaretPosAttrib(Ihandle* ih)
{
  return iupStrReturnInt(qmlTextGetCursor(ih));
}

static int qmlTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int lin = 1, col = 1, pos;
  if (!value)
    return 0;

  iupStrToIntInt(value, &lin, &col, ',');
  if (lin < 1) lin = 1;
  if (col < 1) col = 1;

  iupdrvTextConvertLinColToPos(ih, lin, col, &pos);
  qmlTextSetCursor(ih, pos);
  qmlTextEnsureVisible(ih, pos);
  return 0;
}

static int qmlTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;
  qmlTextSetCursor(ih, pos);
  qmlTextEnsureVisible(ih, pos);
  return 0;
}

static int qmlTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  qmlTextReplaceSelection(ih, QString::fromUtf8(value));
  return 0;
}

static int qmlTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  ih->data->disable_callbacks = 1;

  QString to_insert = QString::fromUtf8(value ? value : "");
  int len = static_cast<int>(qmlTextGetText(ih).length());

  if (ih->data->is_multiline && ih->data->append_newline && len > 0)
    to_insert.prepend('\n');

  int cursor = qmlTextGetCursor(ih);
  qmlTextInsertAt(ih, len, to_insert);

  if (ih->data->is_multiline && ih->data->append_scroll)
  {
    int end = len + static_cast<int>(to_insert.length());
    qmlTextSetCursor(ih, end);
    qmlTextEnsureVisible(ih, end);
  }
  else
    qmlTextSetCursor(ih, cursor);

  ih->data->disable_callbacks = 0;
  return 0;
}

static int qmlTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    edit->setProperty("readOnly", iupStrBoolean(value) ? true : false);
  return 0;
}

static char* qmlTextGetReadOnlyAttrib(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  return iupStrReturnBoolean(edit && edit->property("readOnly").toBool());
}

static int qmlTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = 0;

  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit && !ih->data->is_multiline)
    edit->setProperty("maximumLength", ih->data->nc > 0 ? ih->data->nc : 32767);

  return 0;
}

static char* qmlTextGetNCAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline)
    return iupStrReturnInt(ih->data->nc);
  return nullptr;
}

static int qmlTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!value || !edit)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
    QMetaObject::invokeMethod(edit, "copy", Qt::DirectConnection);
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    if (qmlTextArbitrateClipboard(ih, 1))
      QMetaObject::invokeMethod(edit, "cut", Qt::DirectConnection);
    else
      QMetaObject::invokeMethod(edit, "copy", Qt::DirectConnection);
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    if (qmlTextArbitrateClipboard(ih, 0))
      QMetaObject::invokeMethod(edit, "paste", Qt::DirectConnection);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    int start, end;
    qmlTextGetSelectionRange(ih, &start, &end);
    if (end > start)
      qmlTextRemove(ih, start, end);
    else
      QMetaObject::invokeMethod(edit, "clear", Qt::DirectConnection);
  }
  else if (iupStrEqualNoCase(value, "UNDO"))
  {
    QMetaObject::invokeMethod(edit, "undo", Qt::DirectConnection);
    qmlTextArbitrateHistory(ih, 0);
  }
  else if (iupStrEqualNoCase(value, "REDO"))
  {
    QMetaObject::invokeMethod(edit, "redo", Qt::DirectConnection);
    qmlTextArbitrateHistory(ih, 1);
  }
  else if (iupStrEqualNoCase(value, "CLEARUNDO"))
  {
    QTextDocument* doc = qmlTextGetDocument(ih);
    if (doc)
      doc->clearUndoRedoStacks();
  }

  return 0;
}

static int qmlTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  QObject* spin = qmlTextGetSpin(ih);
  if (spin)
  {
    int pos;
    if (iupStrToInt(value, &pos))
    {
      ih->data->disable_callbacks = 1;
      spin->setProperty("value", pos);
      ih->data->disable_callbacks = 0;
    }
  }
  return 1;
}

static char* qmlTextGetSpinValueAttrib(Ihandle* ih)
{
  QObject* spin = qmlTextGetSpin(ih);
  if (spin)
    return iupStrReturnInt(spin->property("value").toInt());
  return nullptr;
}

static int qmlTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  QObject* spin = qmlTextGetSpin(ih);
  int min;
  if (spin && iupStrToInt(value, &min))
  {
    ih->data->disable_callbacks = 1;
    spin->setProperty("from", min);
    ih->data->disable_callbacks = 0;
  }
  return 1;
}

static int qmlTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  QObject* spin = qmlTextGetSpin(ih);
  int max;
  if (spin && iupStrToInt(value, &max))
  {
    ih->data->disable_callbacks = 1;
    spin->setProperty("to", max);
    ih->data->disable_callbacks = 0;
  }
  return 1;
}

static int qmlTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  QObject* spin = qmlTextGetSpin(ih);
  int inc;
  if (spin && iupStrToInt(value, &inc))
    spin->setProperty("stepSize", inc);
  return 1;
}

static int qmlTextSetPasswordAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit && !ih->data->is_multiline)
    edit->setProperty("echoMode", iupStrBoolean(value) ? 2 : 0);
  return 0;
}

static char* qmlTextGetPasswordAttrib(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit && !ih->data->is_multiline)
    return iupStrReturnBoolean(edit->property("echoMode").toInt() == 2);
  return nullptr;
}

static int qmlTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  int align = Qt::AlignLeft;

  if (iupStrEqualNoCase(value, "ARIGHT"))
    align = Qt::AlignRight;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    align = Qt::AlignHCenter;

  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    edit->setProperty("horizontalAlignment", align);

  return 1;
}

static char* qmlTextGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(static_cast<int>(qmlTextGetText(ih).length()));
}

static char* qmlTextGetLineCountAttrib(Ihandle* ih)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (doc)
    return iupStrReturnInt(doc->blockCount());
  return nullptr;
}

static int qmlTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    edit->setProperty("placeholderText", value ? QString::fromUtf8(value) : QString());
  return 1;
}

static char* qmlTextGetCueBannerAttrib(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
  {
    QString text = edit->property("placeholderText").toString();
    if (!text.isEmpty())
      return iupStrReturnStr(text.toUtf8().constData());
  }
  return nullptr;
}

static int qmlTextSetFilterAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  qmlTextApplyFilter(ih);
  return 1;
}

static int qmlTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
    edit->setProperty("overwriteMode", iupStrBoolean(value) ? true : false);
  return 0;
}

static char* qmlTextGetOverwriteAttrib(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  return iupStrReturnBoolean(edit && edit->property("overwriteMode").toBool());
}

static int qmlTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit && ih->data->is_multiline)
  {
    int tabsize = 8;
    iupStrToInt(value, &tabsize);

    QFont* font = iupqmlGetIhFont(ih);
    if (font)
    {
      QFontMetrics metrics(*font);
      edit->setProperty("tabStopDistance", static_cast<double>(tabsize * metrics.horizontalAdvance(' ')));
    }
  }
  return 1;
}

static void qmlTextFitLine(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit || ih->data->is_multiline || qmlTextGetSpin(ih))
    return;

  if (!edit->property("_iup_fit_top").isValid())
  {
    edit->setProperty("_iup_fit_top", edit->property("topPadding"));
    edit->setProperty("_iup_fit_bottom", edit->property("bottomPadding"));
  }

  double top = edit->property("_iup_fit_top").toDouble();
  double bottom = edit->property("_iup_fit_bottom").toDouble();
  double excess = std::ceil(edit->property("contentHeight").toDouble() - (edit->height() - top - bottom));
  if (excess > 0)
  {
    double cut_top = std::min(top, std::ceil(excess / 2));
    double cut_bottom = std::min(bottom, excess - cut_top);
    cut_top = std::min(top, excess - cut_bottom);
    top -= cut_top;
    bottom -= cut_bottom;
  }

  if (edit->property("topPadding").toDouble() != top)
    edit->setProperty("topPadding", top);
  if (edit->property("bottomPadding").toDouble() != bottom)
    edit->setProperty("bottomPadding", bottom);
}

static void qmlTextApplyPaddingValues(Ihandle* ih, QQuickItem* edit, int border)
{
  int horiz = ih->data->horiz_padding;
  int vert = ih->data->vert_padding;
  int spin = qmlTextGetSpin(ih) != nullptr;
  QQuickItem* tpl = spin ? nullptr : qmlTextTemplate(ih);

  if (border && horiz == 0 && vert == 0)
  {
    iupqmlRestorePaddings(edit, tpl);
    return;
  }

  double left = horiz, right = horiz, top = vert, bottom = vert;
  if (border && tpl)
  {
    left += tpl->property("leftPadding").toDouble();
    right += tpl->property("rightPadding").toDouble();
    top += tpl->property("topPadding").toDouble();
    bottom += tpl->property("bottomPadding").toDouble();
  }

  iupqmlSetPaddings(edit, left, right, top, bottom);
}

static void qmlTextApplyPadding(Ihandle* ih, int border)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit)
    return;

  QVariant fit_top = edit->property("_iup_fit_top");
  if (fit_top.isValid())
  {
    edit->setProperty("topPadding", fit_top);
    edit->setProperty("bottomPadding", edit->property("_iup_fit_bottom"));
    edit->setProperty("_iup_fit_top", QVariant());
    edit->setProperty("_iup_fit_bottom", QVariant());
  }

  qmlTextApplyPaddingValues(ih, edit, border);
  qmlTextFitLine(ih);
}

static int qmlTextSetBorderAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit || qmlTextGetSpin(ih))
    return 1;

  int border = iupStrBoolean(value);
  if (border)
  {
    QVariant saved = edit->property("_iup_border_width");
    if (saved.isValid())
      iupqmlSetProperty(edit, "background.border.width", saved);
    edit->setProperty("_iup_border_off", QVariant());
  }
  else
  {
    QVariant width = iupqmlGetProperty(edit, "background.border.width");
    if (width.isValid() && !edit->property("_iup_border_width").isValid())
      edit->setProperty("_iup_border_width", width);
    iupqmlSetProperty(edit, "background.border.width", 0);
    edit->setProperty("_iup_border_off", true);
  }

  qmlTextApplyPadding(ih, border);
  return 1;
}

static char* qmlTextGetBorderAttrib(Ihandle* ih)
{
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit)
    return nullptr;
  return iupStrReturnBoolean(!edit->property("_iup_border_off").toBool());
}

static char* qmlTextGetScrollVisibleAttrib(Ihandle* ih)
{
  QQuickItem* flick = qmlTextGetFlickable(ih);
  if (!flick)
    return nullptr;

  int horiz = flick->property("contentWidth").toDouble() > flick->width();
  int vert = flick->property("contentHeight").toDouble() > flick->height();

  if (horiz && vert)
    return const_cast<char*>("YES");
  else if (horiz)
    return const_cast<char*>("HORIZONTAL");
  else if (vert)
    return const_cast<char*>("VERTICAL");
  return const_cast<char*>("NO");
}

static int qmlTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (qmlTextGetEdit(ih))
  {
    qmlTextApplyPadding(ih, iupAttribGetBoolean(ih, "BORDER"));
    return 0;
  }

  return 1;
}

static int qmlTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(edit, "base", QColor(r, g, b));
  iupqmlSetPaletteColor(edit, "window", QColor(r, g, b));
  return 1;
}

static int qmlTextSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!edit || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(edit, "text", QColor(r, g, b));
  edit->setProperty("color", QColor(r, g, b));
  return 1;
}

static int qmlTextSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  QQuickItem* edit = qmlTextGetEdit(ih);
  if (edit)
  {
    iupqmlUpdateItemFont(ih, edit);
    if (ih->data->is_multiline)
      qmlTextSetTabSizeAttrib(ih, iupAttribGetStr(ih, "TABSIZE"));
  }

  QObject* spin = qmlTextGetSpin(ih);
  if (spin)
    iupqmlUpdateItemFont(ih, spin);

  return 1;
}

/****************************************************************************
 * Formatting
 ****************************************************************************/

static bool qmlTextParseSelection(Ihandle* ih, const char* value, int* start, int* end)
{
  int lin_start = 1, col_start = 1, lin_end = 1, col_end = 1;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    *start = 0;
    *end = 0;
    return true;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    *start = 0;
    *end = static_cast<int>(qmlTextGetText(ih).length());
    return true;
  }

  if (!iupStrToLinColRange(value, &lin_start, &col_start, &lin_end, &col_end))
    return false;

  if (lin_start < 1 || col_start < 1 || lin_end < 1 || col_end < 1)
    return false;

  iupdrvTextConvertLinColToPos(ih, lin_start, col_start, start);
  iupdrvTextConvertLinColToPos(ih, lin_end, col_end, end);
  return true;
}

static bool qmlTextParseParagraphFormat(Ihandle* formattag, QTextBlockFormat* blockFormat)
{
  int val;
  const char* format;
  bool has_format = false;

  format = iupAttribGet(formattag, "ALIGNMENT");
  if (format)
  {
    if (iupStrEqualNoCase(format, "JUSTIFY"))
      blockFormat->setAlignment(Qt::AlignJustify);
    else if (iupStrEqualNoCase(format, "RIGHT"))
      blockFormat->setAlignment(Qt::AlignRight);
    else if (iupStrEqualNoCase(format, "CENTER"))
      blockFormat->setAlignment(Qt::AlignCenter);
    else
      blockFormat->setAlignment(Qt::AlignLeft);
    has_format = true;
  }

  format = iupAttribGet(formattag, "INDENT");
  if (format && iupStrToInt(format, &val))
  {
    blockFormat->setLeftMargin(val);

    const char* indent_right = iupAttribGet(formattag, "INDENTRIGHT");
    if (indent_right && iupStrToInt(indent_right, &val))
      blockFormat->setRightMargin(val);

    const char* indent_offset = iupAttribGet(formattag, "INDENTOFFSET");
    if (indent_offset && iupStrToInt(indent_offset, &val))
      blockFormat->setTextIndent(val);

    has_format = true;
  }

  format = iupAttribGet(formattag, "SPACEBEFORE");
  if (format && iupStrToInt(format, &val))
  {
    blockFormat->setTopMargin(val);
    has_format = true;
  }

  format = iupAttribGet(formattag, "SPACEAFTER");
  if (format && iupStrToInt(format, &val))
  {
    blockFormat->setBottomMargin(val);
    has_format = true;
  }

  format = iupAttribGet(formattag, "LINESPACING");
  if (format)
  {
    if (iupStrEqualNoCase(format, "SINGLE"))
      blockFormat->setLineHeight(100, QTextBlockFormat::ProportionalHeight);
    else if (iupStrEqualNoCase(format, "ONEHALF"))
      blockFormat->setLineHeight(150, QTextBlockFormat::ProportionalHeight);
    else if (iupStrEqualNoCase(format, "DOUBLE"))
      blockFormat->setLineHeight(200, QTextBlockFormat::ProportionalHeight);
    else if (iupStrToInt(format, &val))
      blockFormat->setLineHeight(val, QTextBlockFormat::FixedHeight);
    has_format = true;
  }

  format = iupAttribGet(formattag, "TABSARRAY");
  if (format)
  {
    QList<QTextOption::Tab> tabs;
    int pos, i = 0;
    char* str;

    while (format)
    {
      str = iupStrDupUntil(&format, ' ');
      if (!str) break;
      iupStrToInt(str, &pos);
      free(str);

      str = iupStrDupUntil(&format, ' ');
      if (!str)
      {
        if (!format || !*format) break;
        str = iupStrDup(format);
        format = nullptr;
      }

      QTextOption::TabType tabType = QTextOption::LeftTab;
      if (iupStrEqualNoCase(str, "RIGHT"))
        tabType = QTextOption::RightTab;
      else if (iupStrEqualNoCase(str, "CENTER"))
        tabType = QTextOption::CenterTab;
      else if (iupStrEqualNoCase(str, "DECIMAL"))
        tabType = QTextOption::DelimiterTab;
      free(str);

      tabs.append(QTextOption::Tab(pos, tabType));
      i++;
      if (i == 32) break;
    }

    blockFormat->setTabPositions(tabs);
    has_format = true;
  }

  return has_format;
}

static void qmlTextParseCharacterFormat(Ihandle* formattag, QTextCharFormat* charFormat)
{
  int val;
  const char* format;

  format = iupAttribGet(formattag, "FONTFACE");
  if (format)
  {
    const char* mapped_name = iupFontGetPangoName(format);
    charFormat->setFontFamilies({QString(mapped_name ? mapped_name : format)});
  }

  format = iupAttribGet(formattag, "FONTSIZE");
  if (format && iupStrToInt(format, &val))
  {
    if (val < 0)
      charFormat->setProperty(QTextFormat::FontPixelSize, -val);
    else
      charFormat->setFontPointSize(val);
  }

  format = iupAttribGet(formattag, "FONTSCALE");
  if (format)
  {
    double fval = 0;
    if (iupStrEqualNoCase(format, "XX-SMALL"))
      fval = 0.5787037037037;
    else if (iupStrEqualNoCase(format, "X-SMALL"))
      fval = 0.6444444444444;
    else if (iupStrEqualNoCase(format, "SMALL"))
      fval = 0.8333333333333;
    else if (iupStrEqualNoCase(format, "MEDIUM"))
      fval = 1.0;
    else if (iupStrEqualNoCase(format, "LARGE"))
      fval = 1.2;
    else if (iupStrEqualNoCase(format, "X-LARGE"))
      fval = 1.4399999999999;
    else if (iupStrEqualNoCase(format, "XX-LARGE"))
      fval = 1.728;
    else
      iupStrToDouble(format, &fval);

    if (fval > 0)
    {
      double currentSize = charFormat->fontPointSize();
      if (currentSize > 0)
        charFormat->setFontPointSize(currentSize * fval);
    }
  }

  format = iupAttribGet(formattag, "RISE");
  if (format)
  {
    if (iupStrEqualNoCase(format, "SUPERSCRIPT"))
      charFormat->setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    else if (iupStrEqualNoCase(format, "SUBSCRIPT"))
      charFormat->setVerticalAlignment(QTextCharFormat::AlignSubScript);
    else if (iupStrToInt(format, &val))
    {
      if (val > 0)
        charFormat->setVerticalAlignment(QTextCharFormat::AlignSuperScript);
      else if (val < 0)
        charFormat->setVerticalAlignment(QTextCharFormat::AlignSubScript);
      else
        charFormat->setVerticalAlignment(QTextCharFormat::AlignNormal);
    }
  }

  format = iupAttribGet(formattag, "SMALLCAPS");
  if (format)
    charFormat->setFontCapitalization(iupStrBoolean(format) ? QFont::SmallCaps : QFont::MixedCase);

  format = iupAttribGet(formattag, "ITALIC");
  if (format)
    charFormat->setFontItalic(iupStrBoolean(format));

  format = iupAttribGet(formattag, "STRIKEOUT");
  if (format)
    charFormat->setFontStrikeOut(iupStrBoolean(format));

  format = iupAttribGet(formattag, "FGCOLOR");
  if (format)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(format, &r, &g, &b))
      charFormat->setForeground(QColor(r, g, b));
  }

  format = iupAttribGet(formattag, "BGCOLOR");
  if (format)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(format, &r, &g, &b))
      charFormat->setBackground(QColor(r, g, b));
  }

  format = iupAttribGet(formattag, "UNDERLINE");
  if (format)
  {
    if (iupStrEqualNoCase(format, "SINGLE"))
      charFormat->setUnderlineStyle(QTextCharFormat::SingleUnderline);
    else if (iupStrEqualNoCase(format, "DOUBLE"))
      charFormat->setUnderlineStyle(QTextCharFormat::WaveUnderline);
    else if (iupStrEqualNoCase(format, "DOTTED"))
      charFormat->setUnderlineStyle(QTextCharFormat::DotLine);
    else
      charFormat->setUnderlineStyle(QTextCharFormat::NoUnderline);
  }

  format = iupAttribGet(formattag, "WEIGHT");
  if (format)
  {
    if (iupStrEqualNoCase(format, "EXTRALIGHT"))
      charFormat->setFontWeight(QFont::ExtraLight);
    else if (iupStrEqualNoCase(format, "LIGHT"))
      charFormat->setFontWeight(QFont::Light);
    else if (iupStrEqualNoCase(format, "SEMIBOLD"))
      charFormat->setFontWeight(QFont::DemiBold);
    else if (iupStrEqualNoCase(format, "BOLD"))
      charFormat->setFontWeight(QFont::Bold);
    else if (iupStrEqualNoCase(format, "EXTRABOLD"))
      charFormat->setFontWeight(QFont::ExtraBold);
    else if (iupStrEqualNoCase(format, "HEAVY"))
      charFormat->setFontWeight(QFont::Black);
    else
      charFormat->setFontWeight(QFont::Normal);
  }

  format = iupAttribGet(formattag, "LINK");
  if (format)
  {
    charFormat->setAnchor(true);
    charFormat->setAnchorHref(QString(format));

    if (!iupAttribGet(formattag, "FGCOLOR"))
      charFormat->setForeground(QColor(0, 0, 255));

    if (!iupAttribGet(formattag, "UNDERLINE"))
      charFormat->setUnderlineStyle(QTextCharFormat::SingleUnderline);
  }
}

extern "C" IUP_SDK_API int iupdrvTextGetFormatTags(Ihandle* ih, Ihandle* bulk_tag)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (!doc)
    return 0;

  for (QTextBlock block = doc->begin(); block.isValid(); block = block.next())
  {
    qreal left_margin = block.blockFormat().leftMargin();

    for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it)
    {
      QTextFragment fragment = it.fragment();
      if (!fragment.isValid())
        continue;

      QTextCharFormat charFormat = fragment.charFormat();
      QFont font = charFormat.font();
      Ihandle* formattag = IupUser();

      IupSetStrf(formattag, "SELECTIONPOS", "%d:%d", fragment.position(), fragment.position() + fragment.length());
      IupAppend(bulk_tag, formattag);

      IupSetAttribute(formattag, "WEIGHT", charFormat.fontWeight() >= QFont::DemiBold ? "BOLD" : "NORMAL");
      IupSetAttribute(formattag, "ITALIC", font.italic() ? "YES" : "NO");
      IupSetAttribute(formattag, "STRIKEOUT", font.strikeOut() ? "YES" : "NO");

      if (!font.family().isEmpty())
        IupSetStrAttribute(formattag, "FONTFACE", font.family().toUtf8().constData());

      qreal base_size = doc->defaultFont().pointSizeF();
      if (charFormat.fontPointSize() > 0 && base_size > 0)
        IupSetDouble(formattag, "FONTSCALE", charFormat.fontPointSize() / base_size);

      if (charFormat.isAnchor() && !charFormat.anchorHref().isEmpty())
        IupSetStrAttribute(formattag, "LINK", charFormat.anchorHref().toUtf8().constData());

      if (charFormat.isImageFormat())
      {
        QString name = charFormat.toImageFormat().name();
        if (name.startsWith(QStringLiteral("iup_image_")))
          name = name.mid(10);
        if (!name.isEmpty())
          IupSetStrAttribute(formattag, "IMAGE", name.toUtf8().constData());
      }

      if (left_margin > 0)
        IupSetInt(formattag, "INDENT", static_cast<int>(left_margin));
    }
  }

  return 1;
}

static int qmlTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (!doc)
    return 0;

  int start, end;
  qmlTextGetSelectionRange(ih, &start, &end);

  QTextCursor cursor(doc);
  if (iupStrEqualNoCase(value, "ALL"))
    cursor.select(QTextCursor::Document);
  else if (start == end)
    return 0;
  else
  {
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
  }

  cursor.setCharFormat(QTextCharFormat());
  cursor.setBlockFormat(QTextBlockFormat());

  return 0;
}

extern "C" IUP_SDK_API void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (!doc)
    return nullptr;

  bool* undo_enabled = new bool(doc->isUndoRedoEnabled());
  doc->setUndoRedoEnabled(false);

  return reinterpret_cast<void*>(undo_enabled);
}

extern "C" IUP_SDK_API void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  QTextDocument* doc = qmlTextGetDocument(ih);
  if (!doc || !state)
    return;

  bool* undo_enabled = static_cast<bool*>(state);
  doc->setUndoRedoEnabled(*undo_enabled);
  delete undo_enabled;
}

extern "C" IUP_SDK_API void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  (void)bulk;

  QTextDocument* doc = qmlTextGetDocument(ih);
  QQuickItem* edit = qmlTextGetEdit(ih);
  if (!doc || !edit)
    return;

  int start_pos = 0, end_pos = 0;

  const char* selection = iupAttribGet(formattag, "SELECTION");
  if (selection)
  {
    if (!qmlTextParseSelection(ih, selection, &start_pos, &end_pos))
      return;
  }
  else
  {
    const char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
    if (selectionpos)
    {
      if (iupStrToIntInt(selectionpos, &start_pos, &end_pos, ':') != 2)
        return;
    }
    else
    {
      start_pos = qmlTextGetCursor(ih);
      end_pos = start_pos;
    }
  }

  QTextCursor cursor(doc);
  cursor.setPosition(start_pos);
  cursor.setPosition(end_pos, QTextCursor::KeepAnchor);

  {
    const char* image_name = iupAttribGet(formattag, "IMAGE");
    if (image_name)
    {
      auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(image_name, ih, 0, nullptr));
      if (pixmap)
      {
        int new_w = 0, new_h = 0;
        const char* attr;

        attr = iupAttribGet(formattag, "WIDTH");
        if (attr) iupStrToInt(attr, &new_w);
        attr = iupAttribGet(formattag, "HEIGHT");
        if (attr) iupStrToInt(attr, &new_h);

        QString resName = QString("iup_image_%1").arg(image_name);
        doc->addResource(QTextDocument::ImageResource, doc->baseUrl().resolved(QUrl(resName)), *pixmap);

        cursor.removeSelectedText();

        QTextImageFormat imgFormat;
        imgFormat.setName(resName);
        if (new_w > 0) imgFormat.setWidth(new_w);
        if (new_h > 0) imgFormat.setHeight(new_h);

        cursor.insertImage(imgFormat);
      }
      return;
    }
  }

  if (iupAttribGet(formattag, "FONTSCALE") && !iupAttribGet(formattag, "FONTSIZE"))
    iupAttribSet(formattag, "FONTSIZE", iupGetFontSizeAttrib(ih));

  QTextCharFormat charFormat;
  qmlTextParseCharacterFormat(formattag, &charFormat);
  cursor.mergeCharFormat(charFormat);

  QTextBlockFormat blockFormat;
  if (qmlTextParseParagraphFormat(formattag, &blockFormat))
    cursor.mergeBlockFormat(blockFormat);

  const char* numbering = iupAttribGet(formattag, "NUMBERING");
  if (numbering)
  {
    if (iupStrEqualNoCase(numbering, "NONE"))
    {
      QTextList* list = cursor.currentList();
      if (list)
        list->remove(cursor.block());
    }
    else
    {
      QTextListFormat listFormat;

      if (iupStrEqualNoCase(numbering, "BULLET"))
        listFormat.setStyle(QTextListFormat::ListDisc);
      else if (iupStrEqualNoCase(numbering, "ARABIC"))
        listFormat.setStyle(QTextListFormat::ListDecimal);
      else if (iupStrEqualNoCase(numbering, "LCLETTER"))
        listFormat.setStyle(QTextListFormat::ListLowerAlpha);
      else if (iupStrEqualNoCase(numbering, "UCLETTER"))
        listFormat.setStyle(QTextListFormat::ListUpperAlpha);
      else if (iupStrEqualNoCase(numbering, "LCROMAN"))
        listFormat.setStyle(QTextListFormat::ListLowerRoman);
      else if (iupStrEqualNoCase(numbering, "UCROMAN"))
        listFormat.setStyle(QTextListFormat::ListUpperRoman);
      else
        listFormat.setStyle(QTextListFormat::ListDisc);

      const char* style = iupAttribGet(formattag, "NUMBERINGSTYLE");
      if (style)
      {
        if (iupStrEqualNoCase(style, "RIGHTPARENTHESIS"))
          listFormat.setNumberSuffix(QStringLiteral(")"));
        else if (iupStrEqualNoCase(style, "PARENTHESES"))
        {
          listFormat.setNumberPrefix(QStringLiteral("("));
          listFormat.setNumberSuffix(QStringLiteral(")"));
        }
        else if (iupStrEqualNoCase(style, "PERIOD"))
          listFormat.setNumberSuffix(QStringLiteral("."));
        else if (iupStrEqualNoCase(style, "NONUMBER"))
        {
          listFormat.setNumberPrefix(QString());
          listFormat.setNumberSuffix(QString());
        }
      }

      const char* numberingtab = iupAttribGet(formattag, "NUMBERINGTAB");
      if (numberingtab)
      {
        int tabval = 0;
        if (iupStrToInt(numberingtab, &tabval))
          listFormat.setIndent(tabval / 8);
      }

      cursor.createList(listFormat);
    }
  }
}

/****************************************************************************
 * Map Method
 ****************************************************************************/


static int qmlTextMapMethod(Ihandle* ih)
{
  QQuickItem* handle = nullptr;
  QQuickItem* edit = nullptr;

  if (ih->data->is_multiline)
  {
    QByteArray qml = IUPQML_IMPORTS "ScrollView { clip: true";
    int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
    qml += (ih->data->sb & IUP_SB_VERT) ? (autohide ? "; ScrollBar.vertical.policy: ScrollBar.vertical.size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff" : "; ScrollBar.vertical.policy: ScrollBar.AlwaysOn") : "; ScrollBar.vertical.policy: ScrollBar.AlwaysOff";
    qml += (ih->data->sb & IUP_SB_HORIZ) ? (autohide ? "; ScrollBar.horizontal.policy: ScrollBar.horizontal.size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff" : "; ScrollBar.horizontal.policy: ScrollBar.AlwaysOn") : "; ScrollBar.horizontal.policy: ScrollBar.AlwaysOff";
    qml += "\n  TextArea { objectName: \"edit\"; selectByMouse: true; persistentSelection: true";
    if (iupAttribGetBoolean(ih, "WORDWRAP"))
    {
      qml += "; wrapMode: TextEdit.Wrap";
      ih->data->sb &= ~IUP_SB_HORIZ;
    }
    else
      qml += "; wrapMode: TextEdit.NoWrap";
    qml += " }\n}";

    handle = iupqmlCreateItem(qml.constData());
    if (!handle)
      return IUP_ERROR;

    edit = handle->findChild<QQuickItem*>("edit");
  }
  else if (iupAttribGetBoolean(ih, "SPIN"))
  {
    handle = iupqmlCreateItem(IUPQML_IMPORTS
      "SpinBox { id: spin; editable: true; live: true; property bool iupAuto: true\n"
      "  textFromValue: function(value, locale) { return spin.iupAuto ? value.toString() : spin.contentItem.text }\n"
      "  valueFromText: function(text, locale) { return spin.iupAuto ? parseInt(text) : spin.value }\n"
      "  validator: spin.iupAuto ? iupIntValidator : null\n"
      "  IntValidator { id: iupIntValidator; bottom: Math.min(spin.from, spin.to); top: Math.max(spin.from, spin.to) }\n"
      "}");
    if (!handle)
      return IUP_ERROR;

    iupAttribSet(ih, "_IUPQML_SPINBOX", reinterpret_cast<char*>(handle));

    int min = iupAttribGetInt(ih, "SPINMIN");
    int max = iupAttribGetInt(ih, "SPINMAX");
    int inc = iupAttribGetInt(ih, "SPININC");
    if (max == 0) max = 100;
    if (inc == 0) inc = 1;

    handle->setProperty("from", min);
    handle->setProperty("to", max);
    handle->setProperty("stepSize", inc);
    handle->setProperty("wrap", iupAttribGetBoolean(ih, "SPINWRAP") ? true : false);
    handle->setProperty("iupAuto", iupAttribGetBoolean(ih, "SPINAUTO") ? true : false);

    const char* spinvalue = iupAttribGetStr(ih, "SPINVALUE");
    if (spinvalue)
    {
      int spinval = 0;
      iupStrToInt(spinvalue, &spinval);
      handle->setProperty("value", spinval);
    }

    iupqmlConnect(handle, "valueModified()", [ih, handle](void**) {
      if (ih->data->disable_callbacks)
        return;

      int value = handle->property("value").toInt();
      iupAttribSetInt(ih, "SPINVALUE", value);
      qmlTextValueChanged(ih);

      IFni spin_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SPIN_CB"));
      if (spin_cb)
        spin_cb(ih, value);
    });

    edit = iupqmlGetItemProperty(handle, "contentItem");
  }
  else
  {
    handle = iupqmlCreateItem(IUPQML_IMPORTS "TextField { selectByMouse: true }");
    if (!handle)
      return IUP_ERROR;
    edit = handle;
  }

  if (!edit)
    edit = handle;

  ih->handle = reinterpret_cast<InativeHandle*>(handle);
  iupAttribSet(ih, "_IUPQML_TEXT_EDIT", reinterpret_cast<char*>(edit));

  if (!qmlTextGetSpin(ih))
  {
    iupqmlConnect(edit, "textChanged()", [ih](void**) {
      qmlTextValueChanged(ih);
    });
    if (!ih->data->is_multiline)
    {
      iupqmlConnect(edit, "contentSizeChanged()", [ih](void**) {
        qmlTextFitLine(ih);
      });
      iupqmlConnect(edit, "heightChanged()", [ih](void**) {
        qmlTextFitLine(ih);
      });
    }
  }

  iupqmlConnect(edit, "cursorPositionChanged()", [ih](void**) {
    qmlTextCursorPositionChanged(ih);
  });

  if (ih->data->is_multiline)
  {
    iupqmlConnect(edit, "linkActivated(QString)", [ih](void** args) {
      qmlTextLinkActivated(ih, *static_cast<QString*>(args[1]));
    });
    iupqmlConnect(edit, "linkHovered(QString)", [](void**) {});
    qmlTextSetTabSizeAttrib(ih, iupAttribGetStr(ih, "TABSIZE"));
  }

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, edit);
  edit->setProperty("_iup_keys_handled", true);
  edit->installEventFilter(new IupQmlTextKeyFilter(edit, ih));

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(edit, 0);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  if (ih->data->formattags)
    iupTextUpdateFormatTags(ih);

  return IUP_NOERROR;
}

static void qmlTextUnMapMethod(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPQML_TEXT_EDIT", nullptr);
  iupAttribSet(ih, "_IUPQML_SPINBOX", nullptr);
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = qmlTextMapMethod;
  ic->UnMap = qmlTextUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlTextSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, qmlTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", qmlTextGetValueAttrib, qmlTextSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", qmlTextGetLineValueAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", qmlTextGetSelectedTextAttrib, qmlTextSetSelectedTextAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", qmlTextGetSelectionAttrib, qmlTextSetSelectionAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", qmlTextGetSelectionPosAttrib, qmlTextSetSelectionPosAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", qmlTextGetCaretAttrib, qmlTextSetCaretAttrib, nullptr, nullptr, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", qmlTextGetCaretPosAttrib, qmlTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", nullptr, qmlTextSetInsertAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", nullptr, qmlTextSetAppendAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", qmlTextGetReadOnlyAttrib, qmlTextSetReadOnlyAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", qmlTextGetNCAttrib, qmlTextSetNCAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", nullptr, qmlTextSetClipboardAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", nullptr, qmlTextSetScrollToAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", nullptr, qmlTextSetScrollToPosAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", nullptr, qmlTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", nullptr, qmlTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", nullptr, qmlTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", qmlTextGetSpinValueAttrib, qmlTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", qmlTextGetCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", qmlTextGetLineCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", nullptr, qmlTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASSWORD", qmlTextGetPasswordAttrib, qmlTextSetPasswordAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CUEBANNER", qmlTextGetCueBannerAttrib, qmlTextSetCueBannerAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", nullptr, qmlTextSetFilterAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", qmlTextGetOverwriteAttrib, qmlTextSetOverwriteAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", nullptr, qmlTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BORDER", qmlTextGetBorderAttrib, qmlTextSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "5", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", qmlTextGetScrollVisibleAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMATTAG", nullptr, iupTextSetAddFormatTagAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", nullptr, iupTextSetAddFormatTagHandleAttrib, nullptr, nullptr, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSARRAY", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", nullptr, qmlTextSetRemoveFormattingAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
}

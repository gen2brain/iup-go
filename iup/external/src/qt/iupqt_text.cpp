/** \file
 * \brief Text Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QWidget>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPalette>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QUrl>
#include <QCompleter>
#include <QStringListModel>
#include <QFrame>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_array.h"
#include "iup_text.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Custom Widgets with Event Handling
 ****************************************************************************/

/* Forward declarations */
static int qtTextKeyPress(Ihandle* ih, QKeyEvent* evt);

/* Custom QLineEdit with additional functionality */
class IupQtLineEdit : public QLineEdit
{
private:
  Ihandle* ih;
  bool overwrite_mode;

public:
  explicit IupQtLineEdit(Ihandle* ihandle) : QLineEdit(), ih(ihandle), overwrite_mode(false)
  {
    /* Accept drops for DROPFILES_CB */
    setAcceptDrops(true);
  }

  void setOverwriteMode(bool mode) { overwrite_mode = mode; }
  bool isOverwriteMode() const { return overwrite_mode; }

  /* Override sizeHint to return minimal size (like GTK's gtk_entry_set_width_chars(1))
   * This prevents Qt from using its default larger size hint, allowing IUP's
   * natural size calculation to control the widget size. */
  QSize sizeHint() const override
  {
    QFontMetrics fm(font());
    int h = fm.height() + 2;  /* Minimal height based on font */
    int w = fm.horizontalAdvance('X');  /* 1 character width minimum */
    return QSize(w, h);
  }

protected:
  void keyPressEvent(QKeyEvent* event) override
  {
    /* Handle INSERT key for overwrite mode */
    if (event->key() == Qt::Key_Insert && event->modifiers() == Qt::NoModifier)
    {
      overwrite_mode = !overwrite_mode;
      event->accept();
      return;
    }

    /* Validate input against mask before allowing text change */
    if (!event->text().isEmpty() && event->text().at(0).isPrint())
    {
      int start = cursorPosition();
      int end = start;

      if (hasSelectedText())
      {
        start = selectionStart();
        end = start + selectedText().length();
      }

      /* Call mask validation (ACTION callback handled separately by qtTextKeyPress) */
      int ret = iupEditCallActionCb(ih, NULL, event->text().toUtf8().constData(),
                                     start, end, ih->data->mask, ih->data->nc, 0, 1);
      if (ret == 0)
      {
        /* Mask validation failed - block the key */
        event->accept();
        return;
      }
    }

    /* Handle overwrite mode */
    if (overwrite_mode && !event->text().isEmpty() && !hasSelectedText())
    {
      int pos = cursorPosition();
      if (pos < text().length())
      {
        QString current = text();
        current.remove(pos, 1);
        setText(current);
        setCursorPosition(pos);
      }
    }

    /* Handle text key press event (includes ACTION callback for single-line) */
    if (qtTextKeyPress(ih, event))
    {
      event->accept();
      return;
    }

    QLineEdit::keyPressEvent(event);
  }

  void dragEnterEvent(QDragEnterEvent* event) override
  {
    if (IupGetCallback(ih, "DROPFILES_CB"))
    {
      if (event->mimeData()->hasUrls())
      {
        event->acceptProposedAction();
        return;
      }
    }
    QLineEdit::dragEnterEvent(event);
  }

  void dropEvent(QDropEvent* event) override
  {
    IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
    if (cb && event->mimeData()->hasUrls())
    {
      QList<QUrl> urls = event->mimeData()->urls();
      int count = urls.size();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      int x = event->position().x();
      int y = event->position().y();
#else
      int x = event->pos().x();
      int y = event->pos().y();
#endif

      QString fileList;
      for (int i = 0; i < count; i++)
      {
        if (i > 0) fileList += "\n";
        fileList += urls[i].toLocalFile();
      }

      QByteArray fileArray = fileList.toUtf8();
      cb(ih, (char*)fileArray.constData(), count, x, y);
      event->acceptProposedAction();
      return;
    }
    QLineEdit::dropEvent(event);
  }
};

/* Custom QTextEdit with additional functionality */
class IupQtTextEdit : public QTextEdit
{
private:
  Ihandle* ih;
  bool overwrite_mode;

public:
  explicit IupQtTextEdit(Ihandle* ihandle) : QTextEdit(), ih(ihandle), overwrite_mode(false)
  {
    setAcceptDrops(true);
  }

  void setOverwriteMode(bool mode) { overwrite_mode = mode; }
  bool isOverwriteMode() const { return overwrite_mode; }

protected:
  void keyPressEvent(QKeyEvent* event) override
  {
    /* Handle INSERT key for overwrite mode */
    if (event->key() == Qt::Key_Insert && event->modifiers() == Qt::NoModifier)
    {
      overwrite_mode = !overwrite_mode;
      setOverwriteCursor(overwrite_mode);
      event->accept();
      return;
    }

    /* Validate input against mask before allowing text change */
    if (!event->text().isEmpty() && event->text().at(0).isPrint())
    {
      QTextCursor cursor = textCursor();
      int start = cursor.position();
      int end = start;

      if (cursor.hasSelection())
      {
        int anchor = cursor.anchor();
        start = qMin(start, anchor);
        end = qMax(start, anchor);
      }

      /* Call mask validation (ACTION callback handled separately) */
      int ret = iupEditCallActionCb(ih, NULL, event->text().toUtf8().constData(),
                                     start, end, ih->data->mask, ih->data->nc, 0, 1);
      if (ret == 0)
      {
        /* Mask validation failed - block the key */
        event->accept();
        return;
      }
    }

    /* Handle overwrite mode */
    if (overwrite_mode && !event->text().isEmpty())
    {
      QTextCursor cursor = textCursor();
      if (!cursor.hasSelection() && !cursor.atEnd())
      {
        cursor.deleteChar();
        setTextCursor(cursor);
      }
    }

    /* Handle iupqt key press event for callbacks */
    if (iupqtKeyPressEvent(this, event, ih))
    {
      event->accept();
      return;
    }

    QTextEdit::keyPressEvent(event);
  }

  void dragEnterEvent(QDragEnterEvent* event) override
  {
    if (IupGetCallback(ih, "DROPFILES_CB"))
    {
      if (event->mimeData()->hasUrls())
      {
        event->acceptProposedAction();
        return;
      }
    }
    QTextEdit::dragEnterEvent(event);
  }

  void dropEvent(QDropEvent* event) override
  {
    IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
    if (cb && event->mimeData()->hasUrls())
    {
      QList<QUrl> urls = event->mimeData()->urls();
      int count = urls.size();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      int x = event->position().x();
      int y = event->position().y();
#else
      int x = event->pos().x();
      int y = event->pos().y();
#endif

      QString fileList;
      for (int i = 0; i < count; i++)
      {
        if (i > 0) fileList += "\n";
        fileList += urls[i].toLocalFile();
      }

      QByteArray fileArray = fileList.toUtf8();
      cb(ih, (char*)fileArray.constData(), count, x, y);
      event->acceptProposedAction();
      return;
    }
    QTextEdit::dropEvent(event);
  }

  void setOverwriteCursor(bool overwrite)
  {
    /* Could change cursor appearance for overwrite mode */
    /* Qt doesn't have a built-in overwrite cursor, but we track the mode */
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

extern "C" void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  static int spin_min_width = -1;

  (void)h;
  (void)ih;

  /* Measure the minimum width required by QSpinBox */
  if (spin_min_width < 0)
  {
    QSpinBox* temp_spin = new QSpinBox();
    temp_spin->setRange(0, 100);
    temp_spin->setSingleStep(1);

    QSize sizeHint = temp_spin->minimumSizeHint();
    spin_min_width = sizeHint.width();

    delete temp_spin;
  }

  /* Only enforce minimum width, don't force expansion */
  if (*w < spin_min_width)
    *w = spin_min_width;
}

extern "C" void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 2 * 5;
  (*x) += border_size;
  (*y) += border_size;
  (void)ih;
}

extern "C" void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextDocument* doc = text->document();

    lin--; /* IUP starts at 1 */
    col--;

    QTextBlock block = doc->findBlockByLineNumber(lin);
    if (block.isValid())
      *pos = block.position() + col;
    else
      *pos = 0;
  }
  else
    *pos = col - 1; /* single line, position is column */
}

extern "C" void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextDocument* doc = text->document();
    QTextBlock block = doc->findBlock(pos);

    if (block.isValid())
    {
      *lin = block.blockNumber() + 1; /* IUP starts at 1 */
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

static void qtTextValueChanged(Ihandle* ih)
{
  if (ih->data->disable_callbacks)
    return;

  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
}

static void qtTextCursorPositionChanged(Ihandle* ih)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "CARET_CB");
  if (cb)
  {
    int lin, col, pos;

    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      QTextCursor cursor = text->textCursor();
      pos = cursor.position();
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      pos = edit->cursorPosition();
    }

    iupdrvTextConvertPosToLinCol(ih, pos, &lin, &col);
    cb(ih, lin, col);
  }
}

static int qtTextKeyPress(Ihandle* ih, QKeyEvent* evt)
{
  int ret = iupqtKeyPressEvent((QWidget*)ih->handle, evt, ih);

  if (ret)
  {
    return 1;
  }

  /* Call ACTION callback for single-line */
  if (!ih->data->is_multiline)
  {
    IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      int key = evt->key();
      QString text = evt->text();

      /* Only call ACTION for printable characters, not control characters
       * Control characters (like backspace, delete, etc.) should be handled
       * by K_ANY callback and default behavior, not by ACTION callback. */
      if (!text.isEmpty())
      {
        QChar ch = text.at(0);
        /* Check if character is printable (not a control character) */
        if (ch.isPrint() && ch.unicode() >= 32)
        {
          int iup_key = ch.toLatin1();
          IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
          int pos = edit->cursorPosition();

          int result = cb(ih, iup_key, (char*)edit->text().toUtf8().constData());

          if (result == IUP_IGNORE)
          {
            return 1;  /* Block the key */
          }
          else if (result == IUP_CLOSE)
          {
            IupExitLoop();
            return 1;
          }
          else if (result != IUP_DEFAULT && result > 0)
          {
            /* Replace the typed character with the returned character
             * (e.g., return K_asterisk to show '*' for password input) */
            QString replacement = QString(QChar(result));

            /* Temporarily disable callbacks to prevent recursion */
            ih->data->disable_callbacks = 1;

            /* Insert the replacement character at cursor position */
            QString current = edit->text();
            current.insert(pos, replacement);
            edit->setText(current);
            edit->setCursorPosition(pos + 1);

            ih->data->disable_callbacks = 0;

            return 1;  /* Block the original key */
          }
        }
      }
    }
  }

  return 0;
}

/****************************************************************************
 * Attribute Setters/Getters
 ****************************************************************************/

static int qtTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";

  /* Safety check - widget must be mapped before setting value */
  if (!ih->handle)
    return 0;

  ih->data->disable_callbacks = 1;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    text->setPlainText(QString::fromUtf8(value));
  }
  else
  {
    /* Check if this is actually a QSpinBox (used for SPIN=YES attribute) */
    QSpinBox* spinbox = qobject_cast<QSpinBox*>((QWidget*)ih->handle);
    if (spinbox)
    {
      /* For spinbox, set the value as an integer */
      int int_value = 0;
      if (value && *value)
        sscanf(value, "%d", &int_value);
      spinbox->setValue(int_value);
    }
    else
    {
      /* Regular line edit */
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->setText(QString::fromUtf8(value));
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static char* qtTextGetValueAttrib(Ihandle* ih)
{
  QString value;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    value = text->toPlainText();
  }
  else
  {
    /* Check if this is a QSpinBox */
    QSpinBox* spinbox = qobject_cast<QSpinBox*>((QWidget*)ih->handle);
    if (spinbox)
    {
      /* Return spinbox value as string */
      return iupStrReturnInt(spinbox->value());
    }
    else
    {
      /* Regular line edit */
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      value = edit->text();
    }
  }

  return iupStrReturnStr(value.toUtf8().constData());
}

static char* qtTextGetLineValueAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    QTextBlock block = cursor.block();
    return iupStrReturnStr(block.text().toUtf8().constData());
  }
  else
    return qtTextGetValueAttrib(ih);
}

static int qtTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  ih->data->disable_callbacks = 1;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    text->insertPlainText(QString::fromUtf8(value));
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->insert(QString::fromUtf8(value));
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static char* qtTextGetSelectedTextAttrib(Ihandle* ih)
{
  QString value;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    value = cursor.selectedText();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    value = edit->selectedText();
  }

  return iupStrReturnStr(value.toUtf8().constData());
}

static int qtTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start = 1, end = 1;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      QTextCursor cursor = text->textCursor();
      cursor.clearSelection();
      text->setTextCursor(cursor);
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->deselect();
    }
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      text->selectAll();
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->selectAll();
    }
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 1) start = 1;
  if (end < 1) end = 1;

  start--; /* IUP starts at 1 */
  end--;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    text->setTextCursor(cursor);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setSelection(start, end - start);
  }

  return 0;
}

static char* qtTextGetSelectionAttrib(Ihandle* ih)
{
  int start, end;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    start = cursor.selectionStart();
    end = cursor.selectionEnd();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    start = edit->selectionStart();
    if (start < 0)
      return nullptr;
    end = start + edit->selectedText().length();
  }

  start++; /* IUP starts at 1 */
  end++;

  return iupStrReturnStrf("%d:%d", start, end);
}

static int qtTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start = 0, end = 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      QTextCursor cursor = text->textCursor();
      cursor.clearSelection();
      text->setTextCursor(cursor);
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->deselect();
    }
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      text->selectAll();
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->selectAll();
    }
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 0) start = 0;
  if (end < 0) end = 0;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    text->setTextCursor(cursor);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setSelection(start, end - start);
  }

  return 0;
}

static char* qtTextGetSelectionPosAttrib(Ihandle* ih)
{
  int start, end;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    start = cursor.selectionStart();
    end = cursor.selectionEnd();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    start = edit->selectionStart();
    if (start < 0)
      return nullptr;
    end = start + edit->selectedText().length();
  }

  return iupStrReturnStrf("%d:%d", start, end);
}

static int qtTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int lin = 1, col = 1, pos;

  iupStrToIntInt(value, &lin, &col, ',');
  iupdrvTextConvertLinColToPos(ih, lin, col, &pos);

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    cursor.setPosition(pos);
    text->setTextCursor(cursor);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setCursorPosition(pos);
  }

  return 0;
}

static char* qtTextGetCaretAttrib(Ihandle* ih)
{
  int lin, col, pos;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    pos = cursor.position();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    pos = edit->cursorPosition();
  }

  iupdrvTextConvertPosToLinCol(ih, pos, &lin, &col);
  return iupStrReturnStrf("%d,%d", lin, col);
}

static int qtTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    cursor.setPosition(pos);
    text->setTextCursor(cursor);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setCursorPosition(pos);
  }

  return 0;
}

static char* qtTextGetCaretPosAttrib(Ihandle* ih)
{
  int pos;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    pos = cursor.position();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    pos = edit->cursorPosition();
  }

  return iupStrReturnInt(pos);
}

static int qtTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int lin = 1, col = 1, pos;

  if (!value)
    return 0;

  iupStrToIntInt(value, &lin, &col, ',');
  if (lin < 1) lin = 1;
  if (col < 1) col = 1;

  iupdrvTextConvertLinColToPos(ih, lin, col, &pos);

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    cursor.setPosition(pos);
    text->setTextCursor(cursor);
    text->ensureCursorVisible();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setCursorPosition(pos);
  }

  return 0;
}

static int qtTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    QTextCursor cursor = text->textCursor();
    cursor.setPosition(pos);
    text->setTextCursor(cursor);
    text->ensureCursorVisible();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setCursorPosition(pos);
  }

  return 0;
}

static int qtTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;
  if (!value)
    return 0;

  ih->data->disable_callbacks = 1;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    text->insertPlainText(QString::fromUtf8(value));
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->insert(QString::fromUtf8(value));
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static int qtTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  ih->data->disable_callbacks = 1;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;

    if (ih->data->append_newline && !text->toPlainText().isEmpty())
      text->append("\n" + QString::fromUtf8(value));
    else
      text->append(QString::fromUtf8(value));
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setText(edit->text() + QString::fromUtf8(value));
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static int qtTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    text->setReadOnly(iupStrBoolean(value));
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setReadOnly(iupStrBoolean(value));
  }

  return 0;
}

static char* qtTextGetReadOnlyAttrib(Ihandle* ih)
{
  bool readonly;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    readonly = text->isReadOnly();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    readonly = edit->isReadOnly();
  }

  return iupStrReturnBoolean(readonly);
}

static int qtTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
  {
    int max;
    if (iupStrToInt(value, &max))
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->setMaxLength(max);
    }
  }

  return 0;
}

static char* qtTextGetNCAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline)
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    return iupStrReturnInt(edit->maxLength());
  }

  return nullptr;
}

static int qtTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      text->copy();
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->copy();
    }
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      text->cut();
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->cut();
    }
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      text->paste();
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      edit->paste();
    }
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    if (ih->data->is_multiline)
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
      QTextCursor cursor = text->textCursor();
      if (cursor.hasSelection())
        cursor.removeSelectedText();
      else
        text->clear();
    }
    else
    {
      IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
      if (edit->hasSelectedText())
        edit->del();
      else
        edit->clear();
    }
  }

  return 0;
}

static int qtTextSetPasswordAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    if (iupStrBoolean(value))
      edit->setEchoMode(QLineEdit::Password);
    else
      edit->setEchoMode(QLineEdit::Normal);
  }

  return 0;
}

static int qtTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  Qt::Alignment align = Qt::AlignLeft;

  if (iupStrEqualNoCase(value, "ARIGHT"))
    align = Qt::AlignRight;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    align = Qt::AlignHCenter;
  else /* "ALEFT" */
    align = Qt::AlignLeft;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    text->setAlignment(align);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setAlignment(align);
  }

  return 1;
}

static char* qtTextGetCountAttrib(Ihandle* ih)
{
  int count;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    count = text->toPlainText().length();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    count = edit->text().length();
  }

  return iupStrReturnInt(count);
}

static char* qtTextGetLineCountAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    return iupStrReturnInt(text->document()->blockCount());
  }

  return nullptr;
}

static int qtTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    if (value)
      edit->setPlaceholderText(QString::fromUtf8(value));
    else
      edit->setPlaceholderText(QString());
  }

  return 1;
}

static char* qtTextGetCueBannerAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline)
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    QString text = edit->placeholderText();
    if (!text.isEmpty())
      return iupStrReturnStr(text.toUtf8().constData());
  }

  return nullptr;
}

static int qtTextSetFilterAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline && value)
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;

    /* Create completer with filter list */
    QStringList filterList;
    QString filterStr = QString::fromUtf8(value);
    QStringList items = filterStr.split(';', Qt::SkipEmptyParts);

    for (const QString& item : items)
      filterList << item.trimmed();

    QCompleter* completer = new QCompleter(filterList, edit);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    edit->setCompleter(completer);
  }

  return 1;
}

static int qtTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  bool overwrite = iupStrBoolean(value);

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    text->setOverwriteMode(overwrite);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    edit->setOverwriteMode(overwrite);
  }

  return 0;
}

static char* qtTextGetOverwriteAttrib(Ihandle* ih)
{
  bool overwrite;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    overwrite = text->isOverwriteMode();
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    overwrite = edit->isOverwriteMode();
  }

  return iupStrReturnBoolean(overwrite);
}

static int qtTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    int tabsize = 8;
    iupStrToInt(value, &tabsize);

    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;

    /* Calculate tab stop in pixels based on font */
    QFontMetrics metrics(text->font());
    int tabStopWidth = tabsize * metrics.horizontalAdvance(' ');

    text->setTabStopDistance(tabStopWidth);
  }

  return 1;
}

static int qtTextSetBorderAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    if (iupStrBoolean(value))
      text->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    else
      text->setFrameStyle(QFrame::NoFrame);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    if (iupStrBoolean(value))
      edit->setFrame(true);
    else
      edit->setFrame(false);
  }

  return 1;
}

static int qtTextSetVisibleColumnsAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
  {
    /* Like GTK's gtk_entry_set_width_chars(1), set minimum width to 1 character.
     * VISIBLECOLUMNS is used by IUP's platform-independent code to calculate
     * natural size, but Qt's widget minimum should not constrain IUP's layout. */
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    QFontMetrics metrics(edit->font());
    int min_width = metrics.horizontalAdvance('X');  /* 1 character minimum */
    edit->setMinimumWidth(min_width);
  }

  return 1;
}

static int qtTextSetVisibleLinesAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    int lines;
    if (iupStrToInt(value, &lines))
    {
      IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;

      /* Calculate height based on font metrics */
      QFontMetrics metrics(text->font());
      int height = lines * metrics.lineSpacing();
      text->setMinimumHeight(height);
    }
  }

  return 1;
}

static char* qtTextGetScrollVisibleAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;

    int horiz_visible = 0, vert_visible = 0;

    QScrollBar* hsb = text->horizontalScrollBar();
    QScrollBar* vsb = text->verticalScrollBar();

    if (hsb && hsb->isVisible())
      horiz_visible = 1;
    if (vsb && vsb->isVisible())
      vert_visible = 1;

    if (horiz_visible && vert_visible)
      return (char*)"YES";
    else if (horiz_visible)
      return (char*)"HORIZONTAL";
    else if (vert_visible)
      return (char*)"VERTICAL";
    else
      return (char*)"NO";
  }

  return nullptr;
}

static char* qtTextGetPasswordAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline)
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    return iupStrReturnBoolean(edit->echoMode() == QLineEdit::Password);
  }

  return nullptr;
}

static char* qtTextGetBorderAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = (IupQtTextEdit*)ih->handle;
    return iupStrReturnBoolean(text->frameStyle() != QFrame::NoFrame);
  }
  else
  {
    IupQtLineEdit* edit = (IupQtLineEdit*)ih->handle;
    return iupStrReturnBoolean(edit->hasFrame());
  }
}

/****************************************************************************
 * UnMap Method
 ****************************************************************************/

static void qtTextUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    QWidget* widget = (QWidget*)ih->handle;

    /* Destroy tooltip if any */
    iupqtTipsDestroy(ih);

    /* Delete the widget - Qt will automatically disconnect signals */
    delete widget;
    ih->handle = nullptr;
  }
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtTextMapMethod(Ihandle* ih)
{
  QWidget* widget = nullptr;

  if (ih->data->is_multiline)
  {
    IupQtTextEdit* text = new IupQtTextEdit(ih);

    /* Configure multiline text */
    text->setAcceptRichText(false);
    text->setLineWrapMode(iupAttribGetBoolean(ih, "WORDWRAP") ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
    text->setTabStopDistance(40);

    /* Vertical scrollbar */
    if (ih->data->sb & IUP_SB_VERT)
      text->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    else
      text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /* Horizontal scrollbar */
    if (ih->data->sb & IUP_SB_HORIZ)
      text->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    else
      text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /* Connect signals */
    QObject::connect(text, &QTextEdit::textChanged, [ih]() {
      qtTextValueChanged(ih);
    });

    QObject::connect(text, &QTextEdit::cursorPositionChanged, [ih]() {
      qtTextCursorPositionChanged(ih);
    });

    widget = text;
  }
  else if (iupAttribGetBoolean(ih, "SPIN"))
  {
    /* Create spin box for numeric input with spin buttons */
    QSpinBox* spin = new QSpinBox();

    /* Set range from attributes */
    int min = iupAttribGetInt(ih, "SPINMIN");
    int max = iupAttribGetInt(ih, "SPINMAX");
    int inc = iupAttribGetInt(ih, "SPININC");

    if (max == 0) max = 100;
    if (inc == 0) inc = 1;

    spin->setRange(min, max);
    spin->setSingleStep(inc);

    /* Set initial value */
    const char* spinvalue = iupAttribGetStr(ih, "SPINVALUE");
    if (spinvalue)
      spin->setValue(atoi(spinvalue));

    /* Connect signal */
    QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), [ih, spin](int value) {
      iupAttribSetInt(ih, "SPINVALUE", value);
      qtTextValueChanged(ih);
    });

    widget = spin;
  }
  else
  {
    IupQtLineEdit* edit = new IupQtLineEdit(ih);

    /* The sizeHint() override in IupQtLineEdit returns minimal size (1 character width)
     * to match GTK's gtk_entry_set_width_chars(1) behavior. */

    /* Connect signals */
    QObject::connect(edit, &QLineEdit::textChanged, [ih]() {
      qtTextValueChanged(ih);
    });

    QObject::connect(edit, &QLineEdit::cursorPositionChanged, [ih]() {
      qtTextCursorPositionChanged(ih);
    });

    widget = edit;
  }

  if (!widget)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)widget;

  iupqtAddToParent(ih);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    if (ih->data->is_multiline)
    {
      QTextEdit* text = (QTextEdit*)ih->handle;
      /* Set document margin for padding */
      text->document()->setDocumentMargin(ih->data->horiz_padding);
      /* Note: Qt's QTextEdit doesn't support separate vertical padding easily */
      ih->data->vert_padding = 0;
    }
    else
    {
      QLineEdit* edit = (QLineEdit*)ih->handle;
      /* Set text margins for padding */
      edit->setTextMargins(ih->data->horiz_padding, ih->data->vert_padding,
                           ih->data->horiz_padding, ih->data->vert_padding);
    }
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtTextMapMethod;
  ic->UnMap = qtTextUnMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, qtTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", qtTextGetValueAttrib, qtTextSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", qtTextGetLineValueAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", qtTextGetSelectedTextAttrib, qtTextSetSelectedTextAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", qtTextGetSelectionAttrib, qtTextSetSelectionAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", qtTextGetSelectionPosAttrib, qtTextSetSelectionPosAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", qtTextGetCaretAttrib, qtTextSetCaretAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", qtTextGetCaretPosAttrib, qtTextSetCaretPosAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", nullptr, qtTextSetInsertAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", nullptr, qtTextSetAppendAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", qtTextGetReadOnlyAttrib, qtTextSetReadOnlyAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", qtTextGetNCAttrib, qtTextSetNCAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", nullptr, qtTextSetClipboardAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", nullptr, qtTextSetScrollToAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", nullptr, qtTextSetScrollToPosAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", qtTextGetCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", qtTextGetLineCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", nullptr, qtTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASSWORD", qtTextGetPasswordAttrib, qtTextSetPasswordAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  /* Additional attributes */
  iupClassRegisterAttribute(ic, "CUEBANNER", qtTextGetCueBannerAttrib, qtTextSetCueBannerAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", nullptr, qtTextSetFilterAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", qtTextGetOverwriteAttrib, qtTextSetOverwriteAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", nullptr, qtTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BORDER", qtTextGetBorderAttrib, qtTextSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", nullptr, qtTextSetVisibleColumnsAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", nullptr, qtTextSetVisibleLinesAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", qtTextGetScrollVisibleAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  /* Rich text formatting (Qt, GTK and Windows) */
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", nullptr, iupTextSetAddFormatTagAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", nullptr, iupTextSetAddFormatTagHandleAttrib, nullptr, nullptr, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
}

/* Format tag functions for rich text support */

/* Helper function to parse SELECTIONPOS attribute (e.g., "7:24") */
static bool qtTextParseSelectionPos(const char* selectionpos, int* start, int* end)
{
  if (!selectionpos)
    return false;

  if (sscanf(selectionpos, "%d:%d", start, end) == 2)
    return true;

  return false;
}

extern "C" void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  /* Qt formatting operations are efficient, but we can disable undo/redo
   * during bulk operations for better performance */
  if (!ih->data->is_multiline)
    return NULL;

  QTextEdit* text = (QTextEdit*)ih->handle;
  if (!text)
    return NULL;

  /* Store whether undo/redo was enabled */
  bool* undo_enabled = new bool(text->isUndoRedoEnabled());
  text->setUndoRedoEnabled(false);

  return (void*)undo_enabled;
}

extern "C" void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  if (!ih->data->is_multiline || !state)
    return;

  QTextEdit* text = (QTextEdit*)ih->handle;
  if (!text)
    return;

  /* Restore undo/redo state */
  bool* undo_enabled = (bool*)state;
  text->setUndoRedoEnabled(*undo_enabled);
  delete undo_enabled;
}

extern "C" void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  (void)bulk;

  if (!ih->data->is_multiline)
    return;

  QTextEdit* text = (QTextEdit*)ih->handle;
  if (!text)
    return;

  /* Get the selection range from the format tag */
  const char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
  if (!selectionpos)
    return;

  int start_pos, end_pos;
  if (!qtTextParseSelectionPos(selectionpos, &start_pos, &end_pos))
    return;

  /* Create a cursor for the specified range */
  QTextCursor cursor = text->textCursor();
  cursor.setPosition(start_pos);
  cursor.setPosition(end_pos, QTextCursor::KeepAnchor);

  /* Create character format */
  QTextCharFormat format;

  /* Apply BGCOLOR if specified */
  const char* bgcolor = iupAttribGet(formattag, "BGCOLOR");
  if (bgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(bgcolor, &r, &g, &b))
    {
      format.setBackground(QColor(r, g, b));
    }
  }

  /* Apply FGCOLOR if specified */
  const char* fgcolor = iupAttribGet(formattag, "FGCOLOR");
  if (fgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(fgcolor, &r, &g, &b))
    {
      format.setForeground(QColor(r, g, b));
    }
  }

  /* Apply UNDERLINE if specified */
  const char* underline = iupAttribGet(formattag, "UNDERLINE");
  if (underline && iupStrBoolean(underline))
  {
    format.setFontUnderline(true);
  }

  /* Apply ITALIC if specified */
  const char* italic = iupAttribGet(formattag, "ITALIC");
  if (italic && iupStrBoolean(italic))
  {
    format.setFontItalic(true);
  }

  /* Apply BOLD/WEIGHT if specified */
  const char* bold = iupAttribGet(formattag, "BOLD");
  if (bold && iupStrBoolean(bold))
  {
    format.setFontWeight(QFont::Bold);
  }
  else
  {
    const char* weight = iupAttribGet(formattag, "WEIGHT");
    if (weight)
    {
      if (iupStrEqualNoCase(weight, "EXTRALIGHT"))
        format.setFontWeight(QFont::ExtraLight);
      else if (iupStrEqualNoCase(weight, "LIGHT"))
        format.setFontWeight(QFont::Light);
      else if (iupStrEqualNoCase(weight, "SEMIBOLD"))
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        format.setFontWeight(QFont::DemiBold);
#else
        format.setFontWeight(QFont::DemiBold);
#endif
      else if (iupStrEqualNoCase(weight, "BOLD"))
        format.setFontWeight(QFont::Bold);
      else if (iupStrEqualNoCase(weight, "HEAVY"))
        format.setFontWeight(QFont::Black);
      else  /* "NORMAL" */
        format.setFontWeight(QFont::Normal);
    }
  }

  /* Apply STRIKEOUT if specified */
  const char* strikeout = iupAttribGet(formattag, "STRIKEOUT");
  if (strikeout && iupStrBoolean(strikeout))
  {
    format.setFontStrikeOut(true);
  }

  /* Apply the format to the selected range */
  cursor.mergeCharFormat(format);
}

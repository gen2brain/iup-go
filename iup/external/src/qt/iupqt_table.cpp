/** \file
 * \brief Table Control - Qt driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <QShowEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyleOptionFocusRect>
#include <QStyleOptionViewItem>
#include <QFontMetrics>
#include <QLineEdit>
#include <QMimeData>
#include <QDrag>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMouseEvent>

#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_table.h"
}

#include "iupqt_drv.h"


static void qtTableConfigureItem(Ihandle* ih, QTableWidgetItem* item, int lin, int col);
static void qtTableApplyCellColors(Ihandle* ih, QTableWidgetItem* item, int lin, int col);

/****************************************************************************
 * Table cell item with natural, numeric-aware sorting
 ****************************************************************************/

class IupQtTableItem : public QTableWidgetItem
{
public:
  bool operator<(const QTableWidgetItem& other) const override
  {
    return iupStrCompare(text().toUtf8().constData(), other.text().toUtf8().constData(), 0, 1) < 0;
  }
};

/****************************************************************************
 * Custom QLineEdit with fixed size hints
 ****************************************************************************/

class IupQtFixedLineEdit : public QLineEdit
{
private:
  QSize m_fixedSize;

public:
  IupQtFixedLineEdit(QWidget* parent = nullptr) : QLineEdit(parent), m_fixedSize(179, 29) {}

  void setFixedSizeHint(const QSize& size) { m_fixedSize = size; }

  QSize sizeHint() const override {
    return m_fixedSize;
  }

  QSize minimumSizeHint() const override {
    return m_fixedSize;
  }
};

/****************************************************************************
 * Virtual Mode Delegate
 ****************************************************************************/

class IupQtTableDelegate : public QStyledItemDelegate
{
private:
  Ihandle* ih;

public:
  explicit IupQtTableDelegate(Ihandle* ih_param, QObject* parent = nullptr)
    : QStyledItemDelegate(parent), ih(ih_param) {}

  QString displayText(const QVariant& value, const QLocale& locale) const override
  {
    (void)locale;

    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (!iupStrBoolean(virtualmode))
      return QStyledItemDelegate::displayText(value, locale);

    return value.toString();
  }

  /* colors are read live, so a change needs no re-baked QTableWidgetItem brushes */
  void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
  {
    QStyledItemDelegate::initStyleOption(option, index);

    int lin = index.row() + 1;
    int col = index.column() + 1;

    char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);
    if (!bgcolor && iupStrBoolean(iupAttribGet(ih, "ALTERNATECOLOR")))
      bgcolor = iupAttribGet(ih, (lin % 2 == 0) ? "EVENROWCOLOR" : "ODDROWCOLOR");

    if (bgcolor && *bgcolor)
    {
      unsigned char r, g, b;
      if (iupStrToRGB(bgcolor, &r, &g, &b))
        option->backgroundBrush = QBrush(QColor(r, g, b));
    }

    char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
    if (!fgcolor)
      fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);
    if (!fgcolor)
      fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);

    if (fgcolor && *fgcolor)
    {
      unsigned char r, g, b;
      if (iupStrToRGB(fgcolor, &r, &g, &b))
        option->palette.setColor(QPalette::Text, QColor(r, g, b));
    }
  }

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    bool hasFocus = (option.state & QStyle::State_HasFocus);

    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;

    QStyledItemDelegate::paint(painter, opt, index);

    if (hasFocus && iupAttribGetBoolean(ih, "FOCUSRECT"))
    {
      QStyleOptionFocusRect focusOption;
      focusOption.rect = option.rect.adjusted(1, 1, -1, -1);
      focusOption.state = option.state | QStyle::State_KeyboardFocusChange;
      focusOption.backgroundColor = option.palette.color(QPalette::Highlight);

      QStyle* style = QApplication::style();
      style->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, painter);

#ifdef __APPLE__
      QPen oldPen = painter->pen();
      painter->setPen(QPen(option.palette.color(QPalette::WindowText), 1, Qt::DotLine));
      painter->drawRect(focusOption.rect);
      painter->setPen(oldPen);
#endif
    }
  }

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    int lin = index.row() + 1;  /* 1-based */
    int col = index.column() + 1;  /* 1-based */

    IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
    if (editbegin_cb)
    {
      int ret = editbegin_cb(ih, lin, col);
      if (ret == IUP_IGNORE)
        return nullptr;
    }

    IupQtFixedLineEdit* fixedLineEdit = new IupQtFixedLineEdit(parent);
    fixedLineEdit->setFrame(false);
    fixedLineEdit->setTextMargins(0, 0, 0, 0);
    fixedLineEdit->setContentsMargins(0, 0, 0, 0);

    fixedLineEdit->installEventFilter(const_cast<IupQtTableDelegate*>(this));
    fixedLineEdit->setProperty("iup_row", lin);
    fixedLineEdit->setProperty("iup_col", col);
    fixedLineEdit->setProperty("iup_fixed_lineedit_ptr", QVariant::fromValue((void*)fixedLineEdit));

    return fixedLineEdit;
  }

  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    if (!editor)
      return;

    QRect editorRect = option.rect;

    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit)
    {
      /* setTextMargins() calls updateGeometry(), so the fixed size has to come after it */
      lineEdit->setTextMargins(0, 0, 0, 0);
    }

    editor->setGeometry(editorRect);

    if (lineEdit)
    {
      QVariant ptrVariant = lineEdit->property("iup_fixed_lineedit_ptr");
      if (ptrVariant.isValid())
      {
        IupQtFixedLineEdit* fixedLineEdit = static_cast<IupQtFixedLineEdit*>(ptrVariant.value<void*>());
        if (fixedLineEdit)
        {
          fixedLineEdit->setFixedSizeHint(QSize(editorRect.width(), editorRect.height()));
        }
      }

      lineEdit->setFixedSize(editorRect.width(), editorRect.height());
    }
  }

protected:
  bool eventFilter(QObject* object, QEvent* event) override
  {
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Escape)
      {
        QWidget* editor = qobject_cast<QWidget*>(object);
        if (editor)
        {
          int lin = editor->property("iup_row").toInt();
          int col = editor->property("iup_col").toInt();

          QVariant value = editor->property("text");
          QString current_text = value.toString();

          IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
          if (editend_cb)
          {
            editend_cb(ih, lin, col, (char*)current_text.toUtf8().constData(), 0);  /* 0 = cancelled */
          }
        }
      }
    }
    return QStyledItemDelegate::eventFilter(object, event);
  }

public:
  void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
  {
    int lin = index.row() + 1;  /* 1-based */
    int col = index.column() + 1;  /* 1-based */

    QVariant value = editor->property("text");
    QString new_text = value.toString();

    IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
    if (editend_cb)
    {
      int ret = editend_cb(ih, lin, col, (char*)new_text.toUtf8().constData(), 1);  /* 1 = accepted */
      if (ret == IUP_IGNORE)
        return;
    }

    QStyledItemDelegate::setModelData(editor, model, index);
  }
};

/****************************************************************************
 * Custom Table Widget
 ****************************************************************************/

class IupQtTableWidget : public QTableWidget
{
private:
  Ihandle* ih;
  bool firstShow;
  QPoint press_pos;  /* drag hotspot, viewport-relative */

public:
  explicit IupQtTableWidget(Ihandle* ih_param, QWidget* parent = nullptr)
    : QTableWidget(parent), ih(ih_param), firstShow(true)
  {
    /* signals stay blocked until the first focus, so populating cells fires no VALUECHANGED_CB */
    blockSignals(true);
    setupCallbacks();
  }

  void enableChangeNotifications()
  {
    firstShow = false;
    blockSignals(false);
  }

  void populateVirtualCells(int firstRow, int lastRow, int firstCol, int lastCol)
  {
    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (!iupStrBoolean(virtualmode))
      return;

    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (!value_cb)
      return;

    bool wasBlocked = signalsBlocked();
    blockSignals(true);

    for (int row = firstRow; row <= lastRow && row < rowCount(); row++)
    {
      for (int col = firstCol; col <= lastCol && col < columnCount(); col++)
      {
        QTableWidgetItem* existingItem = item(row, col);
        if (!existingItem)
        {
          existingItem = new IupQtTableItem();
          setItem(row, col, existingItem);
        }

        char* value = value_cb(ih, row + 1, col + 1);
        if (value)
        {
          existingItem->setText(QString::fromUtf8(value));
        }
        else
        {
          existingItem->setText(QString());
        }

        if (ih->data->show_image)
        {
          char* image_name = iupTableGetCellImageCb(ih, row + 1, col + 1);
          if (image_name)
          {
            QPixmap* pixImage = (QPixmap*)iupImageGetImage(image_name, ih, 0, NULL);
            if (pixImage)
            {
              if (ih->data->fit_image)
              {
                int charheight;
                iupdrvFontGetCharSize(ih, NULL, &charheight);
                int available_height = charheight + 4;
                if (pixImage->height() > available_height)
                {
                  QPixmap scaled = pixImage->scaledToHeight(available_height, Qt::SmoothTransformation);
                  existingItem->setIcon(QIcon(scaled));
                }
                else
                  existingItem->setIcon(QIcon(*pixImage));
              }
              else
                existingItem->setIcon(QIcon(*pixImage));
            }
            else
              existingItem->setIcon(QIcon());
          }
          else
            existingItem->setIcon(QIcon());
        }

        qtTableConfigureItem(ih, existingItem, row + 1, col + 1);
      }
    }

    blockSignals(wasBlocked);
  }

  QSize sizeHint() const override
  {
    QFontMetrics fm(font());
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int charWidth = fm.horizontalAdvance('X');
#else
    int charWidth = fm.width('X');
#endif
    int charHeight = fm.height();

    int w = charWidth * 10;
    int h = charHeight * 3;

    return QSize(w, h);
  }

  QSize minimumSizeHint() const override
  {
    QFontMetrics fm(font());
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int charWidth = fm.horizontalAdvance('X');
#else
    int charWidth = fm.width('X');
#endif
    int charHeight = fm.height();

    return QSize(charWidth * 5, charHeight * 2);
  }

protected:
  void showEvent(QShowEvent* event) override
  {
    QTableWidget::showEvent(event);

    updateVirtualCells();
  }

  void focusInEvent(QFocusEvent* event) override
  {
    if (firstShow && signalsBlocked())
    {
      enableChangeNotifications();
    }
    QTableWidget::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, ih);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QTableWidget::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, ih);
  }

  void scrollContentsBy(int dx, int dy) override
  {
    QTableWidget::scrollContentsBy(dx, dy);
    updateVirtualCells();
  }

  void resizeEvent(QResizeEvent* event) override
  {
    QTableWidget::resizeEvent(event);
    updateVirtualCells();
  }

  QColor rowFillerColor(int row)  /* 0-based */
  {
    if (selectionModel() && selectionModel()->isRowSelected(row, QModelIndex()))
      return palette().color(QPalette::Highlight);

    int lin = row + 1;
    char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);
    if (!bgcolor && iupStrBoolean(iupAttribGet(ih, "ALTERNATECOLOR")))
      bgcolor = (lin % 2 == 0) ? iupAttribGet(ih, "EVENROWCOLOR") : iupAttribGet(ih, "ODDROWCOLOR");
    if (!bgcolor)
      bgcolor = iupAttribGet(ih, "BGCOLOR");

    unsigned char r, g, b;
    if (bgcolor && *bgcolor && iupStrToRGB(bgcolor, &r, &g, &b))
      return QColor(r, g, b);
    return palette().color(QPalette::Base);
  }

  void paintEvent(QPaintEvent* event) override
  {
    QTableWidget::paintEvent(event);

    int cols = columnCount(), rows = rowCount();
    if (cols == 0 || rows == 0)
      return;

    int rightEdge = columnViewportPosition(cols - 1) + columnWidth(cols - 1);
    int vw = viewport()->width(), vh = viewport()->height();
    if (rightEdge >= vw)
      return;

    QStyleOptionViewItem opt;
    opt.initFrom(this);
    int gridHint = style()->styleHint(QStyle::SH_Table_GridLineColor, &opt, this);
    QPen gridPen(QColor::fromRgba(static_cast<QRgb>(gridHint)), 1, gridStyle());

    QPainter painter(viewport());
    for (int r = 0; r < rows; r++)
    {
      int ry = rowViewportPosition(r), rh = rowHeight(r);
      if (ry + rh <= 0 || ry >= vh)
        continue;

      painter.fillRect(rightEdge, ry, vw - rightEdge, rh, rowFillerColor(r));
      if (showGrid())
      {
        painter.setPen(gridPen);
        painter.drawLine(rightEdge, ry + rh - 1, vw, ry + rh - 1);
      }
    }
  }

  /* Qt invalidates only the cell rects on selection change; repaint the filler too. */
  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override
  {
    QTableWidget::selectionChanged(selected, deselected);
    viewport()->update();
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (iupqtKeyPressEvent(this, event, ih))
    {
      event->accept();
      return;
    }

    if (event->matches(QKeySequence::Copy))
    {
      copySelection();
      event->accept();
      return;
    }
    else if (event->matches(QKeySequence::Paste))
    {
      pasteToSelection();
      event->accept();
      return;
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
      QModelIndex index = currentIndex();
      if (index.isValid() && (model()->flags(index) & Qt::ItemIsEditable))
      {
        edit(index, QAbstractItemView::EditKeyPressed, event);
        event->accept();
        return;
      }
    }

    QTableWidget::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    if (event->button() == Qt::LeftButton)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      press_pos = event->position().toPoint();
#else
      press_pos = event->pos();
#endif
    }
    QTableWidget::mousePressEvent(event);
  }

  void startDrag(Qt::DropActions /*supportedActions*/) override
  {
    int row = currentRow();
    if (row < 0)
      return;

    iupAttribSetInt(ih, "_IUPTABLE_DRAGITEM", row + 1);  /* 1-based for the drop side */

    QMimeData* data = new QMimeData();
    data->setData("application/x-iup-table-row", QByteArray::number(row));

    QDrag* drag = new QDrag(this);
    drag->setMimeData(data);

    QRect rect = visualRect(model()->index(row, 0));
    rect.setRight(viewport()->rect().right());
    if (!rect.isEmpty())
    {
      drag->setPixmap(viewport()->grab(rect));
      drag->setHotSpot(press_pos - rect.topLeft());
    }

    drag->exec(Qt::MoveAction, Qt::MoveAction);

    iupAttribSet(ih, "_IUPTABLE_DRAGITEM", NULL);
  }

  void dragEnterEvent(QDragEnterEvent* event) override
  {
    if (event->mimeData()->hasFormat("application/x-iup-table-row"))
      event->acceptProposedAction();
    else
      QTableWidget::dragEnterEvent(event);
  }

  void dragMoveEvent(QDragMoveEvent* event) override
  {
    if (event->mimeData()->hasFormat("application/x-iup-table-row"))
      event->acceptProposedAction();
    else
      QTableWidget::dragMoveEvent(event);
  }

  void dropEvent(QDropEvent* event) override
  {
    int drag_id = iupAttribGetInt(ih, "_IUPTABLE_DRAGITEM");  /* 1-based */
    if (drag_id < 1)
    {
      event->ignore();
      return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint drop_pos = event->position().toPoint();
#else
    QPoint drop_pos = event->pos();
#endif
    QModelIndex drop_index = indexAt(drop_pos);
    int drop_id;
    if (drop_index.isValid())
    {
      drop_id = drop_index.row() + 1;  /* 1-based */
      QRect rect = visualRect(drop_index);
      if (drop_pos.y() > rect.top() + rect.height() / 2)  /* lower half means insert after */
        drop_id++;
    }
    else
      drop_id = -1;

    int is_ctrl = 0;
    if (iupTableCallDragDropCb(ih, drag_id - 1, drop_id - 1, &is_ctrl) == IUP_CONTINUE &&
        !iupStrBoolean(iupAttribGet(ih, "VIRTUALMODE")))
    {
      int src = drag_id - 1;  /* 0-based */
      int ncol = columnCount();
      QList<QTableWidgetItem*> items;
      int dest, c;

      bool wasBlocked = signalsBlocked();
      blockSignals(true);

      for (c = 0; c < ncol; c++)
        items.append(takeItem(src, c));
      removeRow(src);

      if (drop_id < 1)
        dest = rowCount();
      else
        dest = (drop_id - 1 > src) ? drop_id - 2 : drop_id - 1;

      insertRow(dest);
      for (c = 0; c < ncol; c++)
        if (items[c])
          setItem(dest, c, items[c]);

      iupTableMoveLinAttribs(ih, src + 1, dest + 1);
      selectRow(dest);

      blockSignals(wasBlocked);
    }

    event->acceptProposedAction();
  }

  void updateVirtualCells()
  {
    QRect visibleRect = viewport()->rect();
    int firstRow = rowAt(visibleRect.top());
    int lastRow = rowAt(visibleRect.bottom());
    int firstCol = columnAt(visibleRect.left());
    int lastCol = columnAt(visibleRect.right());

    if (firstRow < 0) firstRow = 0;
    if (lastRow < 0) lastRow = rowCount() - 1;
    if (firstCol < 0) firstCol = 0;
    if (lastCol < 0) lastCol = columnCount() - 1;

    populateVirtualCells(firstRow, lastRow, firstCol, lastCol);
  }

  void copySelection()
  {
    QTableWidgetItem* item = currentItem();
    if (!item)
      return;

    QString text = item->text();
    QApplication::clipboard()->setText(text);
  }

  void pasteToSelection()
  {
    QTableWidgetItem* item = currentItem();
    if (!item)
      return;

    if (!(item->flags() & Qt::ItemIsEditable))
      return;

    QString text = QApplication::clipboard()->text();
    item->setText(text);
  }

private:
  void setupCallbacks()
  {
    connect(this, &QTableWidget::cellClicked, this, &IupQtTableWidget::onCellClicked);
    connect(this, &QTableWidget::cellDoubleClicked, this, &IupQtTableWidget::onCellDoubleClicked);
    connect(this, &QTableWidget::currentCellChanged, this, &IupQtTableWidget::onCurrentCellChanged);
    connect(this, &QTableWidget::cellChanged, this, &IupQtTableWidget::onCellChanged);
  }

  void onCellClicked(int row, int column)
  {
    int lin = row + 1;
    int col = column + 1;

    IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
    if (cb)
    {
      cb(ih, lin, col, (char*)"1");  /* "1" = left button, single click */
    }
  }

  void onCellDoubleClicked(int row, int column)
  {
    (void)row;
    (void)column;
  }

  void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
  {
    (void)previousRow;
    (void)previousColumn;

    int lin = currentRow + 1;
    int col = currentColumn + 1;

    IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (cb)
    {
      cb(ih, lin, col);
    }
  }

  void onCellChanged(int row, int column)
  {
    int lin = row + 1;
    int col = column + 1;

    IFnii cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
    if (cb)
    {
      cb(ih, lin, col);
    }
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static QTableWidget* qtTableGetWidget(Ihandle* ih)
{
  return (QTableWidget*)ih->handle;
}

static void qtTableReapplyAllColors(Ihandle* ih)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  for (int row = 0; row < table->rowCount(); row++)
  {
    for (int col = 0; col < table->columnCount(); col++)
    {
      QTableWidgetItem* item = table->item(row, col);
      if (item)
      {
        qtTableApplyCellColors(ih, item, row + 1, col + 1);
      }
    }
  }
}

static Qt::Alignment qtTableGetColumnAlignment(Ihandle* ih, int col)
{
  char name[50];
  snprintf(name, sizeof(name), "ALIGNMENT%d", col);
  char* align_str = iupAttribGet(ih, name);

  if (!align_str)
    return Qt::AlignLeft | Qt::AlignVCenter;  /* Default */

  if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
    return Qt::AlignRight | Qt::AlignVCenter;
  else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
    return Qt::AlignCenter;
  else
    return Qt::AlignLeft | Qt::AlignVCenter;
}

static int qtTableIsColumnEditable(Ihandle* ih, int col)
{
  char name[50];
  snprintf(name, sizeof(name), "EDITABLE%d", col);
  char* editable_str = iupAttribGet(ih, name);

  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");

  return iupStrBoolean(editable_str);
}

static void qtTableEnsureItem(QTableWidget* table, int row, int col)
{
  if (!table->item(row, col))
  {
    QTableWidgetItem* item = new IupQtTableItem();
    table->setItem(row, col, item);
  }
}

static void qtTableApplyCellColors(Ihandle* ih, QTableWidgetItem* item, int lin, int col)
{
  char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);  /* Per-column */
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);  /* Per-row */

  if (!bgcolor)
  {
    char* alternate = iupAttribGet(ih, "ALTERNATECOLOR");

    if (iupStrBoolean(alternate))
    {
      if (lin % 2 == 0)
      {
        bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
      }
      else
      {
        bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
      }
    }
  }

  if (bgcolor && *bgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(bgcolor, &r, &g, &b))
    {
      item->setBackground(QBrush(QColor(r, g, b)));
    }
  }
  else
  {
    item->setData(Qt::BackgroundRole, QVariant());
  }

  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);  /* Per-column */
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);  /* Per-row */

  if (fgcolor && *fgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(fgcolor, &r, &g, &b))
    {
      item->setForeground(QBrush(QColor(r, g, b)));
    }
  }
  else
  {
    item->setData(Qt::ForegroundRole, QVariant());
  }
}

static void qtTableApplyCellFont(Ihandle* ih, QTableWidgetItem* item, int lin, int col)
{
  char* font = iupAttribGetId2(ih, "FONT", lin, col);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", 0, col);  /* Per-column */
  if (!font)
    font = iupAttribGetId2(ih, "FONT", lin, 0);  /* Per-row */

  if (font)
  {
    QFont* qfont = iupqtGetQFont(font);
    if (qfont)
    {
      item->setFont(*qfont);
    }
  }
}

static void qtTableConfigureItem(Ihandle* ih, QTableWidgetItem* item, int lin, int col)
{
  /* lin and col are 1-based IUP indices */
  if (!item)
    return;

  Qt::Alignment alignment = qtTableGetColumnAlignment(ih, col);
  item->setTextAlignment(alignment);

  int editable = qtTableIsColumnEditable(ih, col);
  Qt::ItemFlags flags = item->flags();

  if (editable)
    flags |= Qt::ItemIsEditable;
  else
    flags &= ~Qt::ItemIsEditable;

  item->setFlags(flags);

  qtTableApplyCellColors(ih, item, lin, col);
  qtTableApplyCellFont(ih, item, lin, col);
}

/****************************************************************************
 * Widget Lifecycle
 ****************************************************************************/

static void qtTableLayoutUpdateMethod(Ihandle* ih)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int width = ih->currentwidth;
  int height = ih->currentheight;

  QVariant targetVar = table->property("iup-table-target-height");
  if (targetVar.isValid())
  {
    int target_height = targetVar.toInt();
    if (target_height > 0 && height > target_height)
      height = target_height;
  }

  QVariant visColVar = table->property("iup-table-visible-columns");
  if (visColVar.isValid())
  {
    int visible_columns = visColVar.toInt();
    if (visible_columns > 0)
    {
      int cols_width = 0;
      int num_cols = visible_columns;
      if (num_cols > ih->data->num_col)
        num_cols = ih->data->num_col;

      for (int c = 0; c < num_cols; c++)
      {
        int col_width = table->columnWidth(c);
        if (col_width <= 0)
          col_width = 80;
        cols_width += col_width;
      }

      int sb_size = iupdrvGetScrollbarSize();
      int frame_width = table->frameWidth();

      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
      int need_vert_sb = (visiblelines > 0 && ih->data->num_lin > visiblelines);
      int vert_sb_width = need_vert_sb ? sb_size : 0;

      int target_width = cols_width + vert_sb_width + 2 * frame_width;
      if (width > target_width)
        width = target_width;
    }
  }

  QWidget* parent = table->parentWidget();
  if (parent)
    iupqtSetPosSize(parent, table, ih->x, ih->y, width, height);

  table->horizontalScrollBar()->setValue(0);
  table->verticalScrollBar()->setValue(0);
}

static int qtTableMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;

  IupQtTableWidget* table = new IupQtTableWidget(ih, nullptr);

  IupQtTableDelegate* delegate = new IupQtTableDelegate(ih, table);
  table->setItemDelegate(delegate);

  table->setRowCount(num_lin);
  table->setColumnCount(num_col);

  table->setShowGrid(iupAttribGetBoolean(ih, "SHOWGRID"));
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::SingleSelection);

  table->setEditTriggers(QAbstractItemView::DoubleClicked |
                         QAbstractItemView::EditKeyPressed |
                         QAbstractItemView::AnyKeyPressed);

  QHeaderView* hHeader = table->horizontalHeader();

  if (ih->data->sortable)
  {
    /* SORT_CB can veto, so rows are sorted here and not by the widget */
    table->setSortingEnabled(false);
    hHeader->setSectionsClickable(true);
    hHeader->setSortIndicatorShown(true);

    QObject::connect(hHeader, &QHeaderView::sectionClicked, [ih, hHeader, table](int logicalIndex) {
      int prev_col = iupAttribGetInt(ih, "_QT_SORT_COLUMN");
      int prev_ascending = iupAttribGetInt(ih, "_QT_SORT_ASCENDING");
      int ascending = (prev_col == (logicalIndex + 1)) ? !prev_ascending : 1;

      IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
      if (sort_cb && sort_cb(ih, logicalIndex + 1) == IUP_IGNORE)
      {
        /* QHeaderView flips its own indicator before emitting this, so put it back */
        hHeader->setSortIndicator(prev_col - 1, prev_ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
        return;
      }

      iupAttribSetInt(ih, "_QT_SORT_COLUMN", logicalIndex + 1);
      iupAttribSetInt(ih, "_QT_SORT_ASCENDING", ascending);
      hHeader->setSortIndicator(logicalIndex, ascending ? Qt::AscendingOrder : Qt::DescendingOrder);

      if (!iupAttribGetBoolean(ih, "VIRTUALMODE"))
        table->sortItems(logicalIndex, ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
    });
  }
  else
  {
    table->setSortingEnabled(false);
    hHeader->setSectionsClickable(false);
  }

  hHeader->setSectionsMovable(ih->data->allow_reorder);

  QObject::connect(hHeader, &QHeaderView::sectionMoved, [ih](int logicalIndex, int oldVisualIndex, int newVisualIndex) {
    (void)logicalIndex;
    IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
    if (cb)
      cb(ih, oldVisualIndex + 1, newVisualIndex + 1);
  });

  bool last_col_has_width = false;
  {
    char name[50];
    int width = 0;

    snprintf(name, sizeof(name), "RASTERWIDTH%d", num_col);
    char* width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      snprintf(name, sizeof(name), "WIDTH%d", num_col);
      width_str = iupAttribGet(ih, name);
    }

    last_col_has_width = (width_str && iupStrToInt(width_str, &width) && width > 0);
  }

  {
    int header_height = hHeader->sizeHint().height();
    if (header_height > 0)
      hHeader->setFixedHeight(header_height);
  }

  bool stretch_last = (ih->data->stretch_last && !last_col_has_width);
  hHeader->setStretchLastSection(false);

  for (int col = 1; col <= num_col; col++)
  {
    int qt_col = col - 1;
    char name[50];
    char* width_str = NULL;
    int width = 0;

    snprintf(name, sizeof(name), "RASTERWIDTH%d", col);
    width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      snprintf(name, sizeof(name), "WIDTH%d", col);
      width_str = iupAttribGet(ih, name);
    }

    if (width_str && iupStrToInt(width_str, &width) && width > 0)
    {
      table->setColumnWidth(qt_col, width);

      if (ih->data->user_resize)
        hHeader->setSectionResizeMode(qt_col, QHeaderView::Interactive);
      else
        hHeader->setSectionResizeMode(qt_col, QHeaderView::Fixed);
    }
    else if (col == num_col && stretch_last)
    {
      hHeader->setSectionResizeMode(qt_col, QHeaderView::Stretch);
    }
    else
    {
      hHeader->setSectionResizeMode(qt_col, QHeaderView::ResizeToContents);
    }
  }

  char* sel_mode = iupAttribGetStr(ih, "SELECTIONMODE");
  if (sel_mode)
  {
    if (iupStrEqualNoCase(sel_mode, "MULTIPLE") || iupStrEqualNoCase(sel_mode, "EXTENDED"))
    {
      /* Qt has one ExtendedSelection for both MULTIPLE and EXTENDED */
      table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    else if (iupStrEqualNoCase(sel_mode, "NONE"))
    {
      table->setSelectionMode(QAbstractItemView::NoSelection);
    }
  }

  QHeaderView* vHeader = table->verticalHeader();
  vHeader->setVisible(false);

  QFontMetrics fm(table->font());
  int margin = table->style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, table);
  int row_height = fm.height() + 2 * margin;
  vHeader->setDefaultSectionSize(row_height);

  table->clearSelection();
  table->setCurrentCell(-1, -1);

  if (ih->data->show_dragdrop)
  {
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setDragEnabled(true);
    table->setAcceptDrops(true);
    table->setDragDropMode(QAbstractItemView::DragDrop);
    table->setDropIndicatorShown(true);
    table->setDefaultDropAction(Qt::MoveAction);
  }

  ih->handle = (InativeHandle*)table;

  iupqtAddToParent(ih);

  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int header_height = iupdrvTableGetHeaderHeight(ih);
    int sb_size = iupdrvGetScrollbarSize();
    int frame_width = table->frameWidth();

    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int need_horiz_sb = (visiblecolumns > 0 && ih->data->num_col > visiblecolumns);
    int horiz_sb_height = need_horiz_sb ? sb_size : 0;

    int target_height = header_height + (row_height * visiblelines) + horiz_sb_height + 2 * frame_width;
    table->setProperty("iup-table-target-height", target_height);
  }

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0)
    table->setProperty("iup-table-visible-columns", visiblecolumns);

  return IUP_NOERROR;
}

static void qtTableUnMapMethod(Ihandle* ih)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (table)
  {
    delete table;
    ih->handle = nullptr;
  }

  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Driver Functions - Table Structure
 ****************************************************************************/

IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;

  if (ih->handle)
  {
    QTableWidget* table = qtTableGetWidget(ih);
    table->setColumnCount(num_col);
  }
}

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  if (ih->handle)
  {
    QTableWidget* table = qtTableGetWidget(ih);
    table->setRowCount(num_lin);
  }
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  /* pos is 1-based, 0 means append */
  if (pos == 0)
    pos = ih->data->num_col + 1;

  if (pos < 1 || pos > ih->data->num_col + 1)
    return;

  int qt_col = pos - 1;
  table->insertColumn(qt_col);
  ih->data->num_col++;
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  if (pos < 1 || pos > ih->data->num_col)
    return;

  int qt_col = pos - 1;
  table->removeColumn(qt_col);
  ih->data->num_col--;
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  /* pos is 1-based, 0 means append */
  if (pos == 0)
    pos = ih->data->num_lin + 1;

  if (pos < 1 || pos > ih->data->num_lin + 1)
    return;

  int qt_row = pos - 1;
  table->insertRow(qt_row);
  ih->data->num_lin++;
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  int qt_row = pos - 1;
  table->removeRow(qt_row);
  ih->data->num_lin--;
}

/****************************************************************************
 * Driver Functions - Cell Operations
 ****************************************************************************/

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_row = lin - 1;
  int qt_col = col - 1;

  if (qt_row < 0 || qt_row >= table->rowCount() ||
      qt_col < 0 || qt_col >= table->columnCount())
    return;

  qtTableEnsureItem(table, qt_row, qt_col);

  QTableWidgetItem* item = table->item(qt_row, qt_col);
  if (item)
  {
    /* VALUECHANGED_CB must fire only for interactive changes */
    bool wasBlocked = table->signalsBlocked();
    table->blockSignals(true);

    qtTableConfigureItem(ih, item, lin, col);

    item->setText(value ? QString::fromUtf8(value) : QString());

    table->blockSignals(wasBlocked);
  }
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return nullptr;

  int qt_row = lin - 1;
  int qt_col = col - 1;

  if (qt_row < 0 || qt_row >= table->rowCount() ||
      qt_col < 0 || qt_col >= table->columnCount())
    return nullptr;

  QTableWidgetItem* item = table->item(qt_row, qt_col);
  if (!item)
    return nullptr;

  QString text = item->text();
  if (text.isEmpty())
    return nullptr;

  return iupStrReturnStr(text.toUtf8().constData());
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_row = lin - 1;
  int qt_col = col - 1;

  if (qt_row < 0 || qt_row >= table->rowCount() ||
      qt_col < 0 || qt_col >= table->columnCount())
    return;

  qtTableEnsureItem(table, qt_row, qt_col);
  QTableWidgetItem* item = table->item(qt_row, qt_col);
  if (!item)
    return;

  if (image)
  {
    QPixmap* pixImage = (QPixmap*)iupImageGetImage(image, ih, 0, NULL);
    if (pixImage)
    {
      if (ih->data->fit_image)
      {
        int charheight;
        iupdrvFontGetCharSize(ih, NULL, &charheight);
        int available_height = charheight + 4;
        if (pixImage->height() > available_height)
        {
          QPixmap scaled = pixImage->scaledToHeight(available_height, Qt::SmoothTransformation);
          item->setIcon(QIcon(scaled));
        }
        else
          item->setIcon(QIcon(*pixImage));
      }
      else
        item->setIcon(QIcon(*pixImage));
    }
    else
      item->setIcon(QIcon());
  }
  else
    item->setIcon(QIcon());
}

/****************************************************************************
 * Driver Functions - Column Operations
 ****************************************************************************/

IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_col = col - 1;

  if (qt_col < 0 || qt_col >= table->columnCount())
    return;

  QTableWidgetItem* headerItem = new QTableWidgetItem(title ? QString::fromUtf8(title) : QString());
  table->setHorizontalHeaderItem(qt_col, headerItem);
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return nullptr;

  int qt_col = col - 1;

  if (qt_col < 0 || qt_col >= table->columnCount())
    return nullptr;

  QTableWidgetItem* headerItem = table->horizontalHeaderItem(qt_col);
  if (!headerItem)
    return nullptr;

  QString text = headerItem->text();
  if (text.isEmpty())
    return nullptr;

  return iupStrReturnStr(text.toUtf8().constData());
}

IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_col = col - 1;

  if (qt_col < 0 || qt_col >= table->columnCount())
    return;

  QHeaderView* hHeader = table->horizontalHeader();

  table->setColumnWidth(qt_col, width);

  if (ih->data->user_resize)
  {
    hHeader->setSectionResizeMode(qt_col, QHeaderView::Interactive);
  }
  else
  {
    hHeader->setSectionResizeMode(qt_col, QHeaderView::Fixed);
  }
}

IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return 0;

  int qt_col = col - 1;

  if (qt_col < 0 || qt_col >= table->columnCount())
    return 0;

  return table->columnWidth(qt_col);
}

/****************************************************************************
 * Driver Functions - Navigation and Display
 ****************************************************************************/

IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_row = lin - 1;
  int qt_col = col - 1;

  if (qt_row >= 0 && qt_row < table->rowCount() &&
      qt_col >= 0 && qt_col < table->columnCount())
  {
    table->setCurrentCell(qt_row, qt_col);
  }
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  *lin = table->currentRow() + 1;
  *col = table->currentColumn() + 1;
}

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_row = lin - 1;
  int qt_col = col - 1;

  if (qt_row >= 0 && qt_row < table->rowCount() &&
      qt_col >= 0 && qt_col < table->columnCount())
  {
    qtTableEnsureItem(table, qt_row, qt_col);
    QTableWidgetItem* item = table->item(qt_row, qt_col);
    if (item)
    {
      table->scrollToItem(item);
    }
  }
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (table)
  {
    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (iupStrBoolean(virtualmode))
    {
      sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
      if (value_cb)
      {
        bool wasBlocked = table->signalsBlocked();
        table->blockSignals(true);

        for (int row = 0; row < table->rowCount(); row++)
        {
          for (int col = 0; col < table->columnCount(); col++)
          {
            QTableWidgetItem* existingItem = table->item(row, col);
            if (existingItem)
            {
              char* value = value_cb(ih, row + 1, col + 1);
              if (value)
                existingItem->setText(QString::fromUtf8(value));
              else
                existingItem->setText(QString());
            }
          }
        }

        table->blockSignals(wasBlocked);
      }
    }

    qtTableReapplyAllColors(ih);

    table->viewport()->update();
  }
}

IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (table)
  {
    table->setShowGrid(show != 0);
  }
}

/****************************************************************************
 * Attribute Handlers
 ****************************************************************************/

static int qtTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
    ih->data->sortable = 0;

  if (ih->handle)
  {
    QTableWidget* table = qtTableGetWidget(ih);
    if (table)
    {
      char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
      QHeaderView* hHeader = table->horizontalHeader();

      if (ih->data->sortable)
      {
        table->setSortingEnabled(!iupStrBoolean(virtualmode));
        hHeader->setSectionsClickable(true);
        hHeader->setSortIndicatorShown(true);
      }
      else
      {
        table->setSortingEnabled(false);
        hHeader->setSectionsClickable(false);
      }
    }
  }
  return 0; /* do not store in hash table */
}

static int qtTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;

  if (ih->handle)
  {
    QTableWidget* table = qtTableGetWidget(ih);
    QHeaderView* hHeader = table->horizontalHeader();
    if (hHeader)
      hHeader->setSectionsMovable(ih->data->allow_reorder);
  }
  return 0; /* do not store in hash table */
}

static int qtTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  QTableWidget* table = qtTableGetWidget(ih);

  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;

  if (!table)
    return 0;

  bool last_col_has_width = false;
  {
    char name[50];
    int w = 0;
    snprintf(name, sizeof(name), "RASTERWIDTH%d", ih->data->num_col);
    char* ws = iupAttribGet(ih, name);
    if (!ws) { snprintf(name, sizeof(name), "WIDTH%d", ih->data->num_col); ws = iupAttribGet(ih, name); }
    last_col_has_width = (ws && iupStrToInt(ws, &w) && w > 0);
  }
  bool stretch_last = (ih->data->stretch_last && !last_col_has_width);

  QHeaderView* hHeader = table->horizontalHeader();

  for (int col = 0; col < ih->data->num_col; col++)
  {
    if (ih->data->user_resize)
    {
      hHeader->setSectionResizeMode(col, QHeaderView::Interactive);
    }
    else if (col == ih->data->num_col - 1 && stretch_last)
    {
      hHeader->setSectionResizeMode(col, QHeaderView::Stretch);
    }
    else if (table->columnWidth(col) > 0)
    {
      hHeader->setSectionResizeMode(col, QHeaderView::Fixed);
    }
    else
    {
      hHeader->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }
  }

  return 0; /* do not store in hash table */
}

/****************************************************************************
 * Class Registration
 ****************************************************************************/

extern "C" {

IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  return 0;
}

static int qt_table_row_height = -1;
static int qt_table_header_height = -1;

static void qtTableMeasureRowMetrics(Ihandle* ih)
{
  if (qt_table_row_height >= 0)
    return;

  QTableWidget* temp_table = new QTableWidget(1, 1);
  temp_table->setItem(0, 0, new QTableWidgetItem("WWWWWWWWWW"));

  QFontMetrics fm(temp_table->font());
  int margin = temp_table->style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, temp_table);
  int calculated_height = fm.height() + 2 * margin;

  temp_table->verticalHeader()->setDefaultSectionSize(calculated_height);
  qt_table_row_height = calculated_height;

  qt_table_header_height = temp_table->horizontalHeader()->sizeHint().height();
  if (qt_table_header_height <= 0)
    qt_table_header_height = calculated_height;

  delete temp_table;

  (void)ih;
}

IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  QTableWidget* table = qtTableGetWidget(ih);

  if (table && table->rowCount() > 0)
  {
    int row_height = table->rowHeight(0);
    if (row_height > 0)
      return row_height;
  }

  qtTableMeasureRowMetrics(ih);
  return qt_table_row_height;
}

IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  QTableWidget* table = qtTableGetWidget(ih);

  if (table)
  {
    QHeaderView* header = table->horizontalHeader();
    int height = header->sizeHint().height();
    if (height > 0)
      return height;
  }

  qtTableMeasureRowMetrics(ih);
  return qt_table_header_height;
}

IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  QTableWidget* table = qtTableGetWidget(ih);

  int frame_width = 2;  /* Default */
  if (table)
    frame_width = table->frameWidth();

  int sb_size = iupdrvGetScrollbarSize();

  *w += sb_size + 2 * frame_width;

  *h += 2 * frame_width;

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
    *h += sb_size;
}

IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = qtTableMapMethod;
  ic->UnMap = qtTableUnMapMethod;
  ic->LayoutUpdate = qtTableLayoutUpdateMethod;

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, qtTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, qtTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, qtTableSetUserResizeAttrib);
}

} /* extern "C" */

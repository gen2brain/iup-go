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
#include <QKeyEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyleOptionFocusRect>
#include <QFontMetrics>
#include <QLineEdit>
#include <QtGlobal>

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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_tablecontrol.h"
}

#include "iupqt_drv.h"


/* Forward declarations */
static void qtTableConfigureItem(Ihandle* ih, QTableWidgetItem* item, int lin, int col);
static void qtTableApplyCellColors(Ihandle* ih, QTableWidgetItem* item, int lin, int col);

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

    // Check if virtual mode is enabled
    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (!iupStrBoolean(virtualmode))
      return QStyledItemDelegate::displayText(value, locale);

    /* In virtual mode, we need to get the row/column. This is called during painting, so we get the index from the model. */
    /* However, we don't have direct access to the index here. So we'll rely on the item's display role being set correctly */
    return value.toString();
  }

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    /* Check if this item has focus */
    bool hasFocus = (option.state & QStyle::State_HasFocus);

    /* Remove default focus rectangle by clearing the focus state */
    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;

    /* Draw the item without Qt's default focus rectangle */
    QStyledItemDelegate::paint(painter, opt, index);

    /* Draw native-style focus rectangle if FOCUSRECT=YES and item has focus */
    if (hasFocus && iupAttribGetBoolean(ih, "FOCUSRECT"))
    {
      /* Use Qt's style system to draw a focus rectangle */
      QStyleOptionFocusRect focusOption;
      focusOption.rect = option.rect.adjusted(1, 1, -1, -1);
      focusOption.state = option.state | QStyle::State_KeyboardFocusChange;
      focusOption.backgroundColor = option.palette.color(QPalette::Highlight);

      QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, painter);
    }
  }

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    int lin = index.row() + 1;  /* 1-based */
    int col = index.column() + 1;  /* 1-based */

    /* Call EDITBEGIN_CB - allow application to block editing */
    IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
    if (editbegin_cb)
    {
      int ret = editbegin_cb(ih, lin, col);
      if (ret == IUP_IGNORE)
        return nullptr;  /* Block editing by not creating editor */
    }

    /* Create custom QLineEdit editor with overridden sizeHint */
    IupQtFixedLineEdit* fixedLineEdit = new IupQtFixedLineEdit(parent);
    fixedLineEdit->setFrame(false);
    fixedLineEdit->setTextMargins(0, 0, 0, 0);
    fixedLineEdit->setContentsMargins(0, 0, 0, 0);

    fixedLineEdit->installEventFilter(const_cast<IupQtTableDelegate*>(this));
    /* Store index in editor for later retrieval in eventFilter */
    fixedLineEdit->setProperty("iup_row", lin);
    fixedLineEdit->setProperty("iup_col", col);
    /* Store pointer for later retrieval */
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
      /* NOTE: setTextMargins() triggers updateGeometry() which will override
       * any size we set before this! So we must set fixed size AFTER. */
      lineEdit->setTextMargins(0, 0, 0, 0);
    }

    /* Set geometry first */
    editor->setGeometry(editorRect);

    /* THEN set fixed size hint and fixed size */
    if (lineEdit)
    {
      /* Retrieve the IupQtFixedLineEdit pointer */
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

  bool eventFilter(QObject* object, QEvent* event) override
  {
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Escape)
      {
        /* ESC key pressed - editing cancelled */
        QWidget* editor = qobject_cast<QWidget*>(object);
        if (editor)
        {
          int lin = editor->property("iup_row").toInt();
          int col = editor->property("iup_col").toInt();

          /* Get current cell value before cancel */
          QVariant value = editor->property("text");
          QString current_text = value.toString();

          /* Call EDITEND_CB with apply=0 (cancelled) */
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

  void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
  {
    int lin = index.row() + 1;  /* 1-based */
    int col = index.column() + 1;  /* 1-based */

    /* Get the new value from editor */
    QVariant value = editor->property("text");
    QString new_text = value.toString();

    /* Call EDITEND_CB - allow application to validate/reject edit */
    IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
    if (editend_cb)
    {
      int ret = editend_cb(ih, lin, col, (char*)new_text.toUtf8().constData(), 1);  /* 1 = accepted */
      if (ret == IUP_IGNORE)
        return;  /* Reject edit - don't update model */
    }

    /* Update model data */
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

public:
  explicit IupQtTableWidget(Ihandle* ih_param, QWidget* parent = nullptr)
    : QTableWidget(parent), ih(ih_param), firstShow(true)
  {
    // Minimal size hints to let IUP control sizing
    // Block signals during initialization to prevent VALUECHANGED_CB during cell population
    // Signals will be unblocked when table receives focus (first user interaction)
    blockSignals(true);
    setupCallbacks();
  }

  void enableChangeNotifications()
  {
    firstShow = false;
    blockSignals(false);
  }

  // Populate virtual cells when they become visible
  void populateVirtualCells(int firstRow, int lastRow, int firstCol, int lastCol)
  {
    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (!iupStrBoolean(virtualmode))
      return;

    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (!value_cb)
      return;

    // Block signals during virtual cell population, save previous state
    bool wasBlocked = signalsBlocked();
    blockSignals(true);

    // Populate visible cells
    for (int row = firstRow; row <= lastRow && row < rowCount(); row++)
    {
      for (int col = firstCol; col <= lastCol && col < columnCount(); col++)
      {
        QTableWidgetItem* existingItem = item(row, col);
        if (!existingItem)
        {
          existingItem = new QTableWidgetItem();
          setItem(row, col, existingItem);
        }

        // Query VALUE_CB for cell content (1-based indices)
        char* value = value_cb(ih, row + 1, col + 1);
        if (value)
        {
          existingItem->setText(QString::fromUtf8(value));
        }
        else
        {
          existingItem->setText(QString());
        }

        // Configure item with alignment, colors, fonts, editable flags
        qtTableConfigureItem(ih, existingItem, row + 1, col + 1);
      }
    }

    // Restore previous signal blocking state
    blockSignals(wasBlocked);
  }

  /* Override sizeHint to return minimal size.
   * This prevents Qt from using its default larger size hint, allowing IUP's
   * natural size calculation to control the widget size. */
  QSize sizeHint() const override
  {
    QFontMetrics fm(font());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    int charWidth = fm.width('X');
#else
    int charWidth = fm.horizontalAdvance('X');
#endif
    int charHeight = fm.height();

    // Return minimal size: 10 chars wide, 3 lines tall (similar to List)
    int w = charWidth * 10;
    int h = charHeight * 3;

    return QSize(w, h);
  }

  QSize minimumSizeHint() const override
  {
    QFontMetrics fm(font());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    int charWidth = fm.width('X');
#else
    int charWidth = fm.horizontalAdvance('X');
#endif
    int charHeight = fm.height();

    // Return minimal size: 5 chars wide, 2 lines tall
    return QSize(charWidth * 5, charHeight * 2);
  }

protected:
  void showEvent(QShowEvent* event) override
  {
    QTableWidget::showEvent(event);

    // Populate virtual cells when table is first shown
    updateVirtualCells();
  }

  void focusInEvent(QFocusEvent* event) override
  {
    // Unblock signals on first focus (user is about to interact)
    if (firstShow && signalsBlocked())
    {
      enableChangeNotifications();
    }
    QTableWidget::focusInEvent(event);
  }

  void scrollContentsBy(int dx, int dy) override
  {
    QTableWidget::scrollContentsBy(dx, dy);
    // Populate virtual cells after scrolling
    updateVirtualCells();
  }

  void resizeEvent(QResizeEvent* event) override
  {
    QTableWidget::resizeEvent(event);
    // Populate virtual cells after resizing
    updateVirtualCells();
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    // Handle copy/paste
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
    // Handle Enter/Return to activate cell editing
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

  void updateVirtualCells()
  {
    // Get visible range
    QRect visibleRect = viewport()->rect();
    int firstRow = rowAt(visibleRect.top());
    int lastRow = rowAt(visibleRect.bottom());
    int firstCol = columnAt(visibleRect.left());
    int lastCol = columnAt(visibleRect.right());

    // Handle edge cases
    if (firstRow < 0) firstRow = 0;
    if (lastRow < 0) lastRow = rowCount() - 1;
    if (firstCol < 0) firstCol = 0;
    if (lastCol < 0) lastCol = columnCount() - 1;

    // Populate visible cells
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

    // Check if cell is editable
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
    // Convert to 1-based IUP indices
    int lin = row + 1;
    int col = column + 1;

    IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
    if (cb)
    {
      // Get button and status info (simple left-click)
      cb(ih, lin, col, (char*)"1");  // "1" = left button, single click
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

    // Convert to 1-based IUP indices
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
    // Convert to 1-based IUP indices
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

  // Reapply colors to all existing cells
  for (int row = 0; row < table->rowCount(); row++)
  {
    for (int col = 0; col < table->columnCount(); col++)
    {
      QTableWidgetItem* item = table->item(row, col);
      if (item)
      {
        // Reapply colors (lin and col are 1-based)
        qtTableApplyCellColors(ih, item, row + 1, col + 1);
      }
    }
  }
}

static Qt::Alignment qtTableGetColumnAlignment(Ihandle* ih, int col)
{
  // Check for column-specific alignment (1-based col index)
  char name[50];
  sprintf(name, "ALIGNMENT%d", col);
  char* align_str = iupAttribGet(ih, name);

  if (!align_str)
    return Qt::AlignLeft | Qt::AlignVCenter;  // Default

  if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
    return Qt::AlignRight | Qt::AlignVCenter;
  else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
    return Qt::AlignCenter;
  else  // ALEFT, LEFT, or anything else
    return Qt::AlignLeft | Qt::AlignVCenter;
}

static int qtTableIsColumnEditable(Ihandle* ih, int col)
{
  // Check for column-specific editable (1-based col index)
  char name[50];
  sprintf(name, "EDITABLE%d", col);
  char* editable_str = iupAttribGet(ih, name);

  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");  // Global editable

  return iupStrBoolean(editable_str);
}

static void qtTableEnsureItem(QTableWidget* table, int row, int col)
{
  // Qt uses 0-based indices
  if (!table->item(row, col))
  {
    QTableWidgetItem* item = new QTableWidgetItem();
    table->setItem(row, col, item);
  }
}

static QString qtTableGetVirtualValue(Ihandle* ih, int lin, int col)
{
  // Check if virtual mode is enabled
  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (!iupStrBoolean(virtualmode))
    return QString();

  // Call VALUE_CB to get the cell value
  sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  if (value_cb)
  {
    char* value = value_cb(ih, lin, col);
    if (value)
      return QString::fromUtf8(value);
  }

  return QString();
}

static void qtTableApplyCellColors(Ihandle* ih, QTableWidgetItem* item, int lin, int col)
{
  // Check for cell-specific bgcolor (L:C), then per-column (:C), then per-row (L:*)
  char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);  // Per-column
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);  // Per-row

  // If no specific bgcolor set, check for alternating row colors
  if (!bgcolor)
  {
    char* alternate = iupAttribGet(ih, "ALTERNATECOLOR");

    if (iupStrBoolean(alternate))
    {
      // Determine if this is an even or odd row (lin is 1-based)
      if (lin % 2 == 0)
      {
        // Even row
        bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
      }
      else
      {
        // Odd row
        bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
      }
    }
  }

  if (bgcolor && *bgcolor)  // Check for non-NULL and non-empty
  {
    unsigned char r, g, b;
    if (iupStrToRGB(bgcolor, &r, &g, &b))
    {
      item->setBackground(QBrush(QColor(r, g, b)));
    }
  }
  else
  {
    // Reset to default background if no color is set or empty string
    item->setData(Qt::BackgroundRole, QVariant());
  }

  // Check for cell-specific fgcolor (L:C), then per-column (:C), then per-row (L:*)
  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);  // Per-column
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);  // Per-row

  if (fgcolor && *fgcolor)  // Check for non-NULL and non-empty
  {
    unsigned char r, g, b;
    if (iupStrToRGB(fgcolor, &r, &g, &b))
    {
      item->setForeground(QBrush(QColor(r, g, b)));
    }
  }
  else
  {
    // Reset to default foreground if no color is set or empty string
    item->setData(Qt::ForegroundRole, QVariant());
  }
}

static void qtTableApplyCellFont(Ihandle* ih, QTableWidgetItem* item, int lin, int col)
{
  // Check for cell-specific font (L:C), then per-column (:C), then per-row (L:*)
  char* font = iupAttribGetId2(ih, "FONT", lin, col);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", 0, col);  // Per-column
  if (!font)
    font = iupAttribGetId2(ih, "FONT", lin, 0);  // Per-row

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
  // lin and col are 1-based IUP indices
  if (!item)
    return;

  // Set alignment
  Qt::Alignment alignment = qtTableGetColumnAlignment(ih, col);
  item->setTextAlignment(alignment);

  // Set editable flag
  int editable = qtTableIsColumnEditable(ih, col);
  Qt::ItemFlags flags = item->flags();

  if (editable)
    flags |= Qt::ItemIsEditable;
  else
    flags &= ~Qt::ItemIsEditable;

  item->setFlags(flags);

  // Apply colors and font
  qtTableApplyCellColors(ih, item, lin, col);
  qtTableApplyCellFont(ih, item, lin, col);
}

/****************************************************************************
 * Widget Lifecycle
 ****************************************************************************/

static int qtTableMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  // Get initial dimensions
  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;

  // Create custom table widget
  IupQtTableWidget* table = new IupQtTableWidget(ih, nullptr);

  // Install custom delegate to handle focus rectangle drawing
  IupQtTableDelegate* delegate = new IupQtTableDelegate(ih, table);
  table->setItemDelegate(delegate);

  // Set dimensions
  table->setRowCount(num_lin);
  table->setColumnCount(num_col);

  // Configure basic properties
  table->setShowGrid(iupAttribGetBoolean(ih, "SHOWGRID"));
  table->setSelectionBehavior(QAbstractItemView::SelectRows);  // Select rows by default
  table->setSelectionMode(QAbstractItemView::SingleSelection);  // Single selection by default

  // Virtual mode: cells are populated on-demand via VALUE_CB

  // Configure editing triggers (double-click, selected cell, or F2)
  table->setEditTriggers(QAbstractItemView::DoubleClicked |
                         QAbstractItemView::SelectedClicked |
                         QAbstractItemView::EditKeyPressed |
                         QAbstractItemView::AnyKeyPressed);

  // Check for SORTABLE attribute (from ih->data)
  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  QHeaderView* hHeader = table->horizontalHeader();

  if (ih->data->sortable)
  {
    if (iupStrBoolean(virtualmode))
    {
      // Virtual mode - disable automatic sorting
      table->setSortingEnabled(false);
      hHeader->setSectionsClickable(true);
      hHeader->setSortIndicatorShown(true);
    }
    else
    {
      // Normal mode - enable Qt's automatic sorting
      table->setSortingEnabled(true);
      hHeader->setSectionsClickable(true);
      hHeader->setSortIndicatorShown(true);
    }

    // Connect header section clicked for SORT_CB callback
    QObject::connect(hHeader, &QHeaderView::sectionClicked, [ih, hHeader](int logicalIndex) {
      char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");

      // Virtual mode - manually track sort state
      if (iupStrBoolean(virtualmode))
      {
        // Get current sort state
        int current_col = iupAttribGetInt(ih, "_QT_SORT_COLUMN");
        int ascending = iupAttribGetInt(ih, "_QT_SORT_ASCENDING");

        // Toggle direction for same column, ascending for new column
        if (current_col == (logicalIndex + 1))
        {
          // Same column - toggle direction
          ascending = !ascending;
        }
        else
        {
          // New column - start with ascending
          current_col = logicalIndex + 1;
          ascending = 1;
        }

        // Store new state
        iupAttribSetInt(ih, "_QT_SORT_COLUMN", current_col);
        iupAttribSetInt(ih, "_QT_SORT_ASCENDING", ascending);

        // Update sort indicator
        hHeader->setSortIndicator(logicalIndex,
          ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
      }

      // Call SORT_CB callback if it exists
      IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
      if (sort_cb)
      {
        sort_cb(ih, logicalIndex + 1);  // Convert to 1-based
      }
    });
  }
  else
  {
    table->setSortingEnabled(false);
    hHeader->setSectionsClickable(false);
  }

  // Check for ALLOWREORDER attribute (from ih->data)
  hHeader->setSectionsMovable(ih->data->allow_reorder);

  // Check if last column has explicit width set
  bool last_col_has_width = false;
  {
    char name[50];
    int width = 0;

    sprintf(name, "RASTERWIDTH%d", num_col);
    char* width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      sprintf(name, "WIDTH%d", num_col);
      width_str = iupAttribGet(ih, name);
    }

    last_col_has_width = (width_str && iupStrToInt(width_str, &width) && width > 0);
  }

  // Stretch last section ONLY if STRETCHLAST=YES (default) AND last column doesn't have explicit width
  hHeader->setStretchLastSection((ih->data->stretch_last && !last_col_has_width) ? true : false);

  // Default to ResizeToContents - columns auto-size to their content
  // This must be set AFTER setStretchLastSection so non-stretched last column uses ResizeToContents
  hHeader->setSectionResizeMode(QHeaderView::ResizeToContents);

  // Apply any pre-set column widths (RASTERWIDTH/WIDTH set before mapping)
  // This is especially important for virtual mode where empty cells would shrink columns
  for (int col = 1; col <= num_col; col++)
  {
    char name[50];
    char* width_str = NULL;
    int width = 0;

    // Check RASTERWIDTH first
    sprintf(name, "RASTERWIDTH%d", col);
    width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      // Then check WIDTH
      sprintf(name, "WIDTH%d", col);
      width_str = iupAttribGet(ih, name);
    }

    if (width_str && iupStrToInt(width_str, &width) && width > 0)
    {
      int qt_col = col - 1;  // Convert to 0-based

      // Set the explicit width
      table->setColumnWidth(qt_col, width);

      // Set resize mode based on USERRESIZE setting
      if (ih->data->user_resize)
      {
        hHeader->setSectionResizeMode(qt_col, QHeaderView::Interactive);
      }
      else
      {
        hHeader->setSectionResizeMode(qt_col, QHeaderView::Fixed);
      }
    }
  }

  // Check for SELECTIONMODE attribute
  char* sel_mode = iupAttribGetStr(ih, "SELECTIONMODE");
  if (sel_mode)
  {
    if (iupStrEqualNoCase(sel_mode, "MULTIPLE") || iupStrEqualNoCase(sel_mode, "EXTENDED"))
    {
      // Both MULTIPLE and EXTENDED use ExtendedSelection in Qt
      // This requires Ctrl for multiple selection, Shift for range selection
      table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    else if (iupStrEqualNoCase(sel_mode, "NONE"))
    {
      table->setSelectionMode(QAbstractItemView::NoSelection);
    }
  }

  // Hide row numbers (vertical header)
  table->verticalHeader()->setVisible(false);

  // Clear any default selection - no cell should be selected on start
  table->clearSelection();
  table->setCurrentCell(-1, -1);

  // Store widget handle
  ih->handle = (InativeHandle*)table;

  // Add to parent
  iupqtAddToParent(ih);

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

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
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

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
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

void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  // pos is 1-based, 0 means append
  if (pos == 0)
    pos = ih->data->num_col + 1;

  if (pos < 1 || pos > ih->data->num_col + 1)
    return;

  int qt_col = pos - 1;  // Convert to 0-based
  table->insertColumn(qt_col);
  ih->data->num_col++;
}

void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  if (pos < 1 || pos > ih->data->num_col)
    return;

  int qt_col = pos - 1;  // Convert to 0-based
  table->removeColumn(qt_col);
  ih->data->num_col--;
}

void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  // pos is 1-based, 0 means append
  if (pos == 0)
    pos = ih->data->num_lin + 1;

  if (pos < 1 || pos > ih->data->num_lin + 1)
    return;

  int qt_row = pos - 1;  // Convert to 0-based
  table->insertRow(qt_row);
  ih->data->num_lin++;
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  int qt_row = pos - 1;  // Convert to 0-based
  table->removeRow(qt_row);
  ih->data->num_lin--;
}

/****************************************************************************
 * Driver Functions - Cell Operations
 ****************************************************************************/

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  // Convert to 0-based indices
  int qt_row = lin - 1;
  int qt_col = col - 1;

  if (qt_row < 0 || qt_row >= table->rowCount() ||
      qt_col < 0 || qt_col >= table->columnCount())
    return;

  // Ensure item exists
  qtTableEnsureItem(table, qt_row, qt_col);

  QTableWidgetItem* item = table->item(qt_row, qt_col);
  if (item)
  {
    // Block signals during ALL programmatic updates to prevent VALUECHANGED_CB
    // VALUECHANGED_CB should only fire for interactive user changes
    bool wasBlocked = table->signalsBlocked();
    table->blockSignals(true);

    // Configure item with alignment and editable flags (this modifies the item!)
    qtTableConfigureItem(ih, item, lin, col);

    // Set the text value
    item->setText(value ? QString::fromUtf8(value) : QString());

    // Restore previous signal blocking state
    table->blockSignals(wasBlocked);
  }
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return nullptr;

  // Convert to 0-based indices
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

/****************************************************************************
 * Driver Functions - Column Operations
 ****************************************************************************/

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_col = col - 1;  // Convert to 0-based

  if (qt_col < 0 || qt_col >= table->columnCount())
    return;

  QTableWidgetItem* headerItem = new QTableWidgetItem(title ? QString::fromUtf8(title) : QString());
  table->setHorizontalHeaderItem(qt_col, headerItem);
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return nullptr;

  int qt_col = col - 1;  // Convert to 0-based

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

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return;

  int qt_col = col - 1;  // Convert to 0-based

  if (qt_col < 0 || qt_col >= table->columnCount())
    return;

  QHeaderView* hHeader = table->horizontalHeader();

  // Set the explicit width first
  table->setColumnWidth(qt_col, width);

  // Determine resize mode based on USERRESIZE setting
  if (ih->data->user_resize)
  {
    // USERRESIZE=YES: Allow user to manually resize
    hHeader->setSectionResizeMode(qt_col, QHeaderView::Interactive);
  }
  else
  {
    // Default: Fixed width when explicit width is set
    hHeader->setSectionResizeMode(qt_col, QHeaderView::Fixed);
  }
}

int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
    return 0;

  int qt_col = col - 1;  // Convert to 0-based

  if (qt_col < 0 || qt_col >= table->columnCount())
    return 0;

  return table->columnWidth(qt_col);
}

/****************************************************************************
 * Driver Functions - Navigation and Display
 ****************************************************************************/

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
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

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (!table)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  *lin = table->currentRow() + 1;  // Convert to 1-based
  *col = table->currentColumn() + 1;  // Convert to 1-based
}

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
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

void iupdrvTableRedraw(Ihandle* ih)
{
  QTableWidget* table = qtTableGetWidget(ih);
  if (table)
  {
    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (iupStrBoolean(virtualmode))
    {
      // In virtual mode, repopulate all visible cells from VALUE_CB
      sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
      if (value_cb)
      {
        // Block signals during redraw, save previous state
        bool wasBlocked = table->signalsBlocked();
        table->blockSignals(true);

        for (int row = 0; row < table->rowCount(); row++)
        {
          for (int col = 0; col < table->columnCount(); col++)
          {
            QTableWidgetItem* existingItem = table->item(row, col);
            if (existingItem)
            {
              // Query VALUE_CB for cell content (1-based indices)
              char* value = value_cb(ih, row + 1, col + 1);
              if (value)
                existingItem->setText(QString::fromUtf8(value));
              else
                existingItem->setText(QString());
            }
          }
        }

        // Restore previous signal blocking state
        table->blockSignals(wasBlocked);
      }
    }

    // Reapply colors to all cells before redrawing
    qtTableReapplyAllColors(ih);

    // Then trigger visual update
    table->viewport()->update();
  }
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
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
  /* Store value in ih->data first */
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
    ih->data->sortable = 0;

  /* Apply to native widget if it exists */
  if (ih->handle)
  {
    QTableWidget* table = qtTableGetWidget(ih);
    if (table)
    {
      char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
      QHeaderView* hHeader = table->horizontalHeader();

      if (ih->data->sortable)
      {
        if (iupStrBoolean(virtualmode))
        {
          // Virtual mode - disable automatic sorting
          table->setSortingEnabled(false);
          hHeader->setSectionsClickable(true);
          hHeader->setSortIndicatorShown(true);
        }
        else
        {
          // Normal mode - enable Qt's automatic sorting
          table->setSortingEnabled(true);
          hHeader->setSectionsClickable(true);
          hHeader->setSortIndicatorShown(true);
        }
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
  /* Store value in ih->data first */
  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;

  /* Apply to native widget if it exists */
  if (ih->handle)
  {
    QTableWidget* table = qtTableGetWidget(ih);
    if (table)
    {
      QHeaderView* hHeader = table->horizontalHeader();
      if (hHeader)
        hHeader->setSectionsMovable(ih->data->allow_reorder);
    }
  }
  return 0; /* do not store in hash table */
}

static int qtTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  QTableWidget* table = qtTableGetWidget(ih);

  // First, update ih->data->user_resize flag
  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;

  if (!table)
    return 0;

  // Update resize modes for all existing columns
  QHeaderView* hHeader = table->horizontalHeader();

  for (int col = 0; col < ih->data->num_col; col++)
  {
    if (ih->data->user_resize)
    {
      // USERRESIZE=YES: Interactive mode for all columns
      hHeader->setSectionResizeMode(col, QHeaderView::Interactive);
    }
    else
    {
      // USERRESIZE=NO: Return to Fixed or ResizeToContents based on whether width was set
      if (table->columnWidth(col) > 0)
      {
        hHeader->setSectionResizeMode(col, QHeaderView::Fixed);
      }
      else
      {
        hHeader->setSectionResizeMode(col, QHeaderView::ResizeToContents);
      }
    }
  }

  return 0; /* do not store in hash table */
}

/****************************************************************************
 * Class Registration
 ****************************************************************************/

extern "C" {

int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  /* QTableView doesn't add extra border width */
  return 0;
}

void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = qtTableMapMethod;
  ic->UnMap = qtTableUnMapMethod;

  // Replace core SET handlers to update native widget
  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, qtTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, qtTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, qtTableSetUserResizeAttrib);
}

} // extern "C"

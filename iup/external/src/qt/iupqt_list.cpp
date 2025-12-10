/** \file
 * \brief List Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QComboBox>
#include <QListWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <QStringList>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QCompleter>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QApplication>
#include <QKeyEvent>

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
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"
}

#include "iupqt_drv.h"


/* Declare QPixmap* as a Qt metatype for Qt5 compatibility */
Q_DECLARE_METATYPE(QPixmap*)

/* Custom QComboBox that returns minimal sizeHint to let IUP control sizing */
class IupQtComboBox : public QComboBox
{
public:
  explicit IupQtComboBox(QWidget* parent = nullptr) : QComboBox(parent) {}

  QSize sizeHint() const override
  {
    QFontMetrics fm(font());
    int h = fm.height() + 8;  /* Minimal height for combo */
    int w = fm.horizontalAdvance('X') * 5;  /* 5 character minimum width */
    return QSize(w, h);
  }
};

/* Custom QListWidget that returns minimal sizeHint and supports drag-and-drop */
class IupQtListWidget : public QListWidget
{
private:
  Ihandle* ih;
  QPoint drag_start_pos;
  int drag_item_id;  /* 1-based item id being dragged */

public:
  explicit IupQtListWidget(Ihandle* ih_param, QWidget* parent = nullptr)
    : QListWidget(parent), ih(ih_param), drag_item_id(-1) {}

  QSize sizeHint() const override
  {
    QFontMetrics fm(font());
    int h = fm.height() * 3;  /* 3 lines minimum height */
    int w = fm.horizontalAdvance('X') * 10;  /* 10 character minimum width */
    return QSize(w, h);
  }

  void enableDragDrop()
  {
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
  }

protected:
  /* Internal drag-drop uses Qt's built-in QListWidget drag-drop instead.
   * For cross-widget drag-drop, the universal system (iupqt_dragdrop.cpp) is used. */

  void keyPressEvent(QKeyEvent* event) override
  {
    extern int iupqtKeyPressEvent(QWidget*, QKeyEvent*, Ihandle*);
    int result = iupqtKeyPressEvent(this, event, ih);

    if (result == 0)
    {
      /* Not handled by IUP - let Qt's native list widget handle it */
      QListWidget::keyPressEvent(event);
    }
    else
    {
      /* IUP handled it (returned 1) - accept the event */
      event->accept();
    }
  }

  void dropEvent(QDropEvent *event) override
  {
    /* Get the dragged item id from IUP attribute */
    int drag_id = iupAttribGetInt(ih, "_IUPLIST_DRAGITEM");

    /* If drag_id is not set, this is a universal drag-drop (from another widget)
     * In this case, the event filter (iupqt_dragdrop.cpp) has already handled it.
     * We just accept the event and return. */
    if (drag_id < 1)
    {
      event->accept();
      return;
    }

    /* Internal drag-drop: dragging within this list */

    /* Get the drop position */
    QListWidgetItem* drop_item = itemAt(event->pos());
    int drop_id;
    if (drop_item)
    {
      drop_id = row(drop_item) + 1;  /* 1-based */
    }
    else
    {
      drop_id = -1;  /* Drop at end */
    }

    /* Call the IUP drag-drop callback (uses 0-based indexing internally) */
    int is_ctrl = 0;
    if (iupListCallDragDropCb(ih, drag_id - 1, drop_id - 1, &is_ctrl) == IUP_CONTINUE)
    {
      /* Get the item data before any modifications */
      QListWidgetItem* drag_item = this->item(drag_id - 1);
      if (!drag_item)
      {
        event->ignore();
        return;
      }

      QString text = drag_item->text();
      QVariant image_var = drag_item->data(Qt::UserRole + 1);

      /* Insert the item at drop position */
      int final_drop_id;
      if (drop_id > 0 && drop_id <= count())
      {
        iupdrvListInsertItem(ih, drop_id - 1, "");  /* 0-based insert before */
        final_drop_id = drop_id - 1;  /* 0-based */
        /* Adjust drag_id if we inserted before it */
        if (drag_id > drop_id)
          drag_id++;
      }
      else
      {
        iupdrvListAppendItem(ih, "");
        final_drop_id = count() - 1;  /* 0-based, after append */
      }

      /* Set the text and image for the dropped item */
      QListWidgetItem* dropped_item = this->item(final_drop_id);
      if (dropped_item)
      {
        dropped_item->setText(text);
        dropped_item->setData(Qt::UserRole + 1, image_var);
      }

      /* Select the dropped item */
      setCurrentRow(final_drop_id);

      /* Remove the dragged item if moving (not copying) */
      if (!is_ctrl)
      {
        iupdrvListRemoveItem(ih, drag_id - 1);  /* 0-based */
      }

      event->acceptProposedAction();
    }
    else
    {
      event->ignore();
    }

    iupAttribSet(ih, "_IUPLIST_DRAGITEM", NULL);
  }

  void dragEnterEvent(QDragEnterEvent *event) override
  {
    if (event->mimeData()->hasFormat("application/x-iup-list-item"))
    {
      event->acceptProposedAction();
    }
    else
    {
      event->ignore();
    }
  }

  void dragMoveEvent(QDragMoveEvent *event) override
  {
    if (event->mimeData()->hasFormat("application/x-iup-list-item"))
    {
      event->acceptProposedAction();
    }
    else
    {
      event->ignore();
    }
  }
};

/****************************************************************************
 * Custom Item Delegate for Image Support
 ****************************************************************************/

class IupQtListItemDelegate : public QStyledItemDelegate
{
public:
  Ihandle* ih;

  IupQtListItemDelegate(Ihandle* ih_param) : ih(ih_param) {}

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override
  {
    QStyleOptionViewItem opt = option;
    int image_width = 0;

    /* Draw image if show_image is enabled */
    if (ih && ih->data->show_image)
    {
      QVariant img_var = index.data(Qt::UserRole + 1);
      if (img_var.canConvert<QPixmap*>())
      {
        QPixmap* pixmap = img_var.value<QPixmap*>();
        if (pixmap && !pixmap->isNull())
        {
          QRect rect = option.rect;
          /* Calculate available height based on font metrics, not rect height */
          QFont font = option.font;
          QFontMetrics fm(font);
          int font_height = fm.height();
          int available_height = font_height + 2 * ih->data->spacing;
          int img_x = rect.left() + ih->data->spacing;

          /* Always scale images proportionally down to fit font-based row height */
          QPixmap scaled_pixmap;
          if (pixmap->height() > available_height)
          {
            scaled_pixmap = pixmap->scaled(QSize(pixmap->width(), available_height),
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int img_y = rect.top() + (rect.height() - scaled_pixmap.height()) / 2;
            painter->drawPixmap(img_x, img_y, scaled_pixmap);
            image_width = scaled_pixmap.width() + 2 * ih->data->spacing;
          }
          else
          {
            int img_y = rect.top() + (rect.height() - pixmap->height()) / 2;
            painter->drawPixmap(img_x, img_y, *pixmap);
            image_width = pixmap->width() + 2 * ih->data->spacing;
          }

          /* Adjust the rect for text to start after the image */
          opt.rect.setLeft(opt.rect.left() + image_width);
        }
      }
    }

    /* Draw text and background with adjusted rect */
    QStyledItemDelegate::paint(painter, opt, index);
  }

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override
  {
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    if (ih && ih->data->show_image)
    {
      /* Add space for image width, but do NOT expand row height for images.
       * Images should be scaled down to fit the font-based row height.
       * Only add horizontal space for the image width. */
      size.setWidth(size.width() + ih->data->maximg_w + 2 * ih->data->spacing);
    }

    return size;
  }
};

/* Virtual List Model for VIRTUALMODE */
class IupQtVirtualListModel : public QAbstractListModel
{
public:
  Ihandle* ih;
  int count;  /* Model's own count for change notifications */

  explicit IupQtVirtualListModel(Ihandle* ih_param, QObject* parent = nullptr)
    : QAbstractListModel(parent), ih(ih_param), count(ih_param ? ih_param->data->item_count : 0) {}

  int rowCount(const QModelIndex& parent = QModelIndex()) const override
  {
    if (parent.isValid())
      return 0;
    return count;
  }

  QVariant data(const QModelIndex& index, int role) const override
  {
    if (!index.isValid() || !ih)
      return QVariant();

    int row = index.row();
    if (row < 0 || row >= count)
      return QVariant();

    if (role == Qt::DisplayRole)
    {
      char* text = iupListGetItemValueCb(ih, row + 1);  /* 1-based */
      return QString::fromUtf8(text ? text : "");
    }

    return QVariant();
  }

  void setItemCount(int new_count)
  {
    if (new_count < count)
    {
      beginRemoveRows(QModelIndex(), new_count, count - 1);
      count = new_count;
      endRemoveRows();
    }
    else if (new_count > count)
    {
      beginInsertRows(QModelIndex(), count, new_count - 1);
      count = new_count;
      endInsertRows();
    }
  }

  void refreshAll()
  {
    beginResetModel();
    endResetModel();
  }
};

/* Custom QListView for virtual mode */
class IupQtVirtualListView : public QListView
{
private:
  Ihandle* ih;

public:
  explicit IupQtVirtualListView(Ihandle* ih_param, QWidget* parent = nullptr)
    : QListView(parent), ih(ih_param) {}

  QSize sizeHint() const override
  {
    QFontMetrics fm(font());
    int h = fm.height() * 3;
    int w = fm.horizontalAdvance('X') * 10;
    return QSize(w, h);
  }

protected:
  void keyPressEvent(QKeyEvent* event) override
  {
    extern int iupqtKeyPressEvent(QWidget*, QKeyEvent*, Ihandle*);
    int result = iupqtKeyPressEvent(this, event, ih);

    if (result == 0)
      QListView::keyPressEvent(event);
    else
      event->accept();
  }
};

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  static int spacing = -1;

  if (spacing == -1)
  {
    /* Qt adds internal padding/spacing to each item beyond the text height.
     * We measure a single item's actual rendered height and compare with text height. */
    IupQtListWidget* temp_list = new IupQtListWidget(NULL);
    temp_list->setMinimumSize(0, 0);
    temp_list->addItem("X");  /* Single character */
    temp_list->show();

    /* Get text height */
    int char_height;
    iupdrvFontGetCharSize(ih, NULL, &char_height);

    /* Measure actual item height */
    int qt_item_height = 0;
    QListWidgetItem* item = temp_list->item(0);
    if (item)
    {
      QRect item_rect = temp_list->visualItemRect(item);
      qt_item_height = item_rect.height();
    }

    /* Calculate spacing: item height - text height */
    spacing = (qt_item_height > char_height) ? (qt_item_height - char_height) : 2;

    delete temp_list;
  }

  *h += spacing;
}

extern "C" void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  if (ih->data->is_dropdown)
  {
    (*x) += 2 * 12;  /* base borders/padding: 24 pixels */
    (*y) += 2 * 5;   /* vertical padding */

    if (ih->data->has_editbox)
    {
      (*x) += 30;  /* extra space for editable combo: 24+30=54 pixels */
      (*y) += 4;
    }
    else
    {
      (*x) += 10;  /* extra space for non-editable combo: 24+10=34 pixels */
      (*y) += 4;
    }
  }
  else
  {
    /* Measure list widget borders dynamically */
    static int qt_list_border_x = -1;
    static int qt_list_border_y = -1;
    static int qt_editbox_height = -1;

    /* One-time measurement of list borders */
    if (qt_list_border_x == -1)
    {
      /* Create temporary list to measure frame borders */
      IupQtListWidget* temp_list = new IupQtListWidget(NULL);
      temp_list->setMinimumSize(0, 0);
      temp_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      temp_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      temp_list->show();

      QSize widget_size = temp_list->size();
      QSize viewport_size = temp_list->viewport()->size();

      qt_list_border_x = widget_size.width() - viewport_size.width();
      qt_list_border_y = widget_size.height() - viewport_size.height();

      delete temp_list;
    }

    /* One-time measurement of edit box height */
    if (qt_editbox_height == -1)
    {
      QLineEdit* temp_edit = new QLineEdit();
      temp_edit->show();
      QSize edit_size = temp_edit->size();
      qt_editbox_height = edit_size.height();

      delete temp_edit;
    }

    /* Capture IUP's calculated width before adding borders */
    int x_before = *x;

    /* Add base list borders */
    (*x) += qt_list_border_x;
    (*y) += qt_list_border_y;

    /* Qt's QListWidget uses actual font metrics for text width, which differs from
       IUP's char_width * num_chars calculation. We need to measure the compensation. */
    static int qt_list_width_per_char = -1;
    if (qt_list_width_per_char == -1)
    {
      /* Measure width per character using Qt's actual font metrics */
      int actual_width = iupdrvFontGetStringWidth(ih, "XXXXXXXXX");  /* 9 chars */
      int iup_calculated_width = x_before;  /* IUP calculated this based on char_width */

      /* Calculate compensation per character */
      qt_list_width_per_char = (actual_width - iup_calculated_width + 8) / 9;  /* +8 for rounding */
    }

    /* Add Qt's width compensation based on the number of characters */
    int char_width, char_height;
    iupdrvFontGetCharSize(ih, &char_width, &char_height);
    int num_chars = x_before / char_width;  /* Calculate number of characters from IUP width */
    int compensation = qt_list_width_per_char * num_chars;

    (*x) += compensation;

    /* Handle EDITBOX composite widget */
    if (ih->data->has_editbox)
    {
      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

      if (visiblelines > 0)
      {
        /* VISIBLELINES includes the entry line. */
        int char_width, char_height;
        iupdrvFontGetCharSize(ih, &char_width, &char_height);
        int item_height = char_height;
        iupdrvListAddItemSpace(ih, &item_height);

        (*y) -= item_height;
      }

      /* Add entry widget height */
      (*y) += qt_editbox_height;

      /* Add scrollbar height for list part if scrollbars enabled */
      if (ih->data->sb && !visiblelines)
      {
        int sb_size = iupdrvGetScrollbarSize();
        (*y) += sb_size;
      }
    }
    else
    {
      /* Plain list with scrollbars */
      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

      /* With ScrollBarAsNeeded policy, the horizontal scrollbar appears
       * inside the viewport, taking space from content height. */
      if (ih->data->sb && !visiblelines)
      {
        int sb_size = iupdrvGetScrollbarSize();
        (*y) += sb_size;
      }
    }
  }
}

extern "C" int iupdrvListGetCount(Ihandle* ih)
{
  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    return combo->count();
  }
  else if (ih->data->has_editbox)
  {
    /* Composite widget: get list from stored attribute */
    QListWidget* list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
    if (list)
      return list->count();
    return 0;
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    return list->count();
  }
}

extern "C" void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    combo->addItem(QString::fromUtf8(value));
  }
  else if (ih->data->has_editbox)
  {
    /* Composite widget: get list from stored attribute */
    QListWidget* list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
    if (list)
      list->addItem(QString::fromUtf8(value));
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    list->addItem(QString::fromUtf8(value));
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

extern "C" void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    combo->insertItem(pos, QString::fromUtf8(value));
  }
  else if (ih->data->has_editbox)
  {
    /* Composite widget: get list from stored attribute */
    QListWidget* list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
    if (list)
      list->insertItem(pos, QString::fromUtf8(value));
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    list->insertItem(pos, QString::fromUtf8(value));
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  iupListUpdateOldValue(ih, pos, 0);
}


extern "C" void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;

    /* Check if removing current item */
    if (!ih->data->has_editbox)
    {
      int curpos = combo->currentIndex();
      if (pos == curpos)
      {
        if (curpos > 0)
          curpos--;
        else
        {
          curpos = 1;
          if (combo->count() == 1)
            curpos = -1;
        }

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        combo->setCurrentIndex(curpos);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      }
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    combo->removeItem(pos);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  }
  else if (ih->data->has_editbox)
  {
    /* Composite widget: get list from stored attribute */
    QListWidget* list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
    if (list)
    {
      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
      delete list->takeItem(pos);
      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
    }
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    delete list->takeItem(pos);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  }

  iupListUpdateOldValue(ih, pos, 1);
}

extern "C" void iupdrvListRemoveAllItems(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    combo->clear();
  }
  else if (ih->data->has_editbox)
  {
    /* Composite widget: get list from stored attribute */
    QListWidget* list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
    if (list)
      list->clear();
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    list->clear();
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

extern "C" int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  /* Qt list doesn't support setting image handles directly in this way.
   * Images are managed through the IMAGE attribute with qtListSetImageAttrib.
   * This function is called during drag-drop but Qt handles this differently. */
  (void)ih;
  (void)id;
  (void)hImage;
  return 0;
}

extern "C" void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;

    if (!combo || id < 1 || id > combo->count())
    {
      return NULL;
    }

    QVariant img_var = combo->itemData(id - 1, Qt::UserRole + 1);

    if (img_var.canConvert<QPixmap*>())
    {
      QPixmap* pixmap = img_var.value<QPixmap*>();
      return (void*)pixmap;
    }
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;

    if (ih->data->has_editbox)
      list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");

    if (!list || id < 1 || id > list->count())
      return NULL;

    QListWidgetItem* item = list->item(id - 1);
    if (!item)
      return NULL;

    QVariant img_var = item->data(Qt::UserRole + 1);
    if (img_var.canConvert<QPixmap*>())
    {
      QPixmap* pixmap = img_var.value<QPixmap*>();
      return (void*)pixmap;
    }
  }

  return NULL;
}

extern "C" void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  if (!ih->data->is_virtual)
    return;

  IupQtVirtualListModel* model = (IupQtVirtualListModel*)iupAttribGet(ih, "_IUPQT_VIRTUAL_MODEL");
  if (model)
    model->setItemCount(count);
}

/****************************************************************************
 * Attribute Getters/Setters
 ****************************************************************************/

static char* qtListGetIdValueAttrib(Ihandle* ih, int id)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos >= 0)
  {
    if (ih->data->is_dropdown)
    {
      QComboBox* combo = (QComboBox*)ih->handle;
      if (pos < combo->count())
      {
        QString text = combo->itemText(pos);
        return iupStrReturnStr(text.toUtf8().constData());
      }
    }
    else
    {
      QListWidget* list;
      if (ih->data->has_editbox)
        list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
      else
        list = (QListWidget*)ih->handle;

      if (list && pos < list->count())
      {
        QListWidgetItem* item = list->item(pos);
        if (item)
        {
          QString text = item->text();
          return iupStrReturnStr(text.toUtf8().constData());
        }
      }
    }
  }
  return NULL;
}

static char* qtListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    QLineEdit* edit = NULL;
    if (ih->data->is_dropdown)
    {
      /* Dropdown with editbox */
      QComboBox* combo = (QComboBox*)ih->handle;
      edit = combo->lineEdit();
    }
    else
    {
      /* Composite widget: editbox without dropdown */
      edit = (QLineEdit*)iupAttribGet(ih, "_IUPQT_EDIT");
    }

    if (edit)
    {
      QString text = edit->text();
      return iupStrReturnStr(text.toUtf8().constData());
    }
  }
  else if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    int pos = combo->currentIndex();
    return iupStrReturnInt(pos + 1);  /* IUP starts at 1 */
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;

    if (!ih->data->is_multiple)
    {
      int row = list->currentRow();
      if (row >= 0)
        return iupStrReturnInt(row + 1);  /* IUP starts at 1 */
    }
    else
    {
      /* Multiple selection - return string with +/- for each item */
      int count = list->count();
      char* str = iupStrGetMemory(count + 1);
      memset(str, '-', count);
      str[count] = 0;

      for (int i = 0; i < count; i++)
      {
        QListWidgetItem* item = list->item(i);
        if (item && item->isSelected())
          str[i] = '+';
      }
      return str;
    }
  }

  return NULL;
}

static int qtListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    QLineEdit* edit = NULL;
    if (ih->data->is_dropdown)
    {
      /* Dropdown with editbox */
      QComboBox* combo = (QComboBox*)ih->handle;
      if (!combo || !combo->isEditable())
        return 0;
      edit = combo->lineEdit();
    }
    else
    {
      /* Composite widget: editbox without dropdown */
      edit = (QLineEdit*)iupAttribGet(ih, "_IUPQT_EDIT");
    }

    if (edit)
    {
      iupAttribSet(ih, "_IUPQT_DISABLE_TEXT_CB", "1");
      edit->setText(QString::fromUtf8(value ? value : ""));
      iupAttribSet(ih, "_IUPQT_DISABLE_TEXT_CB", NULL);
    }
  }
  else if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    int pos;

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

    if (iupStrToInt(value, &pos) == 1 && (pos > 0 && pos <= combo->count()))
    {
      combo->setCurrentIndex(pos - 1);  /* IUP starts at 1 */
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    }
    else
    {
      combo->setCurrentIndex(-1);
      iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  }
  else
  {
    QListWidget* list;
    if (ih->data->has_editbox)
      list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
    else
      list = (QListWidget*)ih->handle;

    if (!list)
      return 0;

    if (!ih->data->is_multiple)
    {
      int pos;

      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

      if (iupStrToInt(value, &pos) == 1 && pos > 0)
      {
        list->setCurrentRow(pos - 1);  /* IUP starts at 1 */
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
      }
      else
      {
        list->setCurrentRow(-1);
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
      }

      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
    }
    else
    {
      /* Multiple selection */
      int i, len, count;

      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

      /* Clear all selections */
      list->clearSelection();

      if (!value)
      {
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        return 0;
      }

      len = (int)strlen(value);
      count = list->count();
      if (len < count)
        count = len;

      /* Update selection */
      for (i = 0; i < count; i++)
      {
        if (value[i] == '+')
        {
          QListWidgetItem* item = list->item(i);
          if (item)
            item->setSelected(true);
        }
      }

      iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
    }
  }

  return 0;
}

static int qtListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos < 0)
    return 0;

  QPixmap* pixmap = (QPixmap*)iupImageGetImage(value, ih, 0, NULL);

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    if (pos < combo->count())
    {
      if (pixmap && !pixmap->isNull())
      {
        combo->setItemIcon(pos, QIcon(*pixmap));
        /* Store pixmap pointer in itemData for iupdrvListGetImageHandle() */
        combo->setItemData(pos, QVariant::fromValue(pixmap), Qt::UserRole + 1);
      }
      else
      {
        combo->setItemIcon(pos, QIcon());
        combo->setItemData(pos, QVariant(), Qt::UserRole + 1);
      }

      /* Update max image size */
      if (pixmap)
      {
        if (pixmap->width() > ih->data->maximg_w)
          ih->data->maximg_w = pixmap->width();
        if (pixmap->height() > ih->data->maximg_h)
          ih->data->maximg_h = pixmap->height();
      }
    }
  }
  else
  {
    QListWidget* list;
    if (ih->data->has_editbox)
      list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
    else
      list = (QListWidget*)ih->handle;

    if (list)
    {
      QListWidgetItem* item = list->item(pos);
      if (item)
      {
        if (pixmap && !pixmap->isNull())
        {
          /* Don't use setIcon() - the custom delegate handles all image rendering
           * to support proper scaling. Only store the pixmap pointer in user data. */
          item->setData(Qt::UserRole + 1, QVariant::fromValue(pixmap));
        }
        else
        {
          item->setData(Qt::UserRole + 1, QVariant());
        }

        /* Update max image size */
        if (pixmap)
        {
          if (pixmap->width() > ih->data->maximg_w)
            ih->data->maximg_w = pixmap->width();
          if (pixmap->height() > ih->data->maximg_h)
            ih->data->maximg_h = pixmap->height();
        }
      }
    }
  }

  return 1;
}

static int qtListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos;
    if (iupStrToInt(value, &pos) && pos > 0)
    {
      QListWidget* list = (QListWidget*)ih->handle;
      list->scrollToItem(list->item(pos - 1), QAbstractItemView::PositionAtTop);
    }
  }
  return 0;
}

static int qtListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    if (iupStrBoolean(value))
      combo->showPopup();
    else
      combo->hidePopup();
  }
  return 0;
}

static int qtListSetVisibleItemsAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
  {
    int count;
    if (iupStrToInt(value, &count) && count > 0)
    {
      QComboBox* combo = (QComboBox*)ih->handle;
      combo->setMaxVisibleItems(count);
    }
  }
  return 1;
}

static int qtListSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit)
    {
      if (value)
        edit->setPlaceholderText(QString::fromUtf8(value));
      else
        edit->setPlaceholderText(QString());
    }
  }
  return 1;
}

static int qtListSetDropExpandAttrib(Ihandle* ih, const char* value)
{
  /* Qt doesn't have direct support for expanding dropdown width */
  /* Would need custom implementation with QListView */
  (void)ih;
  (void)value;
  return 1;
}

static char* qtListGetSelectedTextAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    QLineEdit* edit = NULL;
    if (ih->data->is_dropdown)
    {
      QComboBox* combo = (QComboBox*)ih->handle;
      edit = combo->lineEdit();
    }
    else
    {
      edit = (QLineEdit*)iupAttribGet(ih, "_IUPQT_EDIT");
    }

    if (edit)
    {
      QString selected = edit->selectedText();
      return iupStrReturnStr(selected.toUtf8().constData());
    }
  }
  return NULL;
}

static int qtListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit && edit->hasSelectedText())
    {
      edit->insert(QString::fromUtf8(value ? value : ""));
    }
  }
  return 0;
}

static char* qtListGetSelectionAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit)
    {
      int start = edit->selectionStart();
      int len = edit->selectedText().length();
      if (len > 0)
        return iupStrReturnIntInt(start + 1, start + len, ':');  /* IUP starts at 1 */
    }
  }
  return NULL;
}

static int qtListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    int start = 1, end = 1;
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();

    if (!edit)
      return 0;

    if (!value || iupStrEqualNoCase(value, "NONE"))
    {
      edit->deselect();
      return 0;
    }

    if (iupStrEqualNoCase(value, "ALL"))
    {
      edit->selectAll();
      return 0;
    }

    if (iupStrToIntInt(value, &start, &end, ':') == 2)
    {
      if (start < 1) start = 1;
      if (end < 1) end = 1;

      start--; /* IUP starts at 1 */
      end--;

      if (end < start)
        end = start;

      edit->setSelection(start, end - start + 1);
    }
  }
  return 0;
}

static char* qtListGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit)
      return iupStrReturnInt(edit->cursorPosition() + 1);  /* IUP starts at 1 */
  }
  return NULL;
}

static int qtListSetCaretAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    int pos;
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();

    if (!edit)
      return 0;

    if (iupStrToInt(value, &pos))
    {
      if (pos < 1) pos = 1;
      pos--;  /* IUP starts at 1 */

      edit->setCursorPosition(pos);
    }
  }
  return 0;
}

static int qtListSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit)
      edit->insert(QString::fromUtf8(value ? value : ""));
  }
  return 0;
}

static int qtListSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit)
    {
      QString text = edit->text();
      text.append(QString::fromUtf8(value ? value : ""));
      edit->setText(text);
    }
  }
  return 0;
}

static char* qtListGetReadOnlyAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit)
      return iupStrReturnBoolean(edit->isReadOnly());
  }
  return NULL;
}

static char* qtListGetScrollVisibleAttrib(Ihandle* ih)
{
  if (ih->data->is_dropdown)
    return nullptr;

  IupQtListWidget* list = (IupQtListWidget*)ih->handle;

  int horiz_visible = 0, vert_visible = 0;

  QScrollBar* hsb = list->horizontalScrollBar();
  QScrollBar* vsb = list->verticalScrollBar();

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

static int qtListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();
    if (edit)
      edit->setReadOnly(iupStrBoolean(value));
  }
  return 0;
}

static int qtListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    int max;
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();

    if (!edit)
      return 0;

    if (iupStrToInt(value, &max))
    {
      if (max < 0) max = 0;
      edit->setMaxLength(max);
    }
  }
  return 0;
}

static int qtListSetClipboardAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();

    if (!edit)
      return 0;

    if (iupStrEqualNoCase(value, "COPY"))
      edit->copy();
    else if (iupStrEqualNoCase(value, "CUT"))
      edit->cut();
    else if (iupStrEqualNoCase(value, "PASTE"))
      edit->paste();
    else if (iupStrEqualNoCase(value, "CLEAR"))
      edit->clear();
  }
  return 0;
}

static int qtListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox && ih->data->is_dropdown)
  {
    int pos;
    QComboBox* combo = (QComboBox*)ih->handle;
    QLineEdit* edit = combo->lineEdit();

    if (!edit)
      return 0;

    if (iupStrToInt(value, &pos))
    {
      if (pos < 1) pos = 1;
      pos--;  /* IUP starts at 1 */

      edit->setCursorPosition(pos);
      /* LineEdit doesn't have direct scroll-to, but cursor position handles it */
    }
  }
  return 0;
}

static int qtListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToInt(value, &ih->data->spacing))
  {
    if (ih->handle && ih->data->show_image)
    {
      /* Update delegate to use new spacing */
      if (!ih->data->is_dropdown)
      {
        QListWidget* list = (QListWidget*)ih->handle;
        IupQtListItemDelegate* delegate = new IupQtListItemDelegate(ih);
        list->setItemDelegate(delegate);
      }
    }
    return 0;
  }
  return 1;
}

static int qtListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    if (ih->data->is_dropdown)
    {
      QComboBox* combo = (QComboBox*)ih->handle;
      /* Qt doesn't have direct padding control for combo box */
      /* Could use style sheets */
    }
    else
    {
      QListWidget* list = (QListWidget*)ih->handle;
      list->setContentsMargins(ih->data->horiz_padding, ih->data->vert_padding,
                               ih->data->horiz_padding, ih->data->vert_padding);
    }
    return 0;
  }
  return 1;
}

static int qtListSetFilterAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown && ih->data->has_editbox)
  {
    QComboBox* combo = (QComboBox*)ih->handle;

    if (value && iupStrBoolean(value))
    {
      /* Enable filtering */
      combo->setInsertPolicy(QComboBox::NoInsert);
      QCompleter* completer = new QCompleter(combo->model(), combo);
      completer->setCompletionMode(QCompleter::PopupCompletion);
      completer->setCaseSensitivity(Qt::CaseInsensitive);
      combo->setCompleter(completer);
    }
    else
    {
      combo->setCompleter(nullptr);
    }
  }
  return 1;
}

static int qtListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QPalette palette = combo->palette();
    palette.setColor(QPalette::Base, QColor(r, g, b));
    combo->setPalette(palette);
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    QPalette palette = list->palette();
    palette.setColor(QPalette::Base, QColor(r, g, b));
    list->setPalette(palette);
  }

  return 1;
}

static int qtListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    QPalette palette = combo->palette();
    palette.setColor(QPalette::Text, QColor(r, g, b));
    combo->setPalette(palette);
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    QPalette palette = list->palette();
    palette.setColor(QPalette::Text, QColor(r, g, b));
    list->setPalette(palette);
  }

  return 1;
}

static int qtListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    if (ih->data->is_dropdown)
    {
      QComboBox* combo = (QComboBox*)ih->handle;
      iupqtUpdateWidgetFont(ih, combo);
    }
    else
    {
      QListWidget* list = (QListWidget*)ih->handle;
      iupqtUpdateWidgetFont(ih, list);
    }
  }

  return 1;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qtListComboBoxChanged(QComboBox* combo, Ihandle* ih, int index)
{
  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int pos = index + 1;  /* IUP starts at 1 */
    cb(ih, (char*)combo->itemText(index).toUtf8().constData(), pos, 1);
  }

  iupBaseCallValueChangedCb(ih);
}

static void qtListWidgetItemSelectionChanged(QListWidget* list, Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  if (!ih->data->is_multiple)
  {
    int row = list->currentRow();
    if (row >= 0)
    {
      IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
      if (cb)
      {
        QListWidgetItem* item = list->item(row);
        int pos = row + 1;  /* IUP starts at 1 */
        cb(ih, (char*)item->text().toUtf8().constData(), pos, 1);
      }
    }
  }
  else
  {
    /* Multiple selection callback */
    IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
    if (multi_cb)
    {
      char* value = qtListGetValueAttrib(ih);
      multi_cb(ih, value);
    }
  }

  iupBaseCallValueChangedCb(ih);
}

static void qtListEditTextChanged(QLineEdit* edit, Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPQT_DISABLE_TEXT_CB"))
    return;

  IFns cb = (IFns)IupGetCallback(ih, "EDIT_CB");
  if (cb)
  {
    QString text = edit->text();
    cb(ih, (char*)text.toUtf8().constData());
  }

  iupBaseCallValueChangedCb(ih);
}

static void qtListCaretChanged(QLineEdit* edit, Ihandle* ih)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "CARET_CB");
  if (cb)
  {
    int pos = edit->cursorPosition() + 1;  /* IUP starts at 1 */
    int col = 0;  /* Single line */
    cb(ih, pos, col);
  }
}

/****************************************************************************
 * Convert XY to Position - for drag-drop support
 ****************************************************************************/

static int qtListConvertXYToPos(Ihandle* ih, int x, int y)
{
  if (!ih->data->is_dropdown)
  {
    QListWidget* list = (QListWidget*)ih->handle;
    QListWidgetItem* item = list->itemAt(QPoint(x, y));
    if (item)
    {
      int pos = list->row(item) + 1;  /* IUP starts at 1 */
      return pos;
    }
  }

  return -1;
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtListMapMethod(Ihandle* ih)
{
  if (ih->data->is_dropdown)
  {
    /* Create ComboBox for dropdown lists */
    QComboBox* combo = new QComboBox();

    if (!combo)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)combo;

    /* Remove minimum size constraints - similar to GTK's iupgtkClearSizeStyleCSS
     * This allows IUP's layout system to control sizing without Qt enforcing minimums */
    combo->setMinimumSize(0, 0);

    /* Set editable mode */
    combo->setEditable(ih->data->has_editbox ? true : false);

    /* Set visible items */
    char* value = iupAttribGetStr(ih, "VISIBLEITEMS");
    if (value)
      qtListSetVisibleItemsAttrib(ih, value);

    /* Set size policy - Expanding allows widget to grow beyond minimal sizeHint
     * IUP's layout system will set the actual size based on natural size calculation */
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    /* Connect signals */
    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [combo, ih](int index) {
      qtListComboBoxChanged(combo, ih, index);
    });

    /* For editbox */
    if (ih->data->has_editbox)
    {
      QLineEdit* edit = combo->lineEdit();
      if (edit)
      {
        QObject::connect(edit, &QLineEdit::textChanged, [edit, ih]() {
          qtListEditTextChanged(edit, ih);
        });

        QObject::connect(edit, &QLineEdit::cursorPositionChanged, [edit, ih]() {
          qtListCaretChanged(edit, ih);
        });
      }
    }

    /* Set initial items */
    iupListSetInitialItems(ih);

    /* Qt automatically selects the first item when items are added to a ComboBox.
     * Windows/Motif ComboBox starts with no selection. Clear selection to match. */
    combo->setCurrentIndex(-1);
  }
  else if (ih->data->has_editbox)
  {
    /* EDITBOX without DROPDOWN (CBS_SIMPLE style)
     * Create a composite widget with QLineEdit on top and QListWidget below
     * This matches GTK and Windows CBS_SIMPLE behavior */

    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    /* Create edit box */
    QLineEdit* edit = new QLineEdit();
    /* Set fixed height for edit box to match IUP's edit_line_size calculation */
    QFontMetrics fm(edit->font());
    int edit_height = fm.height() + 8;  /* Add some padding like IUP does */
    edit->setFixedHeight(edit_height);
    layout->addWidget(edit, 0);  /* 0 stretch - fixed height */

    /* Create list widget with drag-drop support */
    IupQtListWidget* list = new IupQtListWidget(ih);
    list->setMinimumSize(0, 0);  /* Remove size constraints like GTK */
    layout->addWidget(list, 1);  /* 1 stretch - take remaining space */

    /* Store the container as the main handle */
    ih->handle = (InativeHandle*)container;

    /* Store references to the edit and list for attribute access */
    iupAttribSet(ih, "_IUPQT_EDIT", (char*)edit);
    iupAttribSet(ih, "_IUPQT_LIST", (char*)list);

    /* Set selection mode */
    list->setSelectionMode(QAbstractItemView::SingleSelection);

    /* Set scrollbar policy based on SCROLLBAR attribute
     * Default is to show scrollbars when needed (AUTOHIDE=YES) */
    if (ih->data->sb)
    {
      if (iupAttribGetBoolean(ih, "AUTOHIDE"))
      {
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
      }
      else
      {
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      }
    }
    else
    {
      list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    /* Set size policy to allow the list to expand naturally */
    list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /* For EDITBOX with VISIBLELINES, constrain the list height to show exactly (VISIBLELINES-1) items */
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    if (visiblelines > 0)
    {
      /* Calculate list height: items + borders */
      int char_width, char_height;
      iupdrvFontGetCharSize(ih, &char_width, &char_height);
      int item_height = char_height;
      iupdrvListAddItemSpace(ih, &item_height);

      int num_items_in_list = visiblelines - 1;  /* Subtract 1 for the entry */
      int list_content_height = num_items_in_list * item_height;

      /* Add list borders */
      int list_total_height = list_content_height + 4;

      list->setMaximumHeight(list_total_height);
    }

    /* Connect edit box signals */
    QObject::connect(edit, &QLineEdit::textChanged, [edit, ih]() {
      qtListEditTextChanged(edit, ih);
    });

    QObject::connect(edit, &QLineEdit::cursorPositionChanged, [edit, ih]() {
      qtListCaretChanged(edit, ih);
    });

    /* Connect list selection changed */
    QObject::connect(list, &QListWidget::itemSelectionChanged, [list, edit, ih]() {
      qtListWidgetItemSelectionChanged(list, ih);

      /* Update edit box with selected item text */
      if (!iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
      {
        QListWidgetItem* item = list->currentItem();
        if (item)
        {
          iupAttribSet(ih, "_IUPQT_DISABLE_TEXT_CB", "1");
          edit->setText(item->text());
          iupAttribSet(ih, "_IUPQT_DISABLE_TEXT_CB", NULL);
        }
      }
    });

    /* Setup image delegate if needed */
    if (ih->data->show_image)
    {
      IupQtListItemDelegate* delegate = new IupQtListItemDelegate(ih);
      list->setItemDelegate(delegate);
    }

    /* Set initial items */
    iupListSetInitialItems(ih);
  }
  else if (ih->data->is_virtual)
  {
    /* Virtual mode: Create QListView with custom model */
    IupQtVirtualListView* view = new IupQtVirtualListView(ih);

    if (!view)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)view;

    /* Create and set the virtual model */
    IupQtVirtualListModel* model = new IupQtVirtualListModel(ih, view);
    view->setModel(model);
    iupAttribSet(ih, "_IUPQT_VIRTUAL_MODEL", (char*)model);

    /* Enable uniform item sizes. This tells Qt all items have the same size, avoiding per-item measurement. */
    view->setUniformItemSizes(true);

    /* Remove minimum size constraints */
    view->setMinimumSize(0, 0);

    /* Set selection mode */
    if (ih->data->is_multiple)
      view->setSelectionMode(QAbstractItemView::MultiSelection);
    else
      view->setSelectionMode(QAbstractItemView::SingleSelection);

    /* Set scrollbar policy based on AUTOHIDE attribute */
    int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");

    if (ih->data->sb)
    {
      if (autohide)
      {
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
      }
      else
      {
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      }
    }
    else
    {
      view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    /* Connect selection signals */
    QObject::connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
      [view, ih](const QItemSelection& selected, const QItemSelection& deselected) {
        (void)selected;
        (void)deselected;
        if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
          return;

        QModelIndexList indexes = view->selectionModel()->selectedIndexes();
        if (!indexes.isEmpty())
        {
          int pos = indexes.first().row() + 1;  /* 1-based */
          IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
          if (cb)
          {
            char* text = iupListGetItemValueCb(ih, pos);
            cb(ih, text ? text : (char*)"", pos, 1);
          }
          iupBaseCallValueChangedCb(ih);
        }
      });
  }
  else
  {
    /* Create ListWidget for plain list with drag-drop support */
    IupQtListWidget* list = new IupQtListWidget(ih);

    if (!list)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)list;

    /* Remove minimum size constraints - similar to GTK's iupgtkClearSizeStyleCSS */
    list->setMinimumSize(0, 0);

    /* Set selection mode */
    if (ih->data->is_multiple)
      list->setSelectionMode(QAbstractItemView::MultiSelection);
    else
      list->setSelectionMode(QAbstractItemView::SingleSelection);

    /* Set scrollbar policy based on AUTOHIDE attribute */
    int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");

    if (ih->data->sb)
    {
      if (autohide)
      {
        /* Auto-hide scrollbars - show only when needed */
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
      }
      else
      {
        /* Always show scrollbars */
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      }
    }
    else
    {
      /* No scrollbars */
      list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    /* Setup image delegate if needed */
    if (ih->data->show_image)
    {
      IupQtListItemDelegate* delegate = new IupQtListItemDelegate(ih);
      list->setItemDelegate(delegate);
    }

    /* Connect signals */
    QObject::connect(list, &QListWidget::itemSelectionChanged, [list, ih]() {
      qtListWidgetItemSelectionChanged(list, ih);
    });

    /* Set initial items */
    iupListSetInitialItems(ih);

    /* Enable internal drag and drop support - same conditions as GTK */
    if (ih->data->show_dragdrop && !ih->data->is_dropdown && !ih->data->is_multiple)
    {
      list->enableDragDrop();
    }
  }

  /* Add to parent */
  iupqtAddToParent(ih);

  /* Configure for DRAG&DROP of files */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  /* Configure focus */
  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    if (ih->data->is_dropdown)
      iupqtSetCanFocus((QWidget*)ih->handle, 0);
    else
      iupqtSetCanFocus((QWidget*)ih->handle, 0);
  }

  /* Register XY to position converter for drag-drop support */
  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)qtListConvertXYToPos);

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvListInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtListMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, qtListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, qtListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FONT", NULL, qtListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* IupList only */
  iupClassRegisterAttributeId(ic, "IDVALUE", qtListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", qtListGetValueAttrib, qtListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, qtListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, qtListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, qtListSetVisibleItemsAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, qtListSetDropExpandAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, qtListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, qtListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, qtListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SCROLLBAR", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);

  /* Editbox attributes */
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", qtListGetSelectedTextAttrib, qtListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", qtListGetSelectionAttrib, qtListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", qtListGetCaretAttrib, qtListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, qtListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, qtListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", qtListGetReadOnlyAttrib, qtListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, qtListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, qtListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, qtListSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, qtListSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", qtListGetScrollVisibleAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* Image support */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, qtListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
}

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
#include <QAbstractItemView>
#include <QScrollBar>
#include <QCompleter>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QIcon>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QApplication>
#include <QStyle>

#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_list.h"
}

#include "iupqt_drv.h"


/* Declare QPixmap* as a Qt metatype for Qt5 compatibility */
Q_DECLARE_METATYPE(QPixmap*)

/* Custom QListWidget that returns minimal sizeHint and supports drag-and-drop */
class IupQtListWidget : public QListWidget
{
private:
  Ihandle* ih;
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
  void focusInEvent(QFocusEvent* event) override
  {
    QListWidget::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, ih);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QListWidget::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, ih);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    iupqtMouseButtonEvent(this, event, ih);
    QListWidget::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QListWidget::mouseReleaseEvent(event);
    iupqtMouseButtonEvent(this, event, ih);
  }

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QListWidgetItem* drop_item = itemAt(event->position().toPoint());
#else
    QListWidgetItem* drop_item = itemAt(event->pos());
#endif
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

          /* Scale images proportionally down to fit font-based row height if FITIMAGE=YES */
          QPixmap scaled_pixmap;
          if (ih->data->fit_image && pixmap->height() > available_height)
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

    /* Use font-based height with small fixed padding for consistency */
    QFontMetrics fm(option.font);
    int normalized_height = fm.height() + 4;  /* font height + 4px padding (2px top + 2px bottom) */
    if (size.height() > normalized_height)
      size.setHeight(normalized_height);

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

    if (role == Qt::DecorationRole && ih->data->show_image)
    {
      char* image_name = iupListGetItemImageCb(ih, row + 1);  /* 1-based */
      if (image_name)
      {
        void* handle = iupImageGetImage(image_name, ih, 0, NULL);
        if (handle)
        {
          QPixmap* pixmap = static_cast<QPixmap*>(handle);
          return QIcon(*pixmap);
        }
      }
      return QVariant();
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
  void focusInEvent(QFocusEvent* event) override
  {
    QListView::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, ih);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QListView::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, ih);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    iupqtMouseButtonEvent(this, event, ih);
    QListView::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QListView::mouseReleaseEvent(event);
    iupqtMouseButtonEvent(this, event, ih);
  }

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

/* Cached measurements for list widget metrics */
static int iupqt_list_item_space = -1;
static int iupqt_list_row_height = -1;

/* Cached item horizontal padding */
static int iupqt_list_item_padding_x = -1;

static void iupqtListMeasureItemMetrics(Ihandle* ih)
{
  if (iupqt_list_item_space < 0)
  {
    int char_height;
    iupdrvFontGetCharSize(ih, NULL, &char_height);

    QListWidget temp_list;
    temp_list.setMinimumSize(0, 0);
    temp_list.addItem("WWWWWWWWWW");

    int actual_item_width = temp_list.sizeHintForColumn(0);

    QFontMetrics fm(temp_list.font());
    int text_width = fm.horizontalAdvance("WWWWWWWWWW");

    iupqt_list_item_padding_x = actual_item_width - text_width;
    if (iupqt_list_item_padding_x < 0) iupqt_list_item_padding_x = 0;

    iupqt_list_item_space = 4;
    iupqt_list_row_height = char_height + iupqt_list_item_space;
  }
}

extern "C" IUP_SDK_API void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  iupqtListMeasureItemMetrics(ih);
  *h += iupqt_list_item_space;
}

/* Cached measurements for list borders */
static int iupqt_list_border_x = -1;
static int iupqt_list_border_y = -1;
static int iupqt_dropdown_border_x = -1;
static int iupqt_dropdown_border_y = -1;
static int iupqt_editbox_height = -1;

static void iupqtListMeasureBorders(Ihandle* ih)
{
  if (iupqt_list_border_x < 0)
  {
    /* Parent required so QMacStyle PM_DefaultFrameWidth is non-zero. */
    QWidget temp_parent;
    QListWidget temp_list(&temp_parent);
    temp_list.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    temp_list.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    int frame_width = temp_list.frameWidth();

    iupqt_list_border_x = 2 * frame_width;
    iupqt_list_border_y = 2 * frame_width;
  }

  if (iupqt_dropdown_border_x < 0)
  {
    QWidget temp_parent;
    QComboBox temp_combo(&temp_parent);
    temp_combo.addItem("X");

    QSize combo_size = temp_combo.sizeHint();
    QFontMetrics fm(temp_combo.font());
    int text_height = fm.height();

    QStyleOptionComboBox opt;
    opt.initFrom(&temp_combo);
    opt.subControls = QStyle::SC_All;
    opt.editable = false;
    opt.rect = QRect(0, 0, combo_size.width(), combo_size.height());

    QStyle* style = temp_combo.style();
    QRect editRect = style->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, &temp_combo);

    int sb_size = iupdrvGetScrollbarSize();
    int total_decor = combo_size.width() - editRect.width();

    iupqt_dropdown_border_x = total_decor - sb_size + fm.horizontalAdvance('X');

    /* Windows11 style's CE_ComboBoxLabel shrinks the combo rect by 4px per side
       (newOption.rect.adjust(4, 0, -4, 0)) before delegating to QCommonStyle,
       which then applies its own 2px-per-side drawItemText adjustment. Neither
       adjustment is captured by SC_ComboBoxEditField, so SC_remove under-reports
       the real inset by 12px. See /tmp/qt/src/plugins/styles/modernwindows/qwindows11style.cpp. */
    QString style_name = style->objectName();
    if (style_name.compare(QLatin1String("windows11"), Qt::CaseInsensitive) == 0)
    {
      iupqt_dropdown_border_x += 12;
    }

    if (iupqt_dropdown_border_x < 0) iupqt_dropdown_border_x = 0;

    iupqt_dropdown_border_y = combo_size.height() - text_height;
    if (iupqt_dropdown_border_y < 0) iupqt_dropdown_border_y = 0;
  }

  if (iupqt_editbox_height < 0)
  {
    QLineEdit temp_edit;
    temp_edit.setFrame(true);
    iupqt_editbox_height = temp_edit.sizeHint().height();
  }

  (void)ih;
}

extern "C" IUP_SDK_API void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  iupqtListMeasureBorders(ih);

  if (ih->data->is_dropdown)
  {
    (*x) += iupqt_dropdown_border_x;
    (*y) += iupqt_dropdown_border_y;
  }
  else
  {
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

    /* Add base list borders + item horizontal padding */
    (*x) += iupqt_list_border_x + iupqt_list_item_padding_x;
    (*y) += iupqt_list_border_y;

    /* Handle EDITBOX composite widget */
    if (ih->data->has_editbox)
    {
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
      (*y) += iupqt_editbox_height;
    }
  }
}

static QListWidget* qtListGetListWidget(Ihandle* ih)
{
  if (ih->data->has_editbox && !ih->data->is_dropdown)
    return (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
  else if (!ih->data->is_dropdown)
    return (QListWidget*)ih->handle;
  return nullptr;
}

extern "C" IUP_SDK_API int iupdrvListGetCount(Ihandle* ih)
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

extern "C" IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
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

extern "C" IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
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


extern "C" IUP_SDK_API void iupdrvListRemoveItem(Ihandle* ih, int pos)
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

extern "C" IUP_SDK_API void iupdrvListRemoveAllItems(Ihandle* ih)
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

extern "C" IUP_SDK_API int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  QPixmap* pixmap = (QPixmap*)hImage;

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    if (!combo || id < 0 || id >= combo->count())
      return 0;

    if (pixmap)
      combo->setItemData(id, QVariant::fromValue(pixmap), Qt::UserRole + 1);
    else
      combo->setItemData(id, QVariant(), Qt::UserRole + 1);
  }
  else
  {
    QListWidget* list = (QListWidget*)ih->handle;
    if (ih->data->has_editbox)
      list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");

    if (!list || id < 0 || id >= list->count())
      return 0;

    QListWidgetItem* item = list->item(id);
    if (item)
    {
      if (pixmap)
        item->setData(Qt::UserRole + 1, QVariant::fromValue(pixmap));
      else
        item->setData(Qt::UserRole + 1, QVariant());
    }
  }

  return 1;
}

extern "C" IUP_SDK_API void* iupdrvListGetImageHandle(Ihandle* ih, int id)
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

extern "C" IUP_SDK_API void iupdrvListSetItemCount(Ihandle* ih, int count)
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
    else if (!ih->data->is_virtual)
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

static char* qtListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  return (char*)iupdrvListGetImageHandle(ih, id);
}

static int qtListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos;
    if (iupStrToInt(value, &pos) && pos > 0)
    {
      QListWidget* list;
      if (ih->data->has_editbox)
        list = (QListWidget*)iupAttribGet(ih, "_IUPQT_LIST");
      else
        list = (QListWidget*)ih->handle;

      if (list)
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


static QLineEdit* qtListGetEditBox(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  if (ih->data->is_dropdown)
  {
    QComboBox* combo = (QComboBox*)ih->handle;
    return combo->lineEdit();
  }
  else
  {
    return (QLineEdit*)iupAttribGet(ih, "_IUPQT_EDIT");
  }
}

static char* qtListGetSelectedTextAttrib(Ihandle* ih)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit)
  {
    QString selected = edit->selectedText();
    return iupStrReturnStr(selected.toUtf8().constData());
  }
  return NULL;
}

static int qtListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit && edit->hasSelectedText())
    edit->insert(QString::fromUtf8(value ? value : ""));
  return 0;
}

static char* qtListGetSelectionAttrib(Ihandle* ih)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit)
  {
    int start = edit->selectionStart();
    int len = edit->selectedText().length();
    if (len > 0)
      return iupStrReturnIntInt(start + 1, start + len, ':');
  }
  return NULL;
}

static int qtListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
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

  int start = 1, end = 1;
  if (iupStrToIntInt(value, &start, &end, ':') == 2)
  {
    if (start < 1) start = 1;
    if (end < 1) end = 1;

    start--;
    end--;

    if (end < start)
      end = start;

    edit->setSelection(start, end - start + 1);
  }
  return 0;
}

static char* qtListGetCaretAttrib(Ihandle* ih)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit)
    return iupStrReturnInt(edit->cursorPosition() + 1);
  return NULL;
}

static int qtListSetCaretAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (!edit)
    return 0;

  int pos;
  if (iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    pos--;
    edit->setCursorPosition(pos);
  }
  return 0;
}

static int qtListSetInsertAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit)
    edit->insert(QString::fromUtf8(value ? value : ""));
  return 0;
}

static int qtListSetAppendAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit)
  {
    QString text = edit->text();
    text.append(QString::fromUtf8(value ? value : ""));
    edit->setText(text);
  }
  return 0;
}

static char* qtListGetReadOnlyAttrib(Ihandle* ih)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit)
    return iupStrReturnBoolean(edit->isReadOnly());
  return NULL;
}

static char* qtListGetScrollVisibleAttrib(Ihandle* ih)
{
  if (ih->data->is_dropdown)
    return nullptr;

  QListWidget* list = qtListGetListWidget(ih);
  if (!list)
    return nullptr;

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
  QLineEdit* edit = qtListGetEditBox(ih);
  if (edit)
    edit->setReadOnly(iupStrBoolean(value));
  return 0;
}

static int qtListSetNCAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (!edit)
    return 0;

  int max;
  if (iupStrToInt(value, &max))
  {
    if (max < 0) max = 0;
    edit->setMaxLength(max);
  }
  return 0;
}

static int qtListSetClipboardAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
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
  return 0;
}

static int qtListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  QLineEdit* edit = qtListGetEditBox(ih);
  if (!edit)
    return 0;

  int pos;
  if (iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    pos--;
    edit->setCursorPosition(pos);
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
        QListWidget* list = qtListGetListWidget(ih);
        if (list)
        {
          QAbstractItemDelegate* old_delegate = list->itemDelegate();
          IupQtListItemDelegate* delegate = new IupQtListItemDelegate(ih);
          list->setItemDelegate(delegate);
          if (old_delegate && old_delegate->parent() == list)
            delete old_delegate;
        }
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
      /* Qt doesn't have direct padding control for combo box */
      /* Could use style sheets */
    }
    else
    {
      QListWidget* list = qtListGetListWidget(ih);
      if (list)
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
    QListWidget* list = qtListGetListWidget(ih);
    if (list)
    {
      QPalette palette = list->palette();
      palette.setColor(QPalette::Base, QColor(r, g, b));
      list->setPalette(palette);
    }
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
    QListWidget* list = qtListGetListWidget(ih);
    if (list)
    {
      QPalette palette = list->palette();
      palette.setColor(QPalette::Text, QColor(r, g, b));
      list->setPalette(palette);
    }
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
      QListWidget* list = qtListGetListWidget(ih);
      if (list)
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
  (void)combo;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int pos = index + 1;  /* IUP starts at 1 */
    iupListSingleCallActionCb(ih, cb, pos);
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
        int pos = row + 1;  /* IUP starts at 1 */
        iupListSingleCallActionCb(ih, cb, pos);
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

  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  if (cb)
  {
    QString text = edit->text();
    int start = edit->cursorPosition();
    int end = start;

    if (edit->hasSelectedText())
    {
      start = edit->selectionStart();
      end = start + edit->selectedText().length();
    }

    iupEditCallActionCb(ih, cb, (char*)text.toUtf8().constData(), start, end, ih->data->mask, ih->data->nc, 0, 1);
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
    QListWidget* list = qtListGetListWidget(ih);
    if (list)
    {
      QListWidgetItem* item = list->itemAt(QPoint(x, y));
      if (item)
      {
        int pos = list->row(item) + 1;  /* IUP starts at 1 */
        return pos;
      }
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
     * These matches GTK and Windows CBS_SIMPLE behavior */

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

    QObject::connect(list, &QListWidget::itemActivated, [list, ih](QListWidgetItem* item) {
      IFnis cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
      if (cb)
      {
        int pos = list->row(item) + 1;
        iupListSingleCallDblClickCb(ih, cb, pos);
      }
    });

    /* Use custom delegate to normalize item heights across platforms. The delegate also handles images. */
    IupQtListItemDelegate* delegate = new IupQtListItemDelegate(ih);
    list->setItemDelegate(delegate);

    /* Set initial items */
    iupListSetInitialItems(ih);
  }
  else if (ih->data->is_virtual)
  {
    /* Virtual mode: Create QListView with custom model */
    IupQtVirtualListView* view = new IupQtVirtualListView(ih);

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
      view->setSelectionMode(QAbstractItemView::ExtendedSelection);
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

    /* Use custom delegate to normalize item heights across platforms. The delegate also handles images. */
    IupQtListItemDelegate* delegate = new IupQtListItemDelegate(ih);
    view->setItemDelegate(delegate);

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
            iupListSingleCallActionCb(ih, cb, pos);
          iupBaseCallValueChangedCb(ih);
        }
      });

    QObject::connect(view, &QListView::activated, [ih](const QModelIndex& index) {
      IFnis cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
      if (cb)
      {
        int pos = index.row() + 1;
        iupListSingleCallDblClickCb(ih, cb, pos);
      }
    });
  }
  else
  {
    /* Create ListWidget for plain list with drag-drop support */
    IupQtListWidget* list = new IupQtListWidget(ih);

    ih->handle = (InativeHandle*)list;

    /* Remove minimum size constraints - similar to GTK's iupgtkClearSizeStyleCSS */
    list->setMinimumSize(0, 0);

    /* Set selection mode */
    if (ih->data->is_multiple)
      list->setSelectionMode(QAbstractItemView::ExtendedSelection);
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

    /* Use custom delegate to normalize item heights across platforms. The delegate also handles images. */
    IupQtListItemDelegate* delegate = new IupQtListItemDelegate(ih);
    list->setItemDelegate(delegate);

    /* Connect signals */
    QObject::connect(list, &QListWidget::itemSelectionChanged, [list, ih]() {
      qtListWidgetItemSelectionChanged(list, ih);
    });

    QObject::connect(list, &QListWidget::itemActivated, [list, ih](QListWidgetItem* item) {
      IFnis cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
      if (cb)
      {
        int pos = list->row(item) + 1;
        iupListSingleCallDblClickCb(ih, cb, pos);
      }
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

extern "C" IUP_SDK_API void iupdrvListInitClass(Iclass* ic)
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
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
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
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", qtListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);
}

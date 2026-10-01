/** \file
 * \brief List Control - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QAbstractListModel>
#include <QQuickItem>
#include <QPixmap>
#include <QColor>
#include <QKeyEvent>
#include <QHash>
#include <QByteArray>
#include <QFont>
#include <QPointF>
#include <QGuiApplication>
#include <QClipboard>
#include <QStyleHints>

#include <cstring>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_list.h"
}

#include "iupqml_drv.h"


/****************************************************************************
 * Model
 ****************************************************************************/

class IupQmlListModel : public QAbstractListModel
{
public:
  Ihandle* ih;
  QStringList texts;
  QList<QPixmap*> images;
  QList<bool> selected;
  int virtual_count;

  IupQmlListModel(Ihandle* handle) : QAbstractListModel(nullptr), ih(handle), virtual_count(0) {}

  int count() const
  {
    return ih->data->is_virtual ? virtual_count : texts.size();
  }

  int rowCount(const QModelIndex& parent = QModelIndex()) const override
  {
    if (parent.isValid())
      return 0;
    return count();
  }

  QHash<int, QByteArray> roleNames() const override
  {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "text";
    roles[Qt::UserRole + 1] = "image";
    roles[Qt::UserRole + 2] = "selected";
    return roles;
  }

  QVariant data(const QModelIndex& index, int role) const override
  {
    int row = index.row();
    if (!index.isValid() || row < 0 || row >= count())
      return {};

    if (role == Qt::DisplayRole)
    {
      if (ih->data->is_virtual)
      {
        char* text = iupListGetItemValueCb(ih, row + 1);
        return QString::fromUtf8(text ? text : "");
      }
      return texts.at(row);
    }

    if (role == Qt::UserRole + 1)
    {
      QPixmap* pixmap = nullptr;
      if (ih->data->is_virtual)
      {
        char* image_name = iupListGetItemImageCb(ih, row + 1);
        if (image_name)
          pixmap = static_cast<QPixmap*>(iupImageGetImage(image_name, ih, 0, nullptr));
      }
      else if (row < images.size())
        pixmap = images.at(row);

      return pixmap ? iupqmlImageUrl(pixmap) : QString();
    }

    if (role == Qt::UserRole + 2)
      return (row < selected.size()) ? selected.at(row) : false;

    return {};
  }

  void insertText(int pos, const QString& text)
  {
    beginInsertRows(QModelIndex(), pos, pos);
    texts.insert(pos, text);
    images.insert(pos, nullptr);
    selected.insert(pos, false);
    endInsertRows();
  }

  void removeAt(int pos)
  {
    if (pos < 0 || pos >= texts.size())
      return;
    beginRemoveRows(QModelIndex(), pos, pos);
    texts.removeAt(pos);
    images.removeAt(pos);
    selected.removeAt(pos);
    endRemoveRows();
  }

  void clear()
  {
    beginResetModel();
    texts.clear();
    images.clear();
    selected.clear();
    endResetModel();
  }

  void setVirtualCount(int new_count)
  {
    if (new_count < virtual_count)
    {
      beginRemoveRows(QModelIndex(), new_count, virtual_count - 1);
      virtual_count = new_count;
      while (selected.size() > new_count) selected.removeLast();
      endRemoveRows();
    }
    else if (new_count > virtual_count)
    {
      beginInsertRows(QModelIndex(), virtual_count, new_count - 1);
      virtual_count = new_count;
      while (selected.size() < new_count) selected.append(false);
      endInsertRows();
    }
  }

  void setSelected(int pos, bool value)
  {
    while (selected.size() < count()) selected.append(false);
    if (pos < 0 || pos >= selected.size() || selected[pos] == value)
      return;
    selected[pos] = value;
    emit dataChanged(index(pos), index(pos), {Qt::UserRole + 2});
  }

  void clearSelection()
  {
    for (int i = 0; i < selected.size(); i++)
      setSelected(i, false);
  }

  void refreshRow(int pos)
  {
    if (pos >= 0 && pos < count())
      emit dataChanged(index(pos), index(pos));
  }

  void refreshAll()
  {
    if (count() > 0)
      emit dataChanged(index(0), index(count() - 1));
  }
};

struct IupQmlListData
{
  IupQmlListModel* model;
  QQuickItem* view;
  QQuickItem* edit;
  int anchor;
};

static IupQmlListData* qmlListGetData(Ihandle* ih)
{
  return reinterpret_cast<IupQmlListData*>(iupAttribGet(ih, "_IUPQML_LIST"));
}

static const char* qmlListDelegate =
  "delegate: Rectangle { id: del; width: ListView.view ? ListView.view.width : 0; height: ListView.view ? ListView.view.iupRowHeight : 20;"
  " color: model.selected ? palette.highlight : \"transparent\"; required property var model; required property int index\n"
  "  Row { anchors.fill: parent; anchors.leftMargin: 3; spacing: 4\n"
  "    Image { visible: del.ListView.view && del.ListView.view.iupShowImage && source != \"\"; source: del.model.image; fillMode: Image.PreserveAspectFit; height: Math.min(implicitHeight, parent.height - 2); width: visible ? implicitWidth * (height / Math.max(1, implicitHeight)) : 0; anchors.verticalCenter: parent.verticalCenter; cache: false }\n"
  "    Text { text: del.model.text; color: del.model.selected ? palette.highlightedText : palette.text; anchors.verticalCenter: parent.verticalCenter; font: del.ListView.view ? del.ListView.view.iupFont : Qt.application.font }\n"
  "  }\n"
  "  Rectangle { anchors.fill: parent; color: \"transparent\"; border.width: 1; border.color: del.model.selected ? palette.highlightedText : palette.highlight; visible: del.ListView.isCurrentItem && del.ListView.view && del.ListView.view.activeFocus && del.ListView.view.iupKeyFocus }\n"
  "}";

static const char* qmlComboDelegate =
  "  delegate: ItemDelegate { required property var model; required property int index; width: ListView.view ? ListView.view.width : combo.width; text: model.text; highlighted: combo.highlightedIndex === index; hoverEnabled: combo.hoverEnabled; icon.source: combo.iupShowImage ? model.image : \"\"; icon.color: \"transparent\"; icon.cache: false }\n";

static const char* qmlComboContent =
  "  contentItem: IconLabel { leftPadding: combo.iupContentLeft; rightPadding: combo.iupContentRight; text: combo.displayText; font: combo.font; color: combo.palette.buttonText; alignment: Qt.AlignLeft | Qt.AlignVCenter; spacing: 4; icon.color: \"transparent\"; icon.cache: false\n"
  "    icon.source: (combo.iupImageRev, combo.iupShowImage && combo.currentIndex >= 0 && combo.model ? combo.model.data(combo.model.index(combo.currentIndex, 0), 257) : \"\") }\n";

static const char* qmlComboPopupHeight =
  "  Binding { target: combo.popup; property: \"height\"; when: combo.iupVisibleItems > 0 && combo.count > combo.iupVisibleItems && combo.popup.contentItem.contentHeight > 0\n"
  "    value: combo.iupVisibleItems * combo.popup.contentItem.contentHeight / combo.count + combo.popup.topPadding + combo.popup.bottomPadding }\n"
  "  Binding { target: combo.popup; property: \"height\"; when: !(combo.iupVisibleItems > 0 && combo.count > combo.iupVisibleItems) && combo.popup.contentItem.contentHeight > 0\n"
  "    value: Math.min(combo.popup.contentItem.contentHeight + combo.popup.topPadding + combo.popup.bottomPadding, combo.Screen.desktopAvailableHeight - combo.popup.topMargin - combo.popup.bottomMargin) }\n";

/****************************************************************************
 * Size
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
  (void)ih;
  *h += 4;
}

static QQuickItem* qmlListComboTemplate(int editable)
{
  if (editable)
    return iupqmlTemplateItem(IUPQML_IMPORTS "ComboBox { editable: true; model: [\"X\"] }");
  return iupqmlTemplateItem(IUPQML_IMPORTS "ComboBox { model: [\"X\"] }");
}

static QQuickItem* qmlListComboContentTemplate(int editable)
{
  QQuickItem* combo = qmlListComboTemplate(editable);
  return combo ? iupqmlGetItemProperty(combo, "contentItem") : nullptr;
}

extern "C" IUP_SDK_API void iupdrvListAddBorders(Ihandle* ih, int* w, int* h)
{
  if (ih->data->is_dropdown)
  {
    QQuickItem* combo = qmlListComboTemplate(ih->data->has_editbox);
    QQuickItem* content = qmlListComboContentTemplate(ih->data->has_editbox);
    if (combo)
    {
      (*w) += static_cast<int>(combo->property("leftPadding").toDouble() + combo->property("rightPadding").toDouble());
      (*h) += static_cast<int>(combo->property("topPadding").toDouble() + combo->property("bottomPadding").toDouble());
      if (content)
      {
        (*w) += static_cast<int>(std::ceil(content->property("leftPadding").toDouble() + content->property("rightPadding").toDouble()));
        (*h) += static_cast<int>(std::ceil(content->property("topPadding").toDouble() + content->property("bottomPadding").toDouble()));
      }
      int min_h = static_cast<int>(combo->property("implicitBackgroundHeight").toDouble());
      if (*h < min_h) *h = min_h;
    }
    else
    {
      (*w) += 30;
      (*h) += 8;
    }
  }
  else
  {
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

    (*w) += 4 + 6;
    (*h) += 4;

    if (ih->data->has_editbox)
    {
      QQuickItem* field = iupqmlTemplateItem(IUPQML_IMPORTS "TextField { text: \"X\" }");
      int edit_h = field ? static_cast<int>(field->implicitHeight()) : 24;
      if (field)
      {
        int edit_w = static_cast<int>(std::ceil(field->property("leftPadding").toDouble() + field->property("rightPadding").toDouble()));
        if (edit_w > 4 + 6)
          (*w) += edit_w - (4 + 6);
      }

      if (visiblelines > 0)
      {
        int char_height;
        iupdrvFontGetCharSize(ih, nullptr, &char_height);
        int item_height = char_height;
        iupdrvListAddItemSpace(ih, &item_height);
        (*h) -= item_height;
      }

      (*h) += edit_h;
    }
  }
}

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvListGetCount(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  return data ? data->model->count() : 0;
}

static int qmlListSortPos(IupQmlListModel* model, const char* value)
{
  int n = model->texts.size();
  for (int i = 0; i < n; i++)
    if (iupStrCompare(model->texts.at(i).toUtf8().constData(), value, 0, 1) > 0) return i;
  return n;
}

extern "C" IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data)
    return;

  int sort = iupAttribGetBoolean(ih, "SORT");
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  data->model->insertText(sort ? qmlListSortPos(data->model, value) : data->model->texts.size(), QString::fromUtf8(value));
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);
}

extern "C" IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data)
    return;

  int sort = iupAttribGetBoolean(ih, "SORT");
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  data->model->insertText(sort ? qmlListSortPos(data->model, value) : pos, QString::fromUtf8(value));
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);
  iupListUpdateOldValue(ih, pos, 0);
}

extern "C" IUP_SDK_API void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data)
    return;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  if (ih->data->is_dropdown && !ih->data->has_editbox)
  {
    int curpos = data->view->property("currentIndex").toInt();
    if (pos == curpos)
    {
      if (curpos > 0)
        curpos--;
      else
      {
        curpos = 1;
        if (data->model->count() == 1)
          curpos = -1;
      }
      data->view->setProperty("currentIndex", curpos);
    }
  }
  data->model->removeAt(pos);
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);

  iupListUpdateOldValue(ih, pos, 1);
}

extern "C" IUP_SDK_API void iupdrvListRemoveAllItems(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data)
    return;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  data->model->clear();
  if (ih->data->is_dropdown)
    data->view->setProperty("currentIndex", -1);
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);
}

extern "C" IUP_SDK_API int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || id < 0 || id >= data->model->images.size())
    return 0;

  data->model->images[id] = static_cast<QPixmap*>(hImage);
  data->model->refreshRow(id);
  return 1;
}

extern "C" IUP_SDK_API void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || id < 1 || id > data->model->images.size())
    return nullptr;

  return reinterpret_cast<void*>(data->model->images.at(id - 1));
}

extern "C" IUP_SDK_API void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || !ih->data->is_virtual)
    return;

  data->model->setVirtualCount(count);
}

/****************************************************************************
 * Selection
 ****************************************************************************/

static char* qmlListGetValueString(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  int count = data->model->count();
  char* str = iupStrGetMemory(count + 1);
  memset(str, '-', count);
  str[count] = 0;

  for (int i = 0; i < count && i < data->model->selected.size(); i++)
    if (data->model->selected.at(i))
      str[i] = '+';
  return str;
}

static int qmlListGetSingleSelected(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  for (int i = 0; i < data->model->selected.size(); i++)
    if (data->model->selected.at(i))
      return i;
  return -1;
}

static void qmlListCallSelection(Ihandle* ih, int pos)
{
  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  if (!ih->data->is_multiple)
  {
    if (pos >= 0)
    {
      auto cb = reinterpret_cast<IFnsii>(IupGetCallback(ih, "ACTION"));
      if (cb)
        iupListSingleCallActionCb(ih, cb, pos + 1);
    }
  }
  else
  {
    IFns multi_cb = reinterpret_cast<IFns>(IupGetCallback(ih, "MULTISELECT_CB"));
    auto cb = reinterpret_cast<IFnsii>(IupGetCallback(ih, "ACTION"));
    IupQmlListData* data = qmlListGetData(ih);
    if ((multi_cb || cb) && data)
    {
      QVector<int> selected_pos;
      int count = data->model->count();
      for (int i = 0; i < count && i < data->model->selected.size(); i++)
        if (data->model->selected[i])
          selected_pos.append(i);
      iupListMultipleCallActionCb(ih, cb, multi_cb, selected_pos.data(), static_cast<int>(selected_pos.size()));
    }
  }

  iupBaseCallValueChangedCb(ih);
}

static void qmlListSelectSingle(Ihandle* ih, int pos, int notify)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data)
    return;

  data->model->clearSelection();
  if (pos >= 0)
    data->model->setSelected(pos, true);

  iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", "1");
  data->view->setProperty("currentIndex", pos);
  iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", nullptr);

  if (data->edit && pos >= 0 && !ih->data->is_dropdown)
  {
    iupAttribSet(ih, "_IUPQML_DISABLE_TEXT_CB", "1");
    data->edit->setProperty("text", data->model->texts.value(pos));
    iupAttribSet(ih, "_IUPQML_DISABLE_TEXT_CB", nullptr);
  }

  if (notify)
    qmlListCallSelection(ih, pos);
}

static void qmlListClickAt(Ihandle* ih, int pos, Qt::KeyboardModifiers mods)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || pos < 0)
    return;

  if (!ih->data->is_multiple)
  {
    if (qmlListGetSingleSelected(ih) == pos)
    {
      iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", "1");
      data->view->setProperty("currentIndex", pos);
      iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", nullptr);
      return;
    }
    qmlListSelectSingle(ih, pos, 1);
    return;
  }

  QList<bool> before = data->model->selected;
  while (before.size() < data->model->count()) before.append(false);

  if (mods & Qt::ShiftModifier)
  {
    int a = data->anchor < 0 ? pos : data->anchor;
    int lo = a < pos ? a : pos, hi = a < pos ? pos : a;
    if (!(mods & Qt::ControlModifier))
      data->model->clearSelection();
    for (int i = lo; i <= hi; i++)
      data->model->setSelected(i, true);
  }
  else if (mods & Qt::ControlModifier)
  {
    data->model->setSelected(pos, !data->model->selected.value(pos));
    data->anchor = pos;
  }
  else
  {
    data->model->clearSelection();
    data->model->setSelected(pos, true);
    data->anchor = pos;
  }

  iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", "1");
  data->view->setProperty("currentIndex", pos);
  iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", nullptr);

  if (before != data->model->selected)
    qmlListCallSelection(ih, pos);
}

static void qmlListKeySelect(Ihandle* ih, int pos)
{
  IupQmlListData* data = qmlListGetData(ih);
  Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();

  if (mods & Qt::ControlModifier)
    return;

  if (mods & Qt::ShiftModifier)
  {
    int a = data->anchor < 0 ? pos : data->anchor;
    int lo = a < pos ? a : pos, hi = a < pos ? pos : a;
    data->model->clearSelection();
    for (int i = lo; i <= hi; i++)
      data->model->setSelected(i, true);
  }
  else
  {
    data->model->clearSelection();
    data->model->setSelected(pos, true);
    data->anchor = pos;
  }

  qmlListCallSelection(ih, pos);
}

static int qmlListIndexAt(Ihandle* ih, int x, int y)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || ih->data->is_dropdown)
    return -1;

  int index = -1;
  qreal cx = x - data->view->x(), cy = y - data->view->y() + data->view->property("contentY").toDouble();
  QMetaObject::invokeMethod(data->view, "indexAt", Qt::DirectConnection, Q_RETURN_ARG(int, index), Q_ARG(qreal, cx), Q_ARG(qreal, cy));
  return index;
}

static void qmlListDropped(Ihandle* ih, int drag, const QPointF& scene_pos)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || drag < 0 || drag >= data->model->count())
    return;

  QPointF pos = data->view->mapFromScene(scene_pos);
  int drop = qmlListIndexAt(ih, static_cast<int>(pos.x() + data->view->x()), static_cast<int>(pos.y() + data->view->y()));
  if (drop >= 0)
  {
    QQuickItem* item = nullptr;
    QMetaObject::invokeMethod(data->view, "itemAtIndex", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, item), Q_ARG(int, drop));
    if (item)
    {
      QPointF local = item->mapFromScene(scene_pos);
      if (local.y() > item->height() / 2)
        drop++;
    }
  }
  else if (pos.y() < 0)
    drop = 0;
  else
    drop = -1;

  int is_ctrl = 0;
  if (iupListCallDragDropCb(ih, drag, drop, &is_ctrl) != IUP_CONTINUE)
    return;

  QString text = data->model->texts.value(drag);
  QPixmap* image = data->model->images.value(drag);
  int final_pos;

  if (drop >= 0 && drop < data->model->count())
  {
    data->model->insertText(drop, text);
    data->model->images[drop] = image;
    final_pos = drop;
    if (drag >= drop)
      drag++;
  }
  else
  {
    data->model->insertText(data->model->count(), text);
    final_pos = data->model->count() - 1;
    data->model->images[final_pos] = image;
  }

  if (!is_ctrl)
  {
    data->model->removeAt(drag);
    if (drag < final_pos)
      final_pos--;
  }

  qmlListSelectSingle(ih, final_pos, 0);
}

static void qmlListApplyFilter(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  const char* filter = iupAttribGet(ih, "FILTER");
  if (!data || !data->edit || !filter)
    return;

  bool is_number = iupStrEqualNoCase(filter, "NUMBER");
  bool is_upper = iupStrEqualNoCase(filter, "UPPERCASE");
  bool is_lower = iupStrEqualNoCase(filter, "LOWERCASE");
  if (!is_number && !is_upper && !is_lower)
    return;

  QString text = data->edit->property("text").toString();
  QString filtered;
  if (is_number)
  {
    for (QChar c : text)
      if (c.isDigit())
        filtered.append(c);
  }
  else
    filtered = is_upper ? text.toUpper() : text.toLower();

  if (filtered == text)
    return;

  int pos = data->edit->property("cursorPosition").toInt() - static_cast<int>(text.length() - filtered.length());
  iupAttribSet(ih, "_IUPQML_DISABLE_TEXT_CB", "1");
  data->edit->setProperty("text", filtered);
  data->edit->setProperty("cursorPosition", qBound(0, pos, static_cast<int>(filtered.length())));
  iupAttribSet(ih, "_IUPQML_DISABLE_TEXT_CB", nullptr);
}

static int qmlListSetFilterAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (!ih->data->has_editbox)
    return 0;
  return 1;
}

class IupQmlListFilter : public QObject
{
public:
  Ihandle* ih;
  int drag_from = -1;
  bool dragging = false;
  QPointF press_pos;
  IupQmlListFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (!iupObjectCheck(ih))
      return false;

    auto* view = static_cast<QQuickItem*>(obj);
    bool reorder = ih->data->show_dragdrop && !ih->data->is_multiple && !ih->data->is_dropdown;

    if (event->type() == QEvent::MouseButtonPress)
    {
      auto* me = static_cast<QMouseEvent*>(event);
      if (view->activeFocusOnTab())
        view->forceActiveFocus(Qt::MouseFocusReason);
      if (me->button() == Qt::LeftButton)
      {
        int pos = qmlListIndexAt(ih, static_cast<int>(me->position().x() + view->x()), static_cast<int>(me->position().y() + view->y()));
        if (pos >= 0)
          qmlListClickAt(ih, pos, me->modifiers());
        if (reorder)
        {
          drag_from = pos;
          dragging = false;
          press_pos = me->scenePosition();
          return true;
        }
      }
    }
    else if (event->type() == QEvent::MouseMove && reorder && drag_from >= 0)
    {
      auto* me = static_cast<QMouseEvent*>(event);
      if (!dragging && (me->scenePosition() - press_pos).manhattanLength() >= QGuiApplication::styleHints()->startDragDistance())
        dragging = true;
      return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease && reorder && drag_from >= 0)
    {
      auto* me = static_cast<QMouseEvent*>(event);
      int from = drag_from;
      drag_from = -1;
      if (dragging)
      {
        dragging = false;
        qmlListDropped(ih, from, me->scenePosition());
      }
      return true;
    }
    else if (event->type() == QEvent::MouseButtonDblClick)
    {
      auto* me = static_cast<QMouseEvent*>(event);
      int pos = qmlListIndexAt(ih, static_cast<int>(me->position().x() + view->x()), static_cast<int>(me->position().y() + view->y()));
      auto cb = reinterpret_cast<IFnis>(IupGetCallback(ih, "DBLCLICK_CB"));
      if (cb && pos >= 0)
        iupListSingleCallDblClickCb(ih, cb, pos + 1);
    }
    return false;
  }
};

class IupQmlListEditFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlListEditFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (event->type() != QEvent::KeyPress || !iupObjectCheck(ih))
      return false;

    auto* evt = static_cast<QKeyEvent*>(event);
    QQuickItem* edit = qobject_cast<QQuickItem*>(obj);

    if (iupqmlKeyPressEvent(edit, evt, ih))
      return true;

    auto cb = reinterpret_cast<IFnis>(IupGetCallback(ih, "EDIT_CB"));
    if (!cb && !ih->data->mask && !ih->data->nc)
      return false;

    int start = edit->property("selectionStart").toInt();
    int end = edit->property("selectionEnd").toInt();
    int ret;

    if (evt->matches(QKeySequence::Paste) || evt->matches(QKeySequence::Cut))
    {
      int cut = evt->matches(QKeySequence::Cut);
      if (cut)
      {
        if (start == end)
          return false;
        ret = iupEditCallActionCb(ih, cb, nullptr, start, end, ih->data->mask, ih->data->nc, 0, 1);
      }
      else
      {
        QString clip = QGuiApplication::clipboard()->text();
        if (clip.isEmpty())
          return false;
        ret = iupEditCallActionCb(ih, cb, clip.toUtf8().constData(), start, end, ih->data->mask, ih->data->nc, 0, 1);
      }
      if (ret == 0)
      {
        if (cut)
          QMetaObject::invokeMethod(edit, "copy", Qt::DirectConnection);
        return true;
      }
      return false;
    }

    if ((evt->key() == Qt::Key_Backspace || evt->key() == Qt::Key_Delete) && !(evt->modifiers() & (Qt::ControlModifier | Qt::AltModifier)))
    {
      int remove_dir = evt->key() == Qt::Key_Delete ? 1 : -1;
      int len = static_cast<int>(edit->property("text").toString().length());
      if (start == end && ((remove_dir == -1 && start == 0) || (remove_dir == 1 && start >= len)))
        return false;
      ret = iupEditCallActionCb(ih, cb, nullptr, start, end, ih->data->mask, ih->data->nc, remove_dir, 1);
      return ret == 0;
    }

    QString typed = evt->text();
    if (typed.isEmpty() || !typed.at(0).isPrint() || typed.at(0).unicode() < 32)
      return false;

    ret = iupEditCallActionCb(ih, cb, typed.toUtf8().constData(), start, end, ih->data->mask, ih->data->nc, 0, 1);
    if (ret == 0)
      return true;
    if (ret != -1)
    {
      if (end > start)
        QMetaObject::invokeMethod(edit, "remove", Qt::DirectConnection, Q_ARG(int, start), Q_ARG(int, end));
      QMetaObject::invokeMethod(edit, "insert", Qt::DirectConnection, Q_ARG(int, start), Q_ARG(QString, QString(QChar(ret))));
      return true;
    }
    return false;
  }
};

/****************************************************************************
 * Attributes
 ****************************************************************************/

static char* qmlListGetIdValueAttrib(Ihandle* ih, int id)
{
  IupQmlListData* data = qmlListGetData(ih);
  int pos = iupListGetPosAttrib(ih, id);
  if (data && pos >= 0 && !ih->data->is_virtual && pos < data->model->texts.size())
    return iupStrReturnStr(data->model->texts.at(pos).toUtf8().constData());
  return nullptr;
}

static char* qmlListGetValueAttrib(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data)
    return nullptr;

  if (ih->data->has_editbox)
  {
    if (data->edit)
      return iupStrReturnStr(data->edit->property("text").toString().toUtf8().constData());
    return nullptr;
  }

  if (ih->data->is_dropdown)
  {
    int pos = data->view->property("currentIndex").toInt();
    return iupStrReturnInt(pos + 1);
  }

  if (!ih->data->is_multiple)
  {
    int pos = qmlListGetSingleSelected(ih);
    if (pos >= 0)
      return iupStrReturnInt(pos + 1);
    return nullptr;
  }

  return qmlListGetValueString(ih);
}

static int qmlListSetValueAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data)
    return 0;

  if (ih->data->has_editbox)
  {
    if (data->edit)
    {
      iupAttribSet(ih, "_IUPQML_DISABLE_TEXT_CB", "1");
      data->edit->setProperty("text", QString::fromUtf8(value ? value : ""));
      iupAttribSet(ih, "_IUPQML_DISABLE_TEXT_CB", nullptr);
    }
    return 0;
  }

  if (ih->data->is_dropdown)
  {
    int pos;
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    if (iupStrToInt(value, &pos) == 1 && pos > 0 && pos <= data->model->count())
    {
      data->view->setProperty("currentIndex", pos - 1);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    }
    else
    {
      data->view->setProperty("currentIndex", -1);
      iupAttribSet(ih, "_IUPLIST_OLDVALUE", nullptr);
    }
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);
    return 0;
  }

  if (!ih->data->is_multiple)
  {
    int pos;
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    if (iupStrToInt(value, &pos) == 1 && pos > 0)
    {
      qmlListSelectSingle(ih, pos - 1, 0);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    }
    else
    {
      qmlListSelectSingle(ih, -1, 0);
      iupAttribSet(ih, "_IUPLIST_OLDVALUE", nullptr);
    }
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);
    return 0;
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  data->model->clearSelection();
  if (value)
  {
    int len = static_cast<int>(strlen(value));
    int count = data->model->count();
    if (len < count)
      count = len;
    for (int i = 0; i < count; i++)
      if (value[i] == '+')
        data->model->setSelected(i, true);
  }
  iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", value);
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);
  return 0;
}

static void qmlListUpdateRowHeight(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || ih->data->is_dropdown)
    return;

  int char_height;
  iupdrvFontGetCharSize(ih, nullptr, &char_height);
  int row = char_height;
  if (ih->data->show_image && !ih->data->fit_image && ih->data->maximg_h > row)
    row = ih->data->maximg_h;
  iupdrvListAddItemSpace(ih, &row);
  row += 2 * ih->data->spacing;
  data->view->setProperty("iupRowHeight", row);

  QFont* font = iupqmlGetIhFont(ih);
  if (font)
    data->view->setProperty("iupFont", QVariant::fromValue(*font));
}

static int qmlListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  int pos = iupListGetPosAttrib(ih, id);
  if (!data || pos < 0 || pos >= data->model->images.size())
    return 0;

  auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr));
  data->model->images[pos] = (pixmap && !pixmap->isNull()) ? pixmap : nullptr;
  data->model->refreshRow(pos);

  if (pixmap)
  {
    if (pixmap->width() > ih->data->maximg_w)
      ih->data->maximg_w = pixmap->width();
    if (pixmap->height() > ih->data->maximg_h)
    {
      ih->data->maximg_h = pixmap->height();
      qmlListUpdateRowHeight(ih);
    }
  }

  if (ih->data->is_dropdown)
    data->view->setProperty("iupImageRev", data->view->property("iupImageRev").toInt() + 1);

  return 1;
}

static char* qmlListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  return static_cast<char*>(iupdrvListGetImageHandle(ih, id));
}

static int qmlListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  int pos;
  if (data && !ih->data->is_dropdown && iupStrToInt(value, &pos) && pos > 0)
    QMetaObject::invokeMethod(data->view, "positionViewAtIndex", Qt::DirectConnection, Q_ARG(int, pos - 1), Q_ARG(int, 0));
  return 0;
}

static int qmlListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && ih->data->is_dropdown)
  {
    auto* popup = qvariant_cast<QObject*>(data->view->property("popup"));
    if (popup)
      QMetaObject::invokeMethod(popup, iupStrBoolean(value) ? "open" : "close", Qt::DirectConnection);
  }
  return 0;
}

static int qmlListSetVisibleItemsAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  int count;
  if (data && ih->data->is_dropdown && iupStrToInt(value, &count) && count > 0)
    data->view->setProperty("iupVisibleItems", count);
  return 1;
}

static int qmlListSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
    data->edit->setProperty("placeholderText", value ? QString::fromUtf8(value) : QString());
  return 1;
}

static char* qmlListGetSelectedTextAttrib(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
    return iupStrReturnStr(data->edit->property("selectedText").toString().toUtf8().constData());
  return nullptr;
}

static int qmlListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
  {
    int start = data->edit->property("selectionStart").toInt();
    int end = data->edit->property("selectionEnd").toInt();
    if (end > start)
    {
      QMetaObject::invokeMethod(data->edit, "remove", Qt::DirectConnection, Q_ARG(int, start), Q_ARG(int, end));
      QMetaObject::invokeMethod(data->edit, "insert", Qt::DirectConnection, Q_ARG(int, start), Q_ARG(QString, QString::fromUtf8(value ? value : "")));
    }
  }
  return 0;
}

static char* qmlListGetSelectionAttrib(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
  {
    int start = data->edit->property("selectionStart").toInt();
    int end = data->edit->property("selectionEnd").toInt();
    if (end > start)
      return iupStrReturnIntInt(start + 1, end, ':');
  }
  return nullptr;
}

static int qmlListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || !data->edit)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    QMetaObject::invokeMethod(data->edit, "deselect", Qt::DirectConnection);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    QMetaObject::invokeMethod(data->edit, "selectAll", Qt::DirectConnection);
    return 0;
  }

  int start = 1, end = 1;
  if (iupStrToIntInt(value, &start, &end, ':') == 2)
  {
    if (start < 1) start = 1;
    if (end < 1) end = 1;
    if (end < start) end = start;
    QMetaObject::invokeMethod(data->edit, "select", Qt::DirectConnection, Q_ARG(int, start - 1), Q_ARG(int, end));
  }
  return 0;
}

static char* qmlListGetCaretAttrib(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
    return iupStrReturnInt(data->edit->property("cursorPosition").toInt() + 1);
  return nullptr;
}

static int qmlListSetCaretAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  int pos;
  if (data && data->edit && iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    data->edit->setProperty("cursorPosition", pos - 1);
  }
  return 0;
}

static int qmlListSetInsertAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
  {
    int pos = data->edit->property("cursorPosition").toInt();
    QMetaObject::invokeMethod(data->edit, "insert", Qt::DirectConnection, Q_ARG(int, pos), Q_ARG(QString, QString::fromUtf8(value ? value : "")));
  }
  return 0;
}

static int qmlListSetAppendAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
  {
    QString text = data->edit->property("text").toString();
    text.append(QString::fromUtf8(value ? value : ""));
    data->edit->setProperty("text", text);
  }
  return 0;
}

static char* qmlListGetReadOnlyAttrib(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
    return iupStrReturnBoolean(data->edit->property("readOnly").toBool());
  return nullptr;
}

static int qmlListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data && data->edit)
    data->edit->setProperty("readOnly", iupStrBoolean(value) ? true : false);
  return 0;
}

static int qmlListSetNCAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = 0;
  if (data && data->edit)
    data->edit->setProperty("maximumLength", ih->data->nc > 0 ? ih->data->nc : 32767);
  return 0;
}

static int qmlListSetClipboardAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || !data->edit)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
    QMetaObject::invokeMethod(data->edit, "copy", Qt::DirectConnection);
  else if (iupStrEqualNoCase(value, "CUT"))
    QMetaObject::invokeMethod(data->edit, "cut", Qt::DirectConnection);
  else if (iupStrEqualNoCase(value, "PASTE"))
    QMetaObject::invokeMethod(data->edit, "paste", Qt::DirectConnection);
  else if (iupStrEqualNoCase(value, "CLEAR"))
    QMetaObject::invokeMethod(data->edit, "clear", Qt::DirectConnection);
  return 0;
}

static int qmlListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  IupQmlListData* data = qmlListGetData(ih);
  int pos;
  if (data && data->edit && iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    data->edit->setProperty("cursorPosition", pos - 1);
  }
  return 0;
}

static int qmlListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToInt(value, &ih->data->spacing))
  {
    if (ih->handle)
      qmlListUpdateRowHeight(ih);
    return 0;
  }
  return 1;
}

static void qmlListLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  IupQmlListData* data = qmlListGetData(ih);
  if (!data || ih->data->is_dropdown)
    return;

  int edit_h = 0;
  if (data->edit)
  {
    edit_h = static_cast<int>(data->edit->implicitHeight());
    data->edit->setPosition(QPointF(0, 0));
    data->edit->setSize(QSizeF(ih->currentwidth, edit_h));
  }

  int list_h = ih->currentheight - edit_h;
  if (list_h < 0) list_h = 0;

  auto* back = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_LIST_BACK"));
  if (back)
  {
    back->setPosition(QPointF(0, edit_h));
    back->setSize(QSizeF(ih->currentwidth, list_h));
  }

  int hp = ih->data->horiz_padding, vp = ih->data->vert_padding;
  int view_w = ih->currentwidth - 2 - 2 * hp;
  int view_h = list_h - 2 - 2 * vp;
  data->view->setPosition(QPointF(1 + hp, edit_h + 1 + vp));
  data->view->setSize(QSizeF(view_w > 0 ? view_w : 0, view_h > 0 ? view_h : 0));
}

static int qmlListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    qmlListLayoutUpdateMethod(ih);
    return 0;
  }
  return 1;
}

static int qmlListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(data->view, "base", QColor(r, g, b));
  iupqmlSetPaletteColor(data->view, "window", QColor(r, g, b));
  if (!ih->data->is_dropdown)
  {
    auto* back = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_LIST_BACK"));
    if (back)
      back->setProperty("color", QColor(r, g, b));
  }
  return 1;
}

static int qmlListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(data->view, "text", QColor(r, g, b));
  iupqmlSetPaletteColor(data->view, "windowText", QColor(r, g, b));
  return 1;
}

static int qmlListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  IupQmlListData* data = qmlListGetData(ih);
  if (data)
  {
    iupqmlUpdateItemFont(ih, data->view);
    if (data->edit)
      iupqmlUpdateItemFont(ih, data->edit);
    qmlListUpdateRowHeight(ih);
  }
  return 1;
}

static char* qmlListGetScrollVisibleAttrib(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (!data || ih->data->is_dropdown)
    return nullptr;

  int vert = data->view->property("contentHeight").toDouble() > data->view->height();
  return vert ? const_cast<char*>("VERTICAL") : const_cast<char*>("NO");
}

static int qmlListConvertXYToPos(Ihandle* ih, int x, int y)
{
  int pos = qmlListIndexAt(ih, x, y);
  return pos >= 0 ? pos + 1 : -1;
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qmlListMapMethod(Ihandle* ih)
{
  auto* data = new IupQmlListData();
  memset(data, 0, sizeof(IupQmlListData));
  data->anchor = -1;
  data->model = new IupQmlListModel(ih);

  if (ih->data->is_dropdown)
  {
    QByteArray qml = IUPQML_IMPORTS "import QtQuick.Controls.impl\n"
      "ComboBox { id: combo; textRole: \"text\"; property int iupVisibleItems: 0; property bool iupShowImage: false; property int iupImageRev: 0\n"
      "  property real iupContentLeft: 0; property real iupContentRight: 0; popup.popupType: Popup.Window\n";
    if (ih->data->has_editbox)
      qml += "  editable: true\n";
    if (ih->data->show_image)
    {
      qml += qmlComboDelegate;
      if (!ih->data->has_editbox)
        qml += qmlComboContent;
    }
    qml += qmlComboPopupHeight;
    qml += "}";

    QQuickItem* combo = iupqmlCreateItem(qml.constData());
    if (!combo)
    {
      delete data->model;
      delete data;
      return IUP_ERROR;
    }

    QQuickItem* content = qmlListComboContentTemplate(ih->data->has_editbox);
    if (content)
    {
      combo->setProperty("iupContentLeft", content->property("leftPadding"));
      combo->setProperty("iupContentRight", content->property("rightPadding"));
    }
    combo->setProperty("model", QVariant::fromValue<QObject*>(data->model));
    combo->setProperty("iupShowImage", ih->data->show_image ? true : false);
    data->view = combo;
    ih->handle = reinterpret_cast<InativeHandle*>(combo);
    iupAttribSet(ih, "_IUPQML_LIST", reinterpret_cast<char*>(data));

    iupqmlConnect(combo, "activated(int)", [ih](void** args) {
      int index = *static_cast<int*>(args[1]);
      if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
        return;
      auto cb = reinterpret_cast<IFnsii>(IupGetCallback(ih, "ACTION"));
      if (cb)
        iupListSingleCallActionCb(ih, cb, index + 1);
      iupBaseCallValueChangedCb(ih);
    });

    auto* popup = qvariant_cast<QObject*>(combo->property("popup"));
    if (popup)
    {
      iupqmlConnect(popup, "opened()", [ih](void**) {
        IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "DROPDOWN_CB"));
        if (cb) cb(ih, 1);
      });
      iupqmlConnect(popup, "closed()", [ih](void**) {
        IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "DROPDOWN_CB"));
        if (cb) cb(ih, 0);
      });
    }

    if (ih->data->has_editbox)
    {
      data->edit = iupqmlGetItemProperty(combo, "contentItem");
      if (data->edit)
      {
        data->edit->installEventFilter(new IupQmlListEditFilter(data->edit, ih));
        iupqmlConnect(data->edit, "textEdited()", [ih](void**) {
          if (iupAttribGet(ih, "_IUPQML_DISABLE_TEXT_CB") || iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
            return;
          qmlListApplyFilter(ih);
          iupBaseCallValueChangedCb(ih);
        });
        iupqmlConnect(data->edit, "cursorPositionChanged()", [ih](void**) {
          IupQmlListData* d = qmlListGetData(ih);
          auto cb = reinterpret_cast<IFniii>(IupGetCallback(ih, "CARET_CB"));
          if (cb && d && d->edit)
          {
            int pos = d->edit->property("cursorPosition").toInt();
            cb(ih, 1, pos + 1, pos);
          }
        });
      }
    }

    iupListSetInitialItems(ih);

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    combo->setProperty("currentIndex", -1);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", nullptr);
  }
  else
  {
    QQuickItem* wrapper = iupqmlCreateItem("import QtQuick\nItem { clip: true }");
    if (!wrapper)
    {
      delete data->model;
      delete data;
      return IUP_ERROR;
    }

    QQuickItem* back = iupqmlCreateItem("import QtQuick\nRectangle { property Item iupView: null; color: palette.base; border.width: 1; border.color: iupView && iupView.activeFocus ? palette.highlight : palette.mid }");
    if (back)
    {
      back->setParentItem(wrapper);
      back->setParent(wrapper);
      iupAttribSet(ih, "_IUPQML_LIST_BACK", reinterpret_cast<char*>(back));
    }

    QByteArray qml = IUPQML_IMPORTS "ListView { clip: true; boundsBehavior: Flickable.StopAtBounds; keyNavigationEnabled: true; keyNavigationWraps: false; highlightFollowsCurrentItem: true; activeFocusOnTab: true; property int iupRowHeight: 20; property bool iupKeyFocus: false; property bool iupShowImage: false; property font iupFont\n";
    int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
    if (ih->data->sb)
      qml += autohide ? "  ScrollBar.vertical: ScrollBar { policy: size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff }\n" : "  ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }\n";
    qml += "  ";
    qml += qmlListDelegate;
    qml += "\n}";

    QQuickItem* view = iupqmlCreateItem(qml.constData());
    if (!view)
    {
      delete wrapper;
      delete data->model;
      delete data;
      return IUP_ERROR;
    }

    view->setParentItem(wrapper);
    view->setParent(wrapper);
    view->setProperty("model", QVariant::fromValue<QObject*>(data->model));
    view->setProperty("iupShowImage", ih->data->show_image ? true : false);
    if (ih->data->show_dragdrop && !ih->data->is_multiple)
      view->setProperty("interactive", false);
    data->view = view;
    if (back)
      back->setProperty("iupView", QVariant::fromValue<QObject*>(view));

    ih->handle = reinterpret_cast<InativeHandle*>(wrapper);
    iupAttribSet(ih, "_IUPQML_LIST", reinterpret_cast<char*>(data));

    iupqmlSetIhandle(view, ih);
    view->installEventFilter(new IupQmlListFilter(view, ih));

    iupqmlConnect(view, "activeFocusChanged(bool)", [ih](void** args) {
      IupQmlListData* d = qmlListGetData(ih);
      if (!d)
        return;
      bool focus = *static_cast<bool*>(args[1]);
      d->view->setProperty("iupKeyFocus", focus && iupAttribGet(IupGetDialog(ih), "_IUPQML_KEYBOARD_FOCUS") != nullptr);
      if (!focus || d->view->property("currentIndex").toInt() >= 0 || d->model->count() == 0)
        return;
      iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", "1");
      d->view->setProperty("currentIndex", 0);
      iupAttribSet(ih, "_IUPQML_IGNORE_CURRENT", nullptr);
    });

    iupqmlConnect(view, "currentIndexChanged()", [ih](void**) {
      IupQmlListData* d = qmlListGetData(ih);
      if (!d || iupAttribGet(ih, "_IUPQML_IGNORE_CURRENT") || iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
        return;
      int pos = d->view->property("currentIndex").toInt();
      if (pos < 0)
        return;
      if (!ih->data->is_multiple)
        qmlListSelectSingle(ih, pos, 1);
      else
        qmlListKeySelect(ih, pos);
    });

    if (ih->data->has_editbox)
    {
      QQuickItem* edit = iupqmlCreateItem(IUPQML_IMPORTS "TextField { selectByMouse: true }");
      if (edit)
      {
        edit->setParentItem(wrapper);
        edit->setParent(wrapper);
        data->edit = edit;
        iupqmlInstallFilter(ih, edit);
        edit->setProperty("_iup_keys_handled", true);
        edit->installEventFilter(new IupQmlListEditFilter(edit, ih));
        iupqmlConnect(edit, "textEdited()", [ih](void**) {
          if (iupAttribGet(ih, "_IUPQML_DISABLE_TEXT_CB") || iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
            return;
          qmlListApplyFilter(ih);
          iupBaseCallValueChangedCb(ih);
        });
        iupqmlConnect(edit, "cursorPositionChanged()", [ih](void**) {
          IupQmlListData* d = qmlListGetData(ih);
          auto cb = reinterpret_cast<IFniii>(IupGetCallback(ih, "CARET_CB"));
          if (cb && d && d->edit)
          {
            int pos = d->edit->property("cursorPosition").toInt();
            cb(ih, 1, pos + 1, pos);
          }
        });
      }
    }

    if (!ih->data->is_virtual)
      iupListSetInitialItems(ih);
    else
      data->model->setVirtualCount(ih->data->item_count);

    qmlListUpdateRowHeight(ih);
  }

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, data->view);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(data->view, 0);

  IupSetCallback(ih, "_IUP_XY2POS_CB", reinterpret_cast<Icallback>(qmlListConvertXYToPos));

  return IUP_NOERROR;
}

static void qmlListUnMapMethod(Ihandle* ih)
{
  IupQmlListData* data = qmlListGetData(ih);
  if (data)
  {
    if (data->view)
      data->view->setProperty("model", QVariant());
    data->model->deleteLater();
    delete data;
    iupAttribSet(ih, "_IUPQML_LIST", nullptr);
  }
  iupAttribSet(ih, "_IUPQML_LIST_BACK", nullptr);
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = qmlListMapMethod;
  ic->UnMap = qmlListUnMapMethod;
  ic->LayoutUpdate = qmlListLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttributeId(ic, "IDVALUE", qmlListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", qmlListGetValueAttrib, qmlListSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", nullptr, qmlListSetShowDropdownAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", nullptr, qmlListSetTopItemAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", nullptr, qmlListSetVisibleItemsAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DROPEXPAND", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, qmlListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, qmlListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, qmlListSetNCAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SCROLLBAR", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "SELECTEDTEXT", qmlListGetSelectedTextAttrib, qmlListSetSelectedTextAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", qmlListGetSelectionAttrib, qmlListSetSelectionAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", qmlListGetCaretAttrib, qmlListSetCaretAttrib, nullptr, nullptr, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", nullptr, qmlListSetInsertAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", nullptr, qmlListSetAppendAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", qmlListGetReadOnlyAttrib, qmlListSetReadOnlyAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", nullptr, qmlListSetClipboardAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", nullptr, qmlListSetScrollToAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", nullptr, qmlListSetCueBannerAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", nullptr, qmlListSetFilterAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", qmlListGetScrollVisibleAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", nullptr, qmlListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", qmlListGetImageNativeHandleAttribId, nullptr, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);
}

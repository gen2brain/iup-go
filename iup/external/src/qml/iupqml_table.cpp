/** \file
 * \brief IupTable control - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include <QQuickItem>
#include <QPointer>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QString>
#include <QList>
#include <QPointF>
#include <QVariant>
#include <QTimer>
#include <QCursor>

#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_table.h"
}

#include "iupqml_drv.h"


#define IUPQML_TABLE_ROLE_IMAGE      (Qt::UserRole + 1)
#define IUPQML_TABLE_ROLE_FGCOLOR    (Qt::UserRole + 2)
#define IUPQML_TABLE_ROLE_BGCOLOR    (Qt::UserRole + 3)
#define IUPQML_TABLE_ROLE_FONT       (Qt::UserRole + 4)
#define IUPQML_TABLE_ROLE_HASFONT    (Qt::UserRole + 5)
#define IUPQML_TABLE_ROLE_ALIGNMENT  (Qt::UserRole + 6)
#define IUPQML_TABLE_ROLE_SORTSIGN   (Qt::UserRole + 7)
#define IUPQML_TABLE_ROLE_CELLSEL    (Qt::UserRole + 8)
#define IUPQML_TABLE_ROLE_ROWHEIGHT  (Qt::UserRole + 9)

#define IUPQML_TABLE_SELECT_SINGLE   0
#define IUPQML_TABLE_SELECT_EXTENDED 2

#define IUPQML_TABLE_BEHAVIOR_DISABLED 0
#define IUPQML_TABLE_BEHAVIOR_ROWS     2

static int qmlTableColWidthAttrib(Ihandle* ih, int col)
{
  char name[50];
  int w = 0;
  snprintf(name, sizeof(name), "RASTERWIDTH%d", col);
  char* ws = iupAttribGet(ih, name);
  if (!ws)
  {
    snprintf(name, sizeof(name), "WIDTH%d", col);
    ws = iupAttribGet(ih, name);
  }
  if (ws && iupStrToInt(ws, &w) && w > 0)
    return w;
  return 0;
}

static int qmlTableGetColumnAlignment(Ihandle* ih, int col)
{
  char name[50];
  snprintf(name, sizeof(name), "ALIGNMENT%d", col);
  char* align_str = iupAttribGet(ih, name);

  if (!align_str)
    return Qt::AlignLeft;

  if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
    return Qt::AlignRight;
  else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
    return Qt::AlignHCenter;
  else
    return Qt::AlignLeft;
}

static int qmlTableIsColumnEditable(Ihandle* ih, int col)
{
  char name[50];
  snprintf(name, sizeof(name), "EDITABLE%d", col);
  char* editable_str = iupAttribGet(ih, name);

  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");

  return iupStrBoolean(editable_str);
}

/****************************************************************************
 * Model
 ****************************************************************************/

class IupQmlTableModel : public QAbstractTableModel
{
public:
  Ihandle* ih;
  int num_lin;
  int num_col;
  QList<QStringList> cells;
  QList<QList<QPixmap*> > images;
  QStringList titles;
  int sort_col;
  int sort_ascending;

  IupQmlTableModel(Ihandle* handle) : QAbstractTableModel(nullptr), ih(handle), num_lin(0), num_col(0), sort_col(0), sort_ascending(1) {}

  bool isVirtual() const
  {
    return iupStrBoolean(iupAttribGet(ih, "VIRTUALMODE")) ? true : false;
  }

  int rowCount(const QModelIndex& parent = QModelIndex()) const override
  {
    if (parent.isValid())
      return 0;
    return num_lin;
  }

  int columnCount(const QModelIndex& parent = QModelIndex()) const override
  {
    if (parent.isValid())
      return 0;
    return num_col;
  }

  QHash<int, QByteArray> roleNames() const override
  {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[IUPQML_TABLE_ROLE_IMAGE] = "image";
    roles[IUPQML_TABLE_ROLE_FGCOLOR] = "fgcolor";
    roles[IUPQML_TABLE_ROLE_BGCOLOR] = "bgcolor";
    roles[IUPQML_TABLE_ROLE_FONT] = "font";
    roles[IUPQML_TABLE_ROLE_HASFONT] = "hasfont";
    roles[IUPQML_TABLE_ROLE_ALIGNMENT] = "alignment";
    roles[IUPQML_TABLE_ROLE_SORTSIGN] = "sortsign";
    roles[IUPQML_TABLE_ROLE_CELLSEL] = "cellselected";
    return roles;
  }

  QString cellText(int lin, int col) const
  {
    if (isVirtual())
    {
      auto value_cb = reinterpret_cast<sIFnii>(IupGetCallback(ih, "VALUE_CB"));
      if (value_cb)
      {
        char* value = value_cb(ih, lin, col);
        return QString::fromUtf8(value ? value : "");
      }
      return {};
    }

    if (lin - 1 < cells.size() && col - 1 < cells.at(lin - 1).size())
      return cells.at(lin - 1).at(col - 1);
    return {};
  }

  QPixmap* cellImage(int lin, int col) const
  {
    if (!ih->data->show_image)
      return nullptr;

    if (isVirtual())
    {
      char* image_name = iupTableGetCellImageCb(ih, lin, col);
      if (image_name)
        return static_cast<QPixmap*>(iupImageGetImage(image_name, ih, 0, nullptr));
      return nullptr;
    }

    if (lin - 1 < images.size() && col - 1 < images.at(lin - 1).size())
      return images.at(lin - 1).at(col - 1);
    return nullptr;
  }

  QVariant data(const QModelIndex& index, int role) const override
  {
    if (!index.isValid() || index.row() < 0 || index.row() >= num_lin || index.column() < 0 || index.column() >= num_col)
      return {};

    int lin = index.row() + 1;
    int col = index.column() + 1;

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return cellText(lin, col);

    case IUPQML_TABLE_ROLE_IMAGE:
      {
        QPixmap* pixmap = cellImage(lin, col);
        return pixmap ? iupqmlImageUrl(pixmap) : QString();
      }

    case IUPQML_TABLE_ROLE_CELLSEL:
      return iupTableCellsIsSelected(ih, lin, col) ? true : false;

    case IUPQML_TABLE_ROLE_ROWHEIGHT:
      {
        int height = 0;
        if (ih->data->show_image && !ih->data->fit_image)
        {
          for (int c = 1; c <= num_col; c++)
          {
            QPixmap* pixmap = cellImage(lin, c);
            if (pixmap && pixmap->height() + 4 > height)
              height = pixmap->height() + 4;
          }
        }
        return height;
      }

    case IUPQML_TABLE_ROLE_BGCOLOR:
      {
        char* bgcolor = iupTableCellsIsSelected(ih, lin, col) ? iupTableCellsBgColor() : iupAttribGetId2(ih, "BGCOLOR", lin, col);
        if (!bgcolor)
          bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);
        if (!bgcolor)
          bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);
        if (!bgcolor && iupStrBoolean(iupAttribGet(ih, "ALTERNATECOLOR")))
          bgcolor = iupAttribGet(ih, (lin % 2 == 0) ? "EVENROWCOLOR" : "ODDROWCOLOR");
        if (!bgcolor)
          bgcolor = iupAttribGet(ih, "BGCOLOR");

        unsigned char r, g, b;
        if (bgcolor && *bgcolor && iupStrToRGB(bgcolor, &r, &g, &b))
          return QColor(r, g, b);
        return {};
      }

    case IUPQML_TABLE_ROLE_FGCOLOR:
      {
        char* fgcolor = iupTableCellsIsSelected(ih, lin, col) ? iupTableCellsFgColor() : iupAttribGetId2(ih, "FGCOLOR", lin, col);
        if (!fgcolor)
          fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);
        if (!fgcolor)
          fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);
        if (!fgcolor)
          fgcolor = iupAttribGet(ih, "FGCOLOR");

        unsigned char r, g, b;
        if (fgcolor && *fgcolor && iupStrToRGB(fgcolor, &r, &g, &b))
          return QColor(r, g, b);
        return {};
      }

    case IUPQML_TABLE_ROLE_FONT:
    case IUPQML_TABLE_ROLE_HASFONT:
      {
        char* font = iupAttribGetId2(ih, "FONT", lin, col);
        if (!font)
          font = iupAttribGetId2(ih, "FONT", 0, col);
        if (!font)
          font = iupAttribGetId2(ih, "FONT", lin, 0);

        QFont* qfont = font ? iupqmlGetQFont(font) : nullptr;
        if (role == IUPQML_TABLE_ROLE_HASFONT)
          return qfont ? true : false;
        return qfont ? QVariant::fromValue(*qfont) : QVariant();
      }

    case IUPQML_TABLE_ROLE_ALIGNMENT:
      return qmlTableGetColumnAlignment(ih, col);

    default:
      return {};
    }
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation != Qt::Horizontal || section < 0 || section >= num_col)
      return {};

    if (role == Qt::DisplayRole)
      return section < titles.size() ? titles.at(section) : QString();

    if (role == IUPQML_TABLE_ROLE_SORTSIGN)
    {
      if (sort_col == section + 1)
        return QString::fromUtf8(sort_ascending ? "\xE2\x96\xB2" : "\xE2\x96\xBC");
      return QString();
    }

    return {};
  }

  Qt::ItemFlags flags(const QModelIndex& index) const override
  {
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.isValid() && qmlTableIsColumnEditable(ih, index.column() + 1))
      f |= Qt::ItemIsEditable;
    return f;
  }

  void ensureStorage()
  {
    while (cells.size() < num_lin)
    {
      cells.append(QStringList());
      images.append(QList<QPixmap*>());
    }
    for (int r = 0; r < num_lin; r++)
    {
      while (cells[r].size() < num_col)
        cells[r].append(QString());
      while (images[r].size() < num_col)
        images[r].append(nullptr);
    }
  }

  void setCell(int lin, int col, const QString& value)
  {
    ensureStorage();
    cells[lin - 1][col - 1] = value;
    QModelIndex index = this->index(lin - 1, col - 1);
    emit dataChanged(index, index);
  }

  void setImage(int lin, int col, QPixmap* pixmap)
  {
    ensureStorage();
    images[lin - 1][col - 1] = pixmap;
    QModelIndex index = this->index(lin - 1, col - 1);
    emit dataChanged(index, index);
  }

  void setRows(int count)
  {
    if (count == num_lin)
      return;

    if (count > num_lin)
    {
      beginInsertRows(QModelIndex(), num_lin, count - 1);
      num_lin = count;
      ensureStorage();
      endInsertRows();
    }
    else
    {
      beginRemoveRows(QModelIndex(), count, num_lin - 1);
      num_lin = count;
      while (cells.size() > num_lin)
      {
        cells.removeLast();
        images.removeLast();
      }
      endRemoveRows();
    }
  }

  void setColumns(int count)
  {
    if (count == num_col)
      return;

    if (count > num_col)
    {
      beginInsertColumns(QModelIndex(), num_col, count - 1);
      num_col = count;
      ensureStorage();
      while (titles.size() < num_col)
        titles.append(QString());
      endInsertColumns();
    }
    else
    {
      beginRemoveColumns(QModelIndex(), count, num_col - 1);
      num_col = count;
      for (int r = 0; r < cells.size(); r++)
      {
        while (cells[r].size() > num_col)
        {
          cells[r].removeLast();
          images[r].removeLast();
        }
      }
      while (titles.size() > num_col)
        titles.removeLast();
      endRemoveColumns();
    }
  }

  void insertRow(int pos)
  {
    ensureStorage();
    beginInsertRows(QModelIndex(), pos, pos);
    QStringList row;
    QList<QPixmap*> row_images;
    for (int c = 0; c < num_col; c++)
    {
      row.append(QString());
      row_images.append(nullptr);
    }
    cells.insert(pos, row);
    images.insert(pos, row_images);
    num_lin++;
    endInsertRows();
  }

  void removeRow(int pos)
  {
    ensureStorage();
    beginRemoveRows(QModelIndex(), pos, pos);
    cells.removeAt(pos);
    images.removeAt(pos);
    num_lin--;
    endRemoveRows();
  }

  void insertColumn(int pos)
  {
    ensureStorage();
    beginInsertColumns(QModelIndex(), pos, pos);
    for (int r = 0; r < cells.size(); r++)
    {
      cells[r].insert(pos, QString());
      images[r].insert(pos, nullptr);
    }
    titles.insert(pos, QString());
    num_col++;
    if (sort_col > pos)
      sort_col++;
    endInsertColumns();
  }

  void removeColumn(int pos)
  {
    ensureStorage();
    beginRemoveColumns(QModelIndex(), pos, pos);
    for (int r = 0; r < cells.size(); r++)
    {
      cells[r].removeAt(pos);
      images[r].removeAt(pos);
    }
    titles.removeAt(pos);
    num_col--;
    if (sort_col == pos + 1)
      sort_col = 0;
    else if (sort_col > pos + 1)
      sort_col--;
    endRemoveColumns();
  }

  void moveRow(int from, int to)
  {
    ensureStorage();
    if (from == to)
      return;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to + 1 : to))
      return;
    cells.move(from, to);
    images.move(from, to);
    endMoveRows();
  }

  void moveColumn(int from_col, int to_col)
  {
    ensureStorage();
    if (from_col == to_col)
      return;
    if (!beginMoveColumns(QModelIndex(), from_col - 1, from_col - 1, QModelIndex(), to_col > from_col ? to_col : to_col - 1))
      return;
    for (int r = 0; r < cells.size(); r++)
    {
      cells[r].move(from_col - 1, to_col - 1);
      images[r].move(from_col - 1, to_col - 1);
    }
    titles.move(from_col - 1, to_col - 1);
    if (sort_col > 0)
      sort_col = iupTableMoveColPos(sort_col, from_col, to_col);
    endMoveColumns();
  }

  void reorderRows(const std::vector<int>& order)
  {
    ensureStorage();
    QList<QStringList> new_cells;
    QList<QList<QPixmap*> > new_images;
    for (int pos : order)
    {
      new_cells.append(cells.at(pos - 1));
      new_images.append(images.at(pos - 1));
    }

    beginResetModel();
    cells = new_cells;
    images = new_images;
    endResetModel();
  }

  void refreshAll()
  {
    if (num_lin > 0 && num_col > 0)
      emit dataChanged(index(0, 0), index(num_lin - 1, num_col - 1));
  }

  void refreshHeader()
  {
    if (num_col > 0)
      emit headerDataChanged(Qt::Horizontal, 0, num_col - 1);
  }
};

struct IupQmlTableData
{
  IupQmlTableModel* model = nullptr;
  QItemSelectionModel* selection = nullptr;
  QQuickItem* root = nullptr;
  QQuickItem* view = nullptr;
  QQuickItem* header = nullptr;
  int edit_lin = 0;
  int edit_col = 0;
  QString edit_initial;
  QPointer<QObject> editor;
  int drag_lin = 0;
  QList<int> move_old;
  QList<int> move_new;
};

static IupQmlTableData* qmlTableGetData(Ihandle* ih)
{
  return reinterpret_cast<IupQmlTableData*>(iupAttribGet(ih, "_IUPQML_TABLE"));
}

/****************************************************************************
 * Helpers
 ****************************************************************************/

static void qmlTableUpdateSelectionMode(Ihandle* ih)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  char* sel_mode = iupAttribGetStr(ih, "SELECTIONMODE");
  int behavior = IUPQML_TABLE_BEHAVIOR_ROWS;
  int mode = IUPQML_TABLE_SELECT_SINGLE;

  if (sel_mode)
  {
    if (iupStrEqualNoCase(sel_mode, "MULTIPLE") || iupStrEqualNoCase(sel_mode, "EXTENDED"))
      mode = IUPQML_TABLE_SELECT_EXTENDED;
    else if (iupStrEqualNoCase(sel_mode, "NONE") || iupStrEqualNoCase(sel_mode, "CELLS"))
      behavior = IUPQML_TABLE_BEHAVIOR_DISABLED;
  }

  data->view->setProperty("selectionBehavior", behavior);
  data->view->setProperty("selectionMode", mode);
  data->root->setProperty("iupCellsMode", iupTableCellsMode(ih) ? true : false);
}

static bool qmlTableSelectionDisabled(Ihandle* ih)
{
  char* sel_mode = iupAttribGetStr(ih, "SELECTIONMODE");
  return sel_mode && (iupStrEqualNoCase(sel_mode, "NONE") || iupStrEqualNoCase(sel_mode, "CELLS"));
}

static void qmlTableUpdateRowHeight(Ihandle* ih)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  data->root->setProperty("iupRowHeight", iupdrvTableGetRowHeight(ih));

  QFont* font = iupqmlGetIhFont(ih);
  if (font)
    data->root->setProperty("iupFont", QVariant::fromValue(*font));

  unsigned char r, g, b;
  char* bgcolor = iupAttribGet(ih, "BGCOLOR");
  if (bgcolor && iupStrToRGB(bgcolor, &r, &g, &b))
    data->root->setProperty("iupBgColor", QColor(r, g, b));
  else
    data->root->setProperty("iupBgColor", QVariant());
}

static QList<qreal> qmlTableUserWidths(IupQmlTableData* data)
{
  QList<qreal> widths;
  for (int col = 0; col < data->model->num_col; col++)
  {
    qreal w = -1;
    QMetaObject::invokeMethod(data->view, "explicitColumnWidth", Qt::DirectConnection, Q_RETURN_ARG(qreal, w), Q_ARG(int, col));
    widths.append(w);
  }
  return widths;
}

static void qmlTableApplyColumnWidths(Ihandle* ih, const QList<qreal>* user)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  bool last_col_has_width = qmlTableColWidthAttrib(ih, ih->data->num_col) > 0;
  bool stretch_last = (ih->data->stretch_last && !last_col_has_width);
  data->root->setProperty("iupStretchLast", stretch_last);

  for (int col = 1; col <= ih->data->num_col; col++)
  {
    int width = qmlTableColWidthAttrib(ih, col);
    qreal w = width > 0 ? static_cast<qreal>(width) : static_cast<qreal>(-1);
    if (w < 0 && user && col - 1 < user->size() && user->at(col - 1) > 0)
      w = user->at(col - 1);
    QMetaObject::invokeMethod(data->view, "setColumnWidth", Qt::DirectConnection, Q_ARG(int, col - 1), Q_ARG(qreal, w));
  }

  QMetaObject::invokeMethod(data->view, "forceLayout", Qt::DirectConnection);
}

static void qmlTableSortRows(Ihandle* ih, int col, int ascending)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  IupQmlTableModel* model = data->model;
  int num_lin = model->num_lin;
  int num_col = model->num_col;
  int focus_lin, focus_col;
  int sel_count = 0;
  int* selected;
  char* ignore;

  if (num_lin < 2 || col < 1 || col > num_col)
    return;

  iupdrvTableGetFocusCell(ih, &focus_lin, &focus_col);

  std::vector<QByteArray> keys(num_lin);
  std::vector<int> order(num_lin);
  for (int r = 0; r < num_lin; r++)
  {
    keys[r] = model->cellText(r + 1, col).toUtf8();
    order[r] = r + 1;
  }

  std::stable_sort(order.begin(), order.end(), [&keys, ascending](int a, int b) {
    int cmp = iupStrCompare(keys[a - 1].constData(), keys[b - 1].constData(), 0, 1);
    return ascending ? cmp < 0 : cmp > 0;
  });

  std::vector<int> new_pos(num_lin + 1);
  for (int i = 0; i < num_lin; i++)
    new_pos[order[i]] = i + 1;

  selected = iupdrvTableGetSelectedLins(ih, &sel_count);

  ignore = iupAttribGet(ih, "_IUPTABLE_IGNORE_SELECTION_CB");
  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", "1");

  double content_x = data->view->property("contentX").toDouble();
  double content_y = data->view->property("contentY").toDouble();

  model->reorderRows(order);
  iupTableSortLinAttribs(ih, order.data());

  QMetaObject::invokeMethod(data->view, "forceLayout", Qt::DirectConnection);
  data->view->setProperty("contentX", content_x);
  data->view->setProperty("contentY", content_y);

  if (focus_lin > 0 && focus_col > 0)
    data->selection->setCurrentIndex(model->index(new_pos[focus_lin] - 1, focus_col - 1), QItemSelectionModel::NoUpdate);

  data->selection->clearSelection();
  for (int i = 0; i < sel_count; i++)
    iupdrvTableSelectLin(ih, new_pos[selected[i]], 1);

  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", ignore);

  if (selected)
    free(selected);
}

static void qmlTableMoveColumn(Ihandle* ih, int from_col, int to_col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  int current_lin, current_col;
  iupdrvTableGetFocusCell(ih, &current_lin, &current_col);

  int sel_count = 0;
  int* selected = iupdrvTableGetSelectedLins(ih, &sel_count);

  char* ignore = iupAttribGet(ih, "_IUPTABLE_IGNORE_SELECTION_CB");
  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", "1");

  QList<qreal> widths = qmlTableUserWidths(data);
  widths.move(from_col - 1, to_col - 1);

  data->model->moveColumn(from_col, to_col);
  iupTableMoveColAttribs(ih, from_col, to_col);

  if (current_lin > 0 && current_col > 0)
    data->selection->setCurrentIndex(data->model->index(current_lin - 1, iupTableMoveColPos(current_col, from_col, to_col) - 1), QItemSelectionModel::NoUpdate);

  data->selection->clearSelection();
  for (int i = 0; i < sel_count; i++)
    iupdrvTableSelectLin(ih, selected[i], 1);

  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", ignore);

  if (selected)
    free(selected);

  qmlTableApplyColumnWidths(ih, &widths);
  data->model->refreshHeader();
}

static QModelIndex qmlTableIndexAt(IupQmlTableData* data, const QPointF& scene_pos)
{
  QQuickItem* content = iupqmlGetItemProperty(data->view, "contentItem");
  QPointF local = content ? content->mapFromScene(scene_pos) : data->view->mapFromScene(scene_pos);
  QPoint cell;
  QMetaObject::invokeMethod(data->view, "cellAtPosition", Qt::DirectConnection, Q_RETURN_ARG(QPoint, cell), Q_ARG(QPointF, local), Q_ARG(bool, true));
  if (cell.x() < 0 || cell.y() < 0)
    return {};
  return data->model->index(cell.y(), cell.x());
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qmlTableCurrentChanged(Ihandle* ih, const QModelIndex& current)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || !current.isValid())
    return;

  int lin = current.row() + 1;
  int col = current.column() + 1;

  Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();

  if (!qmlTableSelectionDisabled(ih) && !iupAttribGet(ih, "_IUPQML_TABLE_IN_SELECT") && !iupAttribGet(ih, "_IUPQML_TABLE_KEEP_SEL"))
  {
    if (!(mods & (Qt::ControlModifier | Qt::ShiftModifier)))
    {
      if (!data->selection->isRowSelected(current.row(), QModelIndex()) || data->selection->selectedRows().size() != 1)
      {
        iupAttribSet(ih, "_IUPQML_TABLE_IN_SELECT", "1");
        data->selection->select(current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        iupAttribSet(ih, "_IUPQML_TABLE_IN_SELECT", nullptr);
      }
    }
  }

  if (iupAttribGet(ih, "_IUPTABLE_IGNORE_SELECTION_CB"))
    return;

  if (!(iupTableCellsMode(ih) && (mods & Qt::ShiftModifier)))
    iupTableCellsCollapse(ih);

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "ENTERITEM_CB"));
  if (cb)
    cb(ih, lin, col);
}

static void qmlTableClicked(Ihandle* ih, int row, int column, int qt_button, int modifiers)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  int lin = row + 1;
  int col = column + 1;

  auto cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "CLICK_CB"));
  if (cb)
  {
    int button = 0;
    if (qt_button == Qt::LeftButton)
      button = IUP_BUTTON1;
    else if (qt_button == Qt::MiddleButton)
      button = IUP_BUTTON2;
    else if (qt_button == Qt::RightButton)
      button = IUP_BUTTON3;
    else if (qt_button == Qt::XButton1)
      button = IUP_BUTTON4;
    else if (qt_button == Qt::XButton2)
      button = IUP_BUTTON5;

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupqmlButtonKeySetStatus(Qt::KeyboardModifiers(modifiers), Qt::MouseButtons(qt_button), button, status, 0);
    cb(ih, lin, col, status);
  }

  if (qt_button == Qt::LeftButton && !iupTableCellsMode(ih) && !(modifiers & (Qt::ControlModifier | Qt::ShiftModifier)))
  {
    QModelIndex index = data->model->index(row, column);
    if (index.isValid() && data->selection->currentIndex() != index)
      data->selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
  }

  if (qt_button == Qt::LeftButton && iupTableCellsMode(ih))
  {
    if (modifiers & Qt::ShiftModifier)
      iupTableCellsExtendTo(ih, lin, col);
    else
    {
      data->selection->setCurrentIndex(data->model->index(row, column), QItemSelectionModel::NoUpdate);
      iupTableCellsCollapse(ih);
    }
    data->model->refreshAll();
  }

  if (qt_button == Qt::RightButton)
  {
    iupAttribSet(ih, "_IUPTABLE_CELLS_KEEP", "1");
    if (data->selection->isRowSelected(row, QModelIndex()))
      iupAttribSet(ih, "_IUPQML_TABLE_KEEP_SEL", "1");
    data->selection->setCurrentIndex(data->model->index(row, column), QItemSelectionModel::NoUpdate);
    iupAttribSet(ih, "_IUPQML_TABLE_KEEP_SEL", nullptr);
    iupAttribSet(ih, "_IUPTABLE_CELLS_KEEP", nullptr);

    auto rcb = reinterpret_cast<IFnii>(IupGetCallback(ih, "RIGHTCLICK_CB"));
    if (rcb)
      rcb(ih, lin, col);
  }
}

static void qmlTableCellsDrag(Ihandle* ih, const QPointF& scene_pos)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || !iupTableCellsMode(ih))
    return;

  QModelIndex index = qmlTableIndexAt(data, scene_pos);
  if (index.isValid())
  {
    iupTableCellsExtendTo(ih, index.row() + 1, index.column() + 1);
    data->model->refreshAll();
  }
}

static void qmlTableEditRequest(Ihandle* ih, int row, int column, const QString& initial)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || row < 0 || column < 0)
    return;

  int lin = row + 1;
  int col = column + 1;

  if (!qmlTableIsColumnEditable(ih, col))
    return;

  auto editbegin_cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "EDITBEGIN_CB"));
  if (editbegin_cb && editbegin_cb(ih, lin, col) == IUP_IGNORE)
    return;

  data->edit_lin = lin;
  data->edit_col = col;
  data->edit_initial = initial;

  QModelIndex index = data->model->index(row, column);
  QMetaObject::invokeMethod(data->view, "edit", Qt::DirectConnection, Q_ARG(QModelIndex, index));
}

static void qmlTableEditStarted(Ihandle* ih, QObject* editor)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || !editor)
    return;

  data->editor = editor;
  if (!data->edit_initial.isNull())
  {
    editor->setProperty("text", data->edit_initial);
    editor->setProperty("cursorPosition", data->edit_initial.length());
    data->edit_initial = QString();
  }
}

static void qmlTableEditCommit(Ihandle* ih, int row, int column, const QString& text)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  int lin = row + 1;
  int col = column + 1;

  data->edit_lin = 0;
  data->edit_col = 0;

  QString old_text = data->model->cellText(lin, col);

  auto editend_cb = reinterpret_cast<IFniisi>(IupGetCallback(ih, "EDITEND_CB"));
  if (editend_cb && editend_cb(ih, lin, col, const_cast<char*>(text.toUtf8().constData()), 1) == IUP_IGNORE)
    return;

  if (text == old_text)
    return;

  if (!data->model->isVirtual())
    data->model->setCell(lin, col, text);

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "VALUECHANGED_CB"));
  if (cb)
    cb(ih, lin, col);
}

static void qmlTableEditClosed(Ihandle* ih, int row, int column, const QString& text)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  int lin = row + 1;
  int col = column + 1;

  if (data->edit_lin != lin || data->edit_col != col)
    return;

  data->edit_lin = 0;
  data->edit_col = 0;

  auto editend_cb = reinterpret_cast<IFniisi>(IupGetCallback(ih, "EDITEND_CB"));
  if (editend_cb)
    editend_cb(ih, lin, col, const_cast<char*>(text.toUtf8().constData()), 0);
}

static void qmlTablePressed(Ihandle* ih, int row, int column, int modifiers)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (iupAttribGetBoolean(ih, "CANFOCUS") && data->view->property("keyNavigationEnabled").toBool())
    data->view->forceActiveFocus(Qt::MouseFocusReason);

  if (data->editor && data->edit_lin > 0)
  {
    QObject* editor = data->editor;
    data->editor = nullptr;
    qmlTableEditCommit(ih, data->edit_lin - 1, data->edit_col - 1, editor->property("text").toString());
    QMetaObject::invokeMethod(data->view, "closeEditor", Qt::DirectConnection);
  }

  if (modifiers & (Qt::ControlModifier | Qt::ShiftModifier))
    return;

  QModelIndex index = data->model->index(row, column);
  if (!index.isValid() || data->selection->currentIndex() == index)
    return;
  if (!iupTableCellsMode(ih) && !qmlTableSelectionDisabled(ih) && data->selection->isRowSelected(row, QModelIndex()))
    return;
  data->selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
}

static void qmlTableHeaderClicked(Ihandle* ih, int column)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || !ih->data->sortable)
    return;

  int col = column + 1;
  int prev_col = data->model->sort_col;
  int prev_ascending = data->model->sort_ascending;
  int ascending = (prev_col == col) ? !prev_ascending : 1;

  IFni sort_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SORT_CB"));
  if (sort_cb && sort_cb(ih, col) == IUP_IGNORE)
    return;

  data->model->sort_col = col;
  data->model->sort_ascending = ascending;
  data->model->refreshHeader();

  if (!data->model->isVirtual())
    qmlTableSortRows(ih, col, ascending);
}

static void qmlTableApplyColumnMove(Ihandle* ih)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || data->move_old.isEmpty())
    return;

  int best = 0;
  QPoint cell(-1, -1);
  QQuickItem* header_content = iupqmlGetItemProperty(data->header, "contentItem");
  QPointF local = header_content ? header_content->mapFromGlobal(QCursor::pos()) : data->header->mapFromGlobal(QCursor::pos());
  QMetaObject::invokeMethod(data->header, "cellAtPosition", Qt::DirectConnection, Q_RETURN_ARG(QPoint, cell), Q_ARG(QPointF, local), Q_ARG(bool, false));
  for (int i = 1; i < data->move_old.size(); i++)
  {
    if (data->move_new[best] == cell.x())
      break;
    if (data->move_new[i] == cell.x() || qAbs(data->move_new[i] - data->move_old[i]) > qAbs(data->move_new[best] - data->move_old[best]))
      best = i;
  }
  int old_visual = data->move_old[best];
  int new_visual = data->move_new[best];
  data->move_old.clear();
  data->move_new.clear();

  iupAttribSet(ih, "_IUPQML_TABLE_REORDERING", "1");
  QMetaObject::invokeMethod(data->view, "clearColumnReordering", Qt::DirectConnection);
  QMetaObject::invokeMethod(data->header, "clearColumnReordering", Qt::DirectConnection);
  iupAttribSet(ih, "_IUPQML_TABLE_REORDERING", nullptr);

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "REORDER_CB"));
  int ret = cb ? cb(ih, old_visual + 1, new_visual + 1) : IUP_DEFAULT;
  if (ret != IUP_IGNORE)
    qmlTableMoveColumn(ih, old_visual + 1, new_visual + 1);
  if (ret == IUP_CLOSE)
    IupExitLoop();
}

static void qmlTableColumnMoved(Ihandle* ih, int logical, int old_visual, int new_visual)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  (void)logical;
  if (!data || iupAttribGet(ih, "_IUPQML_TABLE_REORDERING"))
    return;

  bool first = data->move_old.isEmpty();
  data->move_old.append(old_visual);
  data->move_new.append(new_visual);
  if (first)
    QTimer::singleShot(0, data->view, [ih]() {
      if (iupObjectCheck(ih))
        qmlTableApplyColumnMove(ih);
    });
}

static void qmlTableRowDropped(Ihandle* ih, int drag_row, const QPointF& scene_pos)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || !ih->data->show_dragdrop)
    return;

  int drag_id = drag_row + 1;
  QModelIndex drop_index = qmlTableIndexAt(data, scene_pos);
  int drop_id;
  if (drop_index.isValid())
  {
    drop_id = drop_index.row() + 1;
    QPointF local = data->view->mapFromScene(scene_pos);
    QQuickItem* item = nullptr;
    QMetaObject::invokeMethod(data->view, "itemAtIndex", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, item), Q_ARG(QModelIndex, drop_index));
    if (item)
    {
      QPointF in_item = item->mapFromItem(data->view, local);
      if (in_item.y() > item->height() / 2)
        drop_id++;
    }
  }
  else
    drop_id = -1;

  int is_ctrl = 0;
  if (iupTableCallDragDropCb(ih, drag_id - 1, drop_id - 1, &is_ctrl) == IUP_CONTINUE && !data->model->isVirtual())
  {
    int src = drag_id - 1;
    int dest;

    if (drop_id < 1)
      dest = data->model->num_lin - 1;
    else
      dest = (drop_id - 1 > src) ? drop_id - 2 : drop_id - 1;

    char* ignore = iupAttribGet(ih, "_IUPTABLE_IGNORE_SELECTION_CB");
    iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", "1");

    data->model->moveRow(src, dest);
    iupTableMoveLinAttribs(ih, src + 1, dest + 1);
    iupdrvTableSetFocusCell(ih, dest + 1, 1);

    iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", ignore);
  }
}

/****************************************************************************
 * Key Filter
 ****************************************************************************/

class IupQmlTableFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlTableFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    (void)obj;
    if (event->type() != QEvent::KeyPress || !iupObjectCheck(ih))
      return false;

    IupQmlTableData* data = qmlTableGetData(ih);
    if (!data)
      return false;

    auto* evt = static_cast<QKeyEvent*>(event);
    QModelIndex current = data->selection->currentIndex();

    if (iupTableCellsMode(ih))
    {
      if (evt->modifiers() & Qt::ShiftModifier)
      {
        int dlin = 0, dcol = 0;
        switch (evt->key())
        {
          case Qt::Key_Left:  dcol = -1; break;
          case Qt::Key_Right: dcol = 1; break;
          case Qt::Key_Up:    dlin = -1; break;
          case Qt::Key_Down:  dlin = 1; break;
        }
        if (dlin || dcol)
        {
          iupTableCellsExtendBy(ih, dlin, dcol);
          data->model->refreshAll();
          return true;
        }
      }
      else if (evt->matches(QKeySequence::SelectAll))
      {
        iupTableCellsSelectAll(ih);
        data->model->refreshAll();
        return true;
      }
    }

    if (evt->matches(QKeySequence::Copy))
    {
      if (current.isValid())
        QGuiApplication::clipboard()->setText(data->model->cellText(current.row() + 1, current.column() + 1));
      return true;
    }

    if (evt->matches(QKeySequence::Paste))
    {
      if (current.isValid() && qmlTableIsColumnEditable(ih, current.column() + 1))
      {
        QString text = QGuiApplication::clipboard()->text();
        if (!data->model->isVirtual())
          data->model->setCell(current.row() + 1, current.column() + 1, text);
        auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "VALUECHANGED_CB"));
        if (cb)
          cb(ih, current.row() + 1, current.column() + 1);
      }
      return true;
    }

    if (evt->key() == Qt::Key_Return || evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_F2)
    {
      if (current.isValid() && qmlTableIsColumnEditable(ih, current.column() + 1))
      {
        qmlTableEditRequest(ih, current.row(), current.column(), QString());
        return true;
      }
      return false;
    }

    QString typed = evt->text();
    if (current.isValid() && !typed.isEmpty() && typed.at(0).isPrint() && !(evt->modifiers() & (Qt::ControlModifier | Qt::AltModifier)))
    {
      if (qmlTableIsColumnEditable(ih, current.column() + 1))
      {
        qmlTableEditRequest(ih, current.row(), current.column(), typed);
        return true;
      }
    }

    return false;
  }
};

/****************************************************************************
 * Map Method
 ****************************************************************************/

static const char* qmlTableQml =
  IUPQML_IMPORTS
  "Item {\n"
  "  id: root\n"
  "  clip: true\n"
  "  property int iupRowHeight: 20\n"
  "  property font iupFont\n"
  "  property bool iupShowGrid: true\n"
  "  property bool iupFocusRect: true\n"
  "  property bool iupStretchLast: false\n"
  "  property bool iupReorder: false\n"
  "  property bool iupResize: false\n"
  "  property bool iupDragDrop: false\n"
  "  property bool iupCellsMode: false\n"
  "  property bool iupShowImage: false\n"
  "  property bool iupFitImage: true\n"
  "  property var iupBgColor: undefined\n"
  "  property font font\n"
  "  signal iupClicked(int row, int column, int button, int modifiers)\n"
  "  signal iupPressed(int row, int column, int modifiers)\n"
  "  signal iupDoubleClicked(int row, int column)\n"
  "  signal iupCellsDrag(point pos)\n"
  "  signal iupEditStarted(var editor)\n"
  "  signal iupEditCommit(int row, int column, string text)\n"
  "  signal iupEditClosed(int row, int column, string text)\n"
  "  signal iupHeaderClicked(int column)\n"
  "  signal iupRowDropped(int row, point pos)\n"
  "  function iupScrollTo(row, column) { view.forceLayout(); view.positionViewAtCell(Qt.point(column, row), TableView.Contain) }\n"
  "  function iupScrollVisible() { return (view.ScrollBar.horizontal.visible ? 1 : 0) + (view.ScrollBar.vertical.visible ? 2 : 0) }\n"
  "  function iupHeaderHeight() { return header.height }\n"
  "  Rectangle {\n"
  "    id: back\n"
  "    anchors.fill: parent\n"
  "    color: root.iupBgColor !== undefined ? root.iupBgColor : palette.base\n"
  "    border.width: 1\n"
  "    border.color: palette.mid\n"
  "  }\n"
  "  HorizontalHeaderView {\n"
  "    id: header\n"
  "    objectName: \"header\"\n"
  "    syncView: view\n"
  "    x: 1\n"
  "    y: 1\n"
  "    width: parent.width - 2\n"
  "    clip: true\n"
  "    movableColumns: root.iupReorder\n"
  "    resizableColumns: root.iupResize\n"
  "    delegate: HorizontalHeaderViewDelegate {\n"
  "      id: hd\n"
  "      required property int column\n"
  "      padding: 4\n"
  "      implicitWidth: hdrow.implicitWidth + leftPadding + rightPadding\n"
  "      contentItem: Item {\n"
  "        implicitWidth: hdrow.implicitWidth\n"
  "        implicitHeight: hdrow.implicitHeight\n"
  "      Row {\n"
  "        id: hdrow\n"
  "        anchors.centerIn: parent\n"
  "        spacing: 4\n"
  "        Label {\n"
  "          text: hd.model.display\n"
  "          font: root.iupFont\n"
  "          elide: Text.ElideRight\n"
  "          anchors.verticalCenter: parent.verticalCenter\n"
  "        }\n"
  "        Label {\n"
  "          text: hd.model.sortsign\n"
  "          visible: text !== \"\"\n"
  "          font: root.iupFont\n"
  "          anchors.verticalCenter: parent.verticalCenter\n"
  "        }\n"
  "      }\n"
  "      }\n"
  "      TapHandler { onTapped: root.iupHeaderClicked(hd.column) }\n"
  "    }\n"
  "  }\n"
  "  TableView {\n"
  "    id: view\n"
  "    objectName: \"view\"\n"
  "    x: 1\n"
  "    y: header.height + 1\n"
  "    width: parent.width - 2\n"
  "    height: parent.height - header.height - 2\n"
  "    clip: true\n"
  "    boundsBehavior: Flickable.StopAtBounds\n"
  "    selectionBehavior: TableView.SelectRows\n"
  "    pointerNavigationEnabled: false\n"
  "    keyNavigationEnabled: true\n"
  "    activeFocusOnTab: true\n"
  "    editTriggers: TableView.NoEditTriggers\n"
  "    resizableColumns: root.iupResize\n"
  "    rowHeightProvider: function(row) { return (root.iupShowImage && !root.iupFitImage) ? Math.max(root.iupRowHeight, model.data(model.index(row, 0), 265)) : root.iupRowHeight }\n"
  "    columnWidthProvider: function(column) {\n"
  "      var w = explicitColumnWidth(column)\n"
  "      if (w >= 0) return w\n"
  "      var iw = Math.max(implicitColumnWidth(column), header.implicitColumnWidth(column))\n"
  "      if (root.iupStretchLast && column === columns - 1) {\n"
  "        var used = 0\n"
  "        for (var c = 0; c < column; c++) used += columnWidth(c)\n"
  "        return Math.max(width - used, iw)\n"
  "      }\n"
  "      return iw\n"
  "    }\n"
  "    ScrollBar.vertical: ScrollBar { policy: size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff }\n"
  "    ScrollBar.horizontal: ScrollBar { policy: size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff }\n"
  "    delegate: Rectangle {\n"
  "      id: cell\n"
  "      required property var model\n"
  "      required property int row\n"
  "      required property int column\n"
  "      required property bool selected\n"
  "      required property bool current\n"
  "      required property bool editing\n"
  "      implicitWidth: lbl.implicitWidth + img.width + (img.visible ? 4 : 0) + 8\n"
  "      implicitHeight: root.iupRowHeight\n"
  "      color: cell.selected ? cell.palette.highlight : (cell.model.bgcolor ? cell.model.bgcolor : \"transparent\")\n"
  "      Rectangle { visible: root.iupShowGrid; anchors.right: parent.right; width: 1; height: parent.height; color: cell.palette.mid }\n"
  "      Rectangle { visible: root.iupShowGrid; anchors.bottom: parent.bottom; width: parent.width; height: 1; color: cell.palette.mid }\n"
  "      Row {\n"
  "        id: contentRow\n"
  "        anchors.fill: parent\n"
  "        anchors.leftMargin: 4\n"
  "        anchors.rightMargin: 4\n"
  "        spacing: 4\n"
  "        Image {\n"
  "          id: img\n"
  "          source: root.iupShowImage ? cell.model.image : \"\"\n"
  "          visible: source != \"\"\n"
  "          width: visible ? (root.iupFitImage ? implicitWidth * (height / Math.max(1, implicitHeight)) : implicitWidth) : 0\n"
  "          height: root.iupFitImage ? Math.min(implicitHeight, parent.height - 2) : implicitHeight\n"
  "          fillMode: Image.PreserveAspectFit\n"
  "          anchors.verticalCenter: parent.verticalCenter\n"
  "          cache: false\n"
  "        }\n"
  "        Label {\n"
  "          id: lbl\n"
  "          text: cell.model.display\n"
  "          width: parent.width - img.width - (img.visible ? parent.spacing : 0)\n"
  "          height: parent.height\n"
  "          elide: Text.ElideRight\n"
  "          horizontalAlignment: cell.model.alignment\n"
  "          verticalAlignment: Text.AlignVCenter\n"
  "          font: cell.model.hasfont ? cell.model.font : root.iupFont\n"
  "          color: cell.selected ? cell.palette.highlightedText : (cell.model.fgcolor ? cell.model.fgcolor : cell.palette.text)\n"
  "          visible: !cell.editing\n"
  "        }\n"
  "      }\n"
  "      Rectangle {\n"
  "        anchors.fill: parent\n"
  "        anchors.margins: 1\n"
  "        color: \"transparent\"\n"
  "        border.width: 1\n"
  "        border.color: cell.selected ? cell.palette.highlightedText : cell.palette.text\n"
  "        visible: cell.current && root.iupFocusRect && view.activeFocus && !cell.editing\n"
  "      }\n"
  "      TapHandler {\n"
  "        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton | Qt.XButton1 | Qt.XButton2\n"
  "        onTapped: function(eventPoint, button) { root.iupClicked(cell.row, cell.column, button, point.modifiers) }\n"
  "        onDoubleTapped: function(eventPoint, button) { if (button === Qt.LeftButton) root.iupDoubleClicked(cell.row, cell.column) }\n"
  "        onPressedChanged: if (pressed && (point.pressedButtons & Qt.LeftButton)) root.iupPressed(cell.row, cell.column, point.modifiers)\n"
  "      }\n"
  "      DragHandler {\n"
  "        enabled: root.iupDragDrop || root.iupCellsMode\n"
  "        target: null\n"
  "        onCentroidChanged: if (active && root.iupCellsMode) root.iupCellsDrag(centroid.scenePosition)\n"
  "        onActiveChanged: if (!active && root.iupDragDrop) root.iupRowDropped(cell.row, centroid.scenePosition)\n"
  "      }\n"
  "      TableView.editDelegate: TextField {\n"
  "        id: editor\n"
  "        anchors.fill: parent\n"
  "        text: cell.model.display\n"
  "        font: cell.model.hasfont ? cell.model.font : root.iupFont\n"
  "        horizontalAlignment: cell.model.alignment\n"
  "        Component.onCompleted: { selectAll(); root.iupEditStarted(editor) }\n"
  "        Component.onDestruction: root.iupEditClosed(cell.row, cell.column, editor.text)\n"
  "        TableView.onCommit: root.iupEditCommit(cell.row, cell.column, editor.text)\n"
  "      }\n"
  "    }\n"
  "  }\n"
  "}\n";

static int qmlTableMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  QQuickItem* root = iupqmlCreateItem(qmlTableQml);
  if (!root)
    return IUP_ERROR;

  auto* view = root->findChild<QQuickItem*>("view");
  auto* header = root->findChild<QQuickItem*>("header");
  if (!view || !header)
  {
    delete root;
    return IUP_ERROR;
  }

  auto* data = new IupQmlTableData();
  data->model = new IupQmlTableModel(ih);
  data->selection = new QItemSelectionModel(data->model);
  data->root = root;
  data->view = view;
  data->header = header;
  data->edit_lin = 0;
  data->edit_col = 0;
  data->drag_lin = -1;

  data->model->num_lin = ih->data->num_lin;
  data->model->num_col = ih->data->num_col;
  data->model->ensureStorage();
  while (data->model->titles.size() < ih->data->num_col)
    data->model->titles.append(QString());

  ih->handle = reinterpret_cast<InativeHandle*>(root);
  iupAttribSet(ih, "_IUPQML_TABLE", reinterpret_cast<char*>(data));

  view->setProperty("model", QVariant::fromValue<QObject*>(data->model));
  view->setProperty("selectionModel", QVariant::fromValue<QItemSelectionModel*>(data->selection));

  root->setProperty("iupShowGrid", iupAttribGetBoolean(ih, "SHOWGRID") ? true : false);
  root->setProperty("iupFocusRect", iupAttribGetBoolean(ih, "FOCUSRECT") ? true : false);
  root->setProperty("iupReorder", ih->data->allow_reorder ? true : false);
  root->setProperty("iupResize", ih->data->user_resize ? true : false);
  root->setProperty("iupDragDrop", ih->data->show_dragdrop ? true : false);
  root->setProperty("iupShowImage", ih->data->show_image ? true : false);
  root->setProperty("iupFitImage", ih->data->fit_image ? true : false);

  iupqmlSetIhandle(root, ih);
  iupqmlSetIhandle(view, ih);
  view->installEventFilter(new IupQmlTableFilter(view, ih));

  QObject::connect(data->selection, &QItemSelectionModel::selectionChanged, [ih](const QItemSelection&, const QItemSelection&) {
    iupTableCallMultiSelectionCb(ih);
  });
  QObject::connect(data->selection, &QItemSelectionModel::currentChanged, [ih](const QModelIndex& current, const QModelIndex&) {
    qmlTableCurrentChanged(ih, current);
  });

  iupqmlConnect(root, "iupPressed(int,int,int)", [ih](void** args) {
    qmlTablePressed(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]), *static_cast<int*>(args[3]));
  });
  iupqmlConnect(root, "iupClicked(int,int,int,int)", [ih](void** args) {
    qmlTableClicked(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]), *static_cast<int*>(args[3]), *static_cast<int*>(args[4]));
  });
  iupqmlConnect(root, "iupDoubleClicked(int,int)", [ih](void** args) {
    qmlTableEditRequest(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]), QString());
  });
  iupqmlConnect(root, "iupCellsDrag(QPointF)", [ih](void** args) {
    qmlTableCellsDrag(ih, *static_cast<QPointF*>(args[1]));
  });
  iupqmlConnect(root, "iupEditStarted(QVariant)", [ih](void** args) {
    qmlTableEditStarted(ih, qvariant_cast<QObject*>(*static_cast<QVariant*>(args[1])));
  });
  iupqmlConnect(root, "iupEditCommit(int,int,QString)", [ih](void** args) {
    qmlTableEditCommit(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]), *static_cast<QString*>(args[3]));
  });
  iupqmlConnect(root, "iupEditClosed(int,int,QString)", [ih](void** args) {
    qmlTableEditClosed(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]), *static_cast<QString*>(args[3]));
  });
  iupqmlConnect(root, "iupHeaderClicked(int)", [ih](void** args) {
    qmlTableHeaderClicked(ih, *static_cast<int*>(args[1]));
  });
  iupqmlConnect(root, "iupRowDropped(int,QPointF)", [ih](void** args) {
    qmlTableRowDropped(ih, *static_cast<int*>(args[1]), *static_cast<QPointF*>(args[2]));
  });
  iupqmlConnect(view, "columnMoved(int,int,int)", [ih](void** args) {
    qmlTableColumnMoved(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]), *static_cast<int*>(args[3]));
  });
  iupqmlConnect(root, "fontChanged()", [ih](void**) {
    qmlTableUpdateRowHeight(ih);
  });

  qmlTableUpdateSelectionMode(ih);
  qmlTableUpdateRowHeight(ih);
  qmlTableApplyColumnWidths(ih, nullptr);

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, view);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(view, 0);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

static void qmlTableUnMapMethod(Ihandle* ih)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
  {
    data->view->setProperty("selectionModel", QVariant());
    data->view->setProperty("model", QVariant());
    delete data->selection;
    delete data->model;
    delete data;
    iupAttribSet(ih, "_IUPQML_TABLE", nullptr);
  }

  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

static void qmlTableLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  IupQmlTableData* data = qmlTableGetData(ih);
  if (data && data->root->property("iupStretchLast").toBool())
    QMetaObject::invokeMethod(data->view, "forceLayout", Qt::DirectConnection);
}

/****************************************************************************
 * Driver Functions - Table Structure
 ****************************************************************************/

static int qmlTableFollowPos(int cur, int pos, int delta, int count)
{
  if (cur <= 0)
    return cur;
  if (cur >= pos && !(delta < 0 && cur == pos))
    cur += delta;
  if (cur > count)
    cur = count;
  return cur;
}

struct IqmlTableFollow
{
  int focus_lin = 0, focus_col = 0;
  int* selected = nullptr;
  int sel_count = 0;
};

static void qmlTableFollowBegin(Ihandle* ih, IqmlTableFollow* follow)
{
  iupdrvTableGetFocusCell(ih, &follow->focus_lin, &follow->focus_col);
  follow->selected = iupdrvTableGetSelectedLins(ih, &follow->sel_count);
}

static void qmlTableFollowEnd(Ihandle* ih, IqmlTableFollow* follow, int lin_pos, int lin_delta, int col_pos, int col_delta)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  int num_lin = data->model->num_lin;
  int num_col = data->model->num_col;
  int lin = qmlTableFollowPos(follow->focus_lin, lin_pos, lin_delta, num_lin);
  int col = qmlTableFollowPos(follow->focus_col, col_pos, col_delta, num_col);
  int new_count = 0, cur_count = 0;
  int* current;

  char* ignore = iupAttribGet(ih, "_IUPTABLE_IGNORE_SELECTION_CB");
  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", "1");

  if (lin > 0 && col > 0)
  {
    QModelIndex cur = data->selection->currentIndex();
    if (cur.row() != lin - 1 || cur.column() != col - 1)
      data->selection->setCurrentIndex(data->model->index(lin - 1, col - 1), QItemSelectionModel::NoUpdate);
  }

  for (int i = 0; i < follow->sel_count; i++)
  {
    int l = follow->selected[i];
    if (lin_delta < 0 && l == lin_pos)
      continue;
    if (l >= lin_pos)
      l += lin_delta;
    if (l <= num_lin)
      follow->selected[new_count++] = l;
  }

  current = iupdrvTableGetSelectedLins(ih, &cur_count);
  if (cur_count != new_count || (new_count && memcmp(current, follow->selected, new_count * sizeof(int)) != 0))
  {
    data->selection->clearSelection();
    for (int i = 0; i < new_count; i++)
      iupdrvTableSelectLin(ih, follow->selected[i], 1);
  }

  iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", ignore);

  if (current)
    free(current);
  if (follow->selected)
    free(follow->selected);
}

IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;

  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
  {
    IqmlTableFollow follow;
    QList<qreal> widths = qmlTableUserWidths(data);
    qmlTableFollowBegin(ih, &follow);
    data->model->setColumns(num_col);
    qmlTableFollowEnd(ih, &follow, ih->data->num_lin + 1, 0, num_col + 1, 0);
    qmlTableApplyColumnWidths(ih, &widths);
  }
}

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
  {
    IqmlTableFollow follow;
    qmlTableFollowBegin(ih, &follow);
    data->model->setRows(num_lin);
    qmlTableFollowEnd(ih, &follow, num_lin + 1, 0, ih->data->num_col + 1, 0);
  }
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (pos == 0)
    pos = ih->data->num_col + 1;

  if (pos < 1 || pos > ih->data->num_col + 1)
    return;

  IqmlTableFollow follow;
  QList<qreal> widths = qmlTableUserWidths(data);
  widths.insert(pos - 1, -1);
  qmlTableFollowBegin(ih, &follow);
  data->model->insertColumn(pos - 1);
  ih->data->num_col++;
  qmlTableFollowEnd(ih, &follow, ih->data->num_lin + 1, 0, pos, 1);
  qmlTableApplyColumnWidths(ih, &widths);
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (pos < 1 || pos > ih->data->num_col)
    return;

  IqmlTableFollow follow;
  QList<qreal> widths = qmlTableUserWidths(data);
  widths.removeAt(pos - 1);
  qmlTableFollowBegin(ih, &follow);
  data->model->removeColumn(pos - 1);
  ih->data->num_col--;
  qmlTableFollowEnd(ih, &follow, ih->data->num_lin + 1, 0, pos, -1);
  qmlTableApplyColumnWidths(ih, &widths);
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (pos == 0)
    pos = ih->data->num_lin + 1;

  if (pos < 1 || pos > ih->data->num_lin + 1)
    return;

  IqmlTableFollow follow;
  qmlTableFollowBegin(ih, &follow);
  data->model->insertRow(pos - 1);
  ih->data->num_lin++;
  qmlTableFollowEnd(ih, &follow, pos, 1, ih->data->num_col + 1, 0);
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  IqmlTableFollow follow;
  qmlTableFollowBegin(ih, &follow);
  data->model->removeRow(pos - 1);
  ih->data->num_lin--;
  qmlTableFollowEnd(ih, &follow, pos, -1, ih->data->num_col + 1, 0);
}

/****************************************************************************
 * Driver Functions - Cell Operations
 ****************************************************************************/

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (lin < 1 || lin > data->model->num_lin || col < 1 || col > data->model->num_col)
    return;

  data->model->setCell(lin, col, value ? QString::fromUtf8(value) : QString());
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return nullptr;

  if (lin < 1 || lin > data->model->num_lin || col < 1 || col > data->model->num_col)
    return nullptr;

  QString text = data->model->cellText(lin, col);
  if (text.isEmpty())
    return nullptr;

  return iupStrReturnStr(text.toUtf8().constData());
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (lin < 1 || lin > data->model->num_lin || col < 1 || col > data->model->num_col)
    return;

  QPixmap* pixmap = image ? static_cast<QPixmap*>(iupImageGetImage(image, ih, 0, nullptr)) : nullptr;
  data->model->setImage(lin, col, pixmap);

  if (ih->data->show_image && !ih->data->fit_image)
    QMetaObject::invokeMethod(data->view, "forceLayout", Qt::DirectConnection);
}

/****************************************************************************
 * Driver Functions - Column Operations
 ****************************************************************************/

IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (col < 1 || col > data->model->num_col)
    return;

  while (data->model->titles.size() < data->model->num_col)
    data->model->titles.append(QString());

  data->model->titles[col - 1] = title ? QString::fromUtf8(title) : QString();
  data->model->refreshHeader();
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return nullptr;

  if (col < 1 || col > data->model->titles.size())
    return nullptr;

  QString text = data->model->titles.at(col - 1);
  if (text.isEmpty())
    return nullptr;

  return iupStrReturnStr(text.toUtf8().constData());
}

IUP_SDK_API void iupdrvTableSetSortSign(Ihandle* ih, int col, int sign)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (sign == 0)
    data->model->sort_col = 0;
  else
  {
    data->model->sort_col = col;
    data->model->sort_ascending = sign > 0;
  }

  data->model->refreshHeader();
}

IUP_SDK_API int iupdrvTableGetSortSign(Ihandle* ih, int col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data || data->model->sort_col != col)
    return 0;

  return data->model->sort_ascending ? 1 : -1;
}

IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (col < 1 || col > data->model->num_col)
    return;

  qreal w = width > 0 ? static_cast<qreal>(width) : static_cast<qreal>(-1);
  QMetaObject::invokeMethod(data->view, "setColumnWidth", Qt::DirectConnection, Q_ARG(int, col - 1), Q_ARG(qreal, w));
  QMetaObject::invokeMethod(data->view, "forceLayout", Qt::DirectConnection);
}

IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return 0;

  if (col < 1 || col > data->model->num_col)
    return 0;

  qreal w = 0;
  QMetaObject::invokeMethod(data->view, "columnWidth", Qt::DirectConnection, Q_RETURN_ARG(qreal, w), Q_ARG(int, col - 1));
  if (w < 0)
    w = 0;
  return static_cast<int>(w);
}

/****************************************************************************
 * Driver Functions - Navigation and Display
 ****************************************************************************/

IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (lin < 1 || lin > data->model->num_lin || col < 1 || col > data->model->num_col)
    return;

  QModelIndex index = data->model->index(lin - 1, col - 1);
  if (qmlTableSelectionDisabled(ih))
    data->selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
  else
    data->selection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

  iupqmlCallMethod(data->root, "iupScrollTo", QVariant(lin - 1), QVariant(col - 1));
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  QModelIndex index = data->selection->currentIndex();
  *lin = index.row() + 1;
  *col = index.column() + 1;
}

IUP_SDK_API int iupdrvTableIsLinSelected(Ihandle* ih, int lin)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return 0;

  if (lin < 1 || lin > data->model->num_lin)
    return 0;

  return data->selection->isRowSelected(lin - 1, QModelIndex()) ? 1 : 0;
}

IUP_SDK_API void iupdrvTableSelectLin(Ihandle* ih, int lin, int select)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (lin < 1 || lin > data->model->num_lin || data->model->num_col == 0)
    return;

  QModelIndex first = data->model->index(lin - 1, 0);
  QModelIndex last = data->model->index(lin - 1, data->model->num_col - 1);

  data->selection->select(QItemSelection(first, last),
                          (select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) | QItemSelectionModel::Rows);
}

IUP_SDK_API int* iupdrvTableGetSelectedLins(Ihandle* ih, int* count)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  *count = 0;

  if (!data)
    return nullptr;

  QModelIndexList rows = data->selection->selectedRows();
  if (rows.isEmpty())
    return nullptr;

  QList<int> lins;
  for (const QModelIndex& index : rows)
    lins.append(index.row() + 1);

  std::sort(lins.begin(), lins.end());

  int* result = static_cast<int*>(malloc(sizeof(int) * lins.size()));
  for (int i = 0; i < lins.size(); i++)
    result[i] = lins[i];

  *count = static_cast<int>(lins.size());
  return result;
}

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  if (lin < 1 || lin > data->model->num_lin || col < 1 || col > data->model->num_col)
    return;

  iupqmlCallMethod(data->root, "iupScrollTo", QVariant(lin - 1), QVariant(col - 1));
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  data->model->refreshAll();
  data->model->refreshHeader();
}

IUP_SDK_API void iupdrvTableUpdateCellStyle(Ihandle* ih, int lin, int col)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (!data)
    return;

  int num_lin = data->model->num_lin;
  int num_col = data->model->num_col;
  if (num_lin == 0 || num_col == 0)
    return;

  if (lin == 0 && col == 0)
    qmlTableUpdateRowHeight(ih);

  int lin1 = lin > 0 ? lin - 1 : 0;
  int lin2 = lin > 0 ? lin - 1 : num_lin - 1;
  int col1 = col > 0 ? col - 1 : 0;
  int col2 = col > 0 ? col - 1 : num_col - 1;

  if (lin1 >= num_lin || col1 >= num_col)
    return;

  emit data->model->dataChanged(data->model->index(lin1, col1), data->model->index(lin2, col2));
}

IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
    data->root->setProperty("iupShowGrid", show ? true : false);
}

/****************************************************************************
 * Attribute Handlers
 ****************************************************************************/

static int qmlTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  ih->data->sortable = iupStrBoolean(value) ? 1 : 0;

  IupQmlTableData* data = qmlTableGetData(ih);
  if (data && !ih->data->sortable)
  {
    data->model->sort_col = 0;
    data->model->refreshHeader();
  }
  return 0;
}

static int qmlTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  ih->data->allow_reorder = iupStrBoolean(value) ? 1 : 0;

  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
    data->root->setProperty("iupReorder", ih->data->allow_reorder ? true : false);
  return 0;
}

static int qmlTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  ih->data->user_resize = iupStrBoolean(value) ? 1 : 0;

  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
  {
    QList<qreal> widths = qmlTableUserWidths(data);
    data->root->setProperty("iupResize", ih->data->user_resize ? true : false);
    qmlTableApplyColumnWidths(ih, &widths);
  }
  return 0;
}

static int qmlTableSetSelectionModeAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "SELECTIONMODE", value);
  if (ih->handle)
  {
    IupQmlTableData* data = qmlTableGetData(ih);
    qmlTableUpdateSelectionMode(ih);
    if (data && !iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTIONMODE"), "MULTIPLE"))
    {
      char* ignore = iupAttribGet(ih, "_IUPTABLE_IGNORE_SELECTION_CB");
      iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", "1");
      data->selection->clearSelection();
      iupAttribSet(ih, "_IUPTABLE_IGNORE_SELECTION_CB", ignore);
    }
  }
  return 1;
}

static int qmlTableSetFocusRectAttrib(Ihandle* ih, const char* value)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
    data->root->setProperty("iupFocusRect", iupStrBoolean(value) ? true : false);
  return 1;
}

/****************************************************************************
 * Class Registration
 ****************************************************************************/

extern "C" {

IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  return 1;
}

IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  int char_height;
  iupdrvFontGetCharSize(ih, nullptr, &char_height);
  int row = char_height + 8;
  if (row < 20)
    row = 20;
  return row;
}

IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  IupQmlTableData* data = qmlTableGetData(ih);
  if (data)
  {
    QVariant ret;
    QMetaObject::invokeMethod(data->root, "iupHeaderHeight", Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret));
    int h = static_cast<int>(ret.toDouble());
    if (h > 1)
      return h;
  }

  int char_height;
  iupdrvFontGetCharSize(ih, nullptr, &char_height);
  return char_height + 8;
}

IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int frame_width = iupdrvTableGetBorderWidth(ih);
  int sb_size = iupdrvGetScrollbarSize();

  *w += sb_size + 2 * frame_width;
  *h += 2 * frame_width;

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
    *h += sb_size;
}

IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = qmlTableMapMethod;
  ic->UnMap = qmlTableUnMapMethod;
  ic->LayoutUpdate = qmlTableLayoutUpdateMethod;

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", nullptr, qmlTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", nullptr, qmlTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", nullptr, qmlTableSetUserResizeAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "SELECTIONMODE", nullptr, qmlTableSetSelectionModeAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "FOCUSRECT", nullptr, qmlTableSetFocusRectAttrib);
}

}

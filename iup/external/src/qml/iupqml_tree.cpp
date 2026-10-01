/** \file
 * \brief Tree Control - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QQuickItem>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QList>
#include <QPointF>
#include <QVariant>
#include <QTimer>
#include <QStyleHints>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_tree.h"
}

#include "iupqml_drv.h"


#define IUPQML_TREE_SELECT_SINGLE   0
#define IUPQML_TREE_SELECT_EXTENDED 2

#define IUPQML_ROLE_TITLE          Qt::DisplayRole
#define IUPQML_ROLE_IMAGE          (Qt::UserRole + 1)
#define IUPQML_ROLE_FGCOLOR        (Qt::UserRole + 2)
#define IUPQML_ROLE_BGCOLOR        (Qt::UserRole + 3)
#define IUPQML_ROLE_FONT           (Qt::UserRole + 4)
#define IUPQML_ROLE_HASFONT        (Qt::UserRole + 5)
#define IUPQML_ROLE_TOGGLE         (Qt::UserRole + 6)
#define IUPQML_ROLE_TOGGLEVISIBLE  (Qt::UserRole + 7)
#define IUPQML_ROLE_KIND           (Qt::UserRole + 8)

struct IupQmlTreeNode
{
  IupQmlTreeNode* parent;
  QList<IupQmlTreeNode*> children;
  QString title;
  int kind;
  bool expanded;
  QPixmap* image;
  QString image_expanded;
  QString font_name;
  QFont font;
  bool has_font;
  QColor fg;
  QColor bg;
  int toggle;
  bool toggle_visible;

  IupQmlTreeNode() : parent(nullptr), kind(ITREE_LEAF), expanded(false), image(nullptr), has_font(false), toggle(0), toggle_visible(true) {}

  ~IupQmlTreeNode()
  {
    for (IupQmlTreeNode* child : children)
      delete child;
  }

  int depth() const
  {
    int d = 0;
    for (IupQmlTreeNode* p = parent; p; p = p->parent)
      d++;
    return d;
  }

  int totalChildCount() const
  {
    int count = 0;
    for (IupQmlTreeNode* child : children)
      count += 1 + child->totalChildCount();
    return count;
  }
};

class IupQmlTreeModel : public QAbstractItemModel
{
public:
  Ihandle* ih;
  QList<IupQmlTreeNode*> roots;

  IupQmlTreeModel(Ihandle* handle) : QAbstractItemModel(nullptr), ih(handle) {}

  ~IupQmlTreeModel() override
  {
    for (IupQmlTreeNode* node : roots)
      delete node;
  }

  IupQmlTreeNode* nodeAt(const QModelIndex& index) const
  {
    if (!index.isValid())
      return nullptr;
    return static_cast<IupQmlTreeNode*>(index.internalPointer());
  }

  QList<IupQmlTreeNode*>& siblings(IupQmlTreeNode* node)
  {
    return node->parent ? node->parent->children : roots;
  }

  int rowOf(IupQmlTreeNode* node)
  {
    return siblings(node).indexOf(node);
  }

  QModelIndex indexOf(IupQmlTreeNode* node) const
  {
    if (!node)
      return {};
    int row = node->parent ? node->parent->children.indexOf(node) : roots.indexOf(node);
    if (row < 0)
      return {};
    return createIndex(row, 0, node);
  }

  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
  {
    if (column != 0 || row < 0)
      return {};
    const QList<IupQmlTreeNode*>& list = parent.isValid() ? nodeAt(parent)->children : roots;
    if (row >= list.size())
      return {};
    return createIndex(row, 0, list.at(row));
  }

  QModelIndex parent(const QModelIndex& index) const override
  {
    IupQmlTreeNode* node = nodeAt(index);
    if (!node || !node->parent)
      return {};
    return indexOf(node->parent);
  }

  int rowCount(const QModelIndex& parent = QModelIndex()) const override
  {
    if (parent.column() > 0)
      return 0;
    if (!parent.isValid())
      return roots.size();
    return nodeAt(parent)->children.size();
  }

  int columnCount(const QModelIndex& parent = QModelIndex()) const override
  {
    (void)parent;
    return 1;
  }

  bool hasChildren(const QModelIndex& parent = QModelIndex()) const override
  {
    if (!parent.isValid())
      return !roots.isEmpty();
    IupQmlTreeNode* node = nodeAt(parent);
    return node && !node->children.isEmpty();
  }

  QHash<int, QByteArray> roleNames() const override
  {
    QHash<int, QByteArray> roles;
    roles[IUPQML_ROLE_TITLE] = "title";
    roles[IUPQML_ROLE_IMAGE] = "image";
    roles[IUPQML_ROLE_FGCOLOR] = "fgcolor";
    roles[IUPQML_ROLE_BGCOLOR] = "bgcolor";
    roles[IUPQML_ROLE_FONT] = "font";
    roles[IUPQML_ROLE_HASFONT] = "hasfont";
    roles[IUPQML_ROLE_TOGGLE] = "toggle";
    roles[IUPQML_ROLE_TOGGLEVISIBLE] = "togglevisible";
    roles[IUPQML_ROLE_KIND] = "kind";
    return roles;
  }

  QPixmap* nodeImage(IupQmlTreeNode* node) const
  {
    if (node->kind == ITREE_BRANCH && node->expanded)
    {
      if (!node->image_expanded.isEmpty())
      {
        QPixmap* pixmap = static_cast<QPixmap*>(iupImageGetImage(node->image_expanded.toUtf8().constData(), ih, 0, nullptr));
        if (pixmap)
          return pixmap;
      }
      return static_cast<QPixmap*>(ih->data->def_image_expanded);
    }

    if (node->image)
      return node->image;

    if (node->kind == ITREE_BRANCH)
      return static_cast<QPixmap*>(ih->data->def_image_collapsed);

    return static_cast<QPixmap*>(ih->data->def_image_leaf);
  }

  QVariant data(const QModelIndex& index, int role) const override
  {
    IupQmlTreeNode* node = nodeAt(index);
    if (!node)
      return {};

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return node->title;
    case IUPQML_ROLE_IMAGE:
      {
        QPixmap* pixmap = nodeImage(node);
        return pixmap ? iupqmlImageUrl(pixmap) : QString();
      }
    case IUPQML_ROLE_FGCOLOR:
      return node->fg.isValid() ? QVariant(node->fg) : QVariant();
    case IUPQML_ROLE_BGCOLOR:
      return node->bg.isValid() ? QVariant(node->bg) : QVariant();
    case IUPQML_ROLE_FONT:
      return QVariant::fromValue(node->font);
    case IUPQML_ROLE_HASFONT:
      return node->has_font;
    case IUPQML_ROLE_TOGGLE:
      return node->toggle;
    case IUPQML_ROLE_TOGGLEVISIBLE:
      return node->toggle_visible;
    case IUPQML_ROLE_KIND:
      return node->kind;
    default:
      return {};
    }
  }

  bool setData(const QModelIndex& index, const QVariant& value, int role) override
  {
    IupQmlTreeNode* node = nodeAt(index);
    if (!node || (role != Qt::EditRole && role != Qt::DisplayRole))
      return false;
    node->title = value.toString();
    emit dataChanged(index, index);
    return true;
  }

  Qt::ItemFlags flags(const QModelIndex& index) const override
  {
    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (index.isValid() && ih->data->show_rename)
      f |= Qt::ItemIsEditable;
    return f;
  }

  void refresh(IupQmlTreeNode* node)
  {
    QModelIndex index = indexOf(node);
    if (index.isValid())
      emit dataChanged(index, index);
  }

  void insertNode(IupQmlTreeNode* parent, int pos, IupQmlTreeNode* node)
  {
    QList<IupQmlTreeNode*>& list = parent ? parent->children : roots;
    if (pos < 0 || pos > list.size())
      pos = list.size();
    QList<IupQmlTreeNode*> children = node->children;
    node->children.clear();
    beginInsertRows(indexOf(parent), pos, pos);
    node->parent = parent;
    list.insert(pos, node);
    endInsertRows();
    for (int i = 0; i < children.size(); i++)
      insertNode(node, i, children[i]);
  }

  void takeNode(IupQmlTreeNode* node)
  {
    QList<IupQmlTreeNode*>& list = siblings(node);
    int pos = list.indexOf(node);
    if (pos < 0)
      return;
    beginRemoveRows(indexOf(node->parent), pos, pos);
    list.removeAt(pos);
    node->parent = nullptr;
    endRemoveRows();
  }

  void reset()
  {
    beginResetModel();
    endResetModel();
  }

  void clear()
  {
    beginResetModel();
    for (IupQmlTreeNode* node : roots)
      delete node;
    roots.clear();
    endResetModel();
  }
};

struct IupQmlTreeData
{
  IupQmlTreeModel* model;
  QItemSelectionModel* selection;
  QQuickItem* view;
  QQuickItem* wrapper;
  QQuickItem* back;
  IupQmlTreeNode* mark_start;
  QPixmap* themed_leaf;
  QPixmap* themed_collapsed;
  QPixmap* themed_expanded;
  int drag_id;
  int rename_gen;
  int press_row;
  bool press_was_current;
  bool press_collapse;
  bool rename_done;
};

static IupQmlTreeData* qmlTreeGetData(Ihandle* ih)
{
  return reinterpret_cast<IupQmlTreeData*>(iupAttribGet(ih, "_IUPQML_TREE"));
}

/****************************************************************************
 * View Helpers
 ****************************************************************************/

static IupQmlTreeNode* qmlTreeFindNode(Ihandle* ih, int id)
{
  return reinterpret_cast<IupQmlTreeNode*>(iupTreeGetNode(ih, id));
}

static int qmlTreeFindNodeId(Ihandle* ih, IupQmlTreeNode* node)
{
  if (!node)
    return -1;
  return iupTreeFindNodeId(ih, reinterpret_cast<InodeHandle*>(node));
}

static int qmlTreeRowOfNode(IupQmlTreeData* data, IupQmlTreeNode* node)
{
  if (!node)
    return -1;
  QModelIndex index = data->model->indexOf(node);
  if (!index.isValid())
    return -1;
  int row = -1;
  QMetaObject::invokeMethod(data->view, "rowAtIndex", Qt::DirectConnection, Q_RETURN_ARG(int, row), Q_ARG(QModelIndex, index));
  return row;
}

static IupQmlTreeNode* qmlTreeNodeAtRow(IupQmlTreeData* data, int row)
{
  if (row < 0)
    return nullptr;
  QModelIndex index;
  QPoint cell(0, row);
  QMetaObject::invokeMethod(data->view, "modelIndex", Qt::DirectConnection, Q_RETURN_ARG(QModelIndex, index), Q_ARG(QPoint, cell));
  return data->model->nodeAt(index);
}

static int qmlTreeViewRows(IupQmlTreeData* data)
{
  return data->view->property("rows").toInt();
}

static bool qmlTreeRowExpanded(IupQmlTreeData* data, int row)
{
  bool expanded = false;
  QMetaObject::invokeMethod(data->view, "isExpanded", Qt::DirectConnection, Q_RETURN_ARG(bool, expanded), Q_ARG(int, row));
  return expanded;
}

static void qmlTreeExpandRow(IupQmlTreeData* data, int row, bool expand)
{
  if (row < 0)
    return;
  QMetaObject::invokeMethod(data->view, expand ? "expand" : "collapse", Qt::DirectConnection, Q_ARG(int, row));
}

static void qmlTreeSyncExpanded(IupQmlTreeData* data, IupQmlTreeNode* node)
{
  if (node->kind == ITREE_BRANCH && node->expanded)
  {
    int row = qmlTreeRowOfNode(data, node);
    if (row >= 0 && !qmlTreeRowExpanded(data, row))
    {
      iupAttribSet(data->model->ih, "_IUPQML_TREE_SYNC", "1");
      qmlTreeExpandRow(data, row, true);
      iupAttribSet(data->model->ih, "_IUPQML_TREE_SYNC", nullptr);
    }
  }

  for (IupQmlTreeNode* child : node->children)
    qmlTreeSyncExpanded(data, child);
}

static void qmlTreeExpandAncestors(IupQmlTreeData* data, IupQmlTreeNode* node)
{
  QList<IupQmlTreeNode*> chain;
  for (IupQmlTreeNode* p = node->parent; p; p = p->parent)
    chain.prepend(p);

  for (IupQmlTreeNode* p : chain)
  {
    int row = qmlTreeRowOfNode(data, p);
    if (row >= 0 && !qmlTreeRowExpanded(data, row))
    {
      p->expanded = true;
      iupAttribSet(data->model->ih, "_IUPQML_TREE_SYNC", "1");
      qmlTreeExpandRow(data, row, true);
      iupAttribSet(data->model->ih, "_IUPQML_TREE_SYNC", nullptr);
    }
  }
}

static void qmlTreeScrollTo(IupQmlTreeData* data, IupQmlTreeNode* node, int top)
{
  qmlTreeExpandAncestors(data, node);
  int row = qmlTreeRowOfNode(data, node);
  if (row >= 0)
    iupqmlCallMethod(data->view, "iupScroll", QVariant(row), QVariant(top ? true : false));
}

static IupQmlTreeNode* qmlTreeGetCurrent(IupQmlTreeData* data)
{
  return data->model->nodeAt(data->selection->currentIndex());
}

static void qmlTreeResync(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  QList<IupQmlTreeNode*> selected;
  for (const QModelIndex& index : data->selection->selectedRows())
    selected.append(data->model->nodeAt(index));
  IupQmlTreeNode* current = qmlTreeGetCurrent(data);
  qreal content_y = data->view->property("contentY").toDouble();

  char* ignore = iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB");
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  iupAttribSet(ih, "_IUPQML_TREE_IN_SELECT", "1");

  data->model->reset();
  for (IupQmlTreeNode* root : data->model->roots)
    qmlTreeSyncExpanded(data, root);
  QMetaObject::invokeMethod(data->view, "forceLayout", Qt::DirectConnection);

  for (IupQmlTreeNode* node : selected)
  {
    QModelIndex index = data->model->indexOf(node);
    if (index.isValid())
      data->selection->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
  if (current)
    data->selection->setCurrentIndex(data->model->indexOf(current), QItemSelectionModel::NoUpdate);
  data->view->setProperty("contentY", content_y);

  iupAttribSet(ih, "_IUPQML_TREE_IN_SELECT", nullptr);
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", ignore);
}

static void qmlTreeSetCurrent(IupQmlTreeData* data, IupQmlTreeNode* node, bool select)
{
  QModelIndex index = data->model->indexOf(node);
  if (!index.isValid())
    return;

  if (select)
    data->selection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  else
    data->selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
}

static void qmlTreeSelectNode(IupQmlTreeData* data, IupQmlTreeNode* node, bool select)
{
  QModelIndex index = data->model->indexOf(node);
  if (!index.isValid())
    return;
  data->selection->select(index, (select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) | QItemSelectionModel::Rows);
}

static bool qmlTreeIsSelected(IupQmlTreeData* data, IupQmlTreeNode* node)
{
  QModelIndex index = data->model->indexOf(node);
  return index.isValid() && data->selection->isSelected(index);
}

/****************************************************************************
 * Node Cache
 ****************************************************************************/

static void qmlTreeRebuildNodeCacheRec(Ihandle* ih, IupQmlTreeNode* node, int* id)
{
  for (IupQmlTreeNode* child : node->children)
  {
    (*id)++;
    ih->data->node_cache[*id].node_handle = reinterpret_cast<InodeHandle*>(child);
    qmlTreeRebuildNodeCacheRec(ih, child, id);
  }
}

static void qmlTreeRebuildEntireCache(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  int id = -1;
  for (IupQmlTreeNode* node : data->model->roots)
  {
    id++;
    ih->data->node_cache[id].node_handle = reinterpret_cast<InodeHandle*>(node);
    qmlTreeRebuildNodeCacheRec(ih, node, &id);
  }
}

static IupQmlTreeNode* qmlTreeNextNode(IupQmlTreeData* data, IupQmlTreeNode* node)
{
  if (!node->children.isEmpty())
    return node->children.first();

  while (node)
  {
    QList<IupQmlTreeNode*>& list = data->model->siblings(node);
    int index = list.indexOf(node);
    if (index < list.size() - 1)
      return list.at(index + 1);
    node = node->parent;
  }

  return nullptr;
}

static IupQmlTreeNode* qmlTreePreviousNode(IupQmlTreeData* data, IupQmlTreeNode* node)
{
  QList<IupQmlTreeNode*>& list = data->model->siblings(node);
  int index = list.indexOf(node);

  if (index > 0)
  {
    IupQmlTreeNode* prev = list.at(index - 1);
    while (!prev->children.isEmpty())
      prev = prev->children.last();
    return prev;
  }

  return node->parent;
}

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  (void)ih;
  auto* node = reinterpret_cast<IupQmlTreeNode*>(node_handle);
  if (!node)
    return 0;
  return node->totalChildCount();
}

extern "C" IUP_SDK_API void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    data->view->setProperty("selectionMode", IUPQML_TREE_SELECT_SINGLE);
  else
    data->view->setProperty("selectionMode", IUPQML_TREE_SELECT_EXTENDED);

  bool rubber = ih->data->mark_mode == ITREE_MARK_MULTIPLE && iupAttribGetBoolean(ih, "RUBBERBAND") && !ih->data->show_dragdrop && !iupAttribGetBoolean(ih, "DRAGSOURCE");
  data->view->setProperty("iupRubber", rubber);
}

static void qmlTreeInitNode(Ihandle* ih, IupQmlTreeNode* node, int kind, const char* title)
{
  if (title)
    node->title = QString::fromUtf8(title);
  node->kind = kind;
  node->toggle = 0;
  node->toggle_visible = ih->data->show_toggle ? true : false;
}

extern "C" IUP_SDK_API void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  IupQmlTreeNode* ref_node = nullptr;
  int kindPrev = -1;

  if (id == IUP_INVALID_ID && ih->data->node_count != 0)
    id = iupTreeFindNodeId(ih, iupdrvTreeGetFocusNode(ih));

  if (id >= 0 && id < ih->data->node_count)
  {
    ref_node = qmlTreeFindNode(ih, id);
    if (ref_node)
      kindPrev = ref_node->kind;
  }

  auto* new_node = new IupQmlTreeNode();
  qmlTreeInitNode(ih, new_node, kind, title);

  if (ref_node)
  {
    if (kindPrev == ITREE_BRANCH && add)
      data->model->insertNode(ref_node, 0, new_node);
    else
      data->model->insertNode(ref_node->parent, data->model->rowOf(ref_node) + 1, new_node);

    iupTreeAddToCache(ih, add, kindPrev, reinterpret_cast<InodeHandle*>(ref_node), reinterpret_cast<InodeHandle*>(new_node));
  }
  else
  {
    if (id == -1)
      data->model->insertNode(nullptr, 0, new_node);
    else
      data->model->insertNode(nullptr, -1, new_node);

    iupTreeAddToCache(ih, 0, 0, nullptr, reinterpret_cast<InodeHandle*>(new_node));
  }

  IupQmlTreeNode* parent = new_node->parent;
  if (parent && parent->children.size() == 1)
  {
    parent->expanded = ih->data->add_expanded ? true : false;
    int row = qmlTreeRowOfNode(data, parent);
    if (row >= 0 && qmlTreeRowExpanded(data, row) != parent->expanded)
    {
      iupAttribSet(ih, "_IUPQML_TREE_SYNC", "1");
      qmlTreeExpandRow(data, row, parent->expanded);
      iupAttribSet(ih, "_IUPQML_TREE_SYNC", nullptr);
    }
    data->model->refresh(parent);
  }

  if (ih->data->node_count == 1)
  {
    data->mark_start = new_node;
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    iupAttribSet(ih, "_IUPQML_TREE_IN_SELECT", "1");
    qmlTreeSetCurrent(data, new_node, ih->data->mark_mode == ITREE_MARK_SINGLE);
    iupAttribSet(ih, "_IUPQML_TREE_IN_SELECT", nullptr);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);
  }

  qmlTreeRebuildEntireCache(ih);
}

/****************************************************************************
 * Copy/Move Node Functions
 ****************************************************************************/

static IupQmlTreeNode* qmlTreeCloneNode(Ihandle* dst_ih, IupQmlTreeNode* src)
{
  auto* node = new IupQmlTreeNode();
  node->title = src->title;
  node->kind = src->kind;
  node->expanded = src->expanded;
  node->image = src->image;
  node->image_expanded = src->image_expanded;
  node->font = src->font;
  node->has_font = src->has_font;
  node->fg = src->fg;
  node->bg = src->bg;
  node->toggle = src->toggle;
  node->toggle_visible = dst_ih->data->show_toggle ? src->toggle_visible : true;

  for (IupQmlTreeNode* child : src->children)
  {
    IupQmlTreeNode* new_child = qmlTreeCloneNode(dst_ih, child);
    new_child->parent = node;
    node->children.append(new_child);
  }

  return node;
}

static bool qmlTreeIsAncestorOf(IupQmlTreeNode* ancestor, IupQmlTreeNode* node)
{
  while (node)
  {
    if (node == ancestor)
      return true;
    node = node->parent;
  }
  return false;
}

static void qmlTreeGetDropTarget(IupQmlTreeData* data, IupQmlTreeNode* dst, IupQmlTreeNode** dst_parent, int* dst_index, int* id_new, int id_dst)
{
  (void)data;
  if (dst->kind == ITREE_BRANCH && dst->expanded)
  {
    *dst_parent = dst;
    *dst_index = 0;
    *id_new = id_dst + 1;
  }
  else
  {
    *dst_parent = dst->parent;
    *dst_index = data->model->rowOf(dst) + 1;
    *id_new = id_dst + 1;
    if (dst->kind == ITREE_BRANCH)
      *id_new += dst->totalChildCount();
  }
}

static int qmlTreeSetCopyNodeAttrib(Ihandle* ih, int id_src, const char* name_dst)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* src = qmlTreeFindNode(ih, id_src);
  if (!data || !src)
    return 0;

  int id_dst;
  if (iupStrToInt(name_dst, &id_dst))
  {
    IupQmlTreeNode* dst = qmlTreeFindNode(ih, id_dst);
    if (!dst || qmlTreeIsAncestorOf(src, dst))
      return 0;

    IupQmlTreeNode* dst_parent;
    int dst_index, id_new;
    qmlTreeGetDropTarget(data, dst, &dst_parent, &dst_index, &id_new, id_dst);

    IupQmlTreeNode* new_node = qmlTreeCloneNode(ih, src);
    int count = 1 + new_node->totalChildCount();

    data->model->insertNode(dst_parent, dst_index, new_node);
    ih->data->node_count += count;

    iupTreeCopyMoveCache(ih, id_src, id_new, count, 1);
    qmlTreeRebuildEntireCache(ih);
    qmlTreeResync(ih);
  }

  return 1;
}

static int qmlTreeSetMoveNodeAttrib(Ihandle* ih, int id_src, const char* name_dst)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* src = qmlTreeFindNode(ih, id_src);
  if (!data || !src)
    return 0;

  int id_dst;
  if (iupStrToInt(name_dst, &id_dst))
  {
    IupQmlTreeNode* dst = qmlTreeFindNode(ih, id_dst);
    if (!dst || src == dst)
      return 0;

    for (IupQmlTreeNode* p = dst->parent; p; p = p->parent)
    {
      if (p == src)
        return 0;
    }

    int count = 1 + src->totalChildCount();

    IupQmlTreeNode* dst_parent;
    int dst_index, id_new;
    qmlTreeGetDropTarget(data, dst, &dst_parent, &dst_index, &id_new, id_dst);
    if (id_new == id_src)
      return 0;

    ih->data->node_count += count;
    iupTreeCopyMoveCache(ih, id_src, id_new, count, 0);
    ih->data->node_count -= count;

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    IupQmlTreeNode* src_parent = src->parent;
    int src_index = data->model->rowOf(src);
    data->model->takeNode(src);
    if (dst_parent == src_parent && dst_index > src_index)
      dst_index--;
    if (!dst_parent && dst_index > data->model->roots.size())
      dst_index = data->model->roots.size();
    data->model->insertNode(dst_parent, dst_index, src);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);

    qmlTreeRebuildEntireCache(ih);
    qmlTreeResync(ih);
  }

  return 1;
}

/****************************************************************************
 * Attribute Setters/Getters
 ****************************************************************************/

static void qmlTreeRefreshAll(IupQmlTreeData* data)
{
  if (data->model->rowCount() > 0)
  {
    emit data->model->layoutAboutToBeChanged();
    emit data->model->layoutChanged();
  }
}

static int qmlTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, nullptr);
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (data)
    qmlTreeRefreshAll(data);
  return 1;
}

static int qmlTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, nullptr);
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (data)
    qmlTreeRefreshAll(data);
  return 1;
}

static int qmlTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, nullptr);
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (data)
    qmlTreeRefreshAll(data);
  return 1;
}

static int qmlTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return 0;

  if (value)
    node->image = static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr));
  else
    node->image = nullptr;

  data->model->refresh(node);
  return 1;
}

static int qmlTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return 0;

  node->image_expanded = QString::fromUtf8(value ? value : "");
  data->model->refresh(node);
  return 1;
}

static int qmlTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  int indent = 0;
  if (!data || !iupStrToInt(value, &indent))
    return 0;

  data->view->setProperty("iupIndent", indent);
  return 1;
}

static char* qmlTreeGetIndentationAttrib(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return nullptr;

  return iupStrReturnInt(data->view->property("iupIndent").toInt());
}

static void qmlTreeUpdateRowHeight(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  int char_height;
  iupdrvFontGetCharSize(ih, nullptr, &char_height);
  int row = char_height + 2;
  if (row < 18)
    row = 18;
  row += 2 * ih->data->spacing;
  if (data->view->property("iupRowHeight").toInt() != row)
  {
    data->view->setProperty("iupRowHeight", row);
    QMetaObject::invokeMethod(data->view, "forceLayout", Qt::DirectConnection);
  }

  QFont* font = iupqmlGetIhFont(ih);
  if (font)
    data->view->setProperty("iupFont", QVariant::fromValue(*font));
}

static int qmlTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);
  if (ih->data->spacing < 0)
    ih->data->spacing = 0;

  if (!ih->handle)
    return 1;

  qmlTreeUpdateRowHeight(ih);
  return 0;
}

static int qmlTreeSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
    qmlTreeUpdateRowHeight(ih);
  return 1;
}

static int qmlTreeSetHlColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (data)
    iupqmlSetPaletteColor(data->view, "highlight", QColor(r, g, b));

  return 1;
}

static int qmlTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(data->view, "base", QColor(r, g, b));
  iupqmlSetPaletteColor(data->view, "window", QColor(r, g, b));
  if (data->back)
    data->back->setProperty("color", QColor(r, g, b));

  return 1;
}

static char* qmlTreeGetBgColorAttrib(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || !data->back)
    return nullptr;

  auto color = data->back->property("color").value<QColor>();
  return iupStrReturnStrf("%d %d %d", color.red(), color.green(), color.blue());
}

static int qmlTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(data->view, "text", QColor(r, g, b));
  iupqmlSetPaletteColor(data->view, "windowText", QColor(r, g, b));

  return 1;
}

static int qmlTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return 0;

  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);

  if (!node && id == 0 && ih->data->node_count == 0)
  {
    node = new IupQmlTreeNode();
    qmlTreeInitNode(ih, node, ITREE_BRANCH, nullptr);
    data->model->insertNode(nullptr, -1, node);

    ih->data->node_count = 1;
    ih->data->node_cache[0].node_handle = reinterpret_cast<InodeHandle*>(node);
    data->mark_start = node;
  }

  if (!node)
    return 0;

  if (value)
    node->title = QString::fromUtf8(value);
  else
    node->title.clear();

  data->model->refresh(node);
  return 1;
}

static char* qmlTreeGetTitleAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return nullptr;

  return iupStrReturnStr(node->title.toUtf8().constData());
}

static int qmlTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return 0;

  QFont* font = value ? iupqmlGetQFont(value) : nullptr;
  if (font)
  {
    node->font = *font;
    node->font_name = QString::fromUtf8(value);
    node->has_font = true;
  }
  else
  {
    node->font_name.clear();
    node->has_font = false;
  }

  data->model->refresh(node);
  return 1;
}

static char* qmlTreeGetTitleFontAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node || !node->has_font)
    return nullptr;

  return iupStrReturnStr(node->font_name.toUtf8().constData());
}

static int qmlTreeSetTitleFgColorAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    node->fg = QColor(r, g, b);
  else
    node->fg = QColor();

  data->model->refresh(node);
  return 1;
}

static char* qmlTreeGetColorAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node || !node->fg.isValid())
    return nullptr;

  return iupStrReturnRGB(node->fg.red(), node->fg.green(), node->fg.blue());
}

static int qmlTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  return qmlTreeSetTitleFgColorAttrib(ih, id, value);
}

static int qmlTreeSetTitleBgColorAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    node->bg = QColor(r, g, b);
  else
    node->bg = QColor();

  data->model->refresh(node);
  return 1;
}

static int qmlTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node || node->kind != ITREE_BRANCH)
    return 0;

  bool expand = iupStrEqualNoCase(value, "EXPANDED");
  node->expanded = expand;

  int row = qmlTreeRowOfNode(data, node);
  if (row >= 0 && qmlTreeRowExpanded(data, row) != expand)
  {
    iupAttribSet(ih, "_IUPQML_TREE_SYNC", "1");
    qmlTreeExpandRow(data, row, expand);
    iupAttribSet(ih, "_IUPQML_TREE_SYNC", nullptr);
  }

  data->model->refresh(node);
  return 1;
}

static char* qmlTreeGetStateAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node || node->kind != ITREE_BRANCH)
    return nullptr;

  if (node->expanded)
    return const_cast<char*>("EXPANDED");
  else
    return const_cast<char*>("COLLAPSED");
}

static char* qmlTreeGetDepthAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return nullptr;

  return iupStrReturnInt(node->depth());
}

static char* qmlTreeGetKindAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return nullptr;

  if (node->kind == ITREE_BRANCH)
    return const_cast<char*>("BRANCH");
  else
    return const_cast<char*>("LEAF");
}

static char* qmlTreeGetParentAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node || !node->parent)
    return nullptr;

  return iupStrReturnInt(qmlTreeFindNodeId(ih, node->parent));
}

static char* qmlTreeGetNextAttrib(Ihandle* ih, int id)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return nullptr;

  IupQmlTreeNode* next = qmlTreeNextNode(data, node);
  if (!next)
    return nullptr;

  return iupStrReturnInt(qmlTreeFindNodeId(ih, next));
}

static char* qmlTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return nullptr;

  IupQmlTreeNode* prev = qmlTreePreviousNode(data, node);
  if (!prev)
    return nullptr;

  return iupStrReturnInt(qmlTreeFindNodeId(ih, prev));
}

static char* qmlTreeGetFirstAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node || node->children.isEmpty())
    return nullptr;

  return iupStrReturnInt(qmlTreeFindNodeId(ih, node->children.first()));
}

static char* qmlTreeGetLastAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return nullptr;

  IupQmlTreeNode* last = node;
  while (!last->children.isEmpty())
    last = last->children.last();

  if (last == node)
    return nullptr;

  return iupStrReturnInt(qmlTreeFindNodeId(ih, last));
}

static char* qmlTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return nullptr;

  return iupStrReturnInt(node->children.size());
}

static char* qmlTreeGetRootCountAttrib(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return nullptr;

  return iupStrReturnInt(data->model->roots.size());
}

static char* qmlTreeGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->node_count);
}

static int qmlTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return 0;

  IupQmlTreeNode* cur = qmlTreeGetCurrent(data);
  if (!cur && !data->model->roots.isEmpty())
    cur = data->model->roots.first();

  IupQmlTreeNode* node = nullptr;
  int rows = qmlTreeViewRows(data);
  int cur_row = qmlTreeRowOfNode(data, cur);

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
    node = data->model->roots.isEmpty() ? nullptr : data->model->roots.first();
  else if (iupStrEqualNoCase(value, "LAST"))
    node = qmlTreeNodeAtRow(data, rows - 1);
  else if (iupStrEqualNoCase(value, "NEXT"))
    node = cur_row >= 0 ? qmlTreeNodeAtRow(data, cur_row + 1) : nullptr;
  else if (iupStrEqualNoCase(value, "PREVIOUS"))
    node = cur_row > 0 ? qmlTreeNodeAtRow(data, cur_row - 1) : nullptr;
  else if (iupStrEqualNoCase(value, "PGDN"))
  {
    int row = cur_row + 10;
    if (row > rows - 1)
      row = rows - 1;
    node = qmlTreeNodeAtRow(data, row);
  }
  else if (iupStrEqualNoCase(value, "PGUP"))
  {
    int row = cur_row - 10;
    if (row < 0)
      row = 0;
    node = qmlTreeNodeAtRow(data, row);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    data->selection->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
    return 0;
  }
  else
  {
    int id = 0;
    iupStrToInt(value, &id);
    node = qmlTreeFindNode(ih, id);
  }

  if (node)
  {
    qmlTreeExpandAncestors(data, node);

    if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    {
      data->selection->clearSelection();
      qmlTreeSetCurrent(data, node, true);
    }
    else
      qmlTreeSetCurrent(data, node, false);

    qmlTreeScrollTo(data, node, 0);
  }

  return 0;
}

static char* qmlTreeGetValueAttrib(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return nullptr;

  IupQmlTreeNode* node = qmlTreeGetCurrent(data);
  if (!node)
    return iupStrReturnInt(ih->data->node_count ? 0 : -1);

  return iupStrReturnInt(qmlTreeFindNodeId(ih, node));
}

static int qmlTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return 0;

  char* ignore = iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB");
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  if (ih->data->mark_mode == ITREE_MARK_SINGLE && iupStrBoolean(value))
  {
    QModelIndex index = data->model->indexOf(node);
    if (index.isValid())
      data->selection->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }
  else
    qmlTreeSelectNode(data, node, iupStrBoolean(value));
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", ignore);
  return 1;
}

static char* qmlTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!data || !node)
    return nullptr;

  return iupStrReturnBoolean(qmlTreeIsSelected(data, node));
}

static int qmlTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return 0;

  if (iupStrEqualNoCase(value, "BLOCK"))
  {
    IupQmlTreeNode* current = qmlTreeGetCurrent(data);
    if (data->mark_start && current)
    {
      int id_start = qmlTreeFindNodeId(ih, data->mark_start);
      int id_end = qmlTreeFindNodeId(ih, current);

      if (id_start > id_end)
      {
        int tmp = id_start;
        id_start = id_end;
        id_end = tmp;
      }

      for (int i = id_start; i <= id_end; i++)
      {
        IupQmlTreeNode* node = qmlTreeFindNode(ih, i);
        if (node)
          qmlTreeSelectNode(data, node, true);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "CLEARALL"))
    data->selection->clearSelection();
  else if (iupStrEqualNoCase(value, "MARKALL"))
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      IupQmlTreeNode* node = qmlTreeFindNode(ih, i);
      if (node)
        qmlTreeSelectNode(data, node, true);
    }
  }
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      IupQmlTreeNode* node = qmlTreeFindNode(ih, i);
      if (node)
        qmlTreeSelectNode(data, node, !qmlTreeIsSelected(data, node));
    }
  }
  else if (iupStrEqualPartial(value, "INVERT"))
  {
    int id = IUP_INVALID_ID;
    iupStrToInt(&value[strlen("INVERT")], &id);
    IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
    if (!node)
      node = qmlTreeGetCurrent(data);
    if (node)
      qmlTreeSelectNode(data, node, !qmlTreeIsSelected(data, node));
  }
  else
  {
    int id1, id2;
    if (iupStrToIntInt(value, &id1, &id2, '-') == 2)
    {
      if (id1 > id2) { int tmp = id1; id1 = id2; id2 = tmp; }
      for (int i = id1; i <= id2; i++)
      {
        IupQmlTreeNode* node = qmlTreeFindNode(ih, i);
        if (node)
          qmlTreeSelectNode(data, node, true);
      }
    }
  }

  return 1;
}

static int qmlTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  int id = 0;
  iupStrToInt(value, &id);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);

  if (data && node)
    data->mark_start = node;

  return 1;
}

static char* qmlTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return nullptr;

  char* str = iupStrGetMemory(ih->data->node_count + 1);
  for (int i = 0; i < ih->data->node_count; i++)
  {
    IupQmlTreeNode* node = qmlTreeFindNode(ih, i);
    str[i] = (node && qmlTreeIsSelected(data, node)) ? '+' : '-';
  }
  str[ih->data->node_count] = 0;
  return str;
}

static int qmlTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return 0;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE || !value)
    return 0;

  int count = static_cast<int>(strlen(value));
  if (count > ih->data->node_count)
    count = ih->data->node_count;

  char* ignore = iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB");
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  for (int i = 0; i < count; i++)
  {
    IupQmlTreeNode* node = qmlTreeFindNode(ih, i);
    if (node)
      qmlTreeSelectNode(data, node, value[i] == '+');
  }
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", ignore);

  return 0;
}

static int qmlTreeSetToggleValueAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!ih->data->show_toggle || !data)
    return 0;

  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return 0;

  if (iupStrEqualNoCase(value, "ON"))
    node->toggle = 1;
  else if (iupStrEqualNoCase(value, "OFF"))
    node->toggle = 0;
  else if ((iupStrEqualNoCase(value, "NOTDEF") || (iupAttribGetBoolean(ih, "EMPTYAS3STATE") && !value))
           && ih->data->show_toggle == 2)
    node->toggle = -1;

  data->model->refresh(node);
  return 1;
}

static char* qmlTreeGetToggleValueAttrib(Ihandle* ih, int id)
{
  if (!ih->data->show_toggle)
    return nullptr;

  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return nullptr;

  if (node->toggle == 1)
    return const_cast<char*>("ON");
  else if (node->toggle == 0)
    return const_cast<char*>("OFF");
  else if (ih->data->show_toggle == 2)
    return const_cast<char*>("NOTDEF");

  return nullptr;
}

static int qmlTreeSetToggleVisibleAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!ih->data->show_toggle || !data)
    return 0;

  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return 0;

  node->toggle_visible = iupStrBoolean(value) ? true : false;
  data->model->refresh(node);
  return 1;
}

static char* qmlTreeGetToggleVisibleAttrib(Ihandle* ih, int id)
{
  if (!ih->data->show_toggle)
    return nullptr;

  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
  if (!node)
    return nullptr;

  return iupStrReturnBoolean(node->toggle_visible);
}

static char* qmlTreeGetScrollVisibleAttrib(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return nullptr;

  QVariant ret;
  QMetaObject::invokeMethod(data->view, "iupScrollVisible", Qt::DirectConnection, Q_RETURN_ARG(QVariant, ret));
  int flags = ret.toInt();

  if ((flags & 1) && (flags & 2))
    return const_cast<char*>("YES");
  else if (flags & 1)
    return const_cast<char*>("HORIZONTAL");
  else if (flags & 2)
    return const_cast<char*>("VERTICAL");
  else
    return const_cast<char*>("NO");
}

static int qmlTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  ih->data->show_rename = iupStrBoolean(value);
  return 0;
}

static void qmlTreeStartRename(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || !ih->data->show_rename)
    return;

  IupQmlTreeNode* node = qmlTreeGetCurrent(data);
  if (!node)
    return;

  int id = qmlTreeFindNodeId(ih, node);
  IFni cbShowRename = reinterpret_cast<IFni>(IupGetCallback(ih, "SHOWRENAME_CB"));
  if (cbShowRename && cbShowRename(ih, id) == IUP_IGNORE)
    return;

  QModelIndex index = data->model->indexOf(node);
  QMetaObject::invokeMethod(data->view, "edit", Qt::DirectConnection, Q_ARG(QModelIndex, index));
}

static int qmlTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  qmlTreeStartRename(ih);
  return 0;
}

static int qmlTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  int id = 0;
  iupStrToInt(value, &id);
  IupQmlTreeNode* node = qmlTreeFindNode(ih, id);

  if (data && node)
    qmlTreeScrollTo(data, node, 1);

  return 1;
}

static void qmlTreeCallNodeRemoved(Ihandle* ih, IFns cb, IupQmlTreeNode* node)
{
  for (IupQmlTreeNode* child : node->children)
    qmlTreeCallNodeRemoved(ih, cb, child);

  int id = qmlTreeFindNodeId(ih, node);
  if (id != -1)
    cb(ih, static_cast<char*>(ih->data->node_cache[id].userdata));
}

static void qmlTreeRemoveNode(Ihandle* ih, IupQmlTreeData* data, IupQmlTreeNode* node)
{
  int id = qmlTreeFindNodeId(ih, node);
  if (id == -1)
    return;

  IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "NODEREMOVED_CB"));
  if (cb)
    qmlTreeCallNodeRemoved(ih, cb, node);

  int count = 1 + node->totalChildCount();

  if (data->mark_start && qmlTreeIsAncestorOf(node, data->mark_start))
    data->mark_start = nullptr;

  data->model->takeNode(node);
  delete node;
  ih->data->node_count -= count;
  iupTreeDelFromCache(ih, id, count);
}

static int qmlTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return 0;

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  if (iupStrEqualNoCase(value, "ALL"))
  {
    data->mark_start = nullptr;

    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "NODEREMOVED_CB"));
    if (cb)
    {
      for (int i = 0; i < ih->data->node_count; i++)
        cb(ih, static_cast<char*>(ih->data->node_cache[i].userdata));
    }

    {
      int old_count = ih->data->node_count;
      data->model->clear();
      ih->data->node_count = 0;
      iupTreeDelFromCache(ih, 0, old_count);
    }
  }
  else if (iupStrEqualNoCase(value, "SELECTED"))
  {
    IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
    if (node)
    {
      qmlTreeRemoveNode(ih, data, node);
      qmlTreeRebuildEntireCache(ih);
    }
  }
  else if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
    if (node)
    {
      while (!node->children.isEmpty())
        qmlTreeRemoveNode(ih, data, node->children.first());
      qmlTreeRebuildEntireCache(ih);
      data->model->refresh(node);
    }
  }
  else if (iupStrEqualNoCase(value, "MARKED"))
  {
    QList<IupQmlTreeNode*> selected;
    for (int i = 0; i < ih->data->node_count; i++)
    {
      IupQmlTreeNode* node = qmlTreeFindNode(ih, i);
      if (node && qmlTreeIsSelected(data, node))
        selected.append(node);
    }

    QList<IupQmlTreeNode*> to_delete;
    for (IupQmlTreeNode* node : selected)
    {
      bool has_selected_ancestor = false;
      for (IupQmlTreeNode* p = node->parent; p; p = p->parent)
      {
        if (selected.contains(p))
        {
          has_selected_ancestor = true;
          break;
        }
      }
      if (!has_selected_ancestor)
        to_delete.append(node);
    }

    for (IupQmlTreeNode* node : to_delete)
      qmlTreeRemoveNode(ih, data, node);

    qmlTreeRebuildEntireCache(ih);
  }
  else
  {
    IupQmlTreeNode* node = qmlTreeFindNode(ih, id);
    if (node)
    {
      qmlTreeRemoveNode(ih, data, node);
      qmlTreeRebuildEntireCache(ih);
    }
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);
  return 1;
}

static void qmlTreeSetExpandedRec(IupQmlTreeNode* node, bool expand)
{
  if (node->kind == ITREE_BRANCH)
    node->expanded = expand;
  for (IupQmlTreeNode* child : node->children)
    qmlTreeSetExpandedRec(child, expand);
}

static int qmlTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return 0;

  bool expand = iupStrBoolean(value);

  for (IupQmlTreeNode* node : data->model->roots)
    qmlTreeSetExpandedRec(node, expand);

  iupAttribSet(ih, "_IUPQML_TREE_SYNC", "1");
  if (expand)
    QMetaObject::invokeMethod(data->view, "expandRecursively", Qt::DirectConnection, Q_ARG(int, -1), Q_ARG(int, -1));
  else
    QMetaObject::invokeMethod(data->view, "collapseRecursively", Qt::DirectConnection, Q_ARG(int, -1));
  iupAttribSet(ih, "_IUPQML_TREE_SYNC", nullptr);

  qmlTreeRefreshAll(data);
  return 0;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qmlTreeCallSelection(Ihandle* ih, const QList<int>& ids, int status)
{
  if (ids.isEmpty())
    return;

  auto cbMulti = reinterpret_cast<IFnIi>(IupGetCallback(ih, status ? "MULTISELECTION_CB" : "MULTIUNSELECTION_CB"));
  auto cbSelect = reinterpret_cast<IFnii>(IupGetCallback(ih, "SELECTION_CB"));

  if (cbMulti && ih->data->mark_mode == ITREE_MARK_MULTIPLE && ids.size() > 1)
  {
    int* id_array = static_cast<int*>(malloc(sizeof(int) * ids.size()));
    for (int i = 0; i < ids.size(); i++)
      id_array[i] = ids[i];
    cbMulti(ih, id_array, ids.size());
    free(id_array);
  }
  else if (cbSelect)
  {
    for (int id : ids)
      cbSelect(ih, id, status);
  }
}

static void qmlTreeSelectionChanged(Ihandle* ih, const QItemSelection& selected, const QItemSelection& deselected)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB"))
    return;

  QList<int> ids;
  for (const QModelIndex& index : deselected.indexes())
  {
    int id = qmlTreeFindNodeId(ih, data->model->nodeAt(index));
    if (id >= 0)
      ids.append(id);
  }
  std::sort(ids.begin(), ids.end());
  qmlTreeCallSelection(ih, ids, 0);

  ids.clear();
  for (const QModelIndex& index : selected.indexes())
  {
    int id = qmlTreeFindNodeId(ih, data->model->nodeAt(index));
    if (id >= 0)
      ids.append(id);
  }
  std::sort(ids.begin(), ids.end());
  qmlTreeCallSelection(ih, ids, 1);
}

static void qmlTreeCurrentChanged(Ihandle* ih, const QModelIndex& current)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  IupQmlTreeNode* node = data->model->nodeAt(current);
  if (!node)
    return;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    data->mark_start = node;

  if (iupAttribGet(ih, "_IUPQML_TREE_IN_SELECT"))
    return;

  Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();
  if (mods & (Qt::ControlModifier | Qt::ShiftModifier))
    return;

  if (!data->selection->isSelected(current) || data->selection->selectedRows().size() != 1)
  {
    iupAttribSet(ih, "_IUPQML_TREE_IN_SELECT", "1");
    data->selection->select(current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    iupAttribSet(ih, "_IUPQML_TREE_IN_SELECT", nullptr);
  }
}

static void qmlTreeRowExpandedCb(Ihandle* ih, int row, int depth)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  IupQmlTreeNode* node = qmlTreeNodeAtRow(data, row);
  if (!node || node->kind != ITREE_BRANCH)
    return;

  if (depth < 0)
    qmlTreeSetExpandedRec(node, true);
  else
    node->expanded = true;

  data->model->refresh(node);

  if (iupAttribGet(ih, "_IUPQML_TREE_SYNC"))
    return;

  IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "BRANCHOPEN_CB"));
  if (cb && cb(ih, qmlTreeFindNodeId(ih, node)) == IUP_IGNORE)
  {
    node->expanded = false;
    iupAttribSet(ih, "_IUPQML_TREE_SYNC", "1");
    qmlTreeExpandRow(data, row, false);
    iupAttribSet(ih, "_IUPQML_TREE_SYNC", nullptr);
    data->model->refresh(node);
    return;
  }

  for (IupQmlTreeNode* child : node->children)
    qmlTreeSyncExpanded(data, child);
}

static void qmlTreeRowCollapsedCb(Ihandle* ih, int row, bool recursively)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  IupQmlTreeNode* node = qmlTreeNodeAtRow(data, row);
  if (!node || node->kind != ITREE_BRANCH)
    return;

  if (recursively)
    qmlTreeSetExpandedRec(node, false);
  else
    node->expanded = false;

  data->model->refresh(node);

  if (iupAttribGet(ih, "_IUPQML_TREE_SYNC"))
    return;

  IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "BRANCHCLOSE_CB"));
  if (cb && cb(ih, qmlTreeFindNodeId(ih, node)) == IUP_IGNORE)
  {
    node->expanded = true;
    iupAttribSet(ih, "_IUPQML_TREE_SYNC", "1");
    qmlTreeExpandRow(data, row, true);
    iupAttribSet(ih, "_IUPQML_TREE_SYNC", nullptr);
    data->model->refresh(node);
  }
}

static void qmlTreeToggled(Ihandle* ih, int row, int state)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || !ih->data->show_toggle)
    return;

  IupQmlTreeNode* node = qmlTreeNodeAtRow(data, row);
  if (!node)
    return;

  int value = (state == Qt::Checked) ? 1 : (state == Qt::Unchecked ? 0 : -1);
  if (value == -1 && ih->data->show_toggle != 2)
    value = 0;

  node->toggle = value;
  data->model->refresh(node);

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "TOGGLEVALUE_CB"));
  if (cb)
    cb(ih, qmlTreeFindNodeId(ih, node), value);

  if (iupAttribGetBoolean(ih, "MARKWHENTOGGLE"))
    qmlTreeSelectNode(data, node, value == 1);
}

static void qmlTreeExecute(Ihandle* ih, IupQmlTreeNode* node, int toggle)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || !node)
    return;

  int id = qmlTreeFindNodeId(ih, node);

  if (node->kind == ITREE_LEAF)
  {
    IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "EXECUTELEAF_CB"));
    if (cb)
      cb(ih, id);
  }
  else
  {
    IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "EXECUTEBRANCH_CB"));
    if (cb)
      cb(ih, id);

    int row = qmlTreeRowOfNode(data, node);
    if (toggle && row >= 0)
      qmlTreeExpandRow(data, row, !qmlTreeRowExpanded(data, row));
  }
}

static void qmlTreeRenamed(Ihandle* ih, int row, const QString& text)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  IupQmlTreeNode* node = qmlTreeNodeAtRow(data, row);
  if (!node || node->title == text)
    return;

  auto cbRename = reinterpret_cast<IFnis>(IupGetCallback(ih, "RENAME_CB"));
  if (data->rename_done)
  {
    data->rename_done = false;
    cbRename = nullptr;
  }
  if (cbRename && cbRename(ih, qmlTreeFindNodeId(ih, node), const_cast<char*>(text.toUtf8().constData())) == IUP_IGNORE)
    return;

  node->title = text;
  data->model->refresh(node);
}

class IupQmlTreeRenameFilter : public QObject
{
public:
  Ihandle* ih;
  int row;
  IupQmlTreeRenameFilter(QObject* parent, Ihandle* handle, int edit_row) : QObject(parent), ih(handle), row(edit_row) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (event->type() != QEvent::KeyPress || !iupObjectCheck(ih))
      return false;

    auto* key = static_cast<QKeyEvent*>(event);
    if (key->key() != Qt::Key_Return && key->key() != Qt::Key_Enter)
      return false;

    IupQmlTreeData* data = qmlTreeGetData(ih);
    IupQmlTreeNode* node = data ? qmlTreeNodeAtRow(data, row) : nullptr;
    auto cbRename = reinterpret_cast<IFnis>(IupGetCallback(ih, "RENAME_CB"));
    QString text = obj->property("text").toString();
    if (!node || !cbRename || node->title == text)
      return false;

    int ret = cbRename(ih, qmlTreeFindNodeId(ih, node), const_cast<char*>(text.toUtf8().constData()));
    if (!iupObjectCheck(ih) || ret == IUP_IGNORE)
      return true;

    data->rename_done = true;
    return false;
  }
};

static void qmlTreeRenameStarted(Ihandle* ih, QObject* editor, int row)
{
  if (!editor)
    return;

  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (data)
    data->rename_done = false;

  QMetaObject::invokeMethod(editor, [editor, ih, row]() {
    editor->installEventFilter(new IupQmlTreeRenameFilter(editor, ih, row));
  }, Qt::QueuedConnection);

  char* value = iupAttribGetStr(ih, "RENAMECARET");
  int pos = 1;
  if (value && iupStrToInt(value, &pos))
    editor->setProperty("cursorPosition", pos < 1 ? 0 : pos - 1);

  value = iupAttribGetStr(ih, "RENAMESELECTION");
  int start = 1, end = 1;
  if (value && iupStrToIntInt(value, &start, &end, ':') == 2 && start >= 1 && end >= 1)
    QMetaObject::invokeMethod(editor, "select", Qt::DirectConnection, Q_ARG(int, start - 1), Q_ARG(int, end - 1));
}

static void qmlTreeDragDropped(Ihandle* ih, int drag_row, const QPointF& scene_pos)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  IupQmlTreeNode* drag_node = qmlTreeNodeAtRow(data, drag_row);
  if (!drag_node)
    return;

  QQuickItem* content = iupqmlGetItemProperty(data->view, "contentItem");
  QPointF local = content ? content->mapFromScene(scene_pos) : data->view->mapFromScene(scene_pos);
  QPoint cell;
  QMetaObject::invokeMethod(data->view, "cellAtPosition", Qt::DirectConnection, Q_RETURN_ARG(QPoint, cell), Q_ARG(QPointF, local), Q_ARG(bool, true));
  IupQmlTreeNode* drop_node = cell.y() >= 0 ? qmlTreeNodeAtRow(data, cell.y()) : nullptr;
  if (drop_node == drag_node)
    return;

  if (drop_node && qmlTreeIsAncestorOf(drag_node, drop_node))
    return;

  int drag_id = qmlTreeFindNodeId(ih, drag_node);
  int drop_id = drop_node ? qmlTreeFindNodeId(ih, drop_node) : -1;

  int is_shift = 0, is_ctrl = 0;
  char key[5];
  iupdrvGetKeyState(key);
  if (key[0] == 'S')
    is_shift = 1;
  if (key[1] == 'C')
    is_ctrl = 1;

  auto cbDragDrop = reinterpret_cast<IFniiii>(IupGetCallback(ih, "DRAGDROP_CB"));
  int ret = IUP_CONTINUE;
  if (cbDragDrop)
    ret = cbDragDrop(ih, drag_id, drop_id, is_shift, is_ctrl);

  if (ret == IUP_CONTINUE && drop_id >= 0)
  {
    char dst[32];
    snprintf(dst, sizeof(dst), "%d", drop_id);
    if (is_ctrl)
      qmlTreeSetCopyNodeAttrib(ih, drag_id, dst);
    else
      qmlTreeSetMoveNodeAttrib(ih, drag_id, dst);
  }
}

static int qmlTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return -1;

  QQuickItem* content = iupqmlGetItemProperty(data->view, "contentItem");
  QPointF local = content ? content->mapFromItem(data->wrapper, QPointF(x, y)) : data->view->mapFromItem(data->wrapper, QPointF(x, y));
  QPoint cell;
  QMetaObject::invokeMethod(data->view, "cellAtPosition", Qt::DirectConnection, Q_RETURN_ARG(QPoint, cell), Q_ARG(QPointF, local), Q_ARG(bool, false));
  if (cell.y() < 0)
    return -1;

  return qmlTreeFindNodeId(ih, qmlTreeNodeAtRow(data, cell.y()));
}

/****************************************************************************
 * Key Filter
 ****************************************************************************/

class IupQmlTreeFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlTreeFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    (void)obj;
    if (event->type() != QEvent::KeyPress || !iupObjectCheck(ih))
      return false;

    IupQmlTreeData* data = qmlTreeGetData(ih);
    if (!data)
      return false;

    auto* evt = static_cast<QKeyEvent*>(event);

    if (evt->key() == Qt::Key_F2)
    {
      qmlTreeStartRename(ih);
      return true;
    }

    if (evt->key() == Qt::Key_Return || evt->key() == Qt::Key_Enter)
    {
      qmlTreeExecute(ih, qmlTreeGetCurrent(data), 1);
      return true;
    }

    if (evt->key() == Qt::Key_Space && ih->data->show_toggle)
    {
      IupQmlTreeNode* node = qmlTreeGetCurrent(data);
      if (node && node->toggle_visible)
      {
        int next = node->toggle == 1 ? 0 : 1;
        if (ih->data->show_toggle == 2 && node->toggle == 1)
          next = -1;
        else if (ih->data->show_toggle == 2 && node->toggle == -1)
          next = 0;
        int row = qmlTreeRowOfNode(data, node);
        qmlTreeToggled(ih, row, next == 1 ? Qt::Checked : (next == 0 ? Qt::Unchecked : Qt::PartiallyChecked));
        return true;
      }
    }

    return false;
  }
};

/****************************************************************************
 * Default Images
 ****************************************************************************/

#define ITREE_IMG_WIDTH   16
#define ITREE_IMG_HEIGHT  16

#define OO  0,0,0,0
#define DE  112,112,112,255
#define DW  248,248,248,255
#define DS  184,184,184,255
#define FE  160,120,20,255
#define FH  240,220,120,255
#define FL  230,205,90,255
#define FB  220,190,60,255
#define FD  200,170,40,255

static unsigned char qml_img_leaf[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, DE, DE, DE, DE, DE, DE, DE, DE, DE, OO, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DS, DS, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DS, DE, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DE, DE, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DW, DW, DW, DW, DW, DW, DW, DW, DE, OO, OO, OO,
  OO, OO, OO, DE, DE, DE, DE, DE, DE, DE, DE, DE, DE, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

static unsigned char qml_img_collapsed[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, FE, FE, FE, FE, FE, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, FE, FH, FH, FH, FH, FH, FE, OO, OO, OO, OO, OO, OO, OO, OO,
  FE, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FE, OO, OO,
  FE, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO, OO,
  FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

static unsigned char qml_img_expanded[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, FE, FE, FE, FE, FE, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, FE, FH, FH, FH, FH, FH, FE, OO, OO, OO, OO, OO, OO, OO, OO,
  FE, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FE, OO, OO,
  FE, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FE, OO, OO,
  FE, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FE, OO, OO,
  FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, OO,
  FE, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FE, OO,
  FE, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FE, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO,
  FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

#undef OO
#undef DE
#undef DW
#undef DS
#undef FE
#undef FH
#undef FL
#undef FB
#undef FD

static void qmlTreeInitializeImages()
{
  if (IupGetHandle("IMGLEAF_QML"))
    return;

  Ihandle* image_leaf = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, qml_img_leaf);
  Ihandle* image_collapsed = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, qml_img_collapsed);
  Ihandle* image_expanded = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, qml_img_expanded);

  IupSetHandle("IMGLEAF_QML", image_leaf);
  IupSetHandle("IMGCOLLAPSED_QML", image_collapsed);
  IupSetHandle("IMGEXPANDED_QML", image_expanded);
}

#undef ITREE_IMG_WIDTH
#undef ITREE_IMG_HEIGHT

/****************************************************************************
 * Themed Images
 ****************************************************************************/

static QPixmap* qmlTreeGetThemeIcon(const char* icon_name, int size)
{
  QIcon icon = QIcon::fromTheme(QString::fromUtf8(icon_name));
  if (icon.isNull())
    return nullptr;

  QPixmap pixmap = icon.pixmap(QSize(size, size));
  if (pixmap.isNull())
    return nullptr;

  return new QPixmap(pixmap);
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static const char* qmlTreeViewQml =
  IUPQML_IMPORTS
  "TreeView {\n"
  "  id: view\n"
  "  clip: true\n"
  "  boundsBehavior: Flickable.StopAtBounds\n"
  "  selectionBehavior: TableView.SelectRows\n"
  "  pointerNavigationEnabled: true\n"
  "  keyNavigationEnabled: true\n"
  "  activeFocusOnTab: true\n"
  "  editTriggers: TableView.NoEditTriggers\n"
  "  property int iupIndent: 20\n"
  "  property int iupRowHeight: 20\n"
  "  property font iupFont\n"
  "  property bool iupShowToggle: false\n"
  "  property bool iupTristate: false\n"
  "  property bool iupHideButtons: false\n"
  "  property bool iupDragDrop: false\n"
  "  property bool iupRubber: false\n"
  "  signal iupRubberMoved(point pos)\n"
  "  signal iupToggled(int row, int state)\n"
  "  signal iupRightClicked(int row)\n"
  "  signal iupDoubleClicked(int row)\n"
  "  signal iupRowClicked(int row)\n"
  "  signal iupPressed(int row, int modifiers)\n"
  "  signal iupRenamed(int row, string text)\n"
  "  signal iupRenameStarted(var editor, int row)\n"
  "  signal iupDragDropped(int row, point pos)\n"
  "  function iupScroll(row, top) { forceLayout(); positionViewAtRow(row, top ? TableView.AlignTop : TableView.Contain) }\n"
  "  function iupScrollVisible() { return (ScrollBar.horizontal.visible ? 1 : 0) + (ScrollBar.vertical.visible ? 2 : 0) }\n"
  "  ScrollBar.vertical: ScrollBar { policy: size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff }\n"
  "  ScrollBar.horizontal: ScrollBar { policy: size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff }\n"
  "  delegate: TreeViewDelegate {\n"
  "    id: d\n"
  "    indentation: view.iupIndent\n"
  "    implicitHeight: view.iupRowHeight\n"
  "    implicitWidth: Math.max(view.width, leftPadding + implicitContentWidth + rightPadding + rightMargin)\n"
  "    font: d.model.hasfont ? d.model.font : view.iupFont\n"
  "    Component.onCompleted: { if (view.iupHideButtons && indicator) indicator.visible = false }\n"
  "    background: Rectangle {\n"
  "      color: d.highlighted ? d.palette.highlight : (d.model.bgcolor ? d.model.bgcolor : \"transparent\")\n"
  "    }\n"
  "    contentItem: Row {\n"
  "      spacing: 4\n"
  "      CheckBox {\n"
  "        id: chk\n"
  "        visible: view.iupShowToggle && d.model.togglevisible\n"
  "        width: visible ? implicitWidth : 0\n"
  "        tristate: view.iupTristate\n"
  "        padding: 0\n"
  "        anchors.verticalCenter: parent.verticalCenter\n"
  "        checkState: d.model.toggle === 1 ? Qt.Checked : (d.model.toggle === -1 ? Qt.PartiallyChecked : Qt.Unchecked)\n"
  "        onClicked: { view.iupToggled(d.row, checkState); checkState = Qt.binding(function() { return d.model.toggle === 1 ? Qt.Checked : (d.model.toggle === -1 ? Qt.PartiallyChecked : Qt.Unchecked) }) }\n"
  "      }\n"
  "      Image {\n"
  "        source: d.model.image\n"
  "        visible: d.model.image !== \"\"\n"
  "        width: visible ? implicitWidth : 0\n"
  "        anchors.verticalCenter: parent.verticalCenter\n"
  "        cache: false\n"
  "      }\n"
  "      Label {\n"
  "        text: d.model.title\n"
  "        font: d.font\n"
  "        color: d.highlighted ? d.palette.highlightedText : (d.model.fgcolor ? d.model.fgcolor : d.palette.text)\n"
  "        anchors.verticalCenter: parent.verticalCenter\n"
  "        visible: !d.editing\n"
  "      }\n"
  "    }\n"
  "    onClicked: view.iupRowClicked(d.row)\n"
  "    onDoubleClicked: view.iupDoubleClicked(d.row)\n"
  "    TapHandler { acceptedButtons: Qt.LeftButton; acceptedModifiers: Qt.KeyboardModifierMask; onPressedChanged: if (pressed) view.iupPressed(d.row, point.modifiers) }\n"
  "    DragHandler { enabled: view.iupRubber; target: null; xAxis.enabled: false; onCentroidChanged: if (active) view.iupRubberMoved(centroid.scenePosition) }\n"
  "    TapHandler {\n"
  "      acceptedButtons: Qt.RightButton\n"
  "      onTapped: view.iupRightClicked(d.row)\n"
  "    }\n"
  "    DragHandler {\n"
  "      enabled: view.iupDragDrop\n"
  "      target: null\n"
  "      onActiveChanged: if (!active) view.iupDragDropped(d.row, centroid.scenePosition)\n"
  "    }\n"
  "    TableView.editDelegate: TextField {\n"
  "      id: editor\n"
  "      x: d.contentItem.x\n"
  "      y: 0\n"
  "      width: d.width - x\n"
  "      height: d.height\n"
  "      topPadding: 0\n"
  "      bottomPadding: 0\n"
  "      verticalAlignment: TextInput.AlignVCenter\n"
  "      text: d.model.title\n"
  "      font: d.font\n"
  "      Component.onCompleted: { selectAll(); view.iupRenameStarted(editor, d.row) }\n"
  "      TableView.onCommit: view.iupRenamed(d.row, editor.text)\n"
  "    }\n"
  "  }\n"
  "}\n";

static void qmlTreePressed(Ihandle* ih, int row, int modifiers)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return;

  IupQmlTreeNode* node = qmlTreeNodeAtRow(data, row);
  QModelIndex index = data->model->indexOf(node);
  if (!index.isValid())
    return;

  data->press_row = row;
  data->press_was_current = qmlTreeGetCurrent(data) == node;
  data->press_collapse = false;

  if (data->view->activeFocusOnTab())
    data->view->forceActiveFocus(Qt::MouseFocusReason);

  bool multiple = ih->data->mark_mode == ITREE_MARK_MULTIPLE;
  if (multiple && (modifiers & Qt::ControlModifier))
  {
    data->selection->select(index, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    data->selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    data->mark_start = node;
  }
  else if (multiple && (modifiers & Qt::ShiftModifier))
  {
    IupQmlTreeNode* anchor = data->mark_start ? data->mark_start : qmlTreeGetCurrent(data);
    int anchor_row = anchor ? qmlTreeRowOfNode(data, anchor) : row;
    if (anchor_row < 0)
      anchor_row = row;
    QItemSelection range;
    for (int r = qMin(anchor_row, row); r <= qMax(anchor_row, row); r++)
    {
      QModelIndex ri = data->model->indexOf(qmlTreeNodeAtRow(data, r));
      if (ri.isValid())
        range.select(ri, ri);
    }
    data->selection->select(range, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    data->selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
  }
  else if (!multiple)
    data->selection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  else if (!data->selection->isSelected(index))
    data->selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
  else
    data->press_collapse = data->selection->selectedRows().size() > 1;
}

static void qmlTreeRubberMoved(Ihandle* ih, const QPointF& scene_pos)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || data->press_row < 0)
    return;

  QQuickItem* content = iupqmlGetItemProperty(data->view, "contentItem");
  QPointF local = content ? content->mapFromScene(scene_pos) : data->view->mapFromScene(scene_pos);
  QPoint cell;
  QMetaObject::invokeMethod(data->view, "cellAtPosition", Qt::DirectConnection, Q_RETURN_ARG(QPoint, cell), Q_ARG(QPointF, QPointF(0, local.y())), Q_ARG(bool, true));
  int row = cell.y();
  if (row < 0)
    row = local.y() < 0 ? 0 : data->view->property("rows").toInt() - 1;

  QItemSelection range;
  for (int r = qMin(data->press_row, row); r <= qMax(data->press_row, row); r++)
  {
    QModelIndex ri = data->model->indexOf(qmlTreeNodeAtRow(data, r));
    if (ri.isValid())
      range.select(ri, ri);
  }
  data->press_collapse = false;
  data->selection->select(range, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

static void qmlTreeRowClicked(Ihandle* ih, int row)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data || row != data->press_row)
    return;

  if (data->press_collapse)
  {
    QModelIndex index = data->model->indexOf(qmlTreeNodeAtRow(data, row));
    if (index.isValid())
    {
      data->selection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      data->mark_start = qmlTreeNodeAtRow(data, row);
    }
    data->press_collapse = false;
  }

  if (!data->press_was_current || !ih->data->show_rename)
    return;

  int gen = ++data->rename_gen;
  QTimer::singleShot(QGuiApplication::styleHints()->mouseDoubleClickInterval(), data->view, [ih, gen]() {
    IupQmlTreeData* dd = iupObjectCheck(ih) ? qmlTreeGetData(ih) : nullptr;
    if (dd && dd->rename_gen == gen)
      qmlTreeStartRename(ih);
  });
}

static int qmlTreeMapMethod(Ihandle* ih)
{
  QQuickItem* wrapper = iupqmlCreateItem("import QtQuick\nItem { clip: true }");
  if (!wrapper)
    return IUP_ERROR;

  QQuickItem* back = iupqmlCreateItem("import QtQuick\nRectangle { color: palette.base; border.width: 1; border.color: palette.mid }");
  if (back)
    back->setParentItem(wrapper);

  QQuickItem* view = iupqmlCreateItem(qmlTreeViewQml);
  if (!view)
  {
    delete wrapper;
    return IUP_ERROR;
  }

  view->setParentItem(wrapper);
  view->setParent(wrapper);
  if (back)
    back->setParent(wrapper);

  auto* data = new IupQmlTreeData();
  data->model = new IupQmlTreeModel(ih);
  data->selection = new QItemSelectionModel(data->model);
  data->view = view;
  data->wrapper = wrapper;
  data->back = back;
  data->mark_start = nullptr;
  data->themed_leaf = nullptr;
  data->themed_collapsed = nullptr;
  data->themed_expanded = nullptr;
  data->drag_id = -1;
  data->rename_gen = 0;
  data->press_row = -1;
  data->press_was_current = false;
  data->press_collapse = false;

  ih->handle = reinterpret_cast<InativeHandle*>(wrapper);
  iupAttribSet(ih, "_IUPQML_TREE", reinterpret_cast<char*>(data));

  view->setProperty("model", QVariant::fromValue<QObject*>(data->model));
  view->setProperty("selectionModel", QVariant::fromValue<QItemSelectionModel*>(data->selection));
  view->setProperty("iupShowToggle", ih->data->show_toggle ? true : false);
  view->setProperty("iupTristate", ih->data->show_toggle == 2);
  view->setProperty("iupHideButtons", iupAttribGetBoolean(ih, "HIDEBUTTONS") ? true : false);
  view->setProperty("iupDragDrop", ih->data->show_dragdrop ? true : false);

  {
    int indent = iupAttribGetInt(ih, "INDENTATION");
    if (indent > 0)
      view->setProperty("iupIndent", indent);
  }

  iupqmlSetIhandle(view, ih);
  view->installEventFilter(new IupQmlTreeFilter(view, ih));
  for (QObject* child : view->findChildren<QObject*>())
  {
    if (strcmp(child->metaObject()->className(), "QQuickTableViewTapHandler") == 0)
      child->setProperty("enabled", false);
  }

  QObject::connect(data->selection, &QItemSelectionModel::selectionChanged, [ih](const QItemSelection& selected, const QItemSelection& deselected) {
    qmlTreeSelectionChanged(ih, selected, deselected);
  });
  QObject::connect(data->selection, &QItemSelectionModel::currentChanged, [ih](const QModelIndex& current, const QModelIndex&) {
    qmlTreeCurrentChanged(ih, current);
  });

  iupqmlConnect(view, "expanded(int,int)", [ih](void** args) {
    qmlTreeRowExpandedCb(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]));
  });
  iupqmlConnect(view, "collapsed(int,bool)", [ih](void** args) {
    qmlTreeRowCollapsedCb(ih, *static_cast<int*>(args[1]), *static_cast<bool*>(args[2]));
  });
  iupqmlConnect(view, "iupToggled(int,int)", [ih](void** args) {
    qmlTreeToggled(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]));
  });
  iupqmlConnect(view, "iupRightClicked(int)", [ih](void** args) {
    IupQmlTreeData* d = qmlTreeGetData(ih);
    IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "RIGHTCLICK_CB"));
    if (d && cb)
    {
      IupQmlTreeNode* node = qmlTreeNodeAtRow(d, *static_cast<int*>(args[1]));
      if (node)
        cb(ih, qmlTreeFindNodeId(ih, node));
    }
  });
  iupqmlConnect(view, "iupDoubleClicked(int)", [ih](void** args) {
    IupQmlTreeData* d = qmlTreeGetData(ih);
    if (d)
    {
      d->rename_gen++;
      qmlTreeExecute(ih, qmlTreeNodeAtRow(d, *static_cast<int*>(args[1])), 0);
    }
  });
  iupqmlConnect(view, "iupPressed(int,int)", [ih](void** args) {
    qmlTreePressed(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]));
  });
  iupqmlConnect(view, "iupRubberMoved(QPointF)", [ih](void** args) {
    qmlTreeRubberMoved(ih, *static_cast<QPointF*>(args[1]));
  });
  iupqmlConnect(view, "iupRowClicked(int)", [ih](void** args) {
    qmlTreeRowClicked(ih, *static_cast<int*>(args[1]));
  });
  iupqmlConnect(view, "iupRenamed(int,QString)", [ih](void** args) {
    qmlTreeRenamed(ih, *static_cast<int*>(args[1]), *static_cast<QString*>(args[2]));
  });
  iupqmlConnect(view, "iupRenameStarted(QVariant,int)", [ih](void** args) {
    qmlTreeRenameStarted(ih, qvariant_cast<QObject*>(*static_cast<QVariant*>(args[1])), *static_cast<int*>(args[2]));
  });
  iupqmlConnect(view, "iupDragDropped(int,QPointF)", [ih](void** args) {
    qmlTreeDragDropped(ih, *static_cast<int*>(args[1]), *static_cast<QPointF*>(args[2]));
  });

  iupdrvTreeUpdateMarkMode(ih);
  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, view);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(view, 0);

  qmlTreeInitializeImages();

  {
    char* img_name = iupAttribGetStr(ih, "IMAGELEAF");
    if (img_name && !iupStrEqualNoCase(img_name, "IMGLEAF"))
      ih->data->def_image_leaf = iupImageGetImage(img_name, ih, 0, nullptr);
    else
    {
      data->themed_leaf = qmlTreeGetThemeIcon("text-x-generic", 16);
      ih->data->def_image_leaf = data->themed_leaf ? reinterpret_cast<void*>(data->themed_leaf) : iupImageGetImage("IMGLEAF_QML", ih, 0, nullptr);
    }
  }

  {
    char* img_name = iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED");
    if (img_name && !iupStrEqualNoCase(img_name, "IMGCOLLAPSED"))
      ih->data->def_image_collapsed = iupImageGetImage(img_name, ih, 0, nullptr);
    else
    {
      data->themed_collapsed = qmlTreeGetThemeIcon("folder", 16);
      ih->data->def_image_collapsed = data->themed_collapsed ? reinterpret_cast<void*>(data->themed_collapsed) : iupImageGetImage("IMGCOLLAPSED_QML", ih, 0, nullptr);
    }
  }

  {
    char* img_name = iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED");
    if (img_name && !iupStrEqualNoCase(img_name, "IMGEXPANDED"))
      ih->data->def_image_expanded = iupImageGetImage(img_name, ih, 0, nullptr);
    else
    {
      data->themed_expanded = qmlTreeGetThemeIcon("folder-open", 16);
      if (!data->themed_expanded)
        data->themed_expanded = qmlTreeGetThemeIcon("folder", 16);
      ih->data->def_image_expanded = data->themed_expanded ? reinterpret_cast<void*>(data->themed_expanded) : iupImageGetImage("IMGEXPANDED_QML", ih, 0, nullptr);
    }
  }

  qmlTreeUpdateRowHeight(ih);

  if (iupAttribGetBoolean(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  IupSetCallback(ih, "_IUP_XY2POS_CB", reinterpret_cast<Icallback>(qmlTreeConvertXYToPos));

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

static void qmlTreeLayoutUpdateMethod(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  iupdrvBaseLayoutUpdateMethod(ih);
  if (!data)
    return;

  int w = ih->currentwidth, h = ih->currentheight;
  if (data->back)
    iupqmlSetPosSize(data->back, 0, 0, w, h);
  iupqmlSetPosSize(data->view, 1, 1, w - 2, h - 2);
}

static void qmlTreeUnMapMethod(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (data)
  {
    data->view->setProperty("selectionModel", QVariant());
    data->view->setProperty("model", QVariant());
    delete data->selection;
    delete data->model;

    delete data->themed_leaf;
    delete data->themed_collapsed;
    delete data->themed_expanded;

    delete data;
    iupAttribSet(ih, "_IUPQML_TREE", nullptr);
  }

  ih->data->node_count = 0;
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTreeAddBorders(Ihandle* ih, int* w, int* h)
{
  int border = 2 * 2;
  int sb = iupdrvGetScrollbarSize();
  int indent_icon = 16 + 20;
  (void)ih;
  *w += border + sb + indent_icon;
  *h += border;
}

extern "C" IUP_SDK_API void iupdrvTreeInitClass(Iclass* ic)
{
  ic->Map = qmlTreeMapMethod;
  ic->UnMap = qmlTreeUnMapMethod;
  ic->LayoutUpdate = qmlTreeLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", qmlTreeGetBgColorAttrib, qmlTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "HLCOLOR", nullptr, qmlTreeSetHlColorAttrib, IUPAF_SAMEASSYSTEM, "TXTHLCOLOR", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlTreeSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "SHOWRENAME", nullptr, qmlTreeSetShowRenameAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "RENAME", nullptr, qmlTreeSetRenameAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", nullptr, qmlTreeSetTopItemAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", qmlTreeGetCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", qmlTreeGetRootCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDALL", nullptr, qmlTreeSetExpandAllAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", qmlTreeGetIndentationAttrib, qmlTreeSetIndentationAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, qmlTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "EMPTYAS3STATE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INFOTIP", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDELINES", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDEBUTTONS", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGELEAF", nullptr, qmlTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", nullptr, qmlTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", nullptr, qmlTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "STATE", qmlTreeGetStateAttrib, qmlTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", qmlTreeGetDepthAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", qmlTreeGetKindAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", qmlTreeGetParentAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", qmlTreeGetNextAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", qmlTreeGetPreviousAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", qmlTreeGetFirstAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", qmlTreeGetLastAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", qmlTreeGetChildCountAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE", qmlTreeGetTitleAttrib, qmlTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", qmlTreeGetTitleFontAttrib, qmlTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFGCOLOR", nullptr, qmlTreeSetTitleFgColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEBGCOLOR", nullptr, qmlTreeSetTitleBgColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", qmlTreeGetColorAttrib, qmlTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", nullptr, qmlTreeSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", nullptr, qmlTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", qmlTreeGetToggleValueAttrib, qmlTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", qmlTreeGetToggleVisibleAttrib, qmlTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "MARKED", qmlTreeGetMarkedAttrib, qmlTreeSetMarkedAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", nullptr, qmlTreeSetMarkAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", nullptr, qmlTreeSetMarkStartAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", nullptr, qmlTreeSetMarkStartAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", qmlTreeGetMarkedNodesAttrib, qmlTreeSetMarkedNodesAttrib, nullptr, nullptr, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", qmlTreeGetValueAttrib, qmlTreeSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDROOT", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DELNODE", nullptr, qmlTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", nullptr, qmlTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", nullptr, qmlTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "RUBBERBAND", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", qmlTreeGetScrollVisibleAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
}

extern "C" IUP_SDK_API InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  IupQmlTreeData* data = qmlTreeGetData(ih);
  if (!data)
    return nullptr;

  return reinterpret_cast<InodeHandle*>(qmlTreeGetCurrent(data));
}

extern "C" IUP_SDK_API void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* itemSrc, InodeHandle* itemDst)
{
  if (!src || !dst || !itemSrc || !itemDst)
    return;

  IupQmlTreeData* dst_data = qmlTreeGetData(dst);
  if (!dst_data)
    return;

  auto* src_node = reinterpret_cast<IupQmlTreeNode*>(itemSrc);
  auto* dst_node = reinterpret_cast<IupQmlTreeNode*>(itemDst);

  int id_dst = qmlTreeFindNodeId(dst, dst_node);

  IupQmlTreeNode* dst_parent;
  int dst_index, id_new;
  qmlTreeGetDropTarget(dst_data, dst_node, &dst_parent, &dst_index, &id_new, id_dst);

  IupQmlTreeNode* new_node = qmlTreeCloneNode(dst, src_node);
  int count = 1 + new_node->totalChildCount();

  dst_data->model->insertNode(dst_parent, dst_index, new_node);
  dst->data->node_count += count;

  {
    int id_src = (id_new + count < dst->data->node_count) ? id_new + count : 0;
    iupTreeCopyMoveCache(dst, id_src, id_new, count, 1);
  }
  qmlTreeRebuildEntireCache(dst);
  qmlTreeResync(dst);
}

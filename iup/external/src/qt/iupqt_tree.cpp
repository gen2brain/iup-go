/** \file
 * \brief Tree Control - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QScrollBar>
#include <QWidget>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QItemSelectionModel>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QApplication>
#include <QStyle>
#include <QDrag>
#include <QMimeData>
#include <QToolTip>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <functional>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_tree.h"
}

#include "iupqt_drv.h"


/* Tree add position constants (implementation-specific) */
enum { ITREE_ADDROOT = 0, ITREE_ADDBRANCH = 1, ITREE_ADDLEAF = 2 };

/****************************************************************************
 * System Icon Loading
 ****************************************************************************/

static QPixmap* qtTreeGetThemeIcon(Ihandle* ih, const char* icon_name, int size)
{
  (void)ih;

  QIcon icon = QIcon::fromTheme(QString::fromUtf8(icon_name));

  if (icon.isNull())
  {
    QStyle* style = QApplication::style();

    if (strcmp(icon_name, "text-x-generic") == 0 || strcmp(icon_name, "text-plain") == 0)
    {
      icon = style->standardIcon(QStyle::SP_FileIcon);
    }
    else if (strcmp(icon_name, "folder") == 0)
    {
      icon = style->standardIcon(QStyle::SP_DirClosedIcon);
    }
    else if (strcmp(icon_name, "folder-open") == 0)
    {
      icon = style->standardIcon(QStyle::SP_DirOpenIcon);
    }
  }

  if (!icon.isNull())
  {
    QPixmap* pixmap = new QPixmap(icon.pixmap(QSize(size, size)));
    return pixmap;
  }

  return nullptr;
}

/****************************************************************************
 * Custom Tree Widget
 ****************************************************************************/

class IupQtTree : public QTreeWidget
{
private:
  Ihandle* ih;
  QTreeWidgetItem* mark_start_node;

protected:
  void mousePressEvent(QMouseEvent* event) override
  {
    if (!ih)
    {
      QTreeWidget::mousePressEvent(event);
      return;
    }

    if (event->button() == Qt::RightButton)
    {
      QTreeWidgetItem* item = itemAt(event->pos());
      if (item)
      {
        IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
        if (cb)
        {
          int id = (int)(size_t)item->data(0, Qt::UserRole).value<void*>();
          cb(ih, id);
        }
      }
    }
    QTreeWidget::mousePressEvent(event);
  }

  void mouseMoveEvent(QMouseEvent* event) override
  {
    /* All drag-drop handling is done by the universal event filter system in iupqt_dragdrop.cpp
     * Do not implement drag initiation here as it conflicts with the event filter approach. */
    QTreeWidget::mouseMoveEvent(event);
  }

  void mouseDoubleClickEvent(QMouseEvent* event) override
  {
    if (!ih)
    {
      QTreeWidget::mouseDoubleClickEvent(event);
      return;
    }

    QTreeWidgetItem* item = itemAt(event->pos());
    if (item)
    {
      int id = (int)(size_t)item->data(0, Qt::UserRole).value<void*>();
      int kind = item->childCount() > 0 || item->data(0, Qt::UserRole + 1).toBool() ? ITREE_BRANCH : ITREE_LEAF;

      if (kind == ITREE_LEAF)
      {
        IFni cb = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
        if (cb)
        {
          cb(ih, id);
          return;
        }
      }
      else
      {
        IFni cb = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
        if (cb)
        {
          cb(ih, id);
          return;
        }
      }
    }

    QTreeWidget::mouseDoubleClickEvent(event);
  }

  bool event(QEvent* e) override
  {
    if (!ih)
      return QTreeWidget::event(e);

    if (e->type() == QEvent::ToolTip)
    {
      IFnii cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
      if (cb && !iupAttribGetBoolean(ih, "INFOTIP"))
      {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);
        cb(ih, helpEvent->pos().x(), helpEvent->pos().y());
        return true;
      }
    }

    return QTreeWidget::event(e);
  }

  void startDrag()
  {
    QTreeWidgetItem* item = currentItem();
    if (!item || !ih)
      return;

    int id = (int)(size_t)item->data(0, Qt::UserRole).value<void*>();

    IFniiii cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
    if (cb)
    {
      int drag_id = id;
      int drop_id = -1;
      int shift = 0;
      int control = 0;

      cb(ih, drag_id, drop_id, shift, control);
    }
  }

  void dropEvent(QDropEvent* event) override
  {
    event->ignore();
  }

public:
  IupQtTree(Ihandle* ih_param) : QTreeWidget(), ih(ih_param), mark_start_node(nullptr) {}

  void setMarkStartNode(QTreeWidgetItem* node) { mark_start_node = node; }
  QTreeWidgetItem* getMarkStartNode() const { return mark_start_node; }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static QTreeWidgetItem* qtTreeFindNode(Ihandle* ih, int id)
{
  if (id < 0 || id >= ih->data->node_count)
  {
    return nullptr;
  }

  return (QTreeWidgetItem*)ih->data->node_cache[id].node_handle;
}

static int qtTreeFindNodeId(Ihandle* ih, QTreeWidgetItem* item)
{
  if (!item)
    return -1;

  return iupTreeFindNodeId(ih, (InodeHandle*)item);
}

static void qtTreeRebuildNodeCacheRec(Ihandle* ih, QTreeWidgetItem* item, int* id)
{
  int child_count = item->childCount();
  for (int i = 0; i < child_count; i++)
  {
    (*id)++;
    QTreeWidgetItem* child = item->child(i);
    ih->data->node_cache[*id].node_handle = (InodeHandle*)child;
    child->setData(0, Qt::UserRole, QVariant::fromValue((void*)(size_t)(*id)));

    qtTreeRebuildNodeCacheRec(ih, child, id);
  }
}

static void qtTreeRebuildNodeCache(Ihandle* ih, int& id, QTreeWidgetItem* item)
{
  ih->data->node_cache[id].node_handle = (InodeHandle*)item;
  item->setData(0, Qt::UserRole, QVariant::fromValue((void*)(size_t)id));

  qtTreeRebuildNodeCacheRec(ih, item, &id);
}

static void qtTreeRebuildEntireCache(Ihandle* ih)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;
  if (!tree)
    return;

  int id = -1;
  int top_count = tree->topLevelItemCount();
  for (int i = 0; i < top_count; i++)
  {
    QTreeWidgetItem* item = tree->topLevelItem(i);
    id++;
    qtTreeRebuildNodeCache(ih, id, item);
  }
}

static int qtTreeGetNodeKind(QTreeWidgetItem* item)
{
  if (item->data(0, Qt::UserRole + 1).toBool())
    return ITREE_BRANCH;

  if (item->childCount() > 0)
    return ITREE_BRANCH;

  return ITREE_LEAF;
}

static void qtTreeSetNodeKind(QTreeWidgetItem* item, int kind)
{
  item->setData(0, Qt::UserRole + 1, kind == ITREE_BRANCH);
}

static QTreeWidgetItem* qtTreeGetNextItem(Ihandle* ih, QTreeWidgetItem* item, int only_siblings)
{
  QTreeWidgetItem* next_item;

  if (!only_siblings && item->childCount() > 0)
    return item->child(0);

  next_item = item;
  QTreeWidgetItem* parent = next_item->parent();

  while (next_item)
  {
    int index;
    if (parent)
      index = parent->indexOfChild(next_item);
    else
    {
      IupQtTree* tree = (IupQtTree*)ih->handle;
      index = tree->indexOfTopLevelItem(next_item);
    }

    if (parent && index < parent->childCount() - 1)
      return parent->child(index + 1);
    else if (!parent)
    {
      IupQtTree* tree = (IupQtTree*)ih->handle;
      if (index < tree->topLevelItemCount() - 1)
        return tree->topLevelItem(index + 1);
      else
        return nullptr;
    }

    next_item = parent;
    parent = next_item ? next_item->parent() : nullptr;

    if (only_siblings)
      return nullptr;
  }

  return nullptr;
}

static QTreeWidgetItem* qtTreeGetPreviousItem(Ihandle* ih, QTreeWidgetItem* item)
{
  QTreeWidgetItem* parent = item->parent();
  int index;

  if (parent)
    index = parent->indexOfChild(item);
  else
  {
    IupQtTree* tree = (IupQtTree*)ih->handle;
    index = tree->indexOfTopLevelItem(item);
  }

  if (index > 0)
  {
    QTreeWidgetItem* prev;
    if (parent)
      prev = parent->child(index - 1);
    else
    {
      IupQtTree* tree = (IupQtTree*)ih->handle;
      prev = tree->topLevelItem(index - 1);
    }

    /* Get last descendant */
    while (prev->childCount() > 0)
      prev = prev->child(prev->childCount() - 1);

    return prev;
  }

  return parent;
}

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  QTreeWidgetItem* item = (QTreeWidgetItem*)node_handle;
  if (!item)
    return 0;

  int count = 0;
  std::function<void(QTreeWidgetItem*)> count_children = [&](QTreeWidgetItem* it) {
    for (int i = 0; i < it->childCount(); i++)
    {
      count++;
      count_children(it->child(i));
    }
  };

  count_children(item);
  return count;
}

extern "C" void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
  else if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
  {
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    if (iupAttribGetInt(ih, "RUBBERBAND"))
      tree->setSelectionBehavior(QAbstractItemView::SelectItems);
  }
}

/****************************************************************************
 * Node Addition Functions
 ****************************************************************************/

extern "C" void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;
  QTreeWidgetItem* new_item = nullptr;
  QTreeWidgetItem* ref_item = nullptr;
  int kindPrev = -1;

  if (id >= 0 && id < ih->data->node_count)
  {
    ref_item = qtTreeFindNode(ih, id);
    if (ref_item)
      kindPrev = qtTreeGetNodeKind(ref_item);
  }

  // Create the new item
  new_item = new QTreeWidgetItem();
  if (title)
    new_item->setText(0, QString::fromUtf8(title));
  qtTreeSetNodeKind(new_item, kind);

  // Set default image
  QPixmap* def_image = nullptr;
  if (kind == ITREE_BRANCH)
    def_image = (QPixmap*)ih->data->def_image_collapsed;
  else
    def_image = (QPixmap*)ih->data->def_image_leaf;

  if (def_image)
    new_item->setIcon(0, QIcon(*def_image));

  // Add checkbox if enabled
  if (ih->data->show_toggle)
  {
    if (ih->data->show_toggle == 2)
      new_item->setCheckState(0, Qt::PartiallyChecked);
    else
      new_item->setCheckState(0, Qt::Unchecked);
  }

  // Insert the item into the Qt tree widget
  if (ref_item)
  {
    if (kindPrev == ITREE_BRANCH && add)
    {
      // Add as first child of ref_item
      ref_item->insertChild(0, new_item);
    }
    else
    {
      // Add as sibling after ref_item
      QTreeWidgetItem* parent = ref_item->parent();
      if (parent)
      {
        int index = parent->indexOfChild(ref_item);
        parent->insertChild(index + 1, new_item);
      }
      else
      {
        int index = tree->indexOfTopLevelItem(ref_item);
        tree->insertTopLevelItem(index + 1, new_item);
      }
    }
    // Update the IUP cache
    iupTreeAddToCache(ih, add, kindPrev, (InodeHandle*)ref_item, (InodeHandle*)new_item);
  }
  else
  {
    // Add as root node (prepend or append)
    if (id == -1) // Prepend
      tree->insertTopLevelItem(0, new_item);
    else // Append (first node in empty tree)
      tree->addTopLevelItem(new_item);

    // Update the IUP cache
    iupTreeAddToCache(ih, 0, 0, NULL, (InodeHandle*)new_item);
  }

  // Handle ADDEXPANDED attribute for the parent
  QTreeWidgetItem* parent = new_item->parent();
  if (parent && parent->childCount() == 1)
  {
    if (ih->data->add_expanded)
      parent->setExpanded(true);
    else
      parent->setExpanded(false);
  }

  // Handle setting focus on the first node
  if (ih->data->node_count == 1)
  {
    tree->setMarkStartNode(new_item);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    tree->setCurrentItem(new_item);
    if (ih->data->mark_mode != ITREE_MARK_SINGLE)
      new_item->setSelected(false);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  // After adding, we must sync the cache with the tree's visual order
  qtTreeRebuildEntireCache(ih);
}

/****************************************************************************
 * Copy/Move Node Functions
 ****************************************************************************/

static QTreeWidgetItem* qtTreeCopyNode(Ihandle* ih, QTreeWidgetItem* src, QTreeWidgetItem* dst_parent, int dst_index)
{
  QTreeWidgetItem* new_item = new QTreeWidgetItem();

  /* Copy visual properties */
  new_item->setText(0, src->text(0));
  new_item->setIcon(0, src->icon(0));
  new_item->setFont(0, src->font(0));
  new_item->setForeground(0, src->foreground(0));
  new_item->setBackground(0, src->background(0));
  new_item->setData(0, Qt::UserRole + 1, src->data(0, Qt::UserRole + 1));

  /* Initialize checkbox state based on destination tree settings, not source */
  if (ih->data->show_toggle)
  {
    if (ih->data->show_toggle == 2)
      new_item->setCheckState(0, Qt::PartiallyChecked);
    else
      new_item->setCheckState(0, Qt::Unchecked);
  }

  /* Increment node count for the destination tree */
  ih->data->node_count++;

  /* Insert into destination */
  if (dst_parent)
    dst_parent->insertChild(dst_index, new_item);
  else
  {
    IupQtTree* tree = (IupQtTree*)ih->handle;
    tree->insertTopLevelItem(dst_index, new_item);
  }

  /* Recursively copy children */
  for (int i = 0; i < src->childCount(); i++)
  {
    qtTreeCopyNode(ih, src->child(i), new_item, i);
  }

  return new_item;
}

static QTreeWidgetItem* qtTreeDragDropCopyItem(Ihandle* src_ih, Ihandle* dst_ih, QTreeWidgetItem* src_item,
                                                QTreeWidgetItem* dst_parent, int dst_index)
{
  QTreeWidgetItem* new_item = new QTreeWidgetItem();

  /* Copy visual properties from source item */
  new_item->setText(0, src_item->text(0));
  new_item->setIcon(0, src_item->icon(0));
  new_item->setFont(0, src_item->font(0));
  new_item->setForeground(0, src_item->foreground(0));
  new_item->setBackground(0, src_item->background(0));
  /* Copy KIND */
  new_item->setData(0, Qt::UserRole + 1, src_item->data(0, Qt::UserRole + 1));
  /* Copy IMAGEEXPANDED */
  new_item->setData(0, Qt::UserRole + 2, src_item->data(0, Qt::UserRole + 2));


  /* Initialize checkbox state based on destination tree settings */
  if (dst_ih->data->show_toggle)
  {
    if (dst_ih->data->show_toggle == 2)
      new_item->setCheckState(0, Qt::PartiallyChecked);
    else
      new_item->setCheckState(0, Qt::Unchecked);
  }

  /* Increment node count for the destination tree */
  dst_ih->data->node_count++;

  /* Insert into destination tree */
  if (dst_parent)
    dst_parent->insertChild(dst_index, new_item);
  else
  {
    IupQtTree* dst_tree = (IupQtTree*)dst_ih->handle;
    dst_tree->insertTopLevelItem(dst_index, new_item);
  }

  return new_item;
}

static void qtTreeDragDropCopyChildren(Ihandle* src_ih, Ihandle* dst_ih, QTreeWidgetItem* src_item,
                                        QTreeWidgetItem* dst_item)
{
  int child_count = src_item->childCount();

  if (child_count == 0)
    return;

  for (int i = 0; i < child_count; i++)
  {
    QTreeWidgetItem* src_child = src_item->child(i);
    if (!src_child)
      continue;

    QTreeWidgetItem* new_item = qtTreeDragDropCopyItem(src_ih, dst_ih, src_child, dst_item, dst_item->childCount());
    qtTreeDragDropCopyChildren(src_ih, dst_ih, src_child, new_item);
  }
}

static int qtTreeSetCopyNodeAttrib(Ihandle* ih, int id_src, const char* name_dst)
{
  QTreeWidgetItem* src = qtTreeFindNode(ih, id_src);
  if (!src)
    return 0;

  int id_dst;
  if (iupStrToInt(name_dst, &id_dst))
  {
    QTreeWidgetItem* dst = qtTreeFindNode(ih, id_dst);
    if (!dst)
      return 0;

    /* Determine insertion point */
    QTreeWidgetItem* dst_parent;
    int dst_index;

    if (qtTreeGetNodeKind(dst) == ITREE_BRANCH && dst->isExpanded())
    {
      /* Insert as first child */
      dst_parent = dst;
      dst_index = 0;
    }
    else
    {
      /* Insert after dst */
      dst_parent = dst->parent();
      if (dst_parent)
        dst_index = dst_parent->indexOfChild(dst) + 1;
      else
      {
        IupQtTree* tree = (IupQtTree*)ih->handle;
        dst_index = tree->indexOfTopLevelItem(dst) + 1;
      }
    }

    /* Copy node */
    int old_count = ih->data->node_count;
    QTreeWidgetItem* new_item = qtTreeCopyNode(ih, src, dst_parent, dst_index);

    /* Update cache */
    int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)src);
    ih->data->node_count = old_count + count; // Update count based on old_count + added

    int id_new = id_dst + 1;
    if (qtTreeGetNodeKind(dst) == ITREE_BRANCH && !dst->isExpanded())
    {
      id_new += iupdrvTreeTotalChildCount(ih, (InodeHandle*)dst);
    }

    iupTreeCopyMoveCache(ih, id_src, id_new, count, 1);
    qtTreeRebuildNodeCache(ih, id_new, new_item);
  }

  return 1;
}

static int qtTreeSetMoveNodeAttrib(Ihandle* ih, int id_src, const char* name_dst)
{
  QTreeWidgetItem* src = qtTreeFindNode(ih, id_src);
  if (!src)
    return 0;

  int id_dst;
  if (iupStrToInt(name_dst, &id_dst))
  {
    QTreeWidgetItem* dst = qtTreeFindNode(ih, id_dst);
    if (!dst || src == dst)
      return 0;

    /* Cannot move to own descendant */
    QTreeWidgetItem* parent = dst->parent();
    while (parent)
    {
      if (parent == src)
        return 0;
      parent = parent->parent();
    }

    /* Remove from old position */
    QTreeWidgetItem* src_parent = src->parent();
    if (src_parent)
      src_parent->removeChild(src);
    else
    {
      IupQtTree* tree = (IupQtTree*)ih->handle;
      int index = tree->indexOfTopLevelItem(src);
      tree->takeTopLevelItem(index);
    }

    /* Determine insertion point */
    QTreeWidgetItem* dst_parent;
    int dst_index;
    int id_new;

    if (qtTreeGetNodeKind(dst) == ITREE_BRANCH && dst->isExpanded())
    {
      dst_parent = dst;
      dst_index = 0;
      id_new = id_dst + 1;
    }
    else
    {
      dst_parent = dst->parent();
      if (dst_parent)
        dst_index = dst_parent->indexOfChild(dst) + 1;
      else
      {
        IupQtTree* tree = (IupQtTree*)ih->handle;
        dst_index = tree->indexOfTopLevelItem(dst) + 1;
      }

      id_new = id_dst + 1;
      if (qtTreeGetNodeKind(dst) == ITREE_BRANCH)
      {
        id_new += iupdrvTreeTotalChildCount(ih, (InodeHandle*)dst);
      }
    }

    /* Insert at new position */
    if (dst_parent)
      dst_parent->insertChild(dst_index, src);
    else
    {
      IupQtTree* tree = (IupQtTree*)ih->handle;
      tree->insertTopLevelItem(dst_index, src);
    }

    /* Update cache */
    int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)src);
    if (id_new > id_src)
      id_new -= count;

    iupTreeCopyMoveCache(ih, id_src, id_new, count, 0);
    qtTreeRebuildNodeCache(ih, id_new, src);
  }

  return 1;
}

/****************************************************************************
 * Attribute Setters/Getters
 ****************************************************************************/

static int qtTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, nullptr);
  return 1;
}

static int qtTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, nullptr);
  return 1;
}

static int qtTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, nullptr);
  return 1;
}

static int qtTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  if (value)
  {
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(value, ih, 0, nullptr);
    if (pixmap)
      item->setIcon(0, QIcon(*pixmap));
  }
  else
  {
    /* Set default image based on node kind */
    int kind = qtTreeGetNodeKind(item);
    QPixmap* def_image = nullptr;

    if (kind == ITREE_BRANCH)
    {
      if (item->isExpanded())
        def_image = (QPixmap*)ih->data->def_image_expanded;
      else
        def_image = (QPixmap*)ih->data->def_image_collapsed;
    }
    else
      def_image = (QPixmap*)ih->data->def_image_leaf;

    if (def_image)
      item->setIcon(0, QIcon(*def_image));
  }

  return 1;
}

static int qtTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  /* Store expanded image reference */
  item->setData(0, Qt::UserRole + 2, QString::fromUtf8(value ? value : ""));

  /* Update current icon if expanded */
  if (item->isExpanded() && value)
  {
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(value, ih, 0, nullptr);
    if (pixmap)
      item->setIcon(0, QIcon(*pixmap));
  }

  return 1;
}

static int qtTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  int indent = 0;
  iupStrToInt(value, &indent);

  IupQtTree* tree = (IupQtTree*)ih->handle;
  tree->setIndentation(indent);

  return 1;
}

static char* qtTreeGetIndentationAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return nullptr;

  IupQtTree* tree = (IupQtTree*)ih->handle;
  return iupStrReturnInt(tree->indentation());
}

static int qtTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  /* Qt doesn't have direct row spacing control */
  /* Would need custom item delegate for this */
  (void)ih;
  (void)value;
  return 1;
}

static int qtTreeSetHlColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->handle)
  {
    IupQtTree* tree = (IupQtTree*)ih->handle;
    QPalette palette = tree->palette();
    palette.setColor(QPalette::Highlight, QColor(r, g, b));
    tree->setPalette(palette);
  }

  return 1;
}

static int qtTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupQtTree* tree = (IupQtTree*)ih->handle;
  QPalette palette = tree->palette();
  palette.setColor(QPalette::Base, QColor(r, g, b));
  tree->setPalette(palette);

  return 1;
}

static char* qtTreeGetBgColorAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return nullptr;

  IupQtTree* tree = (IupQtTree*)ih->handle;
  QColor color = tree->palette().color(QPalette::Base);
  return iupStrReturnStrf("%d %d %d", color.red(), color.green(), color.blue());
}

static int qtTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupQtTree* tree = (IupQtTree*)ih->handle;
  QPalette palette = tree->palette();
  palette.setColor(QPalette::Text, QColor(r, g, b));
  tree->setPalette(palette);

  return 1;
}

static int qtTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);

  /* If tree is empty and setting TITLE0, create the root node */
  if (!item && id == 0 && ih->data->node_count == 0)
  {
    IupQtTree* tree = (IupQtTree*)ih->handle;
    if (tree)
    {
      /* Create root node */
      item = new QTreeWidgetItem(tree);
      tree->addTopLevelItem(item);
      qtTreeSetNodeKind(item, ITREE_BRANCH);  /* Root is typically a branch */

      /* Initialize cache for node 0 */
      ih->data->node_count = 1;
      ih->data->node_cache[0].node_handle = (InodeHandle*)item;
      item->setData(0, Qt::UserRole, QVariant::fromValue((void*)(size_t)0));

      /* Set default image for branch */
      QPixmap* def_image = (QPixmap*)ih->data->def_image_collapsed;
      if (def_image)
        item->setIcon(0, QIcon(*def_image));

      /* Set as mark start */
      tree->setMarkStartNode(item);
    }
  }

  if (!item)
    return 0;

  if (value)
    item->setText(0, QString::fromUtf8(value));

  return 1;
}

static char* qtTreeGetTitleAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  QString text = item->text(0);
  return iupStrReturnStr(text.toUtf8().constData());
}

static int qtTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  QFont* font = iupqtGetQFont(value);
  if (font)
    item->setFont(0, *font);

  return 1;
}

static char* qtTreeGetTitleFontAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  QFont font = item->font(0);
  return iupStrReturnStr(font.toString().toUtf8().constData());
}

static int qtTreeSetTitleFgColorAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    item->setForeground(0, QBrush(QColor(r, g, b)));

  return 1;
}

static char* qtTreeGetColorAttrib(Ihandle* ih, int id)
{
  return qtTreeGetTitleFontAttrib(ih, id);
}

static int qtTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  return qtTreeSetTitleFgColorAttrib(ih, id, value);
}

static int qtTreeSetTitleBgColorAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    item->setBackground(0, QBrush(QColor(r, g, b)));

  return 1;
}

static int qtTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  if (iupStrEqualNoCase(value, "EXPANDED"))
    item->setExpanded(true);
  else
    item->setExpanded(false);

  /* Update image based on expanded state */
  QString expanded_img = item->data(0, Qt::UserRole + 2).toString();
  if (!expanded_img.isEmpty() && item->isExpanded())
  {
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(expanded_img.toUtf8().constData(), ih, 0, nullptr);
    if (pixmap)
      item->setIcon(0, QIcon(*pixmap));
  }

  return 1;
}

static char* qtTreeGetStateAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  if (item->isExpanded())
    return (char*)"EXPANDED";
  else
    return (char*)"COLLAPSED";
}

static char* qtTreeGetDepthAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  int depth = 0;
  QTreeWidgetItem* parent = item->parent();
  while (parent)
  {
    depth++;
    parent = parent->parent();
  }

  return iupStrReturnInt(depth);
}

static char* qtTreeGetKindAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  int kind = qtTreeGetNodeKind(item);
  if (kind == ITREE_BRANCH)
    return (char*)"BRANCH";
  else
    return (char*)"LEAF";
}

static char* qtTreeGetParentAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  QTreeWidgetItem* parent = item->parent();
  if (!parent)
    return nullptr;

  int parent_id = qtTreeFindNodeId(ih, parent);
  return iupStrReturnInt(parent_id);
}

static char* qtTreeGetNextAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  QTreeWidgetItem* next = qtTreeGetNextItem(ih, item, 0);
  if (!next)
    return nullptr;

  return iupStrReturnInt(qtTreeFindNodeId(ih, next));
}

static char* qtTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  QTreeWidgetItem* prev = qtTreeGetPreviousItem(ih, item);
  if (!prev)
    return nullptr;

  return iupStrReturnInt(qtTreeFindNodeId(ih, prev));
}

static char* qtTreeGetFirstAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item || item->childCount() == 0)
    return nullptr;

  return iupStrReturnInt(qtTreeFindNodeId(ih, item->child(0)));
}

static char* qtTreeGetLastAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  QTreeWidgetItem* last = item;
  while (last->childCount() > 0)
    last = last->child(last->childCount() - 1);

  if (last == item)
    return nullptr;

  return iupStrReturnInt(qtTreeFindNodeId(ih, last));
}

static char* qtTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  return iupStrReturnInt(item->childCount());
}

static char* qtTreeGetRootCountAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return nullptr;

  IupQtTree* tree = (IupQtTree*)ih->handle;
  return iupStrReturnInt(tree->topLevelItemCount());
}

static char* qtTreeGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->node_count);
}

static int qtTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;

  if (iupStrEqualNoCase(value, "ROOT"))
  {
    tree->clearSelection();
    QTreeWidgetItem* root = tree->topLevelItem(0);
    if (root)
      tree->setCurrentItem(root);
  }
  else if (iupStrEqualNoCase(value, "LAST"))
  {
    tree->clearSelection();
    int count = tree->topLevelItemCount();
    if (count > 0)
    {
      QTreeWidgetItem* item = tree->topLevelItem(count - 1);
      tree->setCurrentItem(item);
    }
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    tree->clearSelection();
  }
  else
  {
    int id = atoi(value);
    QTreeWidgetItem* item = qtTreeFindNode(ih, id);
    if (item)
    {
      tree->clearSelection();
      tree->setCurrentItem(item);
      tree->scrollToItem(item);
    }
  }

  return 0;
}

static char* qtTreeGetValueAttrib(Ihandle* ih)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;
  QTreeWidgetItem* item = tree->currentItem();

  if (!item)
    return nullptr;

  int id = qtTreeFindNodeId(ih, item);
  return iupStrReturnInt(id);
}

static int qtTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  item->setSelected(iupStrBoolean(value));
  return 1;
}

static char* qtTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  return iupStrReturnBoolean(item->isSelected());
}

static int qtTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;

  if (iupStrEqualNoCase(value, "BLOCK"))
  {
    /* Mark from markstart to current */
    QTreeWidgetItem* mark_start = tree->getMarkStartNode();
    QTreeWidgetItem* current = tree->currentItem();

    if (mark_start && current)
    {
      int id_start = qtTreeFindNodeId(ih, mark_start);
      int id_end = qtTreeFindNodeId(ih, current);

      if (id_start > id_end)
      {
        int tmp = id_start;
        id_start = id_end;
        id_end = tmp;
      }

      for (int i = id_start; i <= id_end; i++)
      {
        QTreeWidgetItem* item = qtTreeFindNode(ih, i);
        if (item)
          item->setSelected(true);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "CLEARALL"))
  {
    tree->clearSelection();
  }
  else if (iupStrEqualNoCase(value, "MARKALL"))
  {
    tree->selectAll();
  }
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      QTreeWidgetItem* item = qtTreeFindNode(ih, i);
      if (item)
        item->setSelected(!item->isSelected());
    }
  }

  return 1;
}

static int qtTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  int id = atoi(value);
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);

  if (item)
  {
    IupQtTree* tree = (IupQtTree*)ih->handle;
    tree->setMarkStartNode(item);
  }

  return 1;
}

static char* qtTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;
  QList<QTreeWidgetItem*> selected = tree->selectedItems();

  if (selected.isEmpty())
    return nullptr;

  QString result;
  for (int i = 0; i < selected.count(); i++)
  {
    int id = qtTreeFindNodeId(ih, selected[i]);
    if (i > 0)
      result += ":";
    result += QString::number(id);
  }

  return iupStrReturnStr(result.toUtf8().constData());
}

static int qtTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;

  /* Clear current selection */
  tree->clearSelection();

  if (!value)
    return 0;

  /* Parse marked list */
  QString str = QString::fromUtf8(value);
  QStringList ids = str.split(':', Qt::SkipEmptyParts);

  for (const QString& id_str : ids)
  {
    int id = id_str.toInt();
    QTreeWidgetItem* item = qtTreeFindNode(ih, id);
    if (item)
      item->setSelected(true);
  }

  return 1;
}

static int qtTreeSetToggleValueAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->data->show_toggle)
    return 0;

  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  if (iupStrEqualNoCase(value, "ON"))
    item->setCheckState(0, Qt::Checked);
  else if (iupStrEqualNoCase(value, "OFF"))
    item->setCheckState(0, Qt::Unchecked);
  else if ((iupStrEqualNoCase(value, "NOTDEF") || (iupAttribGetBoolean(ih, "EMPTYAS3STATE") && !value))
           && ih->data->show_toggle == 2)
    item->setCheckState(0, Qt::PartiallyChecked);

  return 1;
}

static char* qtTreeGetToggleValueAttrib(Ihandle* ih, int id)
{
  if (!ih->data->show_toggle)
    return nullptr;

  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  Qt::CheckState state = item->checkState(0);
  if (state == Qt::Checked)
    return (char*)"ON";
  else if (state == Qt::Unchecked)
    return (char*)"OFF";
  else if (state == Qt::PartiallyChecked && ih->data->show_toggle == 2)
    return (char*)"NOTDEF";

  return nullptr;
}

static int qtTreeSetToggleVisibleAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->data->show_toggle)
    return 0;

  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return 0;

  if (iupStrBoolean(value))
  {
    /* Show checkbox */
    if (item->checkState(0) == Qt::Unchecked)  // Might be hidden
      item->setCheckState(0, Qt::Unchecked);
  }
  else
  {
    /* Hide checkbox by clearing check state role */
    item->setData(0, Qt::CheckStateRole, QVariant());
  }

  return 1;
}

static char* qtTreeGetToggleVisibleAttrib(Ihandle* ih, int id)
{
  if (!ih->data->show_toggle)
    return nullptr;

  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (!item)
    return nullptr;

  /* Check if checkbox is visible */
  return iupStrReturnBoolean(item->data(0, Qt::CheckStateRole).isValid());
}

static int qtTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  ih->data->show_rename = iupStrBoolean(value);

  if (ih->handle)
  {
    IupQtTree* tree = (IupQtTree*)ih->handle;
    if (ih->data->show_rename)
      tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    else
      tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  }

  return 0;
}

static int qtTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->show_rename)
  {
    IupQtTree* tree = (IupQtTree*)ih->handle;
    QTreeWidgetItem* item = tree->currentItem();
    if (item)
      tree->editItem(item, 0);
  }

  (void)value;
  return 0;
}

static int qtTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;
  int id = atoi(value);
  QTreeWidgetItem* item = qtTreeFindNode(ih, id);

  if (item)
    tree->scrollToItem(item, QAbstractItemView::PositionAtTop);

  return 1;
}

static int qtTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)
    return 0;

  IupQtTree* tree = (IupQtTree*)ih->handle;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
    if (cb)
    {
      for (int i = 0; i < ih->data->node_count; i++)
        cb(ih, (char*)ih->data->node_cache[i].userdata);
    }

    tree->clear();
    iupTreeDelFromCache(ih, 0, ih->data->node_count);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);
    return 0;
  }

  if (iupStrEqualNoCase(value, "SELECTED"))
  {
    QTreeWidgetItem* item = qtTreeFindNode(ih, id);
    if (!item)
      return 0;

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
    if (cb)
    {
      // Call for all descendants + self
      std::function<void(QTreeWidgetItem*)> call_rec = [&](QTreeWidgetItem* it) {
        int it_id = qtTreeFindNodeId(ih, it);
        if (it_id != -1)
        {
          for (int i = 0; i < it->childCount(); i++)
            call_rec(it->child(i));
          cb(ih, (char*)ih->data->node_cache[it_id].userdata);
        }
      };
      call_rec(item);
    }

    int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);

    QTreeWidgetItem* parent = item->parent();
    if (parent)
      parent->removeChild(item);
    else
    {
      int index = tree->indexOfTopLevelItem(item);
      if (index >= 0)
        tree->takeTopLevelItem(index);
    }

    delete item;
    iupTreeDelFromCache(ih, id, count);

    // After deletion, the cache is shifted. We must rebuild it.
    qtTreeRebuildEntireCache(ih);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);
    return 1;
  }

  if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    QTreeWidgetItem* item = qtTreeFindNode(ih, id);
    if (!item)
      return 0;

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    while (item->childCount() > 0)
    {
      QTreeWidgetItem* child = item->child(0);
      int child_id = qtTreeFindNodeId(ih, child);

      IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
      if (cb)
      {
        std::function<void(QTreeWidgetItem*)> call_rec = [&](QTreeWidgetItem* it) {
          int it_id = qtTreeFindNodeId(ih, it);
          if (it_id != -1)
          {
            for (int i = 0; i < it->childCount(); i++)
              call_rec(it->child(i));
            cb(ih, (char*)ih->data->node_cache[it_id].userdata);
          }
        };
        call_rec(child);
      }

      int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)child);
      item->removeChild(child);
      delete child;
      iupTreeDelFromCache(ih, child_id, count);
    }

    // After deletion, the cache is shifted. We must rebuild it.
    qtTreeRebuildEntireCache(ih);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);
    return 1;
  }

  if (iupStrEqualNoCase(value, "MARKED"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    for (int i = 0; i < ih->data->node_count; /* increment only if not removed */)
    {
      QTreeWidgetItem* item = qtTreeFindNode(ih, i);
      if (item && item->isSelected())
      {
        IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
        if (cb)
        {
          std::function<void(QTreeWidgetItem*)> call_rec = [&](QTreeWidgetItem* it) {
            int it_id = qtTreeFindNodeId(ih, it);
            if (it_id != -1)
            {
              for (int j = 0; j < it->childCount(); j++)
                call_rec(it->child(j));
              cb(ih, (char*)ih->data->node_cache[it_id].userdata);
            }
          };
          call_rec(item);
        }

        int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);

        QTreeWidgetItem* parent = item->parent();
        if (parent)
          parent->removeChild(item);
        else
        {
          int index = tree->indexOfTopLevelItem(item);
          if (index >= 0)
            tree->takeTopLevelItem(index);
        }

        delete item;
        iupTreeDelFromCache(ih, i, count);
      }
      else
        i++;
    }

    // After all deletions are done, rebuild the cache once.
    qtTreeRebuildEntireCache(ih);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);
    return 1;
  }

  /* Default case: delete the single node specified by id */
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  QTreeWidgetItem* item = qtTreeFindNode(ih, id);
  if (item)
  {
    IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
    if (cb)
    {
      std::function<void(QTreeWidgetItem*)> call_rec = [&](QTreeWidgetItem* it) {
        int it_id = qtTreeFindNodeId(ih, it);
        if (it_id != -1)
        {
          for (int i = 0; i < it->childCount(); i++)
            call_rec(it->child(i));
          cb(ih, (char*)ih->data->node_cache[it_id].userdata);
        }
      };
      call_rec(item);
    }

    int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);

    QTreeWidgetItem* parent = item->parent();
    if (parent)
      parent->removeChild(item);
    else
    {
      int index = tree->indexOfTopLevelItem(item);
      if (index >= 0)
        tree->takeTopLevelItem(index);
    }

    delete item;
    iupTreeDelFromCache(ih, id, count);

    // After deletion, the cache is shifted. We must rebuild it.
    qtTreeRebuildEntireCache(ih);
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", nullptr);

  return 1;
}

static int qtTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;
  bool expand = iupStrBoolean(value);

  if (expand)
    tree->expandAll();
  else
    tree->collapseAll();

  return 0;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qtTreeSelectionChanged(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB"))
    return;

  IupQtTree* tree = (IupQtTree*)ih->handle;
  QList<QTreeWidgetItem*> selected = tree->selectedItems();

  IFnii cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cb)
  {
    if (selected.count() > 0)
    {
      int id = qtTreeFindNodeId(ih, selected.first());
      cb(ih, id, 1);
    }
  }

  /* Update markstart if in single selection mode */
  if (ih->data->mark_mode == ITREE_MARK_SINGLE && selected.count() > 0)
    tree->setMarkStartNode(selected.first());
}

static void qtTreeItemExpanded(Ihandle* ih, QTreeWidgetItem* item)
{
  int id = qtTreeFindNodeId(ih, item);

  /* Update image to expanded version if available */
  QString expanded_img = item->data(0, Qt::UserRole + 2).toString();
  if (!expanded_img.isEmpty())
  {
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(expanded_img.toUtf8().constData(), ih, 0, nullptr);
    if (pixmap)
      item->setIcon(0, QIcon(*pixmap));
  }

  IFni cb = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
  if (cb)
  {
    if (cb(ih, id) == IUP_IGNORE)
      item->setExpanded(false);
  }
}

static void qtTreeItemCollapsed(Ihandle* ih, QTreeWidgetItem* item)
{
  int id = qtTreeFindNodeId(ih, item);

  /* Update image to collapsed version */
  QPixmap* def_image = (QPixmap*)ih->data->def_image_collapsed;
  if (def_image)
    item->setIcon(0, QIcon(*def_image));

  IFni cb = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
  if (cb)
  {
    if (cb(ih, id) == IUP_IGNORE)
      item->setExpanded(true);
  }
}

static void qtTreeItemChanged(Ihandle* ih, QTreeWidgetItem* item, int column)
{
  if (column != 0)
    return;

  /* Check if this is a toggle change */
  if (ih->data->show_toggle)
  {
    int id = qtTreeFindNodeId(ih, item);
    Qt::CheckState state = item->checkState(0);

    IFnii cb = (IFnii)IupGetCallback(ih, "TOGGLEVALUE_CB");
    if (cb)
    {
      int value = (state == Qt::Checked) ? 1 : (state == Qt::Unchecked ? 0 : -1);
      cb(ih, id, value);
    }

    /* Auto-mark if MARKWHENTOGGLE */
    if (iupAttribGetBoolean(ih, "MARKWHENTOGGLE"))
    {
      item->setSelected(state == Qt::Checked);
    }
  }

  /* Check if this is a rename */
  if (ih->data->show_rename && !iupAttribGet(ih, "_IUPTREE_IGNORE_RENAME"))
  {
    int id = qtTreeFindNodeId(ih, item);
    QString new_title = item->text(0);

    IFnis cb = (IFnis)IupGetCallback(ih, "RENAME_CB");
    if (cb)
    {
      if (cb(ih, id, (char*)new_title.toUtf8().constData()) == IUP_IGNORE)
      {
        /* Restore old title - would need caching */
      }
    }
  }
}


/****************************************************************************
 * Convert XY to Position - for drag-drop support
 ****************************************************************************/

static int qtTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;
  QTreeWidgetItem* item = tree->itemAt(QPoint(x, y));

  if (item)
  {
    int id = qtTreeFindNodeId(ih, item);
    return id;
  }

  return -1;
}


/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtTreeMapMethod(Ihandle* ih)
{
  IupQtTree* tree = new IupQtTree(ih);

  if (!tree)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)tree;

  /* Configure tree */
  tree->setColumnCount(1);
  tree->setHeaderHidden(true);

  /* Set indentation */
  int indent = iupAttribGetInt(ih, "INDENTATION");
  if (indent > 0)
    tree->setIndentation(indent);
  else
    tree->setIndentation(20);

  tree->setUniformRowHeights(false);
  tree->setAnimated(true);

  /* Enable editing if show_rename */
  if (ih->data->show_rename)
    tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  else
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);

  /* Set selection mode */
  iupdrvTreeUpdateMarkMode(ih);

  if (IupGetCallback(ih, "TIPS_CB"))
    tree->viewport()->setAttribute(Qt::WA_AlwaysShowToolTips);

  iupqtAddToParent(ih);

  QObject::connect(tree, &QTreeWidget::itemSelectionChanged, [ih]() {
    qtTreeSelectionChanged(ih);
  });

  QObject::connect(tree, &QTreeWidget::itemExpanded, [ih](QTreeWidgetItem* item) {
    qtTreeItemExpanded(ih, item);
  });

  QObject::connect(tree, &QTreeWidget::itemCollapsed, [ih](QTreeWidgetItem* item) {
    qtTreeItemCollapsed(ih, item);
  });

  QObject::connect(tree, &QTreeWidget::itemChanged, [ih](QTreeWidgetItem* item, int column) {
    qtTreeItemChanged(ih, item, column);
  });

  /* Create root node if ADDROOT=YES */
  if (iupAttribGetBoolean(ih, "ADDROOT"))
  {
    QTreeWidgetItem* root = new QTreeWidgetItem(tree);
    root->setText(0, QString::fromUtf8("ROOT"));
    qtTreeSetNodeKind(root, ITREE_BRANCH);

    /* Initialize cache */
    ih->data->node_count = 1;
    ih->data->node_cache[0].node_handle = (InodeHandle*)root;
    root->setData(0, Qt::UserRole, QVariant::fromValue((void*)(size_t)0));

    /* Set as mark start */
    tree->setMarkStartNode(root);
  }

  /* Load default images from system icons */
  {
    char* img_name = iupAttribGetStr(ih, "IMAGELEAF");
    if (img_name && !iupStrEqualNoCase(img_name, "IMGLEAF"))
      ih->data->def_image_leaf = iupImageGetImage(img_name, ih, 0, nullptr);
    else
    {
      /* Load system icon for leaf nodes (file icon) */
      ih->data->def_image_leaf = qtTreeGetThemeIcon(ih, "text-x-generic", 16);
      if (ih->data->def_image_leaf)
        iupAttribSet(ih, "_IUPQT_THEMED_LEAF", (char*)ih->data->def_image_leaf);
    }
  }

  {
    char* img_name = iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED");
    if (img_name && !iupStrEqualNoCase(img_name, "IMGCOLLAPSED"))
      ih->data->def_image_collapsed = iupImageGetImage(img_name, ih, 0, nullptr);
    else
    {
      /* Load system icon for collapsed branches (folder icon) */
      ih->data->def_image_collapsed = qtTreeGetThemeIcon(ih, "folder", 16);
      if (ih->data->def_image_collapsed)
        iupAttribSet(ih, "_IUPQT_THEMED_COLLAPSED", (char*)ih->data->def_image_collapsed);
    }
  }

  {
    char* img_name = iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED");
    if (img_name && !iupStrEqualNoCase(img_name, "IMGEXPANDED"))
      ih->data->def_image_expanded = iupImageGetImage(img_name, ih, 0, nullptr);
    else
    {
      /* Load system icon for expanded branches (open folder icon) */
      ih->data->def_image_expanded = qtTreeGetThemeIcon(ih, "folder-open", 16);
      if (!ih->data->def_image_expanded)
        ih->data->def_image_expanded = qtTreeGetThemeIcon(ih, "folder", 16);
      if (ih->data->def_image_expanded)
        iupAttribSet(ih, "_IUPQT_THEMED_EXPANDED", (char*)ih->data->def_image_expanded);
    }
  }

  /* Register XY to position converter for drag-drop support */
  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)qtTreeConvertXYToPos);

  return IUP_NOERROR;
}

static void qtTreeUnMapMethod(Ihandle* ih)
{
  IupQtTree* tree = (IupQtTree*)ih->handle;

  if (tree)
  {
    /* Free themed icons if they were created */
    QPixmap* pixmap = (QPixmap*)iupAttribGet(ih, "_IUPQT_THEMED_LEAF");
    if (pixmap)
    {
      delete pixmap;
      iupAttribSet(ih, "_IUPQT_THEMED_LEAF", nullptr);
    }

    pixmap = (QPixmap*)iupAttribGet(ih, "_IUPQT_THEMED_COLLAPSED");
    if (pixmap)
    {
      delete pixmap;
      iupAttribSet(ih, "_IUPQT_THEMED_COLLAPSED", nullptr);
    }

    pixmap = (QPixmap*)iupAttribGet(ih, "_IUPQT_THEMED_EXPANDED");
    if (pixmap)
    {
      delete pixmap;
      iupAttribSet(ih, "_IUPQT_THEMED_EXPANDED", nullptr);
    }

    /* Delete the tree widget */
    delete tree;
    ih->handle = nullptr;
  }

  ih->data->node_count = 0;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvTreeInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtTreeMapMethod;
  ic->UnMap = qtTreeUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", qtTreeGetBgColorAttrib, qtTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qtTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "HLCOLOR", nullptr, qtTreeSetHlColorAttrib, IUPAF_SAMEASSYSTEM, "TXTHLCOLOR", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "SHOWRENAME", nullptr, qtTreeSetShowRenameAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "RENAME", nullptr, qtTreeSetRenameAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", nullptr, qtTreeSetTopItemAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", qtTreeGetCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", qtTreeGetRootCountAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDALL", nullptr, qtTreeSetExpandAllAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", qtTreeGetIndentationAttrib, qtTreeSetIndentationAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, qtTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "EMPTYAS3STATE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INFOTIP", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* IupTree Attributes - IMAGES */
  iupClassRegisterAttribute(ic, "IMAGELEAF", nullptr, qtTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", nullptr, qtTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", nullptr, qtTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttributeId(ic, "STATE", qtTreeGetStateAttrib, qtTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", qtTreeGetDepthAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", qtTreeGetKindAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", qtTreeGetParentAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", qtTreeGetNextAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", qtTreeGetPreviousAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", qtTreeGetFirstAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", qtTreeGetLastAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", qtTreeGetChildCountAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE", qtTreeGetTitleAttrib, qtTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", qtTreeGetTitleFontAttrib, qtTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFGCOLOR", nullptr, qtTreeSetTitleFgColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEBGCOLOR", nullptr, qtTreeSetTitleBgColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", qtTreeGetColorAttrib, qtTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", nullptr, qtTreeSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", nullptr, qtTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", qtTreeGetToggleValueAttrib, qtTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", qtTreeGetToggleVisibleAttrib, qtTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttributeId(ic, "MARKED", qtTreeGetMarkedAttrib, qtTreeSetMarkedAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", nullptr, qtTreeSetMarkAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", nullptr, qtTreeSetMarkStartAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", nullptr, qtTreeSetMarkStartAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", qtTreeGetMarkedNodesAttrib, qtTreeSetMarkedNodesAttrib, nullptr, nullptr, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", qtTreeGetValueAttrib, qtTreeSetValueAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttribute(ic, "ADDROOT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DELNODE", nullptr, qtTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", nullptr, qtTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", nullptr, qtTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Selection/Drag */
  iupClassRegisterAttribute(ic, "RUBBERBAND", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}

extern "C" InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  if (!ih->handle)
    return nullptr;

  IupQtTree* tree = (IupQtTree*)ih->handle;
  QTreeWidgetItem* item = tree->currentItem();

  return (InodeHandle*)item;
}

extern "C" void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* itemSrc, InodeHandle* itemDst)
{
  if (!src || !dst || !itemSrc || !itemDst)
    return;

  QTreeWidgetItem* src_item = (QTreeWidgetItem*)itemSrc;
  QTreeWidgetItem* dst_item = (QTreeWidgetItem*)itemDst;

  int old_count = dst->data->node_count;

  int id_dst = qtTreeFindNodeId(dst, dst_item);
  int id_new = id_dst + 1;

  int kind = qtTreeGetNodeKind(dst_item);

  QTreeWidgetItem* dst_parent;
  int dst_index;

  if (kind == ITREE_BRANCH && dst_item->isExpanded())
  {
    dst_parent = dst_item;
    dst_index = 0;
  }
  else
  {
    dst_parent = dst_item->parent();
    if (dst_parent)
      dst_index = dst_parent->indexOfChild(dst_item) + 1;
    else
    {
      IupQtTree* tree = (IupQtTree*)dst->handle;
      dst_index = tree->indexOfTopLevelItem(dst_item) + 1;
    }

    if (kind == ITREE_BRANCH)
    {
      int child_count = iupdrvTreeTotalChildCount(dst, (InodeHandle*)dst_item);
      id_new += child_count;
    }
  }

  QTreeWidgetItem* new_item = qtTreeDragDropCopyItem(src, dst, src_item, dst_parent, dst_index);
  qtTreeDragDropCopyChildren(src, dst, src_item, new_item);

  int count = dst->data->node_count - old_count;

  iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);
  qtTreeRebuildNodeCache(dst, id_new, new_item);
}

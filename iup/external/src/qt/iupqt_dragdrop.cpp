/** \file
 * \brief Drag and Drop Support - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QUrl>
#include <QList>
#include <QMouseEvent>
#include <QApplication>
#include <QClipboard>
#include <QAbstractItemView>
#include <QTreeWidget>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Drag and Drop Action Constants
 ****************************************************************************/

#define IUP_DRAG_NONE 0
#define IUP_DRAG_COPY 1
#define IUP_DRAG_MOVE 2
#define IUP_DRAG_LINK 3

struct IupQtDragDropData
{
  int is_source;
  int is_target;

  /* Source data */
  QDrag* current_drag;

  /* Last drop position */
  int last_x;
  int last_y;
};

/****************************************************************************
 * Get or Create Drag Drop Data
 ****************************************************************************/

static IupQtDragDropData* qtDragDropGetData(Ihandle* ih, int create)
{
  IupQtDragDropData* dd_data = (IupQtDragDropData*)iupAttribGet(ih, "_IUPQT_DRAGDROP_DATA");

  if (!dd_data && create)
  {
    dd_data = new IupQtDragDropData();
    dd_data->is_source = 0;
    dd_data->is_target = 0;
    dd_data->current_drag = nullptr;
    dd_data->last_x = 0;
    dd_data->last_y = 0;

    iupAttribSet(ih, "_IUPQT_DRAGDROP_DATA", (char*)dd_data);
  }

  return dd_data;
}

/****************************************************************************
 * Convert Drop Action to IUP
 ****************************************************************************/

static int qtDragDropActionToIup(Qt::DropAction action)
{
  switch (action)
  {
    case Qt::CopyAction:
      return IUP_DRAG_COPY;
    case Qt::MoveAction:
      return IUP_DRAG_MOVE;
    case Qt::LinkAction:
      return IUP_DRAG_LINK;
    default:
      return IUP_DRAG_NONE;
  }
}

/****************************************************************************
 * Convert IUP Action to Qt
 ****************************************************************************/

static Qt::DropAction qtDragDropActionFromIup(int action)
{
  switch (action)
  {
    case IUP_DRAG_COPY:
      return Qt::CopyAction;
    case IUP_DRAG_MOVE:
      return Qt::MoveAction;
    case IUP_DRAG_LINK:
      return Qt::LinkAction;
    default:
      return Qt::IgnoreAction;
  }
}

/****************************************************************************
 * Convert Qt Drop Actions to IUP flags
 ****************************************************************************/

static Qt::DropActions qtDragDropActionsFromIup(const char* value)
{
  Qt::DropActions actions = Qt::IgnoreAction;

  if (!value)
    return Qt::CopyAction;  /* Default */

  if (iupStrEqualNoCase(value, "COPY"))
    actions = Qt::CopyAction;
  else if (iupStrEqualNoCase(value, "MOVE"))
    actions = Qt::MoveAction;
  else if (iupStrEqualNoCase(value, "LINK"))
    actions = Qt::LinkAction;
  else if (iupStrEqualNoCase(value, "COPYMOVE"))
    actions = Qt::CopyAction | Qt::MoveAction;
  else if (iupStrEqualNoCase(value, "COPYLINK"))
    actions = Qt::CopyAction | Qt::LinkAction;
  else if (iupStrEqualNoCase(value, "MOVELINK"))
    actions = Qt::MoveAction | Qt::LinkAction;
  else if (iupStrEqualNoCase(value, "ALL"))
    actions = Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
  else
    actions = Qt::CopyAction;

  return actions;
}

/****************************************************************************
 * Get Keyboard Modifier State
 ****************************************************************************/

static void qtDragDropGetModifiers(char* status)
{
  Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();

  /* Use Qt button/key status helper */
  iupqtButtonKeySetStatus(mods, Qt::NoButton, 0, status, 0);
}

/****************************************************************************
 * Custom Event Filter for Drag and Drop
 ****************************************************************************/

class IupQtDragDropFilter : public QObject
{
public:
  Ihandle* ih;

  IupQtDragDropFilter(Ihandle* ih_param) : ih(ih_param) {}

protected:
  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (!iupObjectCheck(ih))
      return false;

    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget)
      return false;

    IupQtDragDropData* dd_data = qtDragDropGetData(ih, 0);
    if (!dd_data)
      return false;

    switch (event->type())
    {
      case QEvent::DragEnter:
      {
        if (!dd_data->is_target)
          return false;

        QDragEnterEvent* de = static_cast<QDragEnterEvent*>(event);

        IFniis cb = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
        if (cb)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          int x = de->position().x();
          int y = de->position().y();
#else
          int x = de->pos().x();
          int y = de->pos().y();
#endif
          dd_data->last_x = x;
          dd_data->last_y = y;

          char status[IUPKEY_STATUS_SIZE];
          qtDragDropGetModifiers(status);

          int ret = cb(ih, x, y, status);
          if (ret == IUP_IGNORE)
          {
            de->ignore();
            return true;
          }
        }

        de->acceptProposedAction();
        return true;
      }

      case QEvent::DragMove:
      {
        if (!dd_data->is_target)
          return false;

        QDragMoveEvent* dm = static_cast<QDragMoveEvent*>(event);

        IFniis cb = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
        if (cb)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          int x = dm->position().x();
          int y = dm->position().y();
#else
          int x = dm->pos().x();
          int y = dm->pos().y();
#endif
          dd_data->last_x = x;
          dd_data->last_y = y;

          char status[IUPKEY_STATUS_SIZE];
          qtDragDropGetModifiers(status);

          int ret = cb(ih, x, y, status);
          if (ret == IUP_IGNORE)
          {
            dm->ignore();
            return true;
          }
        }

        dm->acceptProposedAction();
        return true;
      }

      case QEvent::DragLeave:
      {
        if (!dd_data->is_target)
          return false;

        IFniis cb = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
        if (cb)
        {
          char status[IUPKEY_STATUS_SIZE];
          qtDragDropGetModifiers(status);
          cb(ih, -1, -1, status);
        }

        return false;
      }

      case QEvent::Drop:
      {
        if (!dd_data->is_target)
          return false;

        QDropEvent* drop = static_cast<QDropEvent*>(event);
        const QMimeData* mime = drop->mimeData();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        int x = drop->position().x();
        int y = drop->position().y();
#else
        int x = drop->pos().x();
        int y = drop->pos().y();
#endif
        dd_data->last_x = x;
        dd_data->last_y = y;

        const char* drop_types = iupAttribGetStr(ih, "DROPTYPES");
        bool handled_custom = false;

        if (drop_types && !iupStrEqualNoCase(drop_types, "TEXT") && !iupStrEqualNoCase(drop_types, "FILES"))
        {
          QString mime_type = QString("application/x-iup-") + QString::fromUtf8(drop_types).toLower();

          if (mime->hasFormat(mime_type))
          {
            IFnsViii cbDropData = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
            if (cbDropData)
            {
              QByteArray data = mime->data(mime_type);

              int action = qtDragDropActionToIup(drop->proposedAction());
              int ret = cbDropData(ih, (char*)drop_types, (void*)data.constData(), data.size(), x, y);

              if (ret == IUP_IGNORE)
              {
                drop->ignore();
              }
              else
              {
                drop->acceptProposedAction();
              }

              handled_custom = true;
              return true;
            }
          }
        }

        if (!handled_custom)
        {
          if (mime->hasText())
          {
            iupAttribSetStr(ih, "DROPTEXT", mime->text().toUtf8().constData());
          }

          if (mime->hasUrls())
          {
            QList<QUrl> urls = mime->urls();
            if (!urls.isEmpty())
            {
              QString files;
              for (int i = 0; i < urls.size(); i++)
              {
                if (i > 0)
                  files += "\n";
                files += urls[i].toLocalFile();
              }
              iupAttribSetStr(ih, "DROPFILESTARGET", files.toUtf8().constData());
            }
          }

          IFnsViii cb = (IFnsViii)IupGetCallback(ih, "DROP_CB");
          if (cb)
          {
            char status[IUPKEY_STATUS_SIZE];
            qtDragDropGetModifiers(status);

            int action = qtDragDropActionToIup(drop->proposedAction());

            int ret = cb(ih, nullptr, 0, x, y, action);

            if (ret == IUP_IGNORE)
            {
              drop->ignore();
            }
            else
            {
              drop->acceptProposedAction();
            }

            return true;
          }
        }

        drop->acceptProposedAction();
        return true;
      }

      case QEvent::MouseButtonPress:
      {
        if (!dd_data->is_source)
          return false;

        QMouseEvent* me = static_cast<QMouseEvent*>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        iupAttribSetInt(ih, "_IUPQT_DRAG_START_X", me->position().x());
        iupAttribSetInt(ih, "_IUPQT_DRAG_START_Y", me->position().y());
#else
        iupAttribSetInt(ih, "_IUPQT_DRAG_START_X", me->x());
        iupAttribSetInt(ih, "_IUPQT_DRAG_START_Y", me->y());
#endif

        return false;
      }

      case QEvent::MouseMove:
      {
        if (!dd_data->is_source)
          return false;

        QMouseEvent* me = static_cast<QMouseEvent*>(event);

        if (me->buttons() & Qt::LeftButton)
        {
          int start_x = iupAttribGetInt(ih, "_IUPQT_DRAG_START_X");
          int start_y = iupAttribGetInt(ih, "_IUPQT_DRAG_START_Y");

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          int dx = me->position().x() - start_x;
          int dy = me->position().y() - start_y;
#else
          int dx = me->x() - start_x;
          int dy = me->y() - start_y;
#endif
          int distance = dx * dx + dy * dy;

          if (distance > 25)
          {
            IFnii cb = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
            if (cb)
            {
              int ret = cb(ih, start_x, start_y);
              if (ret == IUP_IGNORE)
                return false;

              const char* drag_types = iupAttribGetStr(ih, "DRAGTYPES");
              if (drag_types)
              {
                QDrag* drag = new QDrag(widget);
                QMimeData* mime_data = new QMimeData();

                if (iupStrEqualNoCase(drag_types, "TEXT"))
                {
                  const char* text = iupAttribGetStr(ih, "DRAGTEXT");
                  if (text)
                    mime_data->setText(QString::fromUtf8(text));
                }
                else if (iupStrEqualNoCase(drag_types, "FILES"))
                {
                  const char* files = iupAttribGetStr(ih, "DRAGFILES");
                  if (files)
                  {
                    QList<QUrl> urls;
                    char* str = iupStrDup(files);
                    char* token = strtok(str, "\n");
                    while (token)
                    {
                      urls.append(QUrl::fromLocalFile(QString::fromUtf8(token)));
                      token = strtok(nullptr, "\n");
                    }
                    free(str);
                    mime_data->setUrls(urls);
                  }
                }
                else
                {
                  IFns cbDragDataSize = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
                  IFnsVi cbDragData = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");

                  if (cbDragDataSize && cbDragData)
                  {
                    int size = cbDragDataSize(ih, (char*)drag_types);

                    if (size > 0)
                    {
                      void* data = malloc(size);
                      cbDragData(ih, (char*)drag_types, data, size);

                      QString mime_type = QString("application/x-iup-") + QString::fromUtf8(drag_types).toLower();

                      QByteArray byte_array((const char*)data, size);
                      mime_data->setData(mime_type, byte_array);

                      free(data);
                    }
                  }
                }

                drag->setMimeData(mime_data);

                const char* actions_str = iupAttribGetStr(ih, "DRAGSOURCEACTION");
                Qt::DropActions actions = qtDragDropActionsFromIup(actions_str);

                dd_data->current_drag = drag;

                Qt::DropAction result = drag->exec(actions);

                IFni end_cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
                if (end_cb)
                {
                  int remove = -1;
                  if (result == Qt::MoveAction)
                    remove = 1;
                  else if (result == Qt::CopyAction)
                    remove = 0;

                  end_cb(ih, remove);
                }

                dd_data->current_drag = nullptr;

                return true;
              }
            }
          }
        }

        return false;
      }

      default:
        break;
    }

    return false;
  }
};

/****************************************************************************
 * Enable Drag Source
 ****************************************************************************/

static int qtDragDropSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  IupQtDragDropData* dd_data = qtDragDropGetData(ih, 1);
  if (!dd_data)
    return 0;

  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
  {
    return 0;
  }

  int enable = iupStrBoolean(value);

  if (enable && !dd_data->is_source)
  {
    dd_data->is_source = 1;

    IupQtDragDropFilter* filter = new IupQtDragDropFilter(ih);
    widget->installEventFilter(filter);
    iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", (char*)filter);

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(widget);
    if (view && view->viewport())
    {
      view->viewport()->installEventFilter(filter);

      QTreeWidget* tree = qobject_cast<QTreeWidget*>(widget);
      if (tree)
      {
        tree->setDragEnabled(false);
        tree->setDropIndicatorShown(false);

        if (dd_data->is_target)
          tree->setDragDropMode(QAbstractItemView::DropOnly);
        else
          tree->setDragDropMode(QAbstractItemView::NoDragDrop);
      }
    }
  }
  else if (!enable && dd_data->is_source)
  {
    dd_data->is_source = 0;

    IupQtDragDropFilter* filter = (IupQtDragDropFilter*)iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER");
    if (filter && !dd_data->is_target)
    {
      widget->removeEventFilter(filter);
      delete filter;
      iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", nullptr);
    }

    QTreeWidget* tree = qobject_cast<QTreeWidget*>(widget);
    if (tree && dd_data->is_target)
    {
      tree->setDragDropMode(QAbstractItemView::DropOnly);
    }
  }

  return 1;
}

/****************************************************************************
 * Enable Drop Target
 ****************************************************************************/

static int qtDragDropSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  IupQtDragDropData* dd_data = qtDragDropGetData(ih, 1);
  if (!dd_data)
    return 0;

  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
  {
    return 0;
  }

  int enable = iupStrBoolean(value);

  if (enable && !dd_data->is_target)
  {
    dd_data->is_target = 1;
    widget->setAcceptDrops(true);

    IupQtDragDropFilter* filter = (IupQtDragDropFilter*)iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER");
    if (!filter)
    {
      filter = new IupQtDragDropFilter(ih);
      widget->installEventFilter(filter);
      iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", (char*)filter);
    }

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(widget);
    if (view && view->viewport())
    {
      if (!filter)
      {
        view->viewport()->installEventFilter((IupQtDragDropFilter*)iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER"));
      }

      QTreeWidget* tree = qobject_cast<QTreeWidget*>(widget);
      if (tree)
      {
        tree->setAcceptDrops(true);
        tree->setDropIndicatorShown(false);
        tree->setDragDropMode(QAbstractItemView::DropOnly);
      }
    }
  }
  else if (!enable && dd_data->is_target)
  {
    dd_data->is_target = 0;
    widget->setAcceptDrops(false);

    if (!dd_data->is_source)
    {
      IupQtDragDropFilter* filter = (IupQtDragDropFilter*)iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER");
      if (filter)
      {
        widget->removeEventFilter(filter);
        delete filter;
        iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", nullptr);
      }
    }

    QTreeWidget* tree = qobject_cast<QTreeWidget*>(widget);
    if (tree && !dd_data->is_source)
    {
      tree->setDragDropMode(QAbstractItemView::NoDragDrop);
    }
  }

  return 1;
}

/****************************************************************************
 * Set Drop Types
 ****************************************************************************/

static int qtDragDropSetDropTypesAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "_IUPQT_DROPTYPES", value);
  return 1;
}

/****************************************************************************
 * Set Drag Source Move (Compatibility Attribute)
 ****************************************************************************/

static int qtDragDropSetDragSourceMoveAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    iupAttribSetStr(ih, "DRAGSOURCEACTION", "COPYMOVE");
  else
    iupAttribSetStr(ih, "DRAGSOURCEACTION", "COPY");

  return 1;
}

/****************************************************************************
 * Set Drop Files Target (Convenience Attribute)
 ****************************************************************************/

static int qtDragDropSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  int enable = iupStrBoolean(value);

  if (enable)
  {
    iupAttribSetStr(ih, "DROPTARGET", "YES");
    iupAttribSetStr(ih, "DROPTYPES", "FILES");
  }
  else
  {
    iupAttribSetStr(ih, "DROPTARGET", "NO");
  }

  qtDragDropSetDropTargetAttrib(ih, enable ? "YES" : "NO");
  qtDragDropSetDropTypesAttrib(ih, enable ? "FILES" : NULL);

  return 1;
}

/****************************************************************************
 * Cleanup Drag Drop Resources
 ****************************************************************************/

extern "C" void iupqtDragDropCleanup(Ihandle* ih)
{
  IupQtDragDropData* dd_data = (IupQtDragDropData*)iupAttribGet(ih, "_IUPQT_DRAGDROP_DATA");

  if (dd_data)
  {
    QWidget* widget = (QWidget*)ih->handle;
    if (widget)
    {
      IupQtDragDropFilter* filter = (IupQtDragDropFilter*)iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER");
      if (filter)
      {
        widget->removeEventFilter(filter);
        delete filter;
        iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", nullptr);
      }
    }

    delete dd_data;
    iupAttribSet(ih, "_IUPQT_DRAGDROP_DATA", nullptr);
  }
}

/****************************************************************************
 * Register Drag Drop Attributes
 ****************************************************************************/

extern "C" void iupqtDragDropRegisterAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "DRAGSOURCE", nullptr, qtDragDropSetDragSourceAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", nullptr, qtDragDropSetDropTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES", nullptr, qtDragDropSetDropTypesAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGTYPES", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEACTION", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGETACTION", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", nullptr, qtDragDropSetDragSourceMoveAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", nullptr, qtDragDropSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", nullptr, qtDragDropSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGTEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGFILES", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");
  iupClassRegisterCallback(ic, "DROP_CB", "sViiii");
}

extern "C" void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupqtDragDropRegisterAttrib(ic);
}

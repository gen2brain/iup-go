/** \file
 * \brief Drag and Drop Support - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QDrag>
#include <QPixmap>
#include <QMimeData>
#include <QDragMoveEvent>
#include <QUrl>
#include <QList>
#include <QApplication>
#include <QTreeWidget>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_key.h"
#include "iup_image.h"
}

#include "iupqt_drv.h"


struct IupQtDragDropData
{
  int is_source;
  int is_target;

  QDrag* current_drag;

  int last_x;
  int last_y;
};

static IupQtDragDropData* qtDragDropGetData(Ihandle* ih, int create)
{
  auto* dd_data = reinterpret_cast<IupQtDragDropData*>(iupAttribGet(ih, "_IUPQT_DRAGDROP_DATA"));

  if (!dd_data && create)
  {
    dd_data = new IupQtDragDropData();
    dd_data->is_source = 0;
    dd_data->is_target = 0;
    dd_data->current_drag = nullptr;
    dd_data->last_x = 0;
    dd_data->last_y = 0;

    iupAttribSet(ih, "_IUPQT_DRAGDROP_DATA", reinterpret_cast<char*>(dd_data));
  }

  return dd_data;
}

static void qtDragDropGetModifiers(char* status)
{
  Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
  iupqtButtonKeySetStatus(mods, Qt::NoButton, 0, status, 0);
}

class IupQtDragDropFilter : public QObject
{
public:
  Ihandle* ih;

  IupQtDragDropFilter(Ihandle* ih_param) : ih(ih_param) {}

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

        auto* de = static_cast<QDragEnterEvent*>(event);

        auto cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "DROPMOTION_CB"));
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

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
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

        auto* dm = static_cast<QDragMoveEvent*>(event);

        auto cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "DROPMOTION_CB"));
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

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
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

        return false;
      }

      case QEvent::Drop:
      {
        if (!dd_data->is_target)
          return false;

        auto* drop = static_cast<QDropEvent*>(event);
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
        if (drop_types)
        {
          QString mime_type = QString("application/x-iup-") + QString::fromUtf8(drop_types).toLower();

          if (mime->hasFormat(mime_type))
          {
            auto cbDropData = reinterpret_cast<IFnsViii>(IupGetCallback(ih, "DROPDATA_CB"));
            if (cbDropData)
            {
              QByteArray data = mime->data(mime_type);
              cbDropData(ih, const_cast<char*>(drop_types), const_cast<char*>(data.constData()), data.size(), x, y);
              drop->acceptProposedAction();
              return true;
            }
          }
        }

        if (mime->hasUrls())
        {
          auto cbDropFiles = reinterpret_cast<IFnsiii>(IupGetCallback(ih, "DROPFILES_CB"));
          if (cbDropFiles)
          {
            QList<QUrl> urls = mime->urls();
            int count = urls.size();

            for (int i = 0; i < count; i++)
            {
              QString filePath = urls[i].toLocalFile();
              if (!filePath.isEmpty())
              {
                QByteArray fileArray = filePath.toUtf8();
                if (cbDropFiles(ih, const_cast<char*>(fileArray.constData()), count - i - 1, x, y) == IUP_IGNORE)
                  break;
              }
            }

            drop->acceptProposedAction();
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

        auto* me = static_cast<QMouseEvent*>(event);
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

        auto* me = static_cast<QMouseEvent*>(event);

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
            auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "DRAGBEGIN_CB"));
            if (cb)
            {
              int ret = cb(ih, start_x, start_y);
              if (ret == IUP_IGNORE)
                return false;

              const char* drag_types = iupAttribGetStr(ih, "DRAGTYPES");
              if (drag_types)
              {
                IFns cbDragDataSize = reinterpret_cast<IFns>(IupGetCallback(ih, "DRAGDATASIZE_CB"));
                auto cbDragData = reinterpret_cast<IFnsVi>(IupGetCallback(ih, "DRAGDATA_CB"));

                if (cbDragDataSize && cbDragData)
                {
                  int size = cbDragDataSize(ih, const_cast<char*>(drag_types));
                  if (size > 0)
                  {
                    auto* drag = new QDrag(widget);
                    auto* mime_data = new QMimeData();

                    void* data = malloc(size);
                    cbDragData(ih, const_cast<char*>(drag_types), data, size);

                    QString mime_type = QString("application/x-iup-") + QString::fromUtf8(drag_types).toLower();
                    QByteArray byte_array(static_cast<const char*>(data), size);
                    mime_data->setData(mime_type, byte_array);
                    free(data);

                    drag->setMimeData(mime_data);

                    char* drag_cursor = iupAttribGet(ih, "DRAGCURSOR");
                    if (drag_cursor)
                    {
                      auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(drag_cursor, ih, 0, nullptr));
                      if (pixmap)
                      {
                        drag->setDragCursor(*pixmap, Qt::MoveAction);
                        drag->setDragCursor(*pixmap, Qt::CopyAction);
                      }
                    }
                    char* drag_cursor_copy = iupAttribGet(ih, "DRAGCURSORCOPY");
                    if (drag_cursor_copy)
                    {
                      auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(drag_cursor_copy, ih, 0, nullptr));
                      if (pixmap)
                        drag->setDragCursor(*pixmap, Qt::CopyAction);
                    }

                    Qt::DropActions actions = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ?
                                              Qt::CopyAction | Qt::MoveAction : Qt::CopyAction;

                    dd_data->current_drag = drag;

                    Qt::DropAction result = drag->exec(actions);

                    IFni end_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "DRAGEND_CB"));
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

static int qtDragDropSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  IupQtDragDropData* dd_data = qtDragDropGetData(ih, 1);

  auto* widget = reinterpret_cast<QWidget*>(ih->handle);
  if (!widget)
    return 0;

  int enable = iupStrBoolean(value);

  if (enable && !dd_data->is_source)
  {
    dd_data->is_source = 1;

    auto* filter = new IupQtDragDropFilter(ih);
    widget->installEventFilter(filter);
    iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", reinterpret_cast<char*>(filter));

    auto* view = qobject_cast<QAbstractItemView*>(widget);
    if (view && view->viewport())
    {
      view->viewport()->installEventFilter(filter);

      auto* tree = qobject_cast<QTreeWidget*>(widget);
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

    auto* filter = reinterpret_cast<IupQtDragDropFilter*>(iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER"));
    if (filter && !dd_data->is_target)
    {
      widget->removeEventFilter(filter);
      delete filter;
      iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", nullptr);
    }

    auto* tree = qobject_cast<QTreeWidget*>(widget);
    if (tree && dd_data->is_target)
    {
      tree->setDragDropMode(QAbstractItemView::DropOnly);
    }
  }

  return 1;
}

static int qtDragDropSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  IupQtDragDropData* dd_data = qtDragDropGetData(ih, 1);

  auto* widget = reinterpret_cast<QWidget*>(ih->handle);
  if (!widget)
    return 0;

  int enable = iupStrBoolean(value);

  if (enable && !dd_data->is_target)
  {
    dd_data->is_target = 1;
    widget->setAcceptDrops(true);

    auto* filter = reinterpret_cast<IupQtDragDropFilter*>(iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER"));
    if (!filter)
    {
      filter = new IupQtDragDropFilter(ih);
      widget->installEventFilter(filter);
      iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", reinterpret_cast<char*>(filter));
    }

    auto* view = qobject_cast<QAbstractItemView*>(widget);
    if (view && view->viewport())
    {
      view->viewport()->installEventFilter(filter);

      auto* tree = qobject_cast<QTreeWidget*>(widget);
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
      auto* filter = reinterpret_cast<IupQtDragDropFilter*>(iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER"));
      if (filter)
      {
        widget->removeEventFilter(filter);
        delete filter;
        iupAttribSet(ih, "_IUPQT_DRAGDROP_FILTER", nullptr);
      }
    }

    auto* tree = qobject_cast<QTreeWidget*>(widget);
    if (tree && !dd_data->is_source)
    {
      tree->setDragDropMode(QAbstractItemView::NoDragDrop);
    }
  }

  return 1;
}

static int qtDragDropSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  return qtDragDropSetDropTargetAttrib(ih, value);
}

IUP_DRV_API void iupqtDragDropCleanup(Ihandle* ih)
{
  auto* dd_data = reinterpret_cast<IupQtDragDropData*>(iupAttribGet(ih, "_IUPQT_DRAGDROP_DATA"));

  if (dd_data)
  {
    auto* widget = reinterpret_cast<QWidget*>(ih->handle);
    if (widget)
    {
      auto* filter = reinterpret_cast<IupQtDragDropFilter*>(iupAttribGet(ih, "_IUPQT_DRAGDROP_FILTER"));
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

extern "C" IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES",  nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", nullptr, qtDragDropSetDragSourceAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", nullptr, qtDragDropSetDropTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", nullptr, qtDragDropSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", nullptr, qtDragDropSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
}

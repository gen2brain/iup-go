/** \file
 * \brief Drag and Drop Support - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickWindow>
#include <QDrag>
#include <QPointer>
#include <QPixmap>
#include <QMimeData>
#include <QDragMoveEvent>
#include <QUrl>
#include <QList>
#include <QGuiApplication>

#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_drv.h"
}

#include "iupqml_drv.h"


struct IupQmlDragDropData
{
  int is_source;
  int is_target;
  int is_files_target;
  QDrag* current_drag;
  int last_x;
  int last_y;
};

static IupQmlDragDropData* qmlDragDropGetData(Ihandle* ih, int create)
{
  auto* dd_data = reinterpret_cast<IupQmlDragDropData*>(iupAttribGet(ih, "_IUPQML_DRAGDROP_DATA"));

  if (!dd_data && create)
  {
    dd_data = new IupQmlDragDropData();
    dd_data->is_source = 0;
    dd_data->is_target = 0;
    dd_data->is_files_target = 0;
    dd_data->current_drag = nullptr;
    dd_data->last_x = 0;
    dd_data->last_y = 0;

    iupAttribSet(ih, "_IUPQML_DRAGDROP_DATA", reinterpret_cast<char*>(dd_data));
  }

  return dd_data;
}

static QQuickItem* qmlDragDropGetItem(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    return iupqmlDialogGetContent(ih);
  auto* item = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_EVENT_ITEM"));
  if (item)
    return item;
  return iupqmlGetItem(ih);
}

static QPoint qmlDragDropControlPos(Ihandle* ih, QQuickItem* from, const QPointF& pos)
{
  QQuickItem* base = ih->iclass->nativetype == IUP_TYPEDIALOG ? iupqmlDialogGetContent(ih) : iupqmlGetItem(ih);
  QPointF p = (base && from && base != from) ? base->mapFromItem(from, pos) : pos;
  return {static_cast<int>(p.x()), static_cast<int>(p.y())};
}

static void qmlDragDropGetModifiers(char* status)
{
  Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();
  iupqmlButtonKeySetStatus(mods, Qt::NoButton, 0, status, 0);
}

static QString qmlDragDropMimeType(const QString& type)
{
  return QString("application/x-iup-") + type.toLower();
}

static QStringList qmlDragDropTypeList(const char* types)
{
  QStringList list;
  const QStringList parts = QString::fromUtf8(types ? types : "").split(',');
  for (const QString& part : parts)
  {
    QString type = part.trimmed();
    if (!type.isEmpty())
      list.append(type);
  }
  return list;
}

static QString qmlDragDropMatchType(Ihandle* ih, const QMimeData* mime)
{
  const QStringList types = qmlDragDropTypeList(iupAttribGetStr(ih, "DROPTYPES"));
  for (const QString& type : types)
  {
    if (mime->hasFormat(qmlDragDropMimeType(type)))
      return type;
  }
  return QString();
}

static bool qmlDragDropAccepts(Ihandle* ih, IupQmlDragDropData* dd_data, const QMimeData* mime)
{
  if (dd_data->is_target && !qmlDragDropMatchType(ih, mime).isEmpty())
    return true;
  return dd_data->is_files_target && mime->hasUrls();
}

class IupQmlDragDropFilter : public QObject
{
public:
  Ihandle* ih;

  IupQmlDragDropFilter(QObject* parent, Ihandle* ih_param) : QObject(parent), ih(ih_param) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (!iupObjectCheck(ih))
      return false;

    QQuickItem* item = qobject_cast<QQuickItem*>(obj);
    if (!item)
      return false;

    IupQmlDragDropData* dd_data = qmlDragDropGetData(ih, 0);
    if (!dd_data)
      return false;

    switch (event->type())
    {
      case QEvent::DragEnter:
      case QEvent::DragMove:
      {
        auto* de = static_cast<QDragMoveEvent*>(event);
        if (!qmlDragDropAccepts(ih, dd_data, de->mimeData()))
          return false;

        auto cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "DROPMOTION_CB"));
        if (cb && dd_data->is_target && !qmlDragDropMatchType(ih, de->mimeData()).isEmpty())
        {
          QPoint p = qmlDragDropControlPos(ih, qobject_cast<QQuickItem*>(obj), de->position());
          int x = p.x();
          int y = p.y();
          dd_data->last_x = x;
          dd_data->last_y = y;

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          qmlDragDropGetModifiers(status);

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

      case QEvent::Drop:
      {
        auto* drop = static_cast<QDropEvent*>(event);
        const QMimeData* mime = drop->mimeData();
        if (!qmlDragDropAccepts(ih, dd_data, mime))
          return false;

        QPoint p = qmlDragDropControlPos(ih, qobject_cast<QQuickItem*>(obj), drop->position());
        int x = p.x();
        int y = p.y();
        dd_data->last_x = x;
        dd_data->last_y = y;

        QString type = dd_data->is_target ? qmlDragDropMatchType(ih, mime) : QString();
        if (!type.isEmpty())
        {
          auto cbDropData = reinterpret_cast<IFnsViii>(IupGetCallback(ih, "DROPDATA_CB"));
          if (cbDropData)
          {
            QByteArray type_bytes = type.toUtf8();
            QByteArray data = mime->data(qmlDragDropMimeType(type));
            cbDropData(ih, type_bytes.data(), data.data(), static_cast<int>(data.size()), x, y);
          }
          drop->acceptProposedAction();
          return true;
        }

        if (dd_data->is_files_target && mime->hasUrls())
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

        return false;
      }

      default:
        break;
    }

    return false;
  }
};

static void qmlDragDropStartDrag(Ihandle* ih, QQuickItem* item, int start_x, int start_y)
{
  IupQmlDragDropData* dd_data = qmlDragDropGetData(ih, 0);
  if (!dd_data)
    return;

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "DRAGBEGIN_CB"));
  if (!cb || cb(ih, start_x, start_y) == IUP_IGNORE)
    return;

  IFns cbDragDataSize = reinterpret_cast<IFns>(IupGetCallback(ih, "DRAGDATASIZE_CB"));
  auto cbDragData = reinterpret_cast<IFnsVi>(IupGetCallback(ih, "DRAGDATA_CB"));
  if (!cbDragDataSize || !cbDragData)
    return;

  auto* mime_data = new QMimeData();
  const QStringList types = qmlDragDropTypeList(iupAttribGetStr(ih, "DRAGTYPES"));
  for (const QString& type : types)
  {
    QByteArray type_bytes = type.toUtf8();
    int size = cbDragDataSize(ih, type_bytes.data());
    if (size <= 0)
      continue;

    QByteArray data(size, 0);
    cbDragData(ih, type_bytes.data(), data.data(), size);
    mime_data->setData(qmlDragDropMimeType(type), data);
  }

  if (mime_data->formats().isEmpty())
  {
    delete mime_data;
    return;
  }

  auto* drag = new QDrag(item);
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

  Qt::DropActions actions = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ? Qt::CopyAction | Qt::MoveAction : Qt::CopyAction;

  dd_data->current_drag = drag;
  Qt::DropAction result = drag->exec(actions);

  if (!iupObjectCheck(ih))
    return;

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

  dd_data = qmlDragDropGetData(ih, 0);
  if (dd_data)
    dd_data->current_drag = nullptr;
}

class IupQmlDragSourceFilter : public QObject
{
public:
  Ihandle* ih;
  QPointer<QQuickItem> item;
  bool armed = false;
  int start_x = 0, start_y = 0;
  QPoint start;

  IupQmlDragSourceFilter(QQuickItem* source, Ihandle* ih_param) : QObject(source), ih(ih_param), item(source) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    (void)obj;
    if (!item || !iupObjectCheck(ih))
      return false;

    switch (event->type())
    {
      case QEvent::MouseButtonPress:
      {
        auto* me = static_cast<QMouseEvent*>(event);
        QPointF pos = item->mapFromScene(me->scenePosition());
        armed = me->button() == Qt::LeftButton && item->isVisible() && item->contains(pos) && iupdrvIsActive(ih);
        start_x = static_cast<int>(pos.x());
        start_y = static_cast<int>(pos.y());
        start = qmlDragDropControlPos(ih, item, pos);
        return false;
      }

      case QEvent::MouseMove:
      {
        auto* me = static_cast<QMouseEvent*>(event);
        if (!armed || !(me->buttons() & Qt::LeftButton))
          return false;

        QPointF pos = item->mapFromScene(me->scenePosition());
        int dx = static_cast<int>(pos.x()) - start_x;
        int dy = static_cast<int>(pos.y()) - start_y;
        if (dx * dx + dy * dy <= 25)
          return false;

        armed = false;
        qmlDragDropStartDrag(ih, item, start.x(), start.y());
        return true;
      }

      case QEvent::MouseButtonRelease:
        armed = false;
        return false;

      default:
        return false;
    }
  }
};

static IupQmlDragDropFilter* qmlDragDropEnsureFilter(Ihandle* ih, QQuickItem* item)
{
  auto* filter = reinterpret_cast<IupQmlDragDropFilter*>(iupAttribGet(ih, "_IUPQML_DRAGDROP_FILTER"));
  if (!filter)
  {
    filter = new IupQmlDragDropFilter(item, ih);
    item->installEventFilter(filter);
    iupAttribSet(ih, "_IUPQML_DRAGDROP_FILTER", reinterpret_cast<char*>(filter));
  }
  return filter;
}

static void qmlDragDropRemoveFilter(Ihandle* ih, QQuickItem* item)
{
  auto* filter = reinterpret_cast<IupQmlDragDropFilter*>(iupAttribGet(ih, "_IUPQML_DRAGDROP_FILTER"));
  if (filter)
  {
    if (item)
      item->removeEventFilter(filter);
    delete filter;
    iupAttribSet(ih, "_IUPQML_DRAGDROP_FILTER", nullptr);
  }
}

static void qmlDragDropRemoveSourceFilter(Ihandle* ih, QQuickItem* item)
{
  auto* filter = reinterpret_cast<IupQmlDragSourceFilter*>(iupAttribGet(ih, "_IUPQML_DRAGSOURCE_FILTER"));
  if (!filter)
    return;

  if (item && item->window())
    item->window()->removeEventFilter(filter);
  delete filter;
  iupAttribSet(ih, "_IUPQML_DRAGSOURCE_FILTER", nullptr);
}

static int qmlDragDropSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  IupQmlDragDropData* dd_data = qmlDragDropGetData(ih, 1);

  QQuickItem* item = qmlDragDropGetItem(ih);
  if (!item)
    return 0;

  int enable = iupStrBoolean(value);

  if (enable && !dd_data->is_source)
  {
    QQuickWindow* window = item->window();
    if (!window)
      return 1;

    dd_data->is_source = 1;
    auto* filter = new IupQmlDragSourceFilter(item, ih);
    window->installEventFilter(filter);
    iupAttribSet(ih, "_IUPQML_DRAGSOURCE_FILTER", reinterpret_cast<char*>(filter));
  }
  else if (!enable && dd_data->is_source)
  {
    dd_data->is_source = 0;
    qmlDragDropRemoveSourceFilter(ih, item);
  }

  return 1;
}

static int qmlDragDropUpdateTarget(Ihandle* ih, int* flag, const char* value)
{
  IupQmlDragDropData* dd_data = qmlDragDropGetData(ih, 1);

  QQuickItem* item = qmlDragDropGetItem(ih);
  if (!item)
    return 0;

  int was_target = dd_data->is_target || dd_data->is_files_target;
  *flag = iupStrBoolean(value);
  int is_target = dd_data->is_target || dd_data->is_files_target;

  if (is_target && !was_target)
  {
    item->setFlag(QQuickItem::ItemAcceptsDrops, true);
    qmlDragDropEnsureFilter(ih, item);
  }
  else if (!is_target && was_target)
    qmlDragDropRemoveFilter(ih, item);

  return 1;
}

static int qmlDragDropSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  IupQmlDragDropData* dd_data = qmlDragDropGetData(ih, 1);
  return qmlDragDropUpdateTarget(ih, &dd_data->is_target, value);
}

static int qmlDragDropSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  IupQmlDragDropData* dd_data = qmlDragDropGetData(ih, 1);
  return qmlDragDropUpdateTarget(ih, &dd_data->is_files_target, value);
}

IUP_DRV_API void iupqmlDragDropCleanup(Ihandle* ih)
{
  auto* dd_data = reinterpret_cast<IupQmlDragDropData*>(iupAttribGet(ih, "_IUPQML_DRAGDROP_DATA"));

  if (dd_data)
  {
    qmlDragDropRemoveSourceFilter(ih, qmlDragDropGetItem(ih));
    qmlDragDropRemoveFilter(ih, qmlDragDropGetItem(ih));
    delete dd_data;
    iupAttribSet(ih, "_IUPQML_DRAGDROP_DATA", nullptr);
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
  iupClassRegisterAttribute(ic, "DRAGSOURCE", nullptr, qmlDragDropSetDragSourceAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", nullptr, qmlDragDropSetDropTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", nullptr, qmlDragDropSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", nullptr, qmlDragDropSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
}

/** \file
 * \brief Canvas Control - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QMouseEvent>
#include <QVarLengthArray>
#include <QColor>

#include <cmath>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_canvas.h"
#include "iup_key.h"
}

#include "iupqml_drv.h"

static qreal qmlCanvasPixelRatio(QQuickItem* item)
{
  QQuickWindow* window = item ? item->window() : nullptr;
  return window ? window->effectiveDevicePixelRatio() : qGuiApp->devicePixelRatio();
}

static void qmlCanvasFillBackground(Ihandle* ih, QPixmap* buffer)
{
  unsigned char r = 255, g = 255, b = 255;
  char* bgcolor = iupAttribGet(ih, "BGCOLOR");
  if (!bgcolor || !iupStrToRGB(bgcolor, &r, &g, &b))
    iupStrToRGB(iupBaseNativeParentGetBgColor(ih), &r, &g, &b);
  buffer->fill(QColor(r, g, b));
}

IUP_DRV_API QPixmap* iupqmlCanvasCreateBuffer(Ihandle* ih, QQuickItem* item, int w, int h)
{
  qreal ratio = qmlCanvasPixelRatio(item);
  auto* buffer = new QPixmap(static_cast<int>(std::ceil(w * ratio)), static_cast<int>(std::ceil(h * ratio)));
  buffer->setDevicePixelRatio(ratio);
  qmlCanvasFillBackground(ih, buffer);
  return buffer;
}

IUP_DRV_API int iupqmlCanvasBufferMatches(QPixmap* buffer, QQuickItem* item, int w, int h)
{
  return buffer->devicePixelRatio() == qmlCanvasPixelRatio(item) && buffer->deviceIndependentSize().toSize() == QSize(w, h);
}


/****************************************************************************
 * Canvas Item
 ****************************************************************************/

class IupQmlCanvas : public QQuickItem
{
public:
  Ihandle* ih;
  bool texture_dirty;
  bool needs_action;
  bool action_queued;
  bool buffer_dirty;
  QImage image;

  IupQmlCanvas(Ihandle* handle) : QQuickItem(), ih(handle), texture_dirty(true), needs_action(true), action_queued(false), buffer_dirty(false)
  {
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setFlag(ItemAcceptsInputMethod, true);
    setActiveFocusOnTab(true);
  }

  void itemChange(ItemChange change, const ItemChangeData& value) override
  {
    if (change == ItemDevicePixelRatioHasChanged)
      requestRedraw();
    QQuickItem::itemChange(change, value);
  }

  void requestRedraw()
  {
    needs_action = true;
    if (action_queued)
      return;
    action_queued = true;
    QMetaObject::invokeMethod(this, [this]() {
      action_queued = false;
      runAction();
    }, Qt::QueuedConnection);
  }

  void redrawNow()
  {
    needs_action = true;
    runAction();
  }

  void flushBuffer()
  {
    buffer_dirty = true;
    polish();
    update();
  }

  void runAction()
  {
    if (!ih)
      return;

    int w = static_cast<int>(width()), h = static_cast<int>(height());
    if (w <= 0 || h <= 0)
      return;

    if (iupAttribGet(ih, "_IUPGL_COMPOSITE"))
    {
      IFn cb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
      iupAttribSet(ih, "_IUPGL_IN_DRAW", "1");
      if (cb && !ih->data->inside_resize)
        cb(ih);
      iupAttribSet(ih, "_IUPGL_IN_DRAW", nullptr);
      needs_action = false;

      auto* px = reinterpret_cast<unsigned char*>(iupAttribGet(ih, "_IUPGL_COMPOSITE_PIXELS"));
      int pw = iupAttribGetInt(ih, "_IUPGL_COMPOSITE_W");
      int ph = iupAttribGetInt(ih, "_IUPGL_COMPOSITE_H");
      if (px && pw > 0 && ph > 0)
      {
        image = QImage(px, pw, ph, pw * 4, QImage::Format_ARGB32).copy();
        texture_dirty = true;
        update();
      }
      return;
    }

    auto* buffer = reinterpret_cast<QPixmap*>(iupAttribGet(ih, "_IUPQML_CANVAS_BUFFER"));
    if (buffer && !iupqmlCanvasBufferMatches(buffer, this, w, h))
    {
      delete buffer;
      buffer = nullptr;
      iupAttribSet(ih, "_IUPQML_CANVAS_BUFFER", nullptr);
      needs_action = true;
    }

    if (needs_action)
    {
      IFn cb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
      if (cb && !ih->data->inside_resize)
      {
        needs_action = false;
        iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", 0, 0, w - 1, h - 1);
        cb(ih);
        iupAttribSet(ih, "CLIPRECT", nullptr);
      }
      else
      {
        if (!cb)
          needs_action = false;
        if (!buffer)
        {
          buffer = iupqmlCanvasCreateBuffer(ih, this, w, h);
          iupAttribSet(ih, "_IUPQML_CANVAS_BUFFER", reinterpret_cast<char*>(buffer));
        }
        else if (!cb)
          qmlCanvasFillBackground(ih, buffer);
      }
    }

    buffer_dirty = true;
    polish();
    update();
  }

protected:
  void updatePolish() override
  {
    if (!ih || !buffer_dirty)
      return;

    auto* buffer = reinterpret_cast<QPixmap*>(iupAttribGet(ih, "_IUPQML_CANVAS_BUFFER"));
    if (buffer && !buffer->isNull() && iupqmlCanvasBufferMatches(buffer, this, static_cast<int>(width()), static_cast<int>(height())))
    {
      image = buffer->toImage();
      texture_dirty = true;
      buffer_dirty = false;
    }
  }

  QSGNode* updatePaintNode(QSGNode* old_node, UpdatePaintNodeData*) override
  {
    auto* node = static_cast<QSGSimpleTextureNode*>(old_node);

    if (image.isNull())
    {
      delete node;
      return nullptr;
    }

    if (!node)
    {
      node = new QSGSimpleTextureNode();
      node->setOwnsTexture(true);
      texture_dirty = true;
    }

    if (texture_dirty)
    {
      node->setTexture(window()->createTextureFromImage(image));
      texture_dirty = false;
    }

    node->setRect(boundingRect());
    return node;
  }

  void geometryChange(const QRectF& new_geometry, const QRectF& old_geometry) override
  {
    QQuickItem::geometryChange(new_geometry, old_geometry);

    if (!ih || new_geometry.size() == old_geometry.size())
      return;

    if (width() <= 0 || height() <= 0)
      return;

    auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "RESIZE_CB"));
    if (cb && !ih->data->inside_resize)
    {
      ih->data->inside_resize = 1;
      cb(ih, static_cast<int>(width()), static_cast<int>(height()));
      ih->data->inside_resize = 0;
    }

    requestRedraw();
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    if (ih)
    {
      if (iupAttribGetBoolean(ih, "HTTRANSPARENT"))
      {
        event->ignore();
        return;
      }
      if (iupAttribGetBoolean(ih, "CANFOCUS"))
        forceActiveFocus(Qt::MouseFocusReason);
      iupqmlMouseButtonEvent(event, ih);
    }
    event->accept();
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    if (ih)
      iupqmlMouseButtonEvent(event, ih);
    event->accept();
  }

  void mouseDoubleClickEvent(QMouseEvent* event) override
  {
    if (ih)
      iupqmlMouseButtonEvent(event, ih);
    event->accept();
  }

  void mouseMoveEvent(QMouseEvent* event) override
  {
    if (ih)
      iupqmlMouseMoveEvent(event, ih);
    event->accept();
  }

  void hoverMoveEvent(QHoverEvent* event) override
  {
    if (ih)
    {
      auto cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "MOTION_CB"));
      if (cb)
      {
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupqmlButtonKeySetStatus(event->modifiers(), Qt::NoButton, 0, status, 0);
        cb(ih, static_cast<int>(event->position().x()), static_cast<int>(event->position().y()), status);
      }
    }
  }

  void hoverEnterEvent(QHoverEvent* event) override
  {
    if (ih)
    {
      if (iupAttribGetBoolean(ih, "HTTRANSPARENT"))
      {
        event->ignore();
        return;
      }
      iupqmlEnterLeaveEvent(event, ih);
    }
  }

  void hoverLeaveEvent(QHoverEvent* event) override
  {
    if (ih)
      iupqmlEnterLeaveEvent(event, ih);
  }

  void wheelEvent(QWheelEvent* event) override
  {
    if (!ih)
      return;

    if (iupAttribGetBoolean(ih, "WHEELDROPFOCUS"))
    {
      Ihandle* ih_focus = IupGetFocus();
      if (iupObjectCheck(ih_focus))
        iupAttribSetClassObject(ih_focus, "SHOWDROPDOWN", "NO");
    }

    QPoint angle = event->angleDelta();
    float delta = angle.y() / 120.0f;

    auto cb = reinterpret_cast<IFnfiis>(IupGetCallback(ih, "WHEEL_CB"));
    if (cb)
    {
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupqmlButtonKeySetStatus(event->modifiers(), Qt::NoButton, 0, status, 0);

      cb(ih, delta, static_cast<int>(event->position().x()), static_cast<int>(event->position().y()), status);
      event->accept();
      return;
    }

    if (angle.y() != 0)
    {
      double posy = ih->data->posy;
      posy -= delta * iupAttribGetDouble(ih, "DY") / 10.0;
      IupSetDouble(ih, "POSY", posy);
    }
    else if (angle.x() != 0)
    {
      double posx = ih->data->posx;
      posx -= angle.x() / 120.0 * iupAttribGetDouble(ih, "DX") / 10.0;
      IupSetDouble(ih, "POSX", posx);
    }

    auto scb = reinterpret_cast<IFniff>(IupGetCallback(ih, "SCROLL_CB"));
    if (scb)
    {
      int op = angle.y() > 0 ? IUP_SBUP : IUP_SBDN;
      if (angle.y() == 0)
        op = angle.x() > 0 ? IUP_SBLEFT : IUP_SBRIGHT;
      scb(ih, op, static_cast<float>(ih->data->posx), static_cast<float>(ih->data->posy));
    }
    else if (IupGetCallback(ih, "ACTION"))
      requestRedraw();

    event->accept();
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (ih && iupqmlKeyPressEvent(this, event, ih))
    {
      event->accept();
      return;
    }
    QQuickItem::keyPressEvent(event);
  }

  void keyReleaseEvent(QKeyEvent* event) override
  {
    if (ih && iupqmlKeyReleaseEvent(this, event, ih))
    {
      event->accept();
      return;
    }
    QQuickItem::keyReleaseEvent(event);
  }

  void inputMethodEvent(QInputMethodEvent* event) override
  {
    if (ih && !event->commitString().isEmpty() && IupGetCallback(ih, "TEXTINPUT_CB"))
    {
      if (iupKeyCallTextInputCb(ih, event->commitString().toUtf8().constData()) == IUP_IGNORE)
      {
        event->accept();
        return;
      }
    }
    QQuickItem::inputMethodEvent(event);
  }

  void focusInEvent(QFocusEvent* event) override
  {
    if (ih)
      iupqmlFocusInOutEvent(event, ih);
    QQuickItem::focusInEvent(event);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    if (ih)
      iupqmlFocusInOutEvent(event, ih);
    QQuickItem::focusOutEvent(event);
  }

  void touchEvent(QTouchEvent* touchEvent) override
  {
    if (!ih)
      return;

    auto single_cb = reinterpret_cast<IFniiis>(IupGetCallback(ih, "TOUCH_CB"));
    auto multi_cb = reinterpret_cast<IFniIIII>(IupGetCallback(ih, "MULTITOUCH_CB"));
    if (!single_cb && !multi_cb)
    {
      touchEvent->ignore();
      return;
    }

    const QList<QEventPoint>& points = touchEvent->points();
    int count = points.size();
    if (count == 0)
      return;

    QVarLengthArray<int> ids(count), xs(count), ys(count), states(count);

    for (int i = 0; i < count; i++)
    {
      const QEventPoint& tp = points[i];
      int id = tp.id();
      int x = static_cast<int>(tp.position().x());
      int y = static_cast<int>(tp.position().y());
      bool down = (tp.state() == QEventPoint::Pressed);
      bool up = (tp.state() == QEventPoint::Released);
      ids[i] = id;
      xs[i] = x;
      ys[i] = y;
      states[i] = down ? 'D' : (up ? 'U' : 'M');

      if (single_cb)
      {
        const char* str;
        if (i == 0)
          str = down ? "DOWN-PRIMARY" : (up ? "UP-PRIMARY" : "MOVE-PRIMARY");
        else
          str = down ? "DOWN" : (up ? "UP" : "MOVE");
        if (single_cb(ih, id, x, y, const_cast<char*>(str)) == IUP_CLOSE)
          IupExitLoop();
      }
    }

    if (multi_cb)
    {
      if (multi_cb(ih, count, ids.data(), xs.data(), ys.data(), states.data()) == IUP_CLOSE)
        IupExitLoop();
    }

    touchEvent->accept();
  }
};

/****************************************************************************
 * Canvas Container
 ****************************************************************************/

struct IupQmlCanvasContainer
{
  QQuickItem* wrapper;
  IupQmlCanvas* canvas;
  QQuickItem* border;
  QObject* sb_horiz;
  QObject* sb_vert;
  int updating;
};

static IupQmlCanvasContainer* qmlCanvasGetContainer(Ihandle* ih)
{
  return reinterpret_cast<IupQmlCanvasContainer*>(iupAttribGet(ih, "_IUPQML_CANVAS_CONTAINER"));
}

IUP_DRV_API QQuickItem* iupqmlCanvasGetItem(Ihandle* ih)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (container)
    return container->canvas;
  return nullptr;
}

IUP_DRV_API void iupqmlCanvasRedraw(Ihandle* ih, int now)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (!container)
    return;
  if (now)
    container->canvas->redrawNow();
  else
    container->canvas->requestRedraw();
}

IUP_DRV_API void iupqmlCanvasFlush(Ihandle* ih)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (container)
    container->canvas->flushBuffer();
}

static int qmlCanvasScrollbarVisible(Ihandle* ih, QObject* sb)
{
  return sb && (static_cast<QQuickItem*>(sb))->isVisible() && !iupAttribGetBoolean(ih, sb == qmlCanvasGetContainer(ih)->sb_vert ? "YHIDDEN" : "XHIDDEN");
}

static void qmlCanvasLayoutContainer(Ihandle* ih)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (!container)
    return;

  int w = ih->currentwidth, h = ih->currentheight;
  int sb_size = iupdrvGetScrollbarSize();
  int vert = qmlCanvasScrollbarVisible(ih, container->sb_vert) ? sb_size : 0;
  int horiz = qmlCanvasScrollbarVisible(ih, container->sb_horiz) ? sb_size : 0;

  int cw = w - vert, ch = h - horiz;
  if (cw < 1) cw = 1;
  if (ch < 1) ch = 1;

  container->canvas->setPosition(QPointF(0, 0));
  container->canvas->setSize(QSizeF(cw, ch));

  if (container->border)
  {
    container->border->setPosition(QPointF(0, 0));
    container->border->setSize(QSizeF(cw, ch));
  }

  if (container->sb_vert)
  {
    auto* sb = static_cast<QQuickItem*>(container->sb_vert);
    sb->setPosition(QPointF(cw, 0));
    sb->setSize(QSizeF(sb_size, ch));
  }

  if (container->sb_horiz)
  {
    auto* sb = static_cast<QQuickItem*>(container->sb_horiz);
    sb->setPosition(QPointF(0, ch));
    sb->setSize(QSizeF(cw, sb_size));
  }
}

/****************************************************************************
 * Scrollbar Callbacks
 ****************************************************************************/

static void qmlCanvasScrollCallback(Ihandle* ih, int op)
{
  auto cb = reinterpret_cast<IFniff>(IupGetCallback(ih, "SCROLL_CB"));
  if (cb)
    cb(ih, op, static_cast<float>(ih->data->posx), static_cast<float>(ih->data->posy));
  else if (IupGetCallback(ih, "ACTION"))
    iupdrvRedrawNow(ih);
}

static void qmlCanvasPositionChanged(Ihandle* ih, int is_vert)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (!container || container->updating)
    return;

  QObject* sb = is_vert ? container->sb_vert : container->sb_horiz;
  double position = sb->property("position").toDouble();
  bool pressed = sb->property("pressed").toBool();

  double vmin = iupAttribGetDouble(ih, is_vert ? "YMIN" : "XMIN");
  double vmax = iupAttribGetDouble(ih, is_vert ? "YMAX" : "XMAX");
  double page = iupAttribGetDouble(ih, is_vert ? "DY" : "DX");
  double pos = vmin + position * (vmax - vmin);
  if (pos > vmax - page) pos = vmax - page;
  if (pos < vmin) pos = vmin;

  if (is_vert)
    ih->data->posy = pos;
  else
    ih->data->posx = pos;

  int op;
  if (is_vert)
    op = pressed ? IUP_SBDRAGV : IUP_SBPOSV;
  else
    op = pressed ? IUP_SBDRAGH : IUP_SBPOSH;

  qmlCanvasScrollCallback(ih, op);
}

static void qmlCanvasUpdateScrollbar(Ihandle* ih, int is_vert, const char* page_value)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (!container)
    return;

  QObject* sb = is_vert ? container->sb_vert : container->sb_horiz;
  if (!sb)
    return;

  double vmin = iupAttribGetDouble(ih, is_vert ? "YMIN" : "XMIN");
  double vmax = iupAttribGetDouble(ih, is_vert ? "YMAX" : "XMAX");
  double page = 0;
  if (!page_value || !iupStrToDouble(page_value, &page))
    page = iupAttribGetDouble(ih, is_vert ? "DY" : "DX");
  double pos = is_vert ? ih->data->posy : ih->data->posx;
  double range = vmax - vmin;
  const char* hidden_attr = is_vert ? "YHIDDEN" : "XHIDDEN";
  const char* autohide_attr = is_vert ? "YAUTOHIDE" : "XAUTOHIDE";

  container->updating = 1;

  if (range <= 0 || page >= range)
  {
    if (iupAttribGetBoolean(ih, autohide_attr))
    {
      iupAttribSet(ih, hidden_attr, "YES");
      (static_cast<QQuickItem*>(sb))->setVisible(false);
    }
    else
    {
      iupAttribSet(ih, hidden_attr, "NO");
      (static_cast<QQuickItem*>(sb))->setVisible(true);
      sb->setProperty("size", 1.0);
      sb->setProperty("position", 0.0);
      (static_cast<QQuickItem*>(sb))->setEnabled(false);
    }

    if (is_vert)
      ih->data->posy = vmin;
    else
      ih->data->posx = vmin;
  }
  else
  {
    iupAttribSet(ih, hidden_attr, "NO");
    (static_cast<QQuickItem*>(sb))->setVisible(true);
    (static_cast<QQuickItem*>(sb))->setEnabled(true);

    double line = iupAttribGetDouble(ih, is_vert ? "LINEY" : "LINEX");
    if (line == 0)
      line = page / 10.0;

    sb->setProperty("size", page / range);
    sb->setProperty("stepSize", line / range);
    sb->setProperty("position", (pos - vmin) / range);
  }

  container->updating = 0;

  if (ih->handle)
    qmlCanvasLayoutContainer(ih);
}

static QObject* qmlCanvasCreateScrollbar(Ihandle* ih, int is_vert, QQuickItem* parent)
{
  QQuickItem* sb = iupqmlCreateItem(is_vert ?
    IUPQML_IMPORTS "ScrollBar { orientation: Qt.Vertical; policy: ScrollBar.AlwaysOn; interactive: true }" :
    IUPQML_IMPORTS "ScrollBar { orientation: Qt.Horizontal; policy: ScrollBar.AlwaysOn; interactive: true }");
  if (!sb)
    return nullptr;

  sb->setParentItem(parent);
  sb->setParent(parent);
  sb->setProperty("active", true);

  iupqmlConnect(sb, "positionChanged()", [ih, is_vert](void**) {
    qmlCanvasPositionChanged(ih, is_vert);
  });

  return sb;
}

/****************************************************************************
 * Attributes
 ****************************************************************************/

static int qmlCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
    qmlCanvasUpdateScrollbar(ih, 0, value);
  return 1;
}

static int qmlCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
    qmlCanvasUpdateScrollbar(ih, 1, value);
  return 1;
}

static int qmlCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double posx;
    if (!iupStrToDouble(value, &posx))
      return 1;

    double xmin = iupAttribGetDouble(ih, "XMIN");
    double xmax = iupAttribGetDouble(ih, "XMAX");
    double dx = iupAttribGetDouble(ih, "DX");

    if (dx >= xmax - xmin)
      return 0;

    if (posx < xmin) posx = xmin;
    if (posx > (xmax - dx)) posx = xmax - dx;
    ih->data->posx = posx;

    qmlCanvasUpdateScrollbar(ih, 0, nullptr);
  }
  return 1;
}

static int qmlCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double posy;
    if (!iupStrToDouble(value, &posy))
      return 1;

    double ymin = iupAttribGetDouble(ih, "YMIN");
    double ymax = iupAttribGetDouble(ih, "YMAX");
    double dy = iupAttribGetDouble(ih, "DY");

    if (dy >= ymax - ymin)
      return 0;

    if (posy < ymin) posy = ymin;
    if (posy > (ymax - dy)) posy = ymax - dy;
    ih->data->posy = posy;

    qmlCanvasUpdateScrollbar(ih, 1, nullptr);
  }
  return 1;
}

static int qmlCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  unsigned char r, g, b;

  if (container && iupStrToRGB(value, &r, &g, &b))
  {
    auto* buffer = reinterpret_cast<QPixmap*>(iupAttribGet(ih, "_IUPQML_CANVAS_BUFFER"));
    if (buffer)
      buffer->fill(QColor(r, g, b));
    container->canvas->requestRedraw();
    return 1;
  }

  return 0;
}

static int qmlCanvasSetBorderAttrib(Ihandle* ih, const char* value)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (!container)
    return 0;

  if (iupStrBoolean(value))
  {
    if (!container->border)
    {
      container->border = iupqmlCreateItem("import QtQuick\nRectangle { color: \"transparent\"; border.width: 1; border.color: palette.mid; z: 1 }");
      if (container->border)
        container->border->setParentItem(container->wrapper);
      qmlCanvasLayoutContainer(ih);
    }
    else
      container->border->setVisible(true);
  }
  else if (container->border)
    container->border->setVisible(false);

  return 1;
}

static char* qmlCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (container)
    return iupStrReturnIntInt(static_cast<int>(container->canvas->width()), static_cast<int>(container->canvas->height()), 'x');
  return nullptr;
}

static char* qmlCanvasGetDrawableAttrib(Ihandle* ih)
{
  return reinterpret_cast<char*>(iupqmlCanvasGetItem(ih));
}

static char* qmlCanvasGetScrollVisibleAttrib(Ihandle* ih)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (!container)
    return nullptr;

  int horiz_visible = qmlCanvasScrollbarVisible(ih, container->sb_horiz);
  int vert_visible = qmlCanvasScrollbarVisible(ih, container->sb_vert);

  if (horiz_visible && vert_visible)
    return const_cast<char*>("YES");
  else if (horiz_visible)
    return const_cast<char*>("HORIZONTAL");
  else if (vert_visible)
    return const_cast<char*>("VERTICAL");
  else
    return const_cast<char*>("NO");
}

static int qmlCanvasSetUpdateRectAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  iupdrvPostRedraw(ih);
  return 0;
}

static int qmlCanvasSetHTTransparentAttrib(Ihandle* ih, const char* value)
{
  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (container)
  {
    int transparent = iupStrBoolean(value);
    container->canvas->setAcceptedMouseButtons(transparent ? Qt::NoButton : Qt::AllButtons);
    container->canvas->setAcceptHoverEvents(transparent ? false : true);
  }
  return 1;
}

static const char* qmlCanvasGestureQml =
  "import QtQuick\n"
  "Item {\n"
  "  id: g\n"
  "  anchors.fill: parent\n"
  "  signal iupGesture(int gesture, int state, real x, real y, real v1, real v2)\n"
  "  PinchHandler {\n"
  "    id: pinch\n"
  "    target: null\n"
  "    onActiveChanged: { g.iupGesture(0, active ? 0 : 2, centroid.position.x, centroid.position.y, activeScale, 0); g.iupGesture(1, active ? 0 : 2, centroid.position.x, centroid.position.y, activeRotation, 0) }\n"
  "    onActiveScaleChanged: if (active) g.iupGesture(0, 1, centroid.position.x, centroid.position.y, activeScale, 0)\n"
  "    onActiveRotationChanged: if (active) g.iupGesture(1, 1, centroid.position.x, centroid.position.y, activeRotation, 0)\n"
  "  }\n"
  "  DragHandler {\n"
  "    id: pan\n"
  "    target: null\n"
  "    acceptedDevices: PointerDevice.TouchScreen\n"
  "    onActiveChanged: g.iupGesture(2, active ? 0 : 2, centroid.position.x, centroid.position.y, activeTranslation.x, activeTranslation.y)\n"
  "    onActiveTranslationChanged: if (active) g.iupGesture(2, 1, centroid.position.x, centroid.position.y, activeTranslation.x, activeTranslation.y)\n"
  "  }\n"
  "  TapHandler {\n"
  "    acceptedDevices: PointerDevice.TouchScreen\n"
  "    onTapped: (eventPoint) => g.iupGesture(4, 2, eventPoint.position.x, eventPoint.position.y, 1, 0)\n"
  "    onLongPressed: g.iupGesture(5, 2, point.position.x, point.position.y, 0, 0)\n"
  "  }\n"
  "}\n";

static void qmlCanvasGesture(Ihandle* ih, int gesture, int state, double x, double y, double v1, double v2)
{
  auto cb = reinterpret_cast<IFniiiidd>(IupGetCallback(ih, "GESTURE_CB"));
  if (cb && cb(ih, gesture, state, static_cast<int>(x), static_cast<int>(y), v1, v2) == IUP_CLOSE)
    IupExitLoop();
}

static void* qmlCanvasGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupqmlCanvasGetItem(ih);
}

/****************************************************************************
 * Map / UnMap / Layout
 ****************************************************************************/

static int qmlCanvasMapMethod(Ihandle* ih)
{
  auto* container = new IupQmlCanvasContainer();
  memset(container, 0, sizeof(IupQmlCanvasContainer));

  container->wrapper = iupqmlCreateItem("import QtQuick\nItem { clip: true }");
  if (!container->wrapper)
  {
    delete container;
    return IUP_ERROR;
  }

  container->canvas = new IupQmlCanvas(ih);
  container->canvas->setParentItem(container->wrapper);
  container->canvas->setParent(container->wrapper);
  iupAttribSet(ih, "_IUPQML_EVENT_ITEM", reinterpret_cast<char*>(container->canvas));

  ih->data->sb = iupBaseGetScrollbar(ih);

  if (ih->data->sb & IUP_SB_VERT)
    container->sb_vert = qmlCanvasCreateScrollbar(ih, 1, container->wrapper);
  if (ih->data->sb & IUP_SB_HORIZ)
    container->sb_horiz = qmlCanvasCreateScrollbar(ih, 0, container->wrapper);

  iupAttribSet(ih, "_IUPQML_CANVAS_CONTAINER", reinterpret_cast<char*>(container));

  ih->handle = reinterpret_cast<InativeHandle*>(container->canvas);
  iupAttribSet(ih, "_IUP_EXTRAPARENT", reinterpret_cast<char*>(container->wrapper));

  iupqmlSetIhandle(container->canvas, ih);
  iupqmlSetIhandle(container->wrapper, ih);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  iupqmlAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    container->canvas->setActiveFocusOnTab(false);

  if (IupGetCallback(ih, "GESTURE_CB"))
  {
    QQuickItem* gestures = iupqmlCreateItem(qmlCanvasGestureQml);
    if (gestures)
    {
      gestures->setParentItem(container->canvas);
      gestures->setParent(container->canvas);
      iupqmlConnect(gestures, "iupGesture(int,int,double,double,double,double)", [ih](void** args) {
        qmlCanvasGesture(ih, *static_cast<int*>(args[1]), *static_cast<int*>(args[2]), *static_cast<double*>(args[3]), *static_cast<double*>(args[4]), *static_cast<double*>(args[5]), *static_cast<double*>(args[6]));
      });
    }
  }

  qmlCanvasSetDXAttrib(ih, nullptr);
  qmlCanvasSetDYAttrib(ih, nullptr);

  return IUP_NOERROR;
}

static void qmlCanvasUnMapMethod(Ihandle* ih)
{
  auto* buffer = reinterpret_cast<QPixmap*>(iupAttribGet(ih, "_IUPQML_CANVAS_BUFFER"));
  if (buffer)
  {
    delete buffer;
    iupAttribSet(ih, "_IUPQML_CANVAS_BUFFER", nullptr);
  }

  IupQmlCanvasContainer* container = qmlCanvasGetContainer(ih);
  if (container)
  {
    container->canvas->ih = nullptr;
    delete container;
    iupAttribSet(ih, "_IUPQML_CANVAS_CONTAINER", nullptr);
  }

  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

static void qmlCanvasLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  qmlCanvasLayoutContainer(ih);
}

/****************************************************************************
 * Canvas Driver Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = qmlCanvasMapMethod;
  ic->UnMap = qmlCanvasUnMapMethod;
  ic->LayoutUpdate = qmlCanvasLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = qmlCanvasGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlCanvasSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BORDER", nullptr, qmlCanvasSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DRAWSIZE", qmlCanvasGetDrawSizeAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWABLE", qmlCanvasGetDrawableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", nullptr, qmlCanvasSetDXAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", nullptr, qmlCanvasSetDYAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, qmlCanvasSetPosXAttrib, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, qmlCanvasSetPosYAttrib, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMIN", nullptr, nullptr, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMAX", nullptr, nullptr, "1", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMIN", nullptr, nullptr, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMAX", nullptr, nullptr, "1", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEX", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEY", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XAUTOHIDE", nullptr, nullptr, "YES", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YAUTOHIDE", nullptr, nullptr, "YES", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XHIDDEN", nullptr, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YHIDDEN", nullptr, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", qmlCanvasGetScrollVisibleAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "TOUCH_CB", "iiis");
  iupClassRegisterCallback(ic, "MULTITOUCH_CB", "iIII");
  iupClassRegisterCallback(ic, "GESTURE_CB", "iiiidd");

  iupClassRegisterAttribute(ic, iupqmlGetNativeWindowHandleName(), iupqmlGetNativeWindowHandleAttrib, nullptr, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TOUCH", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKINGSTORE", nullptr, nullptr, "YES", nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HTTRANSPARENT", nullptr, qmlCanvasSetHTTransparentAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WHEELDROPFOCUS", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPRECT", nullptr, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UPDATERECT", nullptr, qmlCanvasSetUpdateRectAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
}

/** \file
 * \brief Canvas Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QWindow>
#include <QSurface>
#include <QPainter>
#include <QScrollBar>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTouchEvent>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Custom Canvas Widget Class
 ****************************************************************************/

class IupQtCanvas : public QWidget
{
public:
  Ihandle* ih;

  IupQtCanvas(QWidget* parent = nullptr) : QWidget(parent), ih(nullptr)
  {
    /* Enable mouse tracking for motion events */
    setMouseTracking(true);

    /* Accept focus */
    setFocusPolicy(Qt::StrongFocus);

    /* Set size policy */
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /* Accept drops for DROPFILES_CB */
    setAcceptDrops(true);

    /* Use opaque painting - we handle all drawing */
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    /* Enable touch events */
    setAttribute(Qt::WA_AcceptTouchEvents, true);
  }

  /* Override paintEngine() to return nullptr for OpenGL canvases */
  QPaintEngine* paintEngine() const override
  {
    if (ih && iupAttribGet(ih, "_IUP_GLCONTROLDATA"))
    {
      /* For OpenGL canvases, return nullptr - we're using EGL, not Qt's paint system */
      return nullptr;
    }
    /* For normal canvases, use the default paint engine */
    return QWidget::paintEngine();
  }

protected:
  void paintEvent(QPaintEvent* event) override
  {
    if (!ih)
      return;

    /* For EGL/OpenGL canvases, ACTION callback handles everything */
    if (iupAttribGet(ih, "_IUP_GLCONTROLDATA"))
    {
      IFn cb = (IFn)IupGetCallback(ih, "ACTION");
      if (cb && !(ih->data->inside_resize))
      {
        iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d",
                         event->rect().left(), event->rect().top(),
                         event->rect().right(), event->rect().bottom());
        cb(ih);
        iupAttribSet(ih, "CLIPRECT", NULL);
      }
      event->accept();
      return;
    }

    /* Qt: First, copy persistent buffer to screen if it exists (for SCROLL_CB drawings) */
    QPixmap* buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_CANVAS_BUFFER");
    if (buffer && !buffer->isNull())
    {
      QPainter painter(this);
      painter.drawPixmap(0, 0, *buffer);
      /* Qt: Buffer already contains drawing from SCROLL_CB, don't call ACTION callback
       * because it would redraw at position 0,0 and overwrite the scrolled content */
      event->accept();
      return;
    }

    /* No buffer - call ACTION callback to draw */
    IFn cb = (IFn)IupGetCallback(ih, "ACTION");
    if (cb && !(ih->data->inside_resize))
    {
      iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d",
                       event->rect().left(), event->rect().top(),
                       event->rect().right(), event->rect().bottom());

      cb(ih);

      iupAttribSet(ih, "CLIPRECT", NULL);

      /* After ACTION draws to buffer, display it */
      buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_CANVAS_BUFFER");
      if (buffer && !buffer->isNull())
      {
        QPainter painter(this);
        painter.drawPixmap(0, 0, *buffer);
      }
    }
    else
    {
      /* Fallback for regular canvas with no buffer/action */
      QPainter painter(this);
      painter.fillRect(rect(), Qt::white);
    }

    event->accept();
  }

  void resizeEvent(QResizeEvent* event) override
  {
    QWidget::resizeEvent(event);

    if (!ih)
      return;

    /* Qt6: Ignore spurious resize events with invalid (0x0) size that occur during window initialization.
     * These cause the EGL window to collapse to 1x1 minimum size.
     * Only process resize events with valid dimensions. */
    if (event->size().width() <= 0 || event->size().height() <= 0)
      return;

    /* Invalidate the buffer when canvas size changes - forces ACTION callback to redraw at new size */
    QPixmap* buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_CANVAS_BUFFER");
    if (buffer)
    {
      delete buffer;
      iupAttribSet(ih, "_IUPQT_CANVAS_BUFFER", NULL);
    }

    IFnii cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (cb && !ih->data->inside_resize)
    {
      ih->data->inside_resize = 1;
      cb(ih, event->size().width(), event->size().height());
      ih->data->inside_resize = 0;
    }

    /* Trigger repaint after resize */
    update();
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    if (ih)
      iupqtMouseButtonEvent(this, event, ih);
    QWidget::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    if (ih)
      iupqtMouseButtonEvent(this, event, ih);
    QWidget::mouseReleaseEvent(event);
  }

  void mouseMoveEvent(QMouseEvent* event) override
  {
    if (ih)
      iupqtMouseMoveEvent(this, event, ih);
    QWidget::mouseMoveEvent(event);
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

    IFnfiis cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
    if (cb)
    {
      QPoint numDegrees = event->angleDelta() / 8;
      int delta = numDegrees.y() / 15;  /* Number of notches */

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      int x = event->position().x();
      int y = event->position().y();
#else
      int x = event->x();
      int y = event->y();
#endif

      char status[IUPKEY_STATUS_SIZE];
      iupqtButtonKeySetStatus(event->modifiers(), Qt::NoButton, 0, status, 0);

      cb(ih, (float)delta, x, y, status);
      event->accept();
      return;
    }

    QWidget::wheelEvent(event);
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (ih && iupqtKeyPressEvent(this, event, ih))
    {
      event->accept();
      return;
    }
    QWidget::keyPressEvent(event);
  }

  void keyReleaseEvent(QKeyEvent* event) override
  {
    if (ih && iupqtKeyReleaseEvent(this, event, ih))
    {
      event->accept();
      return;
    }
    QWidget::keyReleaseEvent(event);
  }

  void focusInEvent(QFocusEvent* event) override
  {
    if (ih)
      iupqtFocusInOutEvent(this, event, ih);
    QWidget::focusInEvent(event);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    if (ih)
      iupqtFocusInOutEvent(this, event, ih);
    QWidget::focusOutEvent(event);
  }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    if (ih)
      iupqtEnterLeaveEvent(this, event, ih);
    QWidget::enterEvent(event);
  }

  void leaveEvent(QEvent* event) override
  {
    if (ih)
      iupqtEnterLeaveEvent(this, event, ih);
    QWidget::leaveEvent(event);
  }

  void dragEnterEvent(QDragEnterEvent* event) override
  {
    if (!ih)
      return;

    if (IupGetCallback(ih, "DROPFILES_CB"))
    {
      /* Accept drops if they contain URLs (files) */
      if (event->mimeData()->hasUrls())
      {
        event->acceptProposedAction();
        return;
      }
    }

    QWidget::dragEnterEvent(event);
  }

  void dropEvent(QDropEvent* event) override
  {
    if (!ih)
      return;

    IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
    if (cb && event->mimeData()->hasUrls())
    {
      QList<QUrl> urls = event->mimeData()->urls();
      int count = urls.size();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      int x = event->position().x();
      int y = event->position().y();
#else
      int x = event->pos().x();
      int y = event->pos().y();
#endif

      QString fileList;
      for (int i = 0; i < count; i++)
      {
        if (i > 0)
          fileList += "\n";
        fileList += urls[i].toLocalFile();
      }

      QByteArray fileArray = fileList.toUtf8();
      cb(ih, (char*)fileArray.constData(), count, x, y);

      event->acceptProposedAction();
      return;
    }

    QWidget::dropEvent(event);
  }

  bool event(QEvent* event) override
  {
    if (!ih)
      return QWidget::event(event);

    if (event->type() == QEvent::TouchBegin ||
        event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd)
    {
      IFniiiis cb = (IFniiiis)IupGetCallback(ih, "TOUCH_CB");
      if (cb)
      {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QList<QEventPoint>& touchPoints = touchEvent->points();
#else
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
#endif

        if (!touchPoints.isEmpty())
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          const QEventPoint& tp = touchPoints.first();
#else
          const QTouchEvent::TouchPoint& tp = touchPoints.first();
#endif
          int id = tp.id();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          int x = tp.position().x();
          int y = tp.position().y();
#else
          int x = tp.pos().x();
          int y = tp.pos().y();
#endif

          char status[IUPKEY_STATUS_SIZE];
          iupqtButtonKeySetStatus(touchEvent->modifiers(), Qt::NoButton, 0, status, 0);

          /* Convert touch state to IUP state */
          int state = 0;
          if (event->type() == QEvent::TouchBegin)
            state = 0; /* pressed */
          else if (event->type() == QEvent::TouchEnd)
            state = 1; /* released */
          else
            state = 2; /* moved */

          cb(ih, id, x, y, state, status);
          event->accept();
          return true;
        }
      }
    }

    if (event->type() == QEvent::HoverMove || event->type() == QEvent::HoverEnter)
    {
      if (iupAttribGetBoolean(ih, "HTTRANSPARENT"))
      {
        event->ignore();
        return false;
      }
    }

    return QWidget::event(event);
  }
};

/****************************************************************************
 * Canvas Container Structure
 ****************************************************************************/

struct IupQtCanvasContainer
{
  QWidget* container;      /* Main container widget */
  IupQtCanvas* canvas;     /* Canvas widget */
  QScrollBar* sb_horiz;    /* Horizontal scrollbar */
  QScrollBar* sb_vert;     /* Vertical scrollbar */
};

/****************************************************************************
 * Get Canvas Container
 ****************************************************************************/

static IupQtCanvasContainer* qtCanvasGetContainer(Ihandle* ih)
{
  return (IupQtCanvasContainer*)iupAttribGet(ih, "_IUPQT_CANVAS_CONTAINER");
}

/****************************************************************************
 * Scrollbar Callbacks
 ****************************************************************************/

static void qtCanvasScrollCallback(Ihandle* ih, QScrollBar* scrollbar, int orientation, int op)
{
  if (!ih)
    return;

  IFniff cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (cb)
  {
    float posx = (float)ih->data->posx;
    float posy = (float)ih->data->posy;

    cb(ih, op, posx, posy);
  }
  else
  {
    /* If no SCROLL_CB, trigger ACTION callback with full redraw */
    IFn action_cb = (IFn)IupGetCallback(ih, "ACTION");
    if (action_cb)
    {
      action_cb(ih);
      iupdrvRedrawNow(ih);
    }
  }
}

static void qtCanvasProcessScroll(Ihandle* ih, QScrollBar* scrollbar, int orientation)
{
  if (!ih || !scrollbar)
    return;

  double xmin, xmax, ymin, ymax, dx, dy;
  double *pos_ptr, line_val;
  int op = IUP_SBPOSH;  /* default */
  int is_vert = (orientation == Qt::Vertical);

  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");
  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");
  dx = iupAttribGetDouble(ih, "DX");
  dy = iupAttribGetDouble(ih, "DY");

  if (is_vert)
  {
    pos_ptr = &ih->data->posy;
    line_val = iupAttribGetDouble(ih, "LINEY");
    if (line_val == 0)
      line_val = dy / 10.0;
    op = IUP_SBPOSV;
  }
  else
  {
    pos_ptr = &ih->data->posx;
    line_val = iupAttribGetDouble(ih, "LINEX");
    if (line_val == 0)
      line_val = dx / 10.0;
    op = IUP_SBPOSH;
  }

  /* Calculate new position from scrollbar */
  int scrollbar_val = scrollbar->value();
  int scrollbar_max = scrollbar->maximum();

  double range = is_vert ? (ymax - ymin - dy) : (xmax - xmin - dx);
  double min_val = is_vert ? ymin : xmin;

  if (scrollbar_max > 0)
  {
    *pos_ptr = min_val + (range * scrollbar_val / scrollbar_max);
  }
  else
  {
    *pos_ptr = min_val;
  }

  /* Call scroll callback */
  qtCanvasScrollCallback(ih, scrollbar, orientation, op);
}

/****************************************************************************
 * Scrollbar DX/DY Attributes
 ****************************************************************************/

static int qtCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double dx;
    if (!iupStrToDoubleDef(value, &dx, 0.1))
      return 1;

    IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);
    if (container_data && container_data->sb_horiz)
    {
      double xmin = iupAttribGetDouble(ih, "XMIN");
      double xmax = iupAttribGetDouble(ih, "XMAX");
      double linex = iupAttribGetDouble(ih, "LINEX");
      if (linex == 0) linex = dx / 10.0;
      if (linex == 0) linex = 1;

      QScrollBar* sb = container_data->sb_horiz;

      if (dx >= (xmax - xmin))
      {
        if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
        {
          sb->hide();
          iupAttribSet(ih, "SB_RESIZE", "YES");
          iupAttribSet(ih, "XHIDDEN", "YES");
        }
        else
        {
          sb->setEnabled(false);
        }
        ih->data->posx = xmin;
      }
      else
      {
        sb->show();
        iupAttribSet(ih, "SB_RESIZE", "YES");
        sb->setEnabled(true);

        int range = (int)(xmax - xmin - dx);
        if (range < 0) range = 0;
        sb->setRange(0, range);
        sb->setPageStep((int)dx);
        sb->setSingleStep((int)linex);

        iupAttribSet(ih, "XHIDDEN", "NO");
      }
    }
  }
  return 1;
}

static int qtCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double dy;
    if (!iupStrToDoubleDef(value, &dy, 0.1))
      return 1;

    IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);
    if (container_data && container_data->sb_vert)
    {
      double ymin = iupAttribGetDouble(ih, "YMIN");
      double ymax = iupAttribGetDouble(ih, "YMAX");
      double liney = iupAttribGetDouble(ih, "LINEY");
      if (liney == 0) liney = dy / 10.0;
      if (liney == 0) liney = 1;

      QScrollBar* sb = container_data->sb_vert;

      if (dy >= (ymax - ymin))
      {
        if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
        {
          sb->hide();
          iupAttribSet(ih, "SB_RESIZE", "YES");
          iupAttribSet(ih, "YHIDDEN", "YES");
        }
        else
        {
          sb->setEnabled(false);
        }
        ih->data->posy = ymin;
      }
      else
      {
        sb->show();
        iupAttribSet(ih, "SB_RESIZE", "YES");
        sb->setEnabled(true);

        int range = (int)(ymax - ymin - dy);
        if (range < 0) range = 0;
        sb->setRange(0, range);
        sb->setPageStep((int)dy);
        sb->setSingleStep((int)liney);

        iupAttribSet(ih, "YHIDDEN", "NO");
      }
    }
  }
  return 1;
}

/****************************************************************************
 * Map Canvas to Native Control
 ****************************************************************************/

static int qtCanvasMapMethod(Ihandle* ih)
{
  IupQtCanvasContainer* container_data = new IupQtCanvasContainer();

  /* Create extra parent for absolute positioning of IUP children.
     This widget has NO layout, allowing child widgets to be positioned with move(). */
  QWidget* extra_parent = iupqtNativeContainerNew(0);

  /* Create main container widget for canvas and its scrollbars */
  QWidget* container = new QWidget(extra_parent);
  container_data->container = container;

  /* IupQtCanvas handles ACTION callback and paintEvent for both */
  IupQtCanvas* canvas = new IupQtCanvas();
  canvas->ih = ih;
  container_data->canvas = canvas;
  QWidget* canvas_widget = canvas;

  if (iupAttribGet(ih, "_IUP_GLCONTROLDATA"))
  {
    canvas->setAttribute(Qt::WA_NoSystemBackground, true);
  }

  /* Get scrollbar flags, required for DX/DY attribute setters */
  ih->data->sb = iupBaseGetScrollbar(ih);

  int has_sb = (ih->data->sb != IUP_SB_NONE);

  if (has_sb)
  {
    /* Create layout */
    QVBoxLayout* vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    hbox->addWidget(canvas_widget);

    /* Vertical scrollbar */
    if (ih->data->sb & IUP_SB_VERT)
    {
      QScrollBar* sb_vert = new QScrollBar(Qt::Vertical);
      container_data->sb_vert = sb_vert;
      hbox->addWidget(sb_vert);

      QObject::connect(sb_vert, &QScrollBar::valueChanged, [ih, sb_vert]() {
        qtCanvasProcessScroll(ih, sb_vert, Qt::Vertical);
      });
    }
    else
    {
      container_data->sb_vert = nullptr;
    }

    vbox->addLayout(hbox);

    /* Horizontal scrollbar */
    if (ih->data->sb & IUP_SB_HORIZ)
    {
      QScrollBar* sb_horiz = new QScrollBar(Qt::Horizontal);
      container_data->sb_horiz = sb_horiz;
      vbox->addWidget(sb_horiz);

      QObject::connect(sb_horiz, &QScrollBar::valueChanged, [ih, sb_horiz]() {
        qtCanvasProcessScroll(ih, sb_horiz, Qt::Horizontal);
      });
    }
    else
    {
      container_data->sb_horiz = nullptr;
    }
  }
  else
  {
    /* No scrollbars - simple layout */
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(canvas_widget);

    container_data->sb_horiz = nullptr;
    container_data->sb_vert = nullptr;
  }

  iupAttribSet(ih, "_IUPQT_CANVAS_CONTAINER", (char*)container_data);

  /* ih->handle is the container with layout (for canvas drawing)
     _IUP_EXTRAPARENT is the outer container without layout (for absolute child positioning) */
  ih->handle = (InativeHandle*)container;
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)extra_parent);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  iupqtAddToParent(ih);

  /* Initialize scrollbar visibility based on default DX/DY values */
  qtCanvasSetDXAttrib(ih, NULL);
  qtCanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

/****************************************************************************
 * Unmap Canvas
 ****************************************************************************/

static void qtCanvasUnMapMethod(Ihandle* ih)
{
  /* Clean up the persistent buffer */
  QPixmap* buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_CANVAS_BUFFER");
  if (buffer)
  {
    delete buffer;
    iupAttribSet(ih, "_IUPQT_CANVAS_BUFFER", nullptr);
  }

  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (container_data)
  {
    if (container_data->canvas)
    {
      container_data->canvas->ih = nullptr;
    }

    delete container_data;
    iupAttribSet(ih, "_IUPQT_CANVAS_CONTAINER", nullptr);
  }

  /* Delete the extra parent (which will also delete ih->handle as its child) */
  QWidget* extra_parent = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (extra_parent)
  {
    delete extra_parent;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", nullptr);
    ih->handle = nullptr;
  }
  else if (ih->handle)
  {
    QWidget* widget = (QWidget*)ih->handle;
    delete widget;
    ih->handle = nullptr;
  }
}

static void qtCanvasLayoutUpdateMethod(Ihandle* ih)
{
  /* First call the base layout update to position the extra_parent */
  iupdrvBaseLayoutUpdateMethod(ih);

  /* Now resize the container (ih->handle) to fill the extra_parent */
  QWidget* container = (QWidget*)ih->handle;
  if (container)
  {
    container->move(0, 0);
    container->resize(ih->currentwidth, ih->currentheight);
  }
}

/****************************************************************************
 * Update Canvas
 ****************************************************************************/

void qtCanvasUpdate(Ihandle* ih)
{
  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (!container_data || !container_data->canvas)
    return;

  container_data->canvas->update();
}

/****************************************************************************
 * Get Canvas Native Graphics Context
 ****************************************************************************/

extern "C" void* iupqtCanvasGetContext(Ihandle* ih)
{
  /* Return the IupQtCanvas widget for both normal canvas and GLCanvas */
  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (container_data && container_data->canvas)
  {
    return (void*)container_data->canvas;
  }

  return nullptr;
}

/****************************************************************************
 * Scrollbar Management
 ****************************************************************************/

void qtCanvasUpdateScrollPos(Ihandle* ih, float posx, float posy)
{
  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (!container_data)
    return;

  double xmin = iupAttribGetDouble(ih, "XMIN");
  double xmax = iupAttribGetDouble(ih, "XMAX");
  double ymin = iupAttribGetDouble(ih, "YMIN");
  double ymax = iupAttribGetDouble(ih, "YMAX");
  double dx = iupAttribGetDouble(ih, "DX");
  double dy = iupAttribGetDouble(ih, "DY");

  if (container_data->sb_horiz)
  {
    int max = container_data->sb_horiz->maximum();
    double range = xmax - xmin - dx;
    int pos = 0;
    if (range > 0)
      pos = (int)((posx - xmin) * max / range);

    container_data->sb_horiz->blockSignals(true);
    container_data->sb_horiz->setValue(pos);
    container_data->sb_horiz->blockSignals(false);
  }

  if (container_data->sb_vert)
  {
    int max = container_data->sb_vert->maximum();
    double range = ymax - ymin - dy;
    int pos = 0;
    if (range > 0)
      pos = (int)((posy - ymin) * max / range);

    container_data->sb_vert->blockSignals(true);
    container_data->sb_vert->setValue(pos);
    container_data->sb_vert->blockSignals(false);
  }
}

/****************************************************************************
 * Attributes
 ****************************************************************************/

static int qtCanvasSetPosXAttrib(Ihandle* ih, const char* value)
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

    qtCanvasUpdateScrollPos(ih, (float)posx, (float)ih->data->posy);
  }
  return 1;
}

static int qtCanvasSetPosYAttrib(Ihandle* ih, const char* value)
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

    qtCanvasUpdateScrollPos(ih, (float)ih->data->posx, (float)posy);
  }
  return 1;
}

static int qtCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (container_data && container_data->canvas)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
    {
      QPalette palette = container_data->canvas->palette();
      palette.setColor(QPalette::Window, QColor(r, g, b));
      container_data->canvas->setPalette(palette);
      container_data->canvas->setAutoFillBackground(true);
      return 1;
    }
  }

  return 0;
}

static int qtCanvasSetBorderAttrib(Ihandle* ih, const char* value)
{
  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (container_data && container_data->canvas)
  {
    QFrame* frame = qobject_cast<QFrame*>(container_data->canvas);
    if (!frame)
      return 0;

    if (iupStrBoolean(value))
    {
      frame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
      frame->setLineWidth(1);
    }
    else
    {
      frame->setFrameStyle(QFrame::NoFrame);
    }
    return 1;
  }

  return 0;
}

static char* qtCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (container_data && container_data->canvas)
  {
    int w = container_data->canvas->width();
    int h = container_data->canvas->height();
    return iupStrReturnIntInt(w, h, 'x');
  }

  return nullptr;
}

static char* qtCanvasGetDrawableAttrib(Ihandle* ih)
{
  /* Return the native graphics context */
  return (char*)iupqtCanvasGetContext(ih);
}

static char* qtCanvasGetScrollVisibleAttrib(Ihandle* ih)
{
  IupQtCanvasContainer* container_data = qtCanvasGetContainer(ih);

  if (!container_data)
    return nullptr;

  int horiz_visible = 0;
  int vert_visible = 0;

  if (container_data->sb_horiz && container_data->sb_horiz->isVisible())
    horiz_visible = 1;

  if (container_data->sb_vert && container_data->sb_vert->isVisible())
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

/****************************************************************************
 * Get Inner Native Container Handle
 ****************************************************************************/

static void* qtCanvasGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  /* Return the extra_parent widget for absolute positioning of child elements */
  QWidget* extra_parent = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (extra_parent)
    return (void*)extra_parent;
  return ih->handle;
}

/****************************************************************************
 * Canvas Driver Initialization
 ****************************************************************************/

extern "C" void iupdrvCanvasInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtCanvasMapMethod;
  ic->UnMap = qtCanvasUnMapMethod;
  ic->LayoutUpdate = qtCanvasLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = qtCanvasGetInnerNativeContainerHandleMethod;

  /* Canvas specific */
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qtCanvasSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "255 255 255", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BORDER", nullptr, qtCanvasSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DRAWSIZE", qtCanvasGetDrawSizeAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWABLE", qtCanvasGetDrawableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* Scrollbar attributes */
  iupClassRegisterAttribute(ic, "DX", nullptr, qtCanvasSetDXAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", nullptr, qtCanvasSetDYAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, qtCanvasSetPosXAttrib, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, qtCanvasSetPosYAttrib, "0", nullptr, IUPAF_NO_INHERIT);
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

  /* Scrollbar state */
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", qtCanvasGetScrollVisibleAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* Native window handle (platform-specific: XWINDOW, WL_SURFACE, HWND, NSVIEW) */
  iupClassRegisterAttribute(ic, iupqtGetNativeWindowHandleName(), iupqtGetNativeWindowHandleAttrib, nullptr, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NO_INHERIT);

  /* Not supported */
  iupClassRegisterAttribute(ic, "TOUCH", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKINGSTORE", nullptr, nullptr, "YES", nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  /* Other attributes */
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTTRANSPARENT", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WHEELDROPFOCUS", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPRECT", nullptr, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
}

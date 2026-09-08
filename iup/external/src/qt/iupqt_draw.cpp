/** \file
 * \brief Canvas Drawing - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QStringList>
#include <QPixmap>
#include <QImage>
#include <QPainterPath>
#include <QTextLayout>
#include <QTextOption>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Forward Declarations
 ****************************************************************************/

IUP_DRV_API void* iupqtCanvasGetContext(Ihandle* ih);

/****************************************************************************
 * Draw Context Structure
 ****************************************************************************/

struct _IdrawCanvas
{
  Ihandle* ih;
  QPainter* painter;
  QWidget* widget;
  QPixmap* buffer;

  int release_gc;

  QColor fg_color;
  QColor bg_color;
  int line_width;
  int line_style;
  int text_antialias;
  int shape_antialias;
  int winding_rule;  /* Qt::FillRule */

  int clip_x1, clip_y1, clip_x2, clip_y2;
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static void qtDrawGetColor(long color, QColor& qcolor)
{
  unsigned char r, g, b, a;
  r = iupDrawRed(color);
  g = iupDrawGreen(color);
  b = iupDrawBlue(color);
  a = iupDrawAlpha(color);
  qcolor.setRgb(r, g, b, a);
}

/****************************************************************************
 * Create Draw Canvas
 ****************************************************************************/

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = new IdrawCanvas();

  dc->ih = ih;

  /* ih->handle is the container, not the canvas widget */
  dc->widget = (QWidget*)iupqtCanvasGetContext(ih);

  if (!dc->widget)
    dc->widget = (QWidget*)iupAttribGet(ih, "_IUPQT_PREVIEW_CANVAS");

  dc->painter = nullptr;
  dc->release_gc = 0;

  if (dc->widget)
  {
    QSize widget_size = dc->widget->size();

    QPixmap* preview_buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_PREVIEW_BUFFER");
    if (preview_buffer)
    {
      dc->buffer = preview_buffer;
    }
    else
    {
      QPixmap* old_buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_CANVAS_BUFFER");
      if (old_buffer)
      {
        if (old_buffer->size() != widget_size)
        {
          delete old_buffer;
          old_buffer = nullptr;
        }
        else
        {
          dc->buffer = old_buffer;
        }
      }

      if (!old_buffer)
      {
        unsigned char r = 255, g = 255, b = 255;
        char* bgcolor = iupAttribGet(ih, "BGCOLOR");
        if (!bgcolor || !iupStrToRGB(bgcolor, &r, &g, &b))
          iupStrToRGB(iupBaseNativeParentGetBgColor(ih), &r, &g, &b);

        dc->buffer = new QPixmap(widget_size);
        dc->buffer->fill(QColor(r, g, b));

        iupAttribSet(ih, "_IUPQT_CANVAS_BUFFER", (char*)dc->buffer);
      }
    }
  }
  else
  {
    dc->buffer = nullptr;
  }

  dc->fg_color = QColor(0, 0, 0);
  dc->bg_color = QColor(255, 255, 255);

  dc->line_width = 1;
  dc->line_style = IUP_DRAW_STROKE;

  dc->text_antialias = 1;
  dc->shape_antialias = 1;

  dc->winding_rule = Qt::OddEvenFill;

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;

  iupAttribSet(ih, "DRAWDRIVER", "QT");

  if (dc->buffer)
  {
    dc->painter = new QPainter(dc->buffer);
    dc->release_gc = 1;

    dc->painter->setRenderHint(QPainter::Antialiasing, dc->shape_antialias ? true : false);
    dc->painter->setRenderHint(QPainter::TextAntialiasing, dc->text_antialias ? true : false);
  }

  return dc;
}

/****************************************************************************
 * Destroy Draw Canvas
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (dc)
  {
    if (dc->painter && dc->release_gc)
    {
      delete dc->painter;
    }

    /* the buffer outlives the painter; paintEvent displays it until unmap or resize */

    delete dc;
  }
}

/****************************************************************************
 * Get Size of Draw Canvas
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  int width = 0, height = 0;

  if (dc && dc->widget)
  {
    width = dc->widget->width();
    height = dc->widget->height();
  }

  if (w) *w = width;
  if (h) *h = height;
}

/****************************************************************************
 * Update Canvas (Flush)
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc || !dc->widget || !dc->buffer)
    return;

  if (dc->painter && dc->release_gc)
  {
    dc->painter->end();
    delete dc->painter;
    dc->painter = nullptr;
    dc->release_gc = 0;
  }

  dc->widget->update();
}

/****************************************************************************
 * Get Clip Area
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (dc)
  {
    *x1 = dc->clip_x1;
    *y1 = dc->clip_y1;
    *x2 = dc->clip_x2;
    *y2 = dc->clip_y2;
  }
}

/****************************************************************************
 * Set Clip Area
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->painter)
    return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    dc->painter->setClipping(false);
    dc->clip_x1 = 0;
    dc->clip_y1 = 0;
    dc->clip_x2 = 0;
    dc->clip_y2 = 0;
  }
  else
  {
    iupDrawCheckSwapCoord(x1, x2);
    iupDrawCheckSwapCoord(y1, y2);

    dc->painter->setClipRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    dc->painter->setClipping(true);
    dc->clip_x1 = x1;
    dc->clip_y1 = y1;
    dc->clip_x2 = x2;
    dc->clip_y2 = y2;
  }
}

/****************************************************************************
 * Set Clip Rounded Rectangle
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  if (!dc || !dc->painter)
    return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    dc->painter->setClipping(false);
    dc->clip_x1 = 0;
    dc->clip_y1 = 0;
    dc->clip_x2 = 0;
    dc->clip_y2 = 0;
  }
  else
  {
    iupDrawCheckSwapCoord(x1, x2);
    iupDrawCheckSwapCoord(y1, y2);

    int max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
    if (corner_radius > max_radius)
      corner_radius = max_radius;

    int width = x2 - x1;
    int height = y2 - y1;

    QPainterPath path;
    path.addRoundedRect(x1, y1, width, height, corner_radius, corner_radius);

    dc->painter->setClipPath(path);
    dc->painter->setClipping(true);

    dc->clip_x1 = x1;
    dc->clip_y1 = y1;
    dc->clip_x2 = x2;
    dc->clip_y2 = y2;
  }
}

/****************************************************************************
 * Reset Clip Area
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (dc && dc->painter)
  {
    dc->painter->setClipping(false);
    dc->clip_x1 = 0;
    dc->clip_y1 = 0;
    dc->clip_x2 = 0;
    dc->clip_y2 = 0;
  }
}

/****************************************************************************
 * Parent Background
 ****************************************************************************/

void qtDrawParentBackground(IdrawCanvas* dc)
{
  if (!dc || !dc->painter || !dc->widget)
    return;

  unsigned char r, g, b;
  char* color = iupBaseNativeParentGetBgColor(dc->ih);
  if (!color)
    color = (char*)"255 255 255";

  long c = iupDrawStrToColor(color, 0);
  r = iupDrawRed(c);
  g = iupDrawGreen(c);
  b = iupDrawBlue(c);

  dc->painter->fillRect(dc->widget->rect(), QColor(r, g, b));
}

/****************************************************************************
 * Line Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  QPen pen(qcolor);
  pen.setWidth(line_width);

  switch (style)
  {
    case IUP_DRAW_STROKE_DASH:
      pen.setStyle(Qt::DashLine);
      break;
    case IUP_DRAW_STROKE_DOT:
      pen.setStyle(Qt::DotLine);
      break;
    case IUP_DRAW_STROKE_DASH_DOT:
      pen.setStyle(Qt::DashDotLine);
      break;
    case IUP_DRAW_STROKE_DASH_DOT_DOT:
      pen.setStyle(Qt::DashDotDotLine);
      break;
    default:
      pen.setStyle(Qt::SolidLine);
      break;
  }

  pen.setCapStyle(Qt::FlatCap);
  pen.setJoinStyle(Qt::MiterJoin);

  dc->painter->setPen(pen);
  dc->painter->drawLine(x1, y1, x2, y2);
}

/****************************************************************************
 * Rectangle Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->fillRect(x1, y1, w, h, qcolor);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawRect(x1, y1, w - 1, h - 1);
  }
}

/****************************************************************************
 * Arc Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  /* Qt angles are in 1/16 degree, same origin and direction as IUP */
  int start_angle = (int)(a1 * 16.0);
  int span_angle = (int)((a2 - a1) * 16.0);

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawPie(x1, y1, w, h, start_angle, span_angle);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawArc(x1, y1, w, h, start_angle, span_angle);
  }
}

/****************************************************************************
 * Ellipse Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawEllipse(x1, y1, w, h);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawEllipse(x1, y1, w, h);
  }
}

/****************************************************************************
 * Polygon Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (!dc || !dc->painter || count < 2)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  QPolygon polygon;
  polygon.reserve(count);
  for (int i = 0; i < count; i++)
  {
    polygon << QPoint(points[i * 2], points[i * 2 + 1]);
  }

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);

    QPainterPath path;
    path.setFillRule((Qt::FillRule)dc->winding_rule);
    path.addPolygon(polygon);
    path.closeSubpath();
    dc->painter->drawPath(path);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawPolygon(polygon);
  }
}

/****************************************************************************
 * Pixel Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  dc->painter->setPen(qcolor);
  dc->painter->drawPoint(x, y);
}

/****************************************************************************
 * Rounded Rectangle Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (corner_radius > max_radius)
    corner_radius = max_radius;

  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawRoundedRect(x1, y1, width, height, corner_radius, corner_radius);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawRoundedRect(x1, y1, width, height, corner_radius, corner_radius);
  }
}

/****************************************************************************
 * Bezier Curve Draw (Cubic)
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  QPainterPath path;
  path.moveTo(x1, y1);
  path.cubicTo(x2, y2, x3, y3, x4, y4);

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawPath(path);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawPath(path);
  }
}

/****************************************************************************
 * Bezier Curve Draw (Quadratic)
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  QPainterPath path;
  path.moveTo(x1, y1);
  path.quadTo(x2, y2, x3, y3);

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawPath(path);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawPath(path);
  }
}

/****************************************************************************
 * Polyline Draw (not closed)
 ****************************************************************************/

void qtDrawPolyline(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (!dc || !dc->painter || count < 2)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  QPen pen(qcolor);
  pen.setWidth(line_width);

  switch (style)
  {
    case IUP_DRAW_STROKE_DASH:
      pen.setStyle(Qt::DashLine);
      break;
    case IUP_DRAW_STROKE_DOT:
      pen.setStyle(Qt::DotLine);
      break;
    case IUP_DRAW_STROKE_DASH_DOT:
      pen.setStyle(Qt::DashDotLine);
      break;
    case IUP_DRAW_STROKE_DASH_DOT_DOT:
      pen.setStyle(Qt::DashDotDotLine);
      break;
    default:
      pen.setStyle(Qt::SolidLine);
      break;
  }

  dc->painter->setPen(pen);
  dc->painter->setBrush(Qt::NoBrush);

  QPolygon polyline;
  polyline.reserve(count);
  for (int i = 0; i < count; i++)
  {
    polyline << QPoint(points[i * 2], points[i * 2 + 1]);
  }

  dc->painter->drawPolyline(polyline);
}

/****************************************************************************
 * Rounded Rectangle Draw
 ****************************************************************************/

void qtDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int radius_x, int radius_y, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
  {
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawRoundedRect(x1, y1, w, h, radius_x, radius_y);
  }
  else
  {
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawRoundedRect(x1, y1, w, h, radius_x, radius_y);
  }
}

/****************************************************************************
 * Text Draw with improved wrap/ellipsis support
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  if (!dc || !dc->painter || !text)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  int ascent = 0, charheight = 0;
  QFont* qfont = iupqtGetQFontLine(font, &ascent, &charheight);
  if (qfont)
    dc->painter->setFont(*qfont);

  dc->painter->setPen(qcolor);

  QString qtext = QString::fromUtf8(text, len > 0 ? len : -1);
  QFontMetrics fm = dc->painter->fontMetrics();

  int align = Qt::AlignTop;
  if (flags & IUP_DRAW_CENTER)
    align |= Qt::AlignHCenter;
  else if (flags & IUP_DRAW_RIGHT)
    align |= Qt::AlignRight;
  else
    align |= Qt::AlignLeft;

  if (flags & IUP_DRAW_WRAP)
    align |= Qt::TextWordWrap;
  else if ((flags & IUP_DRAW_ELLIPSIS) && w > 0)
  {
    QStringList lines = qtext.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); i++)
      lines[i] = fm.elidedText(lines[i], Qt::ElideRight, w);
    qtext = lines.join(QLatin1Char('\n'));
  }

  if (!(flags & IUP_DRAW_CLIP))
    align |= Qt::TextDontClip;

  dc->painter->save();

  if (flags & IUP_DRAW_CLIP)
    dc->painter->setClipRect(x, y, w, h);

  QRect rect(x, y, w, h);
  if (w <= 0 || h <= 0)
    rect = QRect(x, y, 32767, 32767);

  if (text_orientation != 0.0)
  {
    if (flags & IUP_DRAW_LAYOUTCENTER)
    {
      QRect layout = fm.boundingRect(QRect(0, 0, 32767, 32767), Qt::AlignLeft | Qt::AlignTop, qtext);
      int layout_w = layout.width(), layout_h = layout.height();
      dc->painter->translate((w - layout_w) / 2.0, (h - layout_h) / 2.0);
      dc->painter->translate(x + layout_w / 2.0, y + layout_h / 2.0);
      dc->painter->rotate(-text_orientation);
      dc->painter->translate(-(x + layout_w / 2.0), -(y + layout_h / 2.0));
      rect = QRect(x, y, layout_w, layout_h);
    }
    else
    {
      dc->painter->translate(x, y);
      dc->painter->rotate(-text_orientation);
      dc->painter->translate(-x, -y);
    }
  }

  dc->painter->drawText(rect, align, qtext);

  dc->painter->restore();
}

/****************************************************************************
 * Image Draw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  if (!dc || !dc->painter || !name)
    return;

  QPixmap* pixmap = (QPixmap*)iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
  if (!pixmap)
    return;

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = pixmap->width();
    sh = pixmap->height();
  }
  if (w <= 0) w = sw;
  if (h <= 0) h = sh;

  dc->painter->save();
  dc->painter->setRenderHint(QPainter::SmoothPixmapTransform, quality != IUP_DRAW_IMAGE_NEAREST);
  if (opacity < 255)
    dc->painter->setOpacity(opacity / 255.0);
  dc->painter->drawPixmap(QRect(x, y, w, h), *pixmap, QRect(sx, sy, sw, sh));
  dc->painter->restore();
}

/****************************************************************************
 * Select Rectangle (XOR)
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->painter)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  dc->painter->save();

  dc->painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);

  QPen pen(Qt::white);
  pen.setStyle(Qt::DashLine);
  pen.setWidth(1);

  dc->painter->setPen(pen);
  dc->painter->setBrush(Qt::NoBrush);
  dc->painter->drawRect(x1, y1, x2 - x1, y2 - y1);

  dc->painter->restore();
}

/****************************************************************************
 * Focus Rectangle
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->painter)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  QPen pen(Qt::black);
  pen.setStyle(Qt::DotLine);
  pen.setWidth(1);

  dc->painter->setPen(pen);
  dc->painter->setBrush(Qt::NoBrush);
  dc->painter->drawRect(x1, y1, x2 - x1, y2 - y1);
}

/****************************************************************************
 * Linear Gradient
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  if (!dc || !dc->painter)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* 0 = left to right, 90 = top to bottom, 180 = right to left, 270 = bottom to top */
  qreal rad = angle * M_PI / 180.0;
  qreal w = x2 - x1;
  qreal h = y2 - y1;
  qreal cx = x1 + w / 2.0;
  qreal cy = y1 + h / 2.0;

  QPointF start(cx - (w * cos(rad)) / 2.0, cy - (h * sin(rad)) / 2.0);
  QPointF end(cx + (w * cos(rad)) / 2.0, cy + (h * sin(rad)) / 2.0);

  QLinearGradient gradient(start, end);
  for (int i = 0; i < count; i++)
    gradient.setColorAt(offsets[i], QColor(iupDrawRed(colors[i]), iupDrawGreen(colors[i]), iupDrawBlue(colors[i]), iupDrawAlpha(colors[i])));

  dc->painter->fillRect(x1, y1, x2 - x1, y2 - y1, gradient);
}

/****************************************************************************
 * Radial Gradient
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  if (!dc || !dc->painter)
    return;

  QRadialGradient gradient(cx, cy, radius, cx, cy);
  for (int i = 0; i < count; i++)
    gradient.setColorAt(offsets[i], QColor(iupDrawRed(colors[i]), iupDrawGreen(colors[i]), iupDrawBlue(colors[i]), iupDrawAlpha(colors[i])));

  dc->painter->setPen(Qt::NoPen);
  dc->painter->setBrush(gradient);
  dc->painter->drawEllipse(QPoint(cx, cy), radius, radius);
}

/****************************************************************************
 * Get Text Size
 ****************************************************************************/

void qtDrawGetTextSize(IdrawCanvas* dc, const char* text, int len, int *w, int *h, const char* font)
{
  if (!text)
  {
    *w = 0;
    *h = 0;
    return;
  }

  QFont* qfont = iupqtGetQFont(font);
  if (!qfont)
  {
    *w = 0;
    *h = 0;
    return;
  }

  QFontMetrics metrics(*qfont);

  QString qtext = QString::fromUtf8(text, len > 0 ? len : -1);

  QRect rect = metrics.boundingRect(qtext);
  *w = rect.width();
  *h = rect.height();
}

/****************************************************************************
 * Get Image Info
 ****************************************************************************/

void qtDrawGetImageInfo(const char* name, int *w, int *h, int *bpp)
{
  if (!name)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return;
  }

  void* img_handle = IupGetHandle(name);
  if (img_handle)
  {
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(name, NULL, 0, NULL);
    if (pixmap)
    {
      if (w) *w = pixmap->width();
      if (h) *h = pixmap->height();
      if (bpp) *bpp = pixmap->depth();
      return;
    }
  }

  QPixmap pixmap(QString::fromUtf8(name));
  if (!pixmap.isNull())
  {
    if (w) *w = pixmap.width();
    if (h) *h = pixmap.height();
    if (bpp) *bpp = pixmap.depth();
    return;
  }

  if (w) *w = 0;
  if (h) *h = 0;
  if (bpp) *bpp = 0;
}

/****************************************************************************
 * Winding Rule
 ****************************************************************************/

void qtDrawSetWindingRule(IdrawCanvas* dc, int winding_rule)
{
  if (!dc)
    return;

  /* winding_rule: 0=EVENODD, 1=WINDING */
  if (winding_rule == 0)
    dc->winding_rule = Qt::OddEvenFill;
  else
    dc->winding_rule = Qt::WindingFill;
}

/****************************************************************************
 * Anti-aliasing Control
 ****************************************************************************/

void qtDrawSetTextAntiAlias(IdrawCanvas* dc, int antialias)
{
  if (dc)
  {
    dc->text_antialias = antialias;
    if (dc->painter)
    {
      dc->painter->setRenderHint(QPainter::TextAntialiasing, antialias ? true : false);
    }
  }
}

void qtDrawSetShapeAntiAlias(IdrawCanvas* dc, int antialias)
{
  if (dc)
  {
    dc->shape_antialias = antialias;
    if (dc->painter)
    {
      dc->painter->setRenderHint(QPainter::Antialiasing, antialias ? true : false);
    }
  }
}

/****************************************************************************
 * Update Size (for buffering)
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc || !dc->widget)
    return;

  QSize new_size = dc->widget->size();

  if (!dc->buffer || dc->buffer->size() != new_size)
  {
    if (dc->buffer)
    {
      if (dc->ih)
        iupAttribSet(dc->ih, "_IUPQT_CANVAS_BUFFER", NULL);
      delete dc->buffer;
    }

    dc->buffer = new QPixmap(new_size);
    dc->buffer->fill(Qt::white);

    if (dc->ih)
      iupAttribSet(dc->ih, "_IUPQT_CANVAS_BUFFER", (char*)dc->buffer);

    if (dc->painter && dc->release_gc)
    {
      delete dc->painter;
    }

    dc->painter = new QPainter(dc->buffer);
    dc->release_gc = 1;

    dc->painter->setRenderHint(QPainter::Antialiasing, dc->shape_antialias ? true : false);
    dc->painter->setRenderHint(QPainter::TextAntialiasing, dc->text_antialias ? true : false);
  }
}

/****************************************************************************
 * Begin/End Drawing
 ****************************************************************************/

void qtDrawBegin(IdrawCanvas* dc)
{
  if (!dc || !dc->widget)
    return;

  iupdrvDrawUpdateSize(dc);

  if (!dc->painter)
  {
    dc->painter = new QPainter(dc->buffer);
    dc->release_gc = 1;

    dc->painter->setRenderHint(QPainter::Antialiasing, dc->shape_antialias ? true : false);
    dc->painter->setRenderHint(QPainter::TextAntialiasing, dc->text_antialias ? true : false);
  }
}

void qtDrawEnd(IdrawCanvas* dc)
{
  (void)dc;
}

extern "C" IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !dc->buffer)
    return 0;

  QImage img = dc->buffer->toImage().convertToFormat(QImage::Format_RGBA8888);
  int w = img.width();
  int h = img.height();

  for (int y = 0; y < h; y++)
  {
    const unsigned char* src_line = img.constScanLine(y);
    unsigned char* dst_line = data + y * w * 4;
    memcpy(dst_line, src_line, w * 4);
  }

  return 1;
}

extern "C" IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  QPixmap* buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_CANVAS_BUFFER");
  if (!buffer)
    buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_PREVIEW_BUFFER");
  if (!buffer)
    return 0;

  QImage img = buffer->toImage().convertToFormat(QImage::Format_RGBA8888);

  if (w > img.width())
    w = img.width();
  if (h > img.height())
    h = img.height();

  for (int y = 0; y < h; y++)
  {
    const unsigned char* src_line = img.constScanLine(y);
    unsigned char* dst_line = data + y * w * 4;
    memcpy(dst_line, src_line, (size_t)w * 4);
  }

  return 1;
}

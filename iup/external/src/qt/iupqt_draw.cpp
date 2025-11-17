/** \file
 * \brief Canvas Drawing - Qt Implementation
 *
 * Performance notes:
 * - QPainter can use OpenGL backend for acceleration
 * - Double buffering prevents flicker during redraws
 * - Text rendering uses font hinting and subpixel positioning
 * - Image drawing supports alpha blending and transforms
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QImage>
#include <QPainterPath>
#include <QTransform>
#include <QTextLayout>
#include <QTextOption>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_drvfont.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Forward Declarations
 ****************************************************************************/

/* Forward declare canvas functions needed for draw canvas creation */
extern "C" void* iupqtCanvasGetContext(Ihandle* ih);

/****************************************************************************
 * Draw Context Structure
 ****************************************************************************/

struct _IdrawCanvas
{
  Ihandle* ih;
  QPainter* painter;
  QWidget* widget;
  QPixmap* buffer;    /* Offscreen buffer for double buffering */

  int release_gc;

  /* Drawing state */
  QColor fg_color;
  QColor bg_color;
  int line_width;
  int line_style;
  int text_antialias;
  int shape_antialias;
  int winding_rule;  /* Qt::FillRule */

  /* Clip state */
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

extern "C" IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = new IdrawCanvas();

  dc->ih = ih;

  /* Get the actual canvas widget from qtCanvasGetContext */
  /* ih->handle is the container, not the canvas widget */
  dc->widget = (QWidget*)iupqtCanvasGetContext(ih);
  dc->painter = nullptr;
  dc->release_gc = 0;

  /* Create offscreen buffer */
  if (dc->widget)
  {
    QSize widget_size = dc->widget->size();

    /* Check if buffer already exists */
    QPixmap* old_buffer = (QPixmap*)iupAttribGet(ih, "_IUPQT_CANVAS_BUFFER");
    if (old_buffer)
    {
      /* Check if size changed - recreate buffer if needed */
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
      dc->buffer = new QPixmap(widget_size);
      dc->buffer->fill(Qt::white);

      /* Store buffer pointer so paintEvent can access it */
      iupAttribSet(ih, "_IUPQT_CANVAS_BUFFER", (char*)dc->buffer);
    }
  }
  else
  {
    dc->buffer = nullptr;
  }

  /* Default colors */
  dc->fg_color = QColor(0, 0, 0);
  dc->bg_color = QColor(255, 255, 255);

  /* Default line attributes */
  dc->line_width = 1;
  dc->line_style = IUP_DRAW_STROKE;

  /* Default antialiasing */
  dc->text_antialias = 1;
  dc->shape_antialias = 1;

  /* Default winding rule */
  dc->winding_rule = Qt::OddEvenFill;

  /* Default clip */
  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;

  iupAttribSet(ih, "DRAWDRIVER", "QT");

  /* Create QPainter on buffer for drawing */
  if (dc->buffer)
  {
    dc->painter = new QPainter(dc->buffer);
    dc->release_gc = 1;

    /* Set default antialiasing */
    dc->painter->setRenderHint(QPainter::Antialiasing, dc->shape_antialias ? true : false);
    dc->painter->setRenderHint(QPainter::TextAntialiasing, dc->text_antialias ? true : false);
  }

  return dc;
}

/****************************************************************************
 * Destroy Draw Canvas
 ****************************************************************************/

extern "C" void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (dc)
  {
    if (dc->painter && dc->release_gc)
    {
      delete dc->painter;
    }

    /* DO NOT delete the buffer here!
     * The buffer needs to persist so paintEvent can display it.
     * It will be deleted when the canvas is unmapped or recreated on resize. */

    delete dc;
  }
}

/****************************************************************************
 * Get Size of Draw Canvas
 ****************************************************************************/

extern "C" void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (dc && dc->widget)
  {
    *w = dc->widget->width();
    *h = dc->widget->height();
  }
  else
  {
    *w = 0;
    *h = 0;
  }
}

/****************************************************************************
 * Update Canvas (Flush)
 ****************************************************************************/

extern "C" void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc || !dc->widget || !dc->buffer)
    return;

  /* End drawing on buffer */
  if (dc->painter && dc->release_gc)
  {
    dc->painter->end();
    delete dc->painter;
    dc->painter = nullptr;
    dc->release_gc = 0;
  }

  /* Trigger widget repaint to copy buffer to screen */
  /* The actual buffer->widget copy happens in the paintEvent */
  dc->widget->update();
}

/****************************************************************************
 * Get Clip Area
 ****************************************************************************/

extern "C" void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
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

extern "C" void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->painter)
    return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    /* Reset clipping */
    dc->painter->setClipping(false);
    dc->clip_x1 = 0;
    dc->clip_y1 = 0;
    dc->clip_x2 = 0;
    dc->clip_y2 = 0;
  }
  else
  {
    /* Set clip rectangle */
    dc->painter->setClipRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
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

extern "C" void iupdrvDrawResetClip(IdrawCanvas* dc)
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

extern "C" void qtDrawParentBackground(IdrawCanvas* dc)
{
  if (!dc || !dc->painter || !dc->widget)
    return;

  /* Fill with parent background color */
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

extern "C" void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                               long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  QPen pen(qcolor);
  pen.setWidth(line_width);

  /* Set line style */
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

  /* Set pen cap and join styles */
  pen.setCapStyle(Qt::FlatCap);
  pen.setJoinStyle(Qt::MiterJoin);

  dc->painter->setPen(pen);
  dc->painter->drawLine(x1, y1, x2, y2);
}

/****************************************************************************
 * Rectangle Draw
 ****************************************************************************/

extern "C" void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                                    long color, int style, int line_width)
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
    /* Filled rectangle */
    dc->painter->fillRect(x1, y1, w, h, qcolor);
  }
  else
  {
    /* Stroke rectangle */
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

extern "C" void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                              double a1, double a2, long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  /* Convert angles: IUP uses degrees, Qt uses 1/16th of a degree */
  /* IUP: 0 degrees is at 3 o'clock, positive counter-clockwise */
  /* Qt: 0 degrees is at 3 o'clock, positive counter-clockwise */
  int start_angle = (int)(a1 * 16.0);
  int span_angle = (int)((a2 - a1) * 16.0);

  if (style == IUP_DRAW_FILL)
  {
    /* Filled arc (pie) */
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawPie(x1, y1, w, h, start_angle, span_angle);
  }
  else
  {
    /* Stroke arc */
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

extern "C" void qtDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                                  long color, int style, int line_width)
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
    /* Filled ellipse */
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawEllipse(x1, y1, w, h);
  }
  else
  {
    /* Stroke ellipse */
    QPen pen(qcolor);
    pen.setWidth(line_width);
    pen.setStyle(Qt::SolidLine);

    dc->painter->setPen(pen);
    dc->painter->setBrush(Qt::NoBrush);
    dc->painter->drawEllipse(x1, y1, w, h);
  }
}

extern "C" void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                                   long color, int style, int line_width)
{
  qtDrawEllipse(dc, x1, y1, x2, y2, color, style, line_width);
}

/****************************************************************************
 * Polygon Draw
 ****************************************************************************/

extern "C" void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count,
                                  long color, int style, int line_width)
{
  if (!dc || !dc->painter || count < 2)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  /* Convert points array to QPolygon */
  QPolygon polygon;
  polygon.reserve(count);  /* Reserve capacity to avoid reallocations */
  for (int i = 0; i < count; i++)
  {
    polygon << QPoint(points[i * 2], points[i * 2 + 1]);
  }

  if (style == IUP_DRAW_FILL)
  {
    /* Filled polygon */
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);

    /* Set fill rule based on winding rule */
    QPainterPath path;
    path.setFillRule((Qt::FillRule)dc->winding_rule);
    path.addPolygon(polygon);
    path.closeSubpath();
    dc->painter->drawPath(path);
  }
  else
  {
    /* Stroke polygon */
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

extern "C" void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
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

extern "C" void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1,
                                            int x2, int y2, int corner_radius,
                                            long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  /* Ensure coordinates are properly ordered */
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Clamp radius to prevent oversized corners */
  int max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (corner_radius > max_radius)
    corner_radius = max_radius;

  /* Calculate dimensions */
  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
  {
    /* Filled rounded rectangle */
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawRoundedRect(x1, y1, width, height, corner_radius, corner_radius);
  }
  else
  {
    /* Stroke rounded rectangle */
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

extern "C" void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                                   int x3, int y3, int x4, int y4,
                                   long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  /* Create path for cubic Bezier curve */
  QPainterPath path;
  path.moveTo(x1, y1);
  path.cubicTo(x2, y2, x3, y3, x4, y4);

  if (style == IUP_DRAW_FILL)
  {
    /* Filled Bezier curve */
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawPath(path);
  }
  else
  {
    /* Stroke Bezier curve */
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

extern "C" void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1,
                                           int x2, int y2, int x3, int y3,
                                           long color, int style, int line_width)
{
  if (!dc || !dc->painter)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  /* Qt has native support for quadratic Bezier curves! */
  QPainterPath path;
  path.moveTo(x1, y1);
  path.quadTo(x2, y2, x3, y3);

  if (style == IUP_DRAW_FILL)
  {
    /* Filled quadratic Bezier curve */
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawPath(path);
  }
  else
  {
    /* Stroke quadratic Bezier curve */
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

extern "C" void qtDrawPolyline(IdrawCanvas* dc, int* points, int count,
                                   long color, int style, int line_width)
{
  if (!dc || !dc->painter || count < 2)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  QPen pen(qcolor);
  pen.setWidth(line_width);

  /* Set line style */
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

  /* Convert points array to QPolygon and draw as polyline */
  QPolygon polyline;
  polyline.reserve(count);  /* Reserve capacity to avoid reallocations */
  for (int i = 0; i < count; i++)
  {
    polyline << QPoint(points[i * 2], points[i * 2 + 1]);
  }

  dc->painter->drawPolyline(polyline);
}

/****************************************************************************
 * Rounded Rectangle Draw
 ****************************************************************************/

extern "C" void qtDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                                           int radius_x, int radius_y, long color, int style, int line_width)
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
    /* Filled rounded rectangle */
    dc->painter->setPen(Qt::NoPen);
    dc->painter->setBrush(qcolor);
    dc->painter->drawRoundedRect(x1, y1, w, h, radius_x, radius_y);
  }
  else
  {
    /* Stroke rounded rectangle */
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

extern "C" void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y,
                               int w, int h, long color, const char* font, int flags,
                               double text_orientation)
{
  if (!dc || !dc->painter || !text)
    return;

  QColor qcolor;
  qtDrawGetColor(color, qcolor);

  /* Set font */
  QFont* qfont = iupqtGetQFont(font);
  if (qfont)
    dc->painter->setFont(*qfont);

  /* Set text color */
  dc->painter->setPen(qcolor);

  /* Convert text to QString */
  QString qtext = QString::fromUtf8(text, len > 0 ? len : -1);

  /* Build alignment flags */
  int align_flags = 0;

  if (flags & IUP_DRAW_CENTER)
    align_flags |= Qt::AlignHCenter;
  else if (flags & IUP_DRAW_RIGHT)
    align_flags |= Qt::AlignRight;
  else
    align_flags |= Qt::AlignLeft;

  /* Vertical alignment - IUP doesn't define vertical alignment flags, default to center */
  align_flags |= Qt::AlignVCenter;

  /* Handle wrap and ellipsis using QTextLayout for better control */
  if ((flags & IUP_DRAW_WRAP) || (flags & IUP_DRAW_ELLIPSIS))
  {
    QTextLayout textLayout(qtext, *qfont);
    QTextOption option;

    if (flags & IUP_DRAW_WRAP)
      option.setWrapMode(QTextOption::WordWrap);
    else
      option.setWrapMode(QTextOption::NoWrap);

    option.setAlignment((Qt::Alignment)align_flags);
    textLayout.setTextOption(option);

    /* Begin layout */
    textLayout.beginLayout();
    qreal y_pos = 0;
    while (1)
    {
      QTextLine line = textLayout.createLine();
      if (!line.isValid())
        break;

      line.setLineWidth(w);
      y_pos += line.height();
      if (y_pos > h && (flags & IUP_DRAW_ELLIPSIS))
      {
        /* Add ellipsis for last visible line */
        break;
      }
    }
    textLayout.endLayout();

    /* Draw with transformation if rotated */
    if (text_orientation != 0.0)
    {
      dc->painter->save();
      dc->painter->translate(x, y);
      dc->painter->rotate(-text_orientation);  /* Qt rotates clockwise, IUP counter-clockwise */
      textLayout.draw(dc->painter, QPointF(0, 0));
      dc->painter->restore();
    }
    else
    {
      if (flags & IUP_DRAW_CLIP)
      {
        dc->painter->save();
        dc->painter->setClipRect(x, y, w, h);
      }

      textLayout.draw(dc->painter, QPointF(x, y));

      if (flags & IUP_DRAW_CLIP)
        dc->painter->restore();
    }
  }
  else
  {
    /* Handle text orientation */
    if (text_orientation != 0.0)
    {
      dc->painter->save();
      dc->painter->translate(x, y);
      dc->painter->rotate(-text_orientation);  /* Qt rotates clockwise, IUP counter-clockwise */
      dc->painter->drawText(QRect(0, 0, w, h), align_flags, qtext);
      dc->painter->restore();
    }
    else
    {
      /* Draw text */
      if (w > 0 && h > 0)
      {
        /* Draw in bounded rectangle */
        if (flags & IUP_DRAW_CLIP)
        {
          dc->painter->save();
          dc->painter->setClipRect(x, y, w, h);
          dc->painter->drawText(QRect(x, y, w, h), align_flags, qtext);
          dc->painter->restore();
        }
        else
          dc->painter->drawText(QRect(x, y, w, h), align_flags, qtext);
      }
      else
      {
        /* Draw at position */
        dc->painter->drawText(x, y, qtext);
      }
    }
  }
}

/****************************************************************************
 * Image Draw
 ****************************************************************************/

extern "C" void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive,
                                const char* bgcolor, int x, int y, int w, int h)
{
  if (!dc || !dc->painter || !name)
    return;

  /* Get image */
  QPixmap* pixmap = (QPixmap*)iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!pixmap)
    return;

  /* Draw image */
  if (w > 0 && h > 0 && (w != pixmap->width() || h != pixmap->height()))
  {
    /* Scale to fit - use painter transformation instead of creating scaled pixmap */
    /* This is faster as Qt can use hardware acceleration for the transform */
    dc->painter->save();
    dc->painter->translate(x, y);
    dc->painter->scale((double)w / pixmap->width(), (double)h / pixmap->height());
    dc->painter->drawPixmap(0, 0, *pixmap);
    dc->painter->restore();
  }
  else
  {
    /* Draw at original size */
    dc->painter->drawPixmap(x, y, *pixmap);
  }
}

/****************************************************************************
 * Select Rectangle (XOR)
 ****************************************************************************/

extern "C" void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->painter)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  dc->painter->save();

  /* Use XOR composition mode */
  dc->painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);

  /* Draw dashed rectangle */
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

extern "C" void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->painter)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Draw dotted focus rectangle */
  QPen pen(Qt::black);
  pen.setStyle(Qt::DotLine);
  pen.setWidth(1);

  dc->painter->setPen(pen);
  dc->painter->setBrush(Qt::NoBrush);
  dc->painter->drawRect(x1, y1, x2 - x1, y2 - y1);
}

/****************************************************************************
 * Get Text Size
 ****************************************************************************/

extern "C" void qtDrawGetTextSize(IdrawCanvas* dc, const char* text, int len,
                                      int *w, int *h, const char* font)
{
  if (!text)
  {
    *w = 0;
    *h = 0;
    return;
  }

  /* Get font */
  QFont* qfont = iupqtGetQFont(font);
  if (!qfont)
  {
    *w = 0;
    *h = 0;
    return;
  }

  /* Get font metrics */
  QFontMetrics metrics(*qfont);

  /* Convert text */
  QString qtext = QString::fromUtf8(text, len > 0 ? len : -1);

  /* Get size */
  QRect rect = metrics.boundingRect(qtext);
  *w = rect.width();
  *h = rect.height();
}

/****************************************************************************
 * Get Image Info
 ****************************************************************************/

extern "C" void qtDrawGetImageInfo(const char* name, int *w, int *h, int *bpp)
{
  if (!name)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return;
  }

  /* Get image */
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

  /* Try to load from file */
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

extern "C" void qtDrawSetWindingRule(IdrawCanvas* dc, int winding_rule)
{
  if (!dc)
    return;

  /* Qt uses Qt::FillRule */
  /* winding_rule: 0=EVENODD, 1=WINDING */
  if (winding_rule == 0)
    dc->winding_rule = Qt::OddEvenFill;
  else
    dc->winding_rule = Qt::WindingFill;
}

/****************************************************************************
 * Anti-aliasing Control
 ****************************************************************************/

extern "C" void qtDrawSetTextAntiAlias(IdrawCanvas* dc, int antialias)
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

extern "C" void qtDrawSetShapeAntiAlias(IdrawCanvas* dc, int antialias)
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

extern "C" void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc || !dc->widget)
    return;

  /* Recreate buffer if size changed */
  QSize new_size = dc->widget->size();

  if (!dc->buffer || dc->buffer->size() != new_size)
  {
    if (dc->buffer)
      delete dc->buffer;

    dc->buffer = new QPixmap(new_size);
    dc->buffer->fill(Qt::white);

    /* Update buffer attribute so paintEvent can access the new buffer */
    if (dc->ih)
      iupAttribSet(dc->ih, "_IUPQT_CANVAS_BUFFER", (char*)dc->buffer);

    /* Recreate painter */
    if (dc->painter && dc->release_gc)
    {
      delete dc->painter;
    }

    dc->painter = new QPainter(dc->buffer);
    dc->release_gc = 1;

    /* Set default antialiasing */
    dc->painter->setRenderHint(QPainter::Antialiasing, dc->shape_antialias ? true : false);
    dc->painter->setRenderHint(QPainter::TextAntialiasing, dc->text_antialias ? true : false);
  }
}

/****************************************************************************
 * Begin/End Drawing
 ****************************************************************************/

extern "C" void qtDrawBegin(IdrawCanvas* dc)
{
  if (!dc || !dc->widget)
    return;

  /* Ensure buffer is correct size */
  iupdrvDrawUpdateSize(dc);

  /* Create painter if not already created */
  if (!dc->painter)
  {
    dc->painter = new QPainter(dc->buffer);
    dc->release_gc = 1;

    /* Set default antialiasing */
    dc->painter->setRenderHint(QPainter::Antialiasing, dc->shape_antialias ? true : false);
    dc->painter->setRenderHint(QPainter::TextAntialiasing, dc->text_antialias ? true : false);
  }
}

extern "C" void qtDrawEnd(IdrawCanvas* dc)
{
  if (!dc)
    return;

  /* Don't delete painter here - it's needed until flush */
  if (dc->painter)
  {
    /* Nothing to do - painter will be deleted in flush */
  }
}

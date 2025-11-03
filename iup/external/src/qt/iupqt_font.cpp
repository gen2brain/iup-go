/** \file
 * \brief Qt Font mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <QFont>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QFontDatabase>
#include <QFontInfo>
#include <QApplication>
#include <QWidget>
#include <QScreen>
#include <QString>
#include <QTextDocument>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_array.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Font Cache Structure
 ****************************************************************************/

typedef struct _IqtFont
{
  char font[200];
  QFont* qfont;
  int charwidth, charheight;
  int max_width, ascent, descent;
  bool is_underline;
  bool is_strikeout;
} IqtFont;

static Iarray* qt_fonts = nullptr;

/****************************************************************************
 * Font Cache Management
 ****************************************************************************/

static IqtFont* qtFindFont(const char* font)
{
  int i, count;
  int size = 8;
  int is_bold = 0,
      is_italic = 0,
      is_underline = 0,
      is_strikeout = 0;
  char typeface[1024];
  const char* mapped_name;
  IqtFont* fonts;

  if (!qt_fonts)
    return nullptr;

  count = iupArrayCount(qt_fonts);
  fonts = (IqtFont*)iupArrayGetData(qt_fonts);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(font, fonts[i].font))
      return &fonts[i];
  }

  if (!iupFontParseWin(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    if (!iupFontParseX(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      if (!iupFontParsePango(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
        return nullptr;
    }
  }

  /* Map standard names to native names */
  mapped_name = iupFontGetPangoName(typeface);
  if (mapped_name)
    strcpy(typeface, mapped_name);

  /* Convert size to pixels if negative (already in pixels) */
  int point_size = size;
  if (size < 0)
  {
    /* Size is in pixels, convert to points */
    /* Qt uses point size by default */
    QWidget* widget = QApplication::activeWindow();
    if (!widget)
      widget = QApplication::allWidgets().isEmpty() ? nullptr : QApplication::allWidgets().first();

    int dpi = 96; /* default */
    if (widget)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      dpi = (int)widget->screen()->logicalDotsPerInch();
#else
      dpi = widget->logicalDpiY();
#endif
    }

    /* points = (pixels * 72) / dpi */
    point_size = (-size * 72) / dpi;
  }

  if (point_size <= 0)
    return nullptr;

  QFont* qfont = new QFont(QString::fromUtf8(typeface), point_size);
  if (!qfont)
    return nullptr;

  qfont->setBold(is_bold);
  qfont->setItalic(is_italic);
  qfont->setUnderline(is_underline);
  qfont->setStrikeOut(is_strikeout);

  QFontMetrics metrics(*qfont);
  QFontMetricsF metricsF(*qfont);

  fonts = (IqtFont*)iupArrayInc(qt_fonts);

  strcpy(fonts[i].font, font);
  fonts[i].qfont = qfont;
  fonts[i].is_underline = is_underline;
  fonts[i].is_strikeout = is_strikeout;

  fonts[i].charheight = metrics.height();
  /* Use 'x' character width to match Cocoa/GTK/Windows behavior.
   * Qt's averageCharWidth() returns actual average (e.g., 11 pixels for 10pt font)
   * which is too large for IUP's SIZE calculation (SIZE uses 1/4 char units).
   * Other platforms use approximate/narrower values. */
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  fonts[i].charwidth = metrics.horizontalAdvance('x');
#else
  fonts[i].charwidth = metrics.width('x');
#endif
  fonts[i].max_width = metrics.maxWidth();
  fonts[i].ascent = metrics.ascent();
  fonts[i].descent = metrics.descent();

  return &fonts[i];
}

static IqtFont* qtFontCreateNativeFont(Ihandle* ih, const char* value)
{
  IqtFont* qtfont = qtFindFont(value);
  if (!qtfont)
  {
    iupERROR1("Failed to create Font: %s", value);
    return nullptr;
  }

  iupAttribSet(ih, "_IUP_QTFONT", (char*)qtfont);
  return qtfont;
}

static IqtFont* qtFontGet(Ihandle* ih)
{
  IqtFont* qtfont = (IqtFont*)iupAttribGet(ih, "_IUP_QTFONT");
  if (!qtfont)
  {
    const char* fontvalue = iupGetFontValue(ih);
    qtfont = qtFontCreateNativeFont(ih, fontvalue);
    if (!qtfont)
    {
      const char* defaultfont = IupGetGlobal("DEFAULTFONT");
      qtfont = qtFontCreateNativeFont(ih, defaultfont);
    }
    if (!qtfont)
    {
      /* Last resort: create a default system font to ensure we never return NULL
       * This prevents iupdrvFontGetStringWidth from returning 0, which would cause
       * text controls to have zero natural size */
      qtfont = qtFontCreateNativeFont(ih, "Sans, 10");
    }
  }
  return qtfont;
}

/****************************************************************************
 * Qt-specific Font Functions
 ****************************************************************************/

extern "C" QFont* iupqtGetQFont(const char* value)
{
  IqtFont* qtfont = qtFindFont(value);
  if (qtfont)
    return qtfont->qfont;
  else
    return nullptr;
}

extern "C" char* iupqtGetQFontAttrib(Ihandle* ih)
{
  IqtFont* qtfont = qtFontGet(ih);
  if (qtfont)
    return (char*)qtfont->qfont;
  else
    return nullptr;
}

extern "C" char* iupqtFindQFont(QFont* qfont)
{
  /* Find font string from QFont pointer (similar to Windows iupwinFindHFont) */
  if (!qt_fonts || !qfont)
    return nullptr;

  int i, count = iupArrayCount(qt_fonts);
  IqtFont* fonts = (IqtFont*)iupArrayGetData(qt_fonts);

  for (i = 0; i < count; i++)
  {
    if (qfont == fonts[i].qfont)
      return fonts[i].font;
  }

  return nullptr;
}

extern "C" char* iupqtGetFontIdAttrib(Ihandle* ih)
{
  /* Used by IupGLCanvas for IupGLUseFont */
  /* Qt doesn't have a direct equivalent to X Font ID or HFONT */
  /* Return the QFont pointer as a string representation */
  IqtFont* qtfont = qtFontGet(ih);
  if (!qtfont)
    return nullptr;

  /* Return pointer as string - not ideal but matches the pattern */
  static char buffer[64];
  snprintf(buffer, sizeof(buffer), "%p", (void*)qtfont->qfont);
  return buffer;
}

/****************************************************************************
 * Update Widget Font
 ****************************************************************************/

extern "C" void iupqtUpdateWidgetFont(Ihandle* ih, QWidget* widget)
{
  IqtFont* qtfont = qtFontGet(ih);
  if (!qtfont)
    return;

  widget->setFont(*qtfont->qfont);
}

/****************************************************************************
 * Driver Font Functions
 ****************************************************************************/

extern "C" char* iupdrvGetSystemFont(void)
{
  static char str[200]; /* must return a static string */

  QFont font = QApplication::font();
  QFontInfo info(font);

  QString family = info.family();
  int point_size = info.pointSize();
  bool is_bold = info.bold();
  bool is_italic = info.italic();

  /* Build font string in IUP format (Pango-like) */
  sprintf(str, "%s, %s%s%d",
          family.toUtf8().constData(),
          is_bold ? "Bold " : "",
          is_italic ? "Italic " : "",
          point_size);

  return str;
}

extern "C" int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  IqtFont* qtfont = qtFontCreateNativeFont(ih, value);
  if (!qtfont)
    return 0;

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateAttribFromFont(ih);

  /* FONT attribute must be able to be set before mapping */
  /* Skip for TYPEVOID (no widget) and TYPEMENU (QAction, not QWidget) */
  if (ih->handle &&
      (ih->iclass->nativetype != IUP_TYPEVOID) &&
      (ih->iclass->nativetype != IUP_TYPEMENU))
  {
    QWidget* widget = (QWidget*)ih->handle;
    widget->setFont(*qtfont->qfont);
  }

  return 1;
}

static void qtFontGetTextSize(Ihandle* ih, IqtFont* qtfont, const char* str, int len, int* w, int* h)
{
  int max_w = 0;
  int line_count = 1;

  if (!qtfont)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = qtfont->charheight * 1;
    return;
  }

  if (h)
    line_count = iupStrLineCount(str, len);

  if (str[0])
  {
    /* Check if MARKUP is enabled (similar to GTK implementation) */
    bool use_markup = ih && iupAttribGetBoolean(ih, "MARKUP");

    if (use_markup)
    {
      /* Use QTextDocument for rich text/HTML rendering */
      QTextDocument doc;
      doc.setDefaultFont(*qtfont->qfont);
      doc.setHtml(QString::fromUtf8(str, len));

      QSizeF size = doc.size();
      max_w = (int)size.width();

      /* QTextDocument handles line breaks, so use its height */
      if (h)
        *h = (int)size.height();
    }
    else
    {
      /* Plain text measurement */
      QFontMetrics metrics(*qtfont->qfont);

      /* Handle multi-line text */
      const char* curstr = str;
      const char* nextstr;
      int l_len;
      int sum_len = 0;

      do
      {
        nextstr = iupStrNextLine(curstr, &l_len);
        if (sum_len + l_len > len)
          l_len = len - sum_len;

        if (l_len)
        {
          QString line = QString::fromUtf8(curstr, l_len);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
          int line_w = metrics.horizontalAdvance(line);
#else
          int line_w = metrics.width(line);
#endif
          max_w = iupMAX(max_w, line_w);
        }

        sum_len += l_len;
        if (sum_len >= len)
          break;

        curstr = nextstr;
      } while (*nextstr);

      if (h)
        *h = qtfont->charheight * line_count;
    }
  }

  if (w) *w = max_w;
}

extern "C" void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
  IqtFont* qtfont = qtFontGet(ih);
  if (qtfont)
    qtFontGetTextSize(ih, qtfont, str, str ? (int)strlen(str) : 0, w, h);
}

extern "C" void iupdrvFontGetTextSize(const char* font, const char* str, int len, int* w, int* h)
{
  IqtFont* qtfont = qtFindFont(font);
  if (qtfont)
    qtFontGetTextSize(nullptr, qtfont, str, len, w, h);
}

extern "C" void iupdrvFontGetFontDim(const char* font, int* max_width, int* line_height, int* ascent, int* descent)
{
  IqtFont* qtfont = qtFindFont(font);
  if (qtfont)
  {
    if (max_width)   *max_width = qtfont->max_width;
    if (line_height) *line_height = qtfont->charheight;
    if (ascent)      *ascent = qtfont->ascent;
    if (descent)     *descent = qtfont->descent;
  }
}

extern "C" int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  IqtFont* qtfont;
  const char* line_end;
  int len;
  int result;

  if (!str || str[0] == 0)
    return 0;

  qtfont = qtFontGet(ih);
  if (!qtfont)
  {
    return 0;
  }

  /* Do it only for the first line */
  line_end = strchr(str, '\n');
  if (line_end)
    len = (int)(line_end - str);
  else
    len = (int)strlen(str);

  /* Check if MARKUP is enabled */
  bool use_markup = iupAttribGetBoolean(ih, "MARKUP");

  if (use_markup)
  {
    /* Use QTextDocument for rich text/HTML */
    QTextDocument doc;
    doc.setDefaultFont(*qtfont->qfont);
    doc.setHtml(QString::fromUtf8(str, len));
    result = (int)doc.size().width();
  }
  else
  {
    /* Plain text */
    QFontMetrics metrics(*qtfont->qfont);
    QString text = QString::fromUtf8(str, len);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    result = metrics.horizontalAdvance(text);
#else
    result = metrics.width(text);
#endif
  }

  return result;
}

extern "C" void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
  IqtFont* qtfont = qtFontGet(ih);
  if (!qtfont)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  if (charwidth)  *charwidth = qtfont->charwidth;
  if (charheight) *charheight = qtfont->charheight;
}

/****************************************************************************
 * Font Initialization and Cleanup
 ****************************************************************************/

extern "C" void iupdrvFontInit(void)
{
  qt_fonts = iupArrayCreate(50, sizeof(IqtFont));
}

extern "C" void iupdrvFontFinish(void)
{
  if (!qt_fonts)
    return;

  int i, count = iupArrayCount(qt_fonts);
  IqtFont* fonts = (IqtFont*)iupArrayGetData(qt_fonts);

  for (i = 0; i < count; i++)
  {
    if (fonts[i].qfont)
    {
      delete fonts[i].qfont;
      fonts[i].qfont = nullptr;
    }
  }

  iupArrayDestroy(qt_fonts);
  qt_fonts = nullptr;
}

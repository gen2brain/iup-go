/** \file
 * \brief Qt Quick Font mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <QFontDatabase>
#include <QFontMetrics>
#include <QFontInfo>
#include <QQuickItem>
#include <QScreen>
#include <QString>
#include <QTextDocument>
#include <QtMath>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_array.h"
#include "iup_object.h"
#include "iup_drvfont.h"
#include "iup_assert.h"
#include "iup_markup.h"
}

#include "iupqml_drv.h"


/****************************************************************************
 * Font Cache Structure
 ****************************************************************************/

typedef struct _IqmlFont
{
  char font[200];
  QFont* qfont;
  int charwidth, charheight;
  int max_width, ascent, descent;
  bool is_underline;
  bool is_strikeout;
} IqmlFont;

static Iarray* qml_fonts = nullptr;

/****************************************************************************
 * Font Cache Management
 ****************************************************************************/

static IqmlFont* qmlFindFont(const char* font)
{
  int i, count;
  int size = 8;
  int is_bold = 0,
      is_italic = 0,
      is_underline = 0,
      is_strikeout = 0;
  char typeface[1024];
  const char* mapped_name;
  IqmlFont* fonts;

  if (!qml_fonts)
    return nullptr;

  count = iupArrayCount(qml_fonts);
  fonts = static_cast<IqmlFont*>(iupArrayGetData(qml_fonts));

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(font, fonts[i].font))
      return &fonts[i];
  }

  if (!iupFontParseWin(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    if (!iupFontParseX(font, typeface, sizeof(typeface), &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      if (!iupFontParsePango(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
        return nullptr;
    }
  }

  /* Map standard names to native names */
  mapped_name = iupFontGetPangoName(typeface);
  if (mapped_name)
    iupStrCopyN(typeface, sizeof(typeface), mapped_name);

  int point_size = size;
  if (size < 0)
  {
    int dpi = 96;
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen)
      dpi = static_cast<int>(screen->logicalDotsPerInch());

    point_size = (-size * 72) / dpi;
  }

  if (point_size <= 0)
    return nullptr;

  auto* qfont = new QFont(QString::fromUtf8(typeface), point_size);
  qfont->setBold(is_bold);
  qfont->setItalic(is_italic);
  qfont->setUnderline(is_underline);
  qfont->setStrikeOut(is_strikeout);

  QFontMetrics metrics(*qfont);

  fonts = static_cast<IqmlFont*>(iupArrayInc(qml_fonts));

  iupStrCopyN(fonts[i].font, sizeof(fonts[i].font), font);
  fonts[i].qfont = qfont;
  fonts[i].is_underline = is_underline;
  fonts[i].is_strikeout = is_strikeout;

  fonts[i].charheight = metrics.height();
  /* averageCharWidth() is too wide for SIZE, which counts 1/4 char units */
  fonts[i].charwidth = metrics.horizontalAdvance('x');
  fonts[i].max_width = metrics.maxWidth();
  fonts[i].ascent = metrics.ascent();
  fonts[i].descent = metrics.descent();

  return &fonts[i];
}

static IqmlFont* qmlFontCreateNativeFont(Ihandle* ih, const char* value)
{
  IqmlFont* qtfont = qmlFindFont(value);
  if (!qtfont)
  {
    iupERROR1("Failed to create Font: %s", value);
    return nullptr;
  }

  iupAttribSet(ih, "_IUP_QTFONT", reinterpret_cast<char*>(qtfont));
  return qtfont;
}

static IqmlFont* qmlFontGet(Ihandle* ih)
{
  IqmlFont* qtfont = qmlFindFont(iupGetFontValue(ih));
  if (!qtfont)
    qtfont = qmlFindFont(IupGetGlobal("DEFAULTFONT"));
  if (!qtfont)
    qtfont = qmlFindFont("Sans, 10");
  return qtfont;
}

/****************************************************************************
 * Qt-specific Font Functions
 ****************************************************************************/

IUP_DRV_API QFont* iupqmlGetQFont(const char* value)
{
  IqmlFont* qtfont = qmlFindFont(value);
  if (qtfont)
    return qtfont->qfont;
  else
    return nullptr;
}

IUP_DRV_API QFont* iupqmlGetQFontLine(const char* value, int* ascent, int* charheight)
{
  IqmlFont* qtfont = qmlFindFont(value);
  if (!qtfont)
    return nullptr;
  *ascent = qtfont->ascent;
  *charheight = qtfont->charheight;
  return qtfont->qfont;
}


/****************************************************************************
 * Update Widget Font
 ****************************************************************************/

IUP_DRV_API void iupqmlUpdateItemFont(Ihandle* ih, QObject* item)
{
  IqmlFont* qtfont = qmlFontGet(ih);
  if (!qtfont || !item)
    return;

  item->setProperty("font", QVariant::fromValue(*qtfont->qfont));
}

IUP_DRV_API QFont* iupqmlGetIhFont(Ihandle* ih)
{
  IqmlFont* qtfont = qmlFontGet(ih);
  return qtfont ? qtfont->qfont : nullptr;
}

/****************************************************************************
 * Driver Font Functions
 ****************************************************************************/

extern "C" IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char str[200];

  QFont font = QGuiApplication::font();
  QFontInfo info(font);

  QString family = info.family();
  int point_size = info.pointSize();
  bool is_bold = info.bold();
  bool is_italic = info.italic();

  snprintf(str, sizeof(str), "%s, %s%s%d",
           family.toUtf8().constData(),
           is_bold ? "Bold " : "",
           is_italic ? "Italic " : "",
           point_size);

  return str;
}

extern "C" IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  IqmlFont* qtfont = qmlFontCreateNativeFont(ih, value);
  if (!qtfont)
    return 0;

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateAttribFromFont(ih);

  if (ih->handle &&
      (ih->iclass->nativetype != IUP_TYPEVOID) &&
      (ih->iclass->nativetype != IUP_TYPEDIALOG))
  {
    auto* obj = reinterpret_cast<QObject*>(ih->handle);
    obj->setProperty("font", QVariant::fromValue(*qtfont->qfont));
  }

  return 1;
}

static void qmlFontGetTextSize(Ihandle* ih, IqmlFont* qtfont, const char* str, int len, int* w, int* h)
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
    bool use_markup = ih && iupAttribGetBoolean(ih, "MARKUP");

    if (use_markup)
    {
      char* html = iupMarkupToHtml(str);
      QTextDocument doc;
      doc.setDefaultFont(*qtfont->qfont);
      doc.setHtml(QString::fromUtf8(html));
      doc.setTextWidth(100000);
      free(html);

      if (w) *w = qCeil(doc.idealWidth());
      if (h) *h = qCeil(doc.size().height());
      return;
    }
    else
    {
      QFontMetrics metrics(*qtfont->qfont);

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
          int line_w = metrics.horizontalAdvance(line);
          max_w = iupMAX(max_w, line_w);
        }

        sum_len += l_len;
        if (sum_len >= len)
          break;

        curstr = nextstr;
      } while (*nextstr);
    }
  }

  if (w) *w = max_w;
  if (h) *h = qtfont->charheight * line_count;
}

extern "C" IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
  IqmlFont* qtfont = qmlFontGet(ih);
  if (qtfont)
    qmlFontGetTextSize(ih, qtfont, str, str ? static_cast<int>(strlen(str)) : 0, w, h);
}

extern "C" IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int* w, int* h)
{
  IqmlFont* qtfont = qmlFindFont(font);
  if (qtfont)
    qmlFontGetTextSize(nullptr, qtfont, str, len, w, h);
}

extern "C" IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int* max_width, int* line_height, int* ascent, int* descent)
{
  IqmlFont* qtfont = qmlFindFont(font);
  if (qtfont)
  {
    if (max_width)   *max_width = qtfont->max_width;
    if (line_height) *line_height = qtfont->charheight;
    if (ascent)      *ascent = qtfont->ascent;
    if (descent)     *descent = qtfont->descent;
  }
}

extern "C" IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  IqmlFont* qtfont;
  const char* line_end;
  int len;
  int result;

  if (!str || str[0] == 0)
    return 0;

  qtfont = qmlFontGet(ih);
  if (!qtfont)
  {
    return 0;
  }

  line_end = strchr(str, '\n');
  if (line_end)
    len = static_cast<int>(line_end - str);
  else
    len = static_cast<int>(strlen(str));

  bool use_markup = iupAttribGetBoolean(ih, "MARKUP");

  if (use_markup)
  {
    char* html = iupMarkupToHtml(str);
    QTextDocument doc;
    doc.setDefaultFont(*qtfont->qfont);
    doc.setHtml(QString::fromUtf8(html));
    doc.setTextWidth(100000);
    free(html);
    result = qCeil(doc.idealWidth());
  }
  else
  {
    QFontMetrics metrics(*qtfont->qfont);
    QString text = QString::fromUtf8(str, len);
    result = metrics.horizontalAdvance(text);
  }

  return result;
}

extern "C" IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
  IqmlFont* qtfont = qmlFontGet(ih);
  if (!qtfont)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  if (charwidth)  *charwidth = qtfont->charwidth;
  if (charheight) *charheight = qtfont->charheight;
}

static int qmlFontFamilyCompare(const void* a, const void* b)
{
  return iupStrCompare(*static_cast<const char* const*>(a), *static_cast<const char* const*>(b), 0, 1);
}

extern "C" IUP_SDK_API int iupdrvFontGetFamilyList(char*** list)
{
  QStringList families = QFontDatabase::families();
  int count = families.size();

  if (count == 0)
  {
    *list = nullptr;
    return 0;
  }

  *list = static_cast<char**>(malloc(count * sizeof(char*)));
  for (int i = 0; i < count; i++)
    (*list)[i] = iupStrDup(families.at(i).toUtf8().constData());

  qsort(*list, count, sizeof(char*), qmlFontFamilyCompare);

  return count;
}

/****************************************************************************
 * Font Initialization and Cleanup
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvFontInit(void)
{
  qml_fonts = iupArrayCreate(50, sizeof(IqmlFont));
}

extern "C" IUP_SDK_API void iupdrvFontFinish(void)
{
  if (!qml_fonts)
    return;

  int i, count = iupArrayCount(qml_fonts);
  auto* fonts = static_cast<IqmlFont*>(iupArrayGetData(qml_fonts));

  for (i = 0; i < count; i++)
  {
    if (fonts[i].qfont)
    {
      delete fonts[i].qfont;
      fonts[i].qfont = nullptr;
    }
  }

  iupArrayDestroy(qml_fonts);
  qml_fonts = nullptr;
}

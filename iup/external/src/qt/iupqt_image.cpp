/** \file
 * \brief Image Resource - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QPixmap>
#include <QImage>
#include <QIcon>
#include <QCursor>
#include <QBitmap>
#include <QColor>
#include <QString>
#include <QFile>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStyle>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvinfo.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Image Data Extraction
 ****************************************************************************/

extern "C" void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  QPixmap* pixmap = (QPixmap*)handle;

  if (!pixmap || pixmap->isNull())
    return;

  QImage image = pixmap->toImage();
  int w = image.width();
  int h = image.height();
  int bpp = image.depth();

  if (bpp == 8)
    return;  /* Not supported */

  /* Convert QImage to IUP format (packed, top-bottom) */
  int planesize = w * h;
  for (int y = 0; y < h; y++)
  {
    unsigned char* line_data = imgdata + y * planesize;
    const uchar* scanline = image.constScanLine(y);

    for (int x = 0; x < w; x++)
    {
      QRgb pixel = image.pixel(x, y);
      line_data[x * 3 + 0] = qRed(pixel);
      line_data[x * 3 + 1] = qGreen(pixel);
      line_data[x * 3 + 2] = qBlue(pixel);
    }
  }
}

extern "C" IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  QPixmap* pixmap = (QPixmap*)handle;

  if (!pixmap || pixmap->isNull())
    return;

  QImage image = pixmap->toImage();
  int w = image.width();
  int h = image.height();
  int bpp = image.depth();

  if (bpp == 8)
    return;

  /* Planes are separated in imgdata */
  int planesize = w * h;
  unsigned char* r = imgdata;
  unsigned char* g = imgdata + planesize;
  unsigned char* b = imgdata + 2 * planesize;
  unsigned char* a = imgdata + 3 * planesize;

  bool has_alpha = image.hasAlphaChannel();

  for (int y = 0; y < h; y++)
  {
    int lineoffset = (h - 1 - y) * w;  /* imgdata is bottom up */

    for (int x = 0; x < w; x++)
    {
      QRgb pixel = image.pixel(x, y);

      r[lineoffset + x] = qRed(pixel);
      g[lineoffset + x] = qGreen(pixel);
      b[lineoffset + x] = qBlue(pixel);

      if (has_alpha)
        a[lineoffset + x] = qAlpha(pixel);
    }
  }
}

/****************************************************************************
 * Image Creation from Raw Data
 ****************************************************************************/

extern "C" IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  QImage::Format format;
  bool has_alpha = (bpp == 32);

  if (has_alpha)
    format = QImage::Format_ARGB32;
  else
    format = QImage::Format_RGB888;

  QImage image(width, height, format);

  if (image.isNull())
    return NULL;

  /* QImage is top-bottom, imgdata is bottom-up */

  if (bpp == 8)
  {
    /* Indexed color */
    for (int y = 0; y < height; y++)
    {
      unsigned char* line_data = imgdata + (height - 1 - y) * width;

      for (int x = 0; x < width; x++)
      {
        unsigned char index = line_data[x];
        if (index < colors_count)
        {
          iupColor* c = &colors[index];
          image.setPixel(x, y, qRgba(c->r, c->g, c->b, c->a));
        }
      }
    }
  }
  else /* bpp == 24 or bpp == 32 */
  {
    /* Planes are separated in imgdata */
    int planesize = width * height;
    unsigned char* r = imgdata;
    unsigned char* g = imgdata + planesize;
    unsigned char* b = imgdata + 2 * planesize;
    unsigned char* a = imgdata + 3 * planesize;

    for (int y = 0; y < height; y++)
    {
      int lineoffset = (height - 1 - y) * width;  /* imgdata is bottom up */

      for (int x = 0; x < width; x++)
      {
        if (has_alpha)
          image.setPixel(x, y, qRgba(r[lineoffset + x], g[lineoffset + x],
                                      b[lineoffset + x], a[lineoffset + x]));
        else
          image.setPixel(x, y, qRgb(r[lineoffset + x], g[lineoffset + x],
                                     b[lineoffset + x]));
      }
    }
  }

  QPixmap* pixmap = new QPixmap();
  *pixmap = QPixmap::fromImage(image);

  /* Callback for memory monitoring */
  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
    cb(pixmap, const_cast<char*>("QPixmap"));

  return pixmap;
}

/****************************************************************************
 * Image Creation from IUP Image
 ****************************************************************************/

extern "C" void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int flat_alpha = iupAttribGetBoolean(ih, "FLAT_ALPHA");

  bpp = iupAttribGetInt(ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  /* Always use 32-bit formats to match QRgb* scanLine casting below */
  QImage::Format format;
  if (has_alpha)
    format = QImage::Format_ARGB32;  /* Use standard ARGB32 format */
  else
    format = QImage::Format_RGB32;   /* Use RGB32 instead of RGB888 for scanLine compatibility */

  QImage image(ih->currentwidth, ih->currentheight, format);

  if (image.isNull())
    return NULL;

  unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

  if (make_inactive || flat_alpha)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  if (bpp == 8)
  {
    /* Make colors inactive if requested */
    if (make_inactive)
    {
      for (int i = 0; i < colors_count; i++)
      {
        if (colors[i].a == 0)
        {
          colors[i].r = bg_r;
          colors[i].g = bg_g;
          colors[i].b = bg_b;
          colors[i].a = 255;
        }

        iupImageColorMakeInactive(&(colors[i].r), &(colors[i].g), &(colors[i].b), bg_r, bg_g, bg_b);
      }
    }

    /* Convert indexed to RGBA using QImage scanLine for correct pixel format */
    for (int y = 0; y < ih->currentheight; y++)
    {
      QRgb* scanline = (QRgb*)image.scanLine(y);
      unsigned char* line_data = imgdata + y * ih->currentwidth;

      for (int x = 0; x < ih->currentwidth; x++)
      {
        unsigned char index = line_data[x];
        iupColor* c = &colors[index];

        /* Use qRgba which handles platform byte order correctly */
        scanline[x] = qRgba(c->r, c->g, c->b, has_alpha ? c->a : 255);
      }
    }
  }
  else /* bpp == 24 or bpp == 32 */
  {
    /* Direct RGB(A) format - use QImage scanLine for correct pixel format */
    int channels = (bpp == 32) ? 4 : 3;

    for (int y = 0; y < ih->currentheight; y++)
    {
      QRgb* scanline = (QRgb*)image.scanLine(y);
      unsigned char* line_data = imgdata + y * ih->currentwidth * channels;

      if (!make_inactive && !flat_alpha && bpp == 32)
      {
        /* Fast path: direct copy for 32bpp RGBA when no processing needed */
        for (int x = 0; x < ih->currentwidth; x++)
        {
          unsigned char* src = &line_data[x * 4];
          /* Use qRgba which handles platform byte order correctly */
          scanline[x] = qRgba(src[0], src[1], src[2], src[3]);
        }
      }
      else
      {
        /* Slow path: process each pixel */
        for (int x = 0; x < ih->currentwidth; x++)
        {
          unsigned char r = line_data[x * channels + 0];
          unsigned char g = line_data[x * channels + 1];
          unsigned char b = line_data[x * channels + 2];
          unsigned char a = (bpp == 32) ? line_data[x * channels + 3] : 255;

          /* Handle flat_alpha attribute */
          if (flat_alpha && has_alpha && a != 255)
          {
            r = iupALPHABLEND(r, bg_r, a);
            g = iupALPHABLEND(g, bg_g, a);
            b = iupALPHABLEND(b, bg_b, a);
            a = 255;
          }

          if (make_inactive)
          {
            if (has_alpha && a != 255)
            {
              r = iupALPHABLEND(r, bg_r, a);
              g = iupALPHABLEND(g, bg_g, a);
              b = iupALPHABLEND(b, bg_b, a);
              a = 255;
            }

            iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
          }

          /* Use qRgba which handles platform byte order correctly */
          scanline[x] = qRgba(r, g, b, a);
        }
      }
    }
  }

  /* Convert QImage to QPixmap */
  QPixmap* pixmap = new QPixmap();
  *pixmap = QPixmap::fromImage(image);

  /* Mark as background dependent if needed */
  if (make_inactive || (has_alpha && flat_alpha))
    iupAttribSet(ih, "_IUP_BGCOLOR_DEPEND", "1");

  /* Callback for memory monitoring */
  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
    cb(pixmap, const_cast<char*>("QPixmap"));

  return pixmap;
}

extern "C" void* iupdrvImageCreateIcon(Ihandle *ih)
{
  void* handle = iupdrvImageCreateImage(ih, NULL, 0);

  if (handle)
  {
    IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
    if (cb)
      cb(handle, const_cast<char*>("ICON"));
  }

  return handle;
}

extern "C" void* iupdrvImageCreateCursor(Ihandle *ih)
{
  QPixmap* pixmap = (QPixmap*)iupdrvImageCreateImage(ih, NULL, 0);

  if (!pixmap)
    return NULL;

  /* Get hotspot */
  int hx = 0, hy = 0;
  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  /* Create cursor from pixmap */
  QCursor* cursor = new QCursor(*pixmap, hx, hy);

  delete pixmap;

  /* Callback for memory monitoring */
  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
    cb(cursor, const_cast<char*>("CURSOR"));

  return cursor;
}

/****************************************************************************
 * Image Loading
 ****************************************************************************/

extern "C" void* iupdrvImageLoad(const char* name, int type)
{
  if (!name)
    return NULL;

  (void)type;

  /* Try loading stock icons first */
  if (name[0] == 'I' && name[1] == 'U' && name[2] == 'P')
  {
    /* Stock icon - use Qt's standard icons */
    QStyle::StandardPixmap std_pixmap = QStyle::SP_CustomBase;

    /* Map IUP stock names to Qt standard pixmaps */
    if (strstr(name, "IUP_MessageError"))
      std_pixmap = QStyle::SP_MessageBoxCritical;
    else if (strstr(name, "IUP_MessageWarning"))
      std_pixmap = QStyle::SP_MessageBoxWarning;
    else if (strstr(name, "IUP_MessageInformation"))
      std_pixmap = QStyle::SP_MessageBoxInformation;
    else if (strstr(name, "IUP_MessageQuestion"))
      std_pixmap = QStyle::SP_MessageBoxQuestion;
    else if (strstr(name, "IUP_FileSave"))
      std_pixmap = QStyle::SP_DialogSaveButton;
    else if (strstr(name, "IUP_FileOpen"))
      std_pixmap = QStyle::SP_DialogOpenButton;
    else if (strstr(name, "IUP_ArrowUp"))
      std_pixmap = QStyle::SP_ArrowUp;
    else if (strstr(name, "IUP_ArrowDown"))
      std_pixmap = QStyle::SP_ArrowDown;
    else if (strstr(name, "IUP_ArrowLeft"))
      std_pixmap = QStyle::SP_ArrowLeft;
    else if (strstr(name, "IUP_ArrowRight"))
      std_pixmap = QStyle::SP_ArrowRight;

    if (std_pixmap != QStyle::SP_CustomBase)
    {
      QStyle* style = QApplication::style();
      if (style)
      {
        QPixmap pixmap = style->standardPixmap(std_pixmap);
        if (!pixmap.isNull())
        {
          QPixmap* result = new QPixmap(pixmap);

          /* Callback for memory monitoring */
          IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
          if (cb)
          {
            const char* type_str = (type == IUPIMAGE_CURSOR) ? "CURSOR" :
                                   (type == IUPIMAGE_ICON) ? "ICON" : "QPixmap";
            cb(result, (char*)type_str);
          }

          return result;
        }
      }
    }
  }

  /* Try loading from file */
  QPixmap* pixmap = new QPixmap();
  if (!pixmap->load(QString::fromUtf8(name)))
  {
    /* Failed to load */
    delete pixmap;
    return NULL;
  }

  /* Callback for memory monitoring */
  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
  {
    const char* type_str = (type == IUPIMAGE_CURSOR) ? "CURSOR" :
                           (type == IUPIMAGE_ICON) ? "ICON" : "QPixmap";
    cb(pixmap, (char*)type_str);
  }

  return pixmap;
}

/****************************************************************************
 * Image Information
 ****************************************************************************/

extern "C" int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  QPixmap* pixmap = (QPixmap*)handle;

  if (!pixmap || pixmap->isNull())
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return 0;
  }

  if (w) *w = pixmap->width();
  if (h) *h = pixmap->height();

  if (bpp)
  {
    QImage image = pixmap->toImage();
    int depth = image.depth();

    /* Normalize bpp */
    if (image.hasAlphaChannel())
      *bpp = iupImageNormBpp(32);
    else if (depth > 8)
      *bpp = iupImageNormBpp(24);
    else
      *bpp = iupImageNormBpp(8);
  }

  return 1;
}

extern "C" IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  /* QPixmap/QImage are typically 24 or 32 bpp */
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

/****************************************************************************
 * Image Destruction
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  if (!handle)
    return;

  const char* type_str = NULL;

  if (type == IUPIMAGE_CURSOR)
  {
    type_str = "CURSOR";
    QCursor* cursor = (QCursor*)handle;

    /* Callback for memory monitoring before destruction */
    IFvs cb = (IFvs)IupGetFunction("IMAGEDESTROY_CB");
    if (cb)
      cb(handle, (char*)type_str);

    delete cursor;
  }
  else
  {
    if (type == IUPIMAGE_ICON)
      type_str = "ICON";
    else
      type_str = "QPixmap";

    QPixmap* pixmap = (QPixmap*)handle;

    /* Callback for memory monitoring before destruction */
    IFvs cb = (IFvs)IupGetFunction("IMAGEDESTROY_CB");
    if (cb)
      cb(handle, (char*)type_str);

    delete pixmap;
  }
}

/** \file
 * \brief Qt System Tray Driver - Icon Conversion for Native Implementations
 *
 * This file provides QPixmap to ARGB pixel conversion for native tray
 * implementations (SNI on Linux, Win32 on Windows, Cocoa on macOS).
 *
 * See Copyright Notice in "iup.h"
 */

#include <QPixmap>
#include <QImage>
#include <QString>

extern "C" {
#include "iup.h"
#include "iup_image.h"
}

extern "C" int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  QPixmap* pixmap = nullptr;
  QImage image;
  int w, h;
  unsigned char* dst;

  (void)ih;

  if (!value)
    return 0;

  pixmap = (QPixmap*)iupImageGetIcon(value);
  if (!pixmap || pixmap->isNull())
  {
    QPixmap file_pixmap(QString::fromUtf8(value));
    if (file_pixmap.isNull())
      return 0;

    image = file_pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  }
  else
  {
    image = pixmap->toImage().convertToFormat(QImage::Format_ARGB32);
  }

  if (image.isNull())
    return 0;

  w = image.width();
  h = image.height();

  dst = (unsigned char*)malloc(w * h * 4);
  if (!dst)
    return 0;

  for (int y = 0; y < h; y++)
  {
    const QRgb* src_line = (const QRgb*)image.constScanLine(y);
    for (int x = 0; x < w; x++)
    {
      QRgb pixel = src_line[x];
      int offset = (y * w + x) * 4;

      dst[offset + 0] = qAlpha(pixel);
      dst[offset + 1] = qRed(pixel);
      dst[offset + 2] = qGreen(pixel);
      dst[offset + 3] = qBlue(pixel);
    }
  }

  *width = w;
  *height = h;
  *pixels = dst;

  return 1;
}

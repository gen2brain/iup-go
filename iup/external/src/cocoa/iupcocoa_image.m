/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"


/* scanlines are 4-byte aligned */
static int CalculateBytesPerRow(int width, int bytes_per_pixel)
{
  int pitch = width * bytes_per_pixel;
  switch (bytes_per_pixel * 8)
  {
    case 1:
      pitch = (pitch + 7) / 8;
      break;
    case 4:
      pitch = (pitch + 1) / 2;
      break;
    default:
      break;
  }
  pitch = (pitch + 3) & ~3;   /* 4-byte aligning */
  return pitch;
}

IUP_DRV_API int iupcocoaImageCalculateBytesPerRow(int width, int bytes_per_pixel)
{
  return CalculateBytesPerRow(width, bytes_per_pixel);
}

#ifdef GNUSTEP
/* Opal's NSBitmapImageRep has no -CGImage, so read the samples by hand */
static int cocoaImageBitmapToRGBA(NSBitmapImageRep* bitmap, unsigned char* rgba, int w, int h)
{
  const unsigned char* data = [bitmap bitmapData];
  NSInteger src_stride = [bitmap bytesPerRow];
  NSInteger samples = [bitmap samplesPerPixel];
  NSInteger bits = [bitmap bitsPerSample];
  NSBitmapFormat format = [bitmap bitmapFormat];
  int premul = (format & NSBitmapFormatAlphaNonpremultiplied) == 0;
  int high = (format & NSBitmapFormatSixteenBitLittleEndian) ? 1 : 0;
  int step = (int)(samples * (bits / 8));
  int x, y;

  if (!data || [bitmap isPlanar] || samples < 3 || (bits != 8 && bits != 16) ||
      [bitmap bitsPerPixel] != samples * bits)
    return 0;

  for (y = 0; y < h; y++)
  {
    const unsigned char* src_line = data + (size_t)y * src_stride;
    unsigned char* dest_line = rgba + (size_t)y * w * 4;

    for (x = 0; x < w; x++)
    {
      const unsigned char* src = src_line + (size_t)x * step;
      unsigned char* dest = dest_line + x * 4;
      unsigned int c[4];
      int i;

      for (i = 0; i < 4; i++)
      {
        if (i >= samples)
          c[i] = 255;
        else if (bits == 16)
          c[i] = src[i * 2 + high];   /* the most significant byte is the 8 bit value */
        else
          c[i] = src[i];
      }

      if (premul && c[3] != 0 && c[3] != 255)
      {
        for (i = 0; i < 3; i++)
        {
          c[i] = (c[i] * 255 + c[3] / 2) / c[3];
          if (c[i] > 255) c[i] = 255;
        }
      }

      dest[0] = (unsigned char)c[0];
      dest[1] = (unsigned char)c[1];
      dest[2] = (unsigned char)c[2];
      dest[3] = (unsigned char)c[3];
    }
  }

  return 1;
}
#endif

/* The output format is packed RGB(A), top-down, matching the IUP image data format. */
static void cocoaImageGetData(void* handle, unsigned char* out_img_data)
{
  if (!handle)
    return;

  if (![(__bridge id)handle isKindOfClass:[NSImage class]])
    return;

  NSImage* ns_image = (__bridge NSImage*)handle;
  NSBitmapImageRep* bitmap = nil;

  for (NSImageRep* rep in [ns_image representations])
  {
    if ([rep isKindOfClass:[NSBitmapImageRep class]])
    {
      bitmap = (NSBitmapImageRep*)rep;
      break;
    }
  }

  if (bitmap == nil)
  {
    CGImageRef cg_image = [ns_image CGImageForProposedRect:nil context:nil hints:nil];
    if (cg_image)
    {
      bitmap = [[[NSBitmapImageRep alloc] initWithCGImage:cg_image] autorelease];
    }
  }

  if (bitmap == nil)
    return;

  NSInteger w = [bitmap pixelsWide];
  NSInteger h = [bitmap pixelsHigh];
  int channels = [bitmap hasAlpha] ? 4 : 3;   /* must match the bpp iupdrvImageGetInfo reports */

  size_t rgba_stride = (size_t)w * 4;
  unsigned char* rgba = (unsigned char*)calloc(rgba_stride * h, 1);
  if (!rgba)
    return;

#ifdef GNUSTEP
  if (!cocoaImageBitmapToRGBA(bitmap, rgba, (int)w, (int)h))
  {
    free(rgba);
    return;
  }
#else
  CGImageRef cg_image = [bitmap CGImage];
  if (!cg_image)
  {
    free(rgba);
    return;
  }

  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGContextRef ctx = CGBitmapContextCreate(rgba, w, h, 8, rgba_stride, cs,
    kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  CGColorSpaceRelease(cs);
  if (!ctx)
  {
    free(rgba);
    return;
  }
  CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg_image);
  CGContextRelease(ctx);

  /* the context is pre-multiplied, IUP image data is not */
  {
    size_t i, count = (size_t)w * h;
    for (i = 0; i < count; i++)
    {
      unsigned char* p = rgba + i * 4;
      unsigned int a = p[3];
      int j;

      if (a == 0 || a == 255)
        continue;

      for (j = 0; j < 3; j++)
      {
        unsigned int v = (p[j] * 255 + a / 2) / a;
        p[j] = (unsigned char)(v > 255 ? 255 : v);
      }
    }
  }
#endif

  for (int y = 0; y < h; y++)
  {
    unsigned char* src_line = rgba + y * rgba_stride;
    unsigned char* dest_line = out_img_data + y * ((size_t)w * channels);
    for (int x = 0; x < w; x++)
    {
      dest_line[x * channels + 0] = src_line[x * 4 + 0];
      dest_line[x * channels + 1] = src_line[x * 4 + 1];
      dest_line[x * channels + 2] = src_line[x * 4 + 2];
      if (channels == 4)
        dest_line[x * channels + 3] = src_line[x * 4 + 3];
    }
  }

  free(rgba);
}

IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* out_img_data)
{
  @autoreleasepool {
    cocoaImageGetData(handle, out_img_data);
  }
}


static NSBitmapImageRep* cocoaImageCreateBitmapRep(int width, int height, int bpp)
{
  if (bpp == 32 || bpp == 8)
  {
    return [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                   pixelsWide:width pixelsHigh:height bitsPerSample:8
                                              samplesPerPixel:4 hasAlpha:YES isPlanar:NO
                                               colorSpaceName:NSDeviceRGBColorSpace
                                                 bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                                                  bytesPerRow:CalculateBytesPerRow(width, 4)
                                                 bitsPerPixel:32];
  }
  else if (bpp == 24)
  {
    return [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                   pixelsWide:width pixelsHigh:height bitsPerSample:8
                                              samplesPerPixel:3 hasAlpha:NO isPlanar:NO
                                               colorSpaceName:NSDeviceRGBColorSpace
                                                 bitmapFormat:0
                                                  bytesPerRow:CalculateBytesPerRow(width, 3)
                                                 bitsPerPixel:24];
  }
  return nil;
}

static NSImage* cocoaImageWrapBitmapRep(NSBitmapImageRep* bitmap, int width, int height)
{
  if (!bitmap)
    return nil;

  NSImage* ns_image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
  [ns_image addRepresentation:bitmap];
  [bitmap release];
  return ns_image;
}



static int cocoaImageGetRawInfo(void* handle, int* w, int* h, int* bpp, iupColor* colors, int* colors_count)
{
  /* indexed images are converted to RGB(A) automatically, so there is no palette to read */
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int* w, int* h, int* bpp, iupColor* colors, int* colors_count)
{
  @autoreleasepool {
    return cocoaImageGetRawInfo(handle, w, h, bpp, colors, colors_count);
  }
}

static NSImage* iupCocoaCreateNSImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bpp = iupAttribGetInt(ih, "BPP");
  unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  unsigned char bg_r=0, bg_g=0, bg_b=0;
  int flat_alpha = iupAttribGetBoolean(ih, "FLAT_ALPHA");

  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  NSBitmapImageRep* bitmap_image = cocoaImageCreateBitmapRep(width, height, bpp);
  if (!bitmap_image)
    return NULL;

  unsigned char* pixels = [bitmap_image bitmapData];
  NSInteger bytes_per_row = [bitmap_image bytesPerRow];

  if (bpp == 32)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* dest_line = pixels + y * bytes_per_row;
      unsigned char* src_line = imgdata + y * width * 4;
      for (int x = 0; x < width; x++)
      {
        unsigned char* dest_pixel = dest_line + x * 4;
        unsigned char* src_pixel = src_line + x * 4;
        unsigned char s_r = src_pixel[0];
        unsigned char s_g = src_pixel[1];
        unsigned char s_b = src_pixel[2];
        unsigned char s_a = src_pixel[3];

        if (flat_alpha && s_a != 255)
        {
          s_r = iupALPHABLEND(s_r, bg_r, s_a);
          s_g = iupALPHABLEND(s_g, bg_g, s_a);
          s_b = iupALPHABLEND(s_b, bg_b, s_a);
          s_a = 255;
        }

        if (make_inactive)
        {
          iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
        }

        dest_pixel[0] = s_r;
        dest_pixel[1] = s_g;
        dest_pixel[2] = s_b;
        dest_pixel[3] = s_a;
      }
    }
  }
  else if (bpp == 24)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* dest_line = pixels + y * bytes_per_row;
      unsigned char* src_line = imgdata + y * width * 3;
      for (int x = 0; x < width; x++)
      {
        unsigned char* dest_pixel = dest_line + x * 3;
        unsigned char* src_pixel = src_line + x * 3;
        unsigned char s_r = src_pixel[0];
        unsigned char s_g = src_pixel[1];
        unsigned char s_b = src_pixel[2];

        if (make_inactive)
        {
          iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
        }

        dest_pixel[0] = s_r;
        dest_pixel[1] = s_g;
        dest_pixel[2] = s_b;
      }
    }
  }
  else if (bpp == 8)
  {
    int colors_count = 0;
    iupColor colors[256];
    int has_alpha = iupImageInitColorTable(ih, colors, &colors_count);

    for (int y = 0; y < height; y++)
    {
      unsigned char* dest_line = pixels + y * bytes_per_row;
      unsigned char* src_line = imgdata + y * width;
      for (int x = 0; x < width; x++)
      {
        unsigned char* dest_pixel = dest_line + x * 4;
        unsigned char index = src_line[x];
        iupColor* c = &colors[index];

        unsigned char s_r = c->r;
        unsigned char s_g = c->g;
        unsigned char s_b = c->b;
        unsigned char s_a = has_alpha ? c->a : 255;

        if (flat_alpha && s_a != 255)
        {
          s_r = iupALPHABLEND(s_r, bg_r, s_a);
          s_g = iupALPHABLEND(s_g, bg_g, s_a);
          s_b = iupALPHABLEND(s_b, bg_b, s_a);
          s_a = 255;
        }

        if (make_inactive)
        {
          iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
        }

        dest_pixel[0] = s_r;
        dest_pixel[1] = s_g;
        dest_pixel[2] = s_b;
        dest_pixel[3] = s_a;
      }
    }
  }

  NSImage* ns_image = cocoaImageWrapBitmapRep(bitmap_image, width, height);

  if (make_inactive || flat_alpha)
  {
    iupAttribSetStr(ih, "_IUP_BGCOLOR_DEPEND", "1");
  }

  return ns_image;
}

static void* cocoaImageCreateImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
  NSImage* ns_image = iupCocoaCreateNSImage(ih, bgcolor, make_inactive);

  if (ns_image)
  {
    IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
    if (cb)
      cb(ns_image, "NSImage");
  }

  return ns_image;
}

IUP_SDK_API void* iupdrvImageCreateImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
  @autoreleasepool {
    return cocoaImageCreateImage(ih, bgcolor, make_inactive);
  }
}

static void* cocoaImageCreateIcon(Ihandle* ih)
{
  NSImage* ns_image = iupCocoaCreateNSImage(ih, NULL, 0);

  if (ns_image)
  {
    IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
    if (cb)
      cb(ns_image, "ICON");
  }

  return ns_image;
}

IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle* ih)
{
  @autoreleasepool {
    return cocoaImageCreateIcon(ih);
  }
}

static void* cocoaImageCreateCursor(Ihandle* ih)
{
  int hx=0, hy=0;
  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  NSImage* image = iupCocoaCreateNSImage(ih, NULL, 0);
  if (!image)
  {
    return NULL;
  }

  NSSize size = [image size];
  NSPoint hotSpot = NSMakePoint(hx, size.height - 1 - hy);

  NSCursor* cursor = [[NSCursor alloc] initWithImage:image hotSpot:hotSpot];
  [image release];

  if (cursor)
  {
    IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
    if (cb)
      cb(cursor, "CURSOR");
  }

  return cursor;
}

IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle* ih)
{
  @autoreleasepool {
    return cocoaImageCreateCursor(ih);
  }
}

static void* cocoaImageLoad(const char* name, int type)
{
  if (!name || (name[0] == '\0'))
  {
    return NULL;
  }

  NSImage* the_image = nil;
  NSString* ns_name = [NSString stringWithUTF8String:name];

  the_image = [[NSImage alloc] initWithContentsOfFile:ns_name];

  if (nil == the_image)
  {
    NSString* resource_path = [[NSBundle mainBundle] resourcePath];
    NSString* the_path = [resource_path stringByAppendingPathComponent:ns_name];
    the_image = [[NSImage alloc] initWithContentsOfFile:the_path];
  }

  if (nil == the_image)
  {
    NSString* bundle_path = [[NSBundle mainBundle] bundlePath];
    bundle_path = [bundle_path stringByDeletingLastPathComponent];
    NSString* the_path = [bundle_path stringByAppendingPathComponent:ns_name];
    the_image = [[NSImage alloc] initWithContentsOfFile:the_path];
  }

  if (nil == the_image)
  {
    return NULL;
  }

  if ([[the_image representations] count] > 0)
  {
    id rep = [[the_image representations] objectAtIndex:0];
    if ([rep isKindOfClass:[NSBitmapImageRep class]])
    {
      NSBitmapImageRep* bitmap_rep = (NSBitmapImageRep*)rep;
      NSSize image_size = NSMakeSize([bitmap_rep pixelsWide], [bitmap_rep pixelsHigh]);
      [the_image setSize:image_size];
    }
  }

  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
  {
    const char* type_str = (type == IUPIMAGE_CURSOR) ? "CURSOR" :
                           (type == IUPIMAGE_ICON) ? "ICON" : "NSImage";
    cb(the_image, (char*)type_str);
  }

  return (void*)the_image;
}

IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
  @autoreleasepool {
    return cocoaImageLoad(name, type);
  }
}

static int cocoaImageGetInfo(void* handle, int* w, int* h, int* bpp)
{
  if (w) *w = 0;
  if (h) *h = 0;
  if (bpp) *bpp = 0;

  if (NULL == handle)
    return 0;

  if (![(__bridge id)handle isKindOfClass:[NSImage class]])
    return 0;

  NSImage* image = (__bridge NSImage*)handle;
  NSBitmapImageRep* bitmap = nil;

  for(NSImageRep* image_rep in [image representations])
  {
    if ([image_rep isKindOfClass:[NSBitmapImageRep class]])
    {
      bitmap = (NSBitmapImageRep*)image_rep;
      break;
    }
  }

  if (bitmap == nil)
  {
    CGImageRef cg_image = [image CGImageForProposedRect:nil context:nil hints:nil];
    if (cg_image)
    {
      bitmap = [[[NSBitmapImageRep alloc] initWithCGImage:cg_image] autorelease];
    }
  }

  if (bitmap == nil)
    return 0;

  if (w) *w = (int)[bitmap pixelsWide];
  if (h) *h = (int)[bitmap pixelsHigh];
  if (bpp) *bpp = [bitmap hasAlpha] ? 32 : 24;   /* the data is handed over as 8 bits per channel */
  return 1;
}

IUP_SDK_API int iupdrvImageGetInfo(void* handle, int* w, int* h, int* bpp)
{
  @autoreleasepool {
    return cocoaImageGetInfo(handle, w, h, bpp);
  }
}

static void cocoaImageDestroy(void* handle, int type)
{
  const char* type_str = (type == IUPIMAGE_CURSOR) ? "CURSOR" :
                         (type == IUPIMAGE_ICON) ? "ICON" : "NSImage";

  IFvs cb = (IFvs)IupGetFunction("IMAGEDESTROY_CB");
  if (cb)
    cb(handle, (char*)type_str);

  [((__bridge id)handle) release];
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  @autoreleasepool {
    cocoaImageDestroy(handle, type);
  }
}

static unsigned char* iCocoaImageExpandPalette(unsigned char* imgdata, int width, int height, iupColor* colors, int colors_count)
{
  size_t count = (size_t)width * height;
  int i;
  unsigned char* rgba = (unsigned char*)malloc(count * 4);
  if (!rgba) return NULL;

  (void)colors_count;

  for (i = 0; i < count; i++)
  {
    int idx = imgdata[i];
    rgba[i * 4]     = colors[idx].r;
    rgba[i * 4 + 1] = colors[idx].g;
    rgba[i * 4 + 2] = colors[idx].b;
    rgba[i * 4 + 3] = colors[idx].a;
  }

  return rgba;
}

static NSBitmapImageFileType iCocoaImageGetFileType(const char* format)
{
  if (iupStrEqualNoCase(format, "PNG"))  return NSBitmapImageFileTypePNG;
  if (iupStrEqualNoCase(format, "JPEG")) return NSBitmapImageFileTypeJPEG;
  if (iupStrEqualNoCase(format, "BMP"))  return NSBitmapImageFileTypeBMP;
  return (NSBitmapImageFileType)-1;
}

static NSData* iCocoaImageEncode(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format)
{
  unsigned char* data = imgdata;
  int channels, samplesPerPixel;
  BOOL hasAlpha;
  NSBitmapImageRep* bitmap;
  NSDictionary* props = nil;
  NSBitmapImageFileType fileType = iCocoaImageGetFileType(format);
  NSData* result;

  if ((int)fileType == -1) return nil;

  if (bpp == 8)
  {
    data = iCocoaImageExpandPalette(imgdata, width, height, colors, colors_count);
    if (!data) return nil;
    bpp = 32;
  }

  channels = (bpp == 32) ? 4 : 3;
  samplesPerPixel = channels;
  hasAlpha = (channels == 4) ? YES : NO;

  bitmap = [[NSBitmapImageRep alloc]
    initWithBitmapDataPlanes:NULL
    pixelsWide:width
    pixelsHigh:height
    bitsPerSample:8
    samplesPerPixel:samplesPerPixel
    hasAlpha:hasAlpha
    isPlanar:NO
    colorSpaceName:NSCalibratedRGBColorSpace
    bytesPerRow:width * channels
    bitsPerPixel:channels * 8];

  if (bitmap)
  {
    unsigned char* bitmapData = [bitmap bitmapData];
    memcpy(bitmapData, data, width * height * channels);

    if (iupStrEqualNoCase(format, "JPEG"))
    {
      const char* q = IupGetGlobal("IMAGESAVEQUALITY");
      float quality = 0.85f;
      if (q) quality = (float)atof(q) / 100.0f;
      props = @{NSImageCompressionFactor: @(quality)};
    }

    result = [bitmap representationUsingType:fileType properties:(props ? props : @{})];
    [bitmap release];
  }
  else
    result = nil;

  if (data != imgdata) free(data);

  return result;
}

static int cocoaImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  @autoreleasepool {
    NSData* data = iCocoaImageEncode(imgdata, width, height, bpp, colors, colors_count, format);
    if (!data) return 0;

    NSString* path = [NSString stringWithUTF8String:filename];
    return [data writeToFile:path atomically:YES] ? 1 : 0;
  }
}

IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  @autoreleasepool {
    return cocoaImageSave(imgdata, width, height, bpp, colors, colors_count, filename, format);
  }
}

static unsigned char* cocoaImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  @autoreleasepool {
    NSData* data = iCocoaImageEncode(imgdata, width, height, bpp, colors, colors_count, format);
    if (!data) return NULL;

    *size = (int)[data length];
    unsigned char* result = (unsigned char*)malloc(*size);
    if (!result) return NULL;

    memcpy(result, [data bytes], *size);
    return result;
  }
}

IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  @autoreleasepool {
    return cocoaImageSaveToBuffer(imgdata, width, height, bpp, colors, colors_count, format, size);
  }
}

static int cocoaCopyRepPixels(NSBitmapImageRep* rep, int* width, int* height, unsigned char** pixels)
{
  int w = (int)[rep pixelsWide];
  int h = (int)[rep pixelsHigh];
  int spp = (int)[rep samplesPerPixel];
  int bpp = (int)[rep bitsPerPixel] / 8;
  int stride = (int)[rep bytesPerRow];
  NSBitmapFormat format = [rep bitmapFormat];
  unsigned char* src = [rep bitmapData];
  unsigned char* dst;
  int premultiplied, x, y;

  if (!src || w <= 0 || h <= 0 || [rep isPlanar] || [rep bitsPerSample] != 8 ||
      (spp != 3 && spp != 4) || bpp < spp || (format & NSBitmapFormatAlphaFirst))
    return 0;

  dst = (unsigned char*)malloc((size_t)w * h * 4);
  if (!dst)
    return 0;

  premultiplied = spp == 4 && !(format & NSBitmapFormatAlphaNonpremultiplied);
  for (y = 0; y < h; y++)
  {
    unsigned char* p = src + (size_t)y * stride;
    unsigned char* q = dst + (size_t)y * w * 4;
    for (x = 0; x < w; x++, p += bpp, q += 4)
    {
      unsigned char a = spp == 4 ? p[3] : 255;
      unsigned char r = p[0], g = p[1], b = p[2];
      if (premultiplied && a != 0 && a != 255)
      {
        r = (unsigned char)((r * 255 + a / 2) / a);
        g = (unsigned char)((g * 255 + a / 2) / a);
        b = (unsigned char)((b * 255 + a / 2) / a);
      }
      q[0] = a;
      q[1] = r;
      q[2] = g;
      q[3] = b;
    }
  }

  *width = w;
  *height = h;
  *pixels = dst;
  return 1;
}

static int cocoaGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  NSImage* image;
  NSBitmapImageRep* rep;

  (void)ih;

  if (!value)
    return 0;

  image = (NSImage*)iupImageGetIcon(value);
  if (!image)
    return 0;

  for (NSImageRep* candidate in [image representations])
  {
    if ([candidate isKindOfClass:[NSBitmapImageRep class]] &&
        cocoaCopyRepPixels((NSBitmapImageRep*)candidate, width, height, pixels))
      return 1;
  }

#ifdef GNUSTEP
  rep = [NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
  return rep ? cocoaCopyRepPixels(rep, width, height, pixels) : 0;
#else
  NSSize size = [image size];
  int w = (int)size.width;
  int h = (int)size.height;
  int ok;

  if (w <= 0 || h <= 0)
    return 0;

  rep = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:NULL
                    pixelsWide:w
                    pixelsHigh:h
                 bitsPerSample:8
               samplesPerPixel:4
                      hasAlpha:YES
                      isPlanar:NO
                colorSpaceName:NSDeviceRGBColorSpace
                   bytesPerRow:w * 4
                  bitsPerPixel:32];

  if (!rep)
    return 0;

  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:
      [NSGraphicsContext graphicsContextWithBitmapImageRep:rep]];
  [image drawInRect:NSMakeRect(0, 0, w, h)
           fromRect:NSZeroRect
          operation:NSCompositingOperationCopy
           fraction:1.0];
  [NSGraphicsContext restoreGraphicsState];

  ok = cocoaCopyRepPixels(rep, width, height, pixels);
  [rep release];
  return ok;
#endif
}

IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  @autoreleasepool {
    return cocoaGetIconPixels(ih, value, width, height, pixels);
  }
}

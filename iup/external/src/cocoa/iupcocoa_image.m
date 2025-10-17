/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"


/* Calculate the pad-aligned scanline width of a surface. Surfaces are 4-byte aligned for performance. */
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

int iupCocoaImageCalculateBytesPerRow(int width, int bytes_per_pixel)
{
  return CalculateBytesPerRow(width, bytes_per_pixel);
}

// The output format is packed RGB(A), top-down, matching the IUP image data format.
void iupdrvImageGetData(void* handle, unsigned char* out_img_data)
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
  NSInteger samplesPerPixel = [bitmap samplesPerPixel];
  NSInteger bytesPerRow = [bitmap bytesPerRow];
  unsigned char* bitmap_data = [bitmap bitmapData];

  if (samplesPerPixel < 3)
    return;

  int channels = (int)samplesPerPixel;
  int dest_line_size = (int)w * channels;

  for (int y = 0; y < h; y++)
  {
    unsigned char* src_line = bitmap_data + y * bytesPerRow;
    unsigned char* dest_line = out_img_data + y * dest_line_size;
    memcpy(dest_line, src_line, dest_line_size);
  }
}

// The output format is separated RGB(A) planes, bottom-up, matching the IM image data format.
IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  if (!handle)
    return;

  if (![(__bridge id)handle isKindOfClass:[NSImage class]])
    return;

  NSImage* image = (__bridge NSImage*)handle;
  NSBitmapImageRep *bitmap = nil;

  if ([[image representations] count] > 0)
  {
    id rep = [[image representations] objectAtIndex:0];
    if ([rep isKindOfClass:[NSBitmapImageRep class]])
    {
      bitmap = (NSBitmapImageRep*)rep;
    }
  }

  if (bitmap == nil)
    return;

  NSInteger w = [bitmap pixelsWide];
  NSInteger h = [bitmap pixelsHigh];
  NSInteger bpp = [bitmap bitsPerPixel];
  NSInteger planesize = w * h;

  unsigned char *bits = [bitmap bitmapData];
  unsigned char *red   = imgdata;
  unsigned char *green = imgdata + planesize;
  unsigned char *blue  = imgdata + 2 * planesize;
  unsigned char *alpha = (bpp == 32) ? imgdata + 3 * planesize : NULL;

  NSInteger bytesPerRow = [bitmap bytesPerRow];
  NSInteger samplesPerPixel = [bitmap samplesPerPixel];

  for (int y = 0; y < h; y++)
  {
    unsigned char* src_line = bits + y * bytesPerRow;
    int dest_line_offset = (int)(h - 1 - y) * (int)w;

    for (int x = 0; x < w; x++)
    {
      unsigned char* src_pixel = src_line + x * samplesPerPixel;
      int dest_offset = dest_line_offset + x;

      if (samplesPerPixel >= 3)
      {
        red[dest_offset]   = src_pixel[0];
        green[dest_offset] = src_pixel[1];
        blue[dest_offset]  = src_pixel[2];
      }

      if (alpha && samplesPerPixel == 4)
      {
        alpha[dest_offset] = src_pixel[3];
      }
    }
  }
}

static NSBitmapImageRep* iupCocoaImageNSBitmapImageRepFromRawData(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  NSBitmapImageRep* bitmap_image = nil;

  if (bpp == 32 || bpp == 8) // For 8bpp, we create a 32bpp image
  {
    bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                           pixelsWide:width pixelsHigh:height bitsPerSample:8
                                                      samplesPerPixel:4 hasAlpha:YES isPlanar:NO
                                                       colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                                                          bytesPerRow:CalculateBytesPerRow(width, 4)
                                                         bitsPerPixel:32];
  }
  else if (bpp == 24)
  {
    bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                           pixelsWide:width pixelsHigh:height bitsPerSample:8
                                                      samplesPerPixel:3 hasAlpha:NO isPlanar:NO
                                                       colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:0
                                                          bytesPerRow:CalculateBytesPerRow(width, 3)
                                                         bitsPerPixel:24];
  }
  else
  {
    return NULL;
  }

  unsigned char *pixels = [bitmap_image bitmapData];
  NSInteger bytesPerRow = [bitmap_image bytesPerRow];
  int planesize = width * height;

  if (bpp == 32 || bpp == 24)
  {
    unsigned char *r_plane = imgdata;
    unsigned char *g_plane = imgdata + planesize;
    unsigned char *b_plane = imgdata + 2 * planesize;
    unsigned char *a_plane = (bpp == 32) ? imgdata + 3 * planesize : NULL;
    int channels = (bpp == 32) ? 4 : 3;

    for (int y = 0; y < height; y++)
    {
      // Source imgdata is bottom-up
      int src_line_offset = (height - 1 - y) * width;
      // Destination NSBitmapImageRep is top-down
      unsigned char* dest_pixel_line = pixels + y * bytesPerRow;

      for (int x = 0; x < width; x++)
      {
        unsigned char* dest_pixel = dest_pixel_line + x * channels;
        dest_pixel[0] = r_plane[src_line_offset + x];
        dest_pixel[1] = g_plane[src_line_offset + x];
        dest_pixel[2] = b_plane[src_line_offset + x];
        if (channels == 4 && a_plane)
        {
          dest_pixel[3] = a_plane[src_line_offset + x];
        }
      }
    }
  }
  else if (bpp == 8)
  {
    for (int y = 0; y < height; y++)
    {
      // Source imgdata is bottom-up
      int src_line_offset = (height - 1 - y) * width;
      // Destination NSBitmapImageRep is top-down
      unsigned char* dest_pixel_line = pixels + y * bytesPerRow;

      for (int x = 0; x < width; x++)
      {
        unsigned char* dest_pixel = dest_pixel_line + x * 4;
        unsigned char index = imgdata[src_line_offset + x];
        iupColor* c = &colors[index];
        dest_pixel[0] = c->r;
        dest_pixel[1] = c->g;
        dest_pixel[2] = c->b;
        dest_pixel[3] = 255; // IM palettes do not have alpha
      }
    }
  }

  return bitmap_image;
}

static NSImage* iupCocoaImageNSImageFromRawData(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  NSBitmapImageRep* bitmap_image = iupCocoaImageNSBitmapImageRepFromRawData(width, height, bpp, colors, colors_count, imgdata);
  if (nil == bitmap_image)
  {
    return nil;
  }

  NSImage* ns_image = [[NSImage alloc] initWithSize:NSMakeSize(width,height)];
  if (nil == ns_image)
  {
    [bitmap_image release];
    return nil;
  }

  [ns_image addRepresentation:bitmap_image];
  [bitmap_image release];

  return ns_image;
}

NSBitmapImageRep* iupCocoaImageNSBitmapImageRepFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  NSBitmapImageRep* bitmap_image = nil;

  if (bpp == 32 || bpp == 8)
  {
    bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                           pixelsWide:width pixelsHigh:height bitsPerSample:8
                                                      samplesPerPixel:4 hasAlpha:YES isPlanar:NO
                                                       colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                                                          bytesPerRow:CalculateBytesPerRow(width, 4)
                                                         bitsPerPixel:32];
  }
  else if (bpp == 24)
  {
    bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                           pixelsWide:width pixelsHigh:height bitsPerSample:8
                                                      samplesPerPixel:3 hasAlpha:NO isPlanar:NO
                                                       colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:0
                                                          bytesPerRow:CalculateBytesPerRow(width, 3)
                                                         bitsPerPixel:24];
  }
  else
  {
    return NULL;
  }

  unsigned char *pixels = [bitmap_image bitmapData];
  NSInteger bytesPerRow = [bitmap_image bytesPerRow];

  if (bpp == 32 || bpp == 24)
  {
    int channels = (bpp == 32) ? 4 : 3;
    int src_line_size = width * channels;

    for (int y = 0; y < height; y++)
    {
      unsigned char* dest_line = pixels + y * bytesPerRow;
      unsigned char* src_line = imgdata + y * src_line_size;
      memcpy(dest_line, src_line, src_line_size);
    }
  }
  else if (bpp == 8)
  {
    int has_alpha = 0;
    for (int i = 0; i < colors_count; i++)
    {
      if (colors[i].a < 255)
      {
        has_alpha = 1;
        break;
      }
    }

    for (int y = 0; y < height; y++)
    {
      unsigned char* dest_line = pixels + y * bytesPerRow;
      unsigned char* src_line = imgdata + y * width;

      for (int x = 0; x < width; x++)
      {
        unsigned char* dest_pixel = dest_line + x * 4;
        unsigned char index = src_line[x];
        iupColor* c = &colors[index];
        dest_pixel[0] = c->r;
        dest_pixel[1] = c->g;
        dest_pixel[2] = c->b;
        dest_pixel[3] = has_alpha ? c->a : 255;
      }
    }
  }

  return bitmap_image;
}

NSImage* iupCocoaImageNSImageFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  NSBitmapImageRep* bitmap_image = iupCocoaImageNSBitmapImageRepFromPixels(width, height, bpp, colors, colors_count, imgdata);
  if (nil == bitmap_image)
  {
    return nil;
  }

  NSImage* ns_image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
  if (nil == ns_image)
  {
    [bitmap_image release];
    return nil;
  }

  [ns_image addRepresentation:bitmap_image];
  [bitmap_image release];

  return ns_image;
}

IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  NSImage* ns_image = iupCocoaImageNSImageFromRawData(width, height, bpp, colors, colors_count, imgdata);

  if (ns_image)
  {
    IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
    if (cb)
      cb(ns_image, "NSImage");
  }

  return ns_image;
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  /* Getting the color palette from a native Cocoa image is not straightforward,
     as indexed color images are often converted to RGB(A) automatically. */
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

static NSImage* iupCocoaCreateNSImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bpp = iupAttribGetInt(ih, "BPP");
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  unsigned char bg_r=0, bg_g=0, bg_b=0;
  int flat_alpha = iupAttribGetBoolean(ih, "FLAT_ALPHA");

  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  NSBitmapImageRep* bitmap_image = nil;

  // For 8bpp, we create a 32bpp image for simplicity and to handle alpha.
  if (bpp == 32 || bpp == 8)
  {
    bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                           pixelsWide:width pixelsHigh:height bitsPerSample:8
                                                      samplesPerPixel:4 hasAlpha:YES isPlanar:NO
                                                       colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                                                          bytesPerRow:CalculateBytesPerRow(width, 4)
                                                         bitsPerPixel:32];
  }
  else if (bpp == 24)
  {
    bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                           pixelsWide:width pixelsHigh:height bitsPerSample:8
                                                      samplesPerPixel:3 hasAlpha:NO isPlanar:NO
                                                       colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:0
                                                          bytesPerRow:CalculateBytesPerRow(width, 3)
                                                         bitsPerPixel:24];
  }
  else
  {
    return NULL;
  }

  if (!bitmap_image)
  {
    return NULL;
  }

  unsigned char *pixels = [bitmap_image bitmapData];
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

  NSImage* ns_image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
  [ns_image addRepresentation:bitmap_image];
  [bitmap_image release];

  if (make_inactive || flat_alpha)
  {
    iupAttribSetStr(ih, "_IUP_BGCOLOR_DEPEND", "1");
  }

  return ns_image;
}

void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
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

void* iupdrvImageCreateIcon(Ihandle *ih)
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

void* iupdrvImageCreateCursor(Ihandle *ih)
{
  int hx=0, hy=0;
  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  NSImage *image = iupCocoaCreateNSImage(ih, NULL, 0);
  if (!image)
  {
    return NULL;
  }

  NSSize size = [image size];
  NSPoint hotSpot = NSMakePoint(hx, size.height - 1 - hy);

  NSCursor *cursor = [[NSCursor alloc] initWithImage:image hotSpot:hotSpot];
  [image release];

  if (cursor)
  {
    IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
    if (cb)
      cb(cursor, "CURSOR");
  }

  return cursor;
}

void* iupdrvImageLoad(const char* name, int type)
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

  if (the_image)
  {
    IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
    if (cb)
    {
      const char* type_str = (type == IUPIMAGE_CURSOR) ? "CURSOR" :
                             (type == IUPIMAGE_ICON) ? "ICON" : "NSImage";
      cb(the_image, type_str);
    }
  }

  return (void*)the_image;
}

int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  if (w) *w = 0;
  if (h) *h = 0;
  if (bpp) *bpp = 0;

  if (NULL == handle)
    return 0;

  if (![(__bridge id)handle isKindOfClass:[NSImage class]])
    return 0;

  NSImage *image = (__bridge NSImage*)handle;
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
  if (bpp) *bpp = (int)[bitmap bitsPerPixel];
  return 1;
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  const char* type_str = (type == IUPIMAGE_CURSOR) ? "CURSOR" :
                         (type == IUPIMAGE_ICON) ? "ICON" : "NSImage";

  IFvs cb = (IFvs)IupGetFunction("IMAGEDESTROY_CB");
  if (cb)
    cb(handle, type_str);

  [((__bridge id)handle) release];
}

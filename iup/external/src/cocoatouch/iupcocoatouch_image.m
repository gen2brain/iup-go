/** \file
 * \brief Image Resource (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <ImageIO/ImageIO.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupcocoatouch_drv.h"


static void cocoaTouchImageReleaseMallocPixels(void* info, const void* data, size_t size)
{
	(void)info; (void)size;
	free((void*)data);
}

/* wraps premultiplied-RGBA32 pixels in a UIImage; provider owns and frees `pixels` */
static UIImage* cocoaTouchImageFromRGBA(unsigned char* pixels, int width, int height)
{
	size_t row_bytes = (size_t)width * 4;
	CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, pixels, row_bytes * height, &cocoaTouchImageReleaseMallocPixels);
	CGImageRef cg_image = CGImageCreate(width, height, 8, 32, row_bytes, cs,
	                                    kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big,
	                                    provider, NULL, false, kCGRenderingIntentDefault);
	UIImage* ui_image = cg_image ? [UIImage imageWithCGImage:cg_image] : nil;
	if (cg_image) CGImageRelease(cg_image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(cs);
	return ui_image;
}

/* convert RGB/RGBA/8bpp-palette src to a malloc'd RGBA32 buffer */
static unsigned char* cocoaTouchImageToRGBA(
	const unsigned char* src, int width, int height, int bpp,
	const iupColor* palette, int palette_has_alpha,
	int make_inactive, const unsigned char bg_rgb[3])
{
	size_t pixels = (size_t)width * (size_t)height;
	unsigned char* out = (unsigned char*)malloc(pixels * 4);
	if (!out) return NULL;

	for (size_t i = 0; i < pixels; i++)
	{
		unsigned char r, g, b, a;
		if (bpp == 32) { r = *src++; g = *src++; b = *src++; a = *src++; }
		else if (bpp == 24) { r = *src++; g = *src++; b = *src++; a = 255; }
		else /* bpp == 8 */
		{
			const iupColor* c = &palette[*src++];
			r = c->r; g = c->g; b = c->b;
			a = palette_has_alpha ? c->a : 255;
		}

		if (make_inactive)
		{
			iupImageColorMakeInactive(&r, &g, &b, bg_rgb[0], bg_rgb[1], bg_rgb[2]);
		}

		/* premultiply for kCGImageAlphaPremultipliedLast */
		if (a < 255)
		{
			r = (unsigned char)((int)r * a / 255);
			g = (unsigned char)((int)g * a / 255);
			b = (unsigned char)((int)b * a / 255);
		}

		out[i * 4 + 0] = r;
		out[i * 4 + 1] = g;
		out[i * 4 + 2] = b;
		out[i * 4 + 3] = a;
	}
	return out;
}

/* renders cg_image through a scratch context for straight (un-premultiplied) RGBA */
static unsigned char* cocoaTouchImageReadBackRGBA(CGImageRef cg_image, int* out_w, int* out_h)
{
	if (!cg_image) return NULL;
	int w = (int)CGImageGetWidth(cg_image);
	int h = (int)CGImageGetHeight(cg_image);
	if (w <= 0 || h <= 0) return NULL;

	size_t row_bytes = (size_t)w * 4;
	unsigned char* buf = (unsigned char*)calloc(row_bytes * h, 1);
	if (!buf) return NULL;

	CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
	/* CGBitmapContext only supports premultiplied RGBA32; un-premultiply below */
	CGContextRef ctx = CGBitmapContextCreate(buf, w, h, 8, row_bytes, cs,
	                                          kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
	CGColorSpaceRelease(cs);
	if (!ctx) { free(buf); return NULL; }

	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg_image);
	CGContextRelease(ctx);

	/* un-premultiply for straight channel data */
	for (size_t i = 0; i < (size_t)w * h; i++)
	{
		unsigned char a = buf[i * 4 + 3];
		if (a > 0 && a < 255)
		{
			buf[i * 4 + 0] = (unsigned char)((int)buf[i * 4 + 0] * 255 / a);
			buf[i * 4 + 1] = (unsigned char)((int)buf[i * 4 + 1] * 255 / a);
			buf[i * 4 + 2] = (unsigned char)((int)buf[i * 4 + 2] * 255 / a);
		}
	}

	if (out_w) *out_w = w;
	if (out_h) *out_h = h;
	return buf;
}


IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
	if (!handle || !imgdata) return;
	UIImage* ui_image = (UIImage*)handle;
	CGImageRef cg = [ui_image CGImage];
	int w = 0, h = 0;
	unsigned char* rgba = cocoaTouchImageReadBackRGBA(cg, &w, &h);
	if (!rgba) return;

	int bpp = (int)CGImageGetBitsPerPixel(cg);
	size_t plane = (size_t)w * (size_t)h;
	unsigned char* dst_r = imgdata;
	unsigned char* dst_g = imgdata + plane;
	unsigned char* dst_b = imgdata + 2 * plane;
	unsigned char* dst_a = (bpp >= 32) ? (imgdata + 3 * plane) : NULL;

	for (size_t i = 0; i < plane; i++)
	{
		*dst_r++ = rgba[i * 4 + 0];
		*dst_g++ = rgba[i * 4 + 1];
		*dst_b++ = rgba[i * 4 + 2];
		if (dst_a) *dst_a++ = rgba[i * 4 + 3];
	}
	free(rgba);
}

IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
	/* interleaved RGBA, or RGB if no alpha */
	if (!handle || !imgdata) return;
	UIImage* ui_image = (UIImage*)handle;
	CGImageRef cg = [ui_image CGImage];
	int w = 0, h = 0;
	unsigned char* rgba = cocoaTouchImageReadBackRGBA(cg, &w, &h);
	if (!rgba) return;

	int bpp = (int)CGImageGetBitsPerPixel(cg);
	size_t pixels = (size_t)w * (size_t)h;
	if (bpp >= 32)
	{
		memcpy(imgdata, rgba, pixels * 4);
	}
	else
	{
		for (size_t i = 0; i < pixels; i++)
		{
			imgdata[i * 3 + 0] = rgba[i * 4 + 0];
			imgdata[i * 3 + 1] = rgba[i * 4 + 1];
			imgdata[i * 3 + 2] = rgba[i * 4 + 2];
		}
	}
	free(rgba);
}


IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* palette, int palette_count, unsigned char* imgdata)
{
	if (!imgdata || width <= 0 || height <= 0) return NULL;

	int palette_has_alpha = 0;
	if (bpp == 8 && palette)
	{
		for (int i = 0; i < palette_count; i++)
			if (palette[i].a != 255) { palette_has_alpha = 1; break; }
	}

	unsigned char* rgba = cocoaTouchImageToRGBA(imgdata, width, height, bpp, palette, palette_has_alpha, 0, (unsigned char[3]){0,0,0});
	if (!rgba) return NULL;

	UIImage* ui_image = cocoaTouchImageFromRGBA(rgba, width, height);
	return [ui_image retain];
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int* w, int* h, int* bpp, iupColor* palette, int* palette_count)
{
	(void)palette; (void)palette_count;
	return iupdrvImageGetInfo(handle, w, h, bpp);
}

IUP_SDK_API void* iupdrvImageCreateImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
	int bpp    = iupAttribGetInt(ih, "BPP");
	int width  = ih->currentwidth;
	int height = ih->currentheight;
	unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
	if (!imgdata || width <= 0 || height <= 0) return NULL;

	unsigned char bg[3] = {0, 0, 0};
	iupStrToRGB(bgcolor, &bg[0], &bg[1], &bg[2]);

	iupColor palette[256];
	int palette_has_alpha = 0;
	if (bpp == 8) palette_has_alpha = iupImageInitColorTable(ih, palette, NULL);

	unsigned char* rgba = cocoaTouchImageToRGBA(imgdata, width, height, bpp,
		bpp == 8 ? palette : NULL, palette_has_alpha, make_inactive, bg);
	if (!rgba) return NULL;

	UIImage* ui_image = cocoaTouchImageFromRGBA(rgba, width, height);
	if (make_inactive) iupAttribSetStr(ih, "_IUP_BGCOLOR_DEPEND", "1");
	return [ui_image retain];
}

IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle* ih)
{
	return iupdrvImageCreateImage(ih, NULL, 0);
}

IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle* ih)
{
	(void)ih;
	return NULL;  /* no user-supplied cursor image on iOS */
}

IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
	(void)type;
	if (!name) return NULL;
	NSString* ns_name = [NSString stringWithUTF8String:name];

	/* try raw path, then bundle-relative, then asset catalog, then SF symbol */
	UIImage* ui_image = [[UIImage alloc] initWithContentsOfFile:ns_name];
	if (ui_image) return ui_image;

	NSString* resource_path = [[NSBundle mainBundle] resourcePath];
	if (resource_path)
	{
		NSString* bundled = [resource_path stringByAppendingPathComponent:ns_name];
		ui_image = [[UIImage alloc] initWithContentsOfFile:bundled];
		if (ui_image) return ui_image;
	}

	UIImage* named = [UIImage imageNamed:ns_name];
	if (named) return [named retain];

	UIImage* symbol = [UIImage systemImageNamed:ns_name];
	if (symbol) return [symbol retain];
	return NULL;
}

IUP_SDK_API int iupdrvImageGetInfo(void* handle, int* w, int* h, int* bpp)
{
	UIImage* ui_image = (UIImage*)handle;
	if (!ui_image) return 0;
	CGImageRef cg_image = [ui_image CGImage];
	if (!cg_image) return 0;
	if (w)   *w   = (int)CGImageGetWidth(cg_image);
	if (h)   *h   = (int)CGImageGetHeight(cg_image);
	if (bpp) *bpp = (int)CGImageGetBitsPerPixel(cg_image);
	return 1;
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
	(void)type;
	[(UIImage*)handle release];
}

/* unpremultiplied AARRGGBB icon bytes */
IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
	(void)ih;
	if (!value || !width || !height || !pixels) return 0;

	UIImage* image = (UIImage*)iupImageGetIcon(value);
	if (!image) return 0;

	CGImageRef cg = [image CGImage];
	if (!cg) return 0;

	int w = 0, h = 0;
	unsigned char* rgba = cocoaTouchImageReadBackRGBA(cg, &w, &h);
	if (!rgba) return 0;

	/* repack as AARRGGBB */
	size_t total = (size_t)w * (size_t)h;
	unsigned char* out = (unsigned char*)malloc(total * 4);
	if (!out) { free(rgba); return 0; }
	for (size_t i = 0; i < total; i++)
	{
		out[i * 4 + 0] = rgba[i * 4 + 3];  /* A */
		out[i * 4 + 1] = rgba[i * 4 + 0];  /* R */
		out[i * 4 + 2] = rgba[i * 4 + 1];  /* G */
		out[i * 4 + 3] = rgba[i * 4 + 2];  /* B */
	}
	free(rgba);

	*width = w;
	*height = h;
	*pixels = out;
	return 1;
}


/* IUP image storage is interleaved per pixel: bpp=32 -> RGBA, bpp=24 -> RGB, bpp=8 -> palette index. */
static int cocoaTouchImageEncode(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count,
                                 const char* format, NSData** out_data)
{
	if (!imgdata || width <= 0 || height <= 0 || !out_data) return 0;

	NSString* uti;
	if (format && iupStrEqualNoCase(format, "JPEG"))      uti = IUPCOCOATOUCH_UTI_JPEG;
	else if (format && iupStrEqualNoCase(format, "BMP"))  uti = IUPCOCOATOUCH_UTI_BMP;
	else if (format && iupStrEqualNoCase(format, "TIFF")) uti = IUPCOCOATOUCH_UTI_TIFF;
	else if (format && iupStrEqualNoCase(format, "GIF"))  uti = IUPCOCOATOUCH_UTI_GIF;
	else                                                   uti = IUPCOCOATOUCH_UTI_PNG;

	size_t row_bytes = (size_t)width * 4;
	unsigned char* rgba = malloc(row_bytes * (size_t)height);
	if (!rgba) return 0;

	size_t pixels = (size_t)width * (size_t)height;
	if (bpp == 32)
	{
		for (size_t i = 0; i < pixels; i++)
		{
			rgba[i*4 + 0] = imgdata[i*4 + 0];
			rgba[i*4 + 1] = imgdata[i*4 + 1];
			rgba[i*4 + 2] = imgdata[i*4 + 2];
			rgba[i*4 + 3] = imgdata[i*4 + 3];
		}
	}
	else if (bpp == 24)
	{
		for (size_t i = 0; i < pixels; i++)
		{
			rgba[i*4 + 0] = imgdata[i*3 + 0];
			rgba[i*4 + 1] = imgdata[i*3 + 1];
			rgba[i*4 + 2] = imgdata[i*3 + 2];
			rgba[i*4 + 3] = 255;
		}
	}
	else
	{
		for (size_t i = 0; i < pixels; i++)
		{
			unsigned char idx = imgdata[i];
			if (colors_count > 0 && (int)idx < colors_count)
			{
				rgba[i*4 + 0] = colors[idx].r;
				rgba[i*4 + 1] = colors[idx].g;
				rgba[i*4 + 2] = colors[idx].b;
				rgba[i*4 + 3] = colors[idx].a;
			}
			else
			{
				rgba[i*4 + 0] = idx;
				rgba[i*4 + 1] = idx;
				rgba[i*4 + 2] = idx;
				rgba[i*4 + 3] = 255;
			}
		}
	}

	CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
	CGContextRef ctx = CGBitmapContextCreate(rgba, width, height, 8, row_bytes, cs, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
	CGColorSpaceRelease(cs);
	if (!ctx) { free(rgba); return 0; }
	CGImageRef image = CGBitmapContextCreateImage(ctx);
	CGContextRelease(ctx);
	free(rgba);
	if (!image) return 0;

	NSMutableData* data = [NSMutableData data];
	CGImageDestinationRef dest = CGImageDestinationCreateWithData((CFMutableDataRef)data, (CFStringRef)uti, 1, NULL);
	if (!dest) { CGImageRelease(image); return 0; }
	CGImageDestinationAddImage(dest, image, NULL);
	BOOL ok = CGImageDestinationFinalize(dest);
	CFRelease(dest);
	CGImageRelease(image);
	if (!ok) return 0;

	*out_data = [data retain];
	return 1;
}

IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count,
                    const char* filename, const char* format)
{
	NSData* data = nil;
	if (!cocoaTouchImageEncode(imgdata, width, height, bpp, colors, colors_count, format, &data)) return 0;
	BOOL ok = [data writeToFile:[NSString stringWithUTF8String:filename] atomically:YES];
	[data release];
	return ok ? 1 : 0;
}

IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count,
                                       const char* format, int* size)
{
	NSData* data = nil;
	if (!cocoaTouchImageEncode(imgdata, width, height, bpp, colors, colors_count, format, &data)) return NULL;
	NSUInteger len = [data length];
	unsigned char* buf = (unsigned char*)malloc(len);
	if (!buf) { [data release]; return NULL; }
	memcpy(buf, [data bytes], len);
	[data release];
	if (size) *size = (int)len;
	return buf;
}

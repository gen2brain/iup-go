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

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"


/* Adapted from SDL (zlib)
 * Calculate the pad-aligned scanline width of a surface
 */
static int CalculateBytesPerRow(int width, int bytes_per_pixel)
{
	int pitch;
	int bits_per_pixel = bytes_per_pixel * 8;
	/* Surface should be 4-byte aligned for speed */
	pitch = width * bytes_per_pixel;
	switch (bits_per_pixel) {
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
	return (pitch);
}

static int CalculateRowLength(int width, int bytes_per_pixel)
{
	int pitch = CalculateBytesPerRow(width, bytes_per_pixel);
	return pitch/bytes_per_pixel;
}

int iupCocoaImageCaluclateBytesPerRow(int width, int bytes_per_pixel)
{
	return CalculateBytesPerRow(width, bytes_per_pixel);
}


// The difference between iupdrvImageGetData and iupdrvImageGetRawData is in the output format.
// The input is the same. But the iupdrvImageGetData (packed RGB, top-down) output used the IUP image data format,
// and iupdrvImageGetRawData uses the IM image data format for the imImage (separated RGB planes, bottom-up).
// FIXME: Carried over implementation. Untested.
void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  int x,y;
  unsigned char *red,*green,*blue,*alpha;
  NSImage *image = (__bridge NSImage*)handle;
  NSBitmapImageRep *bitmap = nil;
  if([[image representations] count]>0) bitmap = [[image representations] objectAtIndex:0];
  if(bitmap==nil) return;
	NSInteger w = [bitmap pixelsWide];
  NSInteger h = [bitmap pixelsHigh];
  NSInteger bpp = [bitmap bitsPerPixel];
  NSInteger planesize = w*h;
  unsigned char *bits = [bitmap bitmapData]; 
  red = imgdata;
  green = imgdata+planesize;
  blue = imgdata+2*planesize;
  alpha = imgdata+3*planesize;
  for(y=0;y<h;y++) {
    for(x=0;x<w;x++) {
      if(bpp>=24) {
        *red++ = *bits++;
        *green++ = *bits++;
        *blue++ = *bits++;
      }
      if(bpp==32) {
        *alpha++ = *bits++;
      }
    }
  }
}

// The input is the same. But the iupdrvImageGetData (packed RGB, top-down) output used the IUP image data format,
// and iupdrvImageGetRawData uses the IM image data format for the imImage (separated RGB planes, bottom-up).


// Currently used by Drag & Drop. I'm not sure if the semantics match the original intent of this function.
// I assume handle is an NSImage (which has a NSBitmapImageRep),
// and that out_img_data is already allocated to be bytesPerRow*h.
// This will write the pixel data to out_img_data.
#if 0
void iupdrvImageGetData(void* handle, unsigned char* out_img_data)
{
	NSImage* ns_image = (__bridge NSImage*)handle;

    NSSize image_size = [ns_image size];
    NSRect image_rect = NSMakeRect(0, 0, image_size.width, image_size.height);

	int bpp = 32;

	NSBitmapImageRep* bitmap = nil;
	
	if([[ns_image representations] count] > 0)
	{
		bitmap = (NSBitmapImageRep*)[[ns_image representations] objectAtIndex:0];
	}
	
/*
	{
		NSInteger w = [bitmap pixelsWide];
		NSInteger h = [bitmap pixelsHigh];
		// NSInteger bpp = [bitmap bitsPerPixel];
		NSInteger samplesPerPixel = [bitmap samplesPerPixel];
		NSInteger bytesPerRow = [bitmap bytesPerRow];
		NSInteger bytesPerPlane = [bitmap bytesPerPlane];
		NSInteger numberOfPlanes = [bitmap numberOfPlanes];
		NSLog(@"w: %ld", (long)w);
		NSLog(@"h: %ld", (long)h);
		NSLog(@"bpp: %ld", (long)bpp);
		NSLog(@"samplesPerPixel: %ld", (long)samplesPerPixel);
		NSLog(@"bytesPerRow: %ld", (long)bytesPerRow);
		NSLog(@"bytesPerPlane: %ld", (long)bytesPerPlane);
		NSLog(@"numberOfPlanes: %ld", (long)numberOfPlanes);
 
 
	}
*/
	
	if(bitmap==nil)
	{
		bpp = 32;
	}
	else
	{
		bpp = (int)[bitmap bitsPerPixel];
	}
	if(bpp == 32)
	{
		CGImageAlphaInfo alpha_info = kCGImageAlphaPremultipliedLast;

		int bytes_per_row = CalculateBytesPerRow(image_size.width, bpp/8);

		CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

		// We are going to write directly to out_img_data here instead of passing NULL which will malloc new memory.
		// this must be sizeof bytes_per_row*h
		CGContextRef ctx = CGBitmapContextCreate(
			out_img_data,
			image_size.width,
			image_size.height,
			8,
			bytes_per_row,
			color_space,
			alpha_info
		);

		NSGraphicsContext* saved_context = [NSGraphicsContext currentContext];
	//	[saved_context saveGraphicsState];

		// Wrap graphics context
		NSGraphicsContext* gctx = [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:NO];

		// Make our bitmap context current and render the NSImage into it
		[NSGraphicsContext setCurrentContext:gctx];
		[ns_image drawInRect:image_rect];
		
		/*
		 size_t width = CGBitmapContextGetWidth(ctx);
		 size_t height = CGBitmapContextGetHeight(ctx);
		 uint32_t* pixel = (uint32_t*)CGBitmapContextGetData(ctx);
		 
		 // int bytes_per_row = CGBitmapContextGetBytesPerRow(ctx);
		 int total_bytes = bytes_per_row*height;
		 memcpy(out_img_data, pixel, total_bytes);
	*/

		// Clean up
		[NSGraphicsContext setCurrentContext:saved_context];
	//	[saved_context restoreGraphicsState];

		CGContextRelease(ctx);
		CGColorSpaceRelease(color_space);
		
		
	}
	else if(bpp == 24)
	{
		// Grrr. CGBitmapContextCreate only handles 32-bit.
		// I'm tempted to change the entire backend to always handle convert to 32-bit images, but that may cause too many problems.
		
		
		
		int bytes_per_row = CalculateBytesPerRow(image_size.width, bpp/8);
		int total_bytes = bytes_per_row*image_size.height;

		unsigned char* bitmap_data = [bitmap bitmapData];
		memcpy(out_img_data, bitmap_data, total_bytes);


	}
	else
	{
//		NSLog(@"unsupported bpp: %d", bpp);
//		return;

		
		int bytes_per_row = CalculateBytesPerRow(image_size.width, bpp/8);
		int total_bytes = bytes_per_row*image_size.height;

		unsigned char* bitmap_data = [bitmap bitmapData];
		memcpy(out_img_data, bitmap_data, total_bytes);
	}
	


}
#else
// The input is the same. But the iupdrvImageGetData (packed RGB, top-down) output used the IUP image data format,
// and iupdrvImageGetRawData uses the IM image data format for the imImage (separated RGB planes, bottom-up).

// This implementation assumes there is an NSBitmapImageRep backing the NSImage,
// and that the pixels are already in the exact order we need.
// So this makes it a fast memcpy, and is the best implementation we can hope for.
// So far this seems to work well and I tested on 8-bit GIFs, 24-bit JPEGs, and 32-bit PNGs.
void iupdrvImageGetData(void* handle, unsigned char* out_img_data)
{
	NSImage* ns_image = (__bridge NSImage*)handle;

    NSSize image_size = [ns_image size];

	NSBitmapImageRep* bitmap = nil;
	
	if([[ns_image representations] count] > 0)
	{
		bitmap = (NSBitmapImageRep*)[[ns_image representations] objectAtIndex:0];
	}

	if(bitmap == nil)
	{
		NSLog(@"Could not find NSBitmapImageRep");
		return;
	}

	int bpp = (int)[bitmap bitsPerPixel];

		
	int bytes_per_row = CalculateBytesPerRow(image_size.width, bpp/8);
	int total_bytes = bytes_per_row*image_size.height;

	unsigned char* bitmap_data = [bitmap bitmapData];
	memcpy(out_img_data, bitmap_data, total_bytes);


}
#endif




// FIXME:  Probably wrong. Untested, don't know what calls this, don't know how to test. Started using for drag/drop, but not sure if that usage is the same.
// Update: I'm now worried this is related to the IM library stuff, and is expecting pixels separate R, G, B, A arrays.
// This is not how I'm using it.
NSBitmapImageRep* iupCocoaImageNSBitmapImageRepFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{


	
	NSBitmapImageRep* bitmap_image = nil;

	
	if(32 == bpp)
	{
		bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
														 pixelsWide:width pixelsHigh:height bitsPerSample:8
													samplesPerPixel:4 hasAlpha:YES isPlanar:NO
													 colorSpaceName:NSDeviceRGBColorSpace
															// I thought this should be 0 because I thought I want pre-multipled alpha, but some png's I'm testing render better with this flag.
															 bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
														bytesPerRow:CalculateBytesPerRow(width, 4)
													   bitsPerPixel:32
						];
	}
	else if(24 == bpp)
	{
		bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
															   pixelsWide:width pixelsHigh:height bitsPerSample:8
														  samplesPerPixel:3 hasAlpha:NO isPlanar:NO
														   colorSpaceName:NSDeviceRGBColorSpace
															// untested
															 bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
															  bytesPerRow:CalculateBytesPerRow(width, 3)
													   bitsPerPixel:24
						];
	}
	else if(8 == bpp)
	{
		
		// We'll make a full 32-bit image for this case
		bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
															   pixelsWide:width pixelsHigh:height bitsPerSample:8
														  samplesPerPixel:4 hasAlpha:YES isPlanar:NO
														   colorSpaceName:NSDeviceRGBColorSpace
															// untested
															bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
															  bytesPerRow:CalculateBytesPerRow(width, 4)
													   bitsPerPixel:32
						];
		
	}
	else
	{
		return NULL;
	}
	
	
	
	if(32 == bpp)
	{
		//  unsigned char *red,*green,*blue,*alpha;
		unsigned char* source_pixel;

		//  unsigned char *pixels = malloc(width*height*bpp);
		unsigned char *pixels = [bitmap_image bitmapData];
		int row_length = CalculateRowLength(width, 4);


		
		source_pixel = imgdata;

		
		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;
				unsigned char s_a = *source_pixel;
				source_pixel++;


				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;
			}
		}

		
		
		
		
	}
	else if(24 == bpp)
	{
		//  unsigned char *red,*green,*blue,*alpha;
		unsigned char* source_pixel;
		
		//  unsigned char *pixels = malloc(width*height*bpp);
		unsigned char *pixels = [bitmap_image bitmapData];
		
		int row_length = CalculateRowLength(width, 3);
		
		source_pixel = imgdata;
		
		

		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;



				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
			}
		}

		
		

	}
	else if(8 == bpp)
	{
#if 1
		//  unsigned char *red,*green,*blue,*alpha;
		unsigned char* source_pixel;
		
		//  unsigned char *pixels = malloc(width*height*bpp);
		unsigned char *pixels = [bitmap_image bitmapData];
		
		int row_length = CalculateRowLength(width, 4);


		
		int has_alpha = 0;

		

		
		
		
		source_pixel = imgdata;
		
		
		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char index = *source_pixel;
				iupColor* c = &colors[index];

				unsigned char s_r = c->r;
				unsigned char s_g = c->g;
				unsigned char s_b = c->b;
				unsigned char s_a;

				if(has_alpha)
				{
					s_a = c->a;
				}
				else
				{
					s_a = 255;
				}



				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;
				
				source_pixel++;
			}
		}

		

		
#endif
		
	}
	else
	{

		
	}
	

	
	
	[bitmap_image autorelease];
	


	// I originally thought I needed to return an autoreleased image, but IUP is putting this into a handle with a destroy hook.
	// And I was crashing in NSAutoreleasePool drain when autoreleasing this.
	// Update:
	// The typical pattern is to call image = iupImageGetImage(),
	// and then call [foo setImage:image];
	// This might imply that we should return as autoreleased.
	// But I think IUP is supposed to run iupdrvImageDestroy() if things are written correctly.
	// That would mean we want to return with a retain count of 1
	return bitmap_image;
}

// FIXME:  Probably wrong. Untested, don't know what calls this, don't know how to test. Started using for drag/drop, but not sure if that usage is the same.
// Update: I'm now worried this is related to the IM library stuff, and is expecting pixels separate R, G, B, A arrays.
// This is not how I'm using it.
NSImage* iupCocoaImageNSImageFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
	NSBitmapImageRep* bitmap_image = iupCocoaImageNSBitmapImageRepFromPixels(width, height, bpp, colors, colors_count, imgdata);
	
	if(nil == bitmap_image)
	{
		return nil;
	}
	
	NSImage* ns_image = [[NSImage alloc] initWithSize:NSMakeSize(width,height)];
	if(nil == ns_image)
	{
		return nil;
	}
	[ns_image autorelease];
	
	[ns_image addRepresentation:bitmap_image];
	
	return ns_image;

}

// FIXME:  Probably wrong. Untested, don't know what calls this, don't know how to test.
// Update: I'm now worried this is related to the IM library stuff, and is expecting pixels separate R, G, B, A arrays.
// This is not how I'm using it.
// At one point, I used it for drag & drop, but I refactored it so it no longer calls this, but uses underlying functions.
void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
	return iupCocoaImageNSImageFromPixels(width, height, bpp, colors, colors_count, imgdata);
}

int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  /* How to get the pallete? */
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}


void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int y, x, bpp, bgcolor_depend = 0,
      width = ih->currentwidth,
      height = ih->currentheight;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  unsigned char bg_r=0, bg_g=0, bg_b=0;
  bpp = iupAttribGetInt(ih, "BPP");
  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  NSImage* ns_image = [[NSImage alloc] initWithSize:NSMakeSize(width,height)];
  if (!ns_image)
  {
    return NULL;
  }
	
	NSBitmapImageRep* bitmap_image = nil;

	
	if(32 == bpp)
	{
		bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
														 pixelsWide:width pixelsHigh:height bitsPerSample:8
													samplesPerPixel:4 hasAlpha:YES isPlanar:NO
													 colorSpaceName:NSDeviceRGBColorSpace
															// I thought this should be 0 because I thought I want pre-multipled alpha, but some png's I'm testing render better with this flag.
															 bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
														bytesPerRow:CalculateBytesPerRow(width, 4)
													   bitsPerPixel:32
						];
	}
	else if(24 == bpp)
	{
		bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
															   pixelsWide:width pixelsHigh:height bitsPerSample:8
														  samplesPerPixel:3 hasAlpha:NO isPlanar:NO
														   colorSpaceName:NSDeviceRGBColorSpace
															// untested
															 bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
															  bytesPerRow:CalculateBytesPerRow(width, 3)
													   bitsPerPixel:24
						];
	}
	else if(8 == bpp)
	{
		
		// We'll make a full 32-bit image for this case
		bitmap_image = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
															   pixelsWide:width pixelsHigh:height bitsPerSample:8
														  samplesPerPixel:4 hasAlpha:YES isPlanar:NO
														   colorSpaceName:NSDeviceRGBColorSpace
															// untested
															bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
															  bytesPerRow:CalculateBytesPerRow(width, 4)
													   bitsPerPixel:32
						];
		
	}
	else
	{
		[ns_image release];
		return NULL;
	}
	
	
	
	if(32 == bpp)
	{
		//  unsigned char *red,*green,*blue,*alpha;
		unsigned char* source_pixel;

		//  unsigned char *pixels = malloc(width*height*bpp);
		unsigned char *pixels = [bitmap_image bitmapData];
		int row_length = CalculateRowLength(width, 4);


		
		source_pixel = imgdata;

		
		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;
				unsigned char s_a = *source_pixel;
				source_pixel++;

				if(make_inactive)
				{
					iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
				}

				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;
			}
		}

		
		
		
		
	}
	else if(24 == bpp)
	{
		//  unsigned char *red,*green,*blue,*alpha;
		unsigned char* source_pixel;
		
		//  unsigned char *pixels = malloc(width*height*bpp);
		unsigned char *pixels = [bitmap_image bitmapData];
		
		int row_length = CalculateRowLength(width, 3);
		
		source_pixel = imgdata;
		
		

		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;

				if(make_inactive)
				{
					iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
				}

				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
			}
		}

		
		

	}
	else if(8 == bpp)
	{
#if 1
		//  unsigned char *red,*green,*blue,*alpha;
		unsigned char* source_pixel;
		
		//  unsigned char *pixels = malloc(width*height*bpp);
		unsigned char *pixels = [bitmap_image bitmapData];
		
		int row_length = CalculateRowLength(width, 4);

		int colors_count = 0;
		iupColor colors[256];
		
		int has_alpha = iupImageInitColorTable(ih, colors, &colors_count);

		

		
		
		
		source_pixel = imgdata;
		
		
		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char index = *source_pixel;
				iupColor* c = &colors[index];

				unsigned char s_r = c->r;
				unsigned char s_g = c->g;
				unsigned char s_b = c->b;
				unsigned char s_a;

				if(has_alpha)
				{
					s_a = c->a;
				}
				else
				{
					s_a = 255;
				}

				if(make_inactive)
				{
					iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
				}


				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;
				
				source_pixel++;
			}
		}

		

		
#endif
		
	}
	else
	{

		
	}
	

	
	
  [ns_image addRepresentation:bitmap_image];
	[bitmap_image release];
  if (bgcolor_depend || make_inactive)
    iupAttribSetStr(ih, "_IUP_BGCOLOR_DEPEND", "1");


	// I originally thought I needed to return an autoreleased image, but IUP is putting this into a handle with a destroy hook.
	// And I was crashing in NSAutoreleasePool drain when autoreleasing this.
	// Update:
	// The typical pattern is to call image = iupImageGetImage(),
	// and then call [foo setImage:image];
	// This might imply that we should return as autoreleased.
	// But I think IUP is supposed to run iupdrvImageDestroy() if things are written correctly.
	// That would mean we want to return with a retain count of 1
	return ns_image;
}

void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

void* iupdrvImageCreateCursor(Ihandle *ih)
{
  int bpp,y,x,hx,hy,
      width = ih->currentwidth,
      height = ih->currentheight,
      line_size = (width+7)/8,
      size_bytes = line_size*height;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  char *sbits, *mbits, *sb, *mb;
  unsigned char r, g, b;

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp > 8)
    return NULL;

  sbits = (char*)malloc(2*size_bytes);
  if (!sbits) return NULL;
  memset(sbits, 0, 2*size_bytes);
  mbits = sbits + size_bytes;

  sb = sbits;
  mb = mbits;
  for (y=0; y<height; y++)
  {
    for (x=0; x<width; x++)
    {
      int byte = x/8;
      int bit = x%8;
      int index = (int)imgdata[y*width+x];
      /* index==0 is transparent */
      if (index == 1)
        sb[byte] = (char)(sb[byte] | (1<<bit));
      if (index != 0)
        mb[byte] = (char)(mb[byte] | (1<<bit));
    }

    sb += line_size;
    mb += line_size;
  }

  hx=0; hy=0;
  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  NSData *tiffData = [NSData dataWithBytes:imgdata length:(width*height*(bpp/8))];
  NSImage *source = [[NSImage alloc] initWithData:tiffData];
  NSSize size = {width,height};
  [source setSize:size]; 

  NSPoint point = {hx,hy};

  NSCursor *cursor = [[NSCursor alloc] initWithImage:source hotSpot:point];
	[source release];
  free(sbits);

	return cursor;
}

void* iupdrvImageCreateMask(Ihandle *ih)
{
  int bpp,y,x,
      width = ih->currentwidth,
      height = ih->currentheight,
      line_size = (width+7)/8,
      size_bytes = line_size*height;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  char *bits, *sb;
  unsigned char colors[256];

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp > 8)
    return NULL;

  bits = (char*)malloc(size_bytes);
  if (!bits) return NULL;
  memset(bits, 0, size_bytes);

  iupImageInitNonBgColors(ih, colors);

  sb = bits;
  for (y=0; y<height; y++)
  {
    for (x=0; x<width; x++)
    {
      int byte = x/8;
      int bit = x%8;
      int index = (int)imgdata[y*width+x];
      if (colors[index])
        sb[byte] = (char)(sb[byte] | (1<<bit));
    }

    sb += line_size;
  }

  NSData *tiffData = [NSData dataWithBytes:imgdata length:(width*height*(bpp/8))];
  NSImage *mask = [[NSImage alloc] initWithData:tiffData];
  NSSize size = {width,height};
  [mask setSize:size]; 
  free(bits);

	return (void*)mask;
}

void* iupdrvImageLoad(const char* name, int type)
{
	if(!name || (name[0] == '\0'))
	{
		return NULL;
	}
  //int iup2mac[3] = {IMAGE_BITMAP, IMAGE_ICON, IMAGE_CURSOR};
	NSImage* the_image = nil;
	NSString* bundle_path = [[NSBundle mainBundle] bundlePath];

	NSString* ns_name = [NSString stringWithUTF8String:name];
	
	// Problem: The path either must be absolute, or it must be in the application bundle.
	// TODO: We could also try to look elsewhere if we choose to, but beware of Sandboxing.
	// Do we need to worry about images embedded in the IUP frameworks? (I think not because they are compiled into code.)
	
	// First, just try what was given. This could be an absolute path or current working directory.
	the_image = [[NSImage alloc] initWithContentsOfFile:ns_name];
	if(nil == the_image)
	{
		// Next, let's try the app bundle
		NSString* resource_path = [[NSBundle mainBundle] resourcePath];
		NSString* the_path = [resource_path stringByAppendingPathComponent:ns_name];
		the_image = [[NSImage alloc] initWithContentsOfFile:the_path];


	}
	// if that still failed, let's try the directory where the app bundle resides.
	// NOTE: Nobody should ship an app like this, but this is mainly for the Iup testing directory
	if(nil == the_image)
	{
		NSString* bundle_path = [[NSBundle mainBundle] bundlePath];
		// Chop off the Foo.app part
		bundle_path = [bundle_path stringByDeletingLastPathComponent];
		
		NSString* the_path = [bundle_path stringByAppendingPathComponent:ns_name];
		the_image = [[NSImage alloc] initWithContentsOfFile:the_path];
	}
	
	
	// giving up
	if(nil == the_image)
	{
		return NULL;
	}
	
	NSBitmapImageRep* bitmap_rep = [[the_image representations] objectAtIndex:0];
	// If you think you might get something other than a bitmap image representation,
	// check for it here.

	NSSize image_size = NSMakeSize([bitmap_rep pixelsWide], [bitmap_rep pixelsHigh]);
	[the_image setSize:image_size];

	return (void*)the_image;
	
}

int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  if(w) *w = 0;
  if(h) *h = 0;
  if(bpp) *bpp = 0;
  if(NULL == handle)
  {
    return 0;
  }
  NSImage *image = (__bridge NSImage*)handle;
  NSBitmapImageRep* bitmap = nil;
	

   for(NSImageRep* image_rep in [image representations])
   {
      if([image_rep isKindOfClass:[NSBitmapImageRep class]] )
      {
 	     bitmap = (NSBitmapImageRep*)image_rep;
 	     break;
      }
	}
	
//  if([[image representations] count]>0) bitmap = [[image representations] objectAtIndex:0];
  if(bitmap==nil)
  {
  	CGImageRef cg_image = [image CGImageForProposedRect:nil context:nil hints:nil];
	bitmap = [[[NSBitmapImageRep alloc] initWithCGImage:cg_image] autorelease];


  
  
  }
  if(bitmap==nil)
  {
    return 0;
  }
  if(w) *w = [bitmap pixelsWide];
  if(h) *h = [bitmap pixelsHigh];
  if(bpp) *bpp = [bitmap bitsPerPixel];
  return 1;
}

// [NSApp setApplicationIconImage: [NSImage imageNamed: @"Icon_name.icns"]]

void iupdrvImageDestroy(void* handle, int type)
{
  switch (type)
  {
  case IUPIMAGE_IMAGE:
    [handle release];
    break;
  case IUPIMAGE_ICON:
    [handle release];
    break;
  case IUPIMAGE_CURSOR:
    [handle release];
    break;
	  default:
	  {
		  NSLog(@"Warning: unexpected type in in iupdrvImageDestroy");
		  [handle release];
	  }
  }
}




//Use IupImageGetHandle
/* DEPRECATED: Why doesn't IUP have a Load from file?
IUP_EXPORT Ihandle* IupImageLoadFile(const char* file_name)
{
	int w;
	int h;
	int bpp;
	Ihandle* image_ih = NULL;
	
	NSImage* ns_image = (NSImage*)iupdrvImageLoad(file_name, IUPIMAGE_IMAGE);

	if(nil == ns_image)
	{
		return NULL;
	}
	
	iupdrvImageGetInfo(ns_image, &w, &h, &bpp);
	int bytes_per_row = iupCocoaImageCaluclateBytesPerRow(w, bpp/8);
	size_t buffer_size = bytes_per_row*h;
	unsigned char* img_data = malloc(buffer_size);
	iupdrvImageGetData(ns_image, img_data);
	
	if(bpp == 32)
	{
		image_ih = IupImageRGBA(w, h, img_data);

	}
	else if(bpp == 24)
	{
		image_ih = IupImageRGB(w, h, img_data);
	}
	else
	{
		image_ih = IupImage(w, h, img_data);
	}
	[ns_image release];

	return image_ih;
}
*/

/** \file
 * \brief WebAssembly Image Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupwasm_drv.h"


/* main builds the <img> data URL; entry.canvas is a worker-local raster for drawImage */
EM_JS(int, iupwasmJsImageCreate, (int w, int h, int ptr), {
  if (!globalThis.__iupImg) globalThis.__iupImg = { map: {}, next: 1 };
  var id = globalThis.__iupImg.next++;
  var entry = { w: w, h: h };
  if (typeof document === 'undefined') {  /* worker: OffscreenCanvas raster for drawImage */
    var cv = new OffscreenCanvas(w, h);
    var ctx = cv.getContext('2d');
    var img = ctx.createImageData(w, h);
    img.data.set(HEAPU8.subarray(ptr, ptr + w * h * 4));
    ctx.putImageData(img, 0, 0);
    entry.canvas = cv;
  }
  globalThis.__iupImg.map[id] = entry;
  globalThis.__iupApply({ op: 'imagecreate', imgId: id, w: w, h: h, rgba: HEAPU8.slice(ptr, ptr + w * h * 4) });
  return id;
})

EM_JS(void, iupwasmJsImageDestroy, (int id), {
  if (globalThis.__iupImg) delete globalThis.__iupImg.map[id];
  globalThis.__iupApply({ op: 'imagedestroy', imgId: id });
})

void iupwasmJsSetImage(int elId, int imgId)
{
  EM_ASM({ globalThis.__iupApply({ op: 'setimage', id: $0, imgId: $1 }); }, elId, imgId);
}


typedef struct { int w, h, bpp; } wasmImgInfo;
static wasmImgInfo* s_imginfo = NULL;
static int s_imginfo_count = 0;

static void wasmImgInfoSet(int id, int w, int h, int bpp)
{
  if (id >= s_imginfo_count)
  {
    int i, n = id + 16;
    s_imginfo = (wasmImgInfo*)realloc(s_imginfo, n * sizeof(wasmImgInfo));
    for (i = s_imginfo_count; i < n; i++) { s_imginfo[i].w = s_imginfo[i].h = s_imginfo[i].bpp = 0; }
    s_imginfo_count = n;
  }
  s_imginfo[id].w = w; s_imginfo[id].h = h; s_imginfo[id].bpp = bpp;
}

static void* wasmImageCreateRGBA(int width, int height, int bpp, iupColor* colors, unsigned char* imgdata)
{
  int x, y, id;
  unsigned char* rgba;
  if (width <= 0 || height <= 0 || !imgdata)
    return NULL;

  rgba = (unsigned char*)malloc((size_t)width * height * 4);

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      unsigned char* o = rgba + ((size_t)y * width + x) * 4;
      if (bpp == 8)
      {
        iupColor* c = &colors[imgdata[y * width + x]];
        o[0] = c->r; o[1] = c->g; o[2] = c->b; o[3] = c->a;
      }
      else if (bpp == 24)
      {
        unsigned char* p = imgdata + ((size_t)y * width + x) * 3;
        o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = 255;
      }
      else /* 32 */
      {
        unsigned char* p = imgdata + ((size_t)y * width + x) * 4;
        o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = p[3];
      }
    }
  }

  id = iupwasmJsImageCreate(width, height, (int)(intptr_t)rgba);
  free(rgba);
  if (id)
    wasmImgInfoSet(id, width, height, bpp);
  return (void*)(intptr_t)id;
}

IUP_SDK_API void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int bpp, colors_count = 0;
  iupColor colors[256];
  unsigned char* imgdata;
  (void)bgcolor;
  (void)make_inactive;

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp == 8)
    iupImageInitColorTable(ih, colors, &colors_count);

  imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  return wasmImageCreateRGBA(ih->currentwidth, ih->currentheight, bpp, colors, imgdata);
}

IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  (void)colors_count;
  return wasmImageCreateRGBA(width, height, bpp, colors, imgdata);
}

IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
  (void)name;
  (void)type;
  return NULL;
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  int id = (int)(intptr_t)handle;
  (void)type;
  if (id)
    iupwasmJsImageDestroy(id);
}

IUP_SDK_API int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  int id = (int)(intptr_t)handle;
  if (id <= 0 || id >= s_imginfo_count)
    return 0;
  if (w) *w = s_imginfo[id].w;
  if (h) *h = s_imginfo[id].h;
  if (bpp) *bpp = s_imginfo[id].bpp;
  return 1;
}

EM_JS(void, iupwasmJsImageGetRGBA, (int imgId, int ptr, int w, int h), {
  var im = globalThis.__iupImg && globalThis.__iupImg.map[imgId];
  if (!im || !im.canvas) return;
  var d = im.canvas.getContext('2d').getImageData(0, 0, w, h);
  HEAPU8.set(d.data, ptr);
})

IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  int id = (int)(intptr_t)handle, w, h, bpp, i;
  if (!imgdata || !iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp == 8)  /* an indexed source can't be recovered from the rendered RGBA */
    return;

  if (bpp == 32)
    iupwasmJsImageGetRGBA(id, (int)(intptr_t)imgdata, w, h);
  else  /* 24bpp: read RGBA, drop the alpha channel */
  {
    unsigned char* tmp = (unsigned char*)malloc((size_t)w * h * 4);
    iupwasmJsImageGetRGBA(id, (int)(intptr_t)tmp, w, h);
    for (i = 0; i < w * h; i++)
    {
      imgdata[i * 3 + 0] = tmp[i * 4 + 0];
      imgdata[i * 3 + 1] = tmp[i * 4 + 1];
      imgdata[i * 3 + 2] = tmp[i * 4 + 2];
    }
    free(tmp);
  }
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  (void)colors;
  if (colors_count) *colors_count = 0;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  int id = (int)(intptr_t)handle, w, h, bpp, x, y;
  unsigned char *tmp, *r, *g, *b, *a;
  size_t planesize;
  if (!imgdata || !iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;
  if (bpp == 8)  /* an indexed source can't be recovered from the rendered RGBA */
    return;

  tmp = (unsigned char*)malloc((size_t)w * h * 4);
  if (!tmp)
    return;
  iupwasmJsImageGetRGBA(id, (int)(intptr_t)tmp, w, h);  /* interleaved RGBA, top-down */

  /* IUP raw layout: separate R/G/B/A planes, rows bottom-up */
  planesize = (size_t)w * h;
  r = imgdata;
  g = imgdata + planesize;
  b = imgdata + 2 * planesize;
  a = imgdata + 3 * planesize;
  for (y = 0; y < h; y++)
  {
    int lineoffset = (h - 1 - y) * w;
    unsigned char* src = tmp + (size_t)y * w * 4;
    for (x = 0; x < w; x++)
    {
      r[lineoffset + x] = src[x * 4 + 0];
      g[lineoffset + x] = src[x * 4 + 1];
      b[lineoffset + x] = src[x * 4 + 2];
      if (bpp == 32)
        a[lineoffset + x] = src[x * 4 + 3];
    }
  }
  free(tmp);
}

/* PNG/JPEG via the browser canvas encoder; returns a _malloc'd buffer (caller frees). */
EM_JS(int, iupwasmJsEncodeImage, (int ptr, int w, int h, int bpp, const char* mimeStr, int sizePtr), {
  if (typeof document === 'undefined') { HEAP32[sizePtr >> 2] = 0; return 0; }  /* worker: not yet routed */
  var cv = document.createElement('canvas'); cv.width = w; cv.height = h;
  var ctx = cv.getContext('2d');
  var img = ctx.createImageData(w, h), n = w * h, i;
  if (bpp === 32) img.data.set(HEAPU8.subarray(ptr, ptr + n * 4));
  else if (bpp === 24) { for (i = 0; i < n; i++) { img.data[i*4] = HEAPU8[ptr+i*3]; img.data[i*4+1] = HEAPU8[ptr+i*3+1]; img.data[i*4+2] = HEAPU8[ptr+i*3+2]; img.data[i*4+3] = 255; } }
  else { HEAP32[sizePtr >> 2] = 0; return 0; }
  ctx.putImageData(img, 0, 0);
  var url = cv.toDataURL(UTF8ToString(mimeStr));
  var bin = atob(url.slice(url.indexOf(',') + 1)), len = bin.length;
  var out = _malloc(len);
  for (i = 0; i < len; i++) HEAPU8[out + i] = bin.charCodeAt(i);
  HEAP32[sizePtr >> 2] = len;
  return out;
})

/* browser has no filesystem: a "save" triggers a download of the encoded bytes */
EM_JS(void, iupwasmJsDownloadFile, (const char* nameStr, int ptr, int size, const char* mimeStr), {
  globalThis.__iupApply({ op: 'download', name: UTF8ToString(nameStr), bytes: HEAPU8.slice(ptr, ptr + size), mime: UTF8ToString(mimeStr) });
})

static const char* wasmImageMime(const char* format)
{
  if (iupStrEqualNoCase(format, "JPEG") || iupStrEqualNoCase(format, "JPG")) return "image/jpeg";
  if (iupStrEqualNoCase(format, "BMP")) return "image/bmp";
  return "image/png";
}

IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  if (iupStrEqualNoCase(format, "BMP"))
    return iupImageWriteBMP(imgdata, width, height, bpp, colors, colors_count, size);  /* canvas toDataURL has no BMP */
  {
    int sz = 0;
    unsigned char* buf = (unsigned char*)(intptr_t)iupwasmJsEncodeImage((int)(intptr_t)imgdata, width, height, bpp, wasmImageMime(format), (int)(intptr_t)&sz);
    if (size) *size = sz;
    return buf;
  }
}

IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  int sz = 0;
  unsigned char* buf = iupdrvImageSaveToBuffer(imgdata, width, height, bpp, colors, colors_count, format, &sz);
  if (!buf || !sz)
    return 0;
  iupwasmJsDownloadFile(filename, (int)(intptr_t)buf, sz, wasmImageMime(format));
  free(buf);
  return 1;
}

IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  (void)ih; (void)value;
  if (width) *width = 0;
  if (height) *height = 0;
  if (pixels) *pixels = NULL;
  return 0;
}

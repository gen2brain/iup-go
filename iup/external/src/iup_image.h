/** \file
 * \brief Image Resources (not exported API)
 *
 * See Copyright Notice in "iup.h"
 *
 */

/** \defgroup drvimage Driver Image Interface
 * \ingroup drv */
 
#ifndef __IUP_IMAGE_H 
#define __IUP_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif


enum {IUPIMAGE_IMAGE, IUPIMAGE_ICON, IUPIMAGE_CURSOR};

typedef struct _iupColor {
  unsigned char r, g, b, a;
} iupColor;

/** \addtogroup drvimage
 * @{ */
void* iupdrvImageCreateIcon(Ihandle *ih);
void* iupdrvImageCreateCursor(Ihandle *ih);
void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive);
void* iupdrvImageLoad(const char* name, int type);
IUP_SDK_API void  iupdrvImageDestroy(void* handle, int type);
int   iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp);
void  iupdrvImageGetData(void* handle, unsigned char* imgdata);
IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata);
IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count);
IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata);
int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format);
unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size);
/** @} */

void* iupImageGetIcon(const char* name);
void* iupImageGetCursor(const char* name);
void* iupImageGetImage(const char* name, Ihandle* parent, int make_inactive, const char* bgcolor);
void iupImageGetInfo(const char* name, int *w, int *h, int *bpp);
void iupImageRemoveFromCache(Ihandle* ih, void* handle);

IUP_SDK_API int iupImageInitColorTable(Ihandle *ih, iupColor* colors, int *colors_count);
void iupImageInitNonBgColors(Ihandle* ih, unsigned char *colors);
IUP_SDK_API void iupImageColorMakeInactive(unsigned char *r, unsigned char *g, unsigned char *b,
                               unsigned char bg_r, unsigned char bg_g, unsigned char bg_b);
int iupImageNormBpp(int bpp);

void iupImageResizeRGBA(int src_width, int src_height, unsigned char *src_map,
                        int dst_width, int dst_height, unsigned char *dst_map, int depth);

#define iupALPHABLEND(_src,_dst,_alpha) (unsigned char)(((_src) * (_alpha) + (_dst) * (255 - (_alpha))) / 255)

void iupImageSetHandleFromLoaded(const char* name, void* handle);
Ihandle* iupImageGetImageFromName(const char* name);

void iupImageStockInit(void);
void iupImageStockFinish(void);
typedef Ihandle* (*iupImageStockCreateFunc)(void);
IUP_SDK_API void iupImageStockSet(const char *name, iupImageStockCreateFunc func, const char* native_name);
IUP_SDK_API void iupImageStockSetNoResize(const char *name, iupImageStockCreateFunc func, const char* native_name);
IUP_SDK_API void iupImageStockLoadAll(void);  /* Used only in IupView and IupVisualLED */
int iupImageStockGetSize(void);
void iupImageStockGet(const char* name, Ihandle* *ih, const char* *native_name);

int iupIsHighDpi(void);


#ifdef __cplusplus
}
#endif

#endif

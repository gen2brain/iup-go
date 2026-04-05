/** \file
 * \brief Motif Color Management
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPMOT_COLOR_H
#define __IUPMOT_COLOR_H

#ifdef __cplusplus
extern "C" {
#endif


/* Returns a pixel value from a RGB triple. */
IUP_DRV_API extern unsigned long (* iupmotColorGetPixel)(unsigned char r, unsigned char g, unsigned char b);

/* Returns a RGB triple from a pixel value. */
IUP_DRV_API extern void (* iupmotColorGetRGB)(unsigned long pixel, unsigned char *r, unsigned char *g, unsigned char *b);

/* initialize the toplevel colormap and the iupmotColorGet* functions */
IUP_DRV_API void iupmotColorInit(void);
IUP_DRV_API void iupmotColorFinish(void);

/* returns the toplevel colormap */
IUP_DRV_API Colormap iupmotColorMap(void);

/* Returns a pixel value from a IUP color description. */
IUP_DRV_API unsigned long iupmotColorGetPixelStr(const char* color);


#ifdef __cplusplus
}
#endif

#endif

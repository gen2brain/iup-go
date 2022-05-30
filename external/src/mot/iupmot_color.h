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
extern unsigned long (* iupmotColorGetPixel)(unsigned char r, unsigned char g, unsigned char b);

/* Returns a RGB triple from a pixel value. */
extern void (* iupmotColorGetRGB)(unsigned long pixel, unsigned char *r, unsigned char *g, unsigned char *b);

/* initialize the toplevel colormap and the iupmotColorGet* functions */
void iupmotColorInit(void);
void iupmotColorFinish(void);

/* returns the toplevel colormap */
Colormap iupmotColorMap(void);

/* Returns a pixel value from a IUP color description. */
unsigned long iupmotColorGetPixelStr(const char* color);


#ifdef __cplusplus
}
#endif

#endif

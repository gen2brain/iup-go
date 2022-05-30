/** \file
 * \brief Windows Brush Cache
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPWIN_BRUSH_H 
#define __IUPWIN_BRUSH_H

#ifdef __cplusplus
extern "C" {
#endif

/* returns a brush from the brush cache. */
HBRUSH iupwinBrushGet(COLORREF c);

/* initializes the brush cache */
void iupwinBrushInit(void);
void iupwinBrushFinish(void);


#ifdef __cplusplus
}
#endif

#endif

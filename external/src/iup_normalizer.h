/** \file
 * \brief Normalizer Element (not exported API).
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_NORMALIZER_H 
#define __IUP_NORMALIZER_H

#ifdef __cplusplus
extern "C" {
#endif


void iupNormalizeSizeBoxChild(Ihandle *ih, int normalize, int children_natural_maxwidth, int children_natural_maxheight);
int iupNormalizeGetNormalizeSize(const char* value);
char* iupNormalizeGetNormalizeSizeStr(int normalize);


#ifdef __cplusplus
}
#endif

#endif

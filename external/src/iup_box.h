/** \file
 * \brief Base for box Controls (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_BOX_H 
#define __IUP_BOX_H

#ifdef __cplusplus
extern "C" {
#endif


struct _IcontrolData 
{
  int alignment,
      expand_children,
      is_homogeneous,
      normalize_size,
      margin_horiz,
      margin_vert,
      gap;
  int total_natural_size,   /* calculated in ComputeNaturalSize, used in SetChildrenCurrentSize */
      homogeneous_size;       /* calculated in SetChildrenCurrentSize, used in SetChildrenPosition */
};

Iclass* iupBoxNewClassBase(void);


#ifdef __cplusplus
}
#endif

#endif

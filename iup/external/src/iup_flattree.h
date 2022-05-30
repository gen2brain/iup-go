/** \file
 * \brief FlatTree Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_FLATTREE_H 
#define __IUP_FLATTREE_H

#ifdef __cplusplus
extern "C" {
#endif


struct _IcontrolData 
{
  int sb,  /* scrollbar configuration, can be changed only before map */
      nc,
      spacing,
      horiz_padding, 
      vert_padding,
      last_caret_pos,
      is_multiple,
      is_dropdown,
      has_editbox,
      maximg_w, maximg_h, /* used only in Windows */
      show_image,
      show_dragdrop;
  Imask* mask;
};


#ifdef __cplusplus
}
#endif

#endif

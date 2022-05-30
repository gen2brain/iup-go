/** \file
 * \brief Button Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_BUTTON_H 
#define __IUP_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif


void iupdrvButtonInitClass(Iclass* ic);
void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y);

char* iupButtonGetPaddingAttrib(Ihandle* ih);

enum{IUP_BUTTON_IMAGE=0x01, IUP_BUTTON_TEXT=0x02, IUP_BUTTON_BOTH=0x03};
enum{IUP_IMGPOS_LEFT, IUP_IMGPOS_RIGHT, IUP_IMGPOS_TOP, IUP_IMGPOS_BOTTOM};

struct _IcontrolData 
{
  int type,                         /* the 2 buttons possibilities */
      horiz_padding, vert_padding;  /* button margin */
  int spacing, img_position;        /* used when both text and image are displayed */

  /* used only by the Windows driver */
  int horiz_alignment, vert_alignment;  
  unsigned long fgcolor;
};


#ifdef __cplusplus
}
#endif

#endif

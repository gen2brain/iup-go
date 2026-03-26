/** \file
 * \brief Toggle Controls (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_TOGGLE_H 
#define __IUP_TOGGLE_H

#ifdef __cplusplus
extern "C" {
#endif


/** \addtogroup drv
 * @{ */
IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y);
IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y);
IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic);
/** Adds checkbox indicator size. */
IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str);
/** Adds switch indicator size. */
IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int *x, int *y, const char* str);
/** @} */

IUP_SDK_API Ihandle* iupRadioFindToggleParent(Ihandle* ih_toggle);
char* iupToggleGetPaddingAttrib(Ihandle* ih);

enum {IUP_TOGGLE_IMAGE, IUP_TOGGLE_TEXT};

struct _IcontrolData 
{
  int type,                         /* the 2 toggle possibilities */
      is_radio,
      flat,
      horiz_padding, vert_padding;  /* toggle margin for images */
};


#ifdef __cplusplus
}
#endif

#endif

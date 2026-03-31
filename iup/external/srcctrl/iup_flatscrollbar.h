/** \file
 * \brief flat scrollbar utilities
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_FLATSCROLLBAR_H 
#define __IUP_FLATSCROLLBAR_H

#ifdef __cplusplus
extern "C" {
#endif


IUP_SDK_API int iupFlatScrollBarCreate(Ihandle* ih);
IUP_SDK_API void iupFlatScrollBarRelease(Ihandle* ih);
IUP_SDK_API void iupFlatScrollBarRegister(Iclass* ic);

IUP_SDK_API int iupFlatScrollBarGet(Ihandle* ih);
                                               
IUP_SDK_API void iupFlatScrollBarSetChildrenCurrentSize(Ihandle* ih, int shrink);
IUP_SDK_API void iupFlatScrollBarSetChildrenPosition(Ihandle* ih);

IUP_SDK_API void iupFlatScrollBarWheelUpdate(Ihandle* ih, float delta);
IUP_SDK_API void iupFlatScrollBarMotionUpdate(Ihandle* ih, int x, int y);

/* SCROLL_CB callback must also be defined in client control,
   so the canvas can be redraw while scrolling the flatscrollbars */

/* For now, used only in IupFlatScrollBox */
IUP_SDK_API void iupFlatScrollBarSetPos(Ihandle *ih, int posx, int posy);


#ifdef __cplusplus
}
#endif

#endif

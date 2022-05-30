/** \file
 * \brief Base Class
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_CLASSBASE_H 
#define __IUP_CLASSBASE_H

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup iclassbase Base Class
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclass
 */


/** Register all common base attributes: \n
 * WID                                   \n
 * SIZE, RASTERSIZE, POSITION            \n
 * FONT (and derived)                    \n\n
 * All controls that are positioned inside a dialog must register all common base attributes.
 * \ingroup iclassbase */
  IUP_SDK_API void iupBaseRegisterCommonAttrib(Iclass* ic);

/** Register all visual base attributes: \n
 * VISIBLE, ACTIVE                       \n
 * ZORDER, X, Y                          \n
 * TIP (and derived)                     \n\n
 * All controls that are positioned inside a dialog must register all visual base attributes.
 * \ingroup iclassbase */
IUP_SDK_API void iupBaseRegisterVisualAttrib(Iclass* ic);

/** Register all base callbacks: \n
* MAP_CB, UNMAP_CB, DESTROY_CB, LDESTROY_CB.
* \ingroup iclassbase */
IUP_SDK_API void iupBaseRegisterBaseCallbacks(Iclass* ic);

/** Register all base and common callbacks: \n
* MAP_CB, UNMAP_CB, DESTROY_CB, LDESTROY_CB, GETFOCUS_CB, KILLFOCUS_CB, ENTERWINDOW_CB, LEAVEWINDOW_CB, K_ANY, HELP_CB.
* \ingroup iclassbase */
IUP_SDK_API void iupBaseRegisterCommonCallbacks(Iclass* ic);

/* Register driver dependent common attributes. 
   Used only from iupBaseRegisterCommonAttrib */
IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic);

/* Register driver dependent visual attributes. 
   Used only from iupBaseRegisterVisualAttrib */
IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic);

/** Updates the expand member of the IUP object from the EXPAND attribute.
 * Should be called in the beginning of the ComputeNaturalSize for a container.
 * \ingroup iclassbase */
IUP_SDK_API void iupBaseContainerUpdateExpand(Ihandle* ih);

/** Initializes the natural size using the user size, then
 * if a container then update the "expand" member from the EXPAND attribute, then
 * call \ref iupClassObjectComputeNaturalSize for containers if they have children or
 * call \ref iupClassObjectComputeNaturalSize for non-containers if user size is not defined.
 * Must be called for each children in the container. \n
 * First call is in iupLayoutCompute.
 * \ingroup iclassbase */
IUP_SDK_API void iupBaseComputeNaturalSize(Ihandle* ih);

/** Update the current size from the available size, the natural size, expand and shrink.
 * Call \ref iupClassObjectSetChildrenCurrentSize for containers if they have children.
 * Must be called for each children in the container. \n
 * First call is in iupLayoutCompute.
 * \ingroup iclassbase */
IUP_SDK_API void iupBaseSetCurrentSize(Ihandle* ih, int w, int h, int shrink);

/** Set the current position and update children position for containers.
 * Call \ref iupClassObjectSetChildrenPosition for containers if they have children.
 * Must be called for each children in the container. \n
 * First call is in iupLayoutCompute.
 * \ingroup iclassbase */
IUP_SDK_API void iupBaseSetPosition(Ihandle* ih, int x, int y);

/* Updates the SIZE attribute if defined. 
   Called only from iupdrvSetFontAttrib. */
IUP_SDK_API void iupBaseUpdateAttribFromFont(Ihandle* ih);


/** \defgroup iclassbasemethod Base Class Methods
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclassbase
 */

/** Driver dependent \ref Iclass::LayoutUpdate method.
 * \ingroup iclassbasemethod */
IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih);

/** Driver dependent \ref Iclass::UnMap method.
 * \ingroup iclassbasemethod */
IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih);

/** Native type void \ref Iclass::Map method.
 * \ingroup iclassbasemethod */
IUP_SDK_API int iupBaseTypeVoidMapMethod(Ihandle* ih);


/** \defgroup iclassbaseattribfunc Base Class Attribute Functions
 * \par
 * Used by the controls for iupClassRegisterAttribute. 
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclassbase
 * @{
 */

/* common */
IUP_SDK_API char* iupBaseGetWidAttrib(Ihandle* ih);
IUP_SDK_API int iupBaseSetNameAttrib(Ihandle* ih, const char* value);
IUP_SDK_API int iupBaseSetRasterSizeAttrib(Ihandle* ih, const char* value);
IUP_SDK_API int iupBaseSetSizeAttrib(Ihandle* ih, const char* value);
IUP_SDK_API char* iupBaseGetSizeAttrib(Ihandle* ih);
IUP_SDK_API char* iupBaseGetCurrentSizeAttrib(Ihandle* ih);
IUP_SDK_API char* iupBaseGetRasterSizeAttrib(Ihandle* ih);
IUP_SDK_API char* iupBaseGetClientOffsetAttrib(Ihandle* ih);
IUP_SDK_API char* iupBaseGetClientSizeAttrib(Ihandle* ih);
IUP_SDK_API char* iupBaseCanvasGetClientOffsetAttrib(Ihandle* ih);
IUP_SDK_API char* iupBaseCanvasGetClientSizeAttrib(Ihandle* ih);
IUP_SDK_API int iupBaseSetMaxSizeAttrib(Ihandle* ih, const char* value);
IUP_SDK_API int iupBaseSetMinSizeAttrib(Ihandle* ih, const char* value);
IUP_SDK_API char* iupBaseGetExpandAttrib(Ihandle* ih);
IUP_SDK_API int iupBaseSetExpandAttrib(Ihandle* ih, const char* value);

/* visual */
IUP_SDK_API char* iupBaseGetVisibleAttrib(Ihandle* ih);
IUP_SDK_API int iupBaseSetVisibleAttrib(Ihandle* ih, const char* value);
IUP_SDK_API char* iupBaseGetActiveAttrib(Ihandle *ih);
IUP_SDK_API int iupBaseSetActiveAttrib(Ihandle* ih, const char* value);
IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value);
IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value);
IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value);
IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih);
IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value);
IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value);
IUP_SDK_API char* iupBaseNativeParentGetBgColorAttrib(Ihandle* ih);
IUP_SDK_API int iupBaseSetCPaddingAttrib(Ihandle* ih, const char* value);
IUP_SDK_API char* iupBaseGetCPaddingAttrib(Ihandle* ih);
IUP_SDK_API int iupBaseSetCSpacingAttrib(Ihandle* ih, const char* value);
IUP_SDK_API char* iupBaseGetCSpacingAttrib(Ihandle* ih);

/* other */
IUP_SDK_API char* iupBaseContainerGetExpandAttrib(Ihandle* ih);
IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value);

/* drag&drop */
IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic);

/* util */
IUP_SDK_API int iupBaseNoSaveCheck(Ihandle* ih, const char* name);


/** @} */



/** \defgroup iclassbaseutil Base Class Utilities
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclassbase
 * @{
 */

#define iupMAX(_a,_b) ((_a)>(_b)?(_a):(_b))
#define iupMIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#define iupROUND(_x) ((int)((_x)>0? (_x)+0.5: (_x)-0.5))
IUP_SDK_API int     iupRound(double x);

#define iupCOLOR8TO16(_x) ((unsigned short)(_x*257))  
#define iupCOLOR16TO8(_x) ((unsigned char)(_x/257))   /* 65535/257 = 255 */

#define iupBYTECROP(_x)   ((unsigned char)((_x)<0?0:((_x)>255)?255:(_x)))

enum{IUP_ALIGN_ALEFT, IUP_ALIGN_ACENTER, IUP_ALIGN_ARIGHT};
#define IUP_ALIGN_ABOTTOM IUP_ALIGN_ARIGHT
#define IUP_ALIGN_ATOP IUP_ALIGN_ALEFT

enum{IUP_SB_NONE, IUP_SB_HORIZ, IUP_SB_VERT};
IUP_SDK_API int iupBaseGetScrollbar(Ihandle* ih);

IUP_SDK_API char* iupBaseNativeParentGetBgColor(Ihandle* ih);
IUP_SDK_API void iupBaseCallValueChangedCb(Ihandle* ih);

/** @} */


#ifdef __cplusplus
}
#endif

#endif

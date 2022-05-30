/** \file
 * \brief Abstract Layout Management (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_LAYOUT_H 
#define __IUP_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

/* called from IupMap and IupRefresh */
void iupLayoutCompute(Ihandle* ih);  /* can be called before map */
IUP_SDK_API void iupLayoutUpdate(Ihandle* ih);   /* called after map */

IUP_SDK_API void iupLayoutApplyMinMaxSize(Ihandle* ih, int *w, int *h);

/* Other functions declared in <iup.h> and implemented here. 
IupRefresh
*/

/* at iup_layout_dlg */

IUP_SDK_API Ihandle* iupLayoutFindElementDialog(Ihandle *tree, Ihandle* elem);
IUP_SDK_API void iupLayoutPropertiesUpdate(Ihandle* properties, Ihandle* elem);
char* iupLayoutGetElementTitle(Ihandle* elem);
int iupLayoutAttributeHasChanged(Ihandle* elem, const char* name, const char* value, const char* def_value, int flags);

enum { IUP_LAYOUT_EXPORT_LUA, IUP_LAYOUT_EXPORT_C, IUP_LAYOUT_EXPORT_LED };
/* at iup_export */
#if defined(FILE) || defined(_INC_STDIO) || defined(_STDIO_H_) || defined(_STDIO_H)
IUP_SDK_API void iupLayoutExportNamedElemList(FILE* file, Ihandle* *named_elem, int count, int export_format, int saved_info);
IUP_SDK_API void iupLayoutExportNamedImageListSetHandle(FILE* file, Ihandle* *named_elem, int count, int export_format);
IUP_SDK_API void iupLayoutExportNamedImageList(FILE* file, Ihandle* *named_elem, int count, int export_format);
#endif

#ifdef __cplusplus
}
#endif

#endif

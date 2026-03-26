/** \file
 * \brief Tabs Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_TABS_H
#define __IUP_TABS_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup drvtabs Driver Tabs Interface
 * \ingroup drv */

char* iupTabsGetTabOrientationAttrib(Ihandle* ih);
char* iupTabsGetTabTypeAttrib(Ihandle* ih);
char* iupTabsGetTabPaddingAttrib(Ihandle* ih);
char* iupTabsGetTabVisibleAttrib(Ihandle* ih, int pos);
char* iupTabsGetTitleAttrib(Ihandle* ih, int pos);

void iupTabsCheckCurrentTab(Ihandle* ih, int pos, int removed);

/** \addtogroup drvtabs
 * @{ */
IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* child, int pos);
IUP_SDK_API int iupdrvTabsExtraDecor(Ihandle* ih);
IUP_SDK_API int iupdrvTabsExtraMargin(void);
IUP_SDK_API int iupdrvTabsGetLineCountAttrib(Ihandle* ih);
IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos);
IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih);
IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic);
IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height);
/** @} */

typedef enum
{
  ITABS_TOP, ITABS_BOTTOM, ITABS_LEFT, ITABS_RIGHT
} ItabsType;

typedef enum
{
  ITABS_HORIZONTAL, ITABS_VERTICAL
} ItabsOrientation;

/* Control context */
struct _IcontrolData
{
  ItabsType type;
  ItabsOrientation orientation;
  int horiz_padding, vert_padding;  /* tab title margin */
  int show_close;
  int is_multiline;   /* used only in Windows */
  int has_invisible;  /* used only in Windows */
};


#ifdef __cplusplus
}
#endif

#endif

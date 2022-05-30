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


char* iupTabsGetTabOrientationAttrib(Ihandle* ih);
char* iupTabsGetTabTypeAttrib(Ihandle* ih);
char* iupTabsGetTabPaddingAttrib(Ihandle* ih);
char* iupTabsGetTabVisibleAttrib(Ihandle* ih, int pos);
char* iupTabsGetTitleAttrib(Ihandle* ih, int pos);

void iupTabsCheckCurrentTab(Ihandle* ih, int pos, int removed);

int iupdrvTabsIsTabVisible(Ihandle* child, int pos);
int iupdrvTabsExtraDecor(Ihandle* ih);
int iupdrvTabsExtraMargin(void);
int iupdrvTabsGetLineCountAttrib(Ihandle* ih);
void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos);
int iupdrvTabsGetCurrentTab(Ihandle* ih);
void iupdrvTabsInitClass(Iclass* ic);

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

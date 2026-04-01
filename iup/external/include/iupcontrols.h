/** \file
 * \brief Additional Controls API
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPCONTROLS_H
#define __IUPCONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IUPCONTROLS_API
#ifdef IUPCONTROLS_BUILD_LIBRARY
  #if defined(_WIN32)
    #define IUPCONTROLS_API __declspec(dllexport)
  #elif defined(__GNUC__) && __GNUC__ >= 4
    #define IUPCONTROLS_API __attribute__ ((visibility("default")))
  #else
    #define IUPCONTROLS_API
  #endif
#else
  #define IUPCONTROLS_API
#endif /* IUPCONTROLS_BUILD_LIBRARY */
#endif /* IUPCONTROLS_API */


IUPCONTROLS_API int IupControlsOpen(void);

/** \defgroup matrixcontrols Matrix Controls
 * \ingroup ctrl */
/** @{ */
IUPCONTROLS_API Ihandle* IupCells(void);
IUPCONTROLS_API Ihandle* IupMatrix(const char* action);
IUPCONTROLS_API Ihandle* IupMatrixList(void);
IUPCONTROLS_API Ihandle* IupMatrixEx(void);
/** @} */

/** \defgroup flatcontrols Flat Controls
 * \ingroup ctrl */
/** @{ */
IUPCONTROLS_API Ihandle* IupFlatButton(const char* title);
IUPCONTROLS_API Ihandle* IupFlatLabel(const char* title);
IUPCONTROLS_API Ihandle* IupFlatToggle(const char* title);
IUPCONTROLS_API Ihandle* IupFlatFrame(Ihandle* child);
IUPCONTROLS_API Ihandle* IupFlatTabs(Ihandle* first, ...);
IUPCONTROLS_API Ihandle* IupFlatTabsv(Ihandle** children);
IUPCONTROLS_API Ihandle* IupFlatScrollBox(Ihandle* child);
IUPCONTROLS_API Ihandle* IupFlatList(void);
IUPCONTROLS_API Ihandle* IupFlatTree(void);
IUPCONTROLS_API Ihandle* IupFlatVal(const char* type);
IUPCONTROLS_API Ihandle* IupDropButton(Ihandle* dropchild);
/** @} */


#ifdef __cplusplus
}
#endif

#endif

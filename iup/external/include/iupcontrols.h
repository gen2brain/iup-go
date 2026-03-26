/** \file
 * \brief Cells and Matrix controls.
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


IUPCONTROLS_API int  IupControlsOpen(void);

IUPCONTROLS_API Ihandle* IupCells(void);
IUPCONTROLS_API Ihandle* IupMatrix(const char *action);
IUPCONTROLS_API Ihandle* IupMatrixList(void);
IUPCONTROLS_API Ihandle* IupMatrixEx(void);


#ifdef __cplusplus
}
#endif

#endif

/** \file
 * \brief Web control.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWEB_H
#define __IUPWEB_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IUPWEB_API
#ifdef IUPWEB_BUILD_LIBRARY
  #if defined(_WIN32)
    #define IUPWEB_API __declspec(dllexport)
  #elif defined(__GNUC__) && __GNUC__ >= 4
    #define IUPWEB_API __attribute__ ((visibility("default")))
  #else
    #define IUPWEB_API
  #endif
#else
  #define IUPWEB_API
#endif /* IUPWEB_BUILD_LIBRARY */
#endif /* IUPWEB_API */


IUPWEB_API int IupWebBrowserOpen(void);

IUPWEB_API Ihandle *IupWebBrowser(void);


#ifdef __cplusplus
}
#endif

#endif

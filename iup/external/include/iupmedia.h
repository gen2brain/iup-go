/** \file
 * \brief Media controls.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPMEDIA_H
#define __IUPMEDIA_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IUPMEDIA_API
#ifdef IUPMEDIA_BUILD_LIBRARY
  #if defined(_WIN32)
    #define IUPMEDIA_API __declspec(dllexport)
  #elif defined(__GNUC__) && __GNUC__ >= 4
    #define IUPMEDIA_API __attribute__ ((visibility("default")))
  #else
    #define IUPMEDIA_API
  #endif
#else
  #define IUPMEDIA_API
#endif /* IUPMEDIA_BUILD_LIBRARY */
#endif /* IUPMEDIA_API */

IUPMEDIA_API int IupMediaOpen(void);
IUPMEDIA_API Ihandle *IupAudio(void);
IUPMEDIA_API Ihandle *IupCamera(void);

#ifdef __cplusplus
}
#endif

#endif

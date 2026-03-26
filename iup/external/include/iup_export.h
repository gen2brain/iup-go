#ifndef __IUP_EXPORT_H
#define __IUP_EXPORT_H

/* Mark the official functions */
#ifndef IUP_API
#ifdef IUP_BUILD_LIBRARY
  #if defined(_WIN32)
    #define IUP_API __declspec(dllexport)
  #elif defined(__GNUC__) && __GNUC__ >= 4
    #define IUP_API __attribute__ ((visibility("default")))
  #else
    #define IUP_API
  #endif
#else
  #define IUP_API
#endif /* IUP_BUILD_LIBRARY */
#endif /* IUP_API */

/* Mark the internal SDK functions (some not official but need to be exported) */
#ifndef IUP_SDK_API
#ifdef IUP_BUILD_LIBRARY
  #if defined(_WIN32)
    #define IUP_SDK_API __declspec(dllexport)
  #elif defined(__GNUC__) && __GNUC__ >= 4
    #define IUP_SDK_API __attribute__ ((visibility("default")))
  #else
    #define IUP_SDK_API
  #endif
#else
  #define IUP_SDK_API
#endif /* IUP_BUILD_LIBRARY */
#endif /* IUP_SDK_API */

/* Mark the driver functions that need to be exported */
#ifndef IUP_DRV_API
#ifdef IUP_BUILD_LIBRARY
  #if defined(_WIN32)
    #define IUP_DRV_API __declspec(dllexport)
  #elif defined(__GNUC__) && __GNUC__ >= 4
    #define IUP_DRV_API __attribute__ ((visibility("default")))
  #else
    #define IUP_DRV_API
  #endif
#else
  #define IUP_DRV_API
#endif /* IUP_BUILD_LIBRARY */
#endif /* IUP_DRV_API */


#endif /* __IUP_EXPORT_H */

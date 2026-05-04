/** \file
 * \brief JNI caching macros for the Android Driver.
 *
 * Thin wrappers over FindClass / GetMethodID / GetStaticMethodID / GetObjectClass
 * that optionally cache results, gated by IUP_ENABLE_CACHE_JAVA_CLASS and
 * IUP_ENABLE_CACHE_JAVA_METHOD_ID; flip a switch to bisect.
 *
 * Macros use GNU statement-expressions so they work as rvalues; clang is the
 * only NDK compiler. Cached classes are NewGlobalRef'd for life-of-JVM; method
 * IDs are JVM-stable and don't consume ref slots.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_ANDROID_JNI_MACROS_H
#define __IUP_ANDROID_JNI_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>
#include <android/log.h>


/* Usage:
 *
 *   IUPJNI_DECLARE_CLASS_STATIC(IupButtonHelper);
 *   ... or _GLOBAL(name) in one .c + _EXTERN(name) in a header for cross-file caches.
 *   jclass cls = IUPJNI_FindClass(IupButtonHelper, env, "io/github/gen2brain/iupgo/IupButtonHelper");
 *   ... DeleteLocalRef(cls); is always correct (cached or not).
 *
 *   IUPJNI_DECLARE_METHOD_ID_STATIC(IupButtonHelper_createButton);
 *   jmethodID m = IUPJNI_GetStaticMethodID(IupButtonHelper_createButton, env, cls,
 *                                          "createButton", "(J)Landroid/widget/Button;");
 *   ... varname is `ClassName_methodName`; for overloads, append a discriminator.
 *
 * Class-loader caveat: pthread-attached threads can't FindClass app classes;
 * pre-cache those in JNI_OnLoad (see iupandroid_common.c).
 */


#define IUP_ENABLE_CACHE_JAVA_CLASS     1
#define IUP_ENABLE_CACHE_JAVA_METHOD_ID 1


#if defined(IUP_ENABLE_CACHE_JAVA_CLASS) && (IUP_ENABLE_CACHE_JAVA_CLASS == 1)

    #define IUPJNI_DECLARE_CLASS_GLOBAL(varname) \
        jobject g_javaClass ## varname = NULL

    #define IUPJNI_DECLARE_CLASS_STATIC(varname) \
        static jobject g_javaClass ## varname = NULL

    #define IUPJNI_DECLARE_CLASS_EXTERN(varname) \
        extern jobject g_javaClass ## varname

    #define IUPJNI_FindClass(varname, jni_env, classstr) \
    ({ \
        jobject tmp_jclass_ ## varname = NULL; \
        if (NULL == g_javaClass ## varname) \
        { \
            tmp_jclass_ ## varname = (*jni_env)->FindClass(jni_env, classstr); \
            if (NULL == tmp_jclass_ ## varname) \
            { \
                if ((*jni_env)->ExceptionCheck(jni_env)) \
                { \
                    (*jni_env)->ExceptionDescribe(jni_env); \
                    (*jni_env)->ExceptionClear(jni_env); \
                } \
                __android_log_print(ANDROID_LOG_ERROR, "Iup", "IUPJNI_FindClass: class not found: %s", classstr); \
            } \
            else \
            { \
                g_javaClass ## varname = (jobject)((*jni_env)->NewGlobalRef(jni_env, tmp_jclass_ ## varname)); \
            } \
        } \
        else \
        { \
            tmp_jclass_ ## varname = (jobject)((*jni_env)->NewLocalRef(jni_env, g_javaClass ## varname)); \
        } \
        tmp_jclass_ ## varname; \
    })

    #define IUPJNI_GetObjectClass(varname, jni_env, java_object) \
    ({ \
        jobject tmp_jclass_ ## varname = NULL; \
        if (NULL == g_javaClass ## varname) \
        { \
            tmp_jclass_ ## varname = (*jni_env)->GetObjectClass(jni_env, java_object); \
            if (NULL == tmp_jclass_ ## varname) \
            { \
                if ((*jni_env)->ExceptionCheck(jni_env)) \
                { \
                    (*jni_env)->ExceptionDescribe(jni_env); \
                    (*jni_env)->ExceptionClear(jni_env); \
                } \
                __android_log_print(ANDROID_LOG_ERROR, "Iup", "IUPJNI_GetObjectClass: failed for " #varname); \
            } \
            else \
            { \
                g_javaClass ## varname = (jobject)((*jni_env)->NewGlobalRef(jni_env, tmp_jclass_ ## varname)); \
            } \
        } \
        else \
        { \
            tmp_jclass_ ## varname = (jobject)((*jni_env)->NewLocalRef(jni_env, g_javaClass ## varname)); \
        } \
        tmp_jclass_ ## varname; \
    })

#else  /* caching disabled */

    #define IUPJNI_DECLARE_CLASS_GLOBAL(varname)
    #define IUPJNI_DECLARE_CLASS_STATIC(varname)
    #define IUPJNI_DECLARE_CLASS_EXTERN(varname)

    #define IUPJNI_FindClass(varname, jni_env, classstr) \
        ((*jni_env)->FindClass(jni_env, classstr))

    #define IUPJNI_GetObjectClass(varname, jni_env, java_object) \
        ((*jni_env)->GetObjectClass(jni_env, java_object))

#endif



#if defined(IUP_ENABLE_CACHE_JAVA_METHOD_ID) && (IUP_ENABLE_CACHE_JAVA_METHOD_ID == 1)

    #define IUPJNI_DECLARE_METHOD_ID_GLOBAL(varname) \
        jmethodID s_methodID_ ## varname = 0

    #define IUPJNI_DECLARE_METHOD_ID_STATIC(varname) \
        static jmethodID s_methodID_ ## varname = 0

    #define IUPJNI_DECLARE_METHOD_ID_EXTERN(varname) \
        extern jmethodID s_methodID_ ## varname

    #define IUPJNI_GetStaticMethodID(varname, jni_env, java_class, method_name, method_signature) \
    ({ \
        if (0 == s_methodID_ ## varname) \
        { \
            jmethodID tmp_method_ ## varname = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_signature); \
            if (0 == tmp_method_ ## varname) \
            { \
                if ((*jni_env)->ExceptionCheck(jni_env)) \
                { \
                    (*jni_env)->ExceptionDescribe(jni_env); \
                    (*jni_env)->ExceptionClear(jni_env); \
                } \
                __android_log_print(ANDROID_LOG_ERROR, "Iup", "IUPJNI_GetStaticMethodID: %s %s not found", method_name, method_signature); \
            } \
            s_methodID_ ## varname = tmp_method_ ## varname; \
        } \
        s_methodID_ ## varname; \
    })

    #define IUPJNI_GetMethodID(varname, jni_env, java_class, method_name, method_signature) \
    ({ \
        if (0 == s_methodID_ ## varname) \
        { \
            jmethodID tmp_method_ ## varname = (*jni_env)->GetMethodID(jni_env, java_class, method_name, method_signature); \
            if (0 == tmp_method_ ## varname) \
            { \
                if ((*jni_env)->ExceptionCheck(jni_env)) \
                { \
                    (*jni_env)->ExceptionDescribe(jni_env); \
                    (*jni_env)->ExceptionClear(jni_env); \
                } \
                __android_log_print(ANDROID_LOG_ERROR, "Iup", "IUPJNI_GetMethodID: %s %s not found", method_name, method_signature); \
            } \
            s_methodID_ ## varname = tmp_method_ ## varname; \
        } \
        s_methodID_ ## varname; \
    })

#else  /* caching disabled */

    #define IUPJNI_DECLARE_METHOD_ID_GLOBAL(varname)
    #define IUPJNI_DECLARE_METHOD_ID_STATIC(varname)
    #define IUPJNI_DECLARE_METHOD_ID_EXTERN(varname)

    #define IUPJNI_GetStaticMethodID(varname, jni_env, java_class, method_name, method_signature) \
        ((*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_signature))

    #define IUPJNI_GetMethodID(varname, jni_env, java_class, method_name, method_signature) \
        ((*jni_env)->GetMethodID(jni_env, java_class, method_name, method_signature))

#endif


#ifdef __cplusplus
}
#endif

#endif

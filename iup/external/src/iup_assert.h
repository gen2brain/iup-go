/** \file
 * \brief Assert Utilities
 *
 * See Copyright Notice in "iup.h"
 */

 
#ifndef __IUP_ASSERT_H 
#define __IUP_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup assert Assert Utilities
 * \par
 * All functions of the main API (Iup***) calls iupASSERT to check the parameters.
 * \par
 * The IUP main library must be recompiled with the IUP_ASSERT define to enable these checks.
 * iupASSERT is not called inside driver dependent functions nor in each control implementation, 
 * it is used only in the functions of the main API and in some utilities.
 * \par
 * See \ref iup_assert.h
 * \ingroup util */

/* internal functions */
IUP_SDK_API void iupAssert(const char* expr, const char* file, int line, const char* func);
IUP_SDK_API void iupError(const char* format, ...);

/** \def iupASSERT
 * \brief If the expression if false, 
 * displays a message with information of the source code where the assert happen.
 *
 * \param _expr The evaluated expression.
 * \par
 * It is a macro that calls a function only if IUP_ASSERT is defined.
 * \ingroup assert */

/** \def iupERROR
 * \brief Displays an error message. Also used by the iupASSERT. 
 * \par
 * It is a macro that calls a function only if IUP_ASSERT is defined.
 * \ingroup assert */

#ifndef  IUP_ASSERT
#define iupASSERT(_expr)  ((void)0)
#define iupERROR(_msg)  ((void)0)
#define iupERROR1(_msg, _p1)  ((void)0)
#define iupERROR2(_msg, _p1, _p2)  ((void)0)
#else
#ifdef __FUNCTION__
#define iupASSERT(_expr) ((_expr)? (void)0: iupAssert(#_expr, __FILE__, __LINE__, __FUNCTION__))
#else
#define iupASSERT(_expr) ((_expr)? (void)0: iupAssert(#_expr, __FILE__, __LINE__, NULL))
#endif  /* __FUNCTION__ */

#define iupERROR(_msg) iupError(_msg)
#define iupERROR1(_msg, _p1) iupError(_msg, _p1)
#define iupERROR2(_msg, _p1, _p2) iupError(_msg, _p1, _p2)

#endif  /* IUP_ASSERT */

#ifdef __cplusplus
}
#endif

#endif

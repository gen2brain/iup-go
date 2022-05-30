/** \file
 * \brief Register the Controls
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_REGISTER_H 
#define __IUP_REGISTER_H

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup register Class Registration
 * \par
 * All controls are registered so the creation using IupCreate can work.
 * \par
 * See \ref iup_register.h
 * \ingroup cpi */


/** Returns a class instance from a class name. The class name must be previously registered using \ref iupRegisterClass.
 * \ingroup register */
IUP_SDK_API Iclass* iupRegisterFindClass(const char* name);

/** Register a class.
 * \ingroup register */
IUP_SDK_API void iupRegisterClass(Iclass* ic);
                                     
                                     
/* Register the internal classes. Called only from IupOpen. */
void iupRegisterInternalClasses(void);

/* Class ClassUpdate for all classes that have it */
void iupRegisterUpdateClasses(void);


/* Initializes the class registry. Called only from IupOpen. */
void iupRegisterInit(void);
void iupRegisterFinish(void);


#ifdef __cplusplus
}
#endif

#endif

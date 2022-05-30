/** \file
 * \brief Mask functions
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_MASK_H 
#define __IUP_MASK_H

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup mask Text Mask
 * \par
 * Used to filter text input in IupText.
 * \par
 * See \ref iup_mask.h
 * \ingroup util */

typedef struct _Imask Imask;

/** Creates a mask given a string.  \n
 * If casei is true, will turn the mask case insensitive.
 * \ingroup mask */
IUP_SDK_API Imask* iupMaskCreate(const char* mask_str);

/** Creates an integer mask with limits.
 * \ingroup mask */
IUP_SDK_API Imask* iupMaskCreateInt(int min, int max);

/** Creates a real mask with limits.
 * \ingroup mask */
IUP_SDK_API Imask* iupMaskCreateFloat(float min, float max, const char* decimal_symbol);

/** Creates a real mask.
* \ingroup mask */
IUP_SDK_API Imask* iupMaskCreateReal(int positive, const char* decimal_symbol);

/** If casei is 1, will turn the mask case insensitive.
* Default is case sensitive.
* \ingroup mask */
IUP_SDK_API void iupMaskSetCaseI(Imask* mask, int casei);

/** If noempty is 1, the value can NOT be empty.
* Default can be empty.
* \ingroup mask */
IUP_SDK_API void iupMaskSetNoEmpty(Imask* mask, int noempty);

/** Destroys the mask.
 * \ingroup mask */
IUP_SDK_API void iupMaskDestroy(Imask* mask);

/** Check if the value is valid using the mask to filter it.
 * Returns 1 if full match, -1 if partial match, and 0 otherwise.
 * \ingroup mask */
IUP_SDK_API int iupMaskCheck(Imask* mask, const char *value);

/** Returns the mask string.
 * \ingroup mask */
IUP_SDK_API char* iupMaskGetStr(Imask* mask);


#ifdef __cplusplus
}
#endif

#endif

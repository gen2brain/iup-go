/** \file
 * \brief Mask internal functions (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_MASKPARSE_H 
#define __IUP_MASKPARSE_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _ImaskParsed
{
  char ch;
  int command;
  int next1;
  int next2;
} ImaskParsed;

typedef int (*iMaskMatchFunc) (char which_one, long next_pos, long capture_pos, const char *text, void* user_data);

/* Parse the mask and if it is ok create and returns the internal structure. */
int iupMaskParse(const char* mask, ImaskParsed** imk);

/* Do the pattern matching on the given text. */
int iupMaskMatch(const char* text, ImaskParsed* imk, long start, iMaskMatchFunc mask_func, void* user_data, char *addchar, int icase);

/* Change a control character. */
int iupMaskSetChar(int char_number, char new_char);

/* iupMaskMatch return codes */
#define IMASK_PARSE_OK     0    /* No error      */
#define IMASK_NOMATCH     -1    /* no match      */
#define IMASK_MEM_ERROR   -2    /* memory error  */
#define IMASK_PARSE_ERROR -3    /* parser error  */
#define IMASK_PARTIALMATCH -4   /* partial match */


#ifdef __cplusplus
}
#endif

#endif

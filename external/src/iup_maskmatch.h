/** \file
 * \brief Mask match private definitions (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_MASKMATCH_H 
#define __IUP_MASKMATCH_H

#ifdef __cplusplus
extern "C" {
#endif


enum
{
  IMASK_NULL_CMD=1,
  IMASK_ANY_CMD=2,
  IMASK_CHAR_CMD=3,
  IMASK_SPC_CMD=4,
  IMASK_CLASS_CMD=5,
  IMASK_BEGIN_CMD=6,
  IMASK_END_CMD=7,
  IMASK_CAP_OPEN_CMD=71,
  IMASK_CAP_CLOSE_CMD=72,
  IMASK_NEG_OPEN_CMD=81,
  IMASK_NEG_CLOSE_CMD=82
};

enum
{
  IMASK_CLASS_CMD_RANGE=50,
  IMASK_CLASS_CMD_CHAR=51
};

enum 
{
  IMASK_NORMAL_MATCH, 
  IMASK_NO_CHAR_MATCH, 
  IMASK_NO_MATCH
};

enum
{
  IMASK_CAPTURE=100,
  IMASK_NOCAPTURE=101
};

typedef struct _ImaskMatchFunc
{
  char ch;
  int (*function) (const char *, long);
} ImaskMatchFunc;

ImaskMatchFunc* iupMaskMatchGetFuncs(void);


#ifdef __cplusplus
}
#endif

#endif

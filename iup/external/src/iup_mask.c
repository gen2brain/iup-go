/** \file
 * \brief mask pattern matching
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "iup_export.h"
#include "iup_maskparse.h"
#include "iup_mask.h"
#include "iup_str.h"

/* redefine here to avoid include iup.h */
#define IUP_MASK_FLOAT        "[+/-]?(/d+/.?/d*|/./d+)"
#define IUP_MASK_UFLOAT             "(/d+/.?/d*|/./d+)"
#define IUP_MASK_FLOATCOMMA   "[+/-]?(/d+/,?/d*|/,/d+)"
#define IUP_MASK_UFLOATCOMMA        "(/d+/,?/d*|/,/d+)"
#define IUP_MASK_INT           "[+/-]?/d+"
#define IUP_MASK_UINT                "/d+"

struct _Imask
{
  char* mask_str;
  ImaskParsed* fsm;
  int casei;
  int noempty;
  char type;
  float fmin, 
        fmax;
  int   imin,
        imax;
};


IUP_SDK_API int iupMaskCheck(Imask* mask, const char *val)
{
  int ret;

  /* no mask */
  if (!mask)
    return 1;

  /* NULL or empty text */
  if (!val || val[0]==0)
  {
    if (mask->noempty)
      return 0;
    else
      return 1;
  }

  ret = iupMaskMatch(val, mask->fsm, 0, NULL, NULL, NULL, mask->casei);
  if (ret == IMASK_PARTIALMATCH)
    return -1;
  if (ret != (int)strlen(val))
    return 0;

  switch(mask->type)
  {
  case 'I':
    {
      int ival = 0;
      iupStrToInt(val, &ival);
      if(ival < mask->imin || ival > mask->imax)
        return 0;
      break;
    }
  case 'F':
    {
      float fval = 0;
      iupStrToFloat(val, &fval);
      if(fval < mask->fmin || fval > mask->fmax)
        return 0;
      break;
    }
  }

  return 1;
}

IUP_SDK_API void iupMaskSetNoEmpty(Imask* mask, int noempty)
{
  if (!mask)
    return;

  mask->noempty = noempty;
}

IUP_SDK_API void iupMaskSetCaseI(Imask* mask, int casei)
{
  if (!mask)
    return;

  mask->casei = casei;
}

IUP_SDK_API Imask* iupMaskCreate(const char* mask_str)
{
  ImaskParsed* fsm;
  Imask* mask;
  char* copy_mask_str;

  if (!mask_str)
    return NULL;

  /* Parse the mask first */
  copy_mask_str = iupStrDup(mask_str);
  if (iupMaskParse(copy_mask_str, &fsm) != IMASK_PARSE_OK)
  {
    free(copy_mask_str);
    return NULL;
  }

  mask = (Imask*)malloc(sizeof(Imask));
  memset(mask, 0, sizeof(Imask));

  mask->mask_str = copy_mask_str;
  mask->casei = 0;
  mask->noempty = 0;
  mask->fsm = fsm;

  return mask;
}

IUP_SDK_API Imask* iupMaskCreateInt(int min, int max)
{
  Imask* mask;

  if (min < 0)
    mask = iupMaskCreate(IUP_MASK_INT);
  else
    mask = iupMaskCreate(IUP_MASK_UINT);

  if (mask)
  {
    mask->imin = min;
    mask->imax = max;
    mask->type = 'I';
  }

  return mask;
}

IUP_SDK_API Imask* iupMaskCreateReal(int positive, const char* decimal_symbol)
{
  Imask* mask;
  int use_comma = 0;

  if (decimal_symbol)
  {
    if (decimal_symbol[0] == ',')
      use_comma = 1;
  }
  else
  {
#ifndef __ANDROID__
    struct lconv* locale_info = localeconv();
    if (locale_info->decimal_point[0] == ',')
      use_comma = 1;
#endif
  }

  if (use_comma)
  {
    if (positive)
      mask = iupMaskCreate(IUP_MASK_UFLOATCOMMA);
    else
      mask = iupMaskCreate(IUP_MASK_FLOATCOMMA);
  }
  else
  {
    if (positive)
      mask = iupMaskCreate(IUP_MASK_UFLOAT);
    else
      mask = iupMaskCreate(IUP_MASK_FLOAT);
  }

  return mask;
}

IUP_SDK_API Imask* iupMaskCreateFloat(float min, float max, const char* decimal_symbol)
{
  Imask* mask = iupMaskCreateReal(min >= 0, decimal_symbol);

  if (mask)
  {
    mask->fmin = min;
    mask->fmax = max;
    mask->type = 'F';
  }

  return mask;
}

IUP_SDK_API void iupMaskDestroy(Imask* mask)
{
  free(mask->mask_str); 
  free(mask->fsm); 
  free(mask); 
}

IUP_SDK_API char* iupMaskGetStr(Imask* mask)
{
  return mask->mask_str;
}

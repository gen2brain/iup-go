/** \file
 * \brief EFL String Utilities
 *
 * UTF-8 string handling for EFL (EFL uses UTF-8 internally).
 *
 * See Copyright Notice in "iup.h"
 */

#include "iupefl_drv.h"


char* iupeflStrConvertToSystem(const char* str)
{
  return (char*)str;
}

char* iupeflStrConvertFromSystem(const char* str)
{
  return (char*)str;
}

char* iupeflStrConvertFromFilename(const char* str)
{
  return (char*)str;
}

char* iupeflStrConvertToFilename(const char* str)
{
  return (char*)str;
}

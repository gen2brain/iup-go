/** \file
 * \brief String Utilities
 *
 * See Copyright Notice in "iup.h"
 */

 
#include <string.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <limits.h>  
#include <stdarg.h>

#include "iup.h"

#include "iup_assert.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_predialogs.h"


IUP_SDK_API void iupError(const char* format, ...)
{
  int size;
  char* msg = iupStrGetLargeMem(&size);
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(msg, size, format, arglist);
  va_end(arglist);
#if IUP_ASSERT_CONSOLE 
  fprintf(stderr, "%s", msg);
#else
  if (IupIsOpened())
    IupMessageError(NULL, msg);
  else
    fprintf(stderr, "%s", msg);
#endif
  /* set the breakpoint here, just after the assert dialog */
  size = 0;
}

IUP_SDK_API void iupAssert(const char* expr, const char* file, int line, const char* func)
{
  if (func)
    iupError("File: %s\n"
             "Line: %d\n"
             "Function: %s\n"
             "Assertive: (%s)", 
             file, line, func, expr);
  else
    iupError("File: %s\n"
             "Line: %d\n"
             "Assertive: (%s)", 
             file, line, expr);
}

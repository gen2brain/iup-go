/** \file
 * \brief String Utilities
 *
 * See Copyright Notice in "iup.h"
 *
 * This file is in UTF-8 encoding with BOM, so he comments look right.
 *
 */

 
#include <string.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <limits.h>
#include <stdarg.h>
#include <locale.h>
#include <ctype.h>

#include "iup_export.h"
#include "iup_str.h"


/* Line breaks can be:
\n - UNIX
\r - Mac
\r\n - DOS/Windows
*/


#define IUP_STR_EQUAL(str1, str2)      \
{                                      \
  if (str1 == str2)                    \
    return 1;                          \
                                       \
  if (!str1 || !str2)                  \
    return 0;                          \
                                       \
  while(*str1 && *str2 &&              \
        SF(*str1) == SF(*str2))        \
  {                                    \
    EXTRAINC(str1);                    \
    EXTRAINC(str2);                    \
    str1++;                            \
    str2++;                            \
  }                                    \
                                       \
  /* check also for terminator */      \
  if (*str1 == *str2) return 1;        \
}

IUP_SDK_API int iupStrEqual(const char* str1, const char* str2)
{
#define EXTRAINC(_x) (void)(_x)
#define SF(_x) (_x)
  IUP_STR_EQUAL(str1, str2);
#undef SF
#undef EXTRAINC
  return 0;
}

IUP_SDK_API int iupStrEqualPartial(const char* str1, const char* str2)
{
#define EXTRAINC(_x) (void)(_x)
#define SF(_x) (_x)
  IUP_STR_EQUAL(str1, str2);
#undef SF
#undef EXTRAINC
  if (*str2 == 0) 
    return 1;  /* if second string is at terminator, then it is partially equal */
  return 0;
}

IUP_SDK_API int iupStrEqualNoCase(const char* str1, const char* str2)
{
#define EXTRAINC(_x) (void)(_x)
#define SF(_x) iup_tolower(_x)
  IUP_STR_EQUAL(str1, str2);
#undef SF
#undef EXTRAINC
  return 0;
}

IUP_SDK_API int iupStrEqualNoCasePartial(const char* str1, const char* str2)
{
#define EXTRAINC(_x) (void)(_x)
#define SF(_x) iup_tolower(_x)
  IUP_STR_EQUAL(str1, str2);
#undef SF
#undef EXTRAINC
  if (*str2 == 0) 
    return 1;  /* if second string is at terminator, then it is partially equal */
  return 0;
}

IUP_SDK_API int iupStrEqualNoCaseNoSpace(const char* str1, const char* str2)
{
#define EXTRAINC(_x) { if (*_x == ' ') _x++; }  /* also ignore spaces */
#define SF(_x) iup_tolower(_x)
  IUP_STR_EQUAL(str1, str2);
#undef SF
#undef EXTRAINC
  return 0;
}

IUP_SDK_API int iupStrFalse(const char* str)
{
  if (!str || str[0]==0) return 0;
  if (str[0]=='0' && str[1]==0) return 1;
  if (iupStrEqualNoCase(str, "NO")) return 1;
  if (iupStrEqualNoCase(str, "OFF")) return 1;
  return 0;
}

IUP_SDK_API int iupStrBoolean(const char* str)
{
  if (!str || str[0]==0) return 0;
  if (str[0]=='1' && str[1]==0) return 1;
  if (iupStrEqualNoCase(str, "YES")) return 1;
  if (iupStrEqualNoCase(str, "ON")) return 1;
  return 0;
}

IUP_SDK_API void iupStrUpper(char* dstr, const char* sstr)
{
  if (!sstr || sstr[0] == 0) return;
  for (; *sstr; sstr++, dstr++)
    *dstr = (char)iup_toupper(*sstr);
  *dstr = 0;
}

IUP_SDK_API void iupStrLower(char* dstr, const char* sstr)
{
  if (!sstr || sstr[0] == 0) return;
  for (; *sstr; sstr++, dstr++)
    *dstr = (char)iup_tolower(*sstr);
  *dstr = 0;
}

IUP_SDK_API int iupStrHasSpace(const char* str)
{
  if (!str) return 0;
  while (*str)
  {
    if (*str == ' ')
      return 1;
    str++;
  }
  return 0;
}

IUP_SDK_API char *iupStrDup(const char *str)
{
  if (str)
  {
    int size = (int)strlen(str)+1;
    char *newstr = malloc(size);
    if (newstr) memcpy(newstr, str, size);
    return newstr;
  }
  return NULL;
}

IUP_SDK_API const char* iupStrNextLine(const char* str, int *len)
{
  *len = 0;

  if (!str) return NULL;

  while(*str!=0 && *str!='\n' && *str!='\r') 
  {
    (*len)++;
    str++;
  }

  if (*str=='\r' && *(str+1)=='\n')   /* DOS line end */
    return str+2;
  else if (*str=='\n' || *str=='\r')   /* UNIX or MAC line end */
    return str+1;
  else 
    return str;  /* no next line */
}

IUP_SDK_API const char* iupStrNextValue(const char* str, int str_len, int *len, char sep)
{
  int ignore_sep = 0;

  *len = 0;

  if (!str) return NULL;

  while (*str != 0 && (*str != sep || ignore_sep) && *len<str_len)
  {
    if (*str == '\"')
    {
      if (ignore_sep)
        ignore_sep = 0;
      else
        ignore_sep = 1;
    }

    (*len)++;
    str++;
  }

  if (*str==sep)
    return str+1;
  else 
    return str;  /* no next value */
}

IUP_SDK_API int iupStrLineCount(const char* str, int len)
{
  int line_count = 1, count = 0;

  if (!str)
    return line_count;

  while(*str != 0)
  {
    while (*str != 0 && *str != '\n' && *str != '\r')
    {
      str++;
      count++;
    }

    if (count >= len)
      return line_count;

    if (*str=='\r' && *(str+1)=='\n')   /* DOS line end */
    {
      line_count++;
      str += 2;
      count += 2;
    }
    else if (*str=='\n' || *str=='\r')   /* UNIX or MAC line end */
    {
      line_count++;
      str++;
      count++;
    }

    if (count >= len)
      return line_count;
  }

  return line_count;
}

IUP_SDK_API int iupStrCountChar(const char *str, char c)
{
  int n;
  if (!str) return 0;
  for (n=0; *str; str++)
  {
    if (*str==c)
      n++;
  }
  return n;
}

IUP_SDK_API void iupStrCopyN(char* dst_str, int dst_max_size, const char* src_str)
{
  if (src_str)
  {
    int size = (int)strlen(src_str) + 1;
    if (size > dst_max_size) size = dst_max_size;
    memcpy(dst_str, src_str, size - 1);
    dst_str[size - 1] = 0;
  }
}

IUP_SDK_API char* iupStrDupUntil(const char **str, char c)
{
  const char *p_str;
  if (!str || str[0]==0)
    return NULL;

  p_str = strchr(*str,c);
  if (!p_str) 
    return NULL;
  else
  {
    char *new_str;
    int i;
    int sl = (int)(p_str - (*str));

    new_str = (char *)malloc(sl + 1);
    if (!new_str) return NULL;

    for (i = 0; i < sl; ++i)
      new_str[i] = (*str)[i];

    new_str[sl] = 0;

    *str = p_str+1;
    return new_str;
  }
}

static char *iStrDupUntilNoCase(char **str, char sep)
{
  char *p_str;
  if (!str || str[0]==0)
    return NULL;

  p_str=strchr(*str,sep); /* usually the lower case is enough */
  if (!p_str && (iup_toupper(sep)!=sep)) 
    p_str=strchr(*str, iup_toupper(sep));  /* but check also for upper case */

  /* if both fail, then abort */
  if (!p_str) 
    return NULL;
  else
  {
    char *new_str;
    int i;
    int sl=(int)(p_str - (*str));

    new_str = (char *) malloc (sl + 1);
    if (!new_str) return NULL;

    for (i = 0; i < sl; ++i)
      new_str[i] = (*str)[i];

    new_str[sl] = 0;

    *str = p_str+1;
    return new_str;
  }
}

IUP_SDK_API char *iupStrGetLargeMem(int *size)
{
#define LARGE_MAX_BUFFERS 10
#define LARGE_SIZE SHRT_MAX
  static char buffers[LARGE_MAX_BUFFERS][LARGE_SIZE];
  static int buffers_index = -1;
  char* ret_str;

  /* init buffers array */
  if (buffers_index == -1)
  {
    int i;
    /* clear all memory */
    for (i=0; i<LARGE_MAX_BUFFERS; i++)
      memset(buffers[i], 0, sizeof(char)*LARGE_SIZE);

    buffers_index = 0;
  }

  /* DON'T clear memory everytime because the buffer is too large */
  ret_str = buffers[buffers_index];
  ret_str[0] = 0;

  buffers_index++;
  if (buffers_index == LARGE_MAX_BUFFERS)
    buffers_index = 0;

  if (size) *size = LARGE_SIZE;
  return ret_str;
#undef LARGE_MAX_BUFFERS
#undef LARGE_SIZE 
}

static char* iupStrGetSmallMem(void)
{
#define SMALL_MAX_BUFFERS 100
#define SMALL_SIZE 80  /* maximum for iupStrReturnFloat and iupStrReturnDouble */
  static char buffers[SMALL_MAX_BUFFERS][SMALL_SIZE];
  static int buffers_index = -1;
  char* ret_str;

  /* init buffers array */
  if (buffers_index == -1)
  {
    int i;
    /* clear all memory */
    for (i = 0; i<SMALL_MAX_BUFFERS; i++)
      memset(buffers[i], 0, sizeof(char)*SMALL_SIZE);
    
    buffers_index = 0;
  }

  /* always clear memory before returning a new buffer */
  memset(buffers[buffers_index], 0, SMALL_SIZE);
  ret_str = buffers[buffers_index];

  buffers_index++;
  if (buffers_index == SMALL_MAX_BUFFERS)
    buffers_index = 0;

  return ret_str;
#undef SMALL_MAX_BUFFERS
#undef SMALL_SIZE 
}

IUP_SDK_API char *iupStrGetMemory(int size)
{
#define MAX_BUFFERS 50
  static char* buffers[MAX_BUFFERS];
  static int buffers_sizes[MAX_BUFFERS];
  static int buffers_index = -1;

  if (size == -1) /* Frees memory */
  {
    int i;
    buffers_index = -1;
    for (i = 0; i < MAX_BUFFERS; i++)
    {
      if (buffers[i]) 
      {
        free(buffers[i]);
        buffers[i] = NULL;
      }
      buffers_sizes[i] = 0;
    }
    return NULL;
  }
  else
  {
    char* ret_str;

    /* init buffers array */
    if (buffers_index == -1)
    {
      memset(buffers, 0, sizeof(char*)*MAX_BUFFERS);
      memset(buffers_sizes, 0, sizeof(int)*MAX_BUFFERS);
      buffers_index = 0;
    }

    /* first alocation */
    if (!(buffers[buffers_index]))
    {
      buffers_sizes[buffers_index] = size+1;
      buffers[buffers_index] = (char*)malloc(buffers_sizes[buffers_index]);
    }
    else if (buffers_sizes[buffers_index] < size+1)  /* reallocate if necessary */
    {
      buffers_sizes[buffers_index] = size+1;
      buffers[buffers_index] = (char*)realloc(buffers[buffers_index], buffers_sizes[buffers_index]);
    }

    /* always clear memory before returning a new buffer */
    memset(buffers[buffers_index], 0, buffers_sizes[buffers_index]);
    ret_str = buffers[buffers_index];

    buffers_index++;
    if (buffers_index == MAX_BUFFERS)
      buffers_index = 0;

    return ret_str;
  }
#undef MAX_BUFFERS
}

IUP_SDK_API char* iupStrReturnStrf(const char* format, ...)
{
  char* str = iupStrGetMemory(10240);
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(str, 10240, format, arglist);
  va_end(arglist);
  return str;
}

IUP_SDK_API char* iupStrReturnStr(const char* str)
{
  if (str)
  {
    int size = (int)strlen(str)+1;
    char* ret_str = iupStrGetMemory(size);
    memcpy(ret_str, str, size);
    return ret_str;
  }
  else
    return NULL;
}

IUP_SDK_API char* iupStrReturnBoolean(int b)
{
  if (b)
    return "YES";
  else
    return "NO";
}

IUP_SDK_API char* iupStrReturnChecked(int check)
{
  if (check == -1)
    return "NOTDEF";
  else if (check)
    return "ON";
  else
    return "OFF";
}

IUP_SDK_API char* iupStrReturnUInt(unsigned int i)
{
  char* str = iupStrGetSmallMem();  /* 20 */
  sprintf(str, "%u", i);
  return str;
}

IUP_SDK_API char* iupStrReturnInt(int i)
{
  char* str = iupStrGetSmallMem();  /* 20 */
  sprintf(str, "%d", i);
  return str;
}

IUP_SDK_API char* iupStrReturnFloat(float f)
{
  char* str = iupStrGetSmallMem();  /* 80 */
  sprintf(str, IUP_FLOAT2STR, f);
  return str;
}

IUP_SDK_API char* iupStrReturnDouble(double d)
{
  char* str = iupStrGetSmallMem();  /* 80 */
  sprintf(str, IUP_DOUBLE2STR, d);
  return str;
}

IUP_SDK_API char* iupStrReturnRGB(unsigned char r, unsigned char g, unsigned char b)
{
  char* str = iupStrGetSmallMem();  /* 3*20 */
  sprintf(str, "%d %d %d", (int)r, (int)g, (int)b);
  return str;
}

IUP_SDK_API char* iupStrReturnRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  char* str = iupStrGetSmallMem();  /* 4*20 */
  sprintf(str, "%d %d %d %d", (int)r, (int)g, (int)b, (int)a);
  return str;
}

IUP_SDK_API char* iupStrReturnStrStr(const char *str1, const char *str2, char sep)
{
  if (str1 || str2)
  {
    char* ret_str;
    int size1=0, size2=0;
    if (str1) size1 = (int)strlen(str1);
    if (str2) size2 = (int)strlen(str2);
    ret_str = iupStrGetMemory(size1+size2+2);
    if (str1 && size1) memcpy(ret_str, str1, size1);
    ret_str[size1] = sep;
    if (str2 && size2) memcpy(ret_str+size1+1, str2, size2);
    ret_str[size1+1+size2] = 0;
    return ret_str;
  }
  else
    return NULL;
}

IUP_SDK_API char* iupStrReturnIntInt(int i1, int i2, char sep)
{
  char* str = iupStrGetSmallMem();  /* 2*20 */
  sprintf(str, "%d%c%d", i1, sep, i2);
  return str;
}

IUP_SDK_API int iupStrGetFormatPrecision(const char* format)
{
  int precision;
  while (*format)
  {
    if (*format == '.')
      break;
    format++;
  }

  if (*format != '.')
    return -1;

  format++;
  if (iupStrToInt(format, &precision))
    return precision;

  return -1;
}

IUP_SDK_API int iupStrToRGB(const char *str, unsigned char *r, unsigned char *g, unsigned char *b)
{
  unsigned int ri = 0, gi = 0, bi = 0;
  if (!str) return 0;
  if (str[0]=='#')
  {
    str++;
    if (sscanf(str, "%2X%2X%2X", &ri, &gi, &bi) != 3) return 0;
  }
  else
  {
    if (sscanf(str, "%u %u %u", &ri, &gi, &bi) != 3) return 0;
  }
  if (ri > 255 || gi > 255 || bi > 255) return 0;
  *r = (unsigned char)ri;
  *g = (unsigned char)gi;
  *b = (unsigned char)bi;
  return 1;
}

IUP_SDK_API int iupStrToRGBA(const char *str, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
  unsigned int ri = 0, gi = 0, bi = 0, ai = 255;
  if (!str) return 0;
  if (str[0] == '#')
  {
    str++;
    if (sscanf(str, "%2X%2X%2X%2X", &ri, &gi, &bi, &ai) < 3) return 0;
  }
  else
  {
    if (sscanf(str, "%u %u %u %u", &ri, &gi, &bi, &ai) < 3) return 0;
  }
  if (ri > 255 || gi > 255 || bi > 255 || ai > 255) return 0;
  *r = (unsigned char)ri;
  *g = (unsigned char)gi;
  *b = (unsigned char)bi;
  *a = (unsigned char)ai;
  return 1;
}

/* TODO: are strtod/atof and strtol/atoi faster/better than sscanf? 
         must handle the 0 return value. */

IUP_SDK_API int iupStrToInt(const char *str, int *i)
{
  if (!str) return 0;
  if (sscanf(str, "%d", i) != 1) return 0;
  return 1;
}

IUP_SDK_API int iupStrToUInt(const char *str, unsigned int *i)
{
  if (!str) return 0;
  if (sscanf(str, "%u", i) != 1) return 0;
  return 1;
}

IUP_SDK_API int iupStrToIntInt(const char *str, int *i1, int *i2, char sep)
{
  if (!str) return 0;
                         
  if (iup_tolower(*str) == sep) /* no first value */
  {
    str++; /* skip separator */
    if (sscanf(str, "%d", i2) != 1) return 0;
    return 1;
  }
  else 
  {
    char* p_str = iStrDupUntilNoCase((char**)&str, sep);
    
    if (!p_str)   /* no separator means no second value */
    {        
      if (sscanf(str, "%d", i1) != 1) return 0;
      return 1;
    }
    else if (*str==0)  /* separator exists, but second value empty, also means no second value */
    {        
      int ret = sscanf(p_str, "%d", i1);
      free(p_str);
      if (ret != 1) return 0;
      return 1;
    }
    else
    {
      int ret = 0;
      if (sscanf(p_str, "%d", i1) == 1) ret++;
      if (sscanf(str, "%d", i2) == 1) ret++;
      free(p_str);
      return ret;
    }
  }
}

IUP_SDK_API int iupStrToFloatDef(const char *str, float *f, float def)
{
  if (!str) { *f = def;  return 1; }
  if (sscanf(str, "%f", f) != 1) return 0;
  return 1;
}

IUP_SDK_API int iupStrToFloat(const char *str, float *f)
{
  if (!str) return 0;
  if (sscanf(str, "%f", f) != 1) return 0;
  return 1;
}

IUP_SDK_API int iupStrToDouble(const char *str, double *d)
{
  if (!str) return 0;
  if (sscanf(str, "%lf", d) != 1) return 0;
  return 1;
}

IUP_SDK_API int iupStrToDoubleDef(const char *str, double *d, double def)
{
  if (!str) { *d = def;  return 1; }
  if (sscanf(str, "%lf", d) != 1) return 0;
  return 1;
}

IUP_SDK_API int iupStrToFloatFloat(const char *str, float *f1, float *f2, char sep)
{
  if (!str) return 0;

  if (iup_tolower(*str) == sep) /* no first value */
  {
    str++; /* skip separator */
    if (sscanf(str, "%f", f2) != 1) return 0;
    return 1;
  }
  else 
  {
    char* p_str = iStrDupUntilNoCase((char**)&str, sep);
    
    if (!p_str)   /* no separator means no second value */
    {        
      if (sscanf(str, "%f", f1) != 1) return 0;
      return 1;
    }
    else if (*str==0)    /* separator exists, but second value empty, also means no second value */
    {        
      int ret = sscanf(p_str, "%f", f1);
      free(p_str);
      if (ret != 1) return 0;
      return 1;
    }
    else
    {
      int ret = 0;
      if (sscanf(p_str, "%f", f1) == 1) ret++;
      if (sscanf(str, "%f", f2) == 1) ret++;
      free(p_str);
      return ret;
    }
  }
}

IUP_SDK_API int iupStrToDoubleDouble(const char *str, double *f1, double *f2, char sep)
{
  if (!str) return 0;

  if (iup_tolower(*str) == sep) /* no first value */
  {
    str++; /* skip separator */
    if (sscanf(str, "%lf", f2) != 1) return 0;
    return 1;
  }
  else
  {
    char* p_str = iStrDupUntilNoCase((char**)&str, sep);

    if (!p_str)   /* no separator means no second value */
    {
      if (sscanf(str, "%lf", f1) != 1) return 0;
      return 1;
    }
    else if (*str == 0)    /* separator exists, but second value empty, also means no second value */
    {
      int ret = sscanf(p_str, "%lf", f1);
      free(p_str);
      if (ret != 1) return 0;
      return 1;
    }
    else
    {
      int ret = 0;
      if (sscanf(p_str, "%lf", f1) == 1) ret++;
      if (sscanf(str, "%lf", f2) == 1) ret++;
      free(p_str);
      return ret;
    }
  }
}

IUP_SDK_API int iupStrToStrStr(const char *str, char *str1, char *str2, char sep)
{
  str1[0] = 0;
  str2[0] = 0;

  if (!str)
    return 0;

  if (iup_tolower(*str) == sep) /* starts with separator, no first value */
  {
    str++; /* skip separator */
    strcpy(str2, str);
    return 1;
  }
  else 
  {
    char* p_str = iStrDupUntilNoCase((char**)&str, sep);
    
    if (!p_str)   /* no separator means no second value */
    {        
      strcpy(str1, str);
      return 1;
    }
    else if (*str==0)    /* separator exists, but no second value */
    {        
      strcpy(str1, p_str);
      free(p_str);
      return 1;
    }
    else
    {
      strcpy(str1, p_str);
      strcpy(str2, str);
      free(p_str);
      return 2;
    }
  }
}

IUP_SDK_API char* iupStrFileGetPath(const char *filename)
{
  if (!filename)
    return NULL;
  else
  {
    /* Starts at the last character */
    int len = (int)strlen(filename) - 1;
    while (len != 0)
    {
      if (filename[len] == '\\' || filename[len] == '/')
      {
        len++;
        break;
      }

      len--;
    }
    if (len == 0)
      return NULL;

    {
      char* path = malloc(len + 1);
      memcpy(path, filename, len);
      path[len] = 0;

      return path;
    }
  }
}

IUP_SDK_API char* iupStrFileGetTitle(const char *filename)
{
  if (!filename)
    return NULL;
  else
  {
    /* Starts at the last character */
    int len = (int)strlen(filename);
    int offset = len - 1;
    while (offset != 0)
    {
      if (filename[offset] == '\\' || filename[offset] == '/')
      {
        offset++;
        break;
      }

      offset--;
    }

    {
      int title_size = len - offset + 1;
      char* file_title = malloc(title_size);
      memcpy(file_title, filename + offset, title_size);
      return file_title;
    }
  }
}

IUP_SDK_API char* iupStrFileGetExt(const char *filename)
{
  if (!filename)
    return NULL;
  else
  {
    /* Starts at the last character */
    int len = (int)strlen(filename);
    int offset = len - 1;
    while (offset != 0)
    {
      /* if found a path separator stop. */
      if (filename[offset] == '\\' || filename[offset] == '/')
        return NULL;

      if (filename[offset] == '.')
      {
        offset++;
        break;
      }

      offset--;
    }

    if (offset == 0)
      return NULL;

    {
      int ext_size = len - offset + 1;
      char* file_ext = (char*)malloc(ext_size);
      memcpy(file_ext, filename + offset, ext_size);
      return file_ext;
    }
  }
}

IUP_SDK_API char* iupStrFileMakeFileName(const char* path, const char* title)
{
  if (!path || !title)
    return NULL;
  else
  {
    int size_path = (int)strlen(path);
    int size_title = (int)strlen(title);
    char *filename = malloc(size_path + size_title + 2);
    memcpy(filename, path, size_path);

    if (path[size_path - 1] != '/' && path[size_path - 1] != '\\')
    {
      filename[size_path] = '/';
      size_path++;
    }

    memcpy(filename + size_path, title, size_title);
    filename[size_path + size_title] = 0;

    return filename;
  }
}

IUP_SDK_API void iupStrFileNameSplit(const char* filename, char *path, char *title)
{
  int i, n;

  if (!filename)
    return;

  /* Look for last folder separator and split title from path */
  n = (int)strlen(filename);
  for (i = n - 1; i >= 0; i--)
  {
    if (filename[i] == '\\' || filename[i] == '/') 
    {
      if (path)
      {
        strncpy(path, filename, i+1);
        path[i+1] = 0;
      }

      if (title)
      {
        strcpy(title, filename+i+1);
        title[n-i] = 0;
      }

      return;
    }
  }
}

IUP_SDK_API int iupStrReplace(char* str, char src, char dst)
{
  int i = 0;

  if (!str)
    return 0;

  while (*str)
  {
    if (*str == src)
    {
      *str = dst;
      i++;
    }
    str++;
  }
  return i;
}

static int iStrIsReserved(char c)
{
  /* can only has letters or numbers as characters, or underscore */
  if (c < '0' ||
    (c > '9' && c < 'A') ||
      (c > 'Z' && c < 'a' && c != '_') ||
      c > 'z')
    return 1;

  return 0;
}

IUP_SDK_API void iupStrReplaceReserved(char* str, char c)
{
  if (!str)
    return;

  while (*str)
  {
    if (iStrIsReserved(*str))
    {
      *str = c;
    }
    str++;
  }
}

IUP_SDK_API void iupStrToUnix(char* str)
{
  char* p_str = str;

  if (!str) return;
  
  while (*str)
  {
    if (*str == '\r')
    {
      if (*(str+1) != '\n')  /* MAC line end */
        *p_str++ = '\n';
      str++;
    }
    else
      *p_str++ = *str++;
  }
  
  *p_str = *str;
}

IUP_SDK_API void iupStrToMac(char* str)
{
  char* p_str = str;

  if (!str) return;
  
  while (*str)
  {
    if (*str == '\r')
    {
      if (*(++str) == '\n')  /* DOS line end */
        str++;
      *p_str++ = '\r';
    }
    else if (*str == '\n')  /* UNIX line end */
    {
      str++;
      *p_str++ = '\r';
    }
    else
      *p_str++ = *str++;
  }
  
  *p_str = *str;
}

IUP_SDK_API char* iupStrToDos(const char* str)
{
	char *auxstr, *newstr;
	int line_count, len;

  if (!str) return NULL;

  len = (int)strlen(str);
  line_count = iupStrLineCount(str, len);
  if (line_count == 1)
    return (char*)str;

	newstr = malloc(line_count + len + 1);
  auxstr = newstr;
	while(*str)
	{
		if (*str == '\r' && *(str+1)=='\n')  /* DOS line end */
    {
      *auxstr++ = *str++;
      *auxstr++ = *str++;
    }
    else if (*str == '\r')   /* MAC line end */
    {
		  *auxstr++ = *str++;
			*auxstr++ = '\n';
    }
    else if (*str == '\n')  /* UNIX line end */
    {
			*auxstr++ = '\r';
		  *auxstr++ = *str++;
    }
    else
		  *auxstr++ = *str++;
	}
	*auxstr = 0;

	return newstr;	
}

#define IUP_ISRESERVED(_c) (_c=='\n' || _c=='\r' || _c=='\t' || _c=='\\')

IUP_SDK_API char* iupStrConvertToC(const char* str)
{
  char* new_str, *p_new_str;
  const char* p_str = str;
  int len, count=0;

  if (!str)
    return NULL;

  while(*p_str)
  {
    if (IUP_ISRESERVED(*p_str))
      count++;
    p_str++;
  }
  if (!count)
    return (char*)str;

  len = (int)(p_str-str);
  new_str = malloc(len+count+1);
  p_str = str;
  p_new_str = new_str;
  while(*p_str)
  {
    if (IUP_ISRESERVED(*p_str))
    {
      *p_new_str = '\\';
      p_new_str++;

      switch(*p_str)
      {
      case '\n':
        *p_new_str = 'n';
        break;
      case '\r':
        *p_new_str = 'r';
        break;
      case '\t':
        *p_new_str = 't';
        break;
      case '\\':
        *p_new_str = '\\';
        break;
      }
    }
    else
      *p_new_str = *p_str;

    p_new_str++;
    p_str++;
  }
  *p_new_str = 0;
  return new_str;
}

IUP_SDK_API char* iupStrProcessMnemonic(const char* str, char *c, int action)
{
  int i = 0, found = 0;
  char* new_str, *orig_str = (char*)str;

  if (!str) 
    return NULL;

  if (!strchr(str, '&'))
    return (char*)str;

  new_str = malloc(strlen(str)+1);
  while (*str)
  {
    if (*str == '&')
    {
      if (*(str+1) == '&') /* remove & from the string, add next & to the string */
      {
        found = -1;

        str++;
        new_str[i++] = *str;
      }
      else if (found!=1) /* mnemonic found */
      {
        found = 1;

        if (action == 1) /* replace & by c */
          new_str[i++] = *c;
        else if (action == -1)  /* remove & and return in c */
          *c = *(str+1);  /* next is mnemonic */
        /* else -- only remove & */
      }
    }
    else
    {
      new_str[i++] = *str;
    }

    str++;
  }
  new_str[i] = 0;

  if (found==0)
  {
    free(new_str);
    return orig_str;
  }

  return new_str;
}

IUP_SDK_API int iupStrFindMnemonic(const char* str)
{
  int c = 0, found = 0;

  if (!str) 
    return 0;

  if (!strchr(str, '&'))
    return 0;

  while (*str)
  {
    if (*str == '&')
    {
      if (*(str+1) == '&')
      {
        found = -1;
        str++;
      }
      else if (found!=1) /* mnemonic found */
      {
        found = 1;
        c = *(str+1);  /* next is mnemonic */
      }
    }

    str++;
  }

  if (found==0)
    return 0;
  else
    return c;
}

static unsigned char* Latin1_map = NULL;
static unsigned char* Latin1_map_nocase = NULL;

static void iStrInitLatin1_map(void)
{
  static unsigned char map[256];
  static unsigned char map_nocase[256];

  Latin1_map = map;
  Latin1_map_nocase = map_nocase;

memset(map, 0, 256);

#define mm(_x) (map[(unsigned char)_x])

  /* these characters are sorted in the same order as Excel would sort them */

  mm(  0)=  0;  /* */ mm(  1)=  1; /* */ mm(  2)=  2; /* */ mm(  3)=  3; /* */ mm(  4)=  4; /* */ mm(  5)=  5; /* */ mm(  6)=  6; /* */ mm(  7)=  7;  /* */ mm(  8)=  8; /* */ mm(  9)=  9; /* */ mm( 10)= 10; /* */ mm( 11)= 11; /* */ mm( 12)= 12; /* */ mm( 13)= 13; /* */ mm( 14)= 14; /* */ mm( 15)= 15; /* */
  mm( 16)= 16;  /* */ mm( 17)= 17; /* */ mm( 18)= 18; /* */ mm( 19)= 19; /* */ mm( 20)= 20; /* */ mm( 21)= 21; /* */ mm( 22)= 22; /* */ mm( 23)= 23;  /* */ mm( 24)= 24; /* */ mm( 25)= 25; /* */ mm( 26)= 26; /* */ mm( 27)= 27; /* */ mm( 28)= 28; /* */ mm( 29)= 29; /* */ mm( 30)= 30; /* */ mm( 31)= 31; /* */
  mm(39)= 32;   /*\*/ mm(45)= 33;  /*-*/ mm(150)= 34; /*–*/ mm(151)= 35; /*—*/ mm(32)= 36;  /* */ mm(33)= 37;  /*!*/ mm(34)= 38;  /*"*/ mm(35)= 39;   /*#*/ mm(36)= 40;  /*$*/ mm(37)= 41;  /*%*/ mm(38)= 42;  /*&*/ mm(40)= 43;  /*(*/ mm(41)= 44;  /*)*/ mm(42)= 45;  /***/ mm(44)= 46;  /*,*/ mm(46)= 47;  /*.*/
  mm(47)= 48;   /*/*/ mm(58)= 49;  /*:*/ mm(59)= 50;  /*;*/ mm(63)= 51;  /*?*/ mm(64)= 52;  /*@*/ mm(91)= 53;  /*[*/ mm(93)= 54;  /*]*/ mm(94)= 55;   /*^*/ mm(136)= 56; /*ˆ*/ mm(95)= 57;  /*_*/ mm(96)= 58;  /*`*/ mm(123)= 59; /*{*/ mm(124)= 60; /*|*/ mm(125)= 61; /*}*/ mm(126)= 62; /*~*/ mm(161)= 63; /*¡*/
  mm(166)= 64;  /*¦*/ mm(168)= 65; /*¨*/ mm(175)= 66; /*¯*/ mm(180)= 67; /*´*/ mm(184)= 68; /*¸*/ mm(191)= 69; /*¿*/ mm(152)= 70; /*˜*/ mm(145)= 71;  /*‘*/ mm(146)= 72; /*’*/ mm(130)= 73; /*‚*/ mm(147)= 74; /*“*/ mm(148)= 75; /*”*/ mm(132)= 76; /*„*/ mm(139)= 77; /*‹*/ mm(155)= 78; /*›*/ mm(162)= 79; /*¢*/
  mm(163)= 80;  /*£*/ mm(164)= 81; /*¤*/ mm(165)= 82; /*¥*/ mm(128)= 83; /*€*/ mm(43)= 84;  /*+*/ mm(60)= 85;  /*<*/ mm(61)= 86;  /*=*/ mm(62)= 87;   /*>*/ mm(177)= 88; /*±*/ mm(171)= 89; /*«*/ mm(187)= 90; /*»*/ mm(215)= 91; /*×*/ mm(247)= 92; /*÷*/ mm(167)= 93; /*§*/ mm(169)= 94; /*©*/ mm(172)= 95; /*¬*/
  mm(174)= 96;  /*®*/ mm(176)= 97; /*°*/ mm(181)= 98; /*µ*/ mm(182)= 99; /*¶*/ mm(133)=100; /*…*/ mm(134)=101; /*†*/ mm(135)=102; /*‡*/ mm(183)=103;  /*·*/ mm(149)=104; /*•*/ mm(137)=105; /*‰*/ mm(48)=106;  /*0*/ mm(188)=107; /*¼*/ mm(189)=108; /*½*/ mm(190)=109; /*¾*/ mm(49)=110;  /*1*/ mm(185)=111; /*¹*/
  mm(50)=112;   /*2*/ mm(178)=113; /*²*/ mm(51)=114;  /*3*/ mm(179)=115; /*³*/ mm(52)=116;  /*4*/ mm(53)=117;  /*5*/ mm(54)=118;  /*6*/ mm(55)=119;   /*7*/ mm(56)=120;  /*8*/ mm(57)=121;  /*9*/ mm(97)=122;  /*a*/ mm(65)=123;  /*A*/ mm(170)=124; /*ª*/ mm(225)=125; /*á*/ mm(193)=126; /*Á*/ mm(224)=127; /*à*/
  mm(192)=128;  /*À*/ mm(226)=129; /*â*/ mm(194)=130; /*Â*/ mm(228)=131; /*ä*/ mm(196)=132; /*Ä*/ mm(227)=133; /*ã*/ mm(195)=134; /*Ã*/ mm(229)=135;  /*å*/ mm(197)=136; /*Å*/ mm(230)=137; /*æ*/ mm(198)=138; /*Æ*/ mm(98)=139;  /*b*/ mm(66)=140;  /*B*/ mm(99)=141;  /*c*/ mm(67)=142;  /*C*/ mm(231)=143; /*ç*/
  mm(199)=144;  /*Ç*/ mm(100)=145; /*d*/ mm(68)=146;  /*D*/ mm(240)=147; /*ð*/ mm(208)=148; /*Ð*/ mm(101)=149; /*e*/ mm(69)=150;  /*E*/ mm(233)=151;  /*é*/ mm(201)=152; /*É*/ mm(232)=153; /*è*/ mm(200)=154; /*È*/ mm(234)=155; /*ê*/ mm(202)=156; /*Ê*/ mm(235)=157; /*ë*/ mm(203)=158; /*Ë*/ mm(102)=159; /*f*/
  mm(70)=160;   /*F*/ mm(131)=161; /*ƒ*/ mm(103)=162; /*g*/ mm(71)=163;  /*G*/ mm(104)=164; /*h*/ mm(72)=165;  /*H*/ mm(105)=166; /*i*/ mm(73)=167;   /*I*/ mm(237)=168; /*í*/ mm(205)=169; /*Í*/ mm(236)=170; /*ì*/ mm(204)=171; /*Ì*/ mm(238)=172; /*î*/ mm(206)=173; /*Î*/ mm(239)=174; /*ï*/ mm(207)=175; /*Ï*/
  mm(106)=176;  /*j*/ mm(74)=177;  /*J*/ mm(107)=178; /*k*/ mm(75)=179;  /*K*/ mm(108)=180; /*l*/ mm(76)=181;  /*L*/ mm(109)=182; /*m*/ mm(77)=183;   /*M*/ mm(110)=184; /*n*/ mm(78)=185;  /*N*/ mm(241)=186; /*ñ*/ mm(209)=187; /*Ñ*/ mm(111)=188; /*o*/ mm(79)=189;  /*O*/ mm(186)=190; /*º*/ mm(243)=191; /*ó*/
  mm(211)=192;  /*Ó*/ mm(242)=193; /*ò*/ mm(210)=194; /*Ò*/ mm(244)=195; /*ô*/ mm(212)=196; /*Ô*/ mm(246)=197; /*ö*/ mm(214)=198; /*Ö*/ mm(245)=199;  /*õ*/ mm(213)=200; /*Õ*/ mm(248)=201; /*ø*/ mm(216)=202; /*Ø*/ mm(156)=203; /*œ*/ mm(140)=204; /*Œ*/ mm(112)=205; /*p*/ mm(80)=206;  /*P*/ mm(113)=207; /*q*/
  mm(81)=208;   /*Q*/ mm(114)=209; /*r*/ mm(82)=210;  /*R*/ mm(115)=211; /*s*/ mm(83)=212;  /*S*/ mm(154)=213; /*š*/ mm(138)=214; /*Š*/ mm(223)=215;  /*ß*/ mm(116)=216; /*t*/ mm(84)=217;  /*T*/ mm(254)=218; /*þ*/ mm(222)=219; /*Þ*/ mm(153)=220; /*™*/ mm(117)=221; /*u*/ mm(85)=222;  /*U*/ mm(250)=223; /*ú*/
  mm(218)=224;  /*Ú*/ mm(249)=225; /*ù*/ mm(217)=226; /*Ù*/ mm(251)=227; /*û*/ mm(219)=228; /*Û*/ mm(252)=229; /*ü*/ mm(220)=230; /*Ü*/ mm(118)=231;  /*v*/ mm(86)=232;  /*V*/ mm(119)=233; /*w*/ mm(87)=234;  /*W*/ mm(120)=235; /*x*/ mm(88)=236;  /*X*/ mm(121)=237; /*y*/ mm(89)=238;  /*Y*/ mm(253)=239; /*ý*/
  mm(221)=240;  /*Ý*/ mm(255)=241; /*ÿ*/ mm(159)=242; /*Ÿ*/ mm(122)=243; /*z*/ mm(90)=244;  /*Z*/ mm(158)=245; /*ž*/ mm(142)=246; /*Ž*/ mm(92)=247;   /*\*/ mm(127)=248; /* */ mm(129)=249; /* */ mm(141)=250; /* */ mm(143)=251; /* */ mm(144)=252; /* */ mm(157)=253; /* */ mm(160)=254; /* */ mm(173)=255; /* */

#undef mm

#define mm(_x) (map_nocase[(unsigned char)_x])

  /* here case differences use the same code */
  mm(  0)=  0;  /* */ mm(  1)=  1; /* */ mm(  2)=  2; /* */ mm(  3)=  3; /* */ mm(  4)=  4; /* */ mm(  5)=  5; /* */ mm(  6)=  6; /* */ mm(  7)=  7;  /* */ mm(  8)=  8; /* */ mm(  9)=  9; /* */ mm( 10)= 10; /* */ mm( 11)= 11; /* */ mm( 12)= 12; /* */ mm( 13)= 13; /* */ mm( 14)= 14; /* */ mm( 15)= 15; /* */
  mm( 16)= 16;  /* */ mm( 17)= 17; /* */ mm( 18)= 18; /* */ mm( 19)= 19; /* */ mm( 20)= 20; /* */ mm( 21)= 21; /* */ mm( 22)= 22; /* */ mm( 23)= 23;  /* */ mm( 24)= 24; /* */ mm( 25)= 25; /* */ mm( 26)= 26; /* */ mm( 27)= 27; /* */ mm( 28)= 28; /* */ mm( 29)= 29; /* */ mm( 30)= 30; /* */ mm( 31)= 31; /* */
  mm(39)= 32;   /*\*/ mm(45)= 33;  /*-*/ mm(150)= 34; /*–*/ mm(151)= 35; /*—*/ mm(32)= 36;  /* */ mm(33)= 37;  /*!*/ mm(34)= 38;  /*"*/ mm(35)= 39;   /*#*/ mm(36)= 40;  /*$*/ mm(37)= 41;  /*%*/ mm(38)= 42;  /*&*/ mm(40)= 43;  /*(*/ mm(41)= 44;  /*)*/ mm(42)= 45;  /***/ mm(44)= 46;  /*,*/ mm(46)= 47;  /*.*/
  mm(47)= 48;   /*/*/ mm(58)= 49;  /*:*/ mm(59)= 50;  /*;*/ mm(63)= 51;  /*?*/ mm(64)= 52;  /*@*/ mm(91)= 53;  /*[*/ mm(93)= 54;  /*]*/ mm(94)= 55;   /*^*/ mm(136)= 56; /*ˆ*/ mm(95)= 57;  /*_*/ mm(96)= 58;  /*`*/ mm(123)= 59; /*{*/ mm(124)= 60; /*|*/ mm(125)= 61; /*}*/ mm(126)= 62; /*~*/ mm(161)= 63; /*¡*/
  mm(166)= 64;  /*¦*/ mm(168)= 65; /*¨*/ mm(175)= 66; /*¯*/ mm(180)= 67; /*´*/ mm(184)= 68; /*¸*/ mm(191)= 69; /*¿*/ mm(152)= 70; /*˜*/ mm(145)= 71;  /*‘*/ mm(146)= 72; /*’*/ mm(130)= 73; /*‚*/ mm(147)= 74; /*“*/ mm(148)= 75; /*”*/ mm(132)= 76; /*„*/ mm(139)= 77; /*‹*/ mm(155)= 78; /*›*/ mm(162)= 79; /*¢*/
  mm(163)= 80;  /*£*/ mm(164)= 81; /*¤*/ mm(165)= 82; /*¥*/ mm(128)= 83; /*€*/ mm(43)= 84;  /*+*/ mm(60)= 85;  /*<*/ mm(61)= 86;  /*=*/ mm(62)= 87;   /*>*/ mm(177)= 88; /*±*/ mm(171)= 89; /*«*/ mm(187)= 90; /*»*/ mm(215)= 91; /*×*/ mm(247)= 92; /*÷*/ mm(167)= 93; /*§*/ mm(169)= 94; /*©*/ mm(172)= 95; /*¬*/
  mm(174)= 96;  /*®*/ mm(176)= 97; /*°*/ mm(181)= 98; /*µ*/ mm(182)= 99; /*¶*/ mm(133)=100; /*…*/ mm(134)=101; /*†*/ mm(135)=102; /*‡*/ mm(183)=103;  /*·*/ mm(149)=104; /*•*/ mm(137)=105; /*‰*/ mm(48)=106;  /*0*/ mm(188)=107; /*¼*/ mm(189)=108; /*½*/ mm(190)=109; /*¾*/ mm(49)=110;  /*1*/ mm(185)=111; /*¹*/
  mm(50)=112;   /*2*/ mm(178)=113; /*²*/ mm(51)=114;  /*3*/ mm(179)=115; /*³*/ mm(52)=116;  /*4*/ mm(53)=117;  /*5*/ mm(54)=118;  /*6*/ mm(55)=119;   /*7*/ mm(56)=120;  /*8*/ mm(57)=121;  /*9*/ mm(97)=122;  /*a*/ mm(65)=122;  /*A*/ mm(170)=124; /*ª*/ mm(225)=125; /*á*/ mm(193)=125; /*Á*/ mm(224)=127; /*à*/
  mm(192)=127;  /*À*/ mm(226)=129; /*â*/ mm(194)=129; /*Â*/ mm(228)=131; /*ä*/ mm(196)=131; /*Ä*/ mm(227)=133; /*ã*/ mm(195)=133; /*Ã*/ mm(229)=135;  /*å*/ mm(197)=135; /*Å*/ mm(230)=137; /*æ*/ mm(198)=137; /*Æ*/ mm(98)=139;  /*b*/ mm(66)=139;  /*B*/ mm(99)=141;  /*c*/ mm(67)=141;  /*C*/ mm(231)=143; /*ç*/
  mm(199)=143;  /*Ç*/ mm(100)=145; /*d*/ mm(68)=145;  /*D*/ mm(240)=147; /*ð*/ mm(208)=147; /*Ð*/ mm(101)=149; /*e*/ mm(69)=149;  /*E*/ mm(233)=151;  /*é*/ mm(201)=151; /*É*/ mm(232)=153; /*è*/ mm(200)=153; /*È*/ mm(234)=155; /*ê*/ mm(202)=155; /*Ê*/ mm(235)=157; /*ë*/ mm(203)=157; /*Ë*/ mm(102)=159; /*f*/
  mm(70)=159;   /*F*/ mm(131)=161; /*ƒ*/ mm(103)=162; /*g*/ mm(71)=162;  /*G*/ mm(104)=164; /*h*/ mm(72)=164;  /*H*/ mm(105)=166; /*i*/ mm(73)=166;   /*I*/ mm(237)=168; /*í*/ mm(205)=168; /*Í*/ mm(236)=170; /*ì*/ mm(204)=170; /*Ì*/ mm(238)=172; /*î*/ mm(206)=172; /*Î*/ mm(239)=174; /*ï*/ mm(207)=174; /*Ï*/
  mm(106)=176;  /*j*/ mm(74)=176;  /*J*/ mm(107)=178; /*k*/ mm(75)=178;  /*K*/ mm(108)=180; /*l*/ mm(76)=180;  /*L*/ mm(109)=182; /*m*/ mm(77)=182;   /*M*/ mm(110)=184; /*n*/ mm(78)=184;  /*N*/ mm(241)=186; /*ñ*/ mm(209)=186; /*Ñ*/ mm(111)=188; /*o*/ mm(79)=188;  /*O*/ mm(186)=190; /*º*/ mm(243)=191; /*ó*/
  mm(211)=191;  /*Ó*/ mm(242)=193; /*ò*/ mm(210)=193; /*Ò*/ mm(244)=195; /*ô*/ mm(212)=195; /*Ô*/ mm(246)=197; /*ö*/ mm(214)=197; /*Ö*/ mm(245)=199;  /*õ*/ mm(213)=199; /*Õ*/ mm(248)=201; /*ø*/ mm(216)=201; /*Ø*/ mm(156)=203; /*œ*/ mm(140)=203; /*Œ*/ mm(112)=205; /*p*/ mm(80)=205;  /*P*/ mm(113)=207; /*q*/
  mm(81)=207;   /*Q*/ mm(114)=209; /*r*/ mm(82)=209;  /*R*/ mm(115)=211; /*s*/ mm(83)=211;  /*S*/ mm(154)=213; /*š*/ mm(138)=213; /*Š*/ mm(223)=215;  /*ß*/ mm(116)=216; /*t*/ mm(84)=216;  /*T*/ mm(254)=218; /*þ*/ mm(222)=218; /*Þ*/ mm(153)=220; /*™*/ mm(117)=221; /*u*/ mm(85)=221;  /*U*/ mm(250)=223; /*ú*/
  mm(218)=223;  /*Ú*/ mm(249)=225; /*ù*/ mm(217)=225; /*Ù*/ mm(251)=227; /*û*/ mm(219)=227; /*Û*/ mm(252)=229; /*ü*/ mm(220)=229; /*Ü*/ mm(118)=231;  /*v*/ mm(86)=231;  /*V*/ mm(119)=233; /*w*/ mm(87)=233;  /*W*/ mm(120)=235; /*x*/ mm(88)=235;  /*X*/ mm(121)=237; /*y*/ mm(89)=237;  /*Y*/ mm(253)=239; /*ý*/
  mm(221)=239;  /*Ý*/ mm(255)=241; /*ÿ*/ mm(159)=241; /*Ÿ*/ mm(122)=243; /*z*/ mm(90)=243;  /*Z*/ mm(158)=245; /*ž*/ mm(142)=245; /*Ž*/ mm(92)=247;   /*\*/ mm(127)=248; /* */ mm(129)=249; /* */ mm(141)=250; /* */ mm(143)=251; /* */ mm(144)=252; /* */ mm(157)=253; /* */ mm(160)=254; /* */ mm(173)=255; /* */

#undef mm
}

static char iStrUTF8toLatin1(const char* *s)
{
  char c = **s;

  if (c >= 0) 
    return c;   /* ASCII */

  if ((c & 0x20) == 0)       /* Use 00100000 to detect 110XXXXX */
  {
    short u;
    u  = (c & 0x1F) << 6;    /* first part + make room for second part */
    (*s)++;
    c = **s;
    u |= (c & 0x3F);         /* second part (10XXXXXX) */
    if (u >= -128 && u < 128)
      return (char)u;
    else
      return 0;
  }

  /* only increment the pointer for the remaining codes */
  if ((c & 0x10) == 0)       /* Use 00010000 to detect 1110XXXX */
    *s += 3-1;  
  else if ((c & 0x08) == 0)  /* Use 00001000 to detect 11110XXX */
    *s += 4-1;

  return 0;
}

static char* iStrLatin1toUTF8(char* s, char c)
{
  unsigned char uc = (unsigned char)c;
  if (uc < 128) 
    *s = c; /* s not incremented */
  else
  {
    /* all 11 bit codepoints (0x0 -- 0x7ff) fit within a 2byte utf8 char
     * firstbyte  = 110 +xxxxx := 0xc0 + (char >> 6) MSB
     * secondbyte = 10 +xxxxxx := 0x80 + (char & 63) LSB */
    *s = 0xc0 | ((uc >> 6) & 0x1F); s++;  /* 2+1+5 bits */
    *s = 0x80 | (uc & 0x3F);              /* 1+1+6 bits */
  }
  return s;
}


/*
The Alphanum Algorithm is an improved sorting algorithm for strings
containing numbers.  Instead of sorting numbers in ASCII order like a
standard sort, this algorithm sorts numbers in numeric order.

The Alphanum Algorithm is discussed at http://www.DaveKoelle.com/alphanum.html

This implementation is Copyright (c) 2008 Dirk Jagdmann <doj@cubic.org>.
It is a cleanroom implementation of the algorithm and not derived by
other's works. In contrast to the versions written by Dave Koelle this
source code is distributed with the libpng/zlib license.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you
       must not claim that you wrote the original software. If you use
       this software in a product, an acknowledgment in the product
       documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and
       must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
       distribution.

***************************************************************************

The following code is based on the "alphanum.hpp" code 
downloaded from the Dave Koelle page and implemented by Dirk Jagdmann.

It was modified to the C language and simplified to IUP needs.
*/

IUP_SDK_API int iupStrCompare(const char *l, const char *r, int casesensitive, int utf8)
{
  enum mode_t { STRING, NUMBER } mode=STRING;

  if (l == r)
    return 0;

  if (!l && r)
    return -1;

  if (l && !r)
    return 1;

  if (!Latin1_map)
    iStrInitLatin1_map();

  while(*l && *r)
  {
    if (mode == STRING)
    {
      while((*l) && (*r))
      {
        int diff;
        char l_char = *l, 
             r_char = *r;

        /* check if this are digit characters */
        int l_digit = iup_isdigit(l_char), 
            r_digit = iup_isdigit(r_char);

        /* if both characters are digits, we continue in NUMBER mode */
        if(l_digit && r_digit)
        {
          mode = NUMBER;
          break;
        }

        /* if only the left character is a digit, we have a result */
        if(l_digit) return -1;

        /* if only the right character is a digit, we have a result */
        if(r_digit) return +1;

        if (utf8)
        {
          l_char = iStrUTF8toLatin1(&l);  /* increment n-1 an utf8 character */
          r_char = iStrUTF8toLatin1(&r);
        }

        /* compute the difference of both characters */
        if (casesensitive)
          diff = Latin1_map[(unsigned char)l_char] - Latin1_map[(unsigned char)r_char];
        else
          diff = Latin1_map_nocase[(unsigned char)l_char] - Latin1_map_nocase[(unsigned char)r_char];

        /* if they differ we have a result */
        if(diff != 0) return diff;

        /* otherwise process the next characters */
        ++l;
        ++r;
      }
    }
    else /* mode==NUMBER */
    {
      unsigned long r_int;
      long diff;

      /* get the left number */
      unsigned long l_int=0;
      while(*l && iup_isdigit(*l))
      {
        /* TODO: this can overflow */
        l_int = l_int*10 + *l-'0';
        ++l;
      }

      /* get the right number */
      r_int=0;
      while(*r && iup_isdigit(*r))
      {
        /* TODO: this can overflow */
        r_int = r_int*10 + *r-'0';
        ++r;
      }

      /* if the difference is not equal to zero, we have a comparison result */
      diff = l_int-r_int;
      if (diff != 0)
        return (int)diff;

      /* otherwise we process the next substring in STRING mode */
      mode=STRING;
    }
  }

  if (*r) return -1;
  if (*l) return +1;
  return 0;
}

IUP_SDK_API int iupStrCompareEqual(const char *l, const char *r, int casesensitive, int utf8, int partial)
{
  if (!l || !r)
    return 0;

  if (!Latin1_map)
    iStrInitLatin1_map();

  while(*l && *r)
  {
    int diff;
    char l_char = *l, 
         r_char = *r;

    if (utf8)
    {
      l_char = iStrUTF8toLatin1(&l);  /* increment n-1 an utf8 character */
      r_char = iStrUTF8toLatin1(&r);
    }

    /* compute the difference of both characters */
    if (casesensitive)
      diff = l_char - r_char;
    else
      diff = Latin1_map_nocase[(unsigned char)l_char] - Latin1_map_nocase[(unsigned char)r_char];

    /* if they differ we have a result */
    if(diff != 0) 
      return 0;

    /* otherwise process the next characters */
    ++l;
    ++r;
  }

  /* check also for terminator */
  if (*l == *r) 
    return 1;

  if (partial && *r == 0) 
    return 1;  /* if second string is at terminator, then it is partially equal */

  return 0;
}

static char iStrToUpperLatin1(char c)
{
  unsigned char uc = (unsigned char)c;

  if (c >= 'a' && c <= 'z') 
    return (c - 'a') + 'A';

  if (uc == 154) return (char)(unsigned char)138; /* š / Š */
  if (uc == 156) return (char)(unsigned char)140; /* œ / Œ */
  if (uc == 158) return (char)(unsigned char)142; /* ž / Ž */
  if (uc == 255) return (char)(unsigned char)159; /* ÿ / Ÿ */

  if (uc == 247) return c;  /* ÷ */
  if (uc >= 224 && uc <= 254) return (char)(unsigned char)((uc - 224) + 192); /* à - þ / À - Þ */

  return c;
}

static char iStrToLowerLatin1(char c)
{
  unsigned char uc = (unsigned char)c;

  if (c >= 'A' && c <= 'Z')
    return (c - 'A') + 'a';

  if (uc == 138) return (char)(unsigned char)154; /* š / Š */
  if (uc == 140) return (char)(unsigned char)156; /* œ / Œ */
  if (uc == 142) return (char)(unsigned char)158; /* ž / Ž */
  if (uc == 159) return (char)(unsigned char)255; /* ÿ / Ÿ */

  if (uc == 215) return c;  /* × */
  if (uc >= 192 && uc <= 222) return (char)(unsigned char)((uc - 192) + 224); /* à - þ / À - Þ */

  return c;
}

IUP_SDK_API void iupStrChangeCase(char* dstr, const char* sstr, int case_flag, int utf8)
{
  int first = 1;
  if (!sstr || sstr[0] == 0) return;
  for (; *sstr; sstr++, dstr++)
  {
    char src, dst;

    if (utf8)
      src = iStrUTF8toLatin1(&sstr);  /* may increment an utf8 character */
    else
      src = *sstr;

    dst = src;

    switch (case_flag)
    {
    case IUP_CASE_UPPER:
      dst = iStrToUpperLatin1(src);
      break;
    case IUP_CASE_LOWER:
      dst = iStrToLowerLatin1(src);
      break;
    case IUP_CASE_TOGGLE:
    {
      char c = iStrToUpperLatin1(src);
      if (c != src) /* was lower */
        dst = c;
      else
        dst = iStrToLowerLatin1(src);
      break;
    }
    case IUP_CASE_TITLE:
      if (first || (dstr[-1] == ' ' && 
                    dstr[+1] != 0 && dstr[+1] != ' ' &&
                    dstr[+2] != 0 && dstr[+2] != ' ' &&
                    dstr[+3] != 0 && dstr[+3] != ' ')) /* the first letter of the string or the first letter of a word separated by spaces, but with more than 3 characters */
        dst = iStrToUpperLatin1(src);
      else
        dst = iStrToLowerLatin1(src);
      break;
    }

    if (utf8)
      dstr = iStrLatin1toUTF8(dstr, dst);
    else
      *dstr = dst;

    first = 0;
  }
  *dstr = 0;
}

static int iStrIncUTF8(const char* str)
{
  if (*str >= 0)      /* ASCII */
    return 1;  
  else if ((*str & 0x20) == 0)  /* Use 00100000 to detect 110XXXXX */
    return 2;  
  else if ((*str & 0x10) == 0)  /* Use 00010000 to detect 1110XXXX */
    return 3;  
  else if ((*str & 0x08) == 0)  /* Use 00001000 to detect 11110XXX */
    return 4;
  return 1;
}

IUP_SDK_API int iupStrCompareFind(const char *l, const char *r, int casesensitive, int utf8)
{
  int i, inc, l_len, r_len, count;

  if (!l || !r)
    return 0;

  l_len = (int)strlen(l);
  r_len = (int)strlen(r);
  count = l_len - r_len;
  if (count < 0)
    return 0;

  count++;

  for (i=0; i<count; i++)
  {
    if (iupStrCompareEqual(l, r, casesensitive, utf8, 1))
      return 1;

    if (utf8)
    {
      inc = iStrIncUTF8(l);
      l += inc;
      i += inc-1;
    }
    else
      l++;
  }

  return 0;
}

static void iStrFixPosUTF8(const char* str, int *start, int *end)
{
  int p = 0, i = 0, find = 0, inc;
  while (*(str + i))
  {
    if (find == 0 && p == *start)
    {
      *start = i;
      find = 1;
    }
    if (find == 1 && p == *end)
    {
      *end = i;
      return;
    }

    inc = iStrIncUTF8(str + i);
    i += inc;
    p++;
  }

  if (find == 0 && p == *start)
  {
    *start = i;
    find = 1;
  }

  if (find == 1 && p == *end)
    *end = i;
}

IUP_SDK_API void iupStrRemove(char* str, int start, int end, int dir, int utf8)
{
  int len;

  if (end < start || !str || str[0] == 0)
    return;

  if (start == end)
  {
    if (dir == 1)  /* (forward) */
      end++;
    else  /* dir==-1 (backward) */
    {
      if (start == 0) /* there is nothing to remove before */
        return;
      else
        start--;
    }
  }

  if (utf8)
    iStrFixPosUTF8(str, &start, &end);

  /* from "start" remove up to "end", but not including "end" */
  len = (int)strlen(str);
  if (start >= len) { start = len - 1; end = len; }
  if (end > len) end = len;

  memmove(str + start, str + end, len - end + 1);
}

IUP_SDK_API char* iupStrInsert(const char* str, const char* insert_str, int start, int end, int utf8)
{
  char* new_str = (char*)str;
  int insert_len, len;

  if (!str || !insert_str)
    return NULL;

  insert_len = (int)strlen(insert_str);
  len = (int)strlen(str);

  if (utf8)
    iStrFixPosUTF8(str, &start, &end);

  if (end == start || insert_len > end - start)
  {
    new_str = malloc(len - (end - start) + insert_len + 1);
    memcpy(new_str, str, start);
    memcpy(new_str + start, insert_str, insert_len);
    memcpy(new_str + start + insert_len, str + end, len - end + 1);
  }
  else
  {
    memcpy(new_str + start, insert_str, insert_len);
    memcpy(new_str + start + insert_len, str + end, len - end + 1);
  }

  return new_str;
}

int iupStrIsAscii(const char* str)
{
  if (!str)
    return 0;

  while (*str)
  {
    int c = *str;
    if (c < 0)
      return 0;
    str++;
  }
  return 1;
}

static char* iStrSetLocale(const char* decimal_symbol)
{
  if (decimal_symbol)
  {
#ifdef __ANDROID__
    if ('.' != decimal_symbol[0])
#else
    struct lconv* locale_info = localeconv();
    if (locale_info->decimal_point[0] != decimal_symbol[0])
#endif
    {
      char* old_locale = setlocale(LC_NUMERIC, NULL);

      if (decimal_symbol[0] == '.')
      {
        old_locale = iupStrDup(old_locale);  /* must be before another setlocale */
        setlocale(LC_NUMERIC, "en-US");
        return old_locale;
      }
      else if (decimal_symbol[0] == ',')
      {
        old_locale = iupStrDup(old_locale);  /* must be before another setlocale */
        setlocale(LC_NUMERIC, "pt-BR");
        return old_locale;
      }
    }
  }

  return NULL;
}

static void iStrResetLocale(char* old_locale)
{
  if (old_locale)
  {
    setlocale(LC_NUMERIC, old_locale);
    free(old_locale);
  }
}

IUP_SDK_API int iupStrToDoubleLocale(const char *str, double *d, const char* decimal_symbol)
{
  int ret, locale_set = 0;
  char* old_locale;

  if (!str) 
    return 0;

  old_locale = iStrSetLocale(decimal_symbol);
  if (old_locale) locale_set = 1;

  ret = sscanf(str, "%lf", d);

  iStrResetLocale(old_locale);

  if (ret != 1) 
    return 0;

  if (locale_set)
    return 2;
  else
    return 1;
}

IUP_SDK_API void iupStrPrintfDoubleLocale(char *str, const char *format, double d, const char* decimal_symbol)
{
  char* old_locale = iStrSetLocale(decimal_symbol);

  sprintf(str, format, d);

  iStrResetLocale(old_locale);
}

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h> /* for close */
#endif

IUP_SDK_API int iupStrTmpFileName(char* filename, const char* prefix)
{
#ifdef WIN32
  char tmpPath[10240];
  if (GetTempPathA(10240, tmpPath) == 0)
    return 0;
  if (GetTempFileNameA(tmpPath, prefix, 0, filename) == 0)
    return 0;
#elif OLD_TMPFILENAME
  char* tmp = tempnam(NULL, prefix);
  if (!tmp)
    return 0;
  strcpy(filename, tmp);
  free(tmp);
#else
  char* dirname = getenv("TMPDIR");
  if (!dirname) dirname = "/tmp";
  if (strlen(dirname) >= 10240 - 10)
    return 0;
  strcpy(filename, dirname);
  strcat(filename, "/");
  strcat(filename, prefix);
  strcat(filename, "XXXXXX"); /* will be replaced by mkstemp */
  int fd = mkstemp(filename);
  if (fd == -1)
    return 0;
  close(fd);
#endif
  return 1;
}

IUP_SDK_API char* iupStrFileMakeURL(const char* filename)
{
  int start = filename[0] == '/' ? 7 : 8;
  int i, size = (int)strlen(filename) + 1;
  char* url = malloc(start + size);
  memcpy(url, "file:///", start);
  memcpy(url + start, filename, size);
  for (i = start; i < size + start; i++)
  {
    if (url[i] == '\\')
      url[i] = '/';
  }
  return url;
}

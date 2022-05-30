/** \file
 * \brief IupScanf implementation.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "iup.h"

#include "iup_predialogs.h"
#include "iup_str.h"
#include "iup_assert.h"
         
         
#define ALLOC(n,t)  ((t *)calloc((n),sizeof(t)))
#define REQUIRE(b)  {if (!(b)) goto cleanup;}
#define REQUIRE_ARG(b)  {if (!(b)) goto cleanup_arg;}

IUP_API int IupScanf (const char *format, ...)
{
  int i;
  int fields_out_count=(-1);    /* return code if not error (error <  0) */
  int error=(-1);    /* return code if error     (error >= 0) */
  int fields_in_count;
  int *width=NULL;
  int *scroll=NULL;
  char **prompt=NULL;
  char **text=NULL;
  char *title=NULL;
  char *s=NULL;
  char *s1=NULL;
  char *outf=NULL;
  va_list va;

  iupASSERT(format!=NULL);
  if (!format)
    return 0;

  fields_in_count=iupStrCountChar(format,'\n')-1;
  REQUIRE(fields_in_count>0);
  width=ALLOC(fields_in_count, int);
  REQUIRE(width!=NULL);
  scroll=ALLOC(fields_in_count, int);
  REQUIRE(scroll!=NULL);
  prompt=ALLOC(fields_in_count, char *);
  REQUIRE(prompt!=NULL);
  text=ALLOC(fields_in_count, char *);
  REQUIRE(text!=NULL);

  va_start(va,format);
  REQUIRE_ARG((s1=s=(char *)iupStrDup(format)) != NULL);
  title = iupStrDupUntil((const char**)&s, '\n');
  REQUIRE_ARG(title != NULL);
  for (i=0; i<fields_in_count; ++i)
  {
    int n;
    prompt[i] = iupStrDupUntil((const char**)&s, '%');
    REQUIRE_ARG(prompt[i] != NULL);
    n=sscanf(s,"%d.%d",width+i,scroll+i);
    REQUIRE_ARG(n == 2);
    s=strchr(s,'%');
    REQUIRE_ARG(s != NULL);
    if (outf) free(outf);
    outf = iupStrDupUntil((const char**)&s, '\n');
    text[i]=ALLOC(width[i]+1,char);
    REQUIRE_ARG(text[i] != NULL);

    switch (s[-2])
    {
    case 'd':
    case 'i':
    case 'o':
    case 'u':
    case 'x':
    case 'X':
      if (s[-3]=='l')
        sprintf(text[i],outf,*((long *)va_arg(va,long *)));
      else if (s[-3]=='h')
        sprintf(text[i],outf,*((short *)va_arg(va,short *)));
      else
        sprintf(text[i],outf,*((int *)va_arg(va,int *)));
      break;
    case 'e':
    case 'f':
    case 'g':
    case 'E':
    case 'G':
      if (s[-3]=='l')
        sprintf(text[i],outf,*((double *)va_arg(va,double *)));
      else
        sprintf(text[i],outf,*((float *)va_arg(va,float *)));
      break;
    case 's':
      sprintf(text[i],outf,((char *)va_arg(va,char *)));
      break;
    default:
      goto cleanup_arg;
    }
  }
  va_end(va);

  REQUIRE(iupDataEntry(fields_in_count,width,scroll,title,prompt,text)>0);

  fields_out_count=0;
  va_start(va,format);
  s=strchr(format,'\n')+1;
  for (i=0; i<fields_in_count; ++i)
  {
    s=strchr(s,'\n')+1;
    switch (s[-2])
    {
    case 'd':
    case 'u':
      if (s[-3]=='l')
      {
        if (sscanf(text[i],"%ld",((long *)va_arg(va,long *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      else if (s[-3]=='h')
      {
        if (sscanf(text[i],"%hd",((short *)va_arg(va,short *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      else
      {
        if (sscanf(text[i],"%d",((int *)va_arg(va,int *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      break;
    case 'i':
    case 'o':
    case 'x':
    case 'X':
      if (s[-3]=='l')
      {
        if (sscanf(text[i],"%li",((long *)va_arg(va,long *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      else if (s[-3]=='h')
      {
        if (sscanf(text[i],"%hi",((short *)va_arg(va,short *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      else
      {
        if (sscanf(text[i],"%i",((int *)va_arg(va,int *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      break;
    case 'e':
    case 'f':
    case 'g':
    case 'E':
    case 'G':
      if (s[-3]=='l')
      {
        if (sscanf(text[i],"%lf",((double *)va_arg(va,double *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      else
      {
        if (sscanf(text[i], "%f", ((float *)va_arg(va,float *)))!=1)
          if (error < 0) error = fields_out_count;
      }
      break;
    case 's':
      {
        char *str=(char *)va_arg(va,char *);
        iupStrCopyN(str, 4096, text[i]);
      }
      break;
    }
    ++fields_out_count;
  }
cleanup_arg:
  va_end(va);

cleanup:
  if (s1) free(s1);
  if (title) free(title);
  if (width) free(width);
  if (scroll) free(scroll);
  if (outf) free(outf);
  if (prompt)
  {
    for (i=0; i<fields_in_count; ++i)
    {
      if (prompt[i]) 
        free(prompt[i]);
    }
    free(prompt);
  }
  if (text)
  {
    for (i=0; i<fields_in_count; ++i)
    {
      if (text[i]) 
        free(text[i]);
    }
    free(text);
  }

  return (error < 0) ? fields_out_count : error;
}

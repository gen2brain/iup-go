/** \file
* \brief Line Base Text File Load
*
* See Copyright Notice in "iup.h"
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "iup_export.h"
#include "iup_linefile.h"

#define LINEFILE_STRING_BLOCK 80

struct _IlineFile
{
  FILE* file;
  char* line_buffer;
  int buffer_maxsize;
};

IUP_SDK_API const char* iupLineFileGetBuffer(IlineFile* line_file)
{
  return line_file->line_buffer;
}

IUP_SDK_API int iupLineFileEOF(IlineFile* line_file)
{
  return feof(line_file->file);
}

IUP_SDK_API IlineFile* iupLineFileOpen(const char* filename)
{
  FILE* file = fopen(filename, "rb");
  if (!file)
    return NULL;

  {
    IlineFile* line_file = malloc(sizeof(IlineFile));
    memset(line_file, 0, sizeof(IlineFile));

    line_file->file = file;
    line_file->buffer_maxsize = LINEFILE_STRING_BLOCK;
    line_file->line_buffer = (char*)malloc(line_file->buffer_maxsize);

    return line_file;
  }
}

IUP_SDK_API void iupLineFileClose(IlineFile* line_file)
{
  fclose(line_file->file);
  free(line_file->line_buffer);  /* use free because of realloc */
  free(line_file);
}

IUP_SDK_API int iupLineFileReadLine(IlineFile* line_file)
{
  char char_buffer[1];
  int ret, count = 0;
  
  do 
  {
    ret = (int)fread(char_buffer, 1, 1, line_file->file);
    if (ret == 0)
    {
      if (feof(line_file->file))  /* last line */
        break;

      return -1;  /* error reading */
    }

    if (count+1 > line_file->buffer_maxsize)
    {
      line_file->buffer_maxsize += LINEFILE_STRING_BLOCK;
      line_file->line_buffer = (char*)realloc(line_file->line_buffer, line_file->buffer_maxsize);
      memset(line_file->line_buffer + line_file->buffer_maxsize - LINEFILE_STRING_BLOCK, 0, LINEFILE_STRING_BLOCK);
    }

    if (char_buffer[0] != '\r' && char_buffer[0] != '\n')
    {
      line_file->line_buffer[count] = char_buffer[0];
      count++;
    }
  } while (char_buffer[0] != '\n');

  line_file->line_buffer[count] = 0;
  return count;
}


/** \file
 * \brief Simple expandable array
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "iup_export.h"
#include "iup_array.h"
#include "iup_assert.h"


struct _Iarray
{
  void* data;
  int count;
  int max_count;
  int elem_size;
  int start_count;
};

IUP_SDK_API Iarray* iupArrayCreate(int start_count, int elem_size)
{
  Iarray* iarray = (Iarray*)malloc(sizeof(Iarray));
  iarray->count = 0;
  iarray->elem_size = elem_size;
  iarray->max_count = start_count;
  iarray->start_count = start_count;
  iarray->data = malloc(elem_size*start_count);
  iupASSERT(iarray->data!=NULL);
  if (!iarray->data)
  {
    free(iarray);
    return NULL;
  }
  memset(iarray->data, 0, elem_size*start_count);
  return iarray;
}

IUP_SDK_API void iupArrayDestroy(Iarray* iarray)
{
  iupASSERT(iarray!=NULL);
  if (!iarray)
    return;
  if (iarray->data) 
  {
    memset(iarray->data, 0, iarray->elem_size*iarray->max_count);
    free(iarray->data);
  }
  free(iarray);
}

IUP_SDK_API void* iupArrayGetData(Iarray* iarray)
{
  iupASSERT(iarray!=NULL);
  if (!iarray)
    return NULL;
  return iarray->data;
}

IUP_SDK_API void* iupArrayReleaseData(Iarray* iarray)
{
  void* data;
  iupASSERT(iarray != NULL);
  if (!iarray)
    return NULL;
  data = iarray->data;
  iarray->data = NULL;
  return data; 
}

IUP_SDK_API void* iupArrayInc(Iarray* iarray)
{
  iupASSERT(iarray!=NULL);
  if (!iarray)
    return NULL;
  if (iarray->count >= iarray->max_count)
  {
    int old_count = iarray->max_count;
    iarray->max_count += iarray->start_count;
    iarray->data = realloc(iarray->data, iarray->elem_size*iarray->max_count);
    iupASSERT(iarray->data!=NULL);
    if (!iarray->data)
      return NULL;
    memset((unsigned char*)iarray->data + iarray->elem_size*old_count, 0, iarray->elem_size*(iarray->max_count-old_count));
  }
  iarray->count++;
  return iarray->data;
}

IUP_SDK_API void* iupArrayAdd(Iarray* iarray, int add_count)
{
  iupASSERT(iarray!=NULL);
  if (!iarray)
    return NULL;
  if (iarray->count+add_count > iarray->max_count)
  {
    int old_count = iarray->max_count;
    iarray->max_count += add_count;
    iarray->data = realloc(iarray->data, iarray->elem_size*iarray->max_count);
    iupASSERT(iarray->data!=NULL);
    if (!iarray->data)
      return NULL;
    memset((unsigned char*)iarray->data + iarray->elem_size*old_count, 0, iarray->elem_size*(iarray->max_count-old_count));
  }
  iarray->count += add_count;
  return iarray->data;
}

IUP_SDK_API void* iupArrayInsert(Iarray* iarray, int index, int insert_count)
{
  iupASSERT(iarray!=NULL);
  if (!iarray)
    return NULL;
  if (index < 0 || index > iarray->count)
    return NULL;
  iupArrayAdd(iarray, insert_count);
  if (index < iarray->count)  /* if equal, insert at the end, no need to move data */
    memmove((unsigned char*)iarray->data + iarray->elem_size*(index + insert_count), 
            (unsigned char*)iarray->data + iarray->elem_size*index, 
            iarray->elem_size*(iarray->count - insert_count - index));
  /* clear new data */
  memset((unsigned char*)iarray->data + iarray->elem_size*index, 0, iarray->elem_size*insert_count);
  return iarray->data;
}

IUP_SDK_API void iupArrayRemove(Iarray* iarray, int index, int remove_count)
{
  iupASSERT(iarray!=NULL);
  if (!iarray)
    return;
  if (index < 0 || index+remove_count > iarray->count)
    return;
  if (index+remove_count < iarray->count)  /* if equal, remove at the end, no need to move data */
    memmove((unsigned char*)iarray->data + iarray->elem_size*index, 
            (unsigned char*)iarray->data + iarray->elem_size*(index + remove_count), 
            iarray->elem_size*(iarray->count - remove_count - index));
  /* clear old data */
  memset((unsigned char*)iarray->data + iarray->elem_size*(iarray->count - remove_count), 0, iarray->elem_size*remove_count);
  iarray->count -= remove_count;
}

IUP_SDK_API int iupArrayCount(Iarray* iarray)
{
  iupASSERT(iarray!=NULL);
  if (!iarray)
    return 0;
  return iarray->count;
}

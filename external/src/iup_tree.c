/** \file
 * \brief Tree control
 *
 * See Copyright Notice in iup.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_tree.h"
#include "iup_assert.h"



/************************************************************************************/

/* These utilities must work for IupTree and IupFlatTree */

typedef int (*IFnv)(Ihandle*, void*);

IUP_API int IupTreeGetId(Ihandle* ih, void *userdata)
{
  IFnv find_userdata_cb;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return -1;

  find_userdata_cb = (IFnv)IupGetCallback(ih, "_IUPTREE_FIND_USERDATA_CB");

  return find_userdata_cb(ih, userdata);
}

IUP_API int IupTreeSetUserId(Ihandle* ih, int id, void* userdata)
{
  int count;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  count = IupGetInt(ih, "COUNT");
  if (id >= 0 && id < count)
  {
    IupSetAttributeId(ih, "USERDATA", id, (char*)userdata);
    return 1;
  }

  return 0;
}

IUP_API void* IupTreeGetUserId(Ihandle* ih, int id)
{
  int count;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  count = IupGetInt(ih, "COUNT");
  if (id >= 0 && id < count)
    return IupGetAttributeId(ih, "USERDATA", id);

  return NULL;
}

IUP_API void IupTreeSetAttributeHandle(Ihandle* ih, const char* a, int id, Ihandle* ih_named)
{
  IupSetAttributeHandleId(ih, a, id, ih_named);
}


/************************************************************************************/


static void iTreeInitializeImages(void)
{
  Ihandle *image_leaf, *image_blank, *image_paper;  
  Ihandle *image_collapsed, *image_expanded, *image_empty;

#define ITREE_IMG_WIDTH   16
#define ITREE_IMG_HEIGHT  16

  unsigned char img_leaf[ITREE_IMG_WIDTH*ITREE_IMG_HEIGHT] = 
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 4, 4, 5, 5, 5, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 4, 5, 5, 1, 6, 1, 5, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 4, 4, 5, 5, 1, 6, 1, 5, 0, 0, 0,
    0, 0, 0, 0, 3, 4, 4, 4, 5, 5, 1, 1, 5, 0, 0, 0,
    0, 0, 0, 0, 2, 3, 4, 4, 4, 5, 5, 5, 4, 0, 0, 0,
    0, 0, 0, 0, 2, 3, 3, 4, 4, 4, 5, 4, 4, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 3, 3, 4, 4, 4, 4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 2, 2, 3, 3, 3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  unsigned char img_collapsed[ITREE_IMG_WIDTH*ITREE_IMG_HEIGHT] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0,  
    0, 0, 0, 2, 6, 5, 5, 7, 2, 3, 0, 0, 0, 0, 0, 0, 
    0, 0, 2, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 0, 
    0, 0, 2, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 4, 3, 
    0, 0, 2, 5, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 4, 3, 
    0, 0, 2, 5, 7, 7, 7, 7, 7, 7, 1, 7, 1, 7, 4, 3, 
    0, 0, 2, 5, 7, 7, 7, 7, 7, 7, 7, 1, 7, 1, 4, 3, 
    0, 0, 2, 5, 7, 7, 7, 7, 1, 7, 1, 7, 1, 7, 4, 3, 
    0, 0, 2, 5, 7, 7, 7, 7, 7, 1, 7, 1, 7, 1, 4, 3, 
    0, 0, 2, 5, 7, 7, 7, 7, 1, 7, 1, 7, 1, 1, 4, 3, 
    0, 0, 2, 5, 1, 7, 1, 1, 7, 1, 7, 1, 1, 1, 4, 3, 
    0, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3,  
    0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
  };

  unsigned char img_expanded[ITREE_IMG_WIDTH*ITREE_IMG_HEIGHT] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 2, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 2, 1, 3, 3, 3, 3, 1, 2, 2, 2, 2, 2, 2, 0, 
    0, 0, 2, 1, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 6, 4, 
    0, 0, 2, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 6, 4, 
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 3, 6, 4, 
    0, 2, 1, 3, 3, 3, 3, 3, 5, 3, 5, 6, 4, 6, 6, 4, 
    0, 2, 1, 3, 3, 3, 3, 3, 3, 5, 3, 6, 4, 6, 6, 4, 
    0, 0, 2, 0, 3, 3, 3, 3, 5, 3, 5, 5, 2, 4, 2, 4, 
    0, 0, 2, 0, 3, 3, 5, 5, 3, 5, 5, 5, 6, 4, 2, 4, 
    0, 0, 0, 2, 0, 5, 3, 3, 5, 5, 5, 5, 6, 2, 4, 4, 
    0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 
    0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
  };

  unsigned char img_blank[ITREE_IMG_WIDTH*ITREE_IMG_HEIGHT] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 4, 3, 4, 3, 4, 3, 4, 3, 4, 3, 0, 0, 0, 0,
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 5, 4, 0, 0, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 4, 0, 0,
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0
  };

  unsigned char img_paper[ITREE_IMG_WIDTH*ITREE_IMG_HEIGHT] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 4, 3, 4, 3, 4, 3, 4, 3, 4, 3, 0, 0, 0, 0,
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 5, 4, 0, 0, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 4, 0, 0,
    0, 0, 3, 1, 4, 3, 4, 3, 4, 3, 4, 2, 2, 2, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 3, 4, 3, 4, 3, 4, 3, 4, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 4, 3, 4, 3, 4, 3, 4, 3, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 3, 4, 3, 4, 3, 4, 3, 4, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 1, 4, 3, 4, 3, 4, 3, 4, 3, 1, 5, 2, 0,
    0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 0,
    0, 0, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0
  };

  image_leaf      = IupImage(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, img_leaf);
  image_collapsed = IupImage(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, img_collapsed);
  image_expanded  = IupImage(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, img_expanded);
  image_blank     = IupImage(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, img_blank);
  image_paper     = IupImage(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, img_paper);
  image_empty     = IupImage(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, NULL);

  IupSetAttribute(image_leaf, "0", "BGCOLOR");
  IupSetAttribute(image_leaf, "1", "192 192 192");
  IupSetAttribute(image_leaf, "2", "56 56 56");
  IupSetAttribute(image_leaf, "3", "99 99 99");
  IupSetAttribute(image_leaf, "4", "128 128 128");
  IupSetAttribute(image_leaf, "5", "161 161 161");
  IupSetAttribute(image_leaf, "6", "222 222 222");

  IupSetAttribute(image_collapsed, "0", "BGCOLOR");
  IupSetAttribute(image_collapsed, "1", "255 206 156");
  IupSetAttribute(image_collapsed, "2", "156 156 0");
  IupSetAttribute(image_collapsed, "3", "0 0 0");
  IupSetAttribute(image_collapsed, "4", "206 206 99");
  IupSetAttribute(image_collapsed, "5", "255 255 206");
  IupSetAttribute(image_collapsed, "6", "247 247 247");
  IupSetAttribute(image_collapsed, "7", "255 255 156");

  IupSetAttribute(image_expanded, "0", "BGCOLOR");
  IupSetAttribute(image_expanded, "1", "255 255 255");
  IupSetAttribute(image_expanded, "2", "156 156 0");
  IupSetAttribute(image_expanded, "3", "255 255 156");
  IupSetAttribute(image_expanded, "4", "0 0 0");
  IupSetAttribute(image_expanded, "5", "255 206 156");
  IupSetAttribute(image_expanded, "6", "206 206 99");

  IupSetAttribute(image_blank, "0", "BGCOLOR");
  IupSetAttribute(image_blank, "1", "255 255 255");
  IupSetAttribute(image_blank, "2", "000 000 000");
  IupSetAttribute(image_blank, "3", "119 119 119");
  IupSetAttribute(image_blank, "4", "136 136 136");
  IupSetAttribute(image_blank, "5", "187 187 187");

  IupSetAttribute(image_paper, "0", "BGCOLOR");
  IupSetAttribute(image_paper, "1", "255 255 255");
  IupSetAttribute(image_paper, "2", "000 000 000");
  IupSetAttribute(image_paper, "3", "119 119 119");
  IupSetAttribute(image_paper, "4", "136 136 136");
  IupSetAttribute(image_paper, "5", "187 187 187");

  IupSetAttribute(image_empty, "0", "BGCOLOR");
  IupSetAttribute(image_empty, "FLAT_ALPHA", "Yes"); /* necessary for Windows */

  IupSetHandle("IMGLEAF",      image_leaf);
  IupSetHandle("IMGCOLLAPSED", image_collapsed);
  IupSetHandle("IMGEXPANDED",  image_expanded);
  IupSetHandle("IMGBLANK",     image_blank);
  IupSetHandle("IMGPAPER",     image_paper);
  IupSetHandle("IMGEMPTY",     image_empty);

#undef ITREE_IMG_WIDTH
#undef ITREE_IMG_HEIGHT
}

void iupTreeUpdateImages(Ihandle *ih)
{
  char* value = iupAttribGet(ih, "IMAGELEAF");
  if (!value) value = "IMGLEAF";
  iupAttribSetClassObject(ih, "IMAGELEAF", value);

  value = iupAttribGet(ih, "IMAGEBRANCHCOLLAPSED");
  if (!value) value = "IMGCOLLAPSED";
  iupAttribSetClassObject(ih, "IMAGEBRANCHCOLLAPSED", value);

  value = iupAttribGet(ih, "IMAGEBRANCHEXPANDED");
  if (!value) value = "IMGEXPANDED";
  iupAttribSetClassObject(ih, "IMAGEBRANCHEXPANDED", value);
}

void iupTreeSelectLastCollapsedBranch(Ihandle* ih, int *last_id)
{
  /* if last selected item is a branch, then select its children */
  if (iupStrEqual(IupGetAttributeId(ih, "KIND", *last_id), "BRANCH") && 
      iupStrEqual(IupGetAttributeId(ih, "STATE", *last_id), "COLLAPSED"))
  {
    int childcount = IupGetIntId(ih, "CHILDCOUNT", *last_id);
    if (childcount > 0)
    {
      int start = *last_id + 1;
      int end = *last_id + childcount;
      IupSetfAttribute(ih, "MARK", "%d-%d", start, end);
      *last_id = *last_id + childcount;
    }
  }
}

int iupTreeForEach(Ihandle* ih, iupTreeNodeFunc func, void* userdata)
{
  int i;
  for (i = 0; i < ih->data->node_count; i++)
  {
    if (!func(ih, ih->data->node_cache[i].node_handle, i, userdata))
      return 0;
  }

  return 1;
}

int iupTreeFindNodeId(Ihandle* ih, InodeHandle* node_handle)
{
  /* Unoptimized version:
  int i;
  for (i = 0; i < ih->data->node_count; i++)
  {
    if (ih->data->node_cache[i].node_handle == node_handle)
      return i;
  }
  */
  InodeData *node_cache = ih->data->node_cache;
  while(node_cache->node_handle != node_handle && 
        node_cache->node_handle != NULL)   /* the cache always have zeros at the end */
    node_cache++;

  if (node_cache->node_handle != NULL)
    return (int)(node_cache - ih->data->node_cache);
  else
    return -1;
}

static int iTreeFindUserDataId(Ihandle* ih, void* userdata)
{
  /* Unoptimized version:
  int i;
  for (i = 0; i < ih->data->node_count; i++)
  {
    if (ih->data->node_cache[i].userdata == userdata)
      return i;
  }
  */
  InodeData *node_cache = ih->data->node_cache;
  while(node_cache->userdata != userdata && 
        node_cache->node_handle != NULL)   /* the cache always have zeros at the end */
    node_cache++;

  if (node_cache->node_handle != NULL)
    return (int)(node_cache - ih->data->node_cache);
  else
    return -1;
}

InodeHandle* iupTreeGetNode(Ihandle* ih, int id)
{
  if (id >= 0 && id < ih->data->node_count)
    return ih->data->node_cache[id].node_handle;
  else if (id == IUP_INVALID_ID && ih->data->node_count!=0)
    return iupdrvTreeGetFocusNode(ih);
  else
    return NULL;
}

InodeHandle* iupTreeGetNodeFromString(Ihandle* ih, const char* name_id)
{
  int id = IUP_INVALID_ID;
  iupStrToInt(name_id, &id);
  return iupTreeGetNode(ih, id);
}

static void iTreeAddToCache(Ihandle* ih, int id, InodeHandle* node_handle)
{
  iupASSERT(id >= 0 && id < ih->data->node_count);
  if (id < 0 || id >= ih->data->node_count)
    return;

  /* node_count here already contains the final count */

  if (id == ih->data->node_count-1)
    ih->data->node_cache[id].node_handle = node_handle;
  else
  {
    /* open space for the new id */
    int remain_count = ih->data->node_count-id;
    memmove(ih->data->node_cache+id+1, ih->data->node_cache+id, remain_count*sizeof(InodeData));
    ih->data->node_cache[id].node_handle = node_handle;
  }

  ih->data->node_cache[id].userdata = NULL;
}

void iupTreeIncCacheMem(Ihandle* ih)
{
  /* node_count here already contains the final count */

  if (ih->data->node_count+10 > ih->data->node_cache_max)
  {
    int old_node_cache_max = ih->data->node_cache_max;
    ih->data->node_cache_max += 20;
    ih->data->node_cache = realloc(ih->data->node_cache, ih->data->node_cache_max*sizeof(InodeData));
    memset(ih->data->node_cache+old_node_cache_max, 0, 20*sizeof(InodeData));
  }
}

void iupTreeAddToCache(Ihandle* ih, int add, int kindPrev, InodeHandle* prevNode, InodeHandle* node_handle)
{
  int new_id = 0;

  ih->data->node_count++;

  /* node_count here already contains the final count */
  iupTreeIncCacheMem(ih);

  if (prevNode)
  {
    if (add || kindPrev == ITREE_LEAF)
    {
      /* ADD implies always that id=prev_id+1 */
      /* INSERT after a leaf implies always that new_id=prev_id+1 */
      int prev_id = iupTreeFindNodeId(ih, prevNode);
      new_id = prev_id+1;
    }
    else
    {
      /* INSERT after a branch implies always that new_id=prev_id+1+child_count */
      int prev_id = iupTreeFindNodeId(ih, prevNode);
      int child_count = iupdrvTreeTotalChildCount(ih, prevNode);
      new_id = prev_id+1+child_count;
    }
  }

  iTreeAddToCache(ih, new_id, node_handle);
  iupAttribSetInt(ih, "LASTADDNODE", new_id);
}

void iupTreeDelFromCache(Ihandle* ih, int id, int count)
{
  int remain_count, last_add_node;

  /* id can be the last node, actually==node_count becase node_count is already updated */
  iupASSERT(id >= 0 && id <= ih->data->node_count);  
  if (id < 0 || id > ih->data->node_count)
    return;

  /* minimum safety check for LASTADDNODE */
  last_add_node = iupAttribGetInt(ih, "LASTADDNODE");
  if (last_add_node >= id && last_add_node < id+count)
    iupAttribSet(ih, "LASTADDNODE", NULL);
  else if (last_add_node >= id+count)
    iupAttribSetInt(ih, "LASTADDNODE", last_add_node-count);

  /* node_count here already contains the final count */

  /* remove id+count */
  remain_count = ih->data->node_count-id;
  memmove(ih->data->node_cache+id, ih->data->node_cache+id+count, remain_count*sizeof(InodeData));

  /* clear the remaining space */
  memset(ih->data->node_cache+ih->data->node_count, 0, count*sizeof(InodeData));
}

void iupTreeCopyMoveCache(Ihandle* ih, int id_src, int id_dst, int count, int is_copy)
{
  int remain_count;

  iupASSERT(id_src >= 0 && id_src < ih->data->node_count);
  if (id_src < 0 || id_src >= ih->data->node_count)
    return;

  iupASSERT(id_dst >= 0 && id_dst < ih->data->node_count);
  if (id_dst < 0 || id_dst >= ih->data->node_count)
    return;

  /* dst can NOT be inside src+count area */
  iupASSERT(id_dst < id_src || id_dst > id_src+count);
  if (id_dst >= id_src && id_dst <= id_src+count)
    return;

  /* id_dst here points to the final position for a copy operation */

  /* node_count here contains the final count for a copy operation */
  iupTreeIncCacheMem(ih);

  /* add space for new nodes */
  remain_count = ih->data->node_count - (id_dst + count);
  memmove(ih->data->node_cache + id_dst+count, ih->data->node_cache + id_dst, remain_count * sizeof(InodeData));

  if (is_copy) 
  {
    /* during a copy, the userdata is not reused, so clear it */
    memset(ih->data->node_cache+id_dst, 0, count*sizeof(InodeData));
  }
  else /* move = copy + delete */
  {
    /* compensate because we added space for new nodes */
    if (id_src > id_dst)
      id_src += count;

    /* copy userdata from src to dst */
    memcpy(ih->data->node_cache+id_dst, ih->data->node_cache+id_src, count*sizeof(InodeData));

    /* remove the src */
    remain_count = ih->data->node_count - (id_src + count);
    memmove(ih->data->node_cache+id_src, ih->data->node_cache+id_src+count, remain_count*sizeof(InodeData));

    /* clear the remaining space */
    memset(ih->data->node_cache+ih->data->node_count-count, 0, count*sizeof(InodeData));
  }

  iupAttribSet(ih, "LASTADDNODE", NULL);
}


/*************************************************************************/


char* iupTreeGetSpacingAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->spacing);
}

static char* iTreeGetMarkModeAttrib(Ihandle* ih)
{
  if (ih->data->mark_mode==ITREE_MARK_SINGLE)
    return "SINGLE";
  else
    return "MULTIPLE";
}

static int iTreeSetMarkModeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "MULTIPLE"))
    ih->data->mark_mode = ITREE_MARK_MULTIPLE;    
  else 
    ih->data->mark_mode = ITREE_MARK_SINGLE;

  if (ih->handle)
    iupdrvTreeUpdateMarkMode(ih); /* for this to work, must update during map */

  return 0;
}

static int iTreeSetShiftAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value) && iupAttribGetBoolean(ih, "CTRL"))
    iTreeSetMarkModeAttrib(ih, "MULTIPLE");
  else
    iTreeSetMarkModeAttrib(ih, "SINGLE");
  return 1;
}

static int iTreeSetCtrlAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value) && iupAttribGetBoolean(ih, "SHIFT"))
    iTreeSetMarkModeAttrib(ih, "MULTIPLE");
  else
    iTreeSetMarkModeAttrib(ih, "SINGLE");
  return 1;
}

static char* iTreeGetShowRenameAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->show_rename); 
}

static int iTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_rename = 1;
  else
    ih->data->show_rename = 0;

  return 0;
}

static char* iTreeGetShowToggleAttrib(Ihandle* ih)
{
  if (ih->data->show_toggle)
  {
    if (ih->data->show_toggle == 2)
      return "3STATE";
    else
      return "YES";
  }
  else
    return "NO";
}

static int iTreeSetShowToggleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "3STATE"))
    ih->data->show_toggle = 2;
  else if (iupStrBoolean(value))
    ih->data->show_toggle = 1;
  else
    ih->data->show_toggle = 0;

  return 0;
}

static char* iTreeGetShowDragDropAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->show_dragdrop); 
}

static int iTreeSetShowDragDropAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->show_dragdrop = 1;
  else
    ih->data->show_dragdrop = 0;

  return 0;
}

static int iTreeSetAddLeafAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  iupdrvTreeAddNode(ih, id, ITREE_LEAF, value, 1);
  return 0;
}

static int iTreeSetAddBranchAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  iupdrvTreeAddNode(ih, id, ITREE_BRANCH, value, 1);
  return 0;
}

static int iTreeSetInsertLeafAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  iupdrvTreeAddNode(ih, id, ITREE_LEAF, value, 0);
  return 0;
}

static int iTreeSetInsertBranchAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  iupdrvTreeAddNode(ih, id, ITREE_BRANCH, value, 0);
  return 0;
}

static char* iTreeGetAddExpandedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->add_expanded); 
}

static int iTreeSetAddExpandedAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->add_expanded = 1;
  else
    ih->data->add_expanded = 0;

  return 0;
}

static char* iTreeGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->node_count);
}

static char* iTreeGetTotalChildCountAttrib(Ihandle* ih, int id)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  if (!node_handle)
    return NULL;

  return iupStrReturnInt(iupdrvTreeTotalChildCount(ih, node_handle));
}

static char* iTreeGetUserDataAttrib(Ihandle* ih, int id)
{
  if (id >= 0 && id < ih->data->node_count)
    return ih->data->node_cache[id].userdata;
  else if (id == IUP_INVALID_ID && ih->data->node_count!=0)
  {
    InodeHandle* node_handle = iupdrvTreeGetFocusNode(ih);
    id = iupTreeFindNodeId(ih, node_handle);
    if (id >= 0 && id < ih->data->node_count)
      return ih->data->node_cache[id].userdata;
  }
  return NULL;
}

static int iTreeSetUserDataAttrib(Ihandle* ih, int id, const char* value)
{
  if (id >= 0 && id < ih->data->node_count)
    ih->data->node_cache[id].userdata = (void*)value;
  else if (id == IUP_INVALID_ID && ih->data->node_count!=0)
  {
    InodeHandle* node_handle = iupdrvTreeGetFocusNode(ih);
    id = iupTreeFindNodeId(ih, node_handle);
    if (id >= 0 && id < ih->data->node_count)
      ih->data->node_cache[id].userdata = (void*)value;
  }
  return 0;
}


/*****************************************************************************************/


static int iTreeDropData_CB(Ihandle *ih, char* type, void* data, int len, int x, int y)
{
  int id = IupConvertXYToPos(ih, x, y);
  int is_ctrl = 0;
  char key[5];

  /* Data is not the pointer, it contains the pointer */
  Ihandle* ih_source;
  memcpy((void*)&ih_source, data, len);

  /*TODO support IupFlatTree??? */
  if (!IupClassMatch(ih_source, "tree"))
    return IUP_DEFAULT;

  /* A copy operation is enabled with the CTRL key pressed, or else a move operation will occur.
     A move operation will be possible only if the attribute DRAGSOURCEMOVE is Yes.
     When no key is pressed the default operation is copy when DRAGSOURCEMOVE=No and move when DRAGSOURCEMOVE=Yes. */
  iupdrvGetKeyState(key);
  if (key[1] == 'C')
    is_ctrl = 1;

  /* Here copy/move of multiple selection is not allowed,
     only a single node and its children. */

  if(ih_source->data->mark_mode == ITREE_MARK_SINGLE)
  {
    int src_id = iupAttribGetInt(ih_source, "_IUP_TREE_SOURCEID");
    InodeHandle *itemDst, *itemSrc;

    itemSrc = iupTreeGetNode(ih_source, src_id);
    if (!itemSrc)
      return IUP_DEFAULT;

    itemDst = iupTreeGetNode(ih, id);
    if (!itemDst)
      return IUP_DEFAULT;

    /* Copy the node and its children to the new position */
    iupdrvTreeDragDropCopyNode(ih_source, ih, itemSrc, itemDst);

    if(IupGetInt(ih_source, "DRAGSOURCEMOVE") && !is_ctrl)
      IupSetAttribute(ih_source, "DELNODE0", "MARKED");
  }

  (void)type;
  return IUP_DEFAULT;
}

static int iTreeDragData_CB(Ihandle *ih, char* type, void *data, int len)
{
  int id = iupAttribGetInt(ih, "_IUP_TREE_SOURCEID");
  if (id < 0)
    return IUP_DEFAULT;

  if(ih->data->mark_mode == ITREE_MARK_SINGLE)
  {
    /* Single selection */
    IupSetAttributeId(ih, "MARKED", id, "YES");
  }

  /* Copy source handle */
  memcpy(data, (void*)&ih, len);
 
  (void)type;
  return IUP_DEFAULT;
}

static int iTreeDragDataSize_CB(Ihandle* ih, char* type)
{
  (void)ih;
  (void)type;
  return sizeof(Ihandle*);
}

static int iTreeDragEnd_CB(Ihandle *ih, int del)
{
  iupAttribSetInt(ih, "_IUP_TREE_SOURCEID", -1);
  (void)del;
  return IUP_DEFAULT;
}

static int iTreeDragBegin_CB(Ihandle* ih, int x, int y)
{
  int id = IupConvertXYToPos(ih, x, y);
  iupAttribSetInt(ih, "_IUP_TREE_SOURCEID", id);
  return IUP_DEFAULT;
}

static int iTreeSetDragDropTreeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    /* Register callbacks to enable drag and drop between trees, DRAG&DROP attributes must still be set by the application */
    IupSetCallback(ih, "DRAGBEGIN_CB",    (Icallback)iTreeDragBegin_CB);
    IupSetCallback(ih, "DRAGDATASIZE_CB", (Icallback)iTreeDragDataSize_CB);
    IupSetCallback(ih, "DRAGDATA_CB",     (Icallback)iTreeDragData_CB);
    IupSetCallback(ih, "DRAGEND_CB",      (Icallback)iTreeDragEnd_CB);
    IupSetCallback(ih, "DROPDATA_CB",     (Icallback)iTreeDropData_CB);
  }
  else
  {
    /* Unregister callbacks */
    IupSetCallback(ih, "DRAGBEGIN_CB",    NULL);
    IupSetCallback(ih, "DRAGDATASIZE_CB", NULL);
    IupSetCallback(ih, "DRAGDATA_CB",     NULL);
    IupSetCallback(ih, "DRAGEND_CB",      NULL);
    IupSetCallback(ih, "DROPDATA_CB",     NULL);
  }

  return 1;
}

static int iTreeSetTitleFontStyleAttrib(Ihandle* ih, int id, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = IupGetAttributeId(ih, "TITLEFONT", id);
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId(ih, "TITLEFONT", id, "%s, %s %d", typeface, value, size);

  return 0;
}

static char* iTreeGetTitleFontStyleAttrib(Ihandle* ih, int id)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = IupGetAttributeId(ih, "TITLEFONT", id);
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnStrf("%s%s%s%s", is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "");
}

static int iTreeSetTitleFontSizeAttrib(Ihandle* ih, int id, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = IupGetAttributeId(ih, "TITLEFONT", id);
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId(ih, "TITLEFONT", id, "%s, %s%s%s%s %s", typeface, is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "", value);

  return 0;
}

static char* iTreeGetTitleFontSizeAttrib(Ihandle* ih, int id)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = IupGetAttributeId(ih, "TITLEFONT", id);
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnInt(size);
}


/*************************************************************************/


static int iTreeCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  ih->data = iupALLOCCTRLDATA();

  IupSetAttribute(ih, "RASTERSIZE", "400x200");
  IupSetAttribute(ih, "EXPAND", "YES");

  IupSetCallback(ih, "_IUPTREE_FIND_USERDATA_CB", (Icallback)iTreeFindUserDataId);

  ih->data->add_expanded = 1;
  ih->data->node_cache_max = 20;
  ih->data->node_cache = calloc(ih->data->node_cache_max, sizeof(InodeData));

  return IUP_NOERROR;
}

static void iTreeDestroyMethod(Ihandle* ih)
{
  if (ih->data->node_cache)
    free(ih->data->node_cache);
}

/*************************************************************************/

IUP_API Ihandle* IupTree(void)
{
  return IupCreate("tree");
}

Iclass* iupTreeNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "tree";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iupTreeNewClass;
  ic->Create = iTreeCreateMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->Destroy = iTreeDestroyMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "TOGGLEVALUE_CB",    "ii");
  iupClassRegisterCallback(ic, "SELECTION_CB",      "ii");
  iupClassRegisterCallback(ic, "MULTISELECTION_CB", "Ii");
  iupClassRegisterCallback(ic, "MULTIUNSELECTION_CB", "Ii");
  iupClassRegisterCallback(ic, "BRANCHOPEN_CB",     "i");
  iupClassRegisterCallback(ic, "BRANCHCLOSE_CB",    "i");
  iupClassRegisterCallback(ic, "EXECUTELEAF_CB",    "i");
  iupClassRegisterCallback(ic, "EXECUTEBRANCH_CB",  "i");
  iupClassRegisterCallback(ic, "SHOWRENAME_CB",     "i");
  iupClassRegisterCallback(ic, "RENAME_CB",         "is");
  iupClassRegisterCallback(ic, "DRAGDROP_CB",       "iiii");
  iupClassRegisterCallback(ic, "RIGHTCLICK_CB",     "i");
  iupClassRegisterCallback(ic, "MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "NODEREMOVED_CB", "s");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Drag&Drop */
  iupdrvRegisterDragDropAttrib(ic);

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "DRAGDROPTREE", NULL, iTreeSetDragDropTreeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDRAGDROP", iTreeGetShowDragDropAttrib, iTreeSetShowDragDropAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWRENAME",   iTreeGetShowRenameAttrib,   iTreeSetShowRenameAttrib,   NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWTOGGLE",   iTreeGetShowToggleAttrib,   iTreeSetShowToggleAttrib,   NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDEXPANDED",  iTreeGetAddExpandedAttrib,  iTreeSetAddExpandedAttrib,  IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT",        iTreeGetCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LASTADDNODE", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDROOT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPEQUALDRAG", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDELINES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDEBUTTONS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAMECARET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAMESELECTION", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CPADDING", iupBaseGetCPaddingAttrib, iupBaseSetCPaddingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CSPACING", iupBaseGetCSpacingAttrib, iupBaseSetCSpacingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttribute(ic, "CTRL",  NULL, iTreeSetCtrlAttrib,  NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SHIFT", NULL, iTreeSetShiftAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKMODE",  iTreeGetMarkModeAttrib, iTreeSetMarkModeAttrib,  IUPAF_SAMEASSYSTEM, "SINGLE", IUPAF_NOT_MAPPED);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttributeId(ic, "ADDLEAF",   NULL, iTreeSetAddLeafAttrib,   IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ADDBRANCH", NULL, iTreeSetAddBranchAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "INSERTLEAF",   NULL, iTreeSetInsertLeafAttrib,   IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "INSERTBRANCH", NULL, iTreeSetInsertBranchAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttributeId(ic, "TOTALCHILDCOUNT", iTreeGetTotalChildCountAttrib,   NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "USERDATA", iTreeGetUserDataAttrib, iTreeSetUserDataAttrib, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONTSTYLE", iTreeGetTitleFontStyleAttrib, iTreeSetTitleFontStyleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONTSIZE", iTreeGetTitleFontSizeAttrib, iTreeSetTitleFontSizeAttrib, IUPAF_NO_INHERIT);

  /* Default node images */
  if (!IupGetHandle("IMGLEAF") || !IupGetHandle("IMGBLANK") || !IupGetHandle("IMGPAPER"))
    iTreeInitializeImages();

  iupdrvTreeInitClass(ic);

  return ic;
}


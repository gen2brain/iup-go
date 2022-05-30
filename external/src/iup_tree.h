/** \file
 * \brief Tree control (not exported API)
 *
 * See Copyright Notice in iup.h
 */

#ifndef __IUP_TREE_H 
#define __IUP_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Kinds of node */
#define ITREE_BRANCH        0
#define ITREE_LEAF          1

#define ITREE_MARK_SINGLE 0
#define ITREE_MARK_MULTIPLE 1

#define ITREE_UPDATEIMAGE_LEAF      1
#define ITREE_UPDATEIMAGE_COLLAPSED 2
#define ITREE_UPDATEIMAGE_EXPANDED  3

void iupdrvTreeInitClass(Iclass* ic);
void iupTreeUpdateImages(Ihandle *ih);
void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add);
void iupdrvTreeUpdateMarkMode(Ihandle *ih);

char* iupTreeGetSpacingAttrib(Ihandle* ih);

#if defined(GTK_MAJOR_VERSION)
typedef char InodeHandle;   /* should be void, but we use char to force compiler checks */
#elif defined(XmVERSION)
typedef struct _WidgetRec InodeHandle;
#elif defined(WINVER)
typedef struct _TREEITEM InodeHandle;
#elif defined(__APPLE__)
typedef void InodeHandle;   /* should be id, but don't want Obj-C in C header, and don't want id* */
#else
typedef struct _InodeData InodeHandle;
#endif

typedef struct _InodeData
{
  InodeHandle* node_handle;  /* HTREEITEM (Windows), Widget (Motif), user_data (model-specific GTK), id (Apple) */
  void* userdata;
} InodeData;

typedef int (*iupTreeNodeFunc)(Ihandle* ih, InodeHandle* node_handle, int id, void* userdata);
int iupTreeForEach(Ihandle* ih, iupTreeNodeFunc func, void* userdata);
InodeHandle* iupTreeGetNode(Ihandle* ih, int id);
InodeHandle* iupTreeGetNodeFromString(Ihandle* ih, const char* name_id);
int iupTreeFindNodeId(Ihandle* ih, InodeHandle* node_handle);

InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih);
int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle);
void iupTreeSelectLastCollapsedBranch(Ihandle* ih, int *last_id);

void iupTreeDelFromCache(Ihandle* ih, int id, int count);
void iupTreeIncCacheMem(Ihandle* ih); /* needed for IupCocoa */
void iupTreeAddToCache(Ihandle* ih, int add, int kindPrev, InodeHandle* prevNode, InodeHandle* node_handle);
void iupTreeCopyMoveCache(Ihandle* ih, int id_src, int id_new, int count, int is_copy);

/* copy from one control to another control */
void iupdrvTreeDragDropCopyNode(Ihandle *src, Ihandle *dst, InodeHandle *itemSrc, InodeHandle *itemDst);

/* Structure of the tree */
struct _IcontrolData
{
  int mark_mode,
      add_expanded,
      show_dragdrop,
      show_rename,
      show_toggle,
      stamp,    /* GTK only */
      spacing;

  void* def_image_leaf;       /* Default image leaf */
  void* def_image_collapsed;  /* Default image collapsed */
  void* def_image_expanded;   /* Default image expanded */

  void* def_image_leaf_mask;       /* Motif Only */
  void* def_image_collapsed_mask;  
  void* def_image_expanded_mask;  

  InodeData *node_cache;   /* given the id returns node native handle and user_data */
  int node_cache_max, node_count;
};


#ifdef __cplusplus
}
#endif

#endif

/** \file
 * \brief List Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_LIST_H 
#define __IUP_LIST_H

#ifdef __cplusplus
extern "C" {
#endif


void iupdrvListInitClass(Iclass* ic);
void iupdrvListAddBorders(Ihandle* ih, int *w, int *h);
void iupdrvListAddItemSpace(Ihandle* ih, int *h);
int iupdrvListGetCount(Ihandle* ih);
void iupdrvListAppendItem(Ihandle* ih, const char* value);
void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value);
void iupdrvListRemoveItem(Ihandle* ih, int pos);
void iupdrvListRemoveAllItems(Ihandle* ih);

/* Used by List and Text, implemented in Text */
int iupEditCallActionCb(Ihandle* ih, IFnis cb, const char* insert_value, int start, int end, void *mask, int nc, int remove_dir, int utf8);

int iupListGetPosAttrib(Ihandle* ih, int id);
int iupListSetIdValueAttrib(Ihandle* ih, int id, const char* value);
char* iupListGetNCAttrib(Ihandle* ih);
char* iupListGetPaddingAttrib(Ihandle* ih);
char* iupListGetSpacingAttrib(Ihandle* ih);

void iupListSingleCallActionCb(Ihandle* ih, IFnsii cb, int pos);
void iupListMultipleCallActionCb(Ihandle* ih, IFnsii cb, IFns multi_cb, int* pos, int sel_count);
void iupListSingleCallDblClickCb(Ihandle* ih, IFnis cb, int pos);
int iupListCallDragDropCb(Ihandle* ih, int drag_id, int drop_id, int *is_ctrl);

void iupListSetInitialItems(Ihandle* ih);
void iupListUpdateOldValue(Ihandle* ih, int pos, int removed);
void* iupdrvListGetImageHandle(Ihandle* ih, int id);
int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage);

struct _IcontrolData 
{
  int sb,  /* scrollbar configuration, can be changed only before map */
      nc,
      spacing,
      horiz_padding, 
      vert_padding,
      last_caret_pos,
      is_multiple,
      is_dropdown,
      has_editbox,
      maximg_w, maximg_h, /* used only in Windows */
      show_image,
      show_dragdrop;
  Imask* mask;
};


#ifdef __cplusplus
}
#endif

#endif

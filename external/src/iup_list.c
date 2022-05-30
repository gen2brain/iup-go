/** \file
 * \brief List Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_assert.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_list.h"


void iupListSingleCallDblClickCb(Ihandle* ih, IFnis cb, int pos)
{
  char *text;

  if (pos<=0)
    return;

  text = IupGetAttributeId(ih, "", pos);

  if (cb(ih, pos, text) == IUP_CLOSE)
    IupExitLoop();
}

static void iListCallActionCallback(Ihandle* ih, IFnsii cb, int pos, int state)
{
  char *text;

  if (pos<=0)
    return;

  text = IupGetAttributeId(ih, "", pos);

  if (cb(ih, text, pos, state) == IUP_CLOSE)
    IupExitLoop();
}

void iupListUpdateOldValue(Ihandle* ih, int pos, int removed)
{
  if (!ih->data->has_editbox)
  {
    char* old_value = iupAttribGet(ih, "_IUPLIST_OLDVALUE");
    if (old_value)
    {
      int old_pos = atoi(old_value)-1; /* was in IUP reference, starting at 1 */
      if (ih->data->is_dropdown || !ih->data->is_multiple)
      {
        if (old_pos >= pos)
        {
          if (removed && old_pos == pos)
          {
            /* when the current item is removed nothing remains selected */
            iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
          }
          else
            iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", removed? old_pos-1: old_pos+1);
        }
      }
      else
      {
        /* multiple selection on a non drop-down list. */
        char* value = IupGetAttribute(ih, "VALUE");
        iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", value);
      }
    }
  }
}

void iupListSingleCallActionCb(Ihandle* ih, IFnsii cb, int pos)
{
  char* old_str = iupAttribGet(ih, "_IUPLIST_OLDVALUE");
  if (old_str)
  {
    int oldpos = atoi(old_str);
    if (oldpos != pos)
    {
      iListCallActionCallback(ih, cb, oldpos, 0);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
      iListCallActionCallback(ih, cb, pos, 1);
    }
  }
  else
  {
    iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    iListCallActionCallback(ih, cb, pos, 1);
  }
}

void iupListMultipleCallActionCb(Ihandle* ih, IFnsii cb, IFns multi_cb, int* pos, int sel_count)
{
  int i, count = iupdrvListGetCount(ih);

  char* old_str = iupAttribGet(ih, "_IUPLIST_OLDVALUE");
  int old_count = old_str? (int)strlen(old_str): 0;

  char* str = malloc(count+1);
  memset(str, '-', count);
  str[count]=0;
  for (i=0; i<sel_count; i++)
    str[pos[i]] = '+';

  if (old_count != count)
  {
    old_count = 0;
    old_str = NULL;
  }

  if (multi_cb)
  {
    int unchanged = 1;

    for (i=0; i<count && old_str; i++)
    {
      if (str[i] == old_str[i])
        str[i] = 'x';    /* mark unchanged values */
      else
        unchanged = 0;
    }

    if (old_str && unchanged)
    {
      free(str);
      return;
    }

    if (multi_cb(ih, str) == IUP_CLOSE)
      IupExitLoop();

    for (i=0; i<count && old_str; i++)
    {
      if (str[i] == 'x')
        str[i] = old_str[i];    /* restore unchanged values */
    }
  }
  else
  {
    /* must simulate the click on each item */
    for (i=0; i<count; i++)
    {
      if (i >= old_count)  /* new items, if selected then call the callback */
      {
        if (str[i] == '+')
          iListCallActionCallback(ih, cb, i+1, 1);
      }
      else if (str[i] != old_str[i])
      {
        if (str[i] == '+')
          iListCallActionCallback(ih, cb, i+1, 1);
        else
          iListCallActionCallback(ih, cb, i+1, 0);
      }
    }
  }

  iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", str);
  free(str);
}

int iupListGetPosAttrib(Ihandle* ih, int pos)
{
  int count;

  pos--; /* IUP items start at 1 */

  if (pos < 0) 
    return -1;

  count = iupdrvListGetCount(ih);

  if (pos == count) return -2;
  if (pos > count) return -1;

  return pos;
}

void iupListSetInitialItems(Ihandle* ih)
{
  char *value;
  int i = 1;
  while ((value = iupAttribGetId(ih, "", i))!=NULL)
  {
    iupdrvListAppendItem(ih, value);
    iupAttribSetId(ih, "", i, NULL);

    i++;
  }
}

char* iupListGetSpacingAttrib(Ihandle* ih)
{
  if (!ih->data->is_dropdown)
    return iupStrReturnInt(ih->data->spacing);
  else
    return NULL;
}

char* iupListGetPaddingAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
    return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
  else
    return NULL;
}

char* iupListGetNCAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
    return iupStrReturnInt(ih->data->nc);
  else
    return NULL;
}

int iupListSetIdValueAttrib(Ihandle* ih, int pos, const char* value)
{
  int count = iupdrvListGetCount(ih);

  pos--; /* IUP starts at 1 */

  if (!value)
  {
    if (pos >= 0 && pos <= count-1)
    {
      if (pos == 0)
      {
        iupdrvListRemoveAllItems(ih);
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
      }
      else
      {
        int i = pos;
        while (i < count)
        {
          iupdrvListRemoveItem(ih, pos);
          i++;
        }
      }
    }
  }
  else
  {
    if (pos >= 0 && pos <= count-1)
    {
      iupdrvListRemoveItem(ih, pos);
      iupdrvListInsertItem(ih, pos, value);
    }
    else if (pos == count)
      iupdrvListAppendItem(ih, value);
  }
  return 0;
}

static int iListSetAppendItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* do not do the action before map, and ignore the call */
    return 0;
  if (value)
    iupdrvListAppendItem(ih, value);
  return 0;
}

static int iListSetInsertItemAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)  /* do not do the action before map, and ignore the call */
    return 0;
  if (value)
  {
    int pos = iupListGetPosAttrib(ih, id);
    if (pos >= 0)
      iupdrvListInsertItem(ih, pos, value);
    else if (pos == -2)
      iupdrvListAppendItem(ih, value);
  }
  return 0;
}

static int iListSetRemoveItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* do not do the action before map, and ignore the call */
    return 0;
  if (!value || iupStrEqualNoCase(value, "ALL"))
  {
    iupdrvListRemoveAllItems(ih);
    iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
  }
  else
  {
    int id;
    if (iupStrToInt(value, &id))
    {
      int pos = iupListGetPosAttrib(ih, id);
      if (pos >= 0)
        iupdrvListRemoveItem(ih, pos);
    }
  }
  return 0;
}

static int iListGetCount(Ihandle* ih)
{
  int count;
  if (ih->handle)
    count = iupdrvListGetCount(ih);
  else
  {
    count = 0;
    while (iupAttribGetId(ih, "", count+1))
      count++;
  }
  return count;
}

static char* iListGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(iListGetCount(ih));
}

static int iListSetDropdownAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
  {
    ih->data->is_dropdown = 1;
    ih->data->is_multiple = 0;
  }
  else
    ih->data->is_dropdown = 0;

  return 0;
}

static char* iListGetDropdownAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->is_dropdown); 
}

static int iListSetMultipleAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
  {
    ih->data->is_multiple = 1;
    ih->data->is_dropdown = 0;
    ih->data->has_editbox = 0;
  }
  else
    ih->data->is_multiple = 0;

  return 0;
}

static char* iListGetMultipleAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->is_multiple); 
}

static int iListSetEditboxAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
  {
    ih->data->has_editbox = 1;
    ih->data->is_multiple = 0;
  }
  else
    ih->data->has_editbox = 0;

  return 0;
}

static char* iListGetEditboxAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->has_editbox); 
}

static int iListSetScrollbarAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  else if (iupStrBoolean(value))
    ih->data->sb = 1;
  else
    ih->data->sb = 0;

  return 0;
}

static char* iListGetScrollbarAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->sb); 
}

static char* iListGetMaskAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  if (ih->data->mask)
    return iupMaskGetStr(ih->data->mask);
  else
    return NULL;
}

static int iListSetValueMaskedAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (value)
  {
    if (ih->data->mask && iupMaskCheck(ih->data->mask, value) == 0)
      return 0; /* abort */
    IupStoreAttribute(ih, "VALUE", value);
  }
  return 0;
}

static int iListSetMaskNoEmptyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mask)
  {
    int val = iupStrBoolean(value);
    iupMaskSetNoEmpty(ih->data->mask, val);
  }
  return 1;
}

static int iListSetMaskCaseIAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mask)
  {
    int val = iupStrBoolean(value);
    iupMaskSetCaseI(ih->data->mask, val);
  }
  return 1;
}

static int iListSetMaskAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
  {
    if (ih->data->mask)
    {
      iupMaskDestroy(ih->data->mask);
      ih->data->mask = NULL;
    }
  }
  else
  {
    Imask* mask = iupMaskCreate(value);
    if (mask)
    {
      int val = iupAttribGetInt(ih, "MASKCASEI");
      iupMaskSetCaseI(mask, val);

      val = iupAttribGetInt(ih, "MASKNOEMPTY");
      iupMaskSetNoEmpty(mask, val);

      if (ih->data->mask)
        iupMaskDestroy(ih->data->mask);

      ih->data->mask = mask;
      return 0;
    }
  }

  return 0;
}

static int iListSetMaskIntAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
  {
    if (ih->data->mask)
    {
      iupMaskDestroy(ih->data->mask);
      ih->data->mask = NULL;
    }
  }
  else
  {
    Imask* mask;
    int min, max;

    if (iupStrToIntInt(value, &min, &max, ':')!=2)
      return 0;

    mask = iupMaskCreateInt(min, max);
    if (mask)
    {
      int val = iupAttribGetInt(ih, "MASKNOEMPTY");
      iupMaskSetNoEmpty(mask, val);

      if (ih->data->mask)
        iupMaskDestroy(ih->data->mask);

      ih->data->mask = mask;
    }
  }

  return 0;
}

static int iListSetMaskFloatAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
  {
    if (ih->data->mask)
    {
      iupMaskDestroy(ih->data->mask);
      ih->data->mask = NULL;
    }
  }
  else
  {
    Imask* mask;
    float min, max;
    char* decimal_symbol = iupAttribGet(ih, "MASKDECIMALSYMBOL");
    if (!decimal_symbol)
      decimal_symbol = IupGetGlobal("DEFAULTDECIMALSYMBOL");

    if (iupStrToFloatFloat(value, &min, &max, ':')!=2)
      return 0;

    mask = iupMaskCreateFloat(min, max, decimal_symbol);
    if (mask)
    {
      int val = iupAttribGetInt(ih, "MASKNOEMPTY");
      iupMaskSetNoEmpty(mask, val);

      if (ih->data->mask)
        iupMaskDestroy(ih->data->mask);

      ih->data->mask = mask;
    }
  }

  return 0;
}

static int iListSetMaskRealAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
  {
    if (ih->data->mask)
    {
      iupMaskDestroy(ih->data->mask);
      ih->data->mask = NULL;
    }
  }
  else
  {
    Imask* mask;
    char* decimal_symbol = iupAttribGet(ih, "MASKDECIMALSYMBOL");
    int positive = 0;

    if (iupStrEqualNoCase(value, "UNSIGNED"))
      positive = 1;

    mask = iupMaskCreateReal(positive, decimal_symbol);
    if (mask)
    {
      int val = iupAttribGetInt(ih, "MASKNOEMPTY");
      iupMaskSetNoEmpty(mask, val);

      if (ih->data->mask)
        iupMaskDestroy(ih->data->mask);

      ih->data->mask = mask;
    }
  }

  return 0;
}

static int iListSetShowImageAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->show_image = 1;
  else
    ih->data->show_image = 0;

  return 0;
}

static char* iListGetShowImageAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->show_image); 
}

int iupListCallDragDropCb(Ihandle* ih, int drag_id, int drop_id, int *is_ctrl)
{
  IFniiii cbDragDrop = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
  int is_shift = 0;
  char key[5];
  iupdrvGetKeyState(key);
  if (key[0] == 'S')
    is_shift = 1;
  if (key[1] == 'C')
    *is_ctrl = 1;
  else
    *is_ctrl = 0;

  /* ignore a drop that will do nothing */
  if ((*is_ctrl)==0 && (drag_id+1 == drop_id || drag_id == drop_id))
    return IUP_DEFAULT;
  if ((*is_ctrl)!=0 && drag_id == drop_id)
    return IUP_DEFAULT;

  drag_id++;
  if (drop_id < 0)
    drop_id = -1;
  else
    drop_id++;

  if (cbDragDrop)
    return cbDragDrop(ih, drag_id, drop_id, is_shift, *is_ctrl);  /* starts at 1 */

  return IUP_CONTINUE; /* allow to move/copy by default if callback not defined */
}

static char* iListGetShowDragDropAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->show_dragdrop); 
}

static int iListSetShowDragDropAttrib(Ihandle* ih, const char* value)
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


/*****************************************************************************************/


static int iListDropData_CB(Ihandle *ih, char* type, void* data, int len, int x, int y)
{
  int pos = IupConvertXYToPos(ih, x, y);
  int is_ctrl = 0;
  char key[5];

  /* Data is not the pointer, it contains the pointer */
  Ihandle* ih_source;
  memcpy((void*)&ih_source, data, len);  /* but ih_source can be IupList or IupFlatList, can NOT use ih_source->data here */

  if (!IupClassMatch(ih_source, "list") && !IupClassMatch(ih_source, "flatlist"))
    return IUP_DEFAULT;

  /* A copy operation is enabled with the CTRL key pressed, or else a move operation will occur.
     A move operation will be possible only if the attribute DRAGSOURCEMOVE is Yes.
     When no key is pressed the default operation is copy when DRAGSOURCEMOVE=No and move when DRAGSOURCEMOVE=Yes. */
  iupdrvGetKeyState(key);
  if (key[1] == 'C')
    is_ctrl = 1;

  if (IupGetInt(ih_source, "MULTIPLE"))
  {
    char *buffer = IupGetAttribute(ih_source, "VALUE");

    /* Copy all selected items */
    int src_pos = 1;  /* IUP starts at 1 */
    while (buffer[src_pos - 1] != '\0')
    {
      if (buffer[src_pos - 1] == '+')
      {
        iupdrvListInsertItem(ih, pos, IupGetAttributeId(ih_source, "", src_pos));
        iupdrvListSetImageHandle(ih, pos, IupGetAttributeId(ih_source, "IMAGENATIVEHANDLE", src_pos));     /* works for IupList and IupFlatList */
        pos++;
      }

      src_pos++;
    }

    if (IupGetInt(ih_source, "DRAGSOURCEMOVE") && !is_ctrl)
    {
      /* Remove all item from source if MOVE */
      src_pos = 1;  /* IUP starts at 1 */
      while(*buffer != '\0')
      {
        if (*buffer == '+')
          IupSetInt(ih_source, "REMOVEITEM", src_pos);  /* update index in the source */

        src_pos++;
        buffer++;
      }
    }
  }
  else
  {
    int src_pos = IupGetInt(ih_source, "VALUE");
    iupdrvListInsertItem(ih, pos, IupGetAttributeId(ih_source, "", src_pos));
    iupdrvListSetImageHandle(ih, pos, IupGetAttributeId(ih_source, "IMAGENATIVEHANDLE", src_pos));    /* works for IupList and IupFlatList */

    if (IupGetInt(ih_source, "DRAGSOURCEMOVE") && !is_ctrl)
    {
      src_pos = iupAttribGetInt(ih_source, "_IUP_LIST_SOURCEPOS");
      IupSetInt(ih_source, "REMOVEITEM", src_pos);
    }
  }

  (void)type;
  return IUP_DEFAULT;
}

static int iListDragData_CB(Ihandle *ih, char* type, void *data, int len)
{
  int pos = iupAttribGetInt(ih, "_IUP_LIST_SOURCEPOS");
  if (pos < 1)
    return IUP_DEFAULT;

  if (ih->data->is_multiple)
  {
    char *buffer = IupGetAttribute(ih, "VALUE");

    /* It will not drag all selected items only
       when the user begins to drag an item not selected.
       In this case, unmark all and mark only this item.  */
    if(buffer[pos-1] == '-')
    {
      int buf_len = (int)strlen(buffer);
      IupSetAttribute(ih, "SELECTION", "NONE");
      memset(buffer, '-', buf_len);
      buffer[pos-1] = '+';
      IupSetAttribute(ih, "VALUE", buffer);
    }
  }
  else
  {
    /* Single selection */
    IupSetInt(ih, "VALUE", pos);
  }

  /* Copy source handle */
  memcpy(data, (void*)&ih, len);
 
  (void)type;
  return IUP_DEFAULT;
}

static int iListDragDataSize_CB(Ihandle* ih, char* type)
{
  (void)ih;
  (void)type;
  return sizeof(Ihandle*);
}

static int iListDragEnd_CB(Ihandle *ih, int del)
{
  iupAttribSetInt(ih, "_IUP_LIST_SOURCEPOS", 0);
  (void)del;
  return IUP_DEFAULT;
}

static int iListDragBegin_CB(Ihandle* ih, int x, int y)
{
  int pos = IupConvertXYToPos(ih, x, y);
  iupAttribSetInt(ih, "_IUP_LIST_SOURCEPOS", pos);
  return IUP_DEFAULT;
}

static int iListSetDragDropListAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    /* Register callbacks to enable drag and drop between lists */
    IupSetCallback(ih, "DRAGBEGIN_CB",    (Icallback)iListDragBegin_CB);
    IupSetCallback(ih, "DRAGDATASIZE_CB", (Icallback)iListDragDataSize_CB);
    IupSetCallback(ih, "DRAGDATA_CB",     (Icallback)iListDragData_CB);
    IupSetCallback(ih, "DRAGEND_CB",      (Icallback)iListDragEnd_CB);
    IupSetCallback(ih, "DROPDATA_CB",     (Icallback)iListDropData_CB);
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

static char* iListGetValueStringAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox && (ih->data->is_dropdown || !ih->data->is_multiple))
  {
    int i = IupGetInt(ih, "VALUE");
    return IupGetAttributeId(ih, "", i);
  }
  return NULL;
}

static int iListSetValueStringAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox && (ih->data->is_dropdown || !ih->data->is_multiple))
  {
    int i, count = iListGetCount(ih);

    for (i = 1; i <= count; i++)
    {
      char* item = IupGetAttributeId(ih, "", i);
      if (iupStrEqual(value, item))
      {
        IupSetInt(ih, "VALUE", i);
        return 0;
      }
    }
  }

  return 0;
}


/*****************************************************************************************/


static int iListCreateMethod(Ihandle* ih, void** params)
{
  if (params && params[0])
    iupAttribSetStr(ih, "ACTION", (char*)(params[0]));

  ih->data = iupALLOCCTRLDATA();
  ih->data->sb = 1;

  return IUP_NOERROR;
}

static void iListGetItemImageInfo(Ihandle *ih, int id, int *img_w, int *img_h)
{
  *img_w = 0;
  *img_h = 0;

  if (!ih->handle)
  {
    char *value = iupAttribGetId(ih, "IMAGE", id);
    if (value)
      iupImageGetInfo(value, img_w, img_h, NULL);
  }
  else
  {
    void* handle = iupdrvListGetImageHandle(ih, id);
    if (handle)
    {
      int bpp;
      iupdrvImageGetInfo(handle, img_w, img_h, &bpp);
    }
  }
}

static void iListGetNaturalItemsSize(Ihandle *ih, int *w, int *h)
{
  int visiblecolumns, i, 
      max_h = 0,
      count = iListGetCount(ih);

  *w = 0;
  *h = 0;

  iupdrvFontGetCharSize(ih, w, h);   /* one line height, and one character width */

  visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns)
  {
    *w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
    *w = (visiblecolumns*(*w))/10;
  }
  else
  {
    char *value;
    int item_w;

    for (i=1; i<=count; i++)
    {
      item_w = 0;

      value = IupGetAttributeId(ih, "", i);  /* must use IupGetAttribute to check the native system */
      if (value)
        item_w = iupdrvFontGetStringWidth(ih, value);

      if (item_w > *w)
        *w = item_w;
    }

    if (*w == 0) /* default is 5 characters in 1 item */
      *w = iupdrvFontGetStringWidth(ih, "WWWWW");
  }

  if (ih->data->show_image)
  {
    int max_w = 0;
    for (i=1; i<=count; i++)
    {
      int img_w, img_h;
      iListGetItemImageInfo(ih, i, &img_w, &img_h);
      if (img_w > max_w)
        max_w = img_w;
      if (img_h > max_h)
        max_h = img_h;
    }

    /* Used only in Windows */
    ih->data->maximg_w = max_w;
    ih->data->maximg_h = max_h;

    *w += max_w;
  }

  /* compute height for multiple lines, dropdown is just 1 line */
  if (!ih->data->is_dropdown)
  {
    int visiblelines, num_lines, 
        edit_line_size = *h;  /* don't include the highest image */

    if (ih->data->show_image && max_h > *h)  /* use the highest image to compute the natural size */
      *h = max_h;

    iupdrvListAddItemSpace(ih, h);  /* this is independent from spacing */

    *h += 2*ih->data->spacing;  /* this will be multiplied by the number of lines */
    *w += 2*ih->data->spacing;  /* include also horizontal spacing */

    num_lines = count;
    if (num_lines == 0) num_lines = 1;

    visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    if (visiblelines)
      num_lines = visiblelines;   

    *h = *h * num_lines;

    if (ih->data->has_editbox) 
      *h += edit_line_size;
  }
  else
  {
    if (!ih->data->has_editbox)
    {
      if (ih->data->show_image && max_h > *h)  /* use the highest image to compute the natural size */
        *h = max_h;
    }
  }
}

static void iListComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w, natural_h;
  int sb_size = iupdrvGetScrollbarSize();
  (void)children_expand; /* unset if not a container */

  iListGetNaturalItemsSize(ih, &natural_w, &natural_h);

  /* compute the borders space */
  iupdrvListAddBorders(ih, &natural_w, &natural_h);

  if (ih->data->is_dropdown)
  {
    /* add room for dropdown box */
    natural_w += sb_size;

    if (natural_h < sb_size)
      natural_h = sb_size;
  }
  else
  {
    /* add room for scrollbar */
    if (ih->data->sb)
    {
      natural_h += sb_size;
      natural_w += sb_size;
    }
  }

  if (ih->data->has_editbox)
  {
    natural_w += 2*ih->data->horiz_padding;
    natural_h += 2*ih->data->vert_padding;
  }

  *w = natural_w;
  *h = natural_h;
}

static void iListDestroyMethod(Ihandle* ih)
{
  if (ih->data->mask)
    iupMaskDestroy(ih->data->mask);
}


/******************************************************************************/


IUP_API Ihandle* IupList(const char* action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("list", params);
}

Iclass* iupListNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "list";
  ic->format = "a"; /* one ACTION callback name */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  /* Class functions */
  ic->New = iupListNewClass;
  ic->Create = iListCreateMethod;
  ic->Destroy = iListDestroyMethod;
  ic->ComputeNaturalSize = iListComputeNaturalSizeMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "ACTION", "sii");
  iupClassRegisterCallback(ic, "MULTISELECT_CB", "s");
  iupClassRegisterCallback(ic, "DROPDOWN_CB", "i");
  iupClassRegisterCallback(ic, "DBLCLICK_CB", "is");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupClassRegisterCallback(ic, "MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "DRAGDROP_CB", "iiii");

  iupClassRegisterCallback(ic, "EDIT_CB", "is");
  iupClassRegisterCallback(ic, "CARET_CB", "iii");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Drag&Drop */
  iupdrvRegisterDragDropAttrib(ic);

  /* IupList only */
  iupClassRegisterAttribute(ic, "SCROLLBAR", iListGetScrollbarAttrib, iListSetScrollbarAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOHIDE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MULTIPLE", iListGetMultipleAttrib, iListSetMultipleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPDOWN", iListGetDropdownAttrib, iListSetDropdownAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITBOX", iListGetEditboxAttrib, iListSetEditboxAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iListGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUESTRING", iListGetValueStringAttrib, iListSetValueStringAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CPADDING", iupBaseGetCPaddingAttrib, iupBaseSetCPaddingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CSPACING", iupBaseGetCSpacingAttrib, iupBaseSetCSpacingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  iupClassRegisterAttributeId(ic, "INSERTITEM", NULL, iListSetInsertItemAttrib, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPENDITEM", NULL, iListSetAppendItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEITEM", NULL, iListSetRemoveItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VALUEMASKED", NULL, iListSetValueMaskedAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKCASEI", NULL, iListSetMaskCaseIAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKDECIMALSYMBOL", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASK", iListGetMaskAttrib, iListSetMaskAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKINT", NULL, iListSetMaskIntAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKFLOAT", NULL, iListSetMaskFloatAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKREAL", NULL, iListSetMaskRealAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKNOEMPTY", NULL, iListSetMaskNoEmptyAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWIMAGE", iListGetShowImageAttrib, iListSetShowImageAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDRAGDROP", iListGetShowDragDropAttrib, iListSetShowDragDropAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROPLIST", NULL, iListSetDragDropListAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvListInitClass(ic);

  return ic;
}

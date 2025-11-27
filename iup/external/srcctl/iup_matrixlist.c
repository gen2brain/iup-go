/** \file
 * \brief IupMatrixList
 *
 * See Copyright Notice in "iup.h"
 *
 * Based on MTXLIB, developed at Tecgraf/PUC-Rio
 * by Renata Trautmann and Andre Derraik
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "iup.h"
#include "iup_key.h"
#include "iupcbs.h"
#include "iupcontrols.h"

#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_image.h"
#include "iup_register.h"

#include "iup_controls.h"
#include "iupdraw.h"

/* Use IupMatrix internal definitions to speed up access */
#include "matrix/iupmat_def.h"
#include "matrix/iupmat_getset.h"
#include "matrix/iupmat_edit.h"
#include "matrix/iupmat_numlc.h"


/* default sizes */
#define IMTXL_COLOR_WIDTH 16
#define IMTXL_IMAGE_WIDTH 16

/* inactive line effect */
#define IMAT_LIGHTER(_x)  (_x+192)/2


typedef struct _ImatrixListData  /* Used only by the IupMatrixList control */
{
  /* attributes */
  int editable;     /* allow adding new lines by editing the last line */

  /* internal variables */
  int label_col, color_col, image_col;  /* column order (0 means it is hidden) */
  int last_click_lin, 
      last_click_col;
} ImatrixListData;

/******************************************************************************
    Utilities
******************************************************************************/

static void iMatrixListInitializeImages(void)
{
  Ihandle *image_uncheck, *image_check, *image_del, *image_add;

#define IMTXL_IMG_WIDTH  16
#define IMTXL_IMG_HEIGHT 16

  unsigned char img_check[IMTXL_IMG_WIDTH*IMTXL_IMG_HEIGHT] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  unsigned char img_uncheck[IMTXL_IMG_WIDTH*IMTXL_IMG_HEIGHT] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  unsigned char img_del[IMTXL_IMG_WIDTH*IMTXL_IMG_HEIGHT] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  unsigned char img_add[IMTXL_IMG_WIDTH*IMTXL_IMG_HEIGHT] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  image_uncheck = IupImage(IMTXL_IMG_WIDTH, IMTXL_IMG_HEIGHT, img_uncheck);
  image_check   = IupImage(IMTXL_IMG_WIDTH, IMTXL_IMG_HEIGHT, img_check);
  image_del     = IupImage(IMTXL_IMG_WIDTH, IMTXL_IMG_HEIGHT, img_del);
  image_add     = IupImage(IMTXL_IMG_WIDTH, IMTXL_IMG_HEIGHT, img_add);

  IupSetAttribute(image_uncheck, "0", "100 100 100");
  IupSetAttribute(image_uncheck, "1", "255 255 255");

  IupSetAttribute(image_check, "0", "100 100 100");
  IupSetAttribute(image_check, "1", "255 255 255");

  IupSetAttribute(image_del, "0", "BGCOLOR");
  IupSetAttribute(image_del, "1", "255 0 0");

  IupSetAttribute(image_add, "0", "BGCOLOR");
  IupSetAttribute(image_add, "1", "100 100 100");

  IupSetHandle("MTXLIST_IMG_UNCHECK", image_uncheck);
  IupSetHandle("MTXLIST_IMG_CHECK", image_check);
  IupSetHandle("MTXLIST_IMG_DEL", image_del);
  IupSetHandle("MTXLIST_IMG_ADD", image_add);

#undef IMTXL_IMG_WIDTH
#undef IMTXL_IMG_HEIGHT
}

static void iMatrixListCopyLinAttrib(Ihandle* ih, const char* name, int lin1, int lin2)
{
  char* value = iupAttribGetId(ih, name, lin1);  /* from lin1 to lin2 */
  iupAttribSetStrId(ih, name, lin2, value);
}

static void iMatrixListCopyLinAttributes(Ihandle* ih, int lin1, int lin2)
{
  iMatrixListCopyLinAttrib(ih, "IMAGE", lin1, lin2);
  iMatrixListCopyLinAttrib(ih, "COLOR", lin1, lin2);
  iMatrixListCopyLinAttrib(ih, "ITEMACTIVE", lin1, lin2);
  iMatrixListCopyLinAttrib(ih, "ITEMFGCOLOR", lin1, lin2);
  iMatrixListCopyLinAttrib(ih, "ITEMBGCOLOR", lin1, lin2);
  iMatrixListCopyLinAttrib(ih, "IMAGEACTIVE", lin1, lin2);
  iMatrixListCopyLinAttrib(ih, "IMAGEVALUE", lin1, lin2);
}

static void iMatrixListClearLinAttributes(Ihandle* ih, int lin)
{
  iupAttribSetId(ih, "IMAGE", lin, NULL);
  iupAttribSetId(ih, "COLOR", lin, NULL);
  iupAttribSetId(ih, "ITEMACTIVE", lin, NULL);
  iupAttribSetId(ih, "ITEMFGCOLOR", lin, NULL);
  iupAttribSetId(ih, "ITEMBGCOLOR", lin, NULL);
  iupAttribSetId(ih, "IMAGEACTIVE", lin, NULL);
  iupAttribSetId(ih, "IMAGEVALUE", lin, NULL);
}

static void iMatrixListUpdateLineAttributes(Ihandle* ih, int base, int count, int add)
{
  int lin;

  /* Here the size of the matrix is already updated */

  if (add)  /* ADD */
  {
    /* copy the attributes of the moved cells, from base+count to num */
    /*   do it in reverse order to avoid overlapping */
    /* then clear the new space starting from base to base+count */

    for(lin = ih->data->lines.num-1; lin >= base+count; lin--)
      iMatrixListCopyLinAttributes(ih, lin-count, lin);

    for(lin = base; lin < base+count; lin++)
      iMatrixListClearLinAttributes(ih, lin);
  }
  else  /* DEL */
  {
    /* copy the attributes of the moved cells from base+count to base */
    /* then clear the remaining space starting at num */

    for(lin = base; lin < ih->data->lines.num; lin++)
      iMatrixListCopyLinAttributes(ih, lin+count, lin);

    for(lin = ih->data->lines.num; lin < ih->data->lines.num+count; lin++)
      iMatrixListClearLinAttributes(ih, lin);
  }
}

static void iMatrixListAddLineAttributes(Ihandle* ih, int base, int count)
{
  int lin;
  for(lin = base; lin < base+count; lin++)
  {
    /* all bottom horizontal lines transparent, except title and the last one */
    if (lin!=0 && lin!=ih->data->lines.num-1)
      IupSetAttributeId2(ih, "FRAMEHORIZCOLOR", lin, IUP_INVALID_ID, "BGCOLOR");
  }
}

static void iMatrixListUpdateLastLineAttributes(Ihandle* ih, int lines_num)
{
  if (lines_num < ih->data->lines.num && lines_num > 1)
    IupSetAttributeId2(ih, "FRAMEHORIZCOLOR", lines_num-1, IUP_INVALID_ID, "BGCOLOR");
  IupSetAttributeId2(ih, "FRAMEHORIZCOLOR", ih->data->lines.num-1, IUP_INVALID_ID, NULL);
}

static void iMatrixListInitSize(Ihandle* ih, ImatrixListData* mtxList)
{
  char str[30];
  int num_col = 0;

  if (mtxList->label_col != 0)
    num_col++;
  if (mtxList->color_col != 0)
    num_col++;
  if (mtxList->image_col != 0)
    num_col++;
  
  sprintf(str, "%d", num_col);
  iupMatrixSetNumColAttrib(ih, str);  /* "NUMCOL" */
  IupSetStrAttribute(ih, "NUMCOL_VISIBLE", str);

  if (mtxList->color_col != 0)
  {
    if (!iupAttribGetId(ih, "WIDTH", mtxList->color_col))
      IupSetIntId(ih, "WIDTH", mtxList->color_col, IMTXL_COLOR_WIDTH);
  }

  if (mtxList->image_col != 0)
  {
    if (!iupAttribGetId(ih, "WIDTH", mtxList->image_col))
      IupSetIntId(ih, "WIDTH", mtxList->image_col, IMTXL_IMAGE_WIDTH);
  }
}

static void iMatrixListInitializeAttributes(Ihandle* ih, ImatrixListData* mtxList)
{
  int num_lin, col;

  for(col = 1; col < ih->data->columns.num; col++)
  {
    /* all right vertical lines transparent, except the last one */
    if (col != ih->data->columns.num-1)
      IupSetAttributeId2(ih, "FRAMEVERTCOLOR", IUP_INVALID_ID, col, "BGCOLOR");
  }

  num_lin = ih->data->lines.num-1;  /* remove the title line count, even if not visible */
  if (mtxList->editable)
    IupSetInt(ih, "NUMLIN", num_lin+1);  /* reserve space for the empty line */

  /* Set the text alignment for the item column */
  if (mtxList->label_col)
    IupSetAttributeId(ih, "ALIGNMENT", mtxList->label_col, "ALEFT");
}

static const char* iMatrixListApplyInactiveLineColor(const char* color)
{
  unsigned char r=0, g=0, b=0;

  iupStrToRGB(color, &r, &g, &b);

  r = IMAT_LIGHTER(r);
  g = IMAT_LIGHTER(g);
  b = IMAT_LIGHTER(b);

  return iupStrReturnRGB(r, g, b);
}

static void iMatrixListUpdateItemBgColor(Ihandle* ih, int lin, const char* bgcolor, int itemactive)
{
  if (lin==ih->data->lines.focus_cell)
  {
    char* focuscolor = IupGetAttribute(ih, "FOCUSCOLOR");
    if (!iupStrEqualNoCase(focuscolor, "BGCOLOR"))
      bgcolor = focuscolor;
  }

  if (!bgcolor)
    bgcolor = IupGetAttribute(ih, "BGCOLOR");

  if (!itemactive)
    bgcolor = iMatrixListApplyInactiveLineColor(bgcolor);

  IupSetStrAttributeId2(ih, "BGCOLOR", lin, IUP_INVALID_ID, bgcolor);
}

static void iMatrixListUpdateItemFgColor(Ihandle* ih, int lin, const char* fgcolor, int itemactive)
{
  if (lin == ih->data->lines.focus_cell)
  {
    char* focuscolor = IupGetAttribute(ih, "FOCUSFGCOLOR");
    if (focuscolor)
      fgcolor = focuscolor;
  }

  if (!fgcolor)
    fgcolor = IupGetAttribute(ih, "FGCOLOR");

  if (!itemactive)
    fgcolor = iMatrixListApplyInactiveLineColor(fgcolor);

  IupSetStrAttributeId2(ih, "FGCOLOR", lin, IUP_INVALID_ID, fgcolor);
}

static void iMatrixListSetFocusItem(Ihandle* ih, ImatrixListData* mtxList, int lin)
{
  if (lin != ih->data->lines.focus_cell)
  {
    int old_lin = ih->data->lines.focus_cell;
    int itemactive = IupGetIntId(ih, "ITEMACTIVE", old_lin);
    ih->data->lines.focus_cell = -1;
    iMatrixListUpdateItemBgColor(ih, old_lin, iupAttribGetId(ih, "ITEMBGCOLOR", old_lin), itemactive);
    IupSetfAttribute(ih, "REDRAW", "L%d", old_lin);

    itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
    ih->data->lines.focus_cell = lin;
    iMatrixListUpdateItemBgColor(ih, lin, iupAttribGetId(ih, "ITEMBGCOLOR", lin), itemactive);
    IupSetfAttribute(ih, "REDRAW", "L%d", lin);
  }

  if (mtxList->label_col)
    IupSetfAttribute(ih, "FOCUSCELL", "%d:%d", lin, mtxList->label_col);
  else
    IupSetfAttribute(ih, "FOCUSCELL", "%d:1", lin);
}

/******************************************************************************
 Attributes
******************************************************************************/

static char* iMatrixListGetEditableAttrib(Ihandle *ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  return iupStrReturnBoolean(mtxList->editable);
}

static int iMatrixListSetEditableAttrib(Ihandle* ih, const char* value)
{
  /* Creation attribute, Can set only before map */
  if (!ih->handle)
  {
    ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
    mtxList->editable = iupStrBoolean(value);
  }
  return 0;
}

static int iMatrixListSetAddLinAttrib(Ihandle* ih, const char* value)
{
  int base, count, lines_num = ih->data->lines.num;

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  if (!iupMatrixGetStartEnd(value, &base, &count, lines_num, 0))
    return 0;

  iupMatrixSetAddLinAttrib(ih, value);

  if (base < lines_num)  /* If before the last line. */
    iMatrixListUpdateLineAttributes(ih, base, count, 1);

  iMatrixListAddLineAttributes(ih, base, count);

  iMatrixListUpdateLastLineAttributes(ih, lines_num);

  IupSetAttribute(ih, "REDRAW", "ALL");

  return 0;
}

static int iMatrixListSetDelLinAttrib(Ihandle* ih, const char* value)
{
  int base, count, lines_num = ih->data->lines.num;
  ImatrixListData* mtxList;

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  if (!iupMatrixGetStartEnd(value, &base, &count, lines_num, 1))
    return 0;

  iupMatrixSetDelLinAttrib(ih, value);

  if (base < lines_num)  /* If before the last line. (always true when deleting) */
    iMatrixListUpdateLineAttributes(ih, base, count, 0);

  iMatrixListUpdateLastLineAttributes(ih, lines_num);

  lines_num = ih->data->lines.num;
  mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  if (mtxList->editable && lines_num==1)
    IupSetInt(ih, "NUMLIN", 1);  /* reserve space for the empty line */

  IupSetAttribute(ih, "REDRAW", "ALL");

  return 0;
}

static int iMatrixListSetNumLinAttrib(Ihandle* ih, const char* value)
{
  int lines_num = ih->data->lines.num;

  iupMatrixSetNumLinAttrib(ih, value);

  if (lines_num < ih->data->lines.num)  /* lines were added */
    iMatrixListAddLineAttributes(ih, lines_num, ih->data->lines.num-lines_num);

  iMatrixListUpdateLastLineAttributes(ih, lines_num);

  IupSetAttribute(ih, "REDRAW", "ALL");

  return 0;
}

static int iMatrixListSetCountAttrib(Ihandle* ih, const char* value)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  if (mtxList->editable)
  {
    int count;
    if (iupStrToInt(value, &count))
    {
      char str[50];
      sprintf(str, "%d", count+1);
      iMatrixListSetNumLinAttrib(ih, str);
    }
  }
  else
    iMatrixListSetNumLinAttrib(ih, value);
  return 0;
}

static char* iMatrixListGetColumnOrderAttrib(Ihandle *ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  char* str = iupStrGetMemory(30);

  if (mtxList->label_col == 1)
    strcat(str, "LABEL");
  else if (mtxList->color_col == 1)
    strcat(str, "COLOR");
  else if (mtxList->image_col == 1)
    strcat(str, "IMAGE");

  if (mtxList->label_col == 2)
    strcat(str, ":LABEL");
  else if (mtxList->color_col == 2)
    strcat(str, ":COLOR");
  else if (mtxList->image_col == 2)
    strcat(str, ":IMAGE");
  else
    return str;

  if (mtxList->label_col == 3)
    strcat(str, ":LABEL");
  else if (mtxList->color_col == 3)
    strcat(str, ":COLOR");
  else if (mtxList->image_col == 3)
    strcat(str, ":IMAGE");

  return str;
}

static int iMatrixListSetColumnOrderAttrib(Ihandle *ih, const char* value)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  char value1[30], value2[30], value3[30];
  int ret;

  /* valid only before map */
  if (ih->handle)
    return 0;

  ret = iupStrToStrStr(value, value1, value2, ':');
  if (ret == 0)
    return 0;

  if (iupStrEqualNoCase(value1, "IMAGE"))
  {
    mtxList->image_col = 1;
    mtxList->label_col = 0;
    mtxList->color_col = 0;
  }
  else if (iupStrEqualNoCase(value1, "COLOR"))
  {
    mtxList->color_col = 1;
    mtxList->label_col = 0;
    mtxList->image_col = 0;
  }
  else if (iupStrEqualNoCase(value1, "LABEL"))
  {
    mtxList->label_col = 1;
    mtxList->color_col = 0;
    mtxList->image_col = 0;
  }
  else
    return 0; /* must have a first column */

  if (ret==1)
  {
    if (!ih->handle)
      iMatrixListInitSize(ih, mtxList);
    return 0;
  }
  
  strcpy(value1, value2);
  ret = iupStrToStrStr(value1, value2, value3, ':');
  if (ret == 0)
    return 0;

  if (iupStrEqualNoCase(value2, "IMAGE"))
  {
    if (mtxList->image_col == 0)   /* don't allow repeated columns */
      mtxList->image_col = 2;
  }
  else if (iupStrEqualNoCase(value2, "COLOR"))
  {
    if (mtxList->color_col == 0)
      mtxList->color_col = 2;
  }
  else if (iupStrEqualNoCase(value2, "LABEL"))
  {
    if (mtxList->label_col == 0)
      mtxList->label_col = 2;
  }

  if (ret==1)
  {
    if (!ih->handle)
      iMatrixListInitSize(ih, mtxList);
    return 0;
  }

  if (mtxList->image_col != 2 &&
      mtxList->color_col != 2 &&
      mtxList->label_col != 2)
    return 0;  /* must have the second to allow the third */

  if (iupStrEqualNoCase(value3, "IMAGE"))
  {
    if (mtxList->image_col == 0)   /* don't allow repeated columns */
      mtxList->image_col = 3;
  }
  else if (iupStrEqualNoCase(value3, "COLOR"))
  {
    if (mtxList->color_col == 0)
      mtxList->color_col = 3;
  }
  else if (iupStrEqualNoCase(value3, "LABEL"))
  {
    if (mtxList->label_col == 0)
      mtxList->label_col = 3;
  }

  if (!ih->handle)
    iMatrixListInitSize(ih, mtxList);
  return 0;
}

static char* iMatrixListGetImageColAttrib(Ihandle *ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  return iupStrReturnInt(mtxList->image_col);
}

static char* iMatrixListGetColorColAttrib(Ihandle *ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  return iupStrReturnInt(mtxList->color_col);
}

static char* iMatrixListGetLabelColAttrib(Ihandle *ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  return iupStrReturnInt(mtxList->label_col);
}

static char* iMatrixListGetImageActiveAttrib(Ihandle* ih, int lin)
{
  char* value = iupAttribGetId(ih, "IMAGEACTIVE", lin);
  if (!value)
    return "Yes"; /* default is Yes for all lines */
  else
    return value;
}

static char* iMatrixListGetItemActiveAttrib(Ihandle* ih, int lin)
{
  char* value = iupAttribGetId(ih, "ITEMACTIVE", lin);
  if (!value)
    return "Yes"; /* default is Yes for all lines */
  else
    return value;
}

static int iMatrixListSetItemActiveAttrib(Ihandle* ih, int lin, const char* value)
{
  int itemactive = iupStrBoolean(value);
  iMatrixListUpdateItemBgColor(ih, lin, iupAttribGetId(ih, "ITEMBGCOLOR", lin), itemactive);
  iMatrixListUpdateItemFgColor(ih, lin, iupAttribGetId(ih, "ITEMFGCOLOR", lin), itemactive);
  return 1;
}

static int iMatrixListSetItemBgColorAttrib(Ihandle* ih, int lin, const char* value)
{
  int itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  iMatrixListUpdateItemBgColor(ih, lin, value, itemactive);
  return 1;
}

static int iMatrixListSetItemFgColorAttrib(Ihandle* ih, int lin, const char* value)
{
  int itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  iMatrixListUpdateItemFgColor(ih, lin, value, itemactive);
  return 1;
}

static int iMatrixListSetFocusColorAttrib(Ihandle* ih, const char* value)
{
  int lin = ih->data->lines.focus_cell;
  int itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  iupAttribSetStr(ih, "FOCUSCOLOR", value);
  iMatrixListUpdateItemBgColor(ih, lin, iupAttribGetId(ih, "ITEMBGCOLOR", lin), itemactive);
  return 1;
}

static int iMatrixListSetFocusFgColorAttrib(Ihandle* ih, const char* value)
{
  int lin = ih->data->lines.focus_cell;
  int itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  iupAttribSetStr(ih, "FOCUSFGCOLOR", value);
  iMatrixListUpdateItemFgColor(ih, lin, iupAttribGetId(ih, "ITEMFGCOLOR", lin), itemactive);
  return 1;
}

static int iMatrixListSetTitleAttrib(Ihandle* ih, const char* value)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  if (!ih->handle)
    iupAttribSetStrId2(ih, "", 0, mtxList->label_col, value);
  else
  {
    iupMatrixSetValue(ih, 0, mtxList->label_col, value, 0);
    IupSetfAttribute(ih, "REDRAW", "L%d", 0);
  }
  return 0;
}

static char* iMatrixListGetTitleAttrib(Ihandle* ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  if (!ih->handle)
    return iupAttribGetId2(ih, "", 0, mtxList->label_col);
  else
    return iupMatrixGetValue(ih, 0, mtxList->label_col);
}

static int iMatrixListSetIdValueAttrib(Ihandle* ih, int lin, const char* value)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");

  if (iupMatrixCheckCellPos(ih, lin, mtxList->label_col))
  {
    iupMatrixSetValue(ih, lin, mtxList->label_col, value, 0);
    IupSetfAttribute(ih, "REDRAW", "L%d", lin);
  }
  return 0;
}

static char* iMatrixListGetIdValueAttrib(Ihandle* ih, int lin)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");

  if (iupMatrixCheckCellPos(ih, lin, mtxList->label_col))
    return iupMatrixGetValue(ih, lin, mtxList->label_col);
  return NULL;
}

static int iMatrixListSetValueAttrib(Ihandle* ih, const char* value)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");

  if (!mtxList->label_col)
    return 0;

  if (ih->data->columns.num <= 1 || ih->data->lines.num <= 1)
    return 0;

  if (ih->data->editing)
    IupStoreAttribute(ih->data->datah, "VALUE", value);
  else
  {
    iupMatrixSetValue(ih, ih->data->lines.focus_cell, mtxList->label_col, value, 0);
    IupSetfAttribute(ih, "REDRAW", "L%d", ih->data->lines.focus_cell);
  }
  return 0;
}

static char* iMatrixListGetValueAttrib(Ihandle* ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");

  if (!mtxList->label_col)
    return NULL;

  if (ih->data->columns.num <= 1 || ih->data->lines.num <= 1)
    return NULL;

  if (ih->data->editing)
    return iupMatrixEditGetValue(ih);
  else 
    return iupMatrixGetValue(ih, ih->data->lines.focus_cell, mtxList->label_col);
}

static int iMatrixListSetAppendItemAttrib(Ihandle* ih, const char* value)
{
  char str[50];
  ImatrixListData* mtxList;
  int num_lin = ih->data->lines.num-1;  /* add after the last line */

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  if (mtxList->editable)
    num_lin--;

  sprintf(str, "%d", num_lin);
  iMatrixListSetAddLinAttrib(ih, str);  /* after this id */
  iMatrixListSetIdValueAttrib(ih, num_lin+1, value);
  return 0;
}

static int iMatrixListSetInsertItemAttrib(Ihandle* ih, int lin, const char* value)
{
  char str[50];
  ImatrixListData* mtxList;
  lin--; /* insert before given line, starting at 1 */

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  if (mtxList->editable && lin == ih->data->lines.num-1)
    lin--;

  sprintf(str, "%d", lin);
  iMatrixListSetAddLinAttrib(ih, str);  /* after this id */
  iMatrixListSetIdValueAttrib(ih, lin+1, value);
  return 0;
}

static int iMatrixListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupSetfAttribute(ih, "SHOW", "%s:*", value);
  return 0;
}

static int iMatrixListSetFocusItemAttrib(Ihandle* ih, const char* value)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  int lin;
  if (iupStrToInt(value, &lin))
    iMatrixListSetFocusItem(ih, mtxList, lin);
  return 0;
}

static char* iMatrixListGetFocusItemAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->lines.focus_cell);
}

static int iMatrixListSetVisibleLinesAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "NUMLIN_VISIBLE", value);
  return 1;
}

static int iMatrixListSetAddColAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  /* does nothing */
  return 0;
}

static int iMatrixListSetDelColAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  /* does nothing */
  return 0;
}

static char* iMatrixListGetCountAttrib(Ihandle* ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  if (mtxList->editable)
    return iupStrReturnInt(ih->data->lines.num-2);
  else
    return iupStrReturnInt(ih->data->lines.num-1);  /* the attribute does not include the title */
}

static char* iMatrixListGetNumLinAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->lines.num-1);  /* the attribute does not include the title */
}

static char* iMatrixListGetNumColAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->columns.num-1);  /* the attribute does not include the title */
}

static int iMatrixListSetNumColAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  /* does nothing */
  return 0;
}

static int iMatrixListSetNumColNoScrollAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  /* does nothing */
  return 0;
}
static char* iMatrixListGetNumColNoScrollAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->columns.num_noscroll-1);  /* the attribute does not include the title */
}

static char* iMatrixListGetNumColVisibleAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->columns.last - ih->data->columns.first);
}

/******************************************************************************
         Callbacks
******************************************************************************/

static int iMatrixListDrawColorCol(Ihandle *ih, int lin, int x1, int x2, int y1, int y2)
{
  unsigned char red, green, blue;
  char* color = iupAttribGetId(ih, "COLOR", lin);

  if (iupStrToRGB(color, &red, &green, &blue))
  {
    static const int DX_BORDER = 2;
    static const int DY_BORDER = 3;
    static const int DX_FILL = 3;
    static const int DY_FILL = 4;
    int active = iupdrvIsActive(ih);
    int itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
    const char* framecolor_str;
    char color_str[32];

    if (!itemactive)
    {
      red = IMAT_LIGHTER(red);
      green = IMAT_LIGHTER(green);
      blue = IMAT_LIGHTER(blue);
    }

    if (!active)
    {
      unsigned char bg_r, bg_g, bg_b;
      iupStrToRGB(ih->data->bgcolor, &bg_r, &bg_g, &bg_b);
      iupImageColorMakeInactive(&red, &green, &blue, bg_r, bg_g, bg_b);
    }

    /* Fill the box with the color */
    sprintf(color_str, "%d %d %d", red, green, blue);
    IupSetAttribute(ih, "DRAWCOLOR", color_str);
    IupSetAttribute(ih, "DRAWSTYLE", "FILL");
    IupDrawRectangle(ih, x1 + DX_FILL, y1 + DY_FILL, x2 - DX_FILL, y2 - DY_FILL);

    /* Draw the border */
    framecolor_str = iupAttribGetStr(ih, "FRAMECOLOR");
    IupSetAttribute(ih, "DRAWCOLOR", framecolor_str);
    IupSetAttribute(ih, "DRAWSTYLE", "STROKE");
    IupDrawRectangle(ih, x1 + DX_BORDER, y1 + DY_BORDER, x2 - DX_BORDER, y2 - DY_BORDER);
  }

  return IUP_DEFAULT;  /* draw nothing more */
}

static int iMatrixListDrawImageCol(Ihandle *ih, ImatrixListData* mtxList, int lin, int col, int x1, int x2, int y1, int y2)
{
  char* image_name;
  int make_inactive = 0, itemactive, imageactive, imagevalue, showdelete,
      active = iupdrvIsActive(ih), linedelete;
  int lines_num = ih->data->lines.num;
  Ihandle* image;

  itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  imageactive = IupGetIntId(ih, "IMAGEACTIVE", lin);
  imagevalue = IupGetIntId(ih, "IMAGEVALUE", lin);
  showdelete = IupGetInt(ih, "SHOWDELETE");
  linedelete = IupGetIntId(ih, "LINEDELETE", lin);

  if (!active || !itemactive || !imageactive)
    make_inactive = 1;

  image_name = iupAttribGetId(ih, "IMAGE", lin);
  if (!image_name)
  {
    char* attrib_name;
    if (mtxList->editable)
    {
      if (lin == lines_num-1)
        attrib_name = "IMAGEADD";
      else
      {
        if (showdelete || linedelete)
          attrib_name = "IMAGEDEL";
        else
        {
          if (imagevalue)
            attrib_name = "IMAGECHECK";
          else
            attrib_name = "IMAGEUNCHECK";
        }
      }
    }
    else
    {
      if (imagevalue)
        attrib_name = "IMAGECHECK";
      else
        attrib_name = "IMAGEUNCHECK";
    }

    image_name = iupAttribGetStr(ih, attrib_name);  /* this will check for the default values also */
  }

  image = IupImageGetHandle(image_name);
  if (image)
  {
    int width  = IupGetInt(image, "WIDTH");
    int height = IupGetInt(image, "HEIGHT");
    const char* bgcolor_str = IupGetAttributeId2(ih, "CELLBGCOLOR", lin, col);

    /* Calc the image_name position - IupDraw uses top-down coordinates */
    int x = x2 - x1 - width;
    int y = y2 - y1 - height;
    x /= 2; x += x1;
    y /= 2; y += y1;

    /* Note: IupDrawImage doesn't support make_inactive or bgcolor like CD did */
    IupDrawImage(ih, image_name, x, y, -1, -1);
  }

  return IUP_DEFAULT;  /* draw nothing more */
}

static int iMatrixListDraw_CB(Ihandle *ih, int lin, int col, int x1, int x2, int y1, int y2)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  int lines_num = ih->data->lines.num;

  /* Just checking */
  if (lin < 0 || col <= 0)
    return IUP_IGNORE;  /* draw regular text */

  /* Don't draw on the empty line. */
  if (!mtxList->editable || (lin != lines_num-1) || (mtxList->image_col && col == mtxList->image_col))
  {
    IFniiiiii listdraw_cb = (IFniiiiii)IupGetCallback(ih, "LISTDRAW_CB");

    /* call application callback before anything */
    if (listdraw_cb && listdraw_cb(ih, lin, col, x1, x2, y1, y2)==IUP_DEFAULT)
      return IUP_DEFAULT;  /* draw nothing more */

    if (mtxList->label_col && col == mtxList->label_col)
      return IUP_IGNORE;  /* draw regular text */

    if (mtxList->color_col && col == mtxList->color_col)
      return iMatrixListDrawColorCol(ih, lin, x1, x2, y1, y2);

    if (mtxList->image_col && col == mtxList->image_col)
      return iMatrixListDrawImageCol(ih, mtxList, lin, col, x1, x2, y1, y2);
  }

  return IUP_DEFAULT;  /* draw nothing more */
}

static int iMatrixListEdition_CB(Ihandle *ih, int lin, int col, int mode, int update)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  int lines_num = ih->data->lines.num;
  IFniiii listedition_cb = (IFniiii)IupGetCallback(ih, "LISTEDITION_CB");

  /* allow editing only at the label column */
  if (col != mtxList->label_col)
    return IUP_IGNORE;

  /* allow editing only if active */
  if (!IupGetIntId2(ih, "ITEMACTIVE", lin, col))
    return IUP_IGNORE;

  /* call application callback before anything */
  if (listedition_cb && listedition_cb(ih, lin, col, mode, update)==IUP_IGNORE)
    return IUP_IGNORE;

  if (mode==1 && mtxList->image_col)
  {
    if (!IupGetInt(ih, "SHOWDELETE"))
      IupSetAttributeId(ih, "LINEDELETE", lin, "Yes");
    IupSetfAttribute(ih, "REDRAW", "C%d", mtxList->image_col);
  }

  /* adding a new line */
  if (mtxList->editable && lin == lines_num-1 && mode == 0)
  {
    /* clear any edition if not updating */
    if (update==0)
    {
      IupSetAttribute(ih, "VALUE", "");
      IupSetfAttribute(ih, "REDRAW", "L%d", lin);
    }
    else
    {
      /* check if entered a non empty value */
      char* value = IupGetAttribute(ih, "VALUE");
      if (value && value[0]!=0)
      {
        IFni listinsert_cb = (IFni)IupGetCallback(ih, "LISTINSERT_CB");
        /* notify the application that a line will be inserted */
        if (listinsert_cb && listinsert_cb(ih, lin) == IUP_IGNORE)
        {
          IupSetAttribute(ih, "VALUE", "");
          IupSetfAttribute(ih, "REDRAW", "L%d", lin);
        }
        else
        {
          /* Add a new empty line */
          IupSetInt(ih, "ADDLIN", lin);
        }
      }
    }
  }

  if (mode==0) 
  {
    if (!IupGetInt(ih, "SHOWDELETE"))
    {
      /* turn off drawing, but prepare for delete */
      if (update && ih->data->edit_hidden_byfocus)
        iupAttribSetInt(ih, "_IUPMTXLIST_DELETE", (int)clock());

      IupSetAttributeId(ih, "LINEDELETE", lin, NULL);
    }

    IupSetfAttribute(ih, "REDRAW", "C%d", mtxList->image_col);
  }

  return IUP_DEFAULT;
}

static int iMatrixListClick_CB(Ihandle *ih, int lin, int col, char *status)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  IFniis listclick_cb = (IFniis)IupGetCallback(ih, "LISTCLICK_CB");

  /* call application callback before anything */
  if (listclick_cb && listclick_cb(ih, lin, col, status)==IUP_IGNORE)
    return IUP_IGNORE;

  mtxList->last_click_lin = lin;
  mtxList->last_click_col = col;

  return IUP_DEFAULT;
}

static int iMatrixListCheckDelete(Ihandle *ih)
{
  if (IupGetInt(ih, "SHOWDELETE"))
    return 1;
  else
  {
    char* value = iupAttribGet(ih, "_IUPMTXLIST_DELETE");
    if (value)
    {
      int t, diff;
      iupStrToInt(value, &t);
      diff = (int)clock() - t;

      iupAttribSet(ih, "_IUPMTXLIST_DELETE", NULL);

      if (diff < 200)
        return 1;
    }
    return 0;
  }
}

static int iMatrixListRelease_CB(Ihandle *ih, int lin, int col, char *status)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  IFniis listrelease_cb = (IFniis)IupGetCallback(ih, "LISTRELEASE_CB");
  int lines_num = ih->data->lines.num;
  int itemactive, imageactive;

  /* call application callback before anything */
  if (listrelease_cb && listrelease_cb(ih, lin, col, status)==IUP_IGNORE)
    return IUP_IGNORE;

  if (mtxList->last_click_lin != lin ||
      mtxList->last_click_col != col)
    return IUP_DEFAULT;

  /* only the image column must be processed */
  if (col != mtxList->image_col)
    return IUP_DEFAULT;

  itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  imageactive = IupGetIntId(ih, "IMAGEACTIVE", lin);

  if (!itemactive || !imageactive)
    return IUP_DEFAULT;

  if (mtxList->editable && lin == lines_num-1)
  {
    /* click on IMAGEADD - start editing */
    iMatrixListSetFocusItem(ih, mtxList, lin);
    IupSetAttribute(ih, "EDITMODE", "Yes");
  }
  else if (iMatrixListCheckDelete(ih))
  {
    /* click on IMAGEDEL */
    IFni listremove_cb = (IFni)IupGetCallback(ih, "LISTREMOVE_CB");
    if (lin == 0)
    {
      if (mtxList->editable)
        lines_num--;

      for (lin = lines_num-1; lin>0; lin--)
      {
        itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
        imageactive = IupGetIntId(ih, "IMAGEACTIVE", lin);

        if (!itemactive || !imageactive)
          continue;

        if (!listremove_cb || listremove_cb(ih, lin) != IUP_IGNORE)
          IupSetInt(ih, "DELLIN", lin);
      }
    }
    else
    {
      /* notify the application that a line will be removed */
      if (!listremove_cb || listremove_cb(ih, lin) != IUP_IGNORE)
      {
        /* Remove the line */
        IupSetInt(ih, "DELLIN", lin);
      }
    }
  }
  else
  {
    /* click on IMAGECHECK/IMAGEUNCHECK */
    IFnii imagevaluechanged_cb = (IFnii)IupGetCallback(ih, "IMAGEVALUECHANGED_CB");
    int imagevalue = !IupGetIntId(ih, "IMAGEVALUE", lin);
    if (!imagevaluechanged_cb || imagevaluechanged_cb(ih, lin, imagevalue) != IUP_IGNORE)
    {
      IupSetIntId(ih, "IMAGEVALUE", lin, imagevalue);
      IupSetfAttribute(ih, "REDRAW", "L%d", lin);

      if (lin==0)
      {
        if (mtxList->editable) 
          lines_num--;

        for (lin=1; lin<lines_num; lin++)
        {
          itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
          imageactive = IupGetIntId(ih, "IMAGEACTIVE", lin);

          if (!itemactive || !imageactive)
            continue;

          if (!imagevaluechanged_cb || imagevaluechanged_cb(ih, lin, imagevalue) != IUP_IGNORE)
          {
            IupSetIntId(ih, "IMAGEVALUE", lin, imagevalue);
            IupSetfAttribute(ih, "REDRAW", "L%d", lin);
          }
        }
      }
    }
  }

  return IUP_DEFAULT;
}

static int iMatrixListEnterItem_CB(Ihandle *ih, int lin, int col)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "LISTACTION_CB");
  int itemactive;

  /* Don't update attributes during widget destruction */
  if (!ih->handle || !ih->data || !ih->data->lines.dt || lin < 0 || lin >= ih->data->lines.num)
    return IUP_DEFAULT;

  itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  iMatrixListUpdateItemBgColor(ih, lin, iupAttribGetId(ih, "ITEMBGCOLOR", lin), itemactive);
  IupSetfAttribute(ih, "REDRAW", "L%d", lin);
  if (cb) cb(ih, lin, 1);
  (void)col;
  return IUP_DEFAULT;
}

static int iMatrixListLeaveItem_CB(Ihandle *ih, int lin, int col)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "LISTACTION_CB");
  int itemactive;

  /* Don't update attributes during widget destruction */
  if (!ih->handle || !ih->data || !ih->data->lines.dt || lin < 0 || lin >= ih->data->lines.num)
    return IUP_DEFAULT;

  itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
  ih->data->lines.focus_cell = -1;
  iMatrixListUpdateItemBgColor(ih, lin, iupAttribGetId(ih, "ITEMBGCOLOR", lin), itemactive);
  ih->data->lines.focus_cell = lin;
  IupSetfAttribute(ih, "REDRAW", "L%d", lin);
  if (cb) cb(ih, lin, 0);
  (void)col;
  return IUP_DEFAULT;
}

static int iMatrixListKeyAny_CB(Ihandle *ih, int key)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");

  if (IupGetInt(ih, "EDITMODE"))
    return IUP_CONTINUE;

  if (key == K_SP || key == K_sSP)
  {
    if (mtxList->image_col != 0)
    {
      int lin = ih->data->lines.focus_cell;
      int itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
      int imageactive = IupGetIntId(ih, "IMAGEACTIVE", lin);
      int lines_num = ih->data->lines.num;

      if (!itemactive || !imageactive)
        return IUP_IGNORE;

      if (!mtxList->editable || lin != lines_num-1)
      {
        IFnii imagevaluechanged_cb = (IFnii)IupGetCallback(ih, "IMAGEVALUECHANGED_CB");
        int imagevalue = !IupGetIntId(ih, "IMAGEVALUE", lin);
        if (!imagevaluechanged_cb || imagevaluechanged_cb(ih, lin, imagevalue) != IUP_IGNORE)
        {
          IupSetIntId(ih, "IMAGEVALUE", lin, imagevalue);
          IupSetfAttribute(ih, "REDRAW", "L%d", lin);
          return IUP_IGNORE;
        }
      }

      return IUP_IGNORE;
    }
  }
  else if (key == K_HOME || key == K_sHOME)
  {
    iMatrixListSetFocusItem(ih, mtxList, 1);
    IupSetAttribute(ih, "SHOW", "1:*");
    return IUP_IGNORE;
  }
  else if (key == K_END || key == K_sEND)
  {
    iMatrixListSetFocusItem(ih, mtxList, ih->data->lines.num-1);
    IupSetfAttribute(ih, "SHOW", "%d:*", ih->data->lines.num-1);
    return IUP_IGNORE;
  }
  else if (key == K_DEL || key == K_sDEL)
  {
    if (mtxList->editable)
    {
      IFni listremove_cb = (IFni)IupGetCallback(ih, "LISTREMOVE_CB");
      /* notify the application that a line will be removed */
      int lin = ih->data->lines.focus_cell;
      int itemactive = IupGetIntId(ih, "ITEMACTIVE", lin);
      if (itemactive && (!listremove_cb || listremove_cb(ih, lin) != IUP_IGNORE))
      {
        /* Remove the line */
        IupSetInt(ih, "DELLIN", lin);
        return IUP_IGNORE;
      }
    }
  }
  else if (key == K_F2 || key == K_CR || key == K_sCR)
  {
    int lin = ih->data->lines.focus_cell;
    iMatrixListSetFocusItem(ih, mtxList, lin);  /* this will position focus at the right cell */
    IupSetAttribute(ih, "EDITMODE", "Yes");
    return IUP_IGNORE;
  }
  else
  {
    /* if a valid character is pressed enter edition mode */
    if (iupMatrixIsCharacter(key))
    {
      int lin = ih->data->lines.focus_cell;
      iMatrixListSetFocusItem(ih, mtxList, lin);  /* this will position focus at the right cell */

      IupSetAttribute(ih, "EDITMODE", "Yes");
      if (IupGetInt(ih, "EDITMODE"))
      {
        char value[2] = {0,0};
        value[0] = (char)key;
        IupStoreAttribute(ih->data->datah, "VALUEMASKED", value);
        IupSetAttribute(ih->data->datah, "CARET", "2");
        return IUP_IGNORE;
      }
    }
  }

  return IUP_CONTINUE;
}

/******************************************************************************
 Methods
******************************************************************************/

static void iMatrixListDestroyMethod(Ihandle* ih)
{
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  free(mtxList);
}

static int iMatrixListMapMethod(Ihandle* ih)
{
  /* defining default attributes */
  ImatrixListData* mtxList = (ImatrixListData*)iupAttribGet(ih, "_IUPMTXLIST_DATA");
  iMatrixListInitializeAttributes(ih, mtxList);
  return IUP_NOERROR;
}

static int iMatrixListCreateMethod(Ihandle* ih, void **params)
{
  ImatrixListData* mtxList = (ImatrixListData*)calloc(1, sizeof(ImatrixListData));
  iupAttribSet(ih, "_IUPMTXLIST_DATA", (char*)mtxList);

  /* default matrix list values */
  mtxList->label_col = 1;
  iMatrixListInitSize(ih, mtxList);

  /* change the IupCanvas default values */
  IupSetAttribute(ih, "EXPAND", "NO");

  /* Change the IupMatrix default values */
  iupAttribSet(ih, "HIDEFOCUS", "YES");  /* Hide the matrix focus feedback, but cell focus will still be processed internally */
  iupAttribSet(ih, "SCROLLBAR", "VERTICAL");
  iupAttribSet(ih, "CURSOR", "ARROW");
  iupAttribSet(ih, "ALIGNMENTLIN0", "ALEFT");
  iupAttribSet(ih, "FRAMETITLEHIGHLIGHT", "No");

  /* iMatrix callbacks */
  IupSetCallback(ih, "DRAW_CB",  (Icallback)iMatrixListDraw_CB);
  IupSetCallback(ih, "CLICK_CB", (Icallback)iMatrixListClick_CB);
  IupSetCallback(ih, "RELEASE_CB", (Icallback)iMatrixListRelease_CB);
  IupSetCallback(ih, "EDITION_CB",   (Icallback)iMatrixListEdition_CB);
  IupSetCallback(ih, "LEAVEITEM_CB", (Icallback)iMatrixListLeaveItem_CB);
  IupSetCallback(ih, "ENTERITEM_CB", (Icallback)iMatrixListEnterItem_CB);
  IupSetCallback(ih, "K_ANY", (Icallback)iMatrixListKeyAny_CB);

  (void)params;
  return IUP_NOERROR;
}

Iclass* iupMatrixListNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("matrix"));
  
  ic->name = "matrixlist";
  ic->cons = "MatrixList";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id  = 2;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New    = iupMatrixListNewClass;
  ic->Create = iMatrixListCreateMethod;
  ic->Destroy = iMatrixListDestroyMethod;
  ic->Map = iMatrixListMapMethod;

  /* IupMatrixList Callbacks */
  iupClassRegisterCallback(ic, "IMAGEVALUECHANGED_CB", "ii");
  iupClassRegisterCallback(ic, "LISTCLICK_CB", "iis");
  iupClassRegisterCallback(ic, "LISTRELEASE_CB", "iis");
  iupClassRegisterCallback(ic, "LISTINSERT_CB", "i");
  iupClassRegisterCallback(ic, "LISTREMOVE_CB", "i");
  iupClassRegisterCallback(ic, "LISTEDITION_CB", "iiii");
  iupClassRegisterCallback(ic, "LISTDRAW_CB", "iiiiiiC");
  iupClassRegisterCallback(ic, "LISTACTION_CB", "ii");

  iupClassRegisterReplaceAttribDef(ic, "CURSOR", IUPAF_SAMEASSYSTEM, "ARROW");

  /* IupMatrixList Attributes */

  /* IMPORTANT: this two will hide the IupMatrix VALUE and L:C attributes */
  iupClassRegisterAttributeId(ic, "IDVALUE", iMatrixListGetIdValueAttrib, iMatrixListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iMatrixListGetValueAttrib, iMatrixListSetValueAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", iMatrixListGetTitleAttrib, iMatrixListSetTitleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COLUMNORDER", iMatrixListGetColumnOrderAttrib, iMatrixListSetColumnOrderAttrib, IUPAF_SAMEASSYSTEM, "LABEL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COLORCOL", iMatrixListGetColorColAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGECOL", iMatrixListGetImageColAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LABELCOL", iMatrixListGetLabelColAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* NO redraw */
  iupClassRegisterAttributeId(ic, "ITEMACTIVE", iMatrixListGetItemActiveAttrib, iMatrixListSetItemActiveAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMFGCOLOR", NULL, iMatrixListSetItemFgColorAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMBGCOLOR", NULL, iMatrixListSetItemBgColorAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEACTIVE", iMatrixListGetImageActiveAttrib, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEVALUE", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LINEDELETE", NULL, NULL, IUPAF_NO_INHERIT);

  /* NO redraw */
  iupClassRegisterAttribute(ic, "IMAGEUNCHECK", NULL, NULL, IUPAF_SAMEASSYSTEM, "MTXLIST_IMG_UNCHECK", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGECHECK",   NULL, NULL, IUPAF_SAMEASSYSTEM, "MTXLIST_IMG_CHECK",   IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEDEL",     NULL, NULL, IUPAF_SAMEASSYSTEM, "MTXLIST_IMG_DEL",     IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEADD",     NULL, NULL, IUPAF_SAMEASSYSTEM, "MTXLIST_IMG_ADD",     IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSCOLOR",   NULL, iMatrixListSetFocusColorAttrib, IUPAF_SAMEASSYSTEM, "255 235 155", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSFGCOLOR", NULL, iMatrixListSetFocusFgColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE",   iMatrixListGetEditableAttrib, iMatrixListSetEditableAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDELETE",   NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "APPENDITEM", NULL, iMatrixListSetAppendItemAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);  /* allowing these methods to be called before map will avoid its storage in the hash table */
  iupClassRegisterAttributeId(ic, "INSERTITEM", NULL, iMatrixListSetInsertItemAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);  /* allowing these methods to be called before map will avoid its storage in the hash table */
  iupClassRegisterAttribute(ic, "ADDLIN", NULL, iMatrixListSetAddLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);  /* allowing these methods to be called before map will avoid its storage in the hash table */
  iupClassRegisterAttribute(ic, "DELLIN", NULL, iMatrixListSetDelLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEITEM", NULL, iMatrixListSetDelLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMLIN", iMatrixListGetNumLinAttrib, iMatrixListSetNumLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iMatrixListGetCountAttrib, iMatrixListSetCountAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TOPITEM", NULL, iMatrixListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSITEM", iMatrixListGetFocusItemAttrib, iMatrixListSetFocusItemAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT); /* can be NOT mapped */
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, iMatrixListSetVisibleLinesAttrib, IUPAF_SAMEASSYSTEM, "3", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Does nothing... this control defines automatically the number of columns to be used */
  iupClassRegisterAttribute(ic, "ADDCOL", NULL, iMatrixListSetAddColAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DELCOL", NULL, iMatrixListSetDelColAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL", iMatrixListGetNumColAttrib, iMatrixListSetNumColAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL_NOSCROLL", iMatrixListGetNumColNoScrollAttrib, iMatrixListSetNumColNoScrollAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL_VISIBLE",  iMatrixListGetNumColVisibleAttrib, NULL, IUPAF_SAMEASSYSTEM, "3", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* initialize default images */
  if (!IupGetHandle("MTXLIST_IMG_CHECK") || !IupGetHandle("MTXLIST_IMG_DEL") || !IupGetHandle("MTXLIST_IMG_ADD"))
    iMatrixListInitializeImages();

  return ic;
}

Ihandle* IupMatrixList(void)
{
  return IupCreate("matrixlist");
}

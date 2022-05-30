/** \file
 * \brief Text Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_layout.h"
#include "iup_mask.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_assert.h"


/* Used by List and Text, implemented in Text
   Can NOT use ih->data 
*/
int iupEditCallActionCb(Ihandle* ih, IFnis cb, const char* insert_value, int start, int end, void *mask, int nc, int remove_dir, int utf8)
{
  char *new_value, *value;
  int ret = -1, /* normal processing */
      key = 0;

  if (!cb && !mask)
    return ret;

  value = IupGetAttribute(ih, "VALUE");  /* it will return a non NULL internal buffer */

  if (!insert_value)
  {
    new_value = value;
    iupStrRemove(new_value, start, end, remove_dir, utf8);
  }
  else
  {
    if (value[0]==0)
      new_value = iupStrDup(insert_value);
    else
      new_value = iupStrInsert(value, insert_value, start, end, utf8);
  }

  if (insert_value && insert_value[0]!=0 && insert_value[1]==0)
    key = insert_value[0];

  if (!new_value)
    return ret;

  if (nc && (int)strlen(new_value) > nc)
  {
    if (new_value != value) free(new_value);
    return 0; /* abort */
  }

  if (mask && iupMaskCheck((Imask*)mask, new_value)==0)
  {
    IFns fail_cb = (IFns)IupGetCallback(ih, "MASKFAIL_CB");
    if (fail_cb) fail_cb(ih, new_value);
    if (new_value != value) free(new_value);
    return 0; /* abort */
  }

  if (cb)
  {
    int cb_ret = cb(ih, key, (char*)new_value);
    if (cb_ret==IUP_IGNORE)
      ret = 0; /* abort */
    else if (cb_ret==IUP_CLOSE)
    {
      IupExitLoop();
      ret = 0; /* abort */
    }
    else if (cb_ret!=0 && key!=0 && 
             cb_ret != IUP_DEFAULT && cb_ret != IUP_CONTINUE)  
      ret = cb_ret; /* replace key */
  }

  if (new_value != value) free(new_value);
  return ret;
}

char* iupTextGetFormattingAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->has_formatting); 
}

int iupTextSetFormattingAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)  /* only before map */
    return 0;

  ih->data->has_formatting = iupStrBoolean(value);

  return 0;
}

static void iTextDestroyFormatTags(Ihandle* ih)
{
  /* called if the element was destroyed before it was mapped */
  int i, count = iupArrayCount(ih->data->formattags);
  Ihandle** tag_array = (Ihandle**)iupArrayGetData(ih->data->formattags);
  for (i = 0; i < count; i++)
    IupDestroy(tag_array[i]);
  iupArrayDestroy(ih->data->formattags);
  ih->data->formattags = NULL;
}

static void iTextUpdateValueAttrib(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "VALUE");
  if (value)
  {
    iupAttribSetClassObject(ih, "VALUE", value);

    iupAttribSet(ih, "VALUE", NULL); /* clear hash table */
  }
}

char* iupTextGetNCAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->nc);
}

static void iTextAddFormatTag(Ihandle* ih, Ihandle* formattag)
{
  char* bulk = iupAttribGet(formattag, "BULK");
  if (bulk && iupStrBoolean(bulk))
  {
    Ihandle* child;
    void* state = iupdrvTextAddFormatTagStartBulk(ih);

    char* cleanout = iupAttribGet(formattag, "CLEANOUT");
    if (cleanout && iupStrBoolean(cleanout))
      IupSetAttribute(ih, "REMOVEFORMATTING", "ALL");

    for (child = formattag->firstchild; child; child = child->brother)
      iupdrvTextAddFormatTag(ih, child, 1);

    iupdrvTextAddFormatTagStopBulk(ih, state);
  }
  else
    iupdrvTextAddFormatTag(ih, formattag, 0);

  IupDestroy(formattag);
}

void iupTextUpdateFormatTags(Ihandle* ih)
{
  /* called when the element is mapped */
  int i, count = iupArrayCount(ih->data->formattags);
  Ihandle** tag_array = (Ihandle**)iupArrayGetData(ih->data->formattags);

  /* must update VALUE before updating the format */
  iTextUpdateValueAttrib(ih);

  for (i = 0; i < count; i++)
    iTextAddFormatTag(ih, tag_array[i]);

  iupArrayDestroy(ih->data->formattags);
  ih->data->formattags = NULL;
}

int iupTextSetAddFormatTagHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* formattag = (Ihandle*)value;
  if (!iupObjectCheck(formattag))
    return 0;

  if (ih->handle)
  {
    /* must update VALUE before updating the format */
    iTextUpdateValueAttrib(ih);

    iTextAddFormatTag(ih, formattag);
  }
  else
  {
    Ihandle** tag_array;
    int i;

    if (!ih->data->formattags)
      ih->data->formattags = iupArrayCreate(10, sizeof(Ihandle*));

    i = iupArrayCount(ih->data->formattags);
    tag_array = (Ihandle**)iupArrayInc(ih->data->formattags);
    tag_array[i] = formattag;
  }
  return 0;
}

int iupTextSetAddFormatTagAttrib(Ihandle* ih, const char* value)
{
  iupTextSetAddFormatTagHandleAttrib(ih, (char*)IupGetHandle(value));
  return 1;
}

static char* iTextGetMaskAttrib(Ihandle* ih)
{
  if (ih->data->mask)
    return iupMaskGetStr(ih->data->mask);
  else
    return NULL;
}

static int iTextSetValueMaskedAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    if (ih->data->mask && iupMaskCheck(ih->data->mask, value)==0)
      return 0; /* abort */
    IupStoreAttribute(ih, "VALUE", value);
  }
  return 0;
}

static int iTextSetMaskNoEmptyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mask)
  {
    int val = iupStrBoolean(value);
    iupMaskSetNoEmpty(ih->data->mask, val);
  }
  return 1;
}

static int iTextSetMaskCaseIAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mask)
  {
    int val = iupStrBoolean(value);
    iupMaskSetCaseI(ih->data->mask, val);
  }
  return 1;
}

static int iTextSetMaskAttrib(Ihandle* ih, const char* value)
{
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

static int iTextSetMaskIntAttrib(Ihandle* ih, const char* value)
{
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

    mask = iupMaskCreateInt(min,max);
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

static int iTextSetMaskFloatAttrib(Ihandle* ih, const char* value)
{
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

static int iTextSetMaskRealAttrib(Ihandle* ih, const char* value)
{
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

static int iTextSetChangeCaseAttrib(Ihandle* ih, const char* value)
{
  int case_flag = -1;

  if (iupStrEqualNoCase(value, "UPPER"))
    case_flag = IUP_CASE_UPPER;
  else if (iupStrEqualNoCase(value, "LOWER"))
    case_flag = IUP_CASE_LOWER;
  else if (iupStrEqualNoCase(value, "TOGGLE"))
    case_flag = IUP_CASE_TOGGLE;
  else if (iupStrEqualNoCase(value, "TITLE"))
    case_flag = IUP_CASE_TITLE;

  if (case_flag != -1)
  {
    int utf8 = IupGetInt(NULL, "UTF8MODE");
    char* str = IupGetAttribute(ih, "VALUE");
    iupStrChangeCase(str, str, case_flag, utf8);
    IupSetStrAttribute(ih, "VALUE", str);
  }

  return 0;
}

static int iTextSetMultilineAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
  {
    ih->data->is_multiline = 1;
    ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;  /* reset SCROLLBAR to YES */
    iupAttribSet(ih, "_IUP_MULTILINE_TEXT", "1");
  }
  else
  {
    ih->data->is_multiline = 0;
    iupAttribSet(ih, "_IUP_MULTILINE_TEXT", NULL);
  }

  return 0;
}

static char* iTextGetMultilineAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->is_multiline); 
}

static int iTextSetAppendNewlineAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->append_newline = 1;
  else
    ih->data->append_newline = 0;
  return 0;
}

static char* iTextGetAppendNewlineAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->append_newline); 
}

static int iTextSetScrollbarAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle || !ih->data->is_multiline)
    return 0;

  if (!value)
    value = "YES";    /* default, if multiline, is YES */

  if (iupStrEqualNoCase(value, "YES"))
    ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;
  else if (iupStrEqualNoCase(value, "HORIZONTAL"))
    ih->data->sb = IUP_SB_HORIZ;
  else if (iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->sb = IUP_SB_VERT;
  else
    ih->data->sb = IUP_SB_NONE;

  return 0;
}

static char* iTextGetScrollbarAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline)
    return NULL;
  if (ih->data->sb == (IUP_SB_HORIZ | IUP_SB_VERT))
    return "YES";
  if (ih->data->sb & IUP_SB_HORIZ)
    return "HORIZONTAL";
  if (ih->data->sb & IUP_SB_VERT)
    return "VERTICAL";
  return "NO";   /* IUP_SB_NONE */
}

char* iupTextGetPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}


/********************************************************************/


static int iTextCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    if (params[0]) iupAttribSetStr(ih, "ACTION", (char*)(params[0]));
  }
  ih->data = iupALLOCCTRLDATA();
  ih->data->append_newline = 1;
  return IUP_NOERROR;
}

static int iMultilineCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  ih->data->is_multiline = 1;
  ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;  /* default is YES */
  iupAttribSet(ih, "_IUP_MULTILINE_TEXT", "1");
  return IUP_NOERROR;
}

static void iTextComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, 
      natural_h = 0,
      visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS"),
      visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  (void)children_expand; /* unset if not a container */

  /* Since the contents can be changed by the user, the size can not be dependent on it. */
  iupdrvFontGetCharSize(ih, NULL, &natural_h);  /* one line height */
  natural_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
  natural_w = (visiblecolumns*natural_w)/10;
  if (ih->data->is_multiline)
    natural_h = visiblelines*natural_h;

  /* compute the borders space */
  if (iupAttribGetBoolean(ih, "BORDER"))
    iupdrvTextAddBorders(ih, &natural_w, &natural_h);

  if (iupAttribGetBoolean(ih, "SPIN"))
    iupdrvTextAddSpin(ih, &natural_w, natural_h);

  natural_w += 2*ih->data->horiz_padding;
  natural_h += 2*ih->data->vert_padding;

  /* add scrollbar */
  if (ih->data->is_multiline && ih->data->sb)
  {
    int sb_size = iupdrvGetScrollbarSize();
    if (ih->data->sb & IUP_SB_HORIZ)
      natural_h += sb_size;  /* sb horizontal affects vertical size */
    if (ih->data->sb & IUP_SB_VERT)
      natural_w += sb_size;  /* sb vertical affects horizontal size */
  }

  *w = natural_w;
  *h = natural_h;
}

static void iTextDestroyMethod(Ihandle* ih)
{
  if (ih->data->formattags)
    iTextDestroyFormatTags(ih);
  if (ih->data->mask)
    iupMaskDestroy(ih->data->mask);
}


/******************************************************************************/

typedef void (*Iconvertlincol2pos)(Ihandle* ih, int lin, int col, int *pos);
typedef void (*Iconvertpos2lincol)(Ihandle* ih, int pos, int *lin, int *col);

IUP_API void IupTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!ih->handle)
    return;
    
  if (IupClassMatch(ih, "text"))
  {
    if (ih->data->is_multiline)
      iupdrvTextConvertLinColToPos(ih, lin, col, pos);
    else
      *pos = col - 1; /* IUP starts at 1 */
  }
  else 
  {
    Iconvertlincol2pos convert = (Iconvertlincol2pos)IupGetCallback(ih, "_IUP_LINCOL2POS_CB");
    if (convert)
      convert(ih, lin, col, pos);
  }
}

IUP_API void IupTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!ih->handle)
    return;
    
  if (IupClassMatch(ih, "text"))
  {
    if (ih->data->is_multiline)
      iupdrvTextConvertPosToLinCol(ih, pos, lin, col);
    else
    {
      *col = pos + 1; /* IUP starts at 1 */
      *lin = 1;
    }
  }
  else 
  {
    Iconvertpos2lincol convert = (Iconvertpos2lincol)IupGetCallback(ih, "_IUP_POS2LINCOL_CB");
    if (convert)
      convert(ih, pos, lin, col);
  }
}

IUP_API Ihandle* IupText(const char* action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("text", params);
}

Iclass* iupTextNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "text";
  ic->format = "a"; /* one ACTION callback name */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupTextNewClass;
  ic->Create = iTextCreateMethod;
  ic->Destroy = iTextDestroyMethod;
  ic->ComputeNaturalSize = iTextComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "CARET_CB", "iii");
  iupClassRegisterCallback(ic, "ACTION", "is");
  iupClassRegisterCallback(ic, "BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "SPIN_CB", "i");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Drag&Drop */
  iupdrvRegisterDragDropAttrib(ic);

  /* IupText only */
  iupClassRegisterAttribute(ic, "SCROLLBAR", iTextGetScrollbarAttrib, iTextSetScrollbarAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOHIDE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", iTextGetMultilineAttrib, iTextSetMultilineAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPENDNEWLINE", iTextGetAppendNewlineAttrib, iTextSetAppendNewlineAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CPADDING", iupBaseGetCPaddingAttrib, iupBaseSetCPaddingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "VALUEMASKED", NULL, iTextSetValueMaskedAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKCASEI", NULL, iTextSetMaskCaseIAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKDECIMALSYMBOL", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASK", iTextGetMaskAttrib, iTextSetMaskAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKINT", NULL, iTextSetMaskIntAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKFLOAT", NULL, iTextSetMaskFloatAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKREAL", NULL, iTextSetMaskRealAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKNOEMPTY", NULL, iTextSetMaskNoEmptyAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPIN", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINALIGN", NULL, NULL, IUPAF_SAMEASSYSTEM, "RIGHT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINAUTO", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINWRAP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CHANGECASE", NULL, iTextSetChangeCaseAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupdrvTextInitClass(ic);

  return ic;
}

IUP_API Ihandle* IupMultiLine(const char* action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("multiline", params);
}

Iclass* iupMultilineNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("text"));

  ic->name = "multiline";   /* register the multiline name, so LED will work */
  ic->cons = "MultiLine";
  ic->format = "a"; /* one ACTION callback name */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->Create = iMultilineCreateMethod;

  return ic;
}

/** \file
 * \brief iupmatrix setget control
 * attributes set and get
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "iup.h"
#include "iupcbs.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_drvfont.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_childtree.h"
#include "iup_image.h"
#include "iup_assert.h"

#include "iup_controls.h"

#include "iupmat_def.h"
#include "iupmat_getset.h"
#include "iupmat_edit.h"
#include "iupmat_draw.h"
#include "iupmat_aux.h"


int iupMatrixCheckCellPos(Ihandle* ih, int lin, int col)
{
  /* Check if the cell exists */
  if (!iupMATRIX_CHECK_COL(ih, col) ||
      !iupMATRIX_CHECK_LIN(ih, lin))
      return 0;

  return 1;
}

void iupMatrixModifyValue(Ihandle* ih, int lin, int col, const char* value)
{
  /* Called when modifying multiple cells in a sequence.
     It is useful to support READONLY
     and cell read-only by returning IUP_IGNORE.
     Here is the only place where both mode=1 and update=1.
     */
  if (iupMatrixAuxCallEditionCbLinCol(ih, lin, col, 1, 1) != IUP_IGNORE)
    iupMatrixSetValue(ih, lin, col, value, -1);    /* call value_edit_cb, but NO numeric conversion */
}

static char* iMatrixSetValueNumeric(Ihandle* ih, int lin, int col, const char* value, int convert)
{
  double number;
  int ret = iupStrToDoubleLocale(value, &number, IupGetAttribute(ih, "NUMERICDECIMALSYMBOL"));  /* this will also check for global "DEFAULTDECIMALSYMBOL" */
  if (ret)
  {
    IFniid setvalue_cb;

    if (convert && ih->data->numeric_columns[col].unit_shown != ih->data->numeric_columns[col].unit)
      number = ih->data->numeric_convert_func(number, ih->data->numeric_columns[col].quantity,
      ih->data->numeric_columns[col].unit_shown,  /* from */
      ih->data->numeric_columns[col].unit);       /* to */

    setvalue_cb = (IFniid)IupGetCallback(ih, "NUMERICSETVALUE_CB");
    if (setvalue_cb)
    {
      setvalue_cb(ih, lin, col, number);
      return NULL;
    }
    else if (ret == 2 || (convert && ih->data->numeric_columns[col].unit_shown != ih->data->numeric_columns[col].unit))
    {
      /* only use the number if locale was set or a conversion occurred */
      sprintf(ih->data->numeric_buffer_set, IUP_DOUBLE2STR, number);
      value = ih->data->numeric_buffer_set;
    }
  }

  return (char*)value;
}

void iupMatrixSetValue(Ihandle* ih, int lin, int col, const char* value, int user_edited)
{
  /* NOTICE: this function is NOT called before map */
  char* old_value = NULL;

  if (ih->data->undo_redo) old_value = iupMatrixGetValue(ih, lin, col);

  if (lin != 0 && ih->data->sort_has_index)
    lin = ih->data->sort_line_index[lin];

  if (ih->data->undo_redo) iupAttribSetClassObjectId2(ih, "UNDOPUSHCELL", lin, col, old_value);

  if (value && ih->data->numeric_columns && ih->data->numeric_columns[col].flags & IMAT_IS_NUMERIC)
  {
    value = iMatrixSetValueNumeric(ih, lin, col, value, user_edited == 1);  /* convert only if interactively edited */
    if (!value)
      return;
  }

  if (!ih->data->callback_mode)
  {
    if (ih->data->cells[lin][col].value)
      free(ih->data->cells[lin][col].value);

    ih->data->cells[lin][col].value = iupStrDup(value);
  }

  if (user_edited)
  {
    /* value_edit_cb called when value is "interactively" edited.
       This is NOT called when L:C or VALUE attributes are set.
       It works in normal mode and in callback mode. */
    IFniis value_edit_cb = (IFniis)IupGetCallback(ih, "VALUE_EDIT_CB");
    if (value_edit_cb)
      value_edit_cb(ih, lin, col, (char*)value);
  }

  ih->data->need_redraw = 1;
  if (lin == 0 || col == 0)
    ih->data->need_calcsize = 1;
}

static char* iMatrixGetValueText(Ihandle* ih, int lin, int col)
{
  char* value;

  if (lin != 0 && ih->data->sort_has_index)
    lin = ih->data->sort_line_index[lin];

  if (ih->data->callback_mode)
  {
    /* only called in callback mode */
    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
      value = value_cb(ih, lin, col);
    else
      value = NULL; /* application error!!!! value_cb must exist */
  }
  else
    value = ih->data->cells[lin][col].value;

  return value;
}

double iupMatrixGetValueNumeric(Ihandle* ih, int lin, int col)
{
  sIFniis translate_cb;
  double number;

  /* here lin!=0 */

  char* value = iMatrixGetValueText(ih, lin, col);

  translate_cb = (sIFniis)IupGetCallback(ih, "TRANSLATEVALUE_CB");
  if (translate_cb)
    value = translate_cb(ih, lin, col, value);

  if (!value)
  {
    dIFnii getvalue_cb = (dIFnii)IupGetCallback(ih, "NUMERICGETVALUE_CB");
    if (getvalue_cb)
      number = getvalue_cb(ih, lin, col);
    else
      return 0;
  }
  else
  {
    if (!iupStrToDouble(value, &number))
      return 0;
  }

  if (ih->data->numeric_columns[col].unit_shown != ih->data->numeric_columns[col].unit)
    number = ih->data->numeric_convert_func(number, ih->data->numeric_columns[col].quantity,
    ih->data->numeric_columns[col].unit, /* from */
    ih->data->numeric_columns[col].unit_shown);  /* to */

  return number;
}

char* iupMatrixGetValue(Ihandle* ih, int lin, int col)
{
  char* value = iMatrixGetValueText(ih, lin, col);

  /* no translation here */

  if (!value && lin != 0 && ih->data->numeric_columns && ih->data->numeric_columns[col].flags & IMAT_IS_NUMERIC)
  {
    dIFnii getvalue_cb = (dIFnii)IupGetCallback(ih, "NUMERICGETVALUE_CB");
    if (getvalue_cb)
    {
      /* no formatting and no conversion here */
      double number = getvalue_cb(ih, lin, col);
      sprintf(ih->data->numeric_buffer_get, IUP_DOUBLE2STR, number);  /* maximum precision */
      return ih->data->numeric_buffer_get;
    }
  }

  return value;
}

char* iupMatrixGetNumericFormatDef(Ihandle* ih)
{
  char* format = iupAttribGet(ih, "NUMERICFORMATDEF");
  if (!format)
  {
    int prec = IupGetInt(NULL, "DEFAULTPRECISION");
    if (prec == 2)
      format = "%.2f";
    else
    {
      static char f[30];
      sprintf(f, "%%.%df", prec);
      format = f;
    }
  }
  return format;
}

static char* iMatrixGetValueNumericTitle(Ihandle* ih, int col, const char* value)
{
  char *format = NULL;

  /* here lin==0 */

  if (ih->data->numeric_columns[col].flags & IMAT_HAS_FORMATTITLE)
    format = iupAttribGetId(ih, "NUMERICFORMATTITLE", col);

  if (format)
  {
    char* unit_symbol = IupGetAttributeId(ih, "NUMERICUNITSYMBOLSHOWN", col);
    if (unit_symbol)
    {
      if (value)
        sprintf(ih->data->numeric_buffer_get, format, value, unit_symbol);
      else
        sprintf(ih->data->numeric_buffer_get, format, unit_symbol);
      return ih->data->numeric_buffer_get;
    }
  }

  return (char*)value;
}

static char* iMatrixGetValueNumericFormatted(Ihandle* ih, int lin, int col, const char* value)
{
  char *format = NULL;
  double number;

  /* here lin!=0 */

  if (!value)
  {
    dIFnii getvalue_cb = (dIFnii)IupGetCallback(ih, "NUMERICGETVALUE_CB");
    if (getvalue_cb)
      number = getvalue_cb(ih, lin, col);
    else
      return NULL;
  }
  else
  {
    if (!iupStrToDouble(value, &number))
      return (char*)value;
  }

  if (ih->data->numeric_columns[col].unit_shown != ih->data->numeric_columns[col].unit)
    number = ih->data->numeric_convert_func(number, ih->data->numeric_columns[col].quantity,
    ih->data->numeric_columns[col].unit, /* from */
    ih->data->numeric_columns[col].unit_shown);  /* to */

  if (ih->data->numeric_columns[col].flags & IMAT_HAS_FORMAT)
    format = iupAttribGetId(ih, "NUMERICFORMAT", col);

  if (format == NULL)
    format = iupMatrixGetNumericFormatDef(ih);

  iupStrPrintfDoubleLocale(ih->data->numeric_buffer_get, format, number, IupGetAttribute(ih, "NUMERICDECIMALSYMBOL"));  /* this will also check for global "DEFAULTDECIMALSYMBOL" */
  return ih->data->numeric_buffer_get;
}

char* iupMatrixGetValueDisplay(Ihandle* ih, int lin, int col)
{
  /* NOTICE: this function CAN BE called before map */
  if (!ih->handle)
    return iupAttribGetId2(ih, "", lin, col);
  else
  {
    sIFniis translate_cb;

    char* value = iMatrixGetValueText(ih, lin, col);

    translate_cb = (sIFniis)IupGetCallback(ih, "TRANSLATEVALUE_CB");
    if (translate_cb)
      value = translate_cb(ih, lin, col, value);

    if (ih->data->numeric_columns && ih->data->numeric_columns[col].flags & IMAT_IS_NUMERIC)
    {
      if (lin == 0)
        return iMatrixGetValueNumericTitle(ih, col, value);
      else
        return iMatrixGetValueNumericFormatted(ih, lin, col, value);
    }
    else
      return value;
  }
}

void iupMatrixSetCellFlag(Ihandle* ih, int lin, int col, unsigned char attr, int set)
{
  if (!ih->handle)
    return;

  if (lin == IUP_INVALID_ID)
  {
    if (!iupMATRIX_CHECK_COL(ih, col))
      return;

    if (set)
      ih->data->columns.dt[col].flags |= attr;
    else
      ih->data->columns.dt[col].flags &= ~attr;
  }
  else if (col == IUP_INVALID_ID)
  {
    if (!iupMATRIX_CHECK_LIN(ih, lin))
      return;

    if (set)
      ih->data->lines.dt[lin].flags |= attr;
    else
      ih->data->lines.dt[lin].flags &= ~attr;
  }
  else
  {
    if (!ih->data->callback_mode)
    {
      if (!iupMATRIX_CHECK_COL(ih, col) ||
          !iupMATRIX_CHECK_LIN(ih, lin))
          return;

      if (set)
        ih->data->cells[lin][col].flags |= attr;
      else
        ih->data->cells[lin][col].flags &= ~attr;
    }
  }
}

void iupMatrixPrepareDrawData(Ihandle* ih)
{
  /* FGCOLOR, BGCOLOR and FONT */
  iupAttribSetStr(ih, "_IUPMAT_BGCOLOR_PARENT", iupControlBaseGetParentBgColor(ih));
  iupAttribSetStr(ih, "_IUPMAT_BGCOLOR", IupGetAttribute(ih, "BGCOLOR"));
  iupAttribSetStr(ih, "_IUPMAT_FGCOLOR", IupGetAttribute(ih, "FGCOLOR"));
  iupAttribSetStr(ih, "_IUPMAT_FONT", IupGetAttribute(ih, "FONT"));

  ih->data->bgcolor_parent = iupAttribGet(ih, "_IUPMAT_BGCOLOR_PARENT");
  ih->data->bgcolor = iupAttribGet(ih, "_IUPMAT_BGCOLOR");
  ih->data->fgcolor = iupAttribGet(ih, "_IUPMAT_FGCOLOR");
  ih->data->font = iupAttribGet(ih, "_IUPMAT_FONT");

  {
    unsigned char bg_r, bg_g, bg_b;
    iupStrToRGB(ih->data->bgcolor, &bg_r, &bg_g, &bg_b);
    ih->data->bgcolor_r = bg_r; ih->data->bgcolor_g = bg_g; ih->data->bgcolor_b = bg_b;
  }

  ih->data->font_cb = (sIFnii)IupGetCallback(ih, "FONT_CB");
  ih->data->type_cb = (sIFnii)IupGetCallback(ih, "TYPE_CB");
  ih->data->fgcolor_cb = (IFniiIII)IupGetCallback(ih, "FGCOLOR_CB");
  ih->data->bgcolor_cb = (IFniiIII)IupGetCallback(ih, "BGCOLOR_CB");
}

static char* iMatrixGetCellAttribute(Ihandle* ih, unsigned char attr, int lin, int col, int *native_parent)
{
  char* value = NULL;
  const char* attrib = NULL;
  char* attrib_global = NULL;

  if (attr == IMAT_HAS_FONT)
  {
    attrib = "FONT";
    attrib_global = ih->data->font;
  }
  else if (attr == IMAT_HAS_BGCOLOR)
  {
    attrib = "BGCOLOR";
    attrib_global = ih->data->bgcolor;
  }
  else if (attr == IMAT_HAS_FGCOLOR)
  {
    attrib = "FGCOLOR";
    attrib_global = ih->data->fgcolor;
  }
  else if (attr == IMAT_HAS_TYPE)
  {
    attrib = "TYPE";
    attrib_global = NULL;
  }

  if (!attrib)  /* Internal error */
    return NULL;

  /* 1 -  check for this cell */
  if (ih->data->callback_mode || ih->data->cells[lin][col].flags & attr)
    value = iupAttribGetId2(ih, attrib, lin, col);
  if (!value)
  {
    /* 2 - check for this line, if not title col */
    if (col != 0)
    {
      if (ih->data->lines.dt[lin].flags & attr)
        value = iupAttribGetId2(ih, attrib, lin, IUP_INVALID_ID);
    }

    if (!value)
    {
      /* 3 - check for this column, if not title line */
      if (lin != 0)
      {
        if (ih->data->columns.dt[col].flags & attr)
          value = iupAttribGetId2(ih, attrib, IUP_INVALID_ID, col);
      }

      if (!value)
      {
        /* 4 - check for the matrix or native parent */
        if (native_parent && *native_parent == 1)
        {
          *native_parent = 2;
          value = ih->data->bgcolor_parent;
        }
        else
          value = attrib_global;
      }
    }
  }

  return value;
}

static int iMatrixCallColorCB(Ihandle* ih, IFniiIII cb, int lin, int col, unsigned char *r, unsigned char *g, unsigned char *b)
{
  int ir = 0, ig = 0, ib = 0, ret;
  ret = cb(ih, lin, col, &ir, &ig, &ib);
  if (ret != IUP_IGNORE)
  {
    *r = iupBYTECROP(ir);
    *g = iupBYTECROP(ig);
    *b = iupBYTECROP(ib);
  }
  return ret;
}

char* iupMatrixGetFgColorStr(Ihandle* ih, int lin, int col)
{
  unsigned char r = 0, g = 0, b = 0;
  /* called from Edit only */
  if (!ih->data->fgcolor_cb || (iMatrixCallColorCB(ih, ih->data->fgcolor_cb, lin, col, &r, &g, &b) == IUP_IGNORE))
  {
    char* fgcolor = iMatrixGetCellAttribute(ih, IMAT_HAS_FGCOLOR, lin, col, NULL);
    if (!fgcolor)
    {
      if (lin == 0 || col == 0)
        fgcolor = IupGetGlobal("DLGFGCOLOR");
      else
        fgcolor = IupGetGlobal("TXTFGCOLOR");
    }
    return fgcolor;
  }
  else
    return iupStrReturnRGB(r, g, b);
}

/* Color attenuation factor in a marked cell, 20% darker */
#define IMAT_ATENUATION(_x)    ((unsigned char)(((_x)*8)/10))

static void iMatrixAddMarkedAttenuation(Ihandle* ih, unsigned char *r, unsigned char *g, unsigned char *b)
{
  char* hlcolor = iupAttribGetStr(ih, "HLCOLOR");
  unsigned char hl_r, hl_g, hl_b;
  if (iupStrToRGB(hlcolor, &hl_r, &hl_g, &hl_b))
  {
    unsigned char a = (unsigned char)iupAttribGetInt(ih, "HLCOLORALPHA");
    *r = iupALPHABLEND(*r, hl_r, a);
    *g = iupALPHABLEND(*g, hl_g, a);
    *b = iupALPHABLEND(*b, hl_b, a);
  }

  *r = IMAT_ATENUATION(*r);
  *g = IMAT_ATENUATION(*g);
  *b = IMAT_ATENUATION(*b);
}

void iupMatrixGetFgRGB(Ihandle* ih, int lin, int col, unsigned char *r, unsigned char *g, unsigned char *b, int marked, int active)
{
  /* called from Draw only */
  if (!ih->data->fgcolor_cb || (iMatrixCallColorCB(ih, ih->data->fgcolor_cb, lin, col, r, g, b) == IUP_IGNORE))
  {
    char* fgcolor = iMatrixGetCellAttribute(ih, IMAT_HAS_FGCOLOR, lin, col, NULL);
    if (!fgcolor)
    {
      if (lin == 0 || col == 0)
        fgcolor = IupGetGlobal("DLGFGCOLOR");
      else
        fgcolor = IupGetGlobal("TXTFGCOLOR");
    }
    iupStrToRGB(fgcolor, r, g, b);
  }

  if (marked)
    iMatrixAddMarkedAttenuation(ih, r, g, b);

  if (!active)
  {
    unsigned char bg_r, bg_g, bg_b;
    bg_r = ih->data->bgcolor_r; bg_g = ih->data->bgcolor_g; bg_b = ih->data->bgcolor_b;
    iupImageColorMakeInactive(r, g, b, bg_r, bg_g, bg_b);
  }
}

void iupMatrixGetTypeRGB(Ihandle* ih, const char* color, unsigned char *r, unsigned char *g, unsigned char *b, int marked, int active)
{
  /* called from Draw only */
  iupStrToRGB(color, r, g, b);

  if (marked)
    iMatrixAddMarkedAttenuation(ih, r, g, b);

  if (!active)
  {
    unsigned char bg_r, bg_g, bg_b;
    bg_r = ih->data->bgcolor_r; bg_g = ih->data->bgcolor_g; bg_b = ih->data->bgcolor_b;
    iupImageColorMakeInactive(r, g, b, bg_r, bg_g, bg_b);
  }
}

char* iupMatrixGetBgColorStr(Ihandle* ih, int lin, int col)
{
  unsigned char r = 0, g = 0, b = 0;
  /* called from Edit only */
  if (!ih->data->bgcolor_cb || (iMatrixCallColorCB(ih, ih->data->bgcolor_cb, lin, col, &r, &g, &b) == IUP_IGNORE))
  {
    int native_parent = 0;
    if (lin == 0 || col == 0)
      native_parent = 1;
    return iMatrixGetCellAttribute(ih, IMAT_HAS_BGCOLOR, lin, col, &native_parent);
  }
  else
    return iupStrReturnRGB(r, g, b);
}

#define IMAT_DARKER(_x)    (((_x)*9)/10)

void iupMatrixGetBgRGB(Ihandle* ih, int lin, int col, unsigned char *r, unsigned char *g, unsigned char *b, int marked, int active)
{
  /* called from Draw only */
  if (!ih->data->bgcolor_cb || (iMatrixCallColorCB(ih, ih->data->bgcolor_cb, lin, col, r, g, b) == IUP_IGNORE))
  {
    int native_parent = 0;
    if (lin == 0 || col == 0)
      native_parent = 1;
    iupStrToRGB(iMatrixGetCellAttribute(ih, IMAT_HAS_BGCOLOR, lin, col, &native_parent), r, g, b);
    if (native_parent == 2)
    {
      /* use a darker version of the background by 10% if got it from parent */
      int ir = IMAT_DARKER(*r),
        ig = IMAT_DARKER(*g),
        ib = IMAT_DARKER(*b);
      *r = iupBYTECROP(ir);
      *g = iupBYTECROP(ig);
      *b = iupBYTECROP(ib);
    }
  }

  if (marked)
    iMatrixAddMarkedAttenuation(ih, r, g, b);

  if (!active)
  {
    unsigned char bg_r, bg_g, bg_b;
    bg_r = ih->data->bgcolor_r; bg_g = ih->data->bgcolor_g; bg_b = ih->data->bgcolor_b;
    iupImageColorMakeInactive(r, g, b, bg_r, bg_g, bg_b);
  }
}

char* iupMatrixGetMaskStr(Ihandle* ih, const char* name, int lin, int col)
{
  char* value = iupAttribGetId2(ih, name, lin, col);
  if (!value)
  {
    value = iupAttribGetId2(ih, name, lin, IUP_INVALID_ID);
    if (!value)
      value = iupAttribGetId2(ih, name, IUP_INVALID_ID, col);
  }

  return value;
}

char* iupMatrixGetFont(Ihandle* ih, int lin, int col)
{
  char* font = NULL;
  /* called from Draw and Edit only */
  if (ih->data->font_cb)
    font = ih->data->font_cb(ih, lin, col);
  if (!font)
    font = iMatrixGetCellAttribute(ih, IMAT_HAS_FONT, lin, col, NULL);
  return font;
}

int iupMatrixGetType(Ihandle* ih, int lin, int col)
{
  char* type = NULL;
  /* called from Draw only */
  if (ih->data->type_cb)
    type = ih->data->type_cb(ih, lin, col);
  if (!type)
    type = iMatrixGetCellAttribute(ih, IMAT_HAS_TYPE, lin, col, NULL);
  if (!type)
    return IMAT_TYPE_TEXT;

  if (iupStrEqualNoCase(type, "COLOR"))
    return IMAT_TYPE_COLOR;
  if (iupStrEqualNoCase(type, "IMAGE"))
    return IMAT_TYPE_IMAGE;
  if (iupStrEqualNoCase(type, "FILL"))
    return IMAT_TYPE_FILL;

  return IMAT_TYPE_TEXT;
}

int iupMatrixGetFrameHorizColor(Ihandle* ih, int lin, int col, long *framecolor, int check_title)
{
  if ((ih->data->callback_mode || ih->data->cells[lin][col].flags & IMAT_HAS_FRAMEHORIZCOLOR) ||
      ih->data->lines.dt[lin].flags & IMAT_HAS_FRAMEHORIZCOLOR)
  {
    char* color = NULL;
    unsigned char r, g, b;
    if (check_title && lin == 0)
    {
      color = iupAttribGetId2(ih, "FRAMETITLEHORIZCOLOR", 0, col);
      if (!color)
        color = iupAttribGetId2(ih, "FRAMETITLEHORIZCOLOR", 0, IUP_INVALID_ID);
    }
    if (!color)
      color = iupAttribGetId2(ih, "FRAMEHORIZCOLOR", lin, col);
    if (!color)
      color = iupAttribGetId2(ih, "FRAMEHORIZCOLOR", lin, IUP_INVALID_ID);
    if (!color)
      color = iupAttribGetId2(ih, "FRAMEHORIZCOLOR", IUP_INVALID_ID, col);
    if (iupStrEqual(color, "BGCOLOR"))
      return 1;
    if (iupStrToRGB(color, &r, &g, &b))
      *framecolor = ((long)r << 16) | ((long)g << 8) | (long)b;
  }

  return 0;
}

int iupMatrixGetFrameVertColor(Ihandle* ih, int lin, int col, long *framecolor, int check_title)
{
  if ((ih->data->callback_mode || ih->data->cells[lin][col].flags & IMAT_HAS_FRAMEVERTCOLOR) ||
      ih->data->columns.dt[col].flags & IMAT_HAS_FRAMEVERTCOLOR)
  {
    char* color = NULL;
    unsigned char r, g, b;
    if (check_title && col == 0)
    {
      color = iupAttribGetId2(ih, "FRAMETITLEVERTCOLOR", lin, 0);
      if (!color)
        color = iupAttribGetId2(ih, "FRAMETITLEVERTCOLOR", IUP_INVALID_ID, 0);
    }
    if (!color)
      color = iupAttribGetId2(ih, "FRAMEVERTCOLOR", lin, col);
    if (!color)
      color = iupAttribGetId2(ih, "FRAMEVERTCOLOR", IUP_INVALID_ID, col);
    if (!color)
      color = iupAttribGetId2(ih, "FRAMEVERTCOLOR", lin, IUP_INVALID_ID);
    if (iupStrEqual(color, "BGCOLOR"))
      return 1;
    if (framecolor && iupStrToRGB(color, &r, &g, &b))
      *framecolor = ((long)r << 16) | ((long)g << 8) | (long)b;
  }

  return 0;
}

int iupMatrixGetColAlignmentLin0(Ihandle* ih)
{
  char* align = iupAttribGetStr(ih, "ALIGNMENTLIN0");
  if (!align)
    return IMAT_ALIGN_CENTER;
  else if (iupStrEqualNoCase(align, "ARIGHT"))
    return IMAT_ALIGN_END;
  else if (iupStrEqualNoCase(align, "ALEFT"))
    return IMAT_ALIGN_START;
  else
    return IMAT_ALIGN_CENTER;
}

int iupMatrixGetColAlignment(Ihandle* ih, int col)
{
  char* align = iupAttribGetId(ih, "ALIGNMENT", col);
  if (!align)
    align = iupAttribGet(ih, "ALIGNMENT");
  if (!align)
  {
    if (col == 0)
      return IMAT_ALIGN_START;
    else
      return IMAT_ALIGN_CENTER;
  }
  else if (iupStrEqualNoCase(align, "ARIGHT"))
    return IMAT_ALIGN_END;
  else if (iupStrEqualNoCase(align, "ACENTER"))
    return IMAT_ALIGN_CENTER;
  else
    return IMAT_ALIGN_START;
}

int iupMatrixGetLinAlignment(Ihandle* ih, int lin)
{
  char* align = iupAttribGetId(ih, "LINEALIGNMENT", lin);
  if (!align)
    align = iupAttribGet(ih, "LINEALIGNMENT");
  if (!align)
    return IMAT_ALIGN_CENTER;
  else if (iupStrEqualNoCase(align, "ABOTTOM"))
    return IMAT_ALIGN_END;
  else if (iupStrEqualNoCase(align, "ATOP"))
    return IMAT_ALIGN_START;
  else
    return IMAT_ALIGN_CENTER;
}

void iupMatrixGetCellAlign(Ihandle* ih, int lin, int col, int *col_alignment, int *lin_alignment)
{
  char* align = iupAttribGetId2(ih, "ALIGN", lin, col);
  if (align)
  {
    char col_align[30], lin_align[30];
    if (iupStrToStrStr(align, lin_align, col_align, ':'))
    {
      if (iupStrEqualNoCase(col_align, "ARIGHT"))
        *col_alignment = IMAT_ALIGN_END;
      else if (iupStrEqualNoCase(col_align, "ACENTER"))
        *col_alignment = IMAT_ALIGN_CENTER;
      else if (iupStrEqualNoCase(col_align, "ALEFT"))
        *col_alignment = IMAT_ALIGN_START;

      if (iupStrEqualNoCase(col_align, "ABOTTOM"))
        *lin_alignment = IMAT_ALIGN_END;
      else if (iupStrEqualNoCase(col_align, "ACENTER"))
        *lin_alignment = IMAT_ALIGN_CENTER;
      else if (iupStrEqualNoCase(col_align, "ATOP"))
        *lin_alignment = IMAT_ALIGN_START;
    }
  }
}

int iupMatrixGetColumnWidth(Ihandle* ih, int col, int use_value)
{
  int width = 0, pixels = 0;
  char* value;
  int result_width;

  /* can be called for invalid columns (col>numcol) */

  value = iupAttribGetId(ih, "WIDTH", col);
  if (!value)
  {
    value = iupAttribGetId(ih, "RASTERWIDTH", col);
    if (value)
      pixels = 1;
  }

  if (use_value && !value)
  {
    /* Use the titles to define the size */
    if (col == 0)
    {
      if (!ih->data->callback_mode || ih->data->use_title_size)
      {
        /* find the largest title */
        int lin, max_width = 0;
        for (lin = 0; lin < ih->data->lines.num; lin++)
        {
          char* title_value = iupMatrixGetValueDisplay(ih, lin, 0);
          if (title_value)
          {
            iupdrvFontGetMultiLineStringSize(ih, title_value, &width, NULL);
            if (width > max_width)
              max_width = width;
          }
        }
        width = max_width;
      }
    }
    else if (ih->data->use_title_size && (col > 0 && col < ih->data->columns.num))
    {
      char* title_value = iupMatrixGetValueDisplay(ih, 0, col);
      if (title_value)
        iupdrvFontGetMultiLineStringSize(ih, title_value, &width, NULL);
    }
    if (width)
    {
      result_width = width + IMAT_PADDING_W + IMAT_FRAME_W;
      return result_width;
    }

    if (col != 0)
      value = iupAttribGetStr(ih, "WIDTHDEF");
  }

  if (iupStrToInt(value, &width))
  {
    if (width <= 0)
      return 0;
    else
    {
      if (pixels)
      {
        result_width = width + IMAT_PADDING_W + IMAT_FRAME_W;
        return result_width;
      }
      else
      {
        int charwidth;
        iupdrvFontGetCharSize(ih, &charwidth, NULL);
        result_width = iupWIDTH2RASTER(width, charwidth) + IMAT_PADDING_W + IMAT_FRAME_W;
        return result_width;
      }
    }
  }
  return 0;
}

int iupMatrixGetLineHeight(Ihandle* ih, int lin, int use_value)
{
  int height = 0, pixels = 0;
  char* value;

  /* can be called for invalid lines (lin>numlin) */

  value = iupAttribGetId(ih, "HEIGHT", lin);
  if (!value)
  {
    value = iupAttribGetId(ih, "RASTERHEIGHT", lin);
    if (value)
      pixels = 1;
  }

  if (use_value && !value)
  {
    /* Use the titles to define the size */
    if (lin == 0)
    {
      if (!ih->data->callback_mode || ih->data->use_title_size)
      {
        /* find the highest title */
        int col, max_height = 0;
        for (col = 0; col < ih->data->columns.num; col++)
        {
          char* title_value = iupMatrixGetValueDisplay(ih, 0, col);
          if (title_value && title_value[0])
          {
            iupdrvFontGetMultiLineStringSize(ih, title_value, NULL, &height);
            if (height > max_height)
              max_height = height;
          }
        }
        height = max_height;
      }
    }
    else if (ih->data->use_title_size && (lin > 0 && lin < ih->data->lines.num))
    {
      /* Scan all cells in this row to find the maximum height needed */
      int col, max_height = 0;
      for (col = 0; col < ih->data->columns.num; col++)
      {
        char* cell_value = iupMatrixGetValueDisplay(ih, lin, col);
        if (cell_value && cell_value[0])
        {
          int cell_height;
          iupdrvFontGetMultiLineStringSize(ih, cell_value, NULL, &cell_height);
          if (cell_height > max_height)
            max_height = cell_height;
        }
      }
      height = max_height;
    }
    if (height)
      return height + IMAT_PADDING_H + IMAT_FRAME_H;

    if (lin != 0)
      value = iupAttribGetStr(ih, "HEIGHTDEF");
  }

  if (iupStrToInt(value, &height))
  {
    if (height <= 0)
      return 0;
    else
    {
      if (pixels)
        return height + IMAT_PADDING_H + IMAT_FRAME_H;
      else
      {
        int charheight;
        iupdrvFontGetCharSize(ih, NULL, &charheight);
        return iupHEIGHT2RASTER(height, charheight) + IMAT_PADDING_H + IMAT_FRAME_H;
      }
    }
  }
  return 0;
}

char *iupMatrixGetSize(Ihandle* ih, int index, int m, int pixels_unit)
{
  int size;
  ImatLinColData *p;

  if (m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  if (index < 0 || index > p->num - 1)
    return NULL;

  if (m == IMAT_PROCESS_LIN)
    size = iupMatrixGetLineHeight(ih, index, 1);
  else
    size = iupMatrixGetColumnWidth(ih, index, 1);

  if (size)
  {
    /* remove the decoration added in iupMatrixAuxGet */
    if (m == IMAT_PROCESS_COL)
      size -= IMAT_PADDING_W + IMAT_FRAME_W;
    else
      size -= IMAT_PADDING_H + IMAT_FRAME_H;

    if (!pixels_unit)
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      if (m == IMAT_PROCESS_COL)
        size = iupRASTER2WIDTH(size, charwidth);
      else
        size = iupRASTER2HEIGHT(size, charheight);
    }
  }

  return iupStrReturnInt(size);
}

static int iMatrixGetOffset(int index, int *offset, ImatLinColData *p)
{
  int i;

  *offset = 0;

  /* check if the cell is not empty */
  if (!p->dt[index].size)
    return 0;

  if (index < p->num_noscroll)
  {
    for (i = 0; i < index; i++)
      *offset += p->dt[i].size;
  }
  else
  {
    if (index < p->first ||
        index > p->last)
        return 0;

    for (i = 0; i < p->num_noscroll; i++)
      *offset += p->dt[i].size;

    /* Find the initial position */
    *offset -= p->first_offset;  /* index is always greater or equal to first */
    for (i = p->first; i < index; i++)
      *offset += p->dt[i].size;
  }

  return 1;
}

int iupMatrixGetCellOffset(Ihandle* ih, int lin, int col, int *x, int *y)
{
  if (!iMatrixGetOffset(col, x, &(ih->data->columns)))
    return 0;

  if (!iMatrixGetOffset(lin, y, &(ih->data->lines)))
    return 0;

  return 1;
}

static int iMatrixGetIndexFromOffset(int pos, ImatLinColData *p)
{
  int offset = 0, i;

  if (pos < 0)
    return -1;  /* invalid */

  for (i = 0; i < p->num_noscroll; i++)  /* for all non scrollable cells */
  {
    offset += p->dt[i].size;

    if (pos < offset)
      break;
  }

  if (pos >= offset)
  {
    for (i = p->first; i <= p->last; i++)  /* for all visible cells */
    {
      offset += p->dt[i].size;
      if (i == p->first)
        offset -= p->first_offset;

      if (pos < offset)
        break;
    }

    if (i > p->last)
      i = -1;    /* invisible */
  }

  return i;
}

int iupMatrixGetCellFromXY(Ihandle* ih, int x, int y, int* l, int* c)
{
  int col = iMatrixGetIndexFromOffset(x, &(ih->data->columns));
  int lin = iMatrixGetIndexFromOffset(y, &(ih->data->lines));
  int merged = 0;

  if (col < 0 || lin < 0)
    return 0;

  if (ih->data->merge_info_count)
    merged = iupMatrixGetMerged(ih, lin, col);

  if (merged)
  {
    int startLin, startCol;
    iupMatrixGetMergedRect(ih, merged, &startLin, NULL, &startCol, NULL);

    *l = startLin;
    *c = startCol;
  }
  else
  {
    *l = lin;
    *c = col;
  }

  return 1;
}

static int iMatrixGetCellDim(int index, int* offset, int* size, ImatLinColData *p)
{
  int i, visible = 1;

  *offset = 0;
  if (index < p->num_noscroll)
  {
    for (i = 0; i < index; i++)
      *offset += p->dt[i].size;
  }
  else
  {
    for (i = 0; i < p->num_noscroll; i++)
      *offset += p->dt[i].size;

    for (i = p->first; i < index; i++)
    {
      *offset += p->dt[i].size;

      if (i == p->first)
        *offset -= p->first_offset;  /* add only when index greater than first */
    }

    if (index < p->first)
      visible = 0;
  }

  *size = p->dt[index].size;
  if (index == p->first)
    *size -= p->first_offset;

  return visible;
}

int iupMatrixGetVisibleCellDim(Ihandle* ih, int lin, int col, int* x, int* y, int* w, int* h)
{
  int visible = 1;
  int merged = 0;

  if (ih->data->merge_info_count)
    merged = iupMatrixGetMerged(ih, lin, col);

  if (merged)
  {
    int i, j;
    int dummyX, dummyY;
    int mergedW, mergedH;
    int startLin, endLin, startCol, endCol;

    iupMatrixGetMergedRect(ih, merged, &startLin, &endLin, &startCol, &endCol);

    visible &= iMatrixGetCellDim(startCol, x, w, &(ih->data->columns));
    visible &= iMatrixGetCellDim(startLin, y, h, &(ih->data->lines));

    for (i = startLin+1; i <= endLin; i++)
    {
      iMatrixGetCellDim(i, &dummyY, &mergedH, &(ih->data->lines));
      *h += mergedH;
    }

    for (j = startCol+1; j <= endCol; j++)
    {
      iMatrixGetCellDim(j, &dummyX, &mergedW, &(ih->data->columns));
      *w += mergedW;
    }
  }
  else
  {
    visible &= iMatrixGetCellDim(col, x, w, &(ih->data->columns));
    visible &= iMatrixGetCellDim(lin, y, h, &(ih->data->lines));
  }

  *w = *w - 1;  /* do not include right frame line */
  *h = *h - 1;  /* do not include bottom frame line */
  return visible;
}

int iupMatrixHasMerged(Ihandle* ih, int startLin, int endLin, int startCol, int endCol)
{
  int lin, col, merged;

  if (!ih->data->merge_info_count)
    return 0;

  for (lin = startLin; lin <= endLin; lin++)
  {
    for (col = startCol; col <= endCol; col++)
    {
      merged = iupMatrixGetMerged(ih, lin, col);
      if (merged)
        return 1;
    }
  }

  return 0;
}

static int iMatrixMergeAddRange(Ihandle* ih, int startLin, int endLin, int startCol, int endCol)
{
  int merged;
  ImatMergedData* merged_data;

  if (!ih->data->merge_info)
  {
    ih->data->merge_info_max = 10;
    ih->data->merge_info = malloc(ih->data->merge_info_max * sizeof(ImatMergedData));
    memset(ih->data->merge_info, 0, ih->data->merge_info_max * sizeof(ImatMergedData));
  }

  /* find an empty slot */
  for (merged = 1; merged <= ih->data->merge_info_max; merged++)
  {
    if (ih->data->merge_info[merged - 1].used == 0)
    {
      merged_data = ih->data->merge_info + (merged - 1);
      merged_data->start_lin = startLin;
      merged_data->start_col = startCol;
      merged_data->end_lin = endLin;
      merged_data->end_col = endCol;
      merged_data->used = 1;

      ih->data->merge_info_count++;

      return merged;
    }
  }

  /* add more room */
  ih->data->merge_info_max += 10;
  ih->data->merge_info = realloc(ih->data->merge_info, ih->data->merge_info_max * sizeof(ImatMergedData));
  memset(ih->data->merge_info + ih->data->merge_info_max - 10, 0, 10 * sizeof(ImatMergedData));

  merged_data = ih->data->merge_info + (merged - 1);
  merged_data->start_lin = startLin;
  merged_data->start_col = startCol;
  merged_data->end_lin = endLin;
  merged_data->end_col = endCol;
  merged_data->used = 1;

  ih->data->merge_info_count++;

  return merged;
}

int iupMatrixGetMerged(Ihandle* ih, int lin, int col)
{
  return iupAttribGetIntId2(ih, "MERGED", lin, col);
}

void iupMatrixMergeRange(Ihandle* ih, int startLin, int endLin, int startCol, int endCol)
{
  int lin, col;

  int merged = iMatrixMergeAddRange(ih, startLin, endLin, startCol, endCol);

  for (lin = startLin; lin <= endLin; lin++)
  {
    for (col = startCol; col <= endCol; col++)
    {
      iupAttribSetIntId2(ih, "MERGED", lin, col, merged);
    }
  }
}

void iupMatrixMergeSplitRange(Ihandle* ih, int merged)
{
  int lin, col;
  int startLin, endLin, startCol, endCol;

  iupMatrixGetMergedRect(ih, merged, &startLin, &endLin, &startCol, &endCol);

  for (lin = startLin; lin <= endLin; lin++)
  {
    for (col = startCol; col <= endCol; col++)
    {
      iupAttribSetId2(ih, "MERGED", lin, col, NULL);
    }
  }

  ih->data->merge_info[merged - 1].used = 0;
  ih->data->merge_info_count--;
}

void iupMatrixGetMergedRect(Ihandle *ih, int merged, int *startLin, int *endLin, int *startCol, int *endCol)
{
  ImatMergedData* merged_data = ih->data->merge_info + (merged - 1);
  if (merged_data->used)
  {
    if (startLin) *startLin = merged_data->start_lin;
    if (startCol) *startCol = merged_data->start_col;
    if (endLin)   *endLin   = merged_data->end_lin;
    if (endCol)   *endCol   = merged_data->end_col;
  }
}

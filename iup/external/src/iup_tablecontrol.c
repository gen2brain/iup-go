/** \file
 * \brief IupTable control core
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
#include "iup_assert.h"
#include "iup_classbase.h"

#include "iup_tablecontrol.h"


/* ========================================================================= */
/* Validation Functions                                                      */
/* ========================================================================= */

int iupTableIsValid(Ihandle* ih)
{
  if (!ih->handle)
    return 0;
  return 1;
}

int iupTableCheckCellPos(Ihandle* ih, int lin, int col)
{
  if (lin < 1 || col < 1)
    return 0;
  if (lin > ih->data->num_lin || col > ih->data->num_col)
    return 0;
  return 1;
}

/* ========================================================================= */
/* Attribute Get/Set Functions                                              */
/* ========================================================================= */

static char* iTableGetNumLinAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->num_lin);
}

static int iTableSetNumLinAttrib(Ihandle* ih, const char* value)
{
  int num_lin;
  if (iupStrToInt(value, &num_lin))
  {
    if (num_lin < 0)
      num_lin = 0;

    if (ih->handle)
      iupdrvTableSetNumLin(ih, num_lin);
    else
      ih->data->num_lin = num_lin;  /* Before map, just store */
  }
  return 0;
}

static char* iTableGetNumColAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->num_col);
}

static int iTableSetNumColAttrib(Ihandle* ih, const char* value)
{
  int num_col;
  if (iupStrToInt(value, &num_col))
  {
    if (num_col < 0)
      num_col = 0;

    if (ih->handle)
      iupdrvTableSetNumCol(ih, num_col);
    else
      ih->data->num_col = num_col;  /* Before map, just store */
  }
  return 0;
}

static char* iTableGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->num_lin * ih->data->num_col);
}

static int iTableSetAddLinAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  if (!ih->handle)
    return 0;

  if (iupStrToInt(value, &pos))
  {
    if (pos <= 0 || pos == -1)  /* Append */
      pos = ih->data->num_lin + 1;
    else if (pos > ih->data->num_lin + 1)
      pos = ih->data->num_lin + 1;

    iupdrvTableAddLin(ih, pos);
  }
  return 0;
}

static int iTableSetDelLinAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  if (!ih->handle)
    return 0;

  if (iupStrToInt(value, &pos))
  {
    if (pos < 1 || pos > ih->data->num_lin)
      return 0;

    iupdrvTableDelLin(ih, pos);
  }
  return 0;
}

static int iTableSetAddColAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  if (!ih->handle)
    return 0;

  if (iupStrToInt(value, &pos))
  {
    if (pos <= 0 || pos == -1)  /* Append */
      pos = ih->data->num_col + 1;
    else if (pos > ih->data->num_col + 1)
      pos = ih->data->num_col + 1;

    iupdrvTableAddCol(ih, pos);
  }
  return 0;
}

static int iTableSetDelColAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  if (!ih->handle)
    return 0;

  if (iupStrToInt(value, &pos))
  {
    if (pos < 1 || pos > ih->data->num_col)
      return 0;

    iupdrvTableDelCol(ih, pos);
  }
  return 0;
}

/* ========================================================================= */
/* Cell Value Attributes (L:C notation)                                     */
/* ========================================================================= */

static char* iTableGetIdValueAttrib(Ihandle* ih, int lin, int col)
{
  if (!iupTableCheckCellPos(ih, lin, col))
    return NULL;

  if (!ih->handle)
    return NULL;

  return iupdrvTableGetCellValue(ih, lin, col);
}

static int iTableSetIdValueAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  if (!iupTableCheckCellPos(ih, lin, col))
    return 0;

  if (!ih->handle)
    return 0;

  iupdrvTableSetCellValue(ih, lin, col, value);
  return 0;
}

/* ========================================================================= */
/* Column Attributes (TITLEn, ALIGNMENTn, WIDTHn)                           */
/* ========================================================================= */

static char* iTableGetTitleIdAttrib(Ihandle* ih, int col)
{
  if (col < 1 || col > ih->data->num_col)
    return NULL;

  if (!ih->handle)
  {
    /* Return from hash table if not mapped */
    char name[50];
    sprintf(name, "TITLE%d", col);
    return iupAttribGet(ih, name);
  }

  return iupdrvTableGetColTitle(ih, col);
}

static int iTableSetTitleIdAttrib(Ihandle* ih, int col, const char* value)
{
  if (col < 1 || col > ih->data->num_col)
    return 0;

  if (!ih->handle)
  {
    /* Store in hash table if not mapped */
    char name[50];
    sprintf(name, "TITLE%d", col);
    iupAttribSetStr(ih, name, value);
    return 0;
  }

  iupdrvTableSetColTitle(ih, col, value);
  return 0;
}

static char* iTableGetWidthIdAttrib(Ihandle* ih, int col)
{
  char name[50];
  char* value;

  if (col < 1 || col > ih->data->num_col)
    return NULL;

  /* Always check hash table first */
  sprintf(name, "WIDTH%d", col);
  value = iupAttribGet(ih, name);
  if (value)
    return value;

  /* If mapped and no stored value, get from driver */
  if (ih->handle)
    return iupStrReturnInt(iupdrvTableGetColWidth(ih, col));

  return NULL;
}

static int iTableSetWidthIdAttrib(Ihandle* ih, int col, const char* value)
{
  int width;
  char name[50];

  if (col < 1 || col > ih->data->num_col)
  {
    return 0;
  }

  if (!iupStrToInt(value, &width))
  {
    return 0;
  }

  if (width < 0)
    width = 0;

  /* Always store in hash table for natural size calculation */
  sprintf(name, "WIDTH%d", col);
  iupAttribSetStr(ih, name, value);

  /* If mapped, also apply to native widget */
  if (ih->handle)
  {
    iupdrvTableSetColWidth(ih, col, width);
  }

  return 0;
}

static char* iTableGetRasterWidthIdAttrib(Ihandle* ih, int col)
{
  char name[50];
  char* value;

  if (col < 1 || col > ih->data->num_col)
    return NULL;

  /* Check for RASTERWIDTH first, then fall back to WIDTH */
  sprintf(name, "RASTERWIDTH%d", col);
  value = iupAttribGet(ih, name);
  if (value)
    return value;

  /* Try WIDTH attribute */
  sprintf(name, "WIDTH%d", col);
  value = iupAttribGet(ih, name);
  if (value)
    return value;

  /* If mapped and no stored value, get from driver */
  if (ih->handle)
    return iupStrReturnInt(iupdrvTableGetColWidth(ih, col));

  return NULL;
}

static int iTableSetRasterWidthIdAttrib(Ihandle* ih, int col, const char* value)
{
  /* For now, treat RASTERWIDTH the same as WIDTH (in pixels) */
  return iTableSetWidthIdAttrib(ih, col, value);
}

/* ========================================================================= */
/* Selection Attributes                                                      */
/* ========================================================================= */

static char* iTableGetFocusCellAttrib(Ihandle* ih)
{
  if (!ih->handle)
  {
    char* value = iupAttribGet(ih, "FOCUSCELL");
    if (value)
      return value;
    return "1:1";  /* Default */
  }

  int lin, col;
  iupdrvTableGetFocusCell(ih, &lin, &col);
  return iupStrReturnIntInt(lin, col, ':');
}

static int iTableSetFocusCellAttrib(Ihandle* ih, const char* value)
{
  int lin, col;
  int num_parsed;

  /* Check for column-only syntax ":col" */
  if (value[0] == ':')
  {
    /* Column-only syntax like ":3" - keep current row, only change column */
    if (!iupStrToInt(value + 1, &col) || col < 1)
      return 0;  /* Invalid column */

    if (ih->handle)
    {
      int current_lin, current_col;
      iupdrvTableGetFocusCell(ih, &current_lin, &current_col);
      lin = current_lin;
    }
    else
    {
      /* Not mapped yet, parse stored FOCUSCELL to get row, or default to 1 */
      char* stored = iupAttribGet(ih, "FOCUSCELL");
      int stored_lin, stored_col;
      if (stored && iupStrToIntInt(stored, &stored_lin, &stored_col, ':') >= 1)
      {
        /* Keep the stored row */
        lin = stored_lin;
      }
      else
      {
        /* No stored value, default to row 1 */
        lin = 1;
      }
    }
  }
  else
  {
    /* Parse value - supports "row:col" and "row:" (row-only) */
    num_parsed = iupStrToIntInt(value, &lin, &col, ':');

    if (num_parsed == 1)
    {
      /* Row-only syntax like "2:" - keep current column, only change row */
      if (ih->handle)
      {
        int current_lin, current_col;
        iupdrvTableGetFocusCell(ih, &current_lin, &current_col);
        col = current_col;
      }
      else
      {
        /* Not mapped yet, parse stored FOCUSCELL to get column, or default to 1 */
        char* stored = iupAttribGet(ih, "FOCUSCELL");
        int stored_lin, stored_col;
        if (stored && iupStrToIntInt(stored, &stored_lin, &stored_col, ':') >= 1)
        {
          /* Keep the stored column */
          col = stored_col;
        }
        else
        {
          /* No stored value, default to column 1 */
          col = 1;
        }
      }
    }
    else if (num_parsed != 2)
    {
      /* Invalid format */
      return 0;
    }
  }

  if (!iupTableCheckCellPos(ih, lin, col))
    return 0;

  if (!ih->handle)
  {
    /* Store for later */
    iupAttribSetStr(ih, "FOCUSCELL", value);
    return 0;
  }

  iupdrvTableSetFocusCell(ih, lin, col);
  return 0;
}

static char* iTableGetValueAttrib(Ihandle* ih)
{
  /* VALUE is the value of the focused cell */
  if (!ih->handle)
    return NULL;

  int lin, col;
  iupdrvTableGetFocusCell(ih, &lin, &col);

  if (!iupTableCheckCellPos(ih, lin, col))
    return NULL;

  return iupdrvTableGetCellValue(ih, lin, col);
}

static int iTableSetValueAttrib(Ihandle* ih, const char* value)
{
  /* Set value of focused cell */
  if (!ih->handle)
    return 0;

  int lin, col;
  iupdrvTableGetFocusCell(ih, &lin, &col);

  if (!iupTableCheckCellPos(ih, lin, col))
    return 0;

  iupdrvTableSetCellValue(ih, lin, col, value);
  return 0;
}

/* ========================================================================= */
/* Scrolling Attributes                                                      */
/* ========================================================================= */

static int iTableSetShowAttrib(Ihandle* ih, const char* value)
{
  int lin, col;

  if (!ih->handle)
    return 0;

  if (iupStrToIntInt(value, &lin, &col, ':') != 2)
    return 0;

  if (!iupTableCheckCellPos(ih, lin, col))
    return 0;

  iupdrvTableScrollToCell(ih, lin, col);
  return 0;
}

static int iTableSetRedrawAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvTableRedraw(ih);
  return 0;
}

/* ========================================================================= */
/* Display Attributes                                                        */
/* ========================================================================= */

static int iTableSetShowGridAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    int show = iupStrBoolean(value);
    iupdrvTableSetShowGrid(ih, show);
  }
  return 1;  /* Store in hash */
}

/* ========================================================================= */
/* IupTable Class Methods                                                    */
/* ========================================================================= */

static int iTableCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  /* Allocate control data */
  ih->data = iupALLOCCTRLDATA();

  /* Initialize default values */
  ih->data->num_lin = 0;
  ih->data->num_col = 0;
  ih->data->sortable = 0;  /* Sorting disabled by default */
  ih->data->allow_reorder = 0;  /* Column reordering disabled by default */
  ih->data->user_resize = 0;  /* User column resizing disabled by default */
  ih->data->stretch_last = 1;  /* Last column stretching enabled by default */

  /* Default EXPAND is YES */
  ih->expand = IUP_EXPAND_BOTH;

  return IUP_NOERROR;
}

static void iTableComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  int charwidth, charheight;
  int col;
  int visiblecolumns, visiblelines;
  int max_col, visible_lines;

  /* Tell parent container about our expand settings */
  *children_expand = ih->expand;

  /* Get font metrics */
  iupdrvFontGetCharSize(ih, &charwidth, &charheight);

  /* Get VISIBLECOLUMNS and VISIBLELINES attributes */
  visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

  /* Determine how many columns to include in width calculation */
  if (visiblecolumns > 0 && ih->data->num_col > 0)
    max_col = (visiblecolumns < ih->data->num_col) ? visiblecolumns : ih->data->num_col;
  else
    max_col = ih->data->num_col;

  /* Calculate width from column widths */
  if (max_col > 0)
  {
    for (col = 1; col <= max_col; col++)
    {
      int col_width = 0;
      char* value;

      /* Check if WIDTH or RASTERWIDTH is set (these are ID-based attributes) */
      char name[50];
      sprintf(name, "RASTERWIDTH%d", col);
      value = iupAttribGet(ih, name);
      if (value)
      {
        int success = iupStrToInt(value, &col_width);
        if (success && col_width > 0)
        {
          natural_w += col_width;
          continue;
        }
      }

      sprintf(name, "WIDTH%d", col);
      value = iupAttribGet(ih, name);
      if (value)
      {
        int success = iupStrToInt(value, &col_width);
        if (success && col_width > 0)
        {
          natural_w += col_width;
          continue;
        }
      }

      /* No explicit width set, use a reasonable default. */
      col_width = 100;
      natural_w += col_width;
    }
  }
  else
  {
    natural_w = 80 * charwidth;  /* Default for no columns */
  }

  /* Calculate height from number of lines */
  if (visiblelines > 0)
    visible_lines = visiblelines;
  else
  {
    visible_lines = ih->data->num_lin;
    if (visible_lines == 0)
      visible_lines = 10;  /* Default to 10 visible lines */
    else if (visible_lines > 15)
      visible_lines = 15;  /* Cap at 15 lines for natural size to keep it reasonable */
  }

  /* Get row heights from driver */
  int row_height = iupdrvTableGetRowHeight(ih);
  int header_height = iupdrvTableGetHeaderHeight(ih);

  /* Calculate height: header + visible data rows */
  natural_h = header_height + (row_height * visible_lines);

  /* Add space for grid lines, dummy column */
  int grid_lines = 0;
  int dummy_column = 0;

  /* Only account for grid lines if SHOWGRID is enabled (default is YES) */
  char* showgrid = iupAttribGetStr(ih, "SHOWGRID");
  if (iupStrBoolean(showgrid) && max_col > 0)
    grid_lines = max_col - 1;  /* 1px grid line between each visible column */

  /* Check if drivers will add dummy column (when STRETCHLAST=NO or last column has explicit width) */
  if (!ih->data->stretch_last)
  {
    dummy_column = 1; /* GTK3/GTK4 add 1px dummy column when STRETCHLAST=NO */
  }
  else if (ih->data->num_col > 0)
  {
    /* Check if last column has explicit width */
    char name[50];
    sprintf(name, "RASTERWIDTH%d", ih->data->num_col);
    char* width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      sprintf(name, "WIDTH%d", ih->data->num_col);
      width_str = iupAttribGet(ih, name);
    }
    int width = 0;
    if (width_str && iupStrToInt(width_str, &width) && width > 0)
    {
      dummy_column = 1;
    }
  }

  natural_w += grid_lines + dummy_column;

  /* Add driver-specific borders (scrollbar, frame) */
  iupdrvTableAddBorders(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static char* iTableGetSortableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->sortable);
}

static int iTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
    ih->data->sortable = 0;
  return 0; /* do not store in hash table */
}

static char* iTableGetAllowReorderAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->allow_reorder);
}

static int iTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;
  return 0; /* do not store in hash table */
}

static char* iTableGetUserResizeAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->user_resize);
}

static int iTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;
  return 0; /* do not store in hash table */
}

static char* iTableGetStretchLastAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->stretch_last);
}

static int iTableSetStretchLastAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->stretch_last = 1;
  else
    ih->data->stretch_last = 0;
  return 0; /* do not store in hash table */
}

static void iTableDestroyMethod(Ihandle* ih)
{
  /* Nothing to free in core, native widget handles cleanup */
  (void)ih;
}

/* ========================================================================= */
/* IupTable Class Registration                                              */
/* ========================================================================= */

Iclass* iupTableNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "table";
  ic->format = NULL;  /* No creation parameters */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 2;  /* Has attributes with IDs (both single and double) */

  /* Class methods */
  ic->New = iupTableNewClass;
  ic->Create = iTableCreateMethod;
  ic->Destroy = iTableDestroyMethod;
  ic->ComputeNaturalSize = iTableComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  /* IupTable Callbacks */
  iupClassRegisterCallback(ic, "CLICK_CB", "iis");
  iupClassRegisterCallback(ic, "ENTERITEM_CB", "ii");
  iupClassRegisterCallback(ic, "SORT_CB", "i");  /* col, called when user clicks column header to sort */
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "ii");
  iupClassRegisterCallback(ic, "EDITBEGIN_CB", "ii");  /* lin, col, called when editing starts, return IUP_IGNORE to block */
  iupClassRegisterCallback(ic, "EDITEND_CB", "iisi");  /* lin, col, new_value, apply (1=accepted, 0=cancelled), return IUP_IGNORE to reject */
  iupClassRegisterCallback(ic, "EDITION_CB", "iis");  /* lin, col, new_text */
  iupClassRegisterCallback(ic, "VALUE_CB", "ii=s");  /* lin, col, returns string value for virtual mode */

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Common attributes */
  iupClassRegisterAttribute(ic, "NUMLIN", iTableGetNumLinAttrib, iTableSetNumLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL", iTableGetNumColAttrib, iTableSetNumColAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iTableGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Overwrite common attributes */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseGetExpandAttrib, iupBaseSetExpandAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Action attributes */
  iupClassRegisterAttribute(ic, "ADDLIN", NULL, iTableSetAddLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DELLIN", NULL, iTableSetDelLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDCOL", NULL, iTableSetAddColAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DELCOL", NULL, iTableSetDelColAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOW", NULL, iTableSetShowAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDRAW", NULL, iTableSetRedrawAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Cell value (L:C notation) */
  iupClassRegisterAttributeId2(ic, "IDVALUE", iTableGetIdValueAttrib, iTableSetIdValueAttrib, IUPAF_NO_INHERIT);

  /* Column attributes with ID */
  iupClassRegisterAttributeId(ic, "TITLE", iTableGetTitleIdAttrib, iTableSetTitleIdAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "WIDTH", iTableGetWidthIdAttrib, iTableSetWidthIdAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "RASTERWIDTH", iTableGetRasterWidthIdAttrib, iTableSetRasterWidthIdAttrib, IUPAF_NO_INHERIT);

  /* Selection attributes */
  iupClassRegisterAttribute(ic, "FOCUSCELL", iTableGetFocusCellAttrib, iTableSetFocusCellAttrib, IUPAF_SAMEASSYSTEM, "1:1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iTableGetValueAttrib, iTableSetValueAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONMODE", NULL, NULL, IUPAF_SAMEASSYSTEM, "SINGLE", IUPAF_NO_INHERIT);  /* NONE, SINGLE, MULTIPLE */

  /* Display attributes */
  iupClassRegisterAttribute(ic, "SHOWGRID", NULL, iTableSetShowGridAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSRECT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* Editing attributes */
  iupClassRegisterAttribute(ic, "EDITABLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EDITABLE", NULL, NULL, IUPAF_NO_INHERIT);  /* Per-column EDITABLE */

  /* Cell color attributes (L:C notation for per-cell, :C for per-column, L:* for per-row) */
  iupClassRegisterAttributeId2(ic, "BGCOLOR", NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FGCOLOR", NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Cell alignment attributes */
  iupClassRegisterAttributeId(ic, "ALIGNMENT", NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* Per-column alignment: ALEFT, ACENTER, ARIGHT */

  /* Sorting attributes - driver will replace SET handler to update native widget */
  iupClassRegisterAttribute(ic, "SORTABLE", iTableGetSortableAttrib, iTableSetSortableAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* Enable/disable column sorting: YES, NO */

  /* Column reordering attributes - driver will replace SET handler to update native widget */
  iupClassRegisterAttribute(ic, "ALLOWREORDER", iTableGetAllowReorderAttrib, iTableSetAllowReorderAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* Enable/disable column reordering via drag-and-drop: YES, NO */

  /* Column resizing attributes */
  iupClassRegisterAttribute(ic, "USERRESIZE", iTableGetUserResizeAttrib, iTableSetUserResizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* Enable/disable user column resizing: YES, NO */

  /* Last column stretching attribute (MAP-ONLY: must be set before mapping) */
  iupClassRegisterAttribute(ic, "STRETCHLAST", iTableGetStretchLastAttrib, iTableSetStretchLastAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* Enable/disable last column stretching to fill space: YES, NO */

  /* Alternating row color attributes */
  iupClassRegisterAttribute(ic, "ALTERNATECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);  /* Enable/disable alternating row colors: YES, NO */
  iupClassRegisterAttribute(ic, "EVENROWCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);  /* Background color for even rows */
  iupClassRegisterAttribute(ic, "ODDROWCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);   /* Background color for odd rows */

  /* Virtual mode attributes */
  iupClassRegisterAttribute(ic, "VIRTUALMODE", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT); /* Enable/disable virtual mode for large datasets: YES, NO */

  /* Visible size attributes (for natural size calculation) */
  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);  /* Number of columns to display in natural size */
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);    /* Number of data rows to display in natural size */

  /* Driver-specific attribute registration */
  iupdrvTableInitClass(ic);

  return ic;
}

Ihandle* IupTable(void)
{
  return IupCreate("table");
}

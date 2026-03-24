/** \file
 * \brief IupMatrix Expansion Library.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "iup.h"
#include "iupcbs.h"
#include "iupcontrols.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_matrixex.h"


int iupMatrixExIsColumnVisible(Ihandle* ih, int col)
{
  int width = 0;
  char* value;

  if (col==0)
    return (IupGetIntId(ih, "RASTERWIDTH", 0) != 0);

  /* to be invisible must exist the attribute and must be set to 0 (zero), 
     or else is visible */

  value = iupAttribGetId(ih, "WIDTH", col);
  if (!value)
  {
    value = iupAttribGetId(ih, "RASTERWIDTH", col);
    if (!value)
      return 1;
  }

  if (iupStrToInt(value, &width)==1)
  {
    if (width==0)
      return 0;
  }

  return 1;
}

int iupMatrixExIsLineVisible(Ihandle* ih, int lin)
{
  int height = 0;
  char* value;

  if (lin==0)
    return (IupGetIntId(ih, "RASTERHEIGHT", 0) != 0);

  value = iupAttribGetId(ih, "HEIGHT", lin);
  if(!value)
  {
    value = iupAttribGetId(ih, "RASTERHEIGHT", lin);
    if(!value)
      return 1;
  }

  if (iupStrToInt(value, &height)==1)
  {
    if (height==0)
      return 0;
  }

  return 1;
}

static char* iMatrixGetVisibleColAttribId(Ihandle *ih, int col)
{
  return iupStrReturnBoolean (iupMatrixExIsColumnVisible(ih, col)); 
}

static char* iMatrixGetVisibleLinAttribId(Ihandle *ih, int lin)
{
  return iupStrReturnBoolean (iupMatrixExIsLineVisible(ih, lin)); 
}

static int iMatrixSetVisibleColAttribId(Ihandle *ih, int col, const char* value)
{
  char* old_width;

  if (!iupStrBoolean(value))
  {
    old_width = iupAttribGetId(ih, "WIDTH", col);
    if (old_width) iupAttribSetStrId(ih, "_IUP_SHOWCOL_WIDTH", col, old_width);
    else { old_width = iupAttribGetId(ih, "RASTERWIDTH", col);
         if (old_width) iupAttribSetStrId(ih, "_IUP_SHOWCOL_RASTERWIDTH", col, old_width); }

    IupSetAttributeId(ih, "WIDTH", col, "0");    /* this is enough */
  }
  else
  {
    iupAttribSetId(ih, "WIDTH", col, NULL);  /* this may be insufficient */
    iupAttribSetId(ih, "RASTERWIDTH", col, NULL);

    old_width = iupAttribGetId(ih, "_IUP_SHOWCOL_WIDTH", col);
    if (old_width) IupSetStrAttributeId(ih, "WIDTH", col, old_width);
    else { old_width = iupAttribGetId(ih, "_IUP_SHOWCOL_RASTERWIDTH", col);
         if (old_width) IupSetStrAttributeId(ih, "RASTERWIDTH", col, old_width); 
         else IupSetAttributeId(ih, "RASTERWIDTH", col, NULL); }
  }
  return 0;
}

static int iMatrixSetVisibleLinAttribId(Ihandle *ih, int lin, const char* value)
{
  char* old_height;

  if (!iupStrBoolean(value))
  {
    old_height = iupAttribGetId(ih, "HEIGHT", lin);
    if (old_height) iupAttribSetStrId(ih, "_IUP_SHOWCOL_HEIGHT", lin, old_height);
    else { old_height = iupAttribGetId(ih, "RASTERHEIGHT", lin);
         if (old_height) iupAttribSetStrId(ih, "_IUP_SHOWCOL_RASTERHEIGHT", lin, old_height); }

    IupSetAttributeId(ih, "HEIGHT", lin, "0");    /* this is enough */
  }
  else
  {
    iupAttribSetId(ih, "HEIGHT", lin, NULL);  /* this may be insufficient */
    iupAttribSetId(ih, "RASTERHEIGHT", lin, NULL);

    old_height = iupAttribGetId(ih, "_IUP_SHOWCOL_HEIGHT", lin);
    if (old_height) IupSetStrAttributeId(ih, "HEIGHT", lin, old_height);
    else { old_height = iupAttribGetId(ih, "_IUP_SHOWCOL_RASTERHEIGHT", lin);
         if (old_height) IupSetStrAttributeId(ih, "RASTERHEIGHT", lin, old_height); 
         else IupSetAttributeId(ih, "RASTERHEIGHT", lin, NULL); }
  }
  return 0;
}

void iupMatrixExRegisterVisible(Iclass* ic)
{
  iupClassRegisterAttributeId(ic, "VISIBLECOL", iMatrixGetVisibleColAttribId, iMatrixSetVisibleColAttribId, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "VISIBLELIN", iMatrixGetVisibleLinAttribId, iMatrixSetVisibleLinAttribId, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
}


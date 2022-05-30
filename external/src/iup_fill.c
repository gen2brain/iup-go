/** \file
 * \brief Fill Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"


enum {IUP_FILL_NONE, IUP_FILL_HORIZ, IUP_FILL_VERT};

struct _IcontrolData 
{
  int dir;
};

static int iFillGetDir(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_FILL_NONE;

  if (ih->data->dir != IUP_FILL_NONE)
    return ih->data->dir;

  /* Its parent should be an IupHbox or an IupVbox. */
  if (ih->parent->iclass->nativetype == IUP_TYPEVOID)
  {
    if (IupClassMatch(ih->parent, "vbox"))
      ih->data->dir = IUP_FILL_VERT;
    else
      ih->data->dir = IUP_FILL_HORIZ;
  }

  return ih->data->dir;
}

static int iFillMapMethod(Ihandle* ih)
{
  iFillGetDir(ih);
  return iupBaseTypeVoidMapMethod(ih);
}

static void iFillUnMapMethod(Ihandle* ih)
{
  ih->data->dir = IUP_FILL_NONE;
}

static int iFillSetRasterSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    int s = 0, d = 0;
    if (iFillGetDir(ih) == IUP_FILL_NONE)  /* if Fill is not yet a child of a Vbox or Hbox */
    {
      iupAttribSet(ih, "SIZE", NULL);
      return 1;
    }

    if (iFillGetDir(ih) == IUP_FILL_HORIZ)
      iupStrToIntInt(value, &s, &d, 'x');  /* second value will be ignored, can NOT set height */
    else
    {
      iupStrToIntInt(value, &s, &d, 'x');  /* first value will be ignored if second defined, can NOT set width */
      if (d != 0) s = d;
    }

    if (s > 0) 
    {
      if (iFillGetDir(ih) == IUP_FILL_HORIZ)
      {
        ih->userwidth = s;  /* inside HBOX */
        ih->userheight = 0;
      }
      else
      {
        ih->userheight = s; /* inside VBOX */
        ih->userwidth = 0;
      }
    }
  }
  iupAttribSet(ih, "SIZE", NULL);
  return 0;
}

static int iFillSetSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    int s = 0, d = 0;
    if (iFillGetDir(ih) == IUP_FILL_NONE) /* if Fill is not yet a child of a Vbox or Hbox */
    {
      iupAttribSet(ih, "RASTERSIZE", NULL);
      return 1;
    }

    if (iFillGetDir(ih) == IUP_FILL_HORIZ)
      iupStrToIntInt(value, &s, &d, 'x');  /* second value will be ignored, can NOT set height */
    else
    {
      iupStrToIntInt(value, &s, &d, 'x');  /* first value will be ignored if second defined, can NOT set width */
      if (d != 0) s = d;
    }

    if (s > 0) 
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      if (iFillGetDir(ih) == IUP_FILL_HORIZ)
      {
        ih->userwidth = iupWIDTH2RASTER(s, charwidth);  /* inside HBOX */
        ih->userheight = 0;
      }
      else
      {
        ih->userheight = iupHEIGHT2RASTER(s, charheight); /* inside VBOX */
        ih->userwidth = 0;
      }
    }
  }
  iupAttribSet(ih, "RASTERSIZE", NULL);
  return 1;  /* always save in the hash table, so when FONT is changed SIZE can be updated */
}

static char* iFillGetExpandAttrib(Ihandle* ih)
{
  if (iFillGetDir(ih) == IUP_FILL_NONE) /* if Fill is not yet a child of a Vbox or Hbox */
    return "NO";

  /* if size is not defined, then expansion on that direction is permitted */
  if (iFillGetDir(ih) == IUP_FILL_HORIZ)
  {
    if (ih->userwidth <= 0)
      return "HORIZONTAL";
  }
  else
  {
    if (ih->userheight <= 0)
      return "VERTICAL";
  }

  return "NO";
}

static void iFillUpdateSize(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "SIZE");
  if (value) 
  { 
    iFillSetSizeAttrib(ih, value);
    iupAttribSet(ih, "SIZE", NULL);
  }
  value = iupAttribGet(ih, "RASTERSIZE");
  if (value) 
  { 
    iFillSetRasterSizeAttrib(ih, value);
    iupAttribSet(ih, "RASTERSIZE", NULL);
  }
}

static void iFillComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand;  /* unset if not a container */

  /* EXPAND is initialized as none for FILL */
  ih->expand = IUP_EXPAND_NONE;

  iFillUpdateSize(ih);

  /* always initialize the natural size using the user size,
     must do this again because of iFillUpdateSize */
  ih->naturalwidth = ih->userwidth;
  ih->naturalheight = ih->userheight;

  if (iFillGetDir(ih) == IUP_FILL_NONE) /* if Fill is not a child of a Vbox or Hbox */
    return;

  /* If size is NOT defined, then expansion on that direction is permitted.
     This type of expansion works only when inside a vbox, hbox or gridbox. */
  if (iFillGetDir(ih) == IUP_FILL_HORIZ)
  {
    if (ih->naturalwidth <= 0)
      ih->expand = IUP_EXPAND_W0;
  }
  else
  {
    if (ih->naturalheight <= 0)
      ih->expand = IUP_EXPAND_H0;
  }

  *w = ih->naturalwidth;
  *h = ih->naturalheight;
}

static int iFillCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  ih->data = iupALLOCCTRLDATA();
  return IUP_NOERROR;
}

/******************************************************************************/

IUP_API Ihandle* IupFill(void)
{
  return IupCreate("fill");
}

Iclass* iupFillNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "fill";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupFillNewClass;
  ic->Create = iFillCreateMethod;
  ic->Map = iFillMapMethod;
  ic->UnMap = iFillUnMapMethod;
  ic->ComputeNaturalSize = iFillComputeNaturalSizeMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "SIZE", iupBaseGetSizeAttrib, iFillSetSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RASTERSIZE", iupBaseGetRasterSizeAttrib, iFillSetRasterSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Base */
  iupClassRegisterAttribute(ic, "EXPAND", iFillGetExpandAttrib, NULL, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

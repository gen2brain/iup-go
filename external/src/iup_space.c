/** \file
 * \brief Space Control.
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



IUP_API Ihandle* IupSpace(void)
{
  return IupCreate("space");
}

Iclass* iupSpaceNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "space";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupSpaceNewClass;
  ic->Map = iupBaseTypeVoidMapMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  return ic;
}

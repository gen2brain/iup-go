/** \file
 * \brief User Element.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_stdcontrols.h"


static int iUserSetClearAttributesAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  iupTableClear(ih->attrib);
  return 0;
}

IUP_API Ihandle* IupUser(void)
{
  return IupCreate("user");
}

Iclass* iupUserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "user";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDMANY; /* can have children */
  ic->is_interactive = 0;

  ic->New = iupUserNewClass;

  iupBaseRegisterBaseCallbacks(ic);

  iupClassRegisterAttribute(ic, "CLEARATTRIBUTES", NULL, iUserSetClearAttributesAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  return ic;
}

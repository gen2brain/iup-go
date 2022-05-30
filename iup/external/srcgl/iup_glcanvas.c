/** \file
 * \brief IupBackgroundBox control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_drv.h"


void iupdrvGlCanvasInitClass(Iclass* ic);

static Iclass* iGlCanvasNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "glcanvas";
  ic->cons = "GLCanvas";
  ic->format = "a"; /* one ACTION callback name */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iGlCanvasNewClass;

  iupClassRegisterCallback(ic, "SWAPBUFFERS_CB", "");

  iupClassRegisterAttribute(ic, "BUFFER", NULL, NULL, IUPAF_SAMEASSYSTEM, "SINGLE", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "COLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "RGBA", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ERROR", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTEXT", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "COLORMAP", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "CONTEXTFLAGS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTEXTPROFILE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTEXTVERSION", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ARBCONTEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupdrvGlCanvasInitClass(ic);

  return ic;
}

static Iclass* iGLBackgroundBoxNewClass(void)
{
  Iclass* ic = iupBackgroundBoxNewBaseClass(iupRegisterFindClass("glcanvas"));

  ic->name = "glbackgroundbox";
  ic->cons = "GLBackgroundBox";

  return ic;
}

Ihandle* IupGLBackgroundBox(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("glbackgroundbox", children);
}

Ihandle* IupGLCanvas(const char *action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("glcanvas", params);
}

void IupGLCanvasOpen(void)
{
  if (!IupIsOpened())
    return;

  if (!IupGetGlobal("_IUP_GLCANVAS_OPEN"))
  {
    iupRegisterClass(iGlCanvasNewClass());
    iupRegisterClass(iGLBackgroundBoxNewClass());

    IupSetGlobal("_IUP_GLCANVAS_OPEN", "1");
  }
}

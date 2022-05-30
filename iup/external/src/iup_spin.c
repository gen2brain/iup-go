/** \file
 * \brief Spin control
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
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_childtree.h"



static int iSpinCallCB(Ihandle* ih, int dub, int ten, int sign)
{
  IFni cb;

  /* get the callback on the spin or on the spinbox */
  Ihandle* spinbox = (Ihandle*)iupAttribGet(ih->parent, "_IUPSPIN_BOX");
  if (spinbox) 
    ih = spinbox;
  else
    ih = ih->parent;

  cb = (IFni) IupGetCallback(ih, "SPIN_CB");
  if (cb) 
  {
    return cb(ih, sign*(dub && ten ? 100 :
                               ten ?  10 : 
                               dub ?   2 : 1));
  }

  return IUP_DEFAULT;
}

static int iSpinTimerCB(Ihandle* ih)
{
  Ihandle* spin_button = (Ihandle*)iupAttribGet(ih, "_IUPSPIN_BUTTON");
  char* status   = iupAttribGet(ih, "_IUPSPIN_STATUS");
  int spin_dir   = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  int count      = iupAttribGetInt(ih, "_IUPSPIN_COUNT");
  char* reconfig = NULL;

  if(count == 0) /* first time */
    reconfig = "50";
  else if(count == 14) /* 300 + 14*50 = 1000 (1 second) */
    reconfig = "25";
  else if(count == 34) /* 300 + 14*50 + 20*50 = 2000 (2 seconds) */
    reconfig = "10";

  if (reconfig)
  {
    IupSetAttribute(ih, "RUN", "NO");
    IupSetAttribute(ih, "TIME", reconfig);
    IupSetAttribute(ih, "RUN", "YES");
  }

  iupAttribSetInt(ih, "_IUPSPIN_COUNT", count + 1);

  return iSpinCallCB(spin_button, iup_isshift(status), iup_iscontrol(status), spin_dir);
}

static void iSpinRunTimer(Ihandle* ih, char* status, char* dir)
{
  Ihandle* spin_timer = IupGetHandle("IupSpinTimer");

  iupAttribSet(spin_timer, "_IUPSPIN_BUTTON", (char*)ih);
  iupAttribSetStr(spin_timer, "_IUPSPIN_STATUS", status);
  iupAttribSetStr(spin_timer, "_IUPSPIN_DIR",   dir);
  iupAttribSet(spin_timer, "_IUPSPIN_COUNT", "0");

  IupSetAttribute(spin_timer, "TIME",           "400");
  IupSetAttribute(spin_timer, "RUN",            "YES");
}

static void iSpinStopTimer(void)
{
  Ihandle* spin_timer = IupGetHandle("IupSpinTimer");
  IupSetAttribute(spin_timer, "RUN", "NO");
}

static int iSpinK_SP(Ihandle* ih)
{
  int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  
  return iSpinCallCB(ih, 0, 0, dir);
}

static int iSpinK_sSP(Ihandle* ih)
{
  int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  
  return iSpinCallCB(ih, 1, 0, dir);
}

static int iSpinK_cSP(Ihandle* ih)
{
  int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  
  return iSpinCallCB(ih, 0, 1, dir);
}

static int iSpinButtonCB(Ihandle* ih, int but, int pressed, int x, int y, char* status)
{
  (void)x;
  (void)y;
 
  if (pressed && but == IUP_BUTTON1 && !iup_isdouble(status))
  {
    int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
    
    iSpinRunTimer(ih, status, iupAttribGet(ih, "_IUPSPIN_DIR"));
    
    return iSpinCallCB(ih, iup_isshift(status), iup_iscontrol(status), dir);
  }
  else if (!pressed && but == IUP_BUTTON1)
    iSpinStopTimer();
  
  return IUP_DEFAULT;
}

static int iSpinCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* bt_up;
  Ihandle* bt_down;
  (void)params;

  /* Button UP */
  bt_up = IupButton(NULL, NULL);
  
  IupSetAttribute(bt_up, "EXPAND", "NO");
  IupSetAttribute(bt_up, "IMAGE",  "IupSpinUpImage");
  IupSetAttribute(bt_up, "_IUPSPIN_DIR", "1");
  IupSetAttribute(bt_up, "CANFOCUS", "NO");

  IupSetCallback(bt_up, "BUTTON_CB", (Icallback) iSpinButtonCB);
  IupSetCallback(bt_up, "K_SP",      (Icallback) iSpinK_SP);
  IupSetCallback(bt_up, "K_sSP",     (Icallback) iSpinK_sSP);
  IupSetCallback(bt_up, "K_cSP",     (Icallback) iSpinK_cSP);

  /* Button DOWN */
  bt_down = IupButton(NULL, NULL);
  
  IupSetAttribute(bt_down, "EXPAND", "NO");
  IupSetAttribute(bt_down, "IMAGE",  "IupSpinDownImage");
  IupSetAttribute(bt_down, "_IUPSPIN_DIR", "-1");
  IupSetAttribute(bt_down, "CANFOCUS", "NO");

  IupSetCallback(bt_down, "BUTTON_CB", (Icallback) iSpinButtonCB);
  IupSetCallback(bt_down, "K_SP",      (Icallback) iSpinK_SP);
  IupSetCallback(bt_down, "K_sSP",     (Icallback) iSpinK_sSP);
  IupSetCallback(bt_down, "K_cSP",     (Icallback) iSpinK_cSP);

  /* manually add the buttons as a children */
  ih->firstchild = bt_up;
  bt_up->parent = ih;
  bt_up->brother = bt_down;
  bt_down->parent = ih;
  
  /* avoid inheritance from parent */
  IupSetAttribute(ih, "GAP",    "0");
  IupSetAttribute(ih, "MARGIN", "0x0");

  return IUP_NOERROR;
}


static void iSpinLoadImages(void)
{
  Ihandle* img;

  /* Spin UP image */
  unsigned char iupspin_up_img[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  /* Spin DOWN image */
  unsigned char iupspin_down_img[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 0, 0, 0, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1
  };

  img = IupImage(9, 6, iupspin_up_img);
  IupSetAttribute(img, "0", "0 0 0"); 
  IupSetAttribute(img, "1", "BGCOLOR"); 
  IupSetHandle("IupSpinUpImage", img); 

  img = IupImage(9, 6, iupspin_down_img);
  IupSetAttribute(img, "0", "0 0 0"); 
  IupSetAttribute(img, "1", "BGCOLOR"); 
  IupSetHandle("IupSpinDownImage", img); 
}

Iclass* iupSpinNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("vbox"));

  ic->name = "spin";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupSpinNewClass;
  ic->Create = iSpinCreateMethod;

  iupClassRegisterCallback(ic, "SPIN_CB", "i");

  if (!IupGetHandle("IupSpinUpImage") || !IupGetHandle("IupSpinDownImage"))
  {
    Ihandle* spin_timer = IupTimer();
    IupSetCallback(spin_timer, "ACTION_CB", (Icallback) iSpinTimerCB);
    IupSetHandle("IupSpinTimer", spin_timer);

    iSpinLoadImages();
  }

  return ic;
}

IUP_API Ihandle* IupSpin(void)
{
  return IupCreate("spin");
}

/**************************************************************************************
                                      SPINBOX
**************************************************************************************/

static char* iSpinboxGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  width -= ih->firstchild->currentwidth;
  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static void iSpinboxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  /* update spin natural size */
  iupBaseComputeNaturalSize(ih->firstchild);

  if (ih->firstchild->brother)
  {
    /* update child natural size */
    iupBaseComputeNaturalSize(ih->firstchild->brother);

    *children_expand = ih->firstchild->brother->expand;

    *w = ih->firstchild->brother->naturalwidth + ih->firstchild->naturalwidth;
    *h = iupMAX(ih->firstchild->brother->naturalheight, ih->firstchild->naturalheight);
  }
  else
  {
    *w = ih->firstchild->naturalwidth;
    *h = ih->firstchild->naturalheight;
  }
}

static void iSpinboxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  /* bar */                            
  iupBaseSetCurrentSize(ih->firstchild, ih->firstchild->naturalwidth, ih->firstchild->naturalheight, shrink);

  if (ih->firstchild->brother)
  {
    /* child */
    int width = ih->currentwidth - ih->firstchild->naturalwidth;
    if (width < 0) width = 0;
    iupBaseSetCurrentSize(ih->firstchild->brother, width, ih->currentheight, shrink);
  }
}

static void iSpinboxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild->brother)
  { 
    if (ih->firstchild->brother->currentheight < ih->firstchild->currentheight)
    {
      /* bar */
      iupBaseSetPosition(ih->firstchild, x+ih->firstchild->brother->currentwidth, y);

      y += (ih->firstchild->currentheight-ih->firstchild->brother->currentheight)/2;

      /* child */
      iupBaseSetPosition(ih->firstchild->brother, x, y);
    }
    else
    {
      /* child */
      iupBaseSetPosition(ih->firstchild->brother, x, y);

      y += (ih->firstchild->brother->currentheight-ih->firstchild->currentheight)/2;

      /* bar */
      iupBaseSetPosition(ih->firstchild, x+ih->firstchild->brother->currentwidth, y);
    }
  } 
  else
  {
    /* bar */
    iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static int iSpinboxCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *spin = IupSpin();
  spin->flags |= IUP_INTERNAL;
  iupChildTreeAppend(ih, spin);  /* spin will always be the firstchild */

  iupAttribSet(spin, "_IUPSPIN_BOX", (char*)ih);  /* will be used by the callback */

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }

  return IUP_NOERROR;
}

Iclass* iupSpinboxNewClass(void)
{
  /* we don't inherit from a Hbox here to always position the spin at right */
  Iclass* ic = iupClassNew(NULL);

  ic->name = "spinbox";
  ic->format = "h"; /* one Ihandle* */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY+2;  /* spin+child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupSpinboxNewClass;
  ic->Create = iSpinboxCreateMethod;
  ic->ComputeNaturalSize = iSpinboxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iSpinboxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iSpinboxSetChildrenPositionMethod;
  ic->Map = iupBaseTypeVoidMapMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Callbacks */
  iupClassRegisterCallback(ic, "SPIN_CB", "i");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iSpinboxGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupSpinbox(Ihandle* ctrl)
{
  void *children[2];
  children[0] = (void*)ctrl;
  children[1] = NULL;
  return IupCreatev("spinbox", children);
}

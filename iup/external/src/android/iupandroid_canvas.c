/** \file
 * \brief Canvas Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_classbase.h"
#include "iup_key.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupCanvasHelper);

static int androidCanvasMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupCanvasHelper_createCanvas);

  /* Cache SCROLLBAR mask so POSX/POSY setters accept writes. */
  ih->data->sb = iupBaseGetScrollbar(ih);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCanvasHelper, jni_env, "io/github/gen2brain/iupgo/IupCanvasHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupCanvasHelper_createCanvas, jni_env, java_class, "createCanvas", "(J)Lio/github/gen2brain/iupgo/IupAndroidCanvas;");

  jobject view = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.createCanvas");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!view)
    return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, view));
  (*jni_env)->DeleteLocalRef(jni_env, view);

  iupAndroid_AddWidgetToParent(jni_env, ih);
  return IUP_NOERROR;
}

/* DRAWSIZE = canvas-coord (currentwidth/density), ceiled for exact round-trip. */
static char* androidCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
  int w = (int)ceilf((float)ih->currentwidth  / d);
  int h = (int)ceilf((float)ih->currentheight / d);
  return iupStrReturnIntInt(w, h, 'x');
}

/* No native scrollbars; track DX/DY/POSX/POSY and fire SCROLL_CB on a position change. */
static int androidCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  double dx;
  if (iupStrToDoubleDef(value, &dx, 0.1))
    iupAttribSetDouble(ih, "DX", dx);
  return 1;
}

static int androidCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  double dy;
  if (iupStrToDoubleDef(value, &dy, 0.1))
    iupAttribSetDouble(ih, "DY", dy);
  return 1;
}

static int androidCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double posx, xmin, xmax, dx;
    if (!iupStrToDouble(value, &posx)) return 1;
    xmin = iupAttribGetDouble(ih, "XMIN");
    xmax = iupAttribGetDouble(ih, "XMAX");
    dx = iupAttribGetDouble(ih, "DX");
    if (dx >= xmax - xmin) return 0;
    if (posx < xmin) posx = xmin;
    if (posx > (xmax - dx)) posx = xmax - dx;
    ih->data->posx = posx;

    IFniff cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (cb) cb(ih, IUP_SBPOSH, (float)posx, (float)ih->data->posy);
  }
  return 1;
}

static int androidCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double posy, ymin, ymax, dy;
    if (!iupStrToDouble(value, &posy)) return 1;
    ymin = iupAttribGetDouble(ih, "YMIN");
    ymax = iupAttribGetDouble(ih, "YMAX");
    dy = iupAttribGetDouble(ih, "DY");
    if (dy >= ymax - ymin) return 0;
    if (posy < ymin) posy = ymin;
    if (posy > (ymax - dy)) posy = ymax - dy;
    ih->data->posy = posy;

    IFniff cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (cb) cb(ih, IUP_SBPOSV, (float)ih->data->posx, (float)posy);
  }
  return 1;
}

void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = androidCanvasMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "DRAWSIZE", androidCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, androidCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, androidCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DX", NULL, androidCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, androidCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "TOUCH_CB", "iiis");
  iupClassRegisterCallback(ic, "MULTITOUCH_CB", "iIII");
  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GESTURE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

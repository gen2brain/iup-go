/** \file
 * \brief Camera control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupdraw.h"
#include "iupmedia.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"
#include "iup_thread.h"
#include "iup_stdcontrols.h"
#include "iup_media.h"
#include "iup_camera.h"

enum { ICAMERA_MSG_FRAME, ICAMERA_MSG_ERROR, ICAMERA_MSG_PERMISSION };

typedef struct _IcameraMsg
{
  int type;
  int granted;
  char message[256];
} IcameraMsg;

struct _IcontrolData
{
  iupCanvas canvas;
  void* mutex;
  unsigned char* pending;
  int pending_width, pending_height, pending_posted;
  unsigned char* frame;
  int frame_width, frame_height;
  Ihandle* image;
  char image_name[40];
  int running;
  int width, height, fps;
};

static void iCameraPost(Ihandle* ih, const IcameraMsg* msg)
{
  IcameraMsg* copy = (IcameraMsg*)malloc(sizeof(IcameraMsg));
  if (!copy)
    return;
  *copy = *msg;
  IupPostMessage(ih, NULL, 0, 0, copy);
}

void iupCameraFrame(Ihandle* ih, const unsigned char* rgb, int width, int height)
{
  IcameraMsg msg;
  int size = width * height * 3;
  int post = 0;

  iupdrvMutexLock(ih->data->mutex);
  if (ih->data->pending_width != width || ih->data->pending_height != height)
  {
    unsigned char* buffer = (unsigned char*)realloc(ih->data->pending, size);
    if (!buffer)
    {
      iupdrvMutexUnlock(ih->data->mutex);
      return;
    }
    ih->data->pending = buffer;
    ih->data->pending_width = width;
    ih->data->pending_height = height;
  }
  memcpy(ih->data->pending, rgb, size);
  if (!ih->data->pending_posted)
  {
    ih->data->pending_posted = 1;
    post = 1;
  }
  iupdrvMutexUnlock(ih->data->mutex);

  if (post)
  {
    msg.type = ICAMERA_MSG_FRAME;
    iCameraPost(ih, &msg);
  }
}

void iupCameraError(Ihandle* ih, const char* message)
{
  IcameraMsg msg;
  msg.type = ICAMERA_MSG_ERROR;
  strncpy(msg.message, message ? message : "", sizeof(msg.message) - 1);
  msg.message[sizeof(msg.message) - 1] = 0;
  iCameraPost(ih, &msg);
}

void iupCameraPermission(Ihandle* ih, int granted)
{
  IcameraMsg msg;
  msg.type = ICAMERA_MSG_PERMISSION;
  msg.granted = granted;
  iCameraPost(ih, &msg);
}

static void iCameraTakeFrame(Ihandle* ih)
{
  unsigned char* buffer;
  int width, height;

  iupdrvMutexLock(ih->data->mutex);
  buffer = ih->data->frame;
  width = ih->data->frame_width;
  height = ih->data->frame_height;
  ih->data->frame = ih->data->pending;
  ih->data->frame_width = ih->data->pending_width;
  ih->data->frame_height = ih->data->pending_height;
  ih->data->pending = buffer;
  ih->data->pending_width = width;
  ih->data->pending_height = height;
  ih->data->pending_posted = 0;
  iupdrvMutexUnlock(ih->data->mutex);
}

void iupCameraRotate(const unsigned char* src, int width, int height, int orientation, unsigned char* dst)
{
  int x, y;

  if (orientation == 90)
  {
    for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
      {
        const unsigned char* p = src + (y * width + x) * 3;
        unsigned char* q = dst + (x * height + (height - 1 - y)) * 3;
        q[0] = p[0]; q[1] = p[1]; q[2] = p[2];
      }
    }
  }
  else if (orientation == 270)
  {
    for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
      {
        const unsigned char* p = src + (y * width + x) * 3;
        unsigned char* q = dst + ((width - 1 - x) * height + y) * 3;
        q[0] = p[0]; q[1] = p[1]; q[2] = p[2];
      }
    }
  }
  else
  {
    for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
      {
        const unsigned char* p = src + (y * width + x) * 3;
        unsigned char* q = dst + ((height - 1 - y) * width + (width - 1 - x)) * 3;
        q[0] = p[0]; q[1] = p[1]; q[2] = p[2];
      }
    }
  }
}

static void iCameraMirror(unsigned char* frame, int width, int height)
{
  int x, y;
  for (y = 0; y < height; y++)
  {
    unsigned char* left = frame + y * width * 3;
    unsigned char* right = left + (width - 1) * 3;
    for (x = 0; x < width / 2; x++)
    {
      unsigned char r = left[0], g = left[1], b = left[2];
      left[0] = right[0]; left[1] = right[1]; left[2] = right[2];
      right[0] = r; right[1] = g; right[2] = b;
      left += 3;
      right -= 3;
    }
  }
}

static void iCameraReleaseImage(Ihandle* ih)
{
  if (ih->data->image)
  {
    IupSetHandle(ih->data->image_name, NULL);
    IupDestroy(ih->data->image);
    ih->data->image = NULL;
  }
}

static void iCameraUpdateImage(Ihandle* ih)
{
  int width = ih->data->frame_width;
  int height = ih->data->frame_height;

  if (ih->data->image && (IupGetInt(ih->data->image, "WIDTH") != width || IupGetInt(ih->data->image, "HEIGHT") != height))
    iCameraReleaseImage(ih);

  if (!ih->data->image)
  {
    ih->data->image = IupImageRGB(width, height, ih->data->frame);
    if (!ih->data->image)
      return;
    IupSetHandle(ih->data->image_name, ih->data->image);
  }
  else
  {
    unsigned char* imgdata = (unsigned char*)iupAttribGet(ih->data->image, "WID");
    memcpy(imgdata, ih->data->frame, width * height * 3);
    IupSetAttribute(ih->data->image, "CLEARCACHE", "YES");
  }
}

static void iCameraStop(Ihandle* ih)
{
  if (!ih->data->running)
    return;

  iupdrvCameraStop(ih);
  ih->data->running = 0;
}

static int iCameraPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  IcameraMsg* msg = (IcameraMsg*)p;
  (void)s; (void)i; (void)d;

  if (!msg)
    return IUP_DEFAULT;

  if (msg->type == ICAMERA_MSG_FRAME)
  {
    IFniiV cb;
    int ret = IUP_DEFAULT;

    iCameraTakeFrame(ih);

    if (ih->data->running && ih->data->frame)
    {
      if (iupAttribGetBoolean(ih, "MIRROR"))
        iCameraMirror(ih->data->frame, ih->data->frame_width, ih->data->frame_height);

      cb = (IFniiV)IupGetCallback(ih, "FRAME_CB");
      if (cb)
        ret = cb(ih, ih->data->frame_width, ih->data->frame_height, ih->data->frame);

      if (ret == IUP_CLOSE)
        IupExitLoop();
      else if (ret != IUP_IGNORE)
      {
        iCameraUpdateImage(ih);
        IupUpdate(ih);
      }
    }
  }
  else if (msg->type == ICAMERA_MSG_ERROR)
  {
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    iCameraStop(ih);
    if (cb && cb(ih, msg->message) == IUP_CLOSE)
      IupExitLoop();
  }
  else if (msg->type == ICAMERA_MSG_PERMISSION)
  {
    IFni cb = (IFni)IupGetCallback(ih, "PERMISSION_CB");
    if (cb && cb(ih, msg->granted) == IUP_CLOSE)
      IupExitLoop();
  }

  free(msg);
  return IUP_DEFAULT;
}

static int iCameraRedraw(Ihandle* ih)
{
  int w, h;

  IupDrawBegin(ih);
  IupDrawGetSize(ih, &w, &h);

  iupAttribSetStr(ih, "DRAWCOLOR", iupAttribGetStr(ih, "BGCOLOR"));
  iupAttribSet(ih, "DRAWSTYLE", "FILL");
  IupDrawRectangle(ih, 0, 0, w - 1, h - 1);

  if (ih->data->image)
  {
    int fw = ih->data->frame_width, fh = ih->data->frame_height;
    int dw = w, dh = h, x = 0, y = 0;

    if (fw * h > fh * w)
    {
      dh = (fh * w) / fw;
      y = (h - dh) / 2;
    }
    else
    {
      dw = (fw * h) / fh;
      x = (w - dw) / 2;
    }

    IupDrawImage(ih, ih->data->image_name, x, y, dw, dh);
  }

  IupDrawEnd(ih);
  return IUP_DEFAULT;
}

static int iCameraStart(Ihandle* ih, int device)
{
  int width = 640, height = 480, fps = 30;

  iupStrToIntInt(iupAttribGetStr(ih, "RESOLUTION"), &width, &height, 'x');
  iupStrToInt(iupAttribGetStr(ih, "FPS"), &fps);

  if (!iupdrvCameraStart(ih, device, &width, &height, &fps))
    return 0;

  ih->data->running = 1;
  ih->data->width = width;
  ih->data->height = height;
  ih->data->fps = fps;
  return 1;
}

static int iCameraSetRunAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!ih->data->running)
      iCameraStart(ih, iupAttribGetInt(ih, "DEVICE"));
  }
  else
    iCameraStop(ih);
  return 0;
}

static char* iCameraGetRunAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->running);
}

static int iCameraSetDeviceAttrib(Ihandle* ih, const char* value)
{
  int device;
  if (!iupStrToInt(value, &device) || device < 0)
    return 0;

  if (ih->data->running)
  {
    iCameraStop(ih);
    iCameraStart(ih, device);
  }
  return 1;
}

static char* iCameraGetResolutionAttrib(Ihandle* ih)
{
  if (!ih->data->running)
    return NULL;
  if (ih->data->frame)
    return iupStrReturnStrf("%dx%d", ih->data->frame_width, ih->data->frame_height);
  return iupStrReturnStrf("%dx%d", ih->data->width, ih->data->height);
}

static char* iCameraGetFpsAttrib(Ihandle* ih)
{
  if (ih->data->running)
    return iupStrReturnInt(ih->data->fps);
  return NULL;
}

static char* iCameraGetDeviceCountAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnInt(iupdrvCameraGetDeviceCount());
}

static char* iCameraGetDeviceNameAttrib(Ihandle* ih, int id)
{
  (void)ih;
  return iupdrvCameraGetDeviceName(id);
}

static char* iCameraGetAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(iupdrvCameraIsAvailable());
}

static char* iCameraGetPermissionAttrib(Ihandle* ih)
{
  return iupdrvCameraGetPermission(ih);
}

static int iCameraSetSnapshotAttrib(Ihandle* ih, const char* value)
{
  if (value && ih->data->image)
    IupImageSave(ih->data->image, value, NULL);
  return 0;
}

static int iCameraCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  free(ih->data);
  ih->data = iupALLOCCTRLDATA();
  ih->data->mutex = iupdrvMutexCreate();
  sprintf(ih->data->image_name, "_IUP_CAMERA_%p", (void*)ih);

  IupSetCallback(ih, "ACTION", (Icallback)iCameraRedraw);
  IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback)iCameraPostMessage);
  return IUP_NOERROR;
}

static void iCameraUnMapMethod(Ihandle* ih)
{
  iCameraStop(ih);
}

static void iCameraDestroyMethod(Ihandle* ih)
{
  iCameraStop(ih);
  iCameraReleaseImage(ih);
  iupdrvMutexDestroy(ih->data->mutex);
  free(ih->data->pending);
  free(ih->data->frame);
}

IUPMEDIA_API Ihandle* IupCamera(void)
{
  return IupCreate("camera");
}

Iclass* iupCameraNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "camera";
  ic->cons = "Camera";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  ic->New = iupCameraNewClass;
  ic->Create = iCameraCreateMethod;
  ic->UnMap = iCameraUnMapMethod;
  ic->Destroy = iCameraDestroyMethod;

  iupClassRegisterCallback(ic, "FRAME_CB", "iiV");
  iupClassRegisterCallback(ic, "PERMISSION_CB", "i");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribDef(ic, "CANFOCUS", "NO", NULL);

  iupClassRegisterAttribute(ic, "DEVICE", NULL, iCameraSetDeviceAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DEVICECOUNT", iCameraGetDeviceCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEVICENAME", iCameraGetDeviceNameAttrib, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RUN", iCameraGetRunAttrib, iCameraSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESOLUTION", iCameraGetResolutionAttrib, NULL, IUPAF_SAMEASSYSTEM, "640x480", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FPS", iCameraGetFpsAttrib, NULL, IUPAF_SAMEASSYSTEM, "30", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MIRROR", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SNAPSHOT", NULL, iCameraSetSnapshotAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AVAILABLE", iCameraGetAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PERMISSION", iCameraGetPermissionAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupdrvCameraInitClass(ic);

  return ic;
}

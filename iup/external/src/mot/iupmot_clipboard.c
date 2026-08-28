/** \file
 * \brief Clipboard for the Motif Driver.
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include <Xm/CutPaste.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupmot_drv.h"


static Window motClipboardGetWindow(void)
{
  Ihandle* focus = IupGetFocus();
  if (focus)
  {
    Ihandle* dlg = IupGetDialog(focus);
    if (dlg && dlg->handle)
    {
      Window w = XtWindow(dlg->handle);
      if (w) return w;
    }
  }

  return XtWindow(iupmot_appshell);
}

typedef struct {
  Atom target;
  Atom type;
  int format;
  int is_text;
  char* data;
  unsigned long size;
} motPrimaryBuffer;

static motPrimaryBuffer mot_primary = { 0, 0, 0, 0, NULL, 0 };

typedef struct {
  int state;
  char* data;
  unsigned long size;
  Atom type;
  int format;
} motPrimaryRequest;

static int motClipboardIsPrimary(Ihandle *ih)
{
  return iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTION"), "PRIMARY");
}

/* Xlib returns and expects format 32 data as an array of long */
static unsigned long motPrimaryUnitSize(int format)
{
  return (format == 32)? sizeof(long): (unsigned long)(format / 8);
}

static Atom motPrimaryTargetsAtom(void)
{
  return XInternAtom(iupmot_display, "TARGETS", False);
}

static int motPrimaryIsTextTarget(Atom target)
{
  return target == XA_STRING ||
         target == XInternAtom(iupmot_display, "UTF8_STRING", False) ||
         target == XInternAtom(iupmot_display, "TEXT", False);
}

static Boolean motPrimaryConvertProc(Widget w, Atom *selection, Atom *target, Atom *type, XtPointer *value, unsigned long *length, int *format)
{
  (void)w;
  (void)selection;

  if (!mot_primary.data)
    return False;

  if (*target == motPrimaryTargetsAtom())
  {
    Atom* targets;
    int count = mot_primary.is_text? 4: 2;

    targets = (Atom*)XtMalloc(count * sizeof(Atom));
    targets[0] = motPrimaryTargetsAtom();
    if (mot_primary.is_text)
    {
      targets[1] = XInternAtom(iupmot_display, "UTF8_STRING", False);
      targets[2] = XA_STRING;
      targets[3] = XInternAtom(iupmot_display, "TEXT", False);
    }
    else
      targets[1] = mot_primary.target;

    *type = XA_ATOM;
    *value = (XtPointer)targets;
    *length = count;
    *format = 32;
    return True;
  }

  if (*target == mot_primary.target || (mot_primary.is_text && motPrimaryIsTextTarget(*target)))
  {
    unsigned long bytes = mot_primary.size * motPrimaryUnitSize(mot_primary.format);
    char* copy = XtMalloc(bytes? bytes: 1);
    memcpy(copy, mot_primary.data, bytes);

    *type = mot_primary.is_text? *target: mot_primary.type;
    *value = (XtPointer)copy;
    *length = mot_primary.size;
    *format = mot_primary.format;
    return True;
  }

  return False;
}

static void motPrimaryLoseProc(Widget w, Atom *selection)
{
  (void)w;
  (void)selection;

  if (mot_primary.data)
  {
    free(mot_primary.data);
    mot_primary.data = NULL;
  }
  mot_primary.size = 0;
}

static void motPrimaryOwn(Atom target, Atom type, int format, int is_text, const void* data, unsigned long size)
{
  unsigned long bytes = size * motPrimaryUnitSize(format);

  if (mot_primary.data)
    free(mot_primary.data);

  mot_primary.data = (char*)malloc(bytes? bytes: 1);
  if (!mot_primary.data)
    return;

  memcpy(mot_primary.data, data, bytes);
  mot_primary.target = target;
  mot_primary.type = type;
  mot_primary.format = format;
  mot_primary.is_text = is_text;
  mot_primary.size = size;

  XtOwnSelection(iupmot_appshell, XA_PRIMARY, XtLastTimestampProcessed(iupmot_display),
                 motPrimaryConvertProc, motPrimaryLoseProc, NULL);
}

static void motPrimaryDisown(void)
{
  XtDisownSelection(iupmot_appshell, XA_PRIMARY, XtLastTimestampProcessed(iupmot_display));
  motPrimaryLoseProc(iupmot_appshell, NULL);
}

static void motPrimaryRequestCb(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, unsigned long *length, int *format)
{
  motPrimaryRequest* request = (motPrimaryRequest*)client_data;

  (void)w;
  (void)selection;

  if (value && *type != None && *length > 0)
  {
    unsigned long bytes = (*length) * motPrimaryUnitSize(*format);
    request->data = (char*)malloc(bytes + 1);
    if (request->data)
    {
      memcpy(request->data, value, bytes);
      request->data[bytes] = 0;
      request->size = *length;
      request->type = *type;
      request->format = *format;
    }
  }

  if (value)
    XtFree((char*)value);

  request->state = 1;
}

static void motPrimaryTimeoutCb(XtPointer client_data, XtIntervalId *id)
{
  motPrimaryRequest* request = (motPrimaryRequest*)client_data;
  (void)id;
  request->state = -1;
}

/* an owner that never answers must not hang the getter */
static int motPrimaryRequestTarget(Atom target, motPrimaryRequest* request)
{
  XtAppContext app = XtWidgetToApplicationContext(iupmot_appshell);
  XtIntervalId timeout;

  memset(request, 0, sizeof(motPrimaryRequest));

  XtGetSelectionValue(iupmot_appshell, XA_PRIMARY, target, motPrimaryRequestCb, request,
                      XtLastTimestampProcessed(iupmot_display));

  timeout = XtAppAddTimeOut(app, 2000, motPrimaryTimeoutCb, request);

  while (request->state == 0)
    XtAppProcessEvent(app, XtIMAll);

  if (request->state == 1)
    XtRemoveTimeOut(timeout);

  return request->data != NULL;
}

static int motPrimaryHasTarget(Atom target, int any_text)
{
  motPrimaryRequest request;
  int found = 0;

  if (motPrimaryRequestTarget(motPrimaryTargetsAtom(), &request))
  {
    Atom* targets = (Atom*)request.data;
    unsigned long i;

    for (i = 0; i < request.size; i++)
    {
      if (targets[i] == target || (any_text && motPrimaryIsTextTarget(targets[i])))
      {
        found = 1;
        break;
      }
    }
  }

  if (request.data)
    free(request.data);

  return found;
}

static char* motPrimaryGetText(void)
{
  motPrimaryRequest request;
  char* value = NULL;

  if (!motPrimaryRequestTarget(XInternAtom(iupmot_display, "UTF8_STRING", False), &request))
    motPrimaryRequestTarget(XA_STRING, &request);

  if (request.data)
  {
    value = iupStrReturnStr(request.data);
    free(request.data);
  }

  return value;
}

static int motClipboardSetTextAttrib(Ihandle *ih, const char *value)
{
  long item_id = 0;
  Window window;
  XmString clip_label;

  if (motClipboardIsPrimary(ih))
  {
    if (!value)
      motPrimaryDisown();
    else
      motPrimaryOwn(XA_STRING, XA_STRING, 8, 1, value, (unsigned long)strlen(value));
    return 0;
  }

  window = motClipboardGetWindow();

  if (!value)
  {
    XmClipboardUndoCopy(iupmot_display, window);
    return 0;
  }

  clip_label = XmStringCreateLocalized ("IupClipboard");

  if (XmClipboardStartCopy(iupmot_display, window, clip_label, CurrentTime, NULL, NULL, &item_id)!=ClipboardSuccess)
  {
    XmStringFree(clip_label);
    return 0;
  }

  XmClipboardCopy(iupmot_display, window, item_id, "STRING", (char*)value, (long)strlen(value)+1, 0, NULL);
  XmClipboardEndCopy(iupmot_display, window, item_id);

  XmStringFree(clip_label);
  return 0;
}

static char* motClipboardGetTextAttrib(Ihandle *ih)
{
  unsigned long size;
  char* str;
  Window window;

  if (motClipboardIsPrimary(ih))
    return motPrimaryGetText();

  window = motClipboardGetWindow();

  if (XmClipboardInquireLength(iupmot_display, window, "STRING", &size)!=ClipboardSuccess)
    return NULL;

  str = iupStrGetMemory(size+1);

  if (XmClipboardRetrieve(iupmot_display, window, "STRING", str, size+1, NULL, NULL)!=ClipboardSuccess)
    return NULL;

  return str;
}

static int motClipboardSetImageAttrib(Ihandle *ih, const char *value)
{
  Pixmap pixmap;
  long item_id = 0;
  Window window;
  XmString clip_label;

  if (motClipboardIsPrimary(ih))
  {
    if (!value)
      motPrimaryDisown();
    else
    {
      pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
      motPrimaryOwn(XA_PIXMAP, XA_PIXMAP, 32, 0, &pixmap, 1);
    }
    return 0;
  }

  window = motClipboardGetWindow();

  if (!value)
  {
    XmClipboardUndoCopy(iupmot_display, window);
    return 0;
  }

  clip_label = XmStringCreateLocalized ("IupClipboard");

  if (XmClipboardStartCopy(iupmot_display, window, clip_label, CurrentTime, NULL, NULL, &item_id)!=ClipboardSuccess)
  {
    XmStringFree(clip_label);
    return 0;
  }

  pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);

  XmClipboardCopy(iupmot_display, window, item_id, "PIXMAP", (char*)&pixmap, sizeof(Pixmap), 0, NULL);
  XmClipboardEndCopy(iupmot_display, window, item_id);

  XmStringFree(clip_label);

  (void)ih;
  return 0;
}

static int motClipboardSetNativeImageAttrib(Ihandle *ih, const char *value)
{
  long item_id = 0;
  Window window;
  XmString clip_label;
  Pixmap pixmap = (Pixmap)value;

  if (motClipboardIsPrimary(ih))
  {
    if (!value)
      motPrimaryDisown();
    else
      motPrimaryOwn(XA_PIXMAP, XA_PIXMAP, 32, 0, &pixmap, 1);
    return 0;
  }

  window = motClipboardGetWindow();

  if (!value)
  {
    XmClipboardUndoCopy(iupmot_display, window);
    return 0;
  }

  clip_label = XmStringCreateLocalized ("IupClipboard");

  if (XmClipboardStartCopy(iupmot_display, window, clip_label, CurrentTime, NULL, NULL, &item_id)!=ClipboardSuccess)
  {
    XmStringFree(clip_label);
    return 0;
  }

  XmClipboardCopy(iupmot_display, window, item_id, "PIXMAP", (char*)&pixmap, sizeof(Pixmap), 0, NULL);
  XmClipboardEndCopy(iupmot_display, window, item_id);

  XmStringFree(clip_label);

  (void)ih;
  return 0;
}

static char* motClipboardGetNativeImageAttrib(Ihandle *ih)
{
  unsigned long size;
  void* data;
  Pixmap pixmap;
  Window window;

  if (motClipboardIsPrimary(ih))
  {
    motPrimaryRequest request;

    if (!motPrimaryRequestTarget(XA_PIXMAP, &request))
      return NULL;

    pixmap = (request.format == 32 && request.size >= 1)? (Pixmap)(*((unsigned long*)request.data)): 0;
    free(request.data);
    return (char*)pixmap;
  }

  window = motClipboardGetWindow();

  if (XmClipboardInquireLength(iupmot_display, window, "PIXMAP", &size)!=ClipboardSuccess)
    return NULL;

  data = XtMalloc(size);

  if (XmClipboardRetrieve(iupmot_display, window, "PIXMAP", data, size, NULL, NULL)!=ClipboardSuccess)
    return NULL;

  pixmap = *((Pixmap*)data);
  XtFree(data);
  return (char*)pixmap;
}

static int motClipboardSetFormatDataAttrib(Ihandle *ih, const char *value)
{
  int size;
  long item_id = 0;
  Window window;
  XmString clip_label;
  char* format;

  if (!value)
  {
    if (motClipboardIsPrimary(ih))
      motPrimaryDisown();
    else
      XmClipboardUndoCopy(iupmot_display, motClipboardGetWindow());
    return 0;
  }

  size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (size <= 0)
    return 0;

  format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return 0;

  if (motClipboardIsPrimary(ih))
  {
    Atom target = XInternAtom(iupmot_display, format, False);
    motPrimaryOwn(target, target, 8, 0, value, (unsigned long)size);
    return 0;
  }

  window = motClipboardGetWindow();
  clip_label = XmStringCreateLocalized ("IupClipboard");

  if (XmClipboardStartCopy(iupmot_display, window, clip_label, CurrentTime, NULL, NULL, &item_id)!=ClipboardSuccess)
  {
    XmStringFree(clip_label);
    return 0;
  }

  XmClipboardCopy(iupmot_display, window, item_id, format, (char*)value, (long)size, 0, NULL);
  XmClipboardEndCopy(iupmot_display, window, item_id);

  XmStringFree(clip_label);
  return 0;
}

static char* motClipboardGetFormatDataAttrib(Ihandle *ih)
{
  unsigned long size;
  void* data;
  Window window;

  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return 0;

  if (motClipboardIsPrimary(ih))
  {
    motPrimaryRequest request;
    unsigned long bytes;

    if (!motPrimaryRequestTarget(XInternAtom(iupmot_display, format, False), &request))
      return NULL;

    bytes = request.size * motPrimaryUnitSize(request.format);
    data = iupStrGetMemory((int)bytes + 1);
    memcpy(data, request.data, bytes);
    free(request.data);

    iupAttribSetInt(ih, "FORMATDATASIZE", (int)bytes);
    return data;
  }

  window = motClipboardGetWindow();

  /*  number of bytes of data */
  if (XmClipboardInquireLength(iupmot_display, window, format, &size)!=ClipboardSuccess)
    return NULL;

  data = iupStrGetMemory(size);

  if (XmClipboardRetrieve(iupmot_display, window, format, data, size, NULL, NULL)!=ClipboardSuccess)
    return NULL;

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return data;
}

static int motClipboardIsAvailable(const char* format_name)
{
  Window window = motClipboardGetWindow();
  int count, i;
  unsigned long max_length, length;
  char* str;

  /*  number of targets that exists on the clipboard */
  if (XmClipboardInquireCount(iupmot_display, window, &count, &max_length) != ClipboardSuccess)
    return 0;

  str = iupStrGetMemory(max_length+1);

  for (i = 1; i<=count; i++)
  {
    if (XmClipboardInquireFormat(iupmot_display, window, i, str, max_length+1, &length)==ClipboardSuccess)
    {
      if (iupStrEqualNoCase(str, format_name))
        return 1;
    }
  }

  return 0;
}

static char* motClipboardGetTextAvailableAttrib(Ihandle *ih)
{
  if (motClipboardIsPrimary(ih))
    return iupStrReturnBoolean (motPrimaryHasTarget(XA_STRING, 1));

  return iupStrReturnBoolean (motClipboardIsAvailable("STRING"));
}

static char* motClipboardGetImageAvailableAttrib(Ihandle *ih)
{
  if (motClipboardIsPrimary(ih))
    return iupStrReturnBoolean (motPrimaryHasTarget(XA_PIXMAP, 0));

  return iupStrReturnBoolean (motClipboardIsAvailable("PIXMAP"));
}

static char* motClipboardGetFormatAvailableAttrib(Ihandle *ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return NULL;

  if (motClipboardIsPrimary(ih))
    return iupStrReturnBoolean (motPrimaryHasTarget(XInternAtom(iupmot_display, format, False), 0));

  return iupStrReturnBoolean (motClipboardIsAvailable(format));
}

static char* motClipboardGetFormatDataStringAttrib(Ihandle *ih)
{
  char* data = motClipboardGetFormatDataAttrib(ih);
  if (!data)
    return NULL;

  {
    int size = iupAttribGetInt(ih, "FORMATDATASIZE");
    data[size] = 0;
    return iupStrReturnStr(data);
  }
}

static int motClipboardSetFormatDataStringAttrib(Ihandle *ih, const char *value)
{
  if (value)
  {
    int len = (int)strlen(value);
    iupAttribSetInt(ih, "FORMATDATASIZE", len + 1);
    return motClipboardSetFormatDataAttrib(ih, value);
  }
  else
    return motClipboardSetFormatDataAttrib(ih, NULL);
}

static int motClipboardSetAddFormatAttrib(Ihandle *ih, const char *value)
{
  if (value)
    XmClipboardRegisterFormat(iupmot_display, (char*)value, 8);

  (void)ih;
  return 0;
}

/******************************************************************************/

IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "clipboard";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  /* Attribute functions */
  iupClassRegisterAttribute(ic, "TEXT", motClipboardGetTextAttrib, motClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", motClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", motClipboardGetNativeImageAttrib, motClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, motClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", motClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, motClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", motClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", motClipboardGetFormatDataAttrib, motClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", motClipboardGetFormatDataStringAttrib, motClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTION", NULL, NULL, "CLIPBOARD", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

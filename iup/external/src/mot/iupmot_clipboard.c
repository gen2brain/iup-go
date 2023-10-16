/** \file
 * \brief Clipboard for the Motif Driver.
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdio.h>
#include <stdlib.h>

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
  Ihandle* dlg;
  if (!focus) return (Window)NULL;
  dlg = IupGetDialog(focus);
  if (dlg)
    return XtWindow(dlg->handle);
  else
    return (Window)NULL;
}

static int motClipboardSetTextAttrib(Ihandle *ih, const char *value)
{
  long item_id = 0;
  Window window = motClipboardGetWindow();
  XmString clip_label;
  (void)ih;

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
  Window window = motClipboardGetWindow();
  (void)ih;
  
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
  Window window = motClipboardGetWindow();
  XmString clip_label;

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
  Window window = motClipboardGetWindow();
  XmString clip_label;
  Pixmap pixmap = (Pixmap)value;

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
  Window window = motClipboardGetWindow();
  (void)ih;
  
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
  Window window = motClipboardGetWindow();
  XmString clip_label;
  char* format;

  if (!value)
  {
    XmClipboardUndoCopy(iupmot_display, window);
    return 0;
  }

  size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (!size)
    return 0;

  format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return 0;

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
  Window window = motClipboardGetWindow();

  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return 0;
  
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
  (void)ih;
  return iupStrReturnBoolean (motClipboardIsAvailable("STRING")); 
}

static char* motClipboardGetImageAvailableAttrib(Ihandle *ih)
{
  (void)ih;
  return iupStrReturnBoolean (motClipboardIsAvailable("PIXMAP")); 
}

static char* motClipboardGetFormatAvailableAttrib(Ihandle *ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return NULL;

  return iupStrReturnBoolean (motClipboardIsAvailable(format)); 
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
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

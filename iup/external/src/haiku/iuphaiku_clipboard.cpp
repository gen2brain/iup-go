/** \file
 * \brief Haiku Clipboard
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstring>

#include <Bitmap.h>
#include <Clipboard.h>
#include <Message.h>

extern "C" {
#include "iup.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_image.h"
}


/* BeOS canonical clipboard MIME for archived BBitmap. Used by Tracker / Wonder
 * Brush / Icon-O-Matic / etc. */
#define IUPHAIKU_CLIP_BITMAP_MIME "image/x-vnd.Be-bitmap"

static int haikuClipSetText(Ihandle* /*ih*/, const char* value)
{
  if (!be_clipboard) return 0;
  if (!be_clipboard->Lock()) return 0;
  be_clipboard->Clear();
  if (value)
  {
    BMessage* data = be_clipboard->Data();
    if (data) data->AddData("text/plain", B_MIME_TYPE, value, strlen(value));
  }
  be_clipboard->Commit();
  be_clipboard->Unlock();
  return 0;
}

static char* haikuClipGetText(Ihandle* /*ih*/)
{
  if (!be_clipboard) return NULL;
  if (!be_clipboard->Lock()) return NULL;
  BMessage* data = be_clipboard->Data();
  const void* bytes = NULL;
  ssize_t len = 0;
  char* result = NULL;
  if (data && data->FindData("text/plain", B_MIME_TYPE, &bytes, &len) == B_OK && bytes && len > 0)
  {
    char* buf = iupStrGetMemory((int)len + 1);
    memcpy(buf, bytes, len);
    buf[len] = 0;
    result = buf;
  }
  be_clipboard->Unlock();
  return result;
}

static char* haikuClipGetTextAvailable(Ihandle* /*ih*/)
{
  if (!be_clipboard) return iupStrReturnBoolean(0);
  int has = 0;
  if (be_clipboard->Lock())
  {
    BMessage* data = be_clipboard->Data();
    if (data)
    {
      const void* bytes = NULL; ssize_t len = 0;
      has = (data->FindData("text/plain", B_MIME_TYPE, &bytes, &len) == B_OK);
    }
    be_clipboard->Unlock();
  }
  return iupStrReturnBoolean(has);
}

static int haikuClipPutBitmap(BBitmap* bm)
{
  if (!be_clipboard || !be_clipboard->Lock()) return 0;
  be_clipboard->Clear();
  if (bm)
  {
    BMessage* data = be_clipboard->Data();
    if (data)
    {
      BMessage archive;
      if (bm->Archive(&archive, true) == B_OK)
        data->AddMessage(IUPHAIKU_CLIP_BITMAP_MIME, &archive);
    }
  }
  be_clipboard->Commit();
  be_clipboard->Unlock();
  return 0;
}

static int haikuClipSetImage(Ihandle* ih, const char* value)
{
  if (!value) { haikuClipPutBitmap(NULL); return 0; }
  BBitmap* bm = (BBitmap*)iupImageGetImage(value, ih, 0, NULL);
  return haikuClipPutBitmap(bm);
}

static int haikuClipSetNativeImage(Ihandle* /*ih*/, const char* value)
{
  return haikuClipPutBitmap((BBitmap*)value);
}

static char* haikuClipGetNativeImage(Ihandle* ih)
{
  if (!be_clipboard || !be_clipboard->Lock()) return NULL;
  BMessage* data = be_clipboard->Data();
  BBitmap* bm = NULL;
  if (data)
  {
    BMessage archive;
    if (data->FindMessage(IUPHAIKU_CLIP_BITMAP_MIME, &archive) == B_OK)
      bm = new BBitmap(&archive);
  }
  be_clipboard->Unlock();

  /* Cache so successive Get returns the same pointer until Destroy. */
  BBitmap* prev = (BBitmap*)iupAttribGet(ih, "_IUPHAIKU_CLIP_IMAGE");
  delete prev;
  iupAttribSet(ih, "_IUPHAIKU_CLIP_IMAGE", (char*)bm);
  return (char*)bm;
}

static char* haikuClipGetImageAvailable(Ihandle* /*ih*/)
{
  if (!be_clipboard) return iupStrReturnBoolean(0);
  int has = 0;
  if (be_clipboard->Lock())
  {
    BMessage* data = be_clipboard->Data();
    if (data)
    {
      BMessage probe;
      has = (data->FindMessage(IUPHAIKU_CLIP_BITMAP_MIME, &probe) == B_OK);
    }
    be_clipboard->Unlock();
  }
  return iupStrReturnBoolean(has);
}

static void haikuClipDestroy(Ihandle* ih)
{
  BBitmap* cached = (BBitmap*)iupAttribGet(ih, "_IUPHAIKU_CLIP_IMAGE");
  delete cached;
  iupAttribSet(ih, "_IUPHAIKU_CLIP_IMAGE", NULL);
}

/* Custom format: FORMAT names a BMessage data slot (MIME-typed via B_MIME_TYPE) */

static int haikuClipSetFormatDataAttrib(Ihandle* ih, const char* value)
{
  const char* format = iupAttribGet(ih, "FORMAT");
  if (!format || !*format) return 0;
  if (!be_clipboard || !be_clipboard->Lock()) return 0;
  be_clipboard->Clear();
  if (value)
  {
    int size = iupAttribGetInt(ih, "FORMATDATASIZE");
    if (size > 0)
    {
      BMessage* data = be_clipboard->Data();
      if (data) data->AddData(format, B_MIME_TYPE, value, size);
    }
  }
  be_clipboard->Commit();
  be_clipboard->Unlock();
  return 0;
}

static char* haikuClipGetFormatDataAttrib(Ihandle* ih)
{
  const char* format = iupAttribGet(ih, "FORMAT");
  if (!format || !*format || !be_clipboard) return NULL;
  if (!be_clipboard->Lock()) return NULL;
  BMessage* msg = be_clipboard->Data();
  char* result = NULL;
  if (msg)
  {
    const void* bytes = NULL;
    ssize_t len = 0;
    if (msg->FindData(format, B_MIME_TYPE, &bytes, &len) == B_OK && bytes && len > 0)
    {
      char* buf = iupStrGetMemory((int)len);
      memcpy(buf, bytes, len);
      iupAttribSetInt(ih, "FORMATDATASIZE", (int)len);
      result = buf;
    }
  }
  be_clipboard->Unlock();
  return result;
}

static int haikuClipSetFormatDataStringAttrib(Ihandle* ih, const char* value)
{
  if (!value) return haikuClipSetFormatDataAttrib(ih, NULL);
  iupAttribSetInt(ih, "FORMATDATASIZE", (int)strlen(value) + 1);
  return haikuClipSetFormatDataAttrib(ih, value);
}

static char* haikuClipGetFormatDataStringAttrib(Ihandle* ih)
{
  char* data = haikuClipGetFormatDataAttrib(ih);
  if (!data) return NULL;
  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  data[size - 1] = 0;
  return data;
}

static char* haikuClipGetFormatAvailableAttrib(Ihandle* ih)
{
  const char* format = iupAttribGet(ih, "FORMAT");
  if (!format || !*format || !be_clipboard) return iupStrReturnBoolean(0);
  int has = 0;
  if (be_clipboard->Lock())
  {
    BMessage* data = be_clipboard->Data();
    if (data)
    {
      const void* bytes = NULL;
      ssize_t len = 0;
      has = (data->FindData(format, B_MIME_TYPE, &bytes, &len) == B_OK);
    }
    be_clipboard->Unlock();
  }
  return iupStrReturnBoolean(has);
}

extern "C" IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

extern "C" Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);
  ic->name = (char*)"clipboard";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;
  ic->New = iupClipboardNewClass;
  ic->Destroy = haikuClipDestroy;

  iupClassRegisterAttribute(ic, "TEXT", haikuClipGetText, haikuClipSetText, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", haikuClipGetTextAvailable, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, haikuClipSetImage, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NATIVEIMAGE", haikuClipGetNativeImage, haikuClipSetNativeImage, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", haikuClipGetImageAvailable, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* ADDFORMAT is a no-op (BMessage accepts any name) */
  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", haikuClipGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", haikuClipGetFormatDataAttrib, haikuClipSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", haikuClipGetFormatDataStringAttrib, haikuClipSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  return ic;
}

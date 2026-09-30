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


/* BeOS canonical clipboard MIME for an archived BBitmap. */
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
  if (!be_clipboard) return nullptr;
  if (!be_clipboard->Lock()) return nullptr;
  BMessage* data = be_clipboard->Data();
  const void* bytes = nullptr;
  ssize_t len = 0;
  char* result = nullptr;
  if (data && data->FindData("text/plain", B_MIME_TYPE, &bytes, &len) == B_OK && bytes && len > 0)
  {
    char* buf = iupStrGetMemory(static_cast<int>(len) + 1);
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
      const void* bytes = nullptr; ssize_t len = 0;
      has = (data->FindData("text/plain", B_MIME_TYPE, &bytes, &len) == B_OK);
    }
    be_clipboard->Unlock();
  }
  return iupStrReturnBoolean(has);
}

static int haikuClipPutBitmap(const BBitmap* bm)
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
  if (!value) { haikuClipPutBitmap(nullptr); return 0; }
  auto* bm = static_cast<BBitmap*>(iupImageGetImage(value, ih, 0, nullptr));
  return haikuClipPutBitmap(bm);
}

static int haikuClipSetNativeImage(Ihandle* /*ih*/, const char* value)
{
  return haikuClipPutBitmap(reinterpret_cast<const BBitmap*>(value));
}

static char* haikuClipGetNativeImage(Ihandle* ih)
{
  if (!be_clipboard || !be_clipboard->Lock()) return nullptr;
  BMessage* data = be_clipboard->Data();
  BBitmap* bm = nullptr;
  if (data)
  {
    BMessage archive;
    if (data->FindMessage(IUPHAIKU_CLIP_BITMAP_MIME, &archive) == B_OK)
      bm = new BBitmap(&archive);
  }
  be_clipboard->Unlock();

  /* Cache so successive Get returns the same pointer until Destroy. */
  auto* prev = reinterpret_cast<BBitmap*>(iupAttribGet(ih, "_IUPHAIKU_CLIP_IMAGE"));
  delete prev;
  iupAttribSet(ih, "_IUPHAIKU_CLIP_IMAGE", reinterpret_cast<char*>(bm));
  return reinterpret_cast<char*>(bm);
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
  auto* cached = reinterpret_cast<BBitmap*>(iupAttribGet(ih, "_IUPHAIKU_CLIP_IMAGE"));
  delete cached;
  iupAttribSet(ih, "_IUPHAIKU_CLIP_IMAGE", nullptr);
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
  if (!format || !*format || !be_clipboard) return nullptr;
  if (!be_clipboard->Lock()) return nullptr;
  BMessage* msg = be_clipboard->Data();
  char* result = nullptr;
  if (msg)
  {
    const void* bytes = nullptr;
    ssize_t len = 0;
    if (msg->FindData(format, B_MIME_TYPE, &bytes, &len) == B_OK && bytes && len > 0)
    {
      char* buf = iupStrGetMemory(static_cast<int>(len));
      memcpy(buf, bytes, len);
      iupAttribSetInt(ih, "FORMATDATASIZE", static_cast<int>(len));
      result = buf;
    }
  }
  be_clipboard->Unlock();
  return result;
}

static int haikuClipSetFormatDataStringAttrib(Ihandle* ih, const char* value)
{
  if (!value) return haikuClipSetFormatDataAttrib(ih, nullptr);
  iupAttribSetInt(ih, "FORMATDATASIZE", static_cast<int>(strlen(value)) + 1);
  return haikuClipSetFormatDataAttrib(ih, value);
}

static char* haikuClipGetFormatDataStringAttrib(Ihandle* ih)
{
  char* data = haikuClipGetFormatDataAttrib(ih);
  if (!data) return nullptr;
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
      const void* bytes = nullptr;
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
  Iclass* ic = iupClassNew(nullptr);
  ic->name = const_cast<char*>("clipboard");
  ic->format = nullptr;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;
  ic->New = iupClipboardNewClass;
  ic->Destroy = haikuClipDestroy;

  iupClassRegisterAttribute(ic, "TEXT", haikuClipGetText, haikuClipSetText, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", haikuClipGetTextAvailable, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", nullptr, haikuClipSetImage, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NATIVEIMAGE", haikuClipGetNativeImage, haikuClipSetNativeImage, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", haikuClipGetImageAvailable, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* ADDFORMAT is a no-op (BMessage accepts any name) */
  iupClassRegisterAttribute(ic, "ADDFORMAT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", haikuClipGetFormatAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", haikuClipGetFormatDataAttrib, haikuClipSetFormatDataAttrib, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", haikuClipGetFormatDataStringAttrib, haikuClipSetFormatDataStringAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  return ic;
}

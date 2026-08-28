/** \file
 * \brief EFL Clipboard
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_image.h"
#include "iup_drv.h"

#include "iupefl_drv.h"


static unsigned int eflClipboardGetSeatId(Eo* win)
{
  Eo* seat = efl_canvas_scene_seat_default_get(win);
  if (seat)
    return efl_input_device_seat_id_get(seat);
  return 0;
}

static Efl_Ui_Cnp_Buffer eflClipboardBuffer(Ihandle* ih)
{
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTION"), "PRIMARY"))
    return EFL_UI_CNP_BUFFER_SELECTION;
  return EFL_UI_CNP_BUFFER_COPY_AND_PASTE;
}

typedef struct {
  void* data;
  size_t size;
} eflClipboardResult;

static Eina_Value eflClipboardTextResolveCb(void* data, const Eina_Value value, const Eina_Future* dead_future)
{
  char** result_ptr = (char**)data;

  (void)dead_future;

  if (eina_value_type_get(&value) == EINA_VALUE_TYPE_CONTENT)
  {
    Eina_Content* content = eina_value_to_content(&value);
    if (content)
    {
      Eina_Slice slice = eina_content_data_get(content);
      if (slice.mem && slice.len > 0)
      {
        *result_ptr = (char*)malloc(slice.len + 1);
        if (*result_ptr)
        {
          memcpy(*result_ptr, slice.mem, slice.len);
          (*result_ptr)[slice.len] = '\0';
        }
      }
      eina_content_free(content);
    }
  }

  iupeflModalLoopQuit();
  return value;
}

static Eina_Value eflClipboardFormatResolveCb(void* data, const Eina_Value value, const Eina_Future* dead_future)
{
  eflClipboardResult* result = (eflClipboardResult*)data;

  (void)dead_future;

  if (eina_value_type_get(&value) == EINA_VALUE_TYPE_CONTENT)
  {
    Eina_Content* content = eina_value_to_content(&value);
    if (content)
    {
      Eina_Slice slice = eina_content_data_get(content);
      if (slice.mem && slice.len > 0)
      {
        result->data = malloc(slice.len);
        if (result->data)
        {
          memcpy(result->data, slice.mem, slice.len);
          result->size = slice.len;
        }
      }
      eina_content_free(content);
    }
  }

  iupeflModalLoopQuit();
  return value;
}

static int efl_clipboard_timed_out = 0;

static Eina_Bool eflClipboardTimeoutCb(void* data)
{
  (void)data;
  efl_clipboard_timed_out = 1;
  iupeflModalLoopQuit();
  return ECORE_CALLBACK_CANCEL;
}

static int eflClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  Eo* win;
  Eina_Content* content;
  Eina_Slice slice;

  win = iupeflGetMainWindow();
  if (!win)
    return 0;

  if (value && value[0])
  {
    unsigned int seat = eflClipboardGetSeatId(win);
    slice.mem = value;
    slice.len = strlen(value) + 1;
    content = eina_content_new(slice, "text/plain;charset=utf-8");
    if (content)
      efl_ui_selection_set(win, eflClipboardBuffer(ih), content, seat);
  }
  else
  {
    efl_ui_selection_clear(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win));
  }

  return 0;
}

static char* eflClipboardGetTextAttrib(Ihandle* ih)
{
  Eo* win;
  Eina_Future* future;
  Eina_Iterator* types;
  char* result = NULL;

  win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  types = eina_carray_iterator_new((void*[]){ (void*)"text/plain;charset=utf-8", (void*)"text/plain", NULL });
  future = efl_ui_selection_get(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win), types);
  if (future)
  {
    Ecore_Timer* timeout;
    efl_clipboard_timed_out = 0;
    timeout = ecore_timer_add(2.0, eflClipboardTimeoutCb, NULL);
    eina_future_then(future, eflClipboardTextResolveCb, &result, NULL);
    iupeflModalLoopRun(NULL);
    if (!efl_clipboard_timed_out && timeout)
      ecore_timer_del(timeout);
  }

  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }

  return NULL;
}

/* EFL has_selection only checks if the buffer has any content,
   not whether the content is specifically text. */
static char* eflClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  Eo* win;

  win = iupeflGetMainWindow();
  if (!win)
    return "NO";

  if (efl_ui_selection_has_selection(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win)))
    return "YES";

  return "NO";
}

static const char* eflClipboardGetFormatMimeType(Ihandle* ih)
{
  return iupAttribGetStr(ih, "FORMAT");
}

static int eflClipboardSetFormatDataAttrib(Ihandle* ih, const char* value)
{
  Eo* win;
  Eina_Content* content;
  Eina_Slice slice;
  const char* mime_type;
  int size;

  win = iupeflGetMainWindow();
  if (!win)
    return 0;

  if (!value)
  {
    efl_ui_selection_clear(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win));
    free(iupAttribGet(ih, "_IUP_CLIPBOARD_FORMAT_CACHE"));
    iupAttribSet(ih, "_IUP_CLIPBOARD_FORMAT_CACHE", NULL);
    iupAttribSetInt(ih, "_IUP_CLIPBOARD_FORMAT_CACHE_SIZE", 0);
    return 0;
  }

  mime_type = eflClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return 0;

  size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (size <= 0)
    return 0;

  slice.mem = value;
  slice.len = size;
  content = eina_content_new(slice, mime_type);
  if (content)
    efl_ui_selection_set(win, eflClipboardBuffer(ih), content, eflClipboardGetSeatId(win));

  /* Cache locally to avoid X11 self-selection deadlock */
  {
    char* old_cache = (char*)iupAttribGet(ih, "_IUP_CLIPBOARD_FORMAT_CACHE");
    char* cache = (char*)malloc(size);
    if (cache)
    {
      memcpy(cache, value, size);
      iupAttribSet(ih, "_IUP_CLIPBOARD_FORMAT_CACHE", cache);
      iupAttribSetInt(ih, "_IUP_CLIPBOARD_FORMAT_CACHE_SIZE", size);
    }
    free(old_cache);
  }

  return 0;
}

static eflClipboardResult efl_clipboard_format_result;

static char* eflClipboardGetFormatDataAttrib(Ihandle* ih)
{
  Eo* win;
  Eina_Future* future;
  Eina_Iterator* types;
  const char* mime_type;
  eflClipboardResult *result = &efl_clipboard_format_result;

  result->data = NULL;
  result->size = 0;

  win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  mime_type = eflClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return NULL;

  /* Use local cache to avoid X11 self-selection deadlock */
  {
    const char* cached = iupAttribGet(ih, "_IUP_CLIPBOARD_FORMAT_CACHE");
    int cached_size = iupAttribGetInt(ih, "_IUP_CLIPBOARD_FORMAT_CACHE_SIZE");
    if (cached && cached_size > 0)
    {
      result->data = malloc(cached_size);
      if (result->data)
      {
        memcpy(result->data, cached, cached_size);
        result->size = cached_size;
      }
    }
  }

  if (!result->data)
  {
    types = eina_carray_iterator_new((void*[]){ (void*)mime_type, NULL });
    future = efl_ui_selection_get(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win), types);
    if (future)
    {
      efl_clipboard_timed_out = 0;
      Ecore_Timer* timeout = ecore_timer_add(2.0, eflClipboardTimeoutCb, NULL);
      eina_future_then(future, eflClipboardFormatResolveCb, result, NULL);
      iupeflModalLoopRun(NULL);
      if (!efl_clipboard_timed_out && timeout)
        ecore_timer_del(timeout);
    }
  }

  if (result->data)
  {
    void* ret = iupStrGetMemory((int)result->size + 1);
    memcpy(ret, result->data, result->size);
    ((char*)ret)[result->size] = 0;
    free(result->data);
    result->data = NULL;
    iupAttribSetInt(ih, "FORMATDATASIZE", (int)result->size);
    return ret;
  }

  return NULL;
}

static char* eflClipboardGetFormatDataStringAttrib(Ihandle* ih)
{
  char* data = eflClipboardGetFormatDataAttrib(ih);
  if (!data)
    return NULL;
  return iupStrReturnStr(data);
}

static int eflClipboardSetFormatDataStringAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int len = (int)strlen(value);
    iupAttribSetInt(ih, "FORMATDATASIZE", len + 1);
    return eflClipboardSetFormatDataAttrib(ih, value);
  }
  else
    return eflClipboardSetFormatDataAttrib(ih, NULL);
}

/* EFL has_selection only checks if the buffer has any content,
   not whether the specific format is available. */

#define IUPEFL_CLIPBOARD_IMAGE_MIME "image/png"

static void* eflClipboardFileRead(const char* path, size_t* out_len)
{
  FILE* file = fopen(path, "rb");
  void* data = NULL;
  long len;

  if (!file)
    return NULL;

  fseek(file, 0, SEEK_END);
  len = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (len > 0)
  {
    data = malloc((size_t)len);
    if (data && fread(data, 1, (size_t)len, file) == (size_t)len)
      *out_len = (size_t)len;
    else
    {
      free(data);
      data = NULL;
    }
  }

  fclose(file);
  return data;
}

static void* eflClipboardImageEncode(Ihandle* image, size_t* out_len)
{
  char path[] = "/tmp/iupeflclipXXXXXX.png";
  Eo* img;
  void* data = NULL;
  int fd;

  img = (Eo*)iupdrvImageCreateImage(image, NULL, 0);
  if (!img)
    return NULL;

  fd = mkstemps(path, 4);
  if (fd >= 0)
  {
    close(fd);
    if (evas_object_image_save(img, path, NULL, "compress=9"))
      data = eflClipboardFileRead(path, out_len);
    remove(path);
  }

  efl_unref(img);
  return data;
}

static Eo* eflClipboardImageDecode(const void* data, size_t len)
{
  char path[] = "/tmp/iupeflclipXXXXXX.png";
  Eo* win = iupeflGetMainWindow();
  Eo* img = NULL;
  int fd;

  if (!win || !data || !len)
    return NULL;

  fd = mkstemps(path, 4);
  if (fd < 0)
    return NULL;

  if (write(fd, data, len) == (ssize_t)len)
  {
    close(fd);

    img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
    if (img)
    {
      int stride = 0;
      Eina_Rw_Slice mapped;

      efl_gfx_entity_visible_set(img, EINA_FALSE);
      evas_object_image_file_set(img, path, NULL);

      /* map once so the file is decoded before it goes away */
      mapped = efl_gfx_buffer_map(img, EFL_GFX_BUFFER_ACCESS_MODE_READ,
                                  NULL, EFL_GFX_COLORSPACE_ARGB8888, 0, &stride);
      if (mapped.mem)
        efl_gfx_buffer_unmap(img, mapped);
      else
      {
        efl_unref(img);
        img = NULL;
      }
    }
  }
  else
    close(fd);

  remove(path);
  return img;
}

static void eflClipboardImageCacheSet(Ihandle* ih, void* data, size_t len)
{
  void* old = iupAttribGet(ih, "_IUP_CLIPBOARD_IMAGE_CACHE");

  iupAttribSet(ih, "_IUP_CLIPBOARD_IMAGE_CACHE", (char*)data);
  iupAttribSetInt(ih, "_IUP_CLIPBOARD_IMAGE_CACHE_SIZE", (int)len);

  if (old)
    free(old);
}

static void* eflClipboardImageFetch(Ihandle* ih, size_t* out_len)
{
  Eo* win = iupeflGetMainWindow();
  eflClipboardResult* result = &efl_clipboard_format_result;
  const char* cached;
  int cached_size;

  cached = iupAttribGet(ih, "_IUP_CLIPBOARD_IMAGE_CACHE");
  cached_size = iupAttribGetInt(ih, "_IUP_CLIPBOARD_IMAGE_CACHE_SIZE");
  if (cached && cached_size > 0)
  {
    void* copy = malloc((size_t)cached_size);
    if (!copy)
      return NULL;
    memcpy(copy, cached, (size_t)cached_size);
    *out_len = (size_t)cached_size;
    return copy;
  }

  if (!win)
    return NULL;

  result->data = NULL;
  result->size = 0;

  {
    Eina_Iterator* types = eina_carray_iterator_new((void*[]){ (void*)IUPEFL_CLIPBOARD_IMAGE_MIME, NULL });
    Eina_Future* future = efl_ui_selection_get(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win), types);
    if (future)
    {
      Ecore_Timer* timeout;
      efl_clipboard_timed_out = 0;
      timeout = ecore_timer_add(2.0, eflClipboardTimeoutCb, NULL);
      eina_future_then(future, eflClipboardFormatResolveCb, result, NULL);
      iupeflModalLoopRun(NULL);
      if (!efl_clipboard_timed_out && timeout)
        ecore_timer_del(timeout);
    }
  }

  if (result->data)
  {
    void* data = result->data;
    *out_len = result->size;
    result->data = NULL;
    result->size = 0;
    return data;
  }

  return NULL;
}

static int eflClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetMainWindow();
  Ihandle* image;
  Eina_Content* content;
  Eina_Slice slice;
  void* png;
  size_t len = 0;

  if (!win)
    return 0;

  if (!value)
  {
    efl_ui_selection_clear(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win));
    eflClipboardImageCacheSet(ih, NULL, 0);
    return 0;
  }

  image = IupGetHandle(value);
  if (!image)
    return 0;

  png = eflClipboardImageEncode(image, &len);
  if (!png)
    return 0;

  slice.mem = png;
  slice.len = len;
  content = eina_content_new(slice, IUPEFL_CLIPBOARD_IMAGE_MIME);
  if (content)
    efl_ui_selection_set(win, eflClipboardBuffer(ih), content, eflClipboardGetSeatId(win));

  /* keep a copy, an X11 client cannot read back its own selection */
  eflClipboardImageCacheSet(ih, png, len);

  return 0;
}

static char* eflClipboardGetImageAvailableAttrib(Ihandle* ih)
{
  size_t len = 0;
  void* png = eflClipboardImageFetch(ih, &len);

  if (png)
  {
    free(png);
    return iupStrReturnBoolean(1);
  }

  return iupStrReturnBoolean(0);
}

static char* eflClipboardGetNativeImageAttrib(Ihandle* ih)
{
  size_t len = 0;
  void* png = eflClipboardImageFetch(ih, &len);
  Eo* img;

  if (!png)
    return NULL;

  img = eflClipboardImageDecode(png, len);
  free(png);

  return (char*)img;
}

static int eflClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetMainWindow();
  char path[] = "/tmp/iupeflclipXXXXXX.png";
  Eo* img = (Eo*)value;
  int fd;

  if (!win || !img)
    return 0;

  fd = mkstemps(path, 4);
  if (fd < 0)
    return 0;
  close(fd);

  if (evas_object_image_save(img, path, NULL, "compress=9"))
  {
    size_t len = 0;
    void* png = eflClipboardFileRead(path, &len);
    if (png)
    {
      Eina_Slice slice;
      Eina_Content* content;

      slice.mem = png;
      slice.len = len;
      content = eina_content_new(slice, IUPEFL_CLIPBOARD_IMAGE_MIME);
      if (content)
        efl_ui_selection_set(win, eflClipboardBuffer(ih), content, eflClipboardGetSeatId(win));

      eflClipboardImageCacheSet(ih, png, len);
    }
  }

  remove(path);
  return 0;
}

static char* eflClipboardGetFormatAvailableAttrib(Ihandle* ih)
{
  Eo* win;

  win = iupeflGetMainWindow();
  if (!win)
    return "NO";

  if (efl_ui_selection_has_selection(win, eflClipboardBuffer(ih), eflClipboardGetSeatId(win)))
    return "YES";

  return "NO";
}


Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "clipboard";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", eflClipboardGetTextAttrib, eflClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", eflClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", eflClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NATIVEIMAGE", eflClipboardGetNativeImageAttrib, eflClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, eflClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", eflClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, NULL, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", eflClipboardGetFormatDataAttrib, eflClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", eflClipboardGetFormatDataStringAttrib, eflClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTION", NULL, NULL, "CLIPBOARD", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

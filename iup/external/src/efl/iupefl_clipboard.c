/** \file
 * \brief EFL Clipboard
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"

#include "iupefl_drv.h"


static unsigned int eflClipboardGetSeatId(Eo* win)
{
  Eo* seat = efl_canvas_scene_seat_default_get(win);
  if (seat)
    return efl_input_device_seat_id_get(seat);
  return 0;
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
        *result_ptr = strndup((const char*)slice.mem, slice.len);
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

static int eflClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  Eo* win;
  Eina_Content* content;
  Eina_Slice slice;

  (void)ih;

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
      efl_ui_selection_set(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, content, seat);
  }
  else
  {
    efl_ui_selection_clear(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, eflClipboardGetSeatId(win));
  }

  return 0;
}

static char* eflClipboardGetTextAttrib(Ihandle* ih)
{
  Eo* win;
  Eina_Future* future;
  Eina_Iterator* types;
  char* result = NULL;

  (void)ih;

  win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  types = eina_carray_iterator_new((void*[]){ (void*)"text/plain;charset=utf-8", (void*)"text/plain", NULL });
  future = efl_ui_selection_get(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, eflClipboardGetSeatId(win), types);
  if (future)
  {
    eina_future_then(future, eflClipboardTextResolveCb, &result, NULL);
    iupeflModalLoopRun();
  }

  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }

  return NULL;
}

static char* eflClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  Eo* win;

  (void)ih;

  win = iupeflGetMainWindow();
  if (!win)
    return "NO";

  if (efl_ui_selection_has_selection(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, eflClipboardGetSeatId(win)))
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
    efl_ui_selection_clear(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, eflClipboardGetSeatId(win));
    return 0;
  }

  mime_type = eflClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return 0;

  size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (!size)
    return 0;

  slice.mem = value;
  slice.len = size;
  content = eina_content_new(slice, mime_type);
  if (content)
    efl_ui_selection_set(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, content, eflClipboardGetSeatId(win));

  return 0;
}

static char* eflClipboardGetFormatDataAttrib(Ihandle* ih)
{
  Eo* win;
  Eina_Future* future;
  Eina_Iterator* types;
  const char* mime_type;
  eflClipboardResult result = {NULL, 0};

  win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  mime_type = eflClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return NULL;

  types = eina_carray_iterator_new((void*[]){ (void*)mime_type, NULL });
  future = efl_ui_selection_get(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, eflClipboardGetSeatId(win), types);
  if (future)
  {
    eina_future_then(future, eflClipboardFormatResolveCb, &result, NULL);
    iupeflModalLoopRun();
  }

  if (result.data)
  {
    void* ret = iupStrGetMemory((int)result.size);
    memcpy(ret, result.data, result.size);
    free(result.data);
    iupAttribSetInt(ih, "FORMATDATASIZE", (int)result.size);
    return ret;
  }

  return NULL;
}

static char* eflClipboardGetFormatAvailableAttrib(Ihandle* ih)
{
  Eo* win;

  win = iupeflGetMainWindow();
  if (!win)
    return "NO";

  if (efl_ui_selection_has_selection(win, EFL_UI_CNP_BUFFER_COPY_AND_PASTE, eflClipboardGetSeatId(win)))
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
  iupClassRegisterAttribute(ic, "FORMATDATA", eflClipboardGetFormatDataAttrib, eflClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

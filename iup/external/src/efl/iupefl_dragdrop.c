/** \file
 * \brief EFL Drag&Drop Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iupefl_drv.h"

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_key.h"


/*****************************************************************************
 * Static storage for drag data (avoids async issues with EFL selection)
 *****************************************************************************/

static Ihandle* efl_drag_source_ih = NULL;
static char* efl_drag_data = NULL;
static int efl_drag_data_size = 0;
static char efl_drag_type[64] = {0};

static void eflDragCleanup(void)
{
  if (efl_drag_data)
  {
    free(efl_drag_data);
    efl_drag_data = NULL;
  }
  efl_drag_data_size = 0;
  efl_drag_source_ih = NULL;
  efl_drag_type[0] = '\0';
}

static Eina_Bool eflDragEndIdleCb(void *data)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupAttribGet(ih, "_IUPEFL_DRAGEND_PENDING"))
  {
    IFni cbDragEnd = (IFni)IupGetCallback(ih, "DRAGEND_CB");
    iupAttribSet(ih, "_IUPEFL_DRAGEND_PENDING", NULL);
    if (cbDragEnd)
      cbDragEnd(ih, 0);
    eflDragCleanup();
  }

  return ECORE_CALLBACK_CANCEL;
}

/*****************************************************************************
 * Helper Functions
 *****************************************************************************/

static char efl_mime_type_buffer[256];

static const char* eflParseMimeType(const char* value)
{
  if (!value || !value[0])
    return "text/plain";

  if (iupStrEqualNoCase(value, "TEXT") ||
      iupStrEqualNoCase(value, "STRING") ||
      iupStrEqualNoCase(value, "UTF8_STRING"))
    return "text/plain";
  else if (iupStrEqualNoCase(value, "text/html"))
    return "text/html";
  else if (iupStrEqualNoCase(value, "text/uri-list"))
    return "text/uri-list";
  else if (iupStrEqualNoCase(value, "text/vcard"))
    return "text/vcard";
  else if (iupStrEqualPartial(value, "image/"))
    return "image/png";
  else if (strchr(value, '/') != NULL)
    return value;

  snprintf(efl_mime_type_buffer, sizeof(efl_mime_type_buffer), "application/x-iup-%s", value);
  return efl_mime_type_buffer;
}

static Evas_Modifier* eflGetModifiers(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return NULL;

  Evas* evas = evas_object_evas_get(widget);
  if (!evas)
    return NULL;

  return (Evas_Modifier*)evas_key_modifier_get(evas);
}

/*****************************************************************************
 * Drop Target Callbacks (Modern EFL API)
 *****************************************************************************/

static void eflDropEnteredCb(void *data, const Efl_Event *ev)
{
  (void)data;
  (void)ev;
}

static void eflDropLeftCb(void *data, const Efl_Event *ev)
{
  (void)data;
  (void)ev;
}

static void eflDropPositionChangedCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Ui_Drop_Event* drop_ev = ev->info;
  IFniis cbDropMotion;

  cbDropMotion = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
  if (cbDropMotion)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupeflButtonKeySetStatus(eflGetModifiers(ih), 0, status, 0);
    cbDropMotion(ih, drop_ev->position.x, drop_ev->position.y, status);
  }
}

static void eflDropDroppedCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Ui_Drop_Dropped_Event* drop_ev = ev->info;
  IFnsViii cbDropData;
  int drop_x = 0, drop_y = 0;

  if (drop_ev)
  {
    drop_x = drop_ev->dnd.position.x;
    drop_y = drop_ev->dnd.position.y;
  }

  if (!efl_drag_data || !efl_drag_data_size)
    return;

  cbDropData = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  if (cbDropData)
    cbDropData(ih, efl_drag_type, efl_drag_data, efl_drag_data_size, drop_x, drop_y);

  if (efl_drag_source_ih && iupAttribGet(efl_drag_source_ih, "_IUPEFL_DRAGEND_PENDING"))
  {
    IFni cbDragEnd = (IFni)IupGetCallback(efl_drag_source_ih, "DRAGEND_CB");
    iupAttribSet(efl_drag_source_ih, "_IUPEFL_DRAGEND_PENDING", NULL);
    if (cbDragEnd)
      cbDragEnd(efl_drag_source_ih, 0);
  }

  eflDragCleanup();
}

/*****************************************************************************
 * Drop Target Attribute Setters
 *****************************************************************************/

static int eflSetDropTypesAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    iupAttribSet(ih, "_IUPEFL_DROP_TYPES", NULL);
    return 0;
  }

  iupAttribSetStr(ih, "_IUPEFL_DROP_TYPES", value);
  return 1;
}

static int eflSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    if (iupAttribGet(ih, "_IUPEFL_DROP_TARGET_ACTIVE"))
      return 1;

    efl_event_callback_add(widget, EFL_UI_DND_EVENT_DROP_ENTERED, eflDropEnteredCb, ih);
    efl_event_callback_add(widget, EFL_UI_DND_EVENT_DROP_LEFT, eflDropLeftCb, ih);
    efl_event_callback_add(widget, EFL_UI_DND_EVENT_DROP_POSITION_CHANGED, eflDropPositionChangedCb, ih);
    efl_event_callback_add(widget, EFL_UI_DND_EVENT_DROP_DROPPED, eflDropDroppedCb, ih);

    iupAttribSet(ih, "_IUPEFL_DROP_TARGET_ACTIVE", "1");
  }
  else
  {
    if (iupAttribGet(ih, "_IUPEFL_DROP_TARGET_ACTIVE"))
    {
      efl_event_callback_del(widget, EFL_UI_DND_EVENT_DROP_ENTERED, eflDropEnteredCb, ih);
      efl_event_callback_del(widget, EFL_UI_DND_EVENT_DROP_LEFT, eflDropLeftCb, ih);
      efl_event_callback_del(widget, EFL_UI_DND_EVENT_DROP_POSITION_CHANGED, eflDropPositionChangedCb, ih);
      efl_event_callback_del(widget, EFL_UI_DND_EVENT_DROP_DROPPED, eflDropDroppedCb, ih);

      iupAttribSet(ih, "_IUPEFL_DROP_TARGET_ACTIVE", NULL);
    }
  }

  return 1;
}

/*****************************************************************************
 * Drag Source Callbacks (Modern EFL API)
 *****************************************************************************/

static void eflDragFinishedCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* win = iupeflGetMainWindow();

  if (win)
    efl_event_callback_del(win, EFL_UI_DND_EVENT_DRAG_FINISHED, eflDragFinishedCb, ih);

  iupAttribSet(ih, "_IUPEFL_DRAG_ACTIVE", NULL);
  iupAttribSet(ih, "_IUPEFL_DRAGEND_PENDING", "1");

  ecore_idler_add(eflDragEndIdleCb, ih);

  (void)ev;
}

static void eflStartDrag(Ihandle* ih, int x, int y)
{
  IFnii cbDragBegin;
  IFns cbDragDataSize;
  IFnsVi cbDragData;
  char* typeStr;
  int size;
  char* dragData;
  Eo* widget;
  Eina_Content* content;
  Eina_Slice slice;
  const char* mime_type;

  if (iupAttribGet(ih, "_IUPEFL_DRAG_ACTIVE"))
    return;

  cbDragBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if (cbDragBegin && cbDragBegin(ih, x, y) == IUP_IGNORE)
    return;

  cbDragDataSize = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
  cbDragData = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");

  if (!cbDragDataSize || !cbDragData)
    return;

  typeStr = iupAttribGet(ih, "_IUPEFL_DRAG_TYPES");
  if (!typeStr)
    typeStr = (char*)"text/plain";

  size = cbDragDataSize(ih, typeStr);
  if (size <= 0)
    return;

  dragData = (char*)malloc(size + 1);
  if (!dragData)
    return;

  cbDragData(ih, typeStr, dragData, size);
  dragData[size] = '\0';

  if (efl_drag_data)
    free(efl_drag_data);
  efl_drag_source_ih = ih;
  efl_drag_data = (char*)malloc(size);
  if (efl_drag_data)
  {
    memcpy(efl_drag_data, dragData, size);
    efl_drag_data_size = size;
  }
  strncpy(efl_drag_type, typeStr, sizeof(efl_drag_type) - 1);
  efl_drag_type[sizeof(efl_drag_type) - 1] = '\0';

  widget = iupeflGetWidget(ih);
  if (!widget)
  {
    free(dragData);
    return;
  }

  mime_type = eflParseMimeType(typeStr);

  slice.mem = dragData;
  slice.len = size;
  content = eina_content_new(slice, mime_type);

  if (content)
  {
    Eo* win = iupeflGetMainWindow();
    if (!win)
      win = efl_provider_find(widget, EFL_UI_WIN_CLASS);

    if (win)
    {
      const char* action = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ? "move" : "copy";
      unsigned int seat_id = iupeflGetDefaultSeat(widget);
      Eo* drag_win = efl_ui_dnd_drag_start(win, content, action, seat_id);

      if (drag_win)
      {
        char* cursor_name = iupAttribGet(ih, "DRAGCURSOR");
        char* drag_text = iupAttribGet(ih, "DRAGTEXT");
        Eo* drag_content = NULL;
        int w = 32, h = 32;

        if (cursor_name)
          drag_content = iupeflImageGetImageForParent(cursor_name, ih, 0, drag_win);

        if (drag_content)
        {
          Eina_Size2D img_size = efl_gfx_entity_size_get(drag_content);
          w = img_size.w > 0 ? img_size.w : 32;
          h = img_size.h > 0 ? img_size.h : 32;
        }
        else
        {
          if (!drag_text)
            drag_text = typeStr;
          if (!drag_text)
            drag_text = "drag";

          drag_content = efl_add(EFL_UI_TEXTBOX_CLASS, drag_win,
                                 efl_text_interactive_editable_set(efl_added, EINA_FALSE),
                                 efl_text_set(efl_added, drag_text));
          w = 80;
          h = 24;
        }

        if (drag_content)
        {
          efl_gfx_entity_visible_set(drag_content, EINA_TRUE);
          efl_content_set(drag_win, drag_content);
        }

        efl_ui_dnd_drag_offset_set(win, seat_id, EINA_SIZE2D(-w/2, -h/2));
        efl_gfx_entity_size_set(drag_win, EINA_SIZE2D(w, h));

        iupAttribSet(ih, "_IUPEFL_DRAG_ACTIVE", "1");
        efl_event_callback_add(win, EFL_UI_DND_EVENT_DRAG_FINISHED, eflDragFinishedCb, ih);
      }
    }
  }

  free(dragData);
}

static void eflDragSourcePointerDownCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos;
  int button;

  button = efl_input_pointer_button_get(pointer);
  if (button != 1)
    return;

  pos = efl_input_pointer_position_get(pointer);
  iupAttribSetInt(ih, "_IUPEFL_DRAG_START_X", pos.x);
  iupAttribSetInt(ih, "_IUPEFL_DRAG_START_Y", pos.y);
  iupAttribSet(ih, "_IUPEFL_DRAG_PENDING", "1");
}

static void eflDragSourcePointerMoveCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos;
  int startX, startY, dx, dy;

  if (!iupAttribGet(ih, "_IUPEFL_DRAG_PENDING"))
    return;

  pos = efl_input_pointer_position_get(pointer);
  startX = iupAttribGetInt(ih, "_IUPEFL_DRAG_START_X");
  startY = iupAttribGetInt(ih, "_IUPEFL_DRAG_START_Y");
  dx = pos.x - startX;
  dy = pos.y - startY;

  if (dx*dx + dy*dy > 25)
  {
    iupAttribSet(ih, "_IUPEFL_DRAG_PENDING", NULL);
    eflStartDrag(ih, startX, startY);
  }
}

static void eflDragSourcePointerUpCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;

  (void)ev;

  iupAttribSet(ih, "_IUPEFL_DRAG_PENDING", NULL);
}

/*****************************************************************************
 * Drag Source Attribute Setters
 *****************************************************************************/

static int eflSetDragTypesAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    iupAttribSet(ih, "_IUPEFL_DRAG_TYPES", NULL);
    return 0;
  }

  iupAttribSetStr(ih, "_IUPEFL_DRAG_TYPES", value);
  return 1;
}

static int eflSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    if (iupAttribGet(ih, "_IUPEFL_DRAG_SOURCE_ACTIVE"))
      return 1;

    efl_event_callback_add(widget, EFL_EVENT_POINTER_DOWN, eflDragSourcePointerDownCb, ih);
    efl_event_callback_add(widget, EFL_EVENT_POINTER_MOVE, eflDragSourcePointerMoveCb, ih);
    efl_event_callback_add(widget, EFL_EVENT_POINTER_UP, eflDragSourcePointerUpCb, ih);

    iupAttribSet(ih, "_IUPEFL_DRAG_SOURCE_ACTIVE", "1");
  }
  else
  {
    if (iupAttribGet(ih, "_IUPEFL_DRAG_SOURCE_ACTIVE"))
    {
      efl_event_callback_del(widget, EFL_EVENT_POINTER_DOWN, eflDragSourcePointerDownCb, ih);
      efl_event_callback_del(widget, EFL_EVENT_POINTER_MOVE, eflDragSourcePointerMoveCb, ih);
      efl_event_callback_del(widget, EFL_EVENT_POINTER_UP, eflDragSourcePointerUpCb, ih);

      iupAttribSet(ih, "_IUPEFL_DRAG_SOURCE_ACTIVE", NULL);
    }
  }

  return 1;
}

/*****************************************************************************
 * File Drop Support
 *****************************************************************************/

static Eina_Value eflDropFilesDataReceivedCb(Eo* obj, void* data, const Eina_Value value)
{
  Ihandle* ih = (Ihandle*)data;
  IFnsiii cbDropFiles;
  Eina_Content* content;
  char* dataCopy;
  char* savePtr = NULL;
  char* line;
  int count = 0;
  int remaining;
  Eina_Slice slice;

  (void)obj;

  if (eina_value_type_get(&value) != EINA_VALUE_TYPE_CONTENT)
    return value;

  eina_value_get(&value, &content);
  if (!content)
    return value;

  cbDropFiles = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (!cbDropFiles)
    return value;

  slice = eina_content_data_get(content);
  if (!slice.mem || slice.len <= 0)
    return value;

  dataCopy = (char*)malloc(slice.len + 1);
  if (!dataCopy)
    return value;

  memcpy(dataCopy, slice.mem, slice.len);
  dataCopy[slice.len] = '\0';

  line = strtok_r(dataCopy, "\r\n", &savePtr);
  while (line)
  {
    count++;
    line = strtok_r(NULL, "\r\n", &savePtr);
  }

  free(dataCopy);

  dataCopy = (char*)malloc(slice.len + 1);
  if (!dataCopy)
    return value;

  memcpy(dataCopy, slice.mem, slice.len);
  dataCopy[slice.len] = '\0';

  savePtr = NULL;
  remaining = count - 1;
  line = strtok_r(dataCopy, "\r\n", &savePtr);

  while (line)
  {
    char* filename = line;

    if (strncmp(filename, "file://", 7) == 0)
      filename += 7;

    if (cbDropFiles(ih, filename, remaining, 0, 0) == IUP_IGNORE)
      break;

    remaining--;
    line = strtok_r(NULL, "\r\n", &savePtr);
  }

  free(dataCopy);

  return value;
}

static void eflDropFilesDroppedCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* widget = iupeflGetWidget(ih);
  Eina_Future* future;
  Eina_Array* types;

  if (!widget)
    return;

  types = eina_array_new(1);
  eina_array_push(types, "text/uri-list");

  future = efl_ui_dnd_drop_data_get(widget, iupeflGetDefaultSeat(widget), eina_array_iterator_new(types));
  efl_future_then(ev->object, future, .success = eflDropFilesDataReceivedCb, .data = ih);

  eina_array_free(types);
}

static int eflSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    if (iupAttribGet(ih, "_IUPEFL_DROPFILES_ACTIVE"))
      return 1;

    efl_event_callback_add(widget, EFL_UI_DND_EVENT_DROP_DROPPED, eflDropFilesDroppedCb, ih);

    iupAttribSet(ih, "_IUPEFL_DROPFILES_ACTIVE", "1");
  }
  else
  {
    if (iupAttribGet(ih, "_IUPEFL_DROPFILES_ACTIVE"))
    {
      efl_event_callback_del(widget, EFL_UI_DND_EVENT_DROP_DROPPED, eflDropFilesDroppedCb, ih);

      iupAttribSet(ih, "_IUPEFL_DROPFILES_ACTIVE", NULL);
    }
  }

  return 1;
}

void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES",  NULL, eflSetDragTypesAttrib,  NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  NULL, eflSetDropTypesAttrib,  NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, eflSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, eflSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGTEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, eflSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, eflSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}

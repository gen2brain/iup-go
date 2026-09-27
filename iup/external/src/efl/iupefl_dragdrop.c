/** \file
 * \brief EFL Drag&Drop Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_key.h"

#include "iupefl_drv.h"

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif


/*****************************************************************************
 * Static storage for drag data (avoids async issues with EFL selection)
 *****************************************************************************/

static Ihandle* efl_drag_source_ih = NULL;
static Eo* efl_drag_win = NULL;
static unsigned int efl_drag_seat = 0;
static int efl_drag_is_move = 0;
static int efl_drag_accepted = 0;

static void eflDragCleanup(void)
{
  efl_drag_source_ih = NULL;
  efl_drag_win = NULL;
  efl_drag_is_move = 0;
  efl_drag_accepted = 0;
}

static Eina_Bool eflDragEndIdleCb(void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (!iupObjectCheck(ih))
  {
    eflDragCleanup();
    return ECORE_CALLBACK_CANCEL;
  }

  if (iupAttribGet(ih, "_IUPEFL_DRAGEND_PENDING"))
  {
    IFni cbDragEnd = (IFni)IupGetCallback(ih, "DRAGEND_CB");
    int action = efl_drag_accepted ? (efl_drag_is_move ? 1 : 0) : -1;
    iupAttribSet(ih, "_IUPEFL_DRAGEND_PENDING", NULL);
    if (cbDragEnd)
      cbDragEnd(ih, action);
    eflDragCleanup();
  }

  return ECORE_CALLBACK_CANCEL;
}

#ifdef HAVE_ECORE_X
static Ecore_Event_Handler* efl_drag_x11_finished_handler = NULL;

static Eina_Bool eflDragX11FinishedCb(void* data, int type, void* event)
{
  (void)data;
  (void)type;
  (void)event;

  ecore_event_handler_del(efl_drag_x11_finished_handler);
  efl_drag_x11_finished_handler = NULL;

  if (efl_drag_source_ih)
    eflDragEndIdleCb(efl_drag_source_ih);

  return ECORE_CALLBACK_PASS_ON;
}
#endif

static void eflDragFinishedCb(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Ui_Drag_Finished_Event* finished = ev->info;
  Eo* win = iupeflGetMainWindow();

  if (finished && finished->accepted)
    efl_drag_accepted = 1;

  if (win)
    efl_event_callback_del(win, EFL_UI_DND_EVENT_DRAG_FINISHED, eflDragFinishedCb, ih);

  iupAttribSet(ih, "_IUPEFL_DRAG_ACTIVE", NULL);
  iupAttribSet(ih, "_IUPEFL_DRAGEND_PENDING", "1");

#ifdef HAVE_ECORE_X
  /* on X11 this is emitted at release, the target reads the data and sends XdndFinished afterwards */
  if (efl_drag_accepted && iupeflIsX11())
  {
    if (!efl_drag_x11_finished_handler)
      efl_drag_x11_finished_handler = ecore_event_handler_add(ECORE_X_EVENT_XDND_FINISHED, eflDragX11FinishedCb, NULL);
    return;
  }
#endif

  ecore_idler_add(eflDragEndIdleCb, ih);
}

static void eflDragCompleteLocal(void)
{
  Ihandle* ih = efl_drag_source_ih;

  if (!ih)
    return;

  if (efl_drag_win)
  {
    efl_event_callback_del(efl_drag_win, EFL_UI_DND_EVENT_DRAG_FINISHED, eflDragFinishedCb, ih);
    efl_ui_dnd_drag_cancel(efl_drag_win, efl_drag_seat);
  }

  if (iupObjectCheck(ih))
  {
    iupAttribSet(ih, "_IUPEFL_DRAG_ACTIVE", NULL);
    iupAttribSet(ih, "_IUPEFL_DRAGEND_PENDING", "1");
    eflDragEndIdleCb(ih);
  }
  else
    eflDragCleanup();
}

static void eflDragEndUnread(void)
{
  efl_drag_accepted = 0;

#ifdef HAVE_ECORE_X
  if (efl_drag_x11_finished_handler)
  {
    eflDragX11FinishedCb(NULL, 0, NULL);
    return;
  }
#endif

  if (iupeflIsWayland())
    eflDragCompleteLocal();
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

static void eflDropPositionChangedCb(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Ui_Drop_Event* drop_ev = ev->info;
  IFniis cbDropMotion;

  cbDropMotion = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
  if (cbDropMotion && iupAttribGet(ih, "_IUPEFL_DROP_TARGET_ACTIVE"))
  {
    Eina_Rect r = efl_gfx_entity_geometry_get(ev->object);
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupeflButtonKeySetStatus(eflGetModifiers(ih), 0, status, 0);
    cbDropMotion(ih, drop_ev->position.x - r.rect.x, drop_ev->position.y - r.rect.y, status);
  }
}

typedef struct {
  Ihandle* ih;
  int x, y;
  int local;
  char type[64];
} eflDropRequest;

static Eina_Value eflDropDataSelectionCb(Eo* obj, void* data, const Eina_Value value)
{
  eflDropRequest* req = (eflDropRequest*)data;
  Ihandle* ih = req->ih;
  IFnsViii cbDropData;
  Eina_Content* content;
  Eina_Slice slice;

  (void)obj;

  content = eina_value_to_content(&value);
  cbDropData = iupObjectCheck(ih) ? (IFnsViii)IupGetCallback(ih, "DROPDATA_CB") : NULL;
  if (content && cbDropData)
  {
    slice = eina_content_data_get(content);
    if (slice.mem && slice.len > 0)
      cbDropData(ih, req->type, (void*)slice.mem, (int)slice.len, req->x, req->y);
  }

  if (req->local && iupeflIsWayland())
    eflDragCompleteLocal();

  free(req);
  return value;
}

static Eina_Value eflDropFilesSelectionCb(Eo* obj, void* data, const Eina_Value value)
{
  eflDropRequest* dfd = (eflDropRequest*)data;
  Ihandle* ih = dfd->ih;
  int drop_x = dfd->x;
  int drop_y = dfd->y;
  IFnsiii cbDropFiles;
  Eina_Content* content;
  Eina_Slice slice;
  char* dataCopy;
  char* savePtr = NULL;
  char* line;
  int count = 0;
  int remaining;

  (void)obj;

  free(dfd);

  content = eina_value_to_content(&value);
  if (!content)
    return value;

  cbDropFiles = iupObjectCheck(ih) ? (IFnsiii)IupGetCallback(ih, "DROPFILES_CB") : NULL;
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

  {
    const char* p = (const char*)slice.mem;
    const char* end = p + slice.len;
    while (p < end)
    {
      if (*p == '\n')
        count++;
      p++;
    }
    if (slice.len > 0 && ((const char*)slice.mem)[slice.len - 1] != '\n')
      count++;
  }

  savePtr = NULL;
  remaining = count - 1;
  line = strtok_r(dataCopy, "\r\n", &savePtr);

  while (line)
  {
    char* filename = line;

    if (strncmp(filename, "file://", 7) == 0)
      filename += 7;

    if (cbDropFiles(ih, filename, remaining, drop_x, drop_y) == IUP_IGNORE)
      break;

    remaining--;
    line = strtok_r(NULL, "\r\n", &savePtr);
  }

  free(dataCopy);

  return value;
}

static int eflDropTypeAvailable(Eina_Accessor* available, const char* mime)
{
  const char* offered;
  unsigned int i;

  if (!available)
    return 0;

  EINA_ACCESSOR_FOREACH(available, i, offered)
  {
    if (offered && strcmp(offered, mime) == 0)
      return 1;
  }
  return 0;
}

static void eflDropDroppedCb(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Ui_Drop_Dropped_Event* drop_ev = ev->info;
  int drop_x = 0, drop_y = 0;
  int requested = 0;

  if (drop_ev)
  {
    Eina_Rect r = efl_gfx_entity_geometry_get(ev->object);
    drop_x = drop_ev->dnd.position.x - r.rect.x;
    drop_y = drop_ev->dnd.position.y - r.rect.y;
  }

  if (drop_ev && iupAttribGet(ih, "_IUPEFL_DROP_TARGET_ACTIVE") && IupGetCallback(ih, "DROPDATA_CB"))
  {
    const char* types = iupAttribGet(ih, "_IUPEFL_DROP_TYPES");
    char type[64];
    const char* mime = NULL;

    while (types && *types)
    {
      int len;
      const char* next = iupStrNextValue(types, (int)strlen(types), &len, ',');
      if (len > 0 && len < (int)sizeof(type))
      {
        memcpy(type, types, len);
        type[len] = '\0';
        if (eflDropTypeAvailable(drop_ev->dnd.available_types, eflParseMimeType(type)))
        {
          mime = eflParseMimeType(type);
          break;
        }
      }
      types = next;
    }

    if (mime)
    {
      Eina_Array* mimes = eina_array_new(1);
      Eina_Future* future;
      eina_array_push(mimes, mime);
      future = efl_ui_dnd_drop_data_get(ev->object, drop_ev->dnd.seat, eina_array_iterator_new(mimes));
      if (future)
      {
        eflDropRequest* req = (eflDropRequest*)calloc(1, sizeof(eflDropRequest));
        requested = 1;
        if (req)
        {
          req->ih = ih;
          req->x = drop_x;
          req->y = drop_y;
          req->local = efl_drag_source_ih != NULL;
          strcpy(req->type, type);
          efl_future_then(ev->object, future, .success = eflDropDataSelectionCb, .data = req);
        }
      }
      eina_array_free(mimes);

      if (requested && efl_drag_source_ih)
        efl_drag_accepted = 1;
    }
  }

  if (!requested && drop_ev && iupAttribGet(ih, "_IUPEFL_DROPFILES_ACTIVE") && IupGetCallback(ih, "DROPFILES_CB"))
  {
    Eina_Array* types = eina_array_new(1);
    Eina_Future* future;

    eina_array_push(types, "text/uri-list");
    future = efl_ui_dnd_drop_data_get(ev->object, drop_ev->dnd.seat, eina_array_iterator_new(types));
    if (future)
    {
      eflDropRequest* dfd = (eflDropRequest*)calloc(1, sizeof(eflDropRequest));
      requested = 1;
      if (dfd)
      {
        dfd->ih = ih;
        dfd->x = drop_x;
        dfd->y = drop_y;
        efl_future_then(ev->object, future, .success = eflDropFilesSelectionCb, .data = dfd);
      }
    }
    eina_array_free(types);
  }

  if (!requested && efl_drag_source_ih)
    eflDragEndUnread();
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

static Eo* eflDropGetWidget(Ihandle* ih)
{
  Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (widget)
    return widget;
  if (ih->iclass->nativetype == IUP_TYPECANVAS)
    return iupeflCanvasGetOverlayWidget(ih);
  return iupeflGetWidget(ih);
}

static void eflDropUpdateCallbacks(Ihandle* ih)
{
  Eo* widget = eflDropGetWidget(ih);
  int wanted = iupAttribGet(ih, "_IUPEFL_DROP_TARGET_ACTIVE") || iupAttribGet(ih, "_IUPEFL_DROPFILES_ACTIVE");
  Eo* added = (Eo*)iupAttribGet(ih, "_IUPEFL_DROP_WIDGET");

  if (wanted && !added && widget)
  {
    efl_event_callback_add(widget, EFL_UI_DND_EVENT_DROP_POSITION_CHANGED, eflDropPositionChangedCb, ih);
    efl_event_callback_add(widget, EFL_UI_DND_EVENT_DROP_DROPPED, eflDropDroppedCb, ih);
    iupAttribSet(ih, "_IUPEFL_DROP_WIDGET", (char*)widget);
  }
  else if (!wanted && added)
  {
    efl_event_callback_del(added, EFL_UI_DND_EVENT_DROP_POSITION_CHANGED, eflDropPositionChangedCb, ih);
    efl_event_callback_del(added, EFL_UI_DND_EVENT_DROP_DROPPED, eflDropDroppedCb, ih);
    iupAttribSet(ih, "_IUPEFL_DROP_WIDGET", NULL);
  }
}

static int eflSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  iupAttribSet(ih, "_IUPEFL_DROP_TARGET_ACTIVE", iupStrBoolean(value) ? "1" : NULL);
  eflDropUpdateCallbacks(ih);
  return 1;
}

/*****************************************************************************
 * Drag Source Callbacks (Modern EFL API)
 *****************************************************************************/

#ifdef HAVE_ECORE_X
static char* efl_drag_x11_target = NULL;
static char* efl_drag_x11_data = NULL;
static int efl_drag_x11_size = 0;

static Eina_Bool eflDragX11Converter(char* target, void* data, int size, void** data_ret, int* size_ret, Ecore_X_Atom* ttype, int* typesize)
{
  (void)data;
  (void)size;

  if (!efl_drag_x11_data || !efl_drag_x11_target || strcmp(target, efl_drag_x11_target) != 0)
    return EINA_FALSE;

  *data_ret = malloc(efl_drag_x11_size);
  if (!*data_ret)
    return EINA_FALSE;

  memcpy(*data_ret, efl_drag_x11_data, efl_drag_x11_size);
  *size_ret = efl_drag_x11_size;
  *ttype = ecore_x_atom_get(target);
  *typesize = 8;
  return EINA_TRUE;
}

static int eflDragX11HasConverter(const char* mime)
{
  static const char* efl_x11_types[] = {
    "text/plain", "text/plain;charset=utf-8", "image/png", "image/jpeg", "image/x-ms-bmp", "image/gif",
    "image/tiff", "image/svg+xml", "image/x-xpixmap", "image/x-tga", "image/x-portable-pixmap",
    "text/x-vcard", "text/uri-list", "application/x-elementary-markup", NULL
  };
  int i;

  for (i = 0; efl_x11_types[i]; i++)
  {
    if (strcmp(mime, efl_x11_types[i]) == 0)
      return 1;
  }
  return 0;
}

/* ecore_evas_x converts only the types above, others are refused to the drop target */
static void eflDragX11SetData(const char* mime, const char* data, int size)
{
  free(efl_drag_x11_data);
  efl_drag_x11_data = NULL;
  efl_drag_x11_size = 0;

  if (!iupeflIsX11() || eflDragX11HasConverter(mime))
    return;

  if (!efl_drag_x11_target || strcmp(efl_drag_x11_target, mime) != 0)
  {
    if (efl_drag_x11_target)
    {
      ecore_x_selection_converter_del(efl_drag_x11_target);
      free(efl_drag_x11_target);
    }
    efl_drag_x11_target = strdup(mime);
    ecore_x_selection_converter_add(efl_drag_x11_target, eflDragX11Converter);
  }

  efl_drag_x11_data = (char*)malloc(size);
  if (efl_drag_x11_data)
  {
    memcpy(efl_drag_x11_data, data, size);
    efl_drag_x11_size = size;
  }
}
#endif

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

#ifdef HAVE_ECORE_X
  if (efl_drag_x11_finished_handler)
    eflDragX11FinishedCb(NULL, 0, NULL);
#endif

  if (IupClassMatch(ih, "tree"))
    iupeflTreeDragSelect(ih, x, y);

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

  efl_drag_source_ih = ih;
  efl_drag_is_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE");
  efl_drag_accepted = 0;

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

#ifdef HAVE_ECORE_X
  if (content)
    eflDragX11SetData(mime_type, dragData, size);
#endif

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
      efl_drag_win = win;
      efl_drag_seat = seat_id;

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

          drag_content = efl_add(EFL_UI_TEXTBOX_CLASS, drag_win,
                                 efl_text_interactive_editable_set(efl_added, EINA_FALSE),
                                 efl_ui_textbox_cnp_dnd_mode_set(efl_added, EFL_UI_TEXTBOX_CNP_CONTENT_NOTHING),
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

static void eflDragSourcePointerDownCb(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos, origin;
  int button;

  button = efl_input_pointer_button_get(pointer);
  if (button != 1)
    return;

  pos = efl_input_pointer_position_get(pointer);
  origin = efl_gfx_entity_position_get(ev->object);
  iupAttribSetInt(ih, "_IUPEFL_DRAG_START_X", pos.x - origin.x);
  iupAttribSetInt(ih, "_IUPEFL_DRAG_START_Y", pos.y - origin.y);
  iupAttribSet(ih, "_IUPEFL_DRAG_PENDING", "1");
}

static void eflDragSourcePointerMoveCb(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos, origin;
  int startX, startY, dx, dy;

  if (!iupAttribGet(ih, "_IUPEFL_DRAG_PENDING"))
    return;

  pos = efl_input_pointer_position_get(pointer);
  origin = efl_gfx_entity_position_get(ev->object);
  startX = iupAttribGetInt(ih, "_IUPEFL_DRAG_START_X");
  startY = iupAttribGetInt(ih, "_IUPEFL_DRAG_START_Y");
  dx = pos.x - origin.x - startX;
  dy = pos.y - origin.y - startY;

  if (dx*dx + dy*dy > 25)
  {
    iupAttribSet(ih, "_IUPEFL_DRAG_PENDING", NULL);
    eflStartDrag(ih, startX, startY);
  }
}

static void eflDragSourcePointerUpCb(void* data, const Efl_Event* ev)
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
 * File Drop Support (Ecore_Evas level)
 *****************************************************************************/

extern void ecore_evas_dnd_mark_motion_used(Ecore_Evas* ee, unsigned int seat);

static int eflSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  iupAttribSet(ih, "_IUPEFL_DROPFILES_ACTIVE", iupStrBoolean(value) ? "1" : NULL);
  eflDropUpdateCallbacks(ih);
  return 1;
}

IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
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
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, eflSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, eflSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}

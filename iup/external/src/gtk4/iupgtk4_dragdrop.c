/** \file
 * \brief GTK4 Drag&Drop Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <gtk/gtk.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_image.h"

#include "iupgtk4_drv.h"


/* Note: GTK4 uses event controllers (GtkDragSource/GtkDropTarget) which work better than GTK3's signal-based approach.
   Each widget can have multiple controllers for different drag-drop scenarios. */

static int gtk4SetDragSourceAttrib(Ihandle* ih, const char* value);
static int gtk4SetDropTargetAttrib(Ihandle* ih, const char* value);

static gboolean gtk4DropTargetDrop(GtkDropTarget *target, const GValue *value, double x, double y, Ihandle *ih)
{
  IFnsViii cbDropData = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  GBytes *bytes;
  gconstpointer data;
  gsize size;
  char *type;

  (void)target;

  if (!cbDropData)
    return FALSE;

  /* Get bytes directly from GValue - no stream reading! */
  if (!G_VALUE_HOLDS(value, G_TYPE_BYTES))
    return FALSE;

  bytes = g_value_get_boxed(value);
  if (!bytes)
    return FALSE;

  data = g_bytes_get_data(bytes, &size);
  if (!data || size == 0)
    return FALSE;

  /* Get the drop type from stored attribute */
  type = iupAttribGet(ih, "_IUPGTK4_DROP_TYPE");
  if (!type)
    return FALSE;

  /* Call the drop data callback with the data */
  cbDropData(ih, type, (void*)data, (int)size, (int)x, (int)y);

  return TRUE;
}

static GdkDragAction gtk4DropTargetMotion(GtkDropTarget *target, double x, double y, Ihandle *ih)
{
  IFniis cbDropMotion = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");

  if (cbDropMotion)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType mask;

    /* Get modifier state from the event controller */
    mask = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(target));

    iupgtk4ButtonKeySetStatus(mask, 0, status, 0);
    cbDropMotion(ih, (int)x, (int)y, status);
  }

  /* GtkDropTarget motion callback doesn't select action - drop target does */
  return GDK_ACTION_COPY;
}

static GdkContentProvider* gtk4DragSourcePrepare(GtkDragSource *source, double x, double y, Ihandle *ih)
{
  IFnii cbDragBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  IFns cbDragDataSize = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
  IFnsVi cbDragData = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
  char *type;
  int size;
  void* sourceData;
  GBytes *bytes;
  GdkContentProvider *provider;

  (void)source;

  if (cbDragBegin)
  {
    int ret = cbDragBegin(ih, (int)x, (int)y);
    if (ret == IUP_IGNORE)
      return NULL;
  }

  if (!cbDragData || !cbDragDataSize)
    return NULL;

  /* Get the first drag type */
  type = iupAttribGet(ih, "_IUPGTK4_DRAG_TYPE");
  if (!type)
    return NULL;

  size = cbDragDataSize(ih, type);
  if (size <= 0)
    return NULL;

  sourceData = g_malloc(size);
  if (!sourceData)
    return NULL;

  cbDragData(ih, type, sourceData, size);

  bytes = g_bytes_new_take(sourceData, size);

  /* Create content provider for custom type */
  provider = gdk_content_provider_new_for_bytes(type, bytes);
  g_bytes_unref(bytes);

  return provider;
}

static void gtk4DragSourceDragEnd(GtkDragSource *source, GdkDrag *drag, gboolean delete_data, Ihandle *ih)
{
  IFni cbDrag = (IFni)IupGetCallback(ih, "DRAGEND_CB");

  (void)source;
  (void)drag;

  if (cbDrag)
  {
    int remove = delete_data ? 1 : 0;
    cbDrag(ih, remove);
  }
}

static void gtk4DragSourceDragBegin(GtkDragSource *source, GdkDrag *drag, Ihandle *ih)
{
  char* value = iupAttribGet(ih, "DRAGCURSOR");

  (void)source;

  if (value)
  {
    GdkTexture* texture = iupImageGetImage(value, ih, 0, NULL);
    if (texture)
      gtk_drag_source_set_icon(source, GDK_PAINTABLE(texture), 0, 0);
  }
}

static GdkContentFormats* gtk4CreateContentFormats(const char* value)
{
  GdkContentFormatsBuilder *builder = gdk_content_formats_builder_new();
  char valueCopy[256];
  char valueTemp1[256];
  char valueTemp2[256];

  strcpy(valueCopy, value);
  while (iupStrToStrStr(valueCopy, valueTemp1, valueTemp2, ',') > 0)
  {
    gdk_content_formats_builder_add_mime_type(builder, valueTemp1);

    if (iupStrEqualNoCase(valueTemp2, valueTemp1))
      break;

    strcpy(valueCopy, valueTemp2);
  }

  return gdk_content_formats_builder_free_to_formats(builder);
}

/******************************************************************************************/

static int gtk4SetDropTypesAttrib(Ihandle* ih, const char* value)
{
  GdkContentFormats *formats = (GdkContentFormats*)iupAttribGet(ih, "_IUPGTK4_DROP_FORMATS");
  if (formats)
  {
    gdk_content_formats_unref(formats);
    iupAttribSet(ih, "_IUPGTK4_DROP_FORMATS", NULL);
  }

  if (!value)
    return 0;

  formats = gtk4CreateContentFormats(value);
  iupAttribSet(ih, "_IUPGTK4_DROP_FORMATS", (char*)formats);

  /* Store first type for later use */
  char valueTemp1[256], valueTemp2[256];
  char valueCopy[256];
  strcpy(valueCopy, value);
  if (iupStrToStrStr(valueCopy, valueTemp1, valueTemp2, ',') > 0)
    iupAttribSetStr(ih, "_IUPGTK4_DROP_TYPE", valueTemp1);

  if (ih->handle && iupAttribGetBoolean(ih, "DROPTARGET"))
    gtk4SetDropTargetAttrib(ih, "YES");

  return 1;
}

/* Helper to read stream chunk asynchronously */
typedef struct {
  GdkContentDeserializer *deserializer;
  GByteArray *array;
  guint8 *buffer;
} gtk4DeserializerData;

static void gtk4BytesDeserializerReadCallback(GObject *source, GAsyncResult *result, gpointer user_data)
{
  gtk4DeserializerData *data = (gtk4DeserializerData*)user_data;
  GInputStream *stream = G_INPUT_STREAM(source);
  GError *error = NULL;
  GValue *value;
  GBytes *bytes;
  gssize read_bytes;

  /* This is actually synchronous in callback context but won't block main loop */
  read_bytes = g_input_stream_read_finish(stream, result, &error);

  if (read_bytes > 0)
  {
    g_byte_array_append(data->array, data->buffer, read_bytes);
    /* Continue reading next chunk */
    g_input_stream_read_async(stream, data->buffer, 8192, G_PRIORITY_DEFAULT, NULL, gtk4BytesDeserializerReadCallback, data);
    return;
  }

  /* EOF or error - finish deserialization */

  if (error)
  {
    g_byte_array_free(data->array, TRUE);
    g_free(data->buffer);
    gdk_content_deserializer_return_error(data->deserializer, error);
    g_free(data);
    return;
  }

  /* Convert byte array to GBytes */
  bytes = g_byte_array_free_to_bytes(data->array);

  /* Get the value to populate from the deserializer */
  value = gdk_content_deserializer_get_value(data->deserializer);
  /* Value is already initialized by GTK4 - just set it */
  g_value_set_boxed(value, bytes);
  g_bytes_unref(bytes);  /* Unref since g_value_set_boxed copies */

  gdk_content_deserializer_return_success(data->deserializer);
  g_free(data->buffer);
  g_free(data);
}

static void gtk4BytesDeserializer(GdkContentDeserializer *deserializer)
{
  GInputStream *stream = gdk_content_deserializer_get_input_stream(deserializer);
  gtk4DeserializerData *data = g_new0(gtk4DeserializerData, 1);

  data->deserializer = deserializer;
  data->array = g_byte_array_new();
  data->buffer = g_malloc(8192);

  /* Start async read - this won't block the main loop */
  g_input_stream_read_async(stream, data->buffer, 8192, G_PRIORITY_DEFAULT, NULL, gtk4BytesDeserializerReadCallback, data);
}

static int gtk4SetDropTargetAttrib(Ihandle* ih, const char* value)
{
  GtkDropTarget *drop_target = (GtkDropTarget*)iupAttribGet(ih, "_IUPGTK4_DROP_TARGET");

  if (drop_target && ih->handle)
  {
    gtk_widget_remove_controller(ih->handle, GTK_EVENT_CONTROLLER(drop_target));
    iupAttribSet(ih, "_IUPGTK4_DROP_TARGET", NULL);
  }

  if (iupStrBoolean(value))
  {
    GdkContentFormats *formats = (GdkContentFormats*)iupAttribGet(ih, "_IUPGTK4_DROP_FORMATS");
    char *type;

    if (!formats || !ih->handle)
      return 0;

    /* Register custom deserializer for our MIME type to G_TYPE_BYTES */
    type = iupAttribGet(ih, "_IUPGTK4_DROP_TYPE");
    if (type)
      gdk_content_register_deserializer(type, G_TYPE_BYTES, gtk4BytesDeserializer, NULL, NULL);

    /* Create GtkDropTarget with G_TYPE_BYTES
       This works for custom binary data without needing stream reading!
       The bytes are delivered directly via GValue in the drop callback. */
    drop_target = gtk_drop_target_new(G_TYPE_BYTES, GDK_ACTION_MOVE | GDK_ACTION_COPY);

    g_signal_connect(drop_target, "drop", G_CALLBACK(gtk4DropTargetDrop), ih);
    g_signal_connect(drop_target, "motion", G_CALLBACK(gtk4DropTargetMotion), ih);

    gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(drop_target));
    iupAttribSet(ih, "_IUPGTK4_DROP_TARGET", (char*)drop_target);
  }

  return 1;
}

static int gtk4SetDragTypesAttrib(Ihandle* ih, const char* value)
{
  GdkContentFormats *formats = (GdkContentFormats*)iupAttribGet(ih, "_IUPGTK4_DRAG_FORMATS");
  if (formats)
  {
    gdk_content_formats_unref(formats);
    iupAttribSet(ih, "_IUPGTK4_DRAG_FORMATS", NULL);
  }

  if (!value)
    return 0;

  formats = gtk4CreateContentFormats(value);
  iupAttribSet(ih, "_IUPGTK4_DRAG_FORMATS", (char*)formats);

  /* Store first type for later use */
  char valueTemp1[256], valueTemp2[256];
  char valueCopy[256];
  strcpy(valueCopy, value);
  if (iupStrToStrStr(valueCopy, valueTemp1, valueTemp2, ',') > 0)
    iupAttribSetStr(ih, "_IUPGTK4_DRAG_TYPE", valueTemp1);

  if (ih->handle && iupAttribGetBoolean(ih, "DRAGSOURCE"))
    gtk4SetDragSourceAttrib(ih, "YES");

  return 1;
}

static int gtk4SetDragSourceAttrib(Ihandle* ih, const char* value)
{
  GtkDragSource *drag_source = (GtkDragSource*)iupAttribGet(ih, "_IUPGTK4_DRAG_SOURCE");

  if (drag_source && ih->handle)
  {
    gtk_widget_remove_controller(ih->handle, GTK_EVENT_CONTROLLER(drag_source));
    iupAttribSet(ih, "_IUPGTK4_DRAG_SOURCE", NULL);
  }

  if (iupStrBoolean(value))
  {
    GdkContentFormats *formats = (GdkContentFormats*)iupAttribGet(ih, "_IUPGTK4_DRAG_FORMATS");
    GdkDragAction actions;

    if (!formats || !ih->handle)
      return 0;

    drag_source = gtk_drag_source_new();

    actions = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ?
              GDK_ACTION_MOVE | GDK_ACTION_COPY : GDK_ACTION_COPY;

    gtk_drag_source_set_actions(drag_source, actions);
    gtk_drag_source_set_content(drag_source, NULL);  /* Set in prepare callback */

    g_signal_connect(drag_source, "prepare", G_CALLBACK(gtk4DragSourcePrepare), ih);
    g_signal_connect(drag_source, "drag-begin", G_CALLBACK(gtk4DragSourceDragBegin), ih);
    g_signal_connect(drag_source, "drag-end", G_CALLBACK(gtk4DragSourceDragEnd), ih);

    gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(drag_source));
    iupAttribSet(ih, "_IUPGTK4_DRAG_SOURCE", (char*)drag_source);
  }

  return 1;
}

static gboolean gtk4DropFileDrop(GtkDropTarget *target, const GValue *value, double x, double y, Ihandle *ih)
{
  IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  GSList *file_list;
  GSList *l;
  int count, i;

  (void)target;

  if (!cb)
    return FALSE;

  if (!G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST))
    return FALSE;

  file_list = g_value_get_boxed(value);
  if (!file_list)
    return FALSE;

  count = g_slist_length(file_list);

  /* Call callback for each file */
  i = 0;
  for (l = file_list; l != NULL; l = l->next)
  {
    GFile *file = (GFile*)l->data;
    char *filename = g_file_get_path(file);

    if (!filename)
      filename = g_file_get_uri(file);

    if (filename)
    {
      /* Remove file:// prefix if present */
      char *final_name = filename;
      if (iupStrEqualPartial(filename, "file://"))
      {
        final_name = filename + strlen("file://");
        if (final_name[2] == ':')  /* Windows extra '/' */
          final_name++;
      }

      if (cb(ih, final_name, count - i - 1, (int)x, (int)y) == IUP_IGNORE)
      {
        g_free(filename);
        break;
      }
      g_free(filename);
    }
    i++;
  }

  return TRUE;
}

static int gtk4SetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  GtkDropTarget *drop_target = (GtkDropTarget*)iupAttribGet(ih, "_IUPGTK4_DROPFILES_TARGET");

  if (drop_target)
  {
    gtk_widget_remove_controller(ih->handle, GTK_EVENT_CONTROLLER(drop_target));
    iupAttribSet(ih, "_IUPGTK4_DROPFILES_TARGET", NULL);
  }

  if (iupStrBoolean(value))
  {
    drop_target = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);

    g_signal_connect(drop_target, "drop", G_CALLBACK(gtk4DropFileDrop), ih);

    gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(drop_target));
    iupAttribSet(ih, "_IUPGTK4_DROPFILES_TARGET", (char*)drop_target);
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

  iupClassRegisterAttribute(ic, "DRAGTYPES",  NULL, gtk4SetDragTypesAttrib,  NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  NULL, gtk4SetDropTypesAttrib,  NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, gtk4SetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, gtk4SetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, gtk4SetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, gtk4SetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}

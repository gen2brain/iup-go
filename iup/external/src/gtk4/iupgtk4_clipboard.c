/** \file
 * \brief Clipboard for the GTK4 Driver.
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupgtk4_drv.h"


static const char* gtk4ClipboardGetFormatMimeType(Ihandle *ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return NULL;
  return format;
}

typedef struct {
  const char* mime_type;
  void* data;
  int size;
} gtk4ClipInfo;

/* Clipboard provider callback - called when clipboard is pasted */
static void gtk4ClipboardProvideData(GdkClipboard *clipboard, GOutputStream *stream,
                                     const char *mime_type, int io_priority,
                                     GCancellable *cancellable, gpointer user_data)
{
  gtk4ClipInfo* clip_info = (gtk4ClipInfo*)user_data;
  GError *error = NULL;

  /* Write data to output stream */
  g_output_stream_write_all(stream, clip_info->data, clip_info->size, NULL, cancellable, &error);

  if (error)
  {
    g_error_free(error);
  }

  (void)clipboard;
  (void)mime_type;
  (void)io_priority;
}

static void gtk4ClipboardDataClearFunc(gpointer user_data)
{
  gtk4ClipInfo* clip_info = (gtk4ClipInfo*)user_data;
  free(clip_info->data);
  free(clip_info);
}

static int gtk4ClipboardSetFormatDataAttrib(Ihandle *ih, const char *value)
{
  gtk4ClipInfo* clip_info;
  int size;
  const char* mime_type;
  void* data;
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);

  if (!value)
  {
    /* Clear clipboard by setting empty content */
    gdk_clipboard_set_content(clipboard, NULL);
    return 0;
  }

  mime_type = gtk4ClipboardGetFormatMimeType(ih);
  if (mime_type==NULL)
    return 0;

  size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (!size)
    return 0;

  data = malloc(size);
  if (!data)
    return 0;
  memcpy(data, value, size);

  clip_info = malloc(sizeof(gtk4ClipInfo));
  clip_info->data = data;
  clip_info->size = size;
  clip_info->mime_type = mime_type;

  /* Use GdkContentProvider for custom formats */
  GdkContentFormats *formats = gdk_content_formats_new((const char **)&mime_type, 1);
  GdkContentProvider *provider = gdk_content_provider_new_for_bytes(mime_type,
                                   g_bytes_new_with_free_func(data, size,
                                   gtk4ClipboardDataClearFunc, clip_info));

  gdk_clipboard_set_content(clipboard, provider);

  g_object_unref(provider);
  gdk_content_formats_unref(formats);

  return 0;
}

/* Async callback data for synchronous clipboard read */
typedef struct {
  GInputStream *stream;
  GError *error;
  GMainLoop *loop;
  const char *mime_type;
} gtk4ClipboardReadData;

static void gtk4ClipboardReadCallback(GObject *source, GAsyncResult *result, gpointer user_data)
{
  gtk4ClipboardReadData *read_data = (gtk4ClipboardReadData*)user_data;

  read_data->stream = gdk_clipboard_read_finish(GDK_CLIPBOARD(source), result,
                                                 &read_data->mime_type, &read_data->error);

  g_main_loop_quit(read_data->loop);
}

static char* gtk4ClipboardGetFormatDataAttrib(Ihandle *ih)
{
  const char* mime_type;
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);
  GInputStream *stream;
  void *data;
  gsize size = 0;
  const char* mime_types[2];
  gtk4ClipboardReadData read_data;
  GError *error = NULL;

  mime_type = gtk4ClipboardGetFormatMimeType(ih);
  if (mime_type==NULL)
    return NULL;

  mime_types[0] = mime_type;
  mime_types[1] = NULL;

  /* Read clipboard data using async API with nested main loop */
  read_data.stream = NULL;
  read_data.error = NULL;
  read_data.loop = g_main_loop_new(NULL, FALSE);
  read_data.mime_type = NULL;

  gdk_clipboard_read_async(clipboard, mime_types, G_PRIORITY_DEFAULT, NULL, gtk4ClipboardReadCallback, &read_data);

  /* Run nested loop until callback completes */
  g_main_loop_run(read_data.loop);
  g_main_loop_unref(read_data.loop);

  stream = read_data.stream;
  error = read_data.error;

  if (!stream || error)
  {
    if (error)
      g_error_free(error);
    return NULL;
  }

  /* Read all data from stream */
  GOutputStream *mem_stream = g_memory_output_stream_new_resizable();
  g_output_stream_splice(mem_stream, stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                         NULL, &error);

  if (error)
  {
    g_error_free(error);
    g_object_unref(stream);
    g_object_unref(mem_stream);
    return NULL;
  }

  size = g_memory_output_stream_get_data_size(G_MEMORY_OUTPUT_STREAM(mem_stream));
  gpointer mem_data = g_memory_output_stream_steal_data(G_MEMORY_OUTPUT_STREAM(mem_stream));

  g_object_unref(stream);
  g_object_unref(mem_stream);

  if (size <= 0 || !mem_data)
    return NULL;

  data = iupStrGetMemory((int)size + 1); /* reserve room for terminator */
  memcpy(data, mem_data, size);
  g_free(mem_data);

  iupAttribSetInt(ih, "FORMATDATASIZE", (int)size);
  return data;
}

static char* gtk4ClipboardGetFormatDataStringAttrib(Ihandle *ih)
{
  char* data = gtk4ClipboardGetFormatDataAttrib(ih);
  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (data)
    data[size] = 0;  /* add terminator */
  return iupStrReturnStr(iupgtk4StrConvertFromSystem(data));
}

static int gtk4ClipboardSetFormatDataStringAttrib(Ihandle *ih, const char *value)
{
  if (value)
  {
    int len = (int)strlen(value);
    char* wstr = iupgtk4StrConvertToSystemLen(value, &len);
    iupAttribSetInt(ih, "FORMATDATASIZE", len + 1);  /* include terminator */
    return gtk4ClipboardSetFormatDataAttrib(ih, wstr);
  }
  else
    return gtk4ClipboardSetFormatDataAttrib(ih, NULL);
}

static int gtk4ClipboardSetTextAttrib(Ihandle *ih, const char *value)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);

  if (!value)
  {
    gdk_clipboard_set_content(clipboard, NULL);
    return 0;
  }

  gdk_clipboard_set_text(clipboard, iupgtk4StrConvertToSystem(value));

  (void)ih;
  return 0;
}

/* Callback data structure for text clipboard reading */
typedef struct {
  char *text;
  GMainLoop *loop;
} gtk4ClipboardTextData;

static void text_ready_cb(GObject *source, GAsyncResult *result, gpointer user_data)
{
  gtk4ClipboardTextData *data = (gtk4ClipboardTextData*)user_data;
  GError *error = NULL;
  data->text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, &error);
  if (error)
    g_error_free(error);
  g_main_loop_quit(data->loop);
}

static char* gtk4ClipboardGetTextAttrib(Ihandle *ih)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);
  gtk4ClipboardTextData data;

  /* Synchronous text reading using async API with blocking */
  /* We need to use the async API but wait for it */
  data.text = NULL;
  data.loop = g_main_loop_new(NULL, FALSE);

  gdk_clipboard_read_text_async(clipboard, NULL, text_ready_cb, &data);
  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);

  (void)ih;
  if (data.text)
  {
    char* result = iupStrReturnStr(iupgtk4StrConvertFromSystem(data.text));
    g_free(data.text);
    return result;
  }
  return NULL;
}

static int gtk4ClipboardSetImageAttrib(Ihandle *ih, const char *value)
{
  GdkTexture *texture;
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);

  if (!value)
  {
    gdk_clipboard_set_content(clipboard, NULL);
    return 0;
  }

  /* GdkPixbuf → GdkTexture for clipboard */
  texture = (GdkTexture*)iupImageGetImage(value, ih, 0, NULL);
  if (texture)
    gdk_clipboard_set_texture(clipboard, texture);

  return 0;
}

static int gtk4ClipboardSetNativeImageAttrib(Ihandle *ih, const char *value)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);
  (void)ih;

  if (!value)
  {
    gdk_clipboard_set_content(clipboard, NULL);
    return 0;
  }

  gdk_clipboard_set_texture(clipboard, (GdkTexture*)value);

  return 0;
}

/* Callback data structure for texture clipboard reading */
typedef struct {
  GdkTexture *texture;
  GMainLoop *loop;
} gtk4ClipboardTextureData;

static void texture_ready_cb(GObject *source, GAsyncResult *result, gpointer user_data)
{
  gtk4ClipboardTextureData *data = (gtk4ClipboardTextureData*)user_data;
  GError *error = NULL;
  data->texture = gdk_clipboard_read_texture_finish(GDK_CLIPBOARD(source), result, &error);
  if (error)
    g_error_free(error);
  g_main_loop_quit(data->loop);
}

static char* gtk4ClipboardGetNativeImageAttrib(Ihandle *ih)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);
  gtk4ClipboardTextureData data;

  /* Synchronous texture reading using async API with blocking */
  data.texture = NULL;
  data.loop = g_main_loop_new(NULL, FALSE);

  gdk_clipboard_read_texture_async(clipboard, NULL, texture_ready_cb, &data);
  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);

  (void)ih;
  return (char*)data.texture;
}

static char* gtk4ClipboardGetTextAvailableAttrib(Ihandle *ih)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);
  GdkContentFormats *formats = gdk_clipboard_get_formats(clipboard);

  (void)ih;
  return iupStrReturnBoolean(gdk_content_formats_contain_mime_type(formats, "text/plain"));
}

static char* gtk4ClipboardGetImageAvailableAttrib(Ihandle *ih)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);
  GdkContentFormats *formats = gdk_clipboard_get_formats(clipboard);

  (void)ih;
  /* Check for image MIME types */
  return iupStrReturnBoolean(gdk_content_formats_contain_mime_type(formats, "image/png") ||
                             gdk_content_formats_contain_mime_type(formats, "image/jpeg") ||
                             gdk_content_formats_contain_mime_type(formats, "image/bmp"));
}

static char* gtk4ClipboardGetFormatAvailableAttrib(Ihandle *ih)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkClipboard *clipboard = gdk_display_get_clipboard(display);
  const char* mime_type = gtk4ClipboardGetFormatMimeType(ih);
  GdkContentFormats *formats;

  if (mime_type==NULL)
    return NULL;

  formats = gdk_clipboard_get_formats(clipboard);
  return iupStrReturnBoolean(gdk_content_formats_contain_mime_type(formats, mime_type));
}

static int gtk4ClipboardSetAddFormatAttrib(Ihandle *ih, const char *value)
{
  /* No pre-registration needed for MIME types */
  (void)ih;
  (void)value;
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
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", gtk4ClipboardGetTextAttrib, gtk4ClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", gtk4ClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", gtk4ClipboardGetNativeImageAttrib, gtk4ClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtk4ClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", gtk4ClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, gtk4ClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", gtk4ClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", gtk4ClipboardGetFormatDataAttrib, gtk4ClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", gtk4ClipboardGetFormatDataStringAttrib, gtk4ClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

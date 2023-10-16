/** \file
 * \brief Clipboard for the GTK Driver.
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

#include "iupgtk_drv.h"

static GdkAtom gtkClipboardGetFormatTarget(Ihandle *ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return NULL;
  return gdk_atom_intern(format, FALSE);
}

typedef struct {
  GdkAtom target;
  void* data;
  int size;
} gtkClipInfo;

static void gtkClipboardDataGetFunc(GtkClipboard *clipboard, GtkSelectionData *selection_data, 
                                    guint info, gtkClipInfo* clip_info)
{
  gtk_selection_data_set(selection_data, clip_info->target, 8, (guchar*)(clip_info->data), clip_info->size);
  (void)info;
  (void)clipboard;
}

static void gtkClipboardDataClearFunc(GtkClipboard *clipboard, gtkClipInfo* clip_info)
{
  free(clip_info->data);
  free(clip_info);
  (void)clipboard;
}

static int gtkClipboardSetFormatDataAttrib(Ihandle *ih, const char *value)
{
  gtkClipInfo* clip_info;
  GtkTargetList *list;
  GtkTargetEntry *targets;
  gint n_targets;
  int size;
  GdkAtom target;
  void* data;
  GtkClipboard *clipboard = gtk_clipboard_get_for_display(gdk_display_get_default(), gdk_atom_intern("CLIPBOARD", FALSE));
  if (!value)
  {
    gtk_clipboard_clear(clipboard);
    return 0;
  }

  target = gtkClipboardGetFormatTarget(ih);
  if (target==NULL)
    return 0;

  size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (!size)
    return 0;

  data = malloc(size);
  if (!data)
    return 0;
  memcpy(data, value, size);

  list = gtk_target_list_new (NULL, 0);
  gtk_target_list_add (list, target, 0, 0);  
  targets = gtk_target_table_new_from_list (list, &n_targets);

  clip_info = malloc(sizeof(gtkClipInfo));
  clip_info->data = data;
  clip_info->size = size;
  clip_info->target = target;

  gtk_clipboard_set_with_data (clipboard, 
			       targets, n_targets,
			       (GtkClipboardGetFunc)gtkClipboardDataGetFunc, (GtkClipboardClearFunc)gtkClipboardDataClearFunc,
			       clip_info);
  gtk_clipboard_store(clipboard);

  gtk_target_table_free (targets, n_targets);
  gtk_target_list_unref (list);

  return 0;
}

static char* gtkClipboardGetFormatDataAttrib(Ihandle *ih)
{
  int size, format;
  void* data, *clip_data;
  GtkSelectionData *selection_data;
  GtkClipboard *clipboard = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
  GdkAtom target = gtkClipboardGetFormatTarget(ih);
  if (target==NULL)
    return NULL;

  selection_data = gtk_clipboard_wait_for_contents(clipboard, target);
  if (!selection_data)
    return NULL;

#if GTK_CHECK_VERSION(2, 14, 0)
  clip_data = (void*)gtk_selection_data_get_data(selection_data);
  size = gtk_selection_data_get_length(selection_data);
  format = gtk_selection_data_get_format(selection_data);
#else
  clip_data = (void*)selection_data->data;
  size = selection_data->length;
  format = selection_data->format;
#endif

  if (size <= 0 || format != 8 || !clip_data)
    return NULL;

  data = iupStrGetMemory(size+1); /* reserve room for terminator */
  memcpy(data, clip_data, size);

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return data;
}

static char* gtkClipboardGetFormatDataStringAttrib(Ihandle *ih)
{
  char* data = gtkClipboardGetFormatDataAttrib(ih);
  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  data[size] = 0;  /* add terminator */
  return iupStrReturnStr(iupgtkStrConvertFromSystem(data));
}

static int gtkClipboardSetFormatDataStringAttrib(Ihandle *ih, const char *value)
{
  if (value)
  {
    int len = (int)strlen(value);
    char* wstr = iupgtkStrConvertToSystemLen(value, &len);
    iupAttribSetInt(ih, "FORMATDATASIZE", len + 1);  /* include terminator */
    return gtkClipboardSetFormatDataAttrib(ih, wstr);
  }
  else
    return gtkClipboardSetFormatDataAttrib(ih, NULL);
}

static int gtkClipboardSetTextAttrib(Ihandle *ih, const char *value)
{
  GtkClipboard *clipboard = gtk_clipboard_get_for_display(gdk_display_get_default(), gdk_atom_intern("CLIPBOARD", FALSE));
  if (!value)
  {
    gtk_clipboard_clear(clipboard);
    return 0;
  }
  gtk_clipboard_set_text(clipboard, iupgtkStrConvertToSystem(value), -1);
  (void)ih;
  return 0;
}

static char* gtkClipboardGetTextAttrib(Ihandle *ih)
{
  GtkClipboard *clipboard = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
  (void)ih;
  return iupgtkStrConvertFromSystem(gtk_clipboard_wait_for_text(clipboard));
}

static int gtkClipboardSetImageAttrib(Ihandle *ih, const char *value)
{
#if GTK_CHECK_VERSION(2, 6, 0)
  GdkPixbuf *pixbuf;
  GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE));
  if (!value)
  {
    gtk_clipboard_clear(clipboard);
    return 0;
  }

  pixbuf = (GdkPixbuf*)iupImageGetImage(value, ih, 0, NULL);
  if (pixbuf)
    gtk_clipboard_set_image (clipboard, pixbuf);
#endif
  return 0;
}

static int gtkClipboardSetNativeImageAttrib(Ihandle *ih, const char *value)
{
#if GTK_CHECK_VERSION(2, 6, 0)
  GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE));
  (void)ih;

  if (!value)
  {
    gtk_clipboard_clear(clipboard);
    return 0;
  }

  gtk_clipboard_set_image (clipboard, (GdkPixbuf*)value);
#endif
  return 0;
}

static char* gtkClipboardGetNativeImageAttrib(Ihandle *ih)
{
#if GTK_CHECK_VERSION(2, 6, 0)
  GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE));
  (void)ih;
  return (char*)gtk_clipboard_wait_for_image (clipboard);
#else
  return NULL;
#endif
}

static char* gtkClipboardGetTextAvailableAttrib(Ihandle *ih)
{
  GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE));
  (void)ih;
  return iupStrReturnBoolean (gtk_clipboard_wait_is_text_available(clipboard)); 
}

static char* gtkClipboardGetImageAvailableAttrib(Ihandle *ih)
{
#if GTK_CHECK_VERSION(2, 6, 0)
  GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE));
  (void)ih;
  return iupStrReturnBoolean (gtk_clipboard_wait_is_image_available(clipboard)); 
#else
  return NULL;
#endif
}

static char* gtkClipboardGetFormatAvailableAttrib(Ihandle *ih)
{
#if GTK_CHECK_VERSION(2, 6, 0)
  GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE));
  GdkAtom target = gtkClipboardGetFormatTarget(ih);
  if (target==NULL)
    return NULL;

  return iupStrReturnBoolean (gtk_clipboard_wait_is_target_available(clipboard, target)); 
#else
  return NULL;
#endif
}

static int gtkClipboardSetAddFormatAttrib(Ihandle *ih, const char *value)
{
  if (value)
    gdk_atom_intern(value, FALSE);  /* not exactly register but done for completion */

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
  iupClassRegisterAttribute(ic, "TEXT", gtkClipboardGetTextAttrib, gtkClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", gtkClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", gtkClipboardGetNativeImageAttrib, gtkClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", gtkClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, gtkClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", gtkClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", gtkClipboardGetFormatDataAttrib, gtkClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", gtkClipboardGetFormatDataStringAttrib, gtkClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

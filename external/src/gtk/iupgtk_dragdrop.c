/** \file
 * \brief GTK Drag&Drop Functions
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

#include "iupgtk_drv.h"


static void gtkDragDataReceived(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y,
                            GtkSelectionData *seldata, guint info, guint time, Ihandle *ih)
{
  IFnsViii cbDropData = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  void* targetData = NULL;
  char* type;
  int size, format;
  GdkDragAction action;

#if GTK_CHECK_VERSION(2, 22, 0)
  action = gdk_drag_context_get_selected_action(drag_context);
#else
  action = drag_context->action;
#endif

#if GTK_CHECK_VERSION(2, 14, 0)
  type = gdk_atom_name(gtk_selection_data_get_target(seldata));
  targetData = (void*)gtk_selection_data_get_data(seldata);
  size = gtk_selection_data_get_length(seldata);
  format = gtk_selection_data_get_format(seldata);
#else
  type = gdk_atom_name(seldata->type);
  targetData = (void*)seldata->data;
  size = seldata->length;
  format = seldata->format;
#endif

  if(size <= 0 || format != 8 ||
    (action != GDK_ACTION_MOVE && action != GDK_ACTION_COPY))
  {
    gtk_drag_finish(drag_context, FALSE, FALSE, time);
    return;
  }

  if (cbDropData)
    cbDropData(ih, type, targetData, size, x, y);

  gtk_drag_finish(drag_context, TRUE, FALSE, time);

  (void)info;
  (void)widget;
  (void)time;
}

static void gtkDragDataGet(GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *seldata, guint info, guint time, Ihandle* ih)
{
  IFnsVi cbDragData = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
  IFns cbDragDataSize = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
  if(cbDragData && cbDragDataSize)
  {
    void* sourceData;
    char *type;
    int size;
    GdkDragAction action;

#if GTK_CHECK_VERSION(2, 22, 0)
    action = gdk_drag_context_get_selected_action(drag_context);
#else
    action = drag_context->action;
#endif

#if GTK_CHECK_VERSION(2, 14, 0)
    type = gdk_atom_name(gtk_selection_data_get_target(seldata));
#else
    type = seldata->type;
#endif
    if (action != GDK_ACTION_MOVE && action != GDK_ACTION_COPY)
      return;

    size = cbDragDataSize(ih, type);
    if (size <= 0)
      return;

    sourceData = malloc(size);

    /* fill data */
    cbDragData(ih, type, sourceData, size);

    gtk_selection_data_set(seldata, gdk_atom_intern(type, FALSE), 8, (guchar*)sourceData, size);

    free(sourceData);
  }

  (void)widget;
  (void)drag_context;
  (void)time;
  (void)info;
}

static gboolean gtkDragMotion(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, Ihandle* ih)
{
  GdkAtom targetAtom;

  /* The third argument must be NULL. Internally, GTK will use the list returned
     by the call gtk_drag_dest_get_target_list(widget), which is the list of targets
     that be destination widget can accept (defined in the gtkSetDropTargetAttrib IUP) */
  targetAtom = gtk_drag_dest_find_target(widget, drag_context, NULL);

  if(targetAtom != GDK_NONE)
  {
    IFniis cbDropMotion = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");

    if(cbDropMotion)
    {
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      GdkModifierType mask;
      iupgtkWindowGetPointer(iupgtkGetWindow(widget), NULL, NULL, &mask);

      iupgtkButtonKeySetStatus(mask, 0, status, 0);
      cbDropMotion(ih, x, y, status);
    }

#if GTK_CHECK_VERSION(2, 22, 0)   
    gdk_drag_status(drag_context, gdk_drag_context_get_suggested_action(drag_context), time);
#else
    gdk_drag_status(drag_context, drag_context->suggested_action, time);
#endif
    return TRUE;
  }
  (void)ih;
  (void)x;
  (void)y;

  gdk_drag_status(drag_context, 0, time);
  return FALSE;
}

static void gtkDragEnd(GtkWidget *widget, GdkDragContext *drag_context, Ihandle *ih)
{
  IFni cbDrag = (IFni)IupGetCallback(ih, "DRAGEND_CB");
  if(cbDrag)
  {
    GdkDragAction action;
    int remove = -1;

#if GTK_CHECK_VERSION(2, 22, 0)
    action = gdk_drag_context_get_selected_action(drag_context);
#else
    action = drag_context->action;
#endif

    if (action == GDK_ACTION_MOVE)
      remove = 1;
    else if (action == GDK_ACTION_COPY)
      remove = 0;

    cbDrag(ih, remove);
  }

  (void)widget;
}

static void gtkDragBegin(GtkWidget *widget, GdkDragContext *drag_context, Ihandle *ih)
{
  char* value;

  IFnii cbDragBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if(cbDragBegin)
  {
    int x, y;  /* the returned position is not exactly the start position. */
    iupgtkWindowGetPointer(iupgtkGetWindow(ih->handle), &x, &y, NULL);

    if (cbDragBegin(ih, x, y) == IUP_IGNORE)
      gdk_drag_abort(drag_context, 0);
  }

  value = iupAttribGet(ih, "DRAGCURSOR");
  if (value)
  {
    GdkPixbuf* pixbuf = iupImageGetImage(value, ih, 0, NULL);
    if (pixbuf)
      gtk_drag_source_set_icon_pixbuf(widget, pixbuf);
  }
}

static GtkTargetList* gtkCreateTargetList(const char* value)
{
  GtkTargetList* targetlist = gtk_target_list_new(NULL, 0);
  char valueCopy[256];
  char valueTemp1[256];
  char valueTemp2[256];
  int info = 0;

  strcpy(valueCopy, value);
  while (iupStrToStrStr(valueCopy, valueTemp1, valueTemp2, ',') > 0)
  {
    gtk_target_list_add(targetlist, gdk_atom_intern(valueTemp1, FALSE), 0, info++);

    if (iupStrEqualNoCase(valueTemp2, valueTemp1))
      break;

    strcpy(valueCopy, valueTemp2);
  }

  if (info == 0)
  {
    gtk_target_list_unref(targetlist);
    return NULL;
  }

  return targetlist;
}


/******************************************************************************************/


static int gtkSetDropTypesAttrib(Ihandle* ih, const char* value)
{
  GtkTargetList *targetlist = (GtkTargetList*)iupAttribGet(ih, "_IUPGTK_DROP_TARGETLIST");
  if (targetlist)
  {
    gtk_target_list_unref(targetlist);
    iupAttribSet(ih, "_IUPGTK_DROP_TARGETLIST", NULL);
  }

  if(!value)
    return 0;

  targetlist = gtkCreateTargetList(value);
  iupAttribSet(ih, "_IUPGTK_DROP_TARGETLIST", (char*)targetlist);
  return 1;
}

static int gtkSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  if(iupStrBoolean(value))
  {
    GtkTargetList *targetlist = (GtkTargetList*)iupAttribGet(ih, "_IUPGTK_DROP_TARGETLIST");
    GtkTargetEntry *drop_types_entry;
    int targetlist_count;

    if(!targetlist)
      return 0;

    drop_types_entry = gtk_target_table_new_from_list(targetlist, &targetlist_count);

    gtk_drag_dest_set(ih->handle, GTK_DEST_DEFAULT_ALL, drop_types_entry, targetlist_count, GDK_ACTION_MOVE|GDK_ACTION_COPY);

    g_signal_connect(ih->handle, "drag_motion", G_CALLBACK(gtkDragMotion), ih);
    g_signal_connect(ih->handle, "drag_data_received", G_CALLBACK(gtkDragDataReceived), ih);

    gtk_target_table_free(drop_types_entry, targetlist_count);
  }
  else
    gtk_drag_dest_unset(ih->handle);

  return 1;
}

static int gtkSetDragTypesAttrib(Ihandle* ih, const char* value)
{
  GtkTargetList *targetlist = (GtkTargetList*)iupAttribGet(ih, "_IUPGTK_DRAG_TARGETLIST");
  if (targetlist)
  {
    gtk_target_list_unref(targetlist);
    iupAttribSet(ih, "_IUPGTK_DRAG_TARGETLIST", NULL);
  }

  if (!value)
    return 0;

  targetlist = gtkCreateTargetList(value);
  iupAttribSet(ih, "_IUPGTK_DRAG_TARGETLIST", (char*)targetlist);
  return 1;
}

static int gtkSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    GtkTargetList *targetlist = (GtkTargetList*)iupAttribGet(ih, "_IUPGTK_DRAG_TARGETLIST");
    GtkTargetEntry *drag_types_entry;
    int targetlist_count;

    if(!targetlist)
      return 0;

    drag_types_entry = gtk_target_table_new_from_list(targetlist, &targetlist_count);

    gtk_drag_source_set(ih->handle, GDK_BUTTON1_MASK, drag_types_entry, targetlist_count, 
                        iupAttribGetBoolean(ih, "DRAGSOURCEMOVE")? GDK_ACTION_MOVE|GDK_ACTION_COPY: GDK_ACTION_COPY);

    g_signal_connect(ih->handle, "drag_begin", G_CALLBACK(gtkDragBegin), ih);
    g_signal_connect(ih->handle, "drag_data_get", G_CALLBACK(gtkDragDataGet), ih);
    g_signal_connect(ih->handle, "drag_end", G_CALLBACK(gtkDragEnd), ih);

    gtk_target_table_free(drag_types_entry, targetlist_count);
  }
  else
    gtk_drag_source_unset(ih->handle);

  return 1;
}


/******************************************************************************************/


static void gtkDropFileDragDataReceived(GtkWidget* w, GdkDragContext* context, int x, int y,
                                        GtkSelectionData* seldata, guint info, guint time, Ihandle* ih)
{
  gchar **uris = NULL, *data = NULL;
  int i, count;

  IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (!cb) return; 

#if GTK_CHECK_VERSION(2, 6, 0)
#if GTK_CHECK_VERSION(2, 14, 0)
  data = (char*)gtk_selection_data_get_data(seldata);
#else
  data = (char*)seldata->data;
#endif
  uris = g_uri_list_extract_uris(data);
#endif

  if (!uris)
    return;

  count = 0;
  while (uris[count])
    count++;

  for (i=0; i<count; i++)
  {
    char* filename = uris[i];
    if (iupStrEqualPartial(filename, "file://"))
    {
      filename += strlen("file://");
      if (filename[2] == ':')  /* in Windows there is an extra '/' at the beginning. */
        filename++;
    }
    if (cb(ih, filename, count-i-1, x, y) == IUP_IGNORE)
      break;
  }

  g_strfreev (uris);
  (void)time;
  (void)info;
  (void)w;
  (void)context;
}

static int gtkSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    GtkTargetEntry dragtypes[] = { { "text/uri-list", 0, 0 } };
    gtk_drag_dest_set(ih->handle, GTK_DEST_DEFAULT_ALL, dragtypes,
                      sizeof(dragtypes) / sizeof(dragtypes[0]), GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(ih->handle), "drag_data_received", G_CALLBACK(gtkDropFileDragDataReceived), ih);
  }
  else
    gtk_drag_dest_unset(ih->handle);

  return 1;
}


/******************************************************************************************/


void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES",  NULL, gtkSetDragTypesAttrib,  NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  NULL, gtkSetDropTypesAttrib,  NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, gtkSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, gtkSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, gtkSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, gtkSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}

/* TODO:
  Could not find a way to disable the internal DND support 
  in IupText(GtkTextView or GtkEntry).
  Mixing the generic support from here and the internal gives weird results.
  So the application should use only the internal in this case.
  The edit box in a IupList has the same problem.
*/

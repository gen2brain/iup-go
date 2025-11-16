/** \file
 * \brief List Control for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"
#include "iup_childtree.h"

#include "iupgtk4_drv.h"


enum
{
  IUPGTK4_LIST_IMAGE,
  IUPGTK4_LIST_TEXT,
  IUPGTK4_LIST_LAST_DATA
};

static void gtk4ListSelectionChanged(GtkTreeSelection* selection, Ihandle* ih);
static void gtk4ListComboBoxChanged(GtkComboBox* widget, Ihandle* ih);

void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  (void)ih;
  *h += 2;
}

void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 2 * 5;
  int orig_x = *x, orig_y = *y;
  (*x) += border_size;
  (*y) += border_size;

  if (ih->data->is_dropdown)
  {
    /* GtkComboBox has larger internal padding and dropdown button */
    (*x) += 15; /* dropdown button and internal padding */

    if (ih->data->has_editbox)
      (*x) += 5; /* additional space for editbox combo */
    else
    {
      (*y) += 4;
      (*x) += 5; /* additional padding for non-editbox dropdown */
    }
  }
  else
  {
    if (ih->data->has_editbox)
    {
      /* Add spacing between editbox and list */
      (*y) += 2*3;
      /* Add text entry border size to match standalone IupText */
      (*y) += 2*9;
    }
  }
}

static int gtk4ListConvertXYToPos(Ihandle* ih, int x, int y)
{
  if (!ih->data->is_dropdown)
  {
    GtkTreePath* path;
    if (gtk_tree_view_get_dest_row_at_pos((GtkTreeView*)ih->handle, x, y, &path, NULL))
    {
      int* indices = gtk_tree_path_get_indices(path);
      int pos = indices[0]+1;
      gtk_tree_path_free (path);
      return pos;
    }
  }

  return -1;
}

static GtkTreeModel* gtk4ListGetModel(Ihandle* ih)
{
  if (ih->data->is_dropdown)
    return gtk_combo_box_get_model((GtkComboBox*)ih->handle);
  else
    return gtk_tree_view_get_model((GtkTreeView*)ih->handle);
}

int iupdrvListGetCount(Ihandle* ih)
{
  GtkTreeModel *model = gtk4ListGetModel(ih);
  return gtk_tree_model_iter_n_children(model, NULL);
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  GtkTreeModel *model = gtk4ListGetModel(ih);
  GtkTreeIter iter;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  gtk_list_store_append(GTK_LIST_STORE(model), &iter);

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, IUPGTK4_LIST_TEXT, iupgtk4StrConvertToSystem(value), -1);
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  GtkTreeModel *model = gtk4ListGetModel(ih);
  GtkTreeIter iter;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  gtk_list_store_insert(GTK_LIST_STORE(model), &iter, pos);

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, IUPGTK4_LIST_TEXT, iupgtk4StrConvertToSystem(value), -1);
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

  iupListUpdateOldValue(ih, pos, 0);
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  GtkTreeModel *model = gtk4ListGetModel(ih);
  GtkTreeIter iter;
  if (gtk_tree_model_iter_nth_child(model, &iter, NULL, pos))
  {
    if (ih->data->is_dropdown && !ih->data->has_editbox)
    {
      int curpos = gtk_combo_box_get_active((GtkComboBox*)ih->handle);
      if (pos == curpos)
      {
        if (curpos > 0)
          curpos--;
        else
        {
          curpos=1;
          if (iupdrvListGetCount(ih)==1)
            curpos = -1;
        }

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        gtk_combo_box_set_active((GtkComboBox*)ih->handle, curpos);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      }
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

    iupListUpdateOldValue(ih, pos, 1);
  }
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  GtkTreeModel *model = gtk4ListGetModel(ih);
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  gtk_list_store_clear(GTK_LIST_STORE(model));
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

static int gtk4ListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    if (ih->data->is_dropdown)
    {
      GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
      if (renderer)
        iupgtk4UpdateObjectFont(ih, G_OBJECT(renderer));
    }

    if (ih->data->has_editbox)
    {
      GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
      if (entry)
        iupgtk4UpdateWidgetFont(ih, (GtkWidget*)entry);
    }
  }
  return 1;
}

static char* gtk4ListGetIdValueAttrib(Ihandle* ih, int id)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos >= 0)
  {
    GtkTreeIter iter;
    GtkTreeModel* model = gtk4ListGetModel(ih);
    if (gtk_tree_model_iter_nth_child(model, &iter, NULL, pos))
    {
      gchar *text = NULL;
      gtk_tree_model_get(model, &iter, IUPGTK4_LIST_TEXT, &text, -1);
      if (text)
      {
        char* ret_str = iupStrReturnStr(iupgtk4StrConvertFromSystem(text));
        g_free(text);
        return ret_str;
      }
    }
    else
    {
    }
  }
  return NULL;
}

static int gtk4ListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  GtkScrolledWindow* scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (scrolled_window && !ih->data->is_dropdown && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
  {
    char* parent_value = iupBaseNativeParentGetBgColor(ih);

    if (iupStrToRGB(parent_value, &r, &g, &b))
    {
      GtkWidget* sb;

      if (!GTK_IS_SCROLLED_WINDOW(scrolled_window))
        scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUPGTK4_SCROLLED_WINDOW");

      iupgtk4SetBgColor((GtkWidget*)scrolled_window, r, g, b);

      sb = gtk_scrolled_window_get_hscrollbar(scrolled_window);
      if (sb) iupgtk4SetBgColor(sb, r, g, b);

      sb = gtk_scrolled_window_get_vscrollbar(scrolled_window);
      if (sb) iupgtk4SetBgColor(sb, r, g, b);
    }
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->data->has_editbox)
  {
    GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    iupgtk4SetBgColor(entry, r, g, b);
  }

  if (!ih->data->is_dropdown)
  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
    if (renderer)
    {
      GdkRGBA rgba;
      rgba.red = r / 255.0;
      rgba.green = g / 255.0;
      rgba.blue = b / 255.0;
      rgba.alpha = 1.0;
      g_object_set(G_OBJECT(renderer), "cell-background-rgba", &rgba, NULL);
    }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int gtk4ListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor(ih->handle, r, g, b);

  if (ih->data->has_editbox)
  {
    GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    iupgtk4SetFgColor(entry, r, g, b);
  }

  if (!ih->data->is_dropdown)
  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
    if (renderer)
    {
      GdkRGBA rgba;
      rgba.red = r / 255.0;
      rgba.green = g / 255.0;
      rgba.blue = b / 255.0;
      rgba.alpha = 1.0;
      g_object_set(G_OBJECT(renderer), "foreground-rgba", &rgba, NULL);
    }
  }

  return 1;
}

static char* gtk4ListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    return iupStrReturnStr(iupgtk4StrConvertFromSystem(gtk_editable_get_text(GTK_EDITABLE(entry))));
  }
  else
  {
    if (ih->data->is_dropdown)
    {
      int pos = gtk_combo_box_get_active((GtkComboBox*)ih->handle);
      return iupStrReturnInt(pos+1);
    }
    else
    {
      GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
      if (!ih->data->is_multiple)
      {
        GtkTreeIter iter;
        GtkTreeModel* tree_model;
        if (gtk_tree_selection_get_selected(selection, &tree_model, &iter))
        {
          GtkTreePath *path = gtk_tree_model_get_path(tree_model, &iter);
          int* indices = gtk_tree_path_get_indices(path);
          int ret = indices[0]+1;
          gtk_tree_path_free (path);
          return iupStrReturnInt(ret);
        }
      }
      else
      {
        GList *il, *list = gtk_tree_selection_get_selected_rows(selection, NULL);
        int count = iupdrvListGetCount(ih);
        char* str = iupStrGetMemory(count+1);
        memset(str, '-', count);
        str[count]=0;
        for (il=list; il; il=il->next)
        {
          GtkTreePath* path = (GtkTreePath*)il->data;
          int* indices = gtk_tree_path_get_indices(path);
          str[indices[0]] = '+';
          gtk_tree_path_free(path);
        }
        g_list_free(list);
        return str;
      }
    }
  }

  return NULL;
}

static int gtk4ListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    if (!value) value = "";
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
    gtk_editable_set_text(GTK_EDITABLE(entry), iupgtk4StrConvertToSystem(value));
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
  }
  else
  {
    if (ih->data->is_dropdown)
    {
      int pos;
      GtkTreeModel *model = gtk4ListGetModel(ih);
      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
      if (iupStrToInt(value, &pos) == 1 &&
          (pos>0 && pos<=gtk_tree_model_iter_n_children(model, NULL)))
      {
        gtk_combo_box_set_active((GtkComboBox*)ih->handle, pos-1);
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
      }
      else
      {
        gtk_combo_box_set_active((GtkComboBox*)ih->handle, -1);
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
      }
      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
    }
    else
    {
      GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
      if (!ih->data->is_multiple)
      {
        int pos;
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        if (iupStrToInt(value, &pos) == 1)
        {
          GtkTreePath* path = gtk_tree_path_new_from_indices(pos-1, -1);
          gtk_tree_selection_select_path(selection, path);
          gtk_tree_path_free(path);
          iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
        }
        else
        {
          gtk_tree_selection_unselect_all(selection);
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        }
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      }
      else
      {
        int i, len, count;

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

        gtk_tree_selection_unselect_all(selection);

        if (!value)
        {
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
          iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
          return 0;
        }

        len = (int)strlen(value);
        count = iupdrvListGetCount(ih);
        if (len < count)
          count = len;

        for (i = 0; i<count; i++)
        {
          if (value[i]=='+')
          {
            GtkTreePath* path = gtk_tree_path_new_from_indices(i, -1);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
          }
        }
        iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", value);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      }
    }
  }

  return 0;
}

static int gtk4ListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
  {
    if (iupStrBoolean(value))
      gtk_combo_box_popup((GtkComboBox*)ih->handle);
    else
      gtk_combo_box_popdown((GtkComboBox*)ih->handle);
  }
  return 0;
}

static int gtk4ListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos = 1;
    if (iupStrToInt(value, &pos))
    {
      GtkTreePath* path = gtk_tree_path_new_from_indices(pos-1, -1);
      gtk_tree_view_scroll_to_cell((GtkTreeView*)ih->handle, path, NULL, TRUE, 0, 0);
      gtk_tree_path_free(path);
    }
  }
  return 0;
}

static int gtk4ListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
    return 0;

  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;

  if (ih->handle)
  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
    if (renderer)
      g_object_set(G_OBJECT(renderer), "xpad", ih->data->spacing,
                                       "ypad", ih->data->spacing,
                                       NULL);
    return 0;
  }
  else
    return 1;
}

static int gtk4ListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    iupgtk4SetMargin(GTK_WIDGET(entry), ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  else
    return 1;
}

static int gtk4ListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start=1, end=1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2)
    return 0;

  if (start<1 || end<1)
    return 0;

  start--;
  end--;

  gtk_editable_select_region(GTK_EDITABLE(entry), start, end);

  return 0;
}

static char* gtk4ListGetSelectionAttrib(Ihandle* ih)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    start++;
    end++;
    return iupStrReturnIntInt(start, end, ':');
  }

  return NULL;
}

static char* gtk4ListGetSelectionPosAttrib(Ihandle* ih)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
    return iupStrReturnIntInt(start, end, ':');

  return NULL;
}

static int gtk4ListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2)
    return 0;

  if (start<0 || end<0)
    return 0;

  gtk_editable_select_region(GTK_EDITABLE(entry), start, end);

  return 0;
}

static char* gtk4ListGetSelectedTextAttrib(Ihandle* ih)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    const char* full_text = gtk_editable_get_text(GTK_EDITABLE(entry));
    char* selectedtext = iupStrDup(full_text);
    selectedtext[end] = 0;
    char* str = iupStrReturnStr(iupgtk4StrConvertFromSystem(selectedtext + start));
    free(selectedtext);
    return str;
  }

  return NULL;
}

static int gtk4ListSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  pos--;
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static char* gtk4ListGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    int pos = gtk_editable_get_position(GTK_EDITABLE(entry));
    pos++;
    return iupStrReturnInt((int)pos);
  }
  else
    return NULL;
}

static int gtk4ListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static char* gtk4ListGetCaretPosAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    int pos = gtk_editable_get_position(GTK_EDITABLE(entry));
    return iupStrReturnInt((int)pos);
  }
  else
    return NULL;
}

static int gtk4ListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 1) pos = 1;
  pos--;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static int gtk4ListSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static int gtk4ListSetInsertAttrib(Ihandle* ih, const char* value)
{
  int pos;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  pos = gtk_editable_get_position(GTK_EDITABLE(entry));
  gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtk4StrConvertToSystem(value), -1, &pos);
  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);

  return 0;
}

static int gtk4ListSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    int pos = (int)strlen(gtk_editable_get_text(GTK_EDITABLE(entry)))+1;
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
    gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtk4StrConvertToSystem(value), -1, &pos);
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
  }
  return 0;
}

static int gtk4ListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = INT_MAX;

  if (ih->handle)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    gtk_entry_set_max_length(entry, ih->data->nc);
    return 0;
  }
  else
    return 1;
}

static int gtk4ListSetClipboardAttrib(Ihandle *ih, const char *value)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;

  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (iupStrEqualNoCase(value, "COPY"))
  {
    GdkClipboard* clipboard = gtk_widget_get_clipboard((GtkWidget*)entry);
    gdk_clipboard_set_text(clipboard, gtk_editable_get_text(GTK_EDITABLE(entry)));
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    GdkClipboard* clipboard = gtk_widget_get_clipboard((GtkWidget*)entry);
    gdk_clipboard_set_text(clipboard, gtk_editable_get_text(GTK_EDITABLE(entry)));
    gtk_editable_delete_selection(GTK_EDITABLE(entry));
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    GdkClipboard* clipboard = gtk_widget_get_clipboard((GtkWidget*)entry);
    gdk_clipboard_read_text_async(clipboard, NULL, NULL, NULL);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
    gtk_editable_delete_selection(GTK_EDITABLE(entry));
  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
  return 0;
}

static int gtk4ListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_editable(GTK_EDITABLE(entry), !iupStrBoolean(value));
  return 0;
}

static char* gtk4ListGetReadOnlyAttrib(Ihandle* ih)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  return iupStrReturnBoolean(!gtk_editable_get_editable(GTK_EDITABLE(entry)));
}

static int gtk4ListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeModel* model = gtk4ListGetModel(ih);
  GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(value, ih, 0, NULL);
  GtkTreeIter iter;
  int pos = iupListGetPosAttrib(ih, id);

  if (!gtk_tree_model_iter_nth_child(model, &iter, NULL, pos))
  {
    return 0;
  }

  if (paintable)
  {
    GdkTexture* texture = GDK_TEXTURE(paintable);
    GdkPixbuf* pixbuf = gdk_pixbuf_get_from_texture(texture);

    if (pixbuf)
    {
      int charheight;
      iupdrvFontGetCharSize(ih, NULL, &charheight);
      int available_height = charheight + 2 * ih->data->spacing;
      int img_height = gdk_pixbuf_get_height(pixbuf);
      int img_width = gdk_pixbuf_get_width(pixbuf);

      if (img_height > available_height)
      {
        /* Scale down proportionally to fit available height */
        int scaled_width = (img_width * available_height) / img_height;
        GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, scaled_width, available_height, GDK_INTERP_BILINEAR);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, IUPGTK4_LIST_IMAGE, scaled, -1);
        g_object_unref(scaled);
      }
      else
      {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, IUPGTK4_LIST_IMAGE, pixbuf, -1);
      }

      g_object_unref(pixbuf);
    }
    else
    {
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, IUPGTK4_LIST_IMAGE, NULL, -1);
    }
  }
  else
  {
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, IUPGTK4_LIST_IMAGE, NULL, -1);
  }

  return 0;
}

static void gtk4ListEditMoveCursor(GtkWidget* entry, GtkMovementStep step, gint count, gboolean extend_selection, Ihandle* ih)
{
  int pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = gtk_editable_get_position(GTK_EDITABLE(entry));

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;

    cb(ih, 1, pos+1, pos);
  }

  (void)step;
  (void)count;
  (void)extend_selection;
}

static gboolean gtk4ListEditKeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");

  /* First propagate to parent callbacks (K_ANY, etc.) for the ENTRY widget
   * (not the list), matching GTK3 behavior. This allows parent handlers to
   * process keys, but doesn't interfere with list navigation since entry
   * doesn't have focus navigation for UP/DOWN keys. */
  if (iupgtk4KeyPressEvent(controller, keyval, keycode, state, ih) == TRUE)
  {
    return TRUE;
  }

  if ((keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up) ||
      (keyval == GDK_KEY_Prior || keyval == GDK_KEY_KP_Page_Up) ||
      (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down) ||
      (keyval == GDK_KEY_Next || keyval == GDK_KEY_KP_Page_Down))
  {
    int pos = -1;
    GtkTreeIter iter;
    GtkTreeModel* model = NULL;
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
      int* indices = gtk_tree_path_get_indices(path);
      pos = indices[0];
      gtk_tree_path_free(path);
    }

    if (pos == -1)
      pos = 0;
    else if (keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up)
    {
      pos--;
      if (pos < 0) pos = 0;
    }
    else if (keyval == GDK_KEY_Prior || keyval == GDK_KEY_KP_Page_Up)
    {
      pos -= 5;
      if (pos < 0) pos = 0;
    }
    else if (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down)
    {
      int count = gtk_tree_model_iter_n_children(model, NULL);
      pos++;
      if (pos > count-1) pos = count-1;
    }
    else if (keyval == GDK_KEY_Next || keyval == GDK_KEY_KP_Page_Down)
    {
      int count = gtk_tree_model_iter_n_children(model, NULL);
      pos += 5;
      if (pos > count-1) pos = count-1;
    }

    if (pos != -1)
    {
      GtkTreePath* path = gtk_tree_path_new_from_indices(pos, -1);
      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
      gtk_tree_selection_select_path(selection, path);
      iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      gtk_tree_path_free(path);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos+1);

      if (!model) model = gtk4ListGetModel(ih);

      if (gtk_tree_model_iter_nth_child(model, &iter, NULL, pos))
      {
        gchar *text = NULL;
        gtk_tree_model_get(model, &iter, IUPGTK4_LIST_TEXT, &text, -1);
        if (text)
        {
          gtk_editable_set_text(GTK_EDITABLE(entry), text);
          g_free(text);
        }
      }
    }
  }

  return FALSE;
}

static gboolean gtk4ListEditKeyReleaseEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk4ListEditMoveCursor(entry, 0, 0, 0, ih);
  (void)controller;
  (void)keyval;
  (void)keycode;
  (void)state;
  return FALSE;
}

static void gtk4ListEditButtonPressed(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk4ListEditMoveCursor(entry, 0, 0, 0, ih);
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
}

static void gtk4ListEditDeleteText(GtkEditable *editable, int start, int end, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  int ret;

  if (iupAttribGet(ih, "_IUPGTK4_DISABLE_TEXT_CB"))
    return;

  ret = iupEditCallActionCb(ih, cb, NULL, start, end, ih->data->mask, ih->data->nc, 1, 1);
  if (ret == 0)
    g_signal_stop_emission_by_name(editable, "delete_text");
}

static void gtk4ListEditInsertText(GtkEditable *editable, char *insert_value, int len, int *pos, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  int ret;

  if (iupAttribGet(ih, "_IUPGTK4_DISABLE_TEXT_CB"))
    return;

  ret = iupEditCallActionCb(ih, cb, iupgtk4StrConvertFromSystem(insert_value), *pos, *pos, ih->data->mask, ih->data->nc, 0, 1);
  if (ret == 0)
    g_signal_stop_emission_by_name(editable, "insert_text");
  else if (ret != -1)
  {
    insert_value[0] = (char)ret;

    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
    gtk_editable_insert_text(editable, insert_value, 1, pos);
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);

    g_signal_stop_emission_by_name(editable, "insert_text");
  }

  (void)len;
}

static void gtk4ListEditChanged(void* dummy, Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPGTK4_DISABLE_TEXT_CB"))
    return;

  iupBaseCallValueChangedCb(ih);
  (void)dummy;
}

static void gtk4ListComboBoxPopupShown(GtkComboBox* widget, GParamSpec *pspec, Ihandle* ih)
{
  IFni cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  iupAttribSet(ih, "_IUPDROPDOWN_POPUP", "1");
  if (cb)
  {
    gboolean popup_shown;
    g_object_get(widget, "popup-shown", &popup_shown, NULL);
    cb(ih, popup_shown);
  }
  (void)pspec;
}

static void gtk4ListComboBoxChanged(GtkComboBox* widget, Ihandle* ih)
{
  IFnsii cb;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int pos = gtk_combo_box_get_active((GtkComboBox*)ih->handle);
    pos++;
    iupListSingleCallActionCb(ih, cb, pos);
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);

  (void)widget;
}

static gboolean gtk4ListSimpleKeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  /* First, propagate to parent callbacks (K_ANY, etc.) */
  if (iupgtk4KeyPressEvent(controller, keyval, keycode, state, ih) == TRUE)
    return TRUE;

  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter)
    return TRUE;
  return FALSE;
}

static void gtk4ListRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, Ihandle* ih)
{
  IFnis cb = (IFnis) IupGetCallback(ih, "DBLCLICK_CB");
  if (cb)
  {
    int* indices = gtk_tree_path_get_indices(path);
    iupListSingleCallDblClickCb(ih, cb, indices[0]+1);
  }
  (void)column;
  (void)tree_view;
}

static void gtk4ListSelectionChanged(GtkTreeSelection* selection, Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkTreeIter iter;
    GtkTreeModel* tree_model;
    if (gtk_tree_selection_get_selected(selection, &tree_model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path(tree_model, &iter);
      char* value = NULL;

      gtk_tree_model_get(tree_model, &iter, IUPGTK4_LIST_TEXT, &value, -1);
      if (value)
      {
        GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
        gtk_editable_set_text(GTK_EDITABLE(entry), value);
        g_free(value);
      }

      gtk_tree_path_free(path);
    }
  }

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  if (!ih->data->is_multiple)
  {
    IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      GtkTreeIter iter;
      GtkTreeModel* tree_model;
      if (gtk_tree_selection_get_selected(selection, &tree_model, &iter))
      {
        GtkTreePath *path = gtk_tree_model_get_path(tree_model, &iter);
        int* indices = gtk_tree_path_get_indices(path);
        iupListSingleCallActionCb(ih, cb, indices[0]+1);
        gtk_tree_path_free (path);
      }
    }
  }
  else
  {
    IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
    IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
    if (multi_cb || cb)
    {
      GList *il, *list = gtk_tree_selection_get_selected_rows(selection, NULL);
      int i, sel_count = g_list_length(list);
      int* pos = malloc(sizeof(int)*sel_count);
      for (il=list, i=0; il; il=il->next, i++)
      {
        GtkTreePath* path = (GtkTreePath*)il->data;
        int* indices = gtk_tree_path_get_indices(path);
        pos[i] = indices[0];
        gtk_tree_path_free(path);
      }
      g_list_free(list);

      iupListMultipleCallActionCb(ih, cb, multi_cb, pos, sel_count);
      free(pos);
    }
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);
}

/* Cell data function to convert GdkTexture to GdkPixbuf */
static void gtk4ListImageCellDataFunc(GtkCellLayout *cell_layout,
                                       GtkCellRenderer *renderer,
                                       GtkTreeModel *model,
                                       GtkTreeIter *iter,
                                       gpointer data)
{
  Ihandle* ih = (Ihandle*)data;
  GdkTexture *texture = NULL;

  /* Check show_image flag - if not enabled, hide the image */
  if (!ih || !ih->data->show_image)
  {
    g_object_set(renderer, "pixbuf", NULL, NULL);
    return;
  }

  gtk_tree_model_get(model, iter, IUPGTK4_LIST_IMAGE, &texture, -1);

  if (texture)
  {
    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_texture(texture);
    g_object_set(renderer, "pixbuf", pixbuf, NULL);
    if (pixbuf)
      g_object_unref(pixbuf);
    g_object_unref(texture);
  }
  else
  {
    g_object_set(renderer, "pixbuf", NULL, NULL);
  }

  (void)cell_layout;
}

static int gtk4ListMapMethod(Ihandle* ih)
{
  GtkScrolledWindow* scrolled_window = NULL;
  GtkListStore *store;
  GtkCellRenderer *renderer = NULL, *renderer_img = NULL;

  store = gtk_list_store_new(IUPGTK4_LIST_LAST_DATA, GDK_TYPE_PIXBUF, G_TYPE_STRING);

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      ih->handle = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
      gtk_combo_box_set_entry_text_column ((GtkComboBox*)ih->handle, IUPGTK4_LIST_TEXT);
    }
    else
      ih->handle = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

    g_object_unref(store);

    if (!ih->handle)
      return IUP_ERROR;

    renderer_img = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ih->handle), renderer_img, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ih->handle), renderer_img, "pixbuf", IUPGTK4_LIST_IMAGE, NULL);
    iupAttribSet(ih, "_IUPGTK4_RENDERER_IMG", (char*)renderer_img);

    if (ih->data->has_editbox)
    {
      GtkWidget *entry;
      entry = gtk_combo_box_get_child(GTK_COMBO_BOX(ih->handle));
      iupAttribSet(ih, "_IUPGTK4_ENTRY", (char*)entry);

      iupgtk4SetupEnterLeaveEvents(entry, ih);
      iupgtk4SetupFocusEvents(entry, ih);
      iupgtk4SetupKeyEvents(entry, ih);

      GtkEventController* key_controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(entry, key_controller);
      g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ListEditKeyPressEvent), ih);
      g_signal_connect(key_controller, "key-released", G_CALLBACK(gtk4ListEditKeyReleaseEvent), ih);

      GtkGesture* click_gesture = gtk_gesture_click_new();
      gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(click_gesture));
      g_signal_connect(click_gesture, "pressed", G_CALLBACK(gtk4ListEditButtonPressed), ih);

      g_signal_connect(G_OBJECT(entry), "delete-text", G_CALLBACK(gtk4ListEditDeleteText), ih);
      g_signal_connect(G_OBJECT(entry), "insert-text", G_CALLBACK(gtk4ListEditInsertText), ih);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupgtk4SetCanFocus(ih->handle, 0);

      /* Always reorder image renderer to be first */
      gtk_cell_layout_reorder(GTK_CELL_LAYOUT(ih->handle), renderer_img, IUPGTK4_LIST_IMAGE);
    }
    else
    {
      renderer = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ih->handle), renderer, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ih->handle), renderer, "text", IUPGTK4_LIST_TEXT, NULL);

      iupgtk4SetupFocusEvents(ih->handle, ih);
      iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
      iupgtk4SetupKeyEvents(ih->handle, ih);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupgtk4SetCanFocus(ih->handle, 0);
    }

    g_signal_connect(ih->handle, "changed", G_CALLBACK(gtk4ListComboBoxChanged), ih);
    g_signal_connect(ih->handle, "notify::popup-shown", G_CALLBACK(gtk4ListComboBoxPopupShown), ih);

    if (renderer)
    {
      g_object_set(G_OBJECT(renderer), "xpad", 0, "ypad", 0, NULL);
      iupAttribSet(ih, "_IUPGTK4_RENDERER", (char*)renderer);
    }
  }
  else
  {
    GtkTreeSelection* selection;
    GtkTreeViewColumn *column;
    GtkPolicyType scrollbar_policy;

    ih->handle = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    if (!ih->handle)
      return IUP_ERROR;

    scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new();

    if (ih->data->has_editbox)
    {
      GtkBox* vbox = (GtkBox*)gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

      /* Set homogeneous=FALSE to allow different sizes for children */
      gtk_box_set_homogeneous(vbox, FALSE);

      /* VBox should not expand - let IUP control exact size */
      gtk_widget_set_vexpand((GtkWidget*)vbox, FALSE);
      gtk_widget_set_hexpand((GtkWidget*)vbox, FALSE);

      GtkWidget *entry = gtk_entry_new();

      gtk_widget_set_vexpand(entry, FALSE);
      gtk_widget_set_valign(entry, GTK_ALIGN_CENTER);

      /* Append entry to VBox - will use natural size */
      gtk_box_append(vbox, entry);
      iupAttribSet(ih, "_IUPGTK4_ENTRY", (char*)entry);

      /* Scrolled window should expand to fill remaining space */
      gtk_widget_set_vexpand((GtkWidget*)scrolled_window, TRUE);

      gtk_box_append(vbox, (GtkWidget*)scrolled_window);
      iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)vbox);
      iupAttribSet(ih, "_IUPGTK4_SCROLLED_WINDOW", (char*)scrolled_window);

      iupgtk4SetCanFocus(ih->handle, 0);
      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupgtk4SetCanFocus(entry, 0);

      iupgtk4SetupEnterLeaveEvents(entry, ih);
      iupgtk4SetupFocusEvents(entry, ih);
      iupgtk4SetupKeyEvents(entry, ih);

      GtkEventController* key_controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(entry, key_controller);
      g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ListEditKeyPressEvent), ih);
      g_signal_connect(key_controller, "key-released", G_CALLBACK(gtk4ListEditKeyReleaseEvent), ih);

      GtkGesture* click_gesture = gtk_gesture_click_new();
      gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(click_gesture));
      g_signal_connect(click_gesture, "pressed", G_CALLBACK(gtk4ListEditButtonPressed), ih);

      g_signal_connect(G_OBJECT(entry), "delete-text", G_CALLBACK(gtk4ListEditDeleteText), ih);
      g_signal_connect(G_OBJECT(entry), "insert-text", G_CALLBACK(gtk4ListEditInsertText), ih);
      g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(gtk4ListEditChanged), ih);
    }
    else
    {
      iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupgtk4SetCanFocus(ih->handle, 0);

      iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
      iupgtk4SetupFocusEvents(ih->handle, ih);

      GtkEventController* key_controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(ih->handle, key_controller);
      g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ListSimpleKeyPressEvent), ih);
    }

    column = gtk_tree_view_column_new();

    renderer_img = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer_img, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer_img, "pixbuf", IUPGTK4_LIST_IMAGE, NULL);
    iupAttribSet(ih, "_IUPGTK4_RENDERER_IMG", (char*)renderer_img);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer, "text", IUPGTK4_LIST_TEXT, NULL);
    iupAttribSet(ih, "_IUPGTK4_RENDERER", (char*)renderer);
    g_object_set(G_OBJECT(renderer), "xpad", 0, "ypad", 0, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(ih->handle), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ih->handle), FALSE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ih->handle), FALSE);

    gtk_scrolled_window_set_child(scrolled_window, ih->handle);
    gtk_scrolled_window_set_has_frame(scrolled_window, TRUE);

    if (ih->data->sb)
    {
      if (iupAttribGetBoolean(ih, "AUTOHIDE"))
        scrollbar_policy = GTK_POLICY_AUTOMATIC;
      else
        scrollbar_policy = GTK_POLICY_ALWAYS;
    }
    else
      scrollbar_policy = GTK_POLICY_NEVER;

    gtk_scrolled_window_set_policy(scrolled_window, scrollbar_policy, scrollbar_policy);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
    if (!ih->data->has_editbox && ih->data->is_multiple)
    {
      gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
      gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(ih->handle), TRUE);
    }
    else
      gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);

    g_signal_connect(G_OBJECT(selection), "changed",  G_CALLBACK(gtk4ListSelectionChanged), ih);
    g_signal_connect(G_OBJECT(ih->handle), "row-activated", G_CALLBACK(gtk4ListRowActivated), ih);

    iupgtk4SetupMotionEvents(ih->handle, ih);
    iupgtk4SetupButtonEvents(ih->handle, ih);
  }

  if (iupAttribGetBoolean(ih, "SORT"))
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), IUPGTK4_LIST_TEXT, GTK_SORT_ASCENDING);

  iupgtk4AddToParent(ih);

  if (scrolled_window)
    gtk_widget_realize((GtkWidget*)scrolled_window);
  gtk_widget_realize(ih->handle);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)gtk4ListConvertXYToPos);

  iupListSetInitialItems(ih);

  iupgtk4UpdateMnemonic(ih);

  return IUP_NOERROR;
}

void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  GdkTexture* texture;
  GtkTreeModel* model = gtk4ListGetModel(ih);
  GtkTreeIter iter;

  if (!gtk_tree_model_iter_nth_child(model, &iter, NULL, id-1))
    return NULL;

  gtk_tree_model_get(model, &iter, IUPGTK4_LIST_IMAGE, &texture, -1);

  return texture;
}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  GtkTreeModel* model = gtk4ListGetModel(ih);
  GtkTreeIter iter;

  gtk_tree_model_iter_nth_child(model, &iter, NULL, id);
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, IUPGTK4_LIST_IMAGE, (GdkTexture*)hImage, -1);

  return 0;
}

void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = gtk4ListMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, gtk4ListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtk4ListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtk4ListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VALUE", gtk4ListGetValueAttrib, gtk4ListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IDVALUE", gtk4ListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, gtk4ListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, gtk4ListSetTopItemAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, gtk4ListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, gtk4ListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", gtk4ListGetSelectedTextAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", gtk4ListGetSelectionAttrib, gtk4ListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", gtk4ListGetSelectionPosAttrib, gtk4ListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", gtk4ListGetCaretAttrib, gtk4ListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", gtk4ListGetCaretPosAttrib, gtk4ListSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, gtk4ListSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, gtk4ListSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", gtk4ListGetReadOnlyAttrib, gtk4ListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, gtk4ListSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, gtk4ListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, gtk4ListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, gtk4ListSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, gtk4ListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, NULL, IUPAF_SAMEASSYSTEM, "3", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROPLIST", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED);
}

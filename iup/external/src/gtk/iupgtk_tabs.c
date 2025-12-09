/** \file
* \brief Tabs Control
*
* See Copyright Notice in "iup.h"
*/

#include <gtk/gtk.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_image.h"
#include "iup_tabs.h"

#include "iupgtk_drv.h"


int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvTabsExtraMargin(void)
{
  return 0;
}

int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  iupAttribSet(ih, "_IUPGTK_IGNORE_CHANGE", "1");
  gtk_notebook_set_current_page((GtkNotebook*)ih->handle, pos);
  iupAttribSet(ih, "_IUPGTK_IGNORE_CHANGE", NULL);
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  return gtk_notebook_get_current_page((GtkNotebook*)ih->handle);
}

static void gtkTabsUpdatePageFont(Ihandle* ih)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK_TABLABEL");
    if (tab_label)
      iupgtkUpdateWidgetFont(ih, tab_label);
  }
}

static void gtkTabsUpdatePageBgColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_container = (GtkWidget*)iupAttribGet(child, "_IUPTAB_CONTAINER");
    if (tab_container)
    {
      iupgtkSetBgColor(tab_container, r, g, b);
    }
  }
}

static void gtkTabsUpdatePageFgColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK_TABLABEL");
    if (tab_label)
      iupgtkSetFgColor(tab_label, r, g, b);
  }
}

static void gtkTabsUpdatePagePadding(Ihandle* ih)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK_TABLABEL");
    if (tab_label)
    {
#if GTK_CHECK_VERSION(3, 4, 0)
      iupgtkSetMargin(tab_label, ih->data->horiz_padding, ih->data->vert_padding, 1);
#else
      gtk_misc_set_padding((GtkMisc*)tab_label, ih->data->horiz_padding, ih->data->vert_padding);
#endif
    }
  }
}


/* ------------------------------------------------------------------------- */
/* gtkTabs - Sets and Gets accessors                                         */
/* ------------------------------------------------------------------------- */


static int gtkTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    gtkTabsUpdatePagePadding(ih);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static void gtkTabsUpdateTabType(Ihandle* ih)
{
  int iup2gtk[4] = {GTK_POS_TOP, GTK_POS_BOTTOM, GTK_POS_LEFT, GTK_POS_RIGHT};
  gtk_notebook_set_tab_pos((GtkNotebook*)ih->handle, iup2gtk[ih->data->type]);
}

static int gtkTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  if(iupStrEqualNoCase(value, "BOTTOM"))
    ih->data->type = ITABS_BOTTOM;
  else if(iupStrEqualNoCase(value, "LEFT"))
    ih->data->type = ITABS_LEFT;
  else if(iupStrEqualNoCase(value, "RIGHT"))
    ih->data->type = ITABS_RIGHT;
  else /* "TOP" */
    ih->data->type = ITABS_TOP;

  if (ih->handle)
    gtkTabsUpdateTabType(ih);  /* for this to work must be updated in map */

  return 0;
}

static int gtkTabsSetTabOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* allow to set only before mapping */
    return 0;

  if(iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->orientation = ITABS_VERTICAL;
  else  /* HORIZONTAL */
    ih->data->orientation = ITABS_HORIZONTAL;

  return 0;
}

static int gtkTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABTITLE", value);

    if (value)
    {
      GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK_TABLABEL");
      if (tab_label)
      {
        GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
        iupgtkSetMnemonicTitle(ih, (GtkLabel*)tab_label, value);
        gtk_notebook_set_menu_label_text((GtkNotebook*)ih->handle, tab_page, gtk_label_get_text((GtkLabel*)tab_label));
      }
    }
  }

  return 0;
}

static int gtkTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    GtkWidget* tab_image;

    iupAttribSetStr(child, "TABIMAGE", value);

    tab_image = (GtkWidget*)iupAttribGet(child, "_IUPGTK_TABIMAGE");
    if (tab_image)
    {
      if (value)
      {
        GdkPixbuf* pixbuf = iupImageGetImage(value, ih, 0, NULL);
        if (pixbuf)
          gtk_image_set_from_pixbuf((GtkImage*)tab_image, pixbuf);
      }
      else
        gtk_image_set_from_pixbuf((GtkImage*)tab_image, NULL);
    }
  }
  return 1;
}

static int gtkTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
    if (iupStrBoolean(value))
      gtk_widget_show(tab_page);
    else
      gtk_widget_hide(tab_page);
  }
  return 0;
}

int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
  (void)pos;
  return iupgtkIsVisible(tab_page);
}

static int gtkTabsSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;
  if (ih->handle)
    gtkTabsUpdatePageFont(ih);
  return 1;
}

static int gtkTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetFgColor(ih->handle, r, g, b);
  gtkTabsUpdatePageFgColor(ih, r, g, b);

  return 1;
}

static int gtkTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetBgColor(ih->handle, r, g, b);
  gtkTabsUpdatePageBgColor(ih, r, g, b);

  return 1;
}

static int gtkTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    Ihandle* child;
    gboolean reorderable = iupStrBoolean(value);

    for (child = ih->firstchild; child; child = child->brother)
    {
      GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
      if (tab_page)
        gtk_notebook_set_tab_reorderable((GtkNotebook*)ih->handle, tab_page, reorderable);
    }
  }
  return 1;
}


/* ------------------------------------------------------------------------- */
/* gtkTabs - Callbacks                                                       */
/* ------------------------------------------------------------------------- */

static void gtkTabsSwitchPage(GtkNotebook* notebook, void* page, int pos, Ihandle* ih)
{
  IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  int prev_pos = iupdrvTabsGetCurrentTab(ih);

  Ihandle* child = IupGetChild(ih, pos);
  Ihandle* prev_child = IupGetChild(ih, prev_pos);

  GtkWidget* tab_container = (GtkWidget*)iupAttribGet(child, "_IUPTAB_CONTAINER");
  GtkWidget* prev_tab_container = (GtkWidget*)iupAttribGet(prev_child, "_IUPTAB_CONTAINER");

  if (iupAttribGet(ih, "_IUPGTK_IGNORE_SWITCHPAGE"))
    return;

  if (tab_container) gtk_widget_show(tab_container);   /* show new page, if any */
  if (prev_tab_container) gtk_widget_hide(prev_tab_container);  /* hide previous page, if any */

  if (iupAttribGet(ih, "_IUPGTK_IGNORE_CHANGE"))
    return;

  if (cb)
    cb(ih, child, prev_child);
  else
  {
    IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
    if (cb2)
      cb2(ih, pos, prev_pos);
  }

  (void)notebook;
  (void)page;
}

static gboolean gtkTabsButtonPressEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *child)
{
  Ihandle* ih = IupGetParent(child);
  IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
  
  if (evt->type == GDK_BUTTON_PRESS && evt->button == 3 && cb)
  {
    GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
    int pos = gtk_notebook_page_num((GtkNotebook*)ih->handle, tab_page);
    cb(ih, pos);
  }

  (void)widget;

  return iupgtkButtonEvent(widget, evt, ih);
}

static void gtkTabsCloseButtonClicked(GtkButton *widget, Ihandle* child)
{
  /* Close tab child */
  GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
  if (tab_page)
  {
    Ihandle* ih = IupGetParent(child);
    int pos = gtk_notebook_page_num((GtkNotebook*)ih->handle, tab_page);
    int ret = IUP_DEFAULT;
    IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
    if (cb)
      ret = cb(ih, pos);

    if (ret == IUP_CONTINUE) /* destroy tab and children */
    {
      IupDestroy(child);
      IupRefreshChildren(ih);
    }
    else if (ret == IUP_DEFAULT) /* hide tab and children */
      gtk_widget_hide(tab_page);
  }

  (void)widget;
}

/* ------------------------------------------------------------------------- */
/* gtkTabs - Methods and Init Class                                          */
/* ------------------------------------------------------------------------- */
static void gtkTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  /* make sure it has at least one name */
  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (ih->handle)
  {
    GtkWidget *evtBox, *tab_page, *tab_container, *box = NULL;
    GtkWidget *tab_label = NULL, *tab_image = NULL, *tab_close = NULL;
    char *tabtitle, *tabimage;
    int pos;
    unsigned char r, g, b;

    pos = IupGetChildPos(ih, child);

    /* Can not hide the tab_page,
       or the tab will be automatically hidden.
       So create a secondary container to hide its child instead. */
#if GTK_CHECK_VERSION(3, 0, 0)
    tab_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    tab_page = gtk_vbox_new(FALSE, 0);
#endif
    gtk_widget_show(tab_page);

    tab_container = gtk_fixed_new(); /* can not use iupgtkNativeContainerNew here in GTK3 */
    gtk_widget_show(tab_container);

    gtk_container_add((GtkContainer*)tab_page, tab_container);

    tabtitle = iupAttribGet(child, "TABTITLE");
    if (!tabtitle)
    {
      tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
      if (tabtitle)
        iupAttribSetStr(child, "TABTITLE", tabtitle);
    }
    tabimage = iupAttribGet(child, "TABIMAGE");
    if (!tabimage)
    {
      tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
      if (tabimage)
        iupAttribSetStr(child, "TABIMAGE", tabimage);
    }
    if (!tabtitle && !tabimage)
      tabtitle = "     ";

    if (tabtitle)
    {
      tab_label = gtk_label_new(NULL);
      iupgtkSetMnemonicTitle(ih, (GtkLabel*)tab_label, tabtitle);

#if GTK_CHECK_VERSION(2, 6, 0)
      if (ih->data->orientation == ITABS_VERTICAL)
        gtk_label_set_angle((GtkLabel*)tab_label, 90);
#endif
    }

    if (tabimage)
    {
      GdkPixbuf* pixbuf = iupImageGetImage(tabimage, ih, 0, NULL);

      tab_image = gtk_image_new();

      if (pixbuf)
        gtk_image_set_from_pixbuf((GtkImage*)tab_image, pixbuf);
    }

    if(ih->data->show_close)
    {
#if GTK_CHECK_VERSION(3, 10, 0)
      GtkWidget* image = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
#else
      GtkWidget* image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
#endif

      tab_close = gtk_button_new();
      gtk_button_set_image((GtkButton*)tab_close, image);
      gtk_button_set_relief((GtkButton*)tab_close, GTK_RELIEF_NONE);
      gtk_button_set_focus_on_click((GtkButton*)tab_close, FALSE);
      iupgtkSetCanFocus(tab_close, FALSE);

      g_signal_connect(G_OBJECT(tab_close), "clicked", G_CALLBACK(gtkTabsCloseButtonClicked), child);
    }

    iupAttribSet(ih, "_IUPGTK_IGNORE_CHANGE", "1");

    if ((tabimage && tabtitle) || ih->data->show_close)
    {
#if GTK_CHECK_VERSION(3, 0, 0)
      if (ih->data->orientation == ITABS_VERTICAL)
        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
      else
        box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
      if (ih->data->orientation == ITABS_VERTICAL)
        box = gtk_vbox_new(FALSE, 2);
      else
        box = gtk_hbox_new(FALSE, 2);
#endif
      gtk_widget_show(box);
    }

    /* RIGHTCLICK_CB will not work without the eventbox */
    evtBox = gtk_event_box_new();
#if GTK_CHECK_VERSION(2, 2, 0) && !GTK_CHECK_VERSION(3, 0, 0)
    gtk_event_box_set_visible_window((GtkEventBox*)evtBox, FALSE);
#endif
    gtk_widget_add_events(evtBox, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(evtBox), "button-press-event", G_CALLBACK(gtkTabsButtonPressEvent), child);

    if (tabimage && tabtitle)
    {
      gtk_container_add((GtkContainer*)box, tab_image);
      gtk_container_add((GtkContainer*)box, tab_label);

      if(ih->data->show_close)
        gtk_container_add((GtkContainer*)box, tab_close);

      gtk_container_add((GtkContainer*)evtBox, box);
      gtk_notebook_insert_page((GtkNotebook*)ih->handle, tab_page, evtBox, pos);
      gtk_notebook_set_menu_label_text((GtkNotebook*)ih->handle, tab_page, gtk_label_get_text((GtkLabel*)tab_label));
    }
    else if(tabimage && ih->data->show_close)
    {
      gtk_container_add((GtkContainer*)box, tab_image);
      gtk_container_add((GtkContainer*)box, tab_close);
      gtk_container_add((GtkContainer*)evtBox, box);
      gtk_notebook_insert_page((GtkNotebook*)ih->handle, tab_page, evtBox, pos);
    }
    else if(tabtitle && ih->data->show_close)
    {
      gtk_container_add((GtkContainer*)box, tab_label);
      gtk_container_add((GtkContainer*)box, tab_close);
      gtk_container_add((GtkContainer*)evtBox, box);
      gtk_notebook_insert_page((GtkNotebook*)ih->handle, tab_page, evtBox, pos);
      gtk_notebook_set_menu_label_text((GtkNotebook*)ih->handle, tab_page, gtk_label_get_text((GtkLabel*)tab_label));
    }
    else if (tabimage)
    {
      gtk_container_add((GtkContainer*)evtBox, tab_image);
      gtk_notebook_insert_page((GtkNotebook*)ih->handle, tab_page, evtBox, pos);
    }
    else
    {
      gtk_container_add((GtkContainer*)evtBox, tab_label);
      gtk_notebook_insert_page((GtkNotebook*)ih->handle, tab_page, evtBox, pos);
    }

    gtk_widget_realize(tab_page);

    iupAttribSet(child, "_IUPGTK_TABCLOSE", (char*)tab_close);
    iupAttribSet(child, "_IUPGTK_TABIMAGE", (char*)tab_image);  /* store it even if its NULL */
    iupAttribSet(child, "_IUPGTK_TABLABEL", (char*)tab_label);
    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)tab_container);
    iupAttribSet(child, "_IUPTAB_PAGE", (char*)tab_page);

    /* check if tab reordering is allowed */
    char* allow_reorder = iupAttribGet(ih, "ALLOWREORDER");
    if (iupStrBoolean(allow_reorder))
      gtk_notebook_set_tab_reorderable((GtkNotebook*)ih->handle, tab_page, TRUE);

    iupStrToRGB(IupGetAttribute(ih, "BGCOLOR"), &r, &g, &b);
    iupgtkSetBgColor(tab_container, r, g, b);

    if (tabtitle)
    {
      iupgtkUpdateWidgetFont(ih, tab_label);

      iupStrToRGB(IupGetAttribute(ih, "FGCOLOR"), &r, &g, &b);
      iupgtkSetFgColor(tab_label, r, g, b);

      gtk_widget_show(tab_label);
      gtk_widget_realize(tab_label);
    }

    if (tabimage)
    {
      gtk_widget_show(tab_image);
      gtk_widget_realize(tab_image);
    }

    if (ih->data->show_close)
    {
      gtk_widget_show(tab_close);
      gtk_widget_realize(tab_close);
    }

    iupAttribSet(ih, "_IUPGTK_IGNORE_CHANGE", NULL);

    if (pos != iupdrvTabsGetCurrentTab(ih))
      gtk_widget_hide(tab_container);
  }
}

static void gtkTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");

    if (tab_page && GTK_IS_WIDGET(tab_page))
    {
      int page_num = gtk_notebook_page_num((GtkNotebook*)ih->handle, tab_page);

      if (page_num != -1)
      {
        if (iupdrvTabsGetCurrentTab(ih) == page_num)
          iupAttribSet(ih, "_IUPGTK_IGNORE_SWITCHPAGE", "1");

        iupTabsCheckCurrentTab(ih, pos, 1);

        iupAttribSet(ih, "_IUPGTK_IGNORE_CHANGE", "1");
        gtk_notebook_remove_page((GtkNotebook*)ih->handle, page_num);
        iupAttribSet(ih, "_IUPGTK_IGNORE_CHANGE", NULL);
      }
    }
  }

  child->handle = NULL;

  iupAttribSet(child, "_IUPGTK_TABCLOSE", NULL);
  iupAttribSet(child, "_IUPGTK_TABIMAGE", NULL);
  iupAttribSet(child, "_IUPGTK_TABLABEL", NULL);
  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
  iupAttribSet(child, "_IUPTAB_PAGE", NULL);

  iupAttribSet(ih, "_IUPGTK_IGNORE_SWITCHPAGE", NULL);
}

static int gtkTabsGetTabOverlap(Ihandle* ih)
{
  if (!ih->handle)
    return 0;

  GtkNotebook* notebook = (GtkNotebook*)ih->handle;
  gint tab_overlap = 0;

  /* Query GTK style property for tab overlap (negative values mean spacing between tabs) */
  gtk_widget_style_get(GTK_WIDGET(notebook), "tab-overlap", &tab_overlap, NULL);

  return tab_overlap;
}

void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int width = 0;
  int height = 0;
  int text_width = 0;
  int text_height = 0;

  /* Measure text dimensions */
  if (tab_title)
  {
    text_width = iupdrvFontGetStringWidth(ih, tab_title);
    iupdrvFontGetCharSize(ih, NULL, &text_height);
    width = text_width;
    height = text_height;
  }

  /* Add image dimensions */
  if (tab_image)
  {
    void* img = iupImageGetImage(tab_image, ih, 0, NULL);
    if (img)
    {
      int img_w, img_h;
      iupdrvImageGetInfo(img, &img_w, &img_h, NULL);

      /* Width: add image width + spacing */
      width += img_w;
      if (tab_title)
        width += 2;

      /* Height: use MAX of text and image */
      if (img_h > height)
        height = img_h;
    }
  }

  /* Add GTK tab padding and margin */
#if GTK_CHECK_VERSION(3, 0, 0)
  width += 56;  /* GTK3 CSS: 24px padding + 8px margin + ~24px label padding */
  height += 14;  /* GTK3 vertical padding for tabs */
#else
  /* GTK2 padding includes borders, shadows, and internal spacing */
  width += 44;  /* Accounts for GTK2's border/shadow overhead and internal spacing */
  height += 18;  /* GTK2 vertical padding for tabs */
#endif

  /* Add scroll arrows size if scrollable mode is enabled */
  if (ih->handle && gtk_notebook_get_scrollable((GtkNotebook*)ih->handle))
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkNotebook* notebook = (GtkNotebook*)ih->handle;
    gint arrow_length = 16;  /* default */
    const char* style_prop;

    /* Different property names for horizontal vs vertical tabs */
    if (ih->data->type == ITABS_LEFT || ih->data->type == ITABS_RIGHT)
      style_prop = "scroll-arrow-vlength";
    else
      style_prop = "scroll-arrow-hlength";

    gtk_widget_style_get(GTK_WIDGET(notebook), style_prop, &arrow_length, NULL);

    /* GTK has 4 possible arrows (2 on each side), but typically only 2 are active
       Add size for both arrows (left + right for TOP/BOTTOM, or top + bottom for LEFT/RIGHT) */
    width += arrow_length * 2;
#else
    /* GTK2: Use fixed arrow size */
    width += 32;
#endif
  }

  /* For LEFT/RIGHT tabs, swap width/height because tabs are arranged vertically */
  if (ih->data->type == ITABS_LEFT || ih->data->type == ITABS_RIGHT)
  {
    if (tab_width) *tab_width = height;   /* Use text height as tab width */
    if (tab_height) *tab_height = width;  /* Use text width as tab height */
  }
  else
  {
    if (tab_width) *tab_width = width;
    if (tab_height) *tab_height = height;
  }
}

static void gtkTabsSizeAllocateCallback(GtkWidget* widget, GdkRectangle* allocation, Ihandle* ih)
{
  GtkNotebook* notebook = (GtkNotebook*)widget;
  int n_pages = gtk_notebook_get_n_pages(notebook);
  gboolean scrollable = gtk_notebook_get_scrollable(notebook);
  int i;

  /* Calculate total width needed for all tabs */
  if (n_pages > 0 && (ih->data->type == ITABS_TOP || ih->data->type == ITABS_BOTTOM))
  {
    int total_tabs_width = 0;
    int total_allocated_width = 0;
    int i;
    int m, s;

    /* Get decoration margins and spacing */
    m = 4;
    s = 2;

    /* Sum up all tab widths and show actual allocations */
    for (i = 0; i < n_pages; i++)
    {
      GtkWidget* page = gtk_notebook_get_nth_page(notebook, i);
      GtkWidget* tab_label = gtk_notebook_get_tab_label(notebook, page);
      if (tab_label)
      {
        GtkAllocation tab_alloc;
        gtk_widget_get_allocation(tab_label, &tab_alloc);
        total_allocated_width += tab_alloc.width;
#if GTK_CHECK_VERSION(3, 0, 0)
        GtkRequisition tab_min, tab_nat;
        gtk_widget_get_preferred_size(tab_label, &tab_min, &tab_nat);
        total_tabs_width += tab_nat.width;
#else
        GtkRequisition tab_req;
        gtk_widget_size_request(tab_label, &tab_req);
        total_tabs_width += tab_req.width;
#endif
      }
    }

    total_tabs_width += 2 * m;  /* left and right margins */
    if (n_pages > 1)
      total_tabs_width += (n_pages - 1) * s;  /* spacing between tabs */

    gboolean need_scrollable = (total_tabs_width > allocation->width);

    if (need_scrollable != scrollable)
    {
      gtk_notebook_set_scrollable(notebook, need_scrollable);
    }
  }
  else if (n_pages > 0 && (ih->data->type == ITABS_LEFT || ih->data->type == ITABS_RIGHT))
  {
    /* Similar logic for LEFT/RIGHT tabs but checking height instead */
    int total_tabs_height = 0;
    int total_allocated_height = 0;
    int total_tabs_width = 0;
    int total_allocated_width = 0;
    int i;
    int m = 4;

    for (i = 0; i < n_pages; i++)
    {
      GtkWidget* page = gtk_notebook_get_nth_page(notebook, i);
      GtkWidget* tab_label = gtk_notebook_get_tab_label(notebook, page);
      if (tab_label)
      {
        GtkAllocation tab_alloc;
        gtk_widget_get_allocation(tab_label, &tab_alloc);
        total_allocated_height += tab_alloc.height;
        total_allocated_width += tab_alloc.width;
#if GTK_CHECK_VERSION(3, 0, 0)
        GtkRequisition tab_min, tab_nat;
        gtk_widget_get_preferred_size(tab_label, &tab_min, &tab_nat);
        total_tabs_height += tab_nat.height;
        total_tabs_width += tab_nat.width;
#else
        GtkRequisition tab_req;
        gtk_widget_size_request(tab_label, &tab_req);
        total_tabs_height += tab_req.height;
        total_tabs_width += tab_req.width;
#endif
      }
    }

    total_tabs_height += 2 * m;

    gboolean need_scrollable = (total_tabs_height > allocation->height);

    if (need_scrollable != scrollable)
    {
      gtk_notebook_set_scrollable(notebook, need_scrollable);
    }
  }
}

static int gtkTabsMapMethod(Ihandle* ih)
{
  ih->handle = gtk_notebook_new();
  if (!ih->handle)
    return IUP_ERROR;

  /* Start with scrollable=FALSE, will be enabled dynamically if needed in size-allocate */
  gtk_notebook_set_scrollable((GtkNotebook*)ih->handle, FALSE);

  gtkTabsUpdateTabType(ih);

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  gtk_widget_add_events(ih->handle, GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event",  G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event",  G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",      G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",     G_CALLBACK(iupgtkKeyPressEvent),   ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",           G_CALLBACK(iupgtkShowHelp),        ih);

  g_signal_connect(G_OBJECT(ih->handle), "switch-page",         G_CALLBACK(gtkTabsSwitchPage), ih);
  g_signal_connect(G_OBJECT(ih->handle), "size-allocate",       G_CALLBACK(gtkTabsSizeAllocateCallback), ih);

  gtk_widget_realize(ih->handle);

  /* Create pages and tabs */
  if (ih->firstchild)
  {
    Ihandle* child;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");

    for (child = ih->firstchild; child; child = child->brother)
      gtkTabsChildAddedMethod(ih, child);

    if (current_child)
    {
      IupSetAttribute(ih, "VALUE_HANDLE", (char*)current_child);

      /* current value is now given by the native system */
      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
    }

  }

  return IUP_NOERROR;
}

static int gtkTabsSetTipAttrib(Ihandle* ih, const char* value)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK_TABLABEL");
    if (tab_label)
    {
      if (value)
      {
        if (iupAttribGetBoolean(ih, "TIPMARKUP"))
          gtk_widget_set_tooltip_markup(tab_label, iupgtkStrConvertToSystem(value));
        else
          gtk_widget_set_tooltip_text(tab_label, iupgtkStrConvertToSystem(value));
      }
      else
      {
        gtk_widget_set_tooltip_text(tab_label, NULL);
      }
    }
  }

  return 1;
}

void iupdrvTabsInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkTabsMapMethod;
  ic->ChildAdded     = gtkTabsChildAddedMethod;
  ic->ChildRemoved   = gtkTabsChildRemovedMethod;

  /* Driver Dependent Attribute functions */

  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, gtkTabsSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIP", NULL, gtkTabsSetTipAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupTabs only */
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, gtkTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, gtkTabsSetTabOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, gtkTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, gtkTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, gtkTabsSetTabImageAttrib, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, gtkTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, gtkTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}

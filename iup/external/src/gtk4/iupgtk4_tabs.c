/** \file
* \brief Tabs Control for GTK4
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

#include "iupgtk4_drv.h"


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
  iupAttribSet(ih, "_IUPGTK4_IGNORE_CHANGE", "1");
  gtk_notebook_set_current_page((GtkNotebook*)ih->handle, pos);
  iupAttribSet(ih, "_IUPGTK4_IGNORE_CHANGE", NULL);
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  return gtk_notebook_get_current_page((GtkNotebook*)ih->handle);
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
      width += img_w;
      if (tab_title)
        width += 2;  /* spacing between icon and text */
      if (img_h > height)
        height = img_h;
    }
  }

  /* Add GTK4 tab padding and margin */
  width += 56;   /* Match GTK CSS */
  height += 14;  /* 7px top + 7px bottom */
  (void)ih;

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

static void gtk4TabsUpdatePageFont(Ihandle* ih)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK4_TABLABEL");
    if (tab_label)
      iupgtk4UpdateWidgetFont(ih, tab_label);
  }
}

static void gtk4TabsUpdatePageBgColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_container = (GtkWidget*)iupAttribGet(child, "_IUPTAB_CONTAINER");
    if (tab_container)
    {
      GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK4_TABLABEL");
      if (tab_label)
        iupgtk4SetBgColor(tab_label, r, g, b);
      iupgtk4SetBgColor(tab_container, r, g, b);
    }
  }
}

static void gtk4TabsUpdatePageFgColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK4_TABLABEL");
    if (tab_label)
      iupgtk4SetFgColor(tab_label, r, g, b);
  }
}

static void gtk4TabsUpdatePagePadding(Ihandle* ih)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK4_TABLABEL");
    if (tab_label)
    {
      /* GTK4: Use margin properties (no mandatory_gtk3 param needed) */
      iupgtk4SetMargin(tab_label, ih->data->horiz_padding, ih->data->vert_padding);
    }
  }
}

static int gtk4TabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    gtk4TabsUpdatePagePadding(ih);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static void gtk4TabsUpdateTabType(Ihandle* ih)
{
  int iup2gtk[4] = {GTK_POS_TOP, GTK_POS_BOTTOM, GTK_POS_LEFT, GTK_POS_RIGHT};
  gtk_notebook_set_tab_pos((GtkNotebook*)ih->handle, iup2gtk[ih->data->type]);
}

static int gtk4TabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  if(iupStrEqualNoCase(value, "BOTTOM"))
    ih->data->type = ITABS_BOTTOM;
  else if(iupStrEqualNoCase(value, "LEFT"))
    ih->data->type = ITABS_LEFT;
  else if(iupStrEqualNoCase(value, "RIGHT"))
    ih->data->type = ITABS_RIGHT;
  else
    ih->data->type = ITABS_TOP;

  if (ih->handle)
    gtk4TabsUpdateTabType(ih);  /* for this to work must be updated in map */

  return 0;
}

static int gtk4TabsSetTabOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* allow to set only before mapping */
    return 0;

  if(iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->orientation = ITABS_VERTICAL;
  else
    ih->data->orientation = ITABS_HORIZONTAL;

  return 0;
}

static int gtk4TabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABTITLE", value);

    if (value)
    {
      GtkWidget* tab_label = (GtkWidget*)iupAttribGet(child, "_IUPGTK4_TABLABEL");
      if (tab_label)
      {
        GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
        iupgtk4SetMnemonicTitle(ih, (GtkLabel*)tab_label, value);
        gtk_notebook_set_menu_label_text((GtkNotebook*)ih->handle, tab_page, gtk_label_get_text((GtkLabel*)tab_label));
      }
    }
  }

  return 0;
}

static int gtk4TabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    GtkWidget* tab_image;

    iupAttribSetStr(child, "TABIMAGE", value);

    tab_image = (GtkWidget*)iupAttribGet(child, "_IUPGTK4_TABIMAGE");
    if (tab_image)
    {
      if (value)
      {
        GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(value, ih, 0, NULL);
        if (paintable)
          gtk_image_set_from_paintable((GtkImage*)tab_image, paintable);
      }
      else
        gtk_image_set_from_paintable((GtkImage*)tab_image, NULL);
    }
  }
  return 1;
}

static int gtk4TabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
    gtk_widget_set_visible(tab_page, iupStrBoolean(value));
  }
  return 0;
}

int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
  (void)pos;
  return iupgtk4IsVisible(tab_page);
}

static int gtk4TabsSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;
  if (ih->handle)
    gtk4TabsUpdatePageFont(ih);
  return 1;
}

static int gtk4TabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor(ih->handle, r, g, b);
  gtk4TabsUpdatePageFgColor(ih, r, g, b);

  return 1;
}

static int gtk4TabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetBgColor(ih->handle, r, g, b);
  gtk4TabsUpdatePageBgColor(ih, r, g, b);

  return 1;
}

static int gtk4TabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
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

static void gtk4TabsSwitchPage(GtkNotebook* notebook, GtkWidget* page, guint page_num, Ihandle* ih)
{
  IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  int prev_pos = iupdrvTabsGetCurrentTab(ih);

  Ihandle* child = IupGetChild(ih, page_num);
  Ihandle* prev_child = IupGetChild(ih, prev_pos);

  GtkWidget* tab_container = (GtkWidget*)iupAttribGet(child, "_IUPTAB_CONTAINER");
  GtkWidget* prev_tab_container = (GtkWidget*)iupAttribGet(prev_child, "_IUPTAB_CONTAINER");

  if (iupAttribGet(ih, "_IUPGTK4_IGNORE_SWITCHPAGE"))
    return;

  if (tab_container) gtk_widget_set_visible(tab_container, TRUE);
  if (prev_tab_container) gtk_widget_set_visible(prev_tab_container, FALSE);

  if (iupAttribGet(ih, "_IUPGTK4_IGNORE_CHANGE"))
    return;

  if (cb)
    cb(ih, child, prev_child);
  else
  {
    IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
    if (cb2)
      cb2(ih, page_num, prev_pos);
  }

  (void)notebook;
  (void)page;
}

static void gtk4TabsButtonPressed(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *child)
{
  Ihandle* ih = IupGetParent(child);
  IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
  guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

  if (n_press == 1 && button == 3 && cb)
  {
    GtkWidget* tab_page = (GtkWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
    int pos = gtk_notebook_page_num((GtkNotebook*)ih->handle, tab_page);
    cb(ih, pos);
  }

  /* Call standard button press handler for BUTTON_CB support */
  iupgtk4ButtonPressed(gesture, n_press, x, y, ih);
}

static void gtk4TabsCloseButtonClicked(GtkButton *widget, Ihandle* child)
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
      gtk_widget_set_visible(tab_page, FALSE);
  }

  (void)widget;
}

static void gtk4TabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  /* make sure it has at least one name */
  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (ih->handle)
  {
    GtkWidget *tab_page, *tab_container, *box = NULL;
    GtkWidget *tab_label = NULL, *tab_image = NULL, *tab_close = NULL;
    char *tabtitle, *tabimage;
    int pos;
    unsigned char r, g, b;

    pos = IupGetChildPos(ih, child);

    /* Can not hide the tab_page, or the tab will be automatically hidden.
       So create a secondary container to hide its child instead. */
    tab_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_visible(tab_page, TRUE);

    /* Use iupGtk4Fixed (custom container) for proper sizing and positioning */
    tab_container = iupgtk4NativeContainerNew();
    gtk_widget_set_visible(tab_container, TRUE);
    gtk_box_append(GTK_BOX(tab_page), tab_container);

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
      iupgtk4SetMnemonicTitle(ih, (GtkLabel*)tab_label, tabtitle);

      if (ih->data->orientation == ITABS_VERTICAL)
      {
        if (ih->data->type == ITABS_LEFT)
          iupgtk4CssAddStaticRule(".iup-tab-label-vertical-left", "transform: rotate(-90deg);");
        else
          iupgtk4CssAddStaticRule(".iup-tab-label-vertical-right", "transform: rotate(90deg);");
        gtk_widget_add_css_class(tab_label, ih->data->type == ITABS_LEFT ? "iup-tab-label-vertical-left" : "iup-tab-label-vertical-right");
      }
    }

    if (tabimage)
    {
      GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(tabimage, ih, 0, NULL);

      tab_image = gtk_image_new();

      if (paintable)
        gtk_image_set_from_paintable((GtkImage*)tab_image, paintable);
    }

    if(ih->data->show_close)
    {
      GtkWidget* image = gtk_image_new_from_icon_name("window-close-symbolic");
      gtk_image_set_icon_size(GTK_IMAGE(image), GTK_ICON_SIZE_NORMAL);

      tab_close = gtk_button_new();
      gtk_button_set_child(GTK_BUTTON(tab_close), image);
      gtk_button_set_has_frame(GTK_BUTTON(tab_close), FALSE);

      gtk_widget_set_can_focus(tab_close, FALSE);

      g_signal_connect(G_OBJECT(tab_close), "clicked", G_CALLBACK(gtk4TabsCloseButtonClicked), child);
    }

    iupAttribSet(ih, "_IUPGTK4_IGNORE_CHANGE", "1");

    if ((tabimage && tabtitle) || ih->data->show_close)
    {
      if (ih->data->orientation == ITABS_VERTICAL)
        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
      else
        box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_widget_set_visible(box, TRUE);
    }

    GtkWidget* tab_widget;

    if (tabimage && tabtitle)
    {
      gtk_box_append(GTK_BOX(box), tab_image);
      gtk_box_append(GTK_BOX(box), tab_label);

      if(ih->data->show_close)
        gtk_box_append(GTK_BOX(box), tab_close);

      tab_widget = box;
    }
    else if(tabimage && ih->data->show_close)
    {
      gtk_box_append(GTK_BOX(box), tab_image);
      gtk_box_append(GTK_BOX(box), tab_close);
      tab_widget = box;
    }
    else if(tabtitle && ih->data->show_close)
    {
      gtk_box_append(GTK_BOX(box), tab_label);
      gtk_box_append(GTK_BOX(box), tab_close);
      tab_widget = box;
    }
    else if (tabimage)
    {
      tab_widget = tab_image;
    }
    else
    {
      tab_widget = tab_label;
    }

    /* Add gesture controller for right-click */
    GtkGesture* gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0); /* all buttons */
    gtk_widget_add_controller(tab_widget, GTK_EVENT_CONTROLLER(gesture));
    g_signal_connect(gesture, "pressed", G_CALLBACK(gtk4TabsButtonPressed), child);

    gtk_notebook_insert_page((GtkNotebook*)ih->handle, tab_page, tab_widget, pos);

    if (tab_label)
      gtk_notebook_set_menu_label_text((GtkNotebook*)ih->handle, tab_page, gtk_label_get_text((GtkLabel*)tab_label));

    gtk_widget_realize(tab_page);

    iupAttribSet(child, "_IUPGTK4_TABCLOSE", (char*)tab_close);
    iupAttribSet(child, "_IUPGTK4_TABIMAGE", (char*)tab_image);  /* store it even if its NULL */
    iupAttribSet(child, "_IUPGTK4_TABLABEL", (char*)tab_label);
    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)tab_container);
    iupAttribSet(child, "_IUPTAB_PAGE", (char*)tab_page);

    char* allow_reorder = iupAttribGet(ih, "ALLOWREORDER");
    if (iupStrBoolean(allow_reorder))
      gtk_notebook_set_tab_reorderable((GtkNotebook*)ih->handle, tab_page, TRUE);

    iupStrToRGB(IupGetAttribute(ih, "BGCOLOR"), &r, &g, &b);
    iupgtk4SetBgColor(tab_container, r, g, b);

    if (tabtitle)
    {
      iupgtk4UpdateWidgetFont(ih, tab_label);

      iupgtk4SetBgColor(tab_label, r, g, b);

      iupStrToRGB(IupGetAttribute(ih, "FGCOLOR"), &r, &g, &b);
      iupgtk4SetFgColor(tab_label, r, g, b);

      gtk_widget_set_visible(tab_label, TRUE);
      gtk_widget_realize(tab_label);
    }

    if (tabimage)
    {
      gtk_widget_set_visible(tab_image, TRUE);
      gtk_widget_realize(tab_image);
    }

    if (ih->data->show_close)
    {
      gtk_widget_set_visible(tab_close, TRUE);
      gtk_widget_realize(tab_close);
    }

    iupAttribSet(ih, "_IUPGTK4_IGNORE_CHANGE", NULL);

    if (pos != iupdrvTabsGetCurrentTab(ih))
      gtk_widget_set_visible(tab_container, FALSE);
  }
}

static void gtk4TabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
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
          iupAttribSet(ih, "_IUPGTK4_IGNORE_SWITCHPAGE", "1");

        iupTabsCheckCurrentTab(ih, pos, 1);

        iupAttribSet(ih, "_IUPGTK4_IGNORE_CHANGE", "1");
        gtk_notebook_remove_page((GtkNotebook*)ih->handle, page_num);
        iupAttribSet(ih, "_IUPGTK4_IGNORE_CHANGE", NULL);
      }
    }
  }

  child->handle = NULL;

  iupAttribSet(child, "_IUPGTK4_TABCLOSE", NULL);
  iupAttribSet(child, "_IUPGTK4_TABIMAGE", NULL);
  iupAttribSet(child, "_IUPGTK4_TABLABEL", NULL);
  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
  iupAttribSet(child, "_IUPTAB_PAGE", NULL);

  iupAttribSet(ih, "_IUPGTK4_IGNORE_SWITCHPAGE", NULL);
}

static int gtk4TabsMapMethod(Ihandle* ih)
{
  ih->handle = gtk_notebook_new();
  if (!ih->handle)
    return IUP_ERROR;

  /* Prevent GTK4 from expanding notebook beyond its natural size.
   * Content inside can still expand, this only prevents unnecessary expansion. */
  gtk_widget_set_hexpand(ih->handle, FALSE);
  gtk_widget_set_vexpand(ih->handle, FALSE);
  gtk_widget_set_halign(ih->handle, GTK_ALIGN_FILL);
  gtk_widget_set_valign(ih->handle, GTK_ALIGN_FILL);

  gtk_notebook_set_scrollable((GtkNotebook*)ih->handle, TRUE);

  gtk4TabsUpdateTabType(ih);

  /* add to the parent, all GTK controls must call this. */
  iupgtk4AddToParent(ih);

  /* Event controllers instead of signals */
  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
  iupgtk4SetupFocusEvents(ih->handle, ih);
  iupgtk4SetupKeyEvents(ih->handle, ih);

  g_signal_connect(G_OBJECT(ih->handle), "switch-page", G_CALLBACK(gtk4TabsSwitchPage), ih);

  gtk_widget_realize(ih->handle);

  /* Create pages and tabs */
  if (ih->firstchild)
  {
    Ihandle* child;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");

    for (child = ih->firstchild; child; child = child->brother)
      gtk4TabsChildAddedMethod(ih, child);

    if (current_child)
    {
      IupSetAttribute(ih, "VALUE_HANDLE", (char*)current_child);

      /* current value is now given by the native system */
      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
    }
  }

  return IUP_NOERROR;
}

void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = gtk4TabsMapMethod;
  ic->ChildAdded     = gtk4TabsChildAddedMethod;
  ic->ChildRemoved   = gtk4TabsChildRemovedMethod;

  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  iupClassRegisterAttribute(ic, "FONT", NULL, gtk4TabsSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtk4TabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtk4TabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, gtk4TabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, gtk4TabsSetTabOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, gtk4TabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, gtk4TabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, gtk4TabsSetTabImageAttrib, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, gtk4TabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, gtk4TabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}

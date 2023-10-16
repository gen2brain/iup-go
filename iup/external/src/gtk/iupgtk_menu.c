/** \file
 * \brief Menu Resources
 *
 * See Copyright Notice in "iup.h"
 */

#undef GTK_DISABLE_DEPRECATED  /* Since GTK 3.10 gtk_image_menu is deprecated. */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms-compat.h>
#endif

#ifdef HILDON
#include <hildon/hildon-window.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_menu.h"

#include "iupgtk_drv.h"


typedef struct _ImenuPos
{
  int x, y;
  Ihandle* ih;
} ImenuPos;

static void gtkMenuPositionFunc(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, ImenuPos *menupos)
{
  char* value = iupAttribGet(menupos->ih, "POPUPALIGN");

  *x = menupos->x;
  *y = menupos->y;

  if (value)
  {
    GtkRequisition size;
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, value2, ':');

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_get_preferred_size(menupos->ih->handle, NULL, &size);
#else
    gtk_widget_size_request(menupos->ih->handle, &size);
#endif

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      *x -= size.width;
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      *x -= size.width/2;

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      *y -= size.height;
    else if (iupStrEqualNoCase(value2, "ACENTER"))
      *y -= size.height/2;
  }

  *push_in = FALSE;
  (void)menu;
}

int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  ImenuPos menupos;
  menupos.x = x;
  menupos.y = y;
  menupos.ih = ih;
  gtk_menu_popup((GtkMenu*)ih->handle, NULL, NULL, (GtkMenuPositionFunc)gtkMenuPositionFunc,
                 (gpointer)&menupos, 0, gtk_get_current_event_time());
  gtk_main();
  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, NULL, &ch);
#ifdef WIN32
  return 3 + ch + 3;
#else
  return 4 + ch + 4;
#endif
}

/* TODO:
GtkImageMenuItem has been deprecated since GTK+ 3.10. 
If you want to display an icon in a menu item, 
you should use GtkMenuItem and pack a GtkBox with a GtkImage and a GtkLabel instead. 

Furthermore, if you would like to display keyboard accelerator, 
you must pack the accel label into the box using gtk_box_pack_end() 
and align the label, otherwise the accelerator will not display correctly. 

Example:

GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
GtkWidget *icon = gtk_image_new_from_icon_name ("folder-music-symbolic", GTK_ICON_SIZE_MENU);
GtkWidget *label = gtk_accel_label_new ("Music");
GtkWidget *menu_item = gtk_menu_item_new ();
GtkAccelGroup *accel_group = gtk_accel_group_new ();

gtk_container_add (GTK_CONTAINER (box), icon);

gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
gtk_label_set_xalign (GTK_LABEL (label), 0.0);

gtk_widget_add_accelerator (menu_item, "activate", accel_group,
GDK_KEY_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menu_item);

gtk_box_pack_end (GTK_BOX (box), label, TRUE, TRUE, 0);

gtk_container_add (GTK_CONTAINER (menu_item), box);

gtk_widget_show_all (menu_item);
*/

static void gtkItemUpdateImage(Ihandle* ih, const char* value, const char* image, const char* impress)
{
  GdkPixbuf* pixbuf;

  if (!impress || !iupStrBoolean(value))
    pixbuf = iupImageGetImage(image, ih, 0, NULL);
  else
    pixbuf = iupImageGetImage(impress, ih, 0, NULL);

  if (pixbuf)
  {
    GtkWidget* image_label = gtk_image_menu_item_get_image((GtkImageMenuItem*)ih->handle);
    if (!image_label)
    {
      image_label = gtk_image_new();
      gtk_image_menu_item_set_image((GtkImageMenuItem*)ih->handle, image_label);
    }

    if (pixbuf != gtk_image_get_pixbuf((GtkImage*)image_label))
      gtk_image_set_from_pixbuf((GtkImage*)image_label, pixbuf);
  }
  else
    gtk_image_menu_item_set_image((GtkImageMenuItem*)ih->handle, NULL);
}


/*******************************************************************************************/


static void gtkMenuMap(GtkWidget *widget, Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "OPEN_CB");
  if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "OPEN_CB");  /* check also in the Submenu */
  if (cb) cb(ih);

  (void)widget;
}

static void gtkMenuUnMap(GtkWidget *widget, Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "MENUCLOSE_CB");
  if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "MENUCLOSE_CB");  /* check also in the Submenu */
  if (cb) cb(ih);

  (void)widget;
}

static void gtkPopupMenuUnMap(GtkWidget *widget, Ihandle* ih)
{
  gtkMenuUnMap(widget, ih);

  /* quit the popup loop */
  gtk_main_quit();
}

static void gtkItemSelect(GtkWidget *widget, Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "HIGHLIGHT_CB");
  if (cb)
    cb(ih);

  cb = IupGetCallback(ih, "HELP_CB");
  if (cb)
    gtk_menu_set_active((GtkMenu*)ih->parent->handle, IupGetChildPos(ih->parent, ih));

  (void)widget;
}

static void gtkItemActivate(GtkWidget *widget, Ihandle* ih)
{
  Icallback cb;

  if (GTK_IS_CHECK_MENU_ITEM(ih->handle) && !iupAttribGetBoolean(ih, "AUTOTOGGLE") && !iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    /* GTK by default will do autotoggle */
    g_signal_handlers_block_by_func(G_OBJECT(ih->handle), G_CALLBACK(gtkItemActivate), ih);
    gtk_check_menu_item_set_active((GtkCheckMenuItem*)ih->handle, !gtk_check_menu_item_get_active((GtkCheckMenuItem*)ih->handle));
    g_signal_handlers_unblock_by_func(G_OBJECT(ih->handle), G_CALLBACK(gtkItemActivate), ih);
  }

  if (GTK_IS_IMAGE_MENU_ITEM(ih->handle))
  {
    if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
    {
      if (iupAttribGetBoolean(ih, "VALUE"))
        iupAttribSet(ih, "VALUE", "OFF");
      else
        iupAttribSet(ih, "VALUE", "ON");

      gtkItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), iupAttribGet(ih, "IMAGE"), iupAttribGet(ih, "IMPRESS"));
    }
  }

  cb = IupGetCallback(ih, "ACTION");
  if (cb && cb(ih)==IUP_CLOSE)
    IupExitLoop();

  (void)widget;
}

static gboolean gtkMenuKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (evt->keyval == GDK_F1)
  {
    Ihandle* child;
    GtkWidget* active = gtk_menu_get_active((GtkMenu*)widget);
    for (child=ih->firstchild; child; child=child->brother)
    {
      if (child->handle == active)
        iupgtkShowHelp(NULL, NULL, child);
    }
  }

  (void)widget;
  (void)evt;
  return FALSE;
}


/*******************************************************************************************/


static int gtkMenuMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    /* top level menu used for MENU attribute in IupDialog (a menu bar) */
#ifdef HILDON
    Ihandle *pih;
    ih->handle = gtk_menu_new();
    if (!ih->handle)
      return IUP_ERROR;

    pih = iupChildTreeGetNativeParent(ih);
    hildon_window_set_menu(HILDON_WINDOW(pih->handle), GTK_MENU(ih->handle));
#else
    ih->handle = gtk_menu_bar_new();
    if (!ih->handle)
      return IUP_ERROR;

    iupgtkAddToParent(ih);
#endif
  }
  else
  {
    ih->handle = gtk_menu_new();
    if (!ih->handle)
      return IUP_ERROR;

    if (ih->parent)
    {
      /* parent is a submenu */
      gtk_menu_item_set_submenu((GtkMenuItem*)ih->parent->handle, ih->handle);

      g_signal_connect(G_OBJECT(ih->handle), "map", G_CALLBACK(gtkMenuMap), ih);
      g_signal_connect(G_OBJECT(ih->handle), "unmap", G_CALLBACK(gtkMenuUnMap), ih);
    }
    else
    {
      /* top level menu used for IupPopup */
      g_signal_connect(G_OBJECT(ih->handle), "map", G_CALLBACK(gtkMenuMap), ih);
      g_signal_connect(G_OBJECT(ih->handle), "unmap", G_CALLBACK(gtkPopupMenuUnMap), ih);
    }
  }

  gtk_widget_add_events(ih->handle, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event", G_CALLBACK(gtkMenuKeyPressEvent), ih);

  ih->serial = iupMenuGetChildId(ih); 
  gtk_widget_show(ih->handle);

  return IUP_NOERROR;
}

static void gtkMenuUnMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
    ih->parent = NULL;

  iupdrvBaseUnMapMethod(ih);
}

void iupdrvMenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkMenuMapMethod;
  ic->UnMap = gtkMenuUnMapMethod;

  /* Used by iupdrvMenuGetMenuBarSize */
  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);  /* inherited */

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}


/*******************************************************************************************/

static int gtkItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_IMAGE_MENU_ITEM(ih->handle))
  {
    gtkItemUpdateImage(ih, NULL, value, NULL);
    return 1;
  }
  else
    return 0;
}

static int gtkItemSetImageAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_IMAGE_MENU_ITEM(ih->handle))
  {
    gtkItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), value, iupAttribGet(ih, "IMPRESS"));
    return 1;
  }
  else
    return 0;
}

static int gtkItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_IMAGE_MENU_ITEM(ih->handle))
  {
    gtkItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), iupAttribGet(ih, "IMAGE"), value);
    return 1;
  }
  else
    return 0;
}

static int gtkItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  char *str;
  GtkWidget* label;

  if (!value)
  {
    str = "     ";
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  label = gtk_bin_get_child((GtkBin*)ih->handle);

  iupgtkSetMnemonicTitle(ih, (GtkLabel*)label, str);

  if (str != value) free(str);
  return 1;
}

static int gtkItemSetValueAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_CHECK_MENU_ITEM(ih->handle))
  {
    if (iupAttribGetBoolean(ih->parent, "RADIO"))
      value = "ON";

    g_signal_handlers_block_by_func(G_OBJECT(ih->handle), G_CALLBACK(gtkItemActivate), ih);
    gtk_check_menu_item_set_active((GtkCheckMenuItem*)ih->handle, iupStrBoolean(value));
    g_signal_handlers_unblock_by_func(G_OBJECT(ih->handle), G_CALLBACK(gtkItemActivate), ih);
    return 0;
  }
  else if (GTK_IS_IMAGE_MENU_ITEM(ih->handle))
  {
    gtkItemUpdateImage(ih, value, iupAttribGet(ih, "IMAGE"), iupAttribGet(ih, "IMPRESS"));
    return 1;
  }
  else
    return 0;
}

static char* gtkItemGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnChecked(GTK_IS_CHECK_MENU_ITEM(ih->handle) && gtk_check_menu_item_get_active((GtkCheckMenuItem*)ih->handle));
}

static int gtkItemMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent)
    return IUP_ERROR;

#ifndef HILDON
  if (iupMenuIsMenuBar(ih->parent))
    ih->handle = gtk_menu_item_new_with_label("");
  else
#endif
  {
    if (iupAttribGet(ih, "IMAGE")||iupAttribGet(ih, "TITLEIMAGE"))
      ih->handle = gtk_image_menu_item_new_with_label("");
    else if (iupAttribGetBoolean(ih->parent, "RADIO"))
    {
      GtkRadioMenuItem* last_tg = (GtkRadioMenuItem*)iupAttribGet(ih->parent, "_IUPGTK_LASTRADIOITEM");
      if (last_tg)
        ih->handle = gtk_radio_menu_item_new_with_label_from_widget(last_tg, "");
      else
        ih->handle = gtk_radio_menu_item_new_with_label(NULL, "");
      iupAttribSet(ih->parent, "_IUPGTK_LASTRADIOITEM", (char*)ih->handle);
    }
    else
    {
      char* hidemark = iupAttribGetStr(ih, "HIDEMARK");
#if GTK_CHECK_VERSION(2, 14, 0)
      if (!hidemark)
      {
        /* change HIDEMARK default if VALUE is not defined, after GTK 2.14 */
        if (!iupAttribGet(ih, "VALUE")) 
          hidemark = "YES";
      }
#endif

      if (iupStrBoolean(hidemark))
        ih->handle = gtk_menu_item_new_with_label("");
      else
        ih->handle = gtk_check_menu_item_new_with_label("");
    }
  }

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupMenuGetChildId(ih); 

  g_signal_connect(G_OBJECT(ih->handle), "select", G_CALLBACK(gtkItemSelect), ih);
  g_signal_connect(G_OBJECT(ih->handle), "activate", G_CALLBACK(gtkItemActivate), ih);

  pos = IupGetChildPos(ih->parent, ih);
  gtk_menu_shell_insert((GtkMenuShell*)ih->parent->handle, ih->handle, pos);
  gtk_widget_show(ih->handle);

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

void iupdrvItemInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkItemMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupItem only */
  iupClassRegisterAttribute(ic, "VALUE", gtkItemGetValueAttrib, gtkItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, gtkItemSetTitleImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, gtkItemSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupItem GTK and Motif only */
  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}


/*******************************************************************************************/


static int gtkSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_IMAGE_MENU_ITEM(ih->handle))
  {
    gtkItemUpdateImage(ih, NULL, value, NULL);
    return 1;
  }
  else
    return 0;
}

static int gtkSubmenuMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent)
    return IUP_ERROR;

#ifndef HILDON
  if (iupMenuIsMenuBar(ih->parent))
    ih->handle = gtk_menu_item_new_with_label("");
  else
#endif
    ih->handle = gtk_image_menu_item_new_with_label("");

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupMenuGetChildId(ih); 

  pos = IupGetChildPos(ih->parent, ih);
  gtk_menu_shell_insert((GtkMenuShell*)ih->parent->handle, ih->handle, pos);
  gtk_widget_show(ih->handle);

  g_signal_connect(G_OBJECT(ih->handle), "select", G_CALLBACK(gtkItemSelect), ih);

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

void iupdrvSubmenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkSubmenuMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupSubmenu only */
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkSubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}


/*******************************************************************************************/


static int gtkSeparatorMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent)
    return IUP_ERROR;

  ih->handle = gtk_separator_menu_item_new();
  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupMenuGetChildId(ih); 

  pos = IupGetChildPos(ih->parent, ih);
  gtk_menu_shell_insert((GtkMenuShell*)ih->parent->handle, ih->handle, pos);
  gtk_widget_show(ih->handle);

  return IUP_NOERROR;
}

void iupdrvSeparatorInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkSeparatorMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
}

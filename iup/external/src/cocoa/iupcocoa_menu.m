/** \file
 * \brief Menu Resources for Cocoa
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

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
#include "iup_key.h"
#include "iup_menu.h"

#include "iupcocoa_drv.h"


/* Global application menu reference (IUP managed) */
static Ihandle* s_currentIupApplicationMenu = NULL;
/* Global default application menu (Native, created if no IUP menu is provided) */
static NSMenu* s_defaultApplicationMenu = NULL;

static const void* MENUITEM_TARGET_ASSOCIATED_OBJ_KEY = &MENUITEM_TARGET_ASSOCIATED_OBJ_KEY;
static const void* MENU_DELEGATE_ASSOCIATED_OBJ_KEY = &MENU_DELEGATE_ASSOCIATED_OBJ_KEY;
/* Used when an IupItem is placed directly on the menu bar, which requires wrapping it in a submenu on macOS. */
static const void* MENUBAR_ITEM_WRAPPER_KEY = &MENUBAR_ITEM_WRAPPER_KEY;

static void cocoaMenuUpdateImage(Ihandle* ih);
static void cocoaItemUpdateRadioGroup(Ihandle* ih);
static char* cocoaItemGetActiveAttrib(Ihandle* ih);

/*******************************************************************************************/
/* Helper Objects                                                                          */
/*******************************************************************************************/

@interface IupCocoaMenuDelegate : NSObject<NSMenuDelegate>
{
  Ihandle* _ih;
}
@property (nonatomic, assign) Ihandle* ih;
- (instancetype) initWithIhandle:(Ihandle*)ih;
@end

@implementation IupCocoaMenuDelegate
@synthesize ih = _ih;

- (instancetype) initWithIhandle:(Ihandle*)ih
{
  self = [super init];
  if (self)
  {
    _ih = ih;
  }
  return self;
}

- (void) dealloc
{
  _ih = NULL;
  [super dealloc];
}

- (void) menuWillOpen:(NSMenu*)menu
{
  if (!_ih) return;

  Icallback cb = IupGetCallback(_ih, "OPEN_CB");
  if (!cb && _ih->parent)
    cb = (Icallback)IupGetCallback(_ih->parent, "OPEN_CB");

  if (cb) cb(_ih);
}

- (void) menuDidClose:(NSMenu*)menu
{
  if (!_ih) return;

  Icallback cb = IupGetCallback(_ih, "MENUCLOSE_CB");
  /* Check also in the Submenu parent */
  if (!cb && _ih->parent)
    cb = (Icallback)IupGetCallback(_ih->parent, "MENUCLOSE_CB");

  if (cb) cb(_ih);
}

- (void) menuWillHighlightItem:(NSMenuItem*)item
{
  if (!item) return;

  Ihandle* item_ih = (Ihandle*)objc_getAssociatedObject(item, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!item_ih) return;

  Icallback cb = IupGetCallback(item_ih, "HIGHLIGHT_CB");
  if (cb) cb(item_ih);
}

@end


@interface IupCocoaMenuItemTarget : NSObject<NSMenuItemValidation>
{
  Ihandle* _ih;
}
@property (nonatomic, assign) Ihandle* ih;
- (instancetype) initWithIhandle:(Ihandle*)ih;
- (void) onMenuItemAction:(id)sender;
- (BOOL) validateMenuItem:(NSMenuItem*)menuItem;
@end

@implementation IupCocoaMenuItemTarget
@synthesize ih = _ih;

- (instancetype) initWithIhandle:(Ihandle*)ih
{
  self = [super init];
  if (self)
  {
    _ih = ih;
  }
  return self;
}

- (void) dealloc
{
  _ih = NULL;
  [super dealloc];
}

- (void) onMenuItemAction:(id)sender
{
  if (!_ih) return;

  if (iupStrEqual(_ih->iclass->name, "submenu"))
    return;

  NSMenuItem* menu_item = (NSMenuItem*)sender;

  if (iupAttribGetBoolean(_ih->parent, "RADIO"))
  {
    cocoaItemUpdateRadioGroup(_ih);
  }
  else if (iupAttribGetBoolean(_ih, "AUTOTOGGLE"))
  {
    NSInteger current_state = [menu_item state];
    BOOL new_state = (current_state != NSControlStateValueOn);

    iupAttribSet(_ih, "VALUE", new_state ? "ON" : "OFF");
    [menu_item setState:new_state ? NSControlStateValueOn : NSControlStateValueOff];

    cocoaMenuUpdateImage(_ih);
  }

  Icallback cb = IupGetCallback(_ih, "ACTION");
  if (cb && cb(_ih) == IUP_CLOSE)
    IupExitLoop();
}

- (BOOL) validateMenuItem:(NSMenuItem*)menuItem
{
  if (!_ih) return YES;

  char* active = cocoaItemGetActiveAttrib(_ih);
  return (BOOL)iupStrBoolean(active);
}

@end


/*******************************************************************************************/
/* Helper Functions                                                                        */
/*******************************************************************************************/

static void cocoaItemUpdateRadioGroup(Ihandle* ih)
{
  if (!ih || !ih->parent) return;
  if (!iupAttribGetBoolean(ih->parent, "RADIO")) return;

  Ihandle* child;

  /* Set the selected item to ON and all others to OFF */
  for (child = ih->parent->firstchild; child; child = child->brother)
  {
    if (iupStrEqual(child->iclass->name, "item") && child->handle)
    {
      BOOL shouldBeOn = (child == ih);
      NSInteger desiredState = shouldBeOn ? NSControlStateValueOn : NSControlStateValueOff;
      [(NSMenuItem*)child->handle setState:desiredState];
      iupAttribSet(child, "VALUE", shouldBeOn ? "ON" : "OFF");
    }
  }

  /* Update images for all items to reflect the new state */
  for (child = ih->parent->firstchild; child; child = child->brother)
  {
    if (iupStrEqual(child->iclass->name, "item") && child->handle)
    {
      cocoaMenuUpdateImage(child);
    }
  }
}

static int cocoaMenuSetBgColorAttrib(Ihandle* ih, const char* value)
{
  /* NSMenu does not support custom background colors on macOS.
     Menu appearance is controlled by the system. */
  (void)ih;
  (void)value;
  return 0;
}

/* Checks if a given menu contains the standard "Quit" application item. */
static BOOL cocoaMenuContainsQuitItem(NSMenu* menu)
{
  if (!menu) return NO;
  for (NSMenuItem* item in [menu itemArray])
  {
    if ([item action] == @selector(terminate:))
    {
      return YES;
    }
  }
  return NO;
}

/* Finds an NSMenuItem with a specific title (case-insensitive) within a menu. */
static NSMenuItem* cocoaMenuFindTitledItem(NSMenu* menu, NSString* title)
{
  if (!menu || !title) return nil;
  for (NSMenuItem* item in [menu itemArray])
  {
    NSString* itemTitle = [item title];

    /* If the item title is empty (common for the App Menu wrapper at index 0, or IupItem wrappers), check the submenu title. */
    if ((!itemTitle || [itemTitle length] == 0) && [item submenu]) {
      itemTitle = [[item submenu] title];
    }

    if (itemTitle && [itemTitle caseInsensitiveCompare:title] == NSOrderedSame)
    {
      return item;
    }
  }
  return nil;
}

static void cocoaMenuUpdateImage(Ihandle* ih)
{
  NSMenuItem* menu_item = (NSMenuItem*)ih->handle;
  if (!menu_item) return;

  int active = iupAttribGetBoolean(ih, "ACTIVE");
  const char* inactive_suffix = active ? NULL : "IMINACTIVE";

  char* icon_name = iupAttribGet(ih, "TITLEIMAGE");
  if (icon_name)
  {
    NSImage* image = (NSImage*)iupImageGetImage(icon_name, ih, 0, inactive_suffix);
    [menu_item setImage:image];
  }
  else
  {
    if (iupStrEqual(ih->iclass->name, "submenu"))
    {
      char* image_name = iupAttribGet(ih, "IMAGE");
      if (image_name)
      {
        NSImage* image = (NSImage*)iupImageGetImage(image_name, ih, 0, inactive_suffix);
        [menu_item setImage:image];
      }
      else
      {
        [menu_item setImage:nil];
      }
    }
    else
    {
      [menu_item setImage:nil];
    }
  }

  if (iupStrEqual(ih->iclass->name, "item"))
  {
    char* image_off = iupAttribGet(ih, "IMAGE");
    char* image_on = iupAttribGet(ih, "IMPRESS");

    if (iupAttribGetBoolean(ih, "HIDEMARK") && !iupAttribGetBoolean(ih->parent, "RADIO"))
    {
      [menu_item setOnStateImage:nil];
      [menu_item setOffStateImage:nil];
      [menu_item setMixedStateImage:nil];
    }
    else if (image_off || image_on)
    {
      NSImage* ns_image_off = nil;
      if (image_off)
      {
        ns_image_off = (NSImage*)iupImageGetImage(image_off, ih, 0, inactive_suffix);
      }

      NSImage* ns_image_on = nil;
      if (image_on)
      {
        ns_image_on = (NSImage*)iupImageGetImage(image_on, ih, 0, inactive_suffix);
      }
      else if (image_off)
      {
        ns_image_on = ns_image_off;
      }

      [menu_item setOffStateImage:ns_image_off];
      [menu_item setOnStateImage:ns_image_on];
    }
    else
    {
      NSImage* checkmark = [NSImage imageNamed:NSImageNameMenuOnStateTemplate];
      if (checkmark)
      {
        [menu_item setOnStateImage:checkmark];
      }
      [menu_item setOffStateImage:nil];

      NSImage* mixedmark = [NSImage imageNamed:NSImageNameMenuMixedStateTemplate];
      if (mixedmark)
      {
        [menu_item setMixedStateImage:mixedmark];
      }
    }
  }
}

static void cocoaMenuSetTitle(Ihandle* ih, id handle, const char* value)
{
  if (!handle) return;

  if (!value || value[0] == 0)
  {
    value = "     ";
  }

  char* title_str = iupStrProcessMnemonic(value, NULL, 0);

  NSString* ns_title = nil;
  if (title_str)
  {
    ns_title = [NSString stringWithUTF8String:title_str];
  }

  if (!ns_title)
  {
    ns_title = @"";
  }

  if ([handle isKindOfClass:[NSMenuItem class]])
  {
    NSMenuItem* item = (NSMenuItem*)handle;

    IupCocoaFont* iup_font = iupcocoaGetFont(ih);
    NSDictionary* attributes = nil;
    if (iup_font)
    {
      attributes = [iup_font attributeDictionary];
    }

    void (^applyTitle)(NSMenuItem*) = ^(NSMenuItem* targetItem) {
      if (attributes && [attributes count] > 0)
      {
        NSAttributedString* newAttributedTitle = [[NSAttributedString alloc] initWithString:ns_title attributes:attributes];
        [targetItem setAttributedTitle:newAttributedTitle];
        [newAttributedTitle release];
      }
      else
      {
        [targetItem setAttributedTitle:nil];
        [targetItem setTitle:ns_title];
      }
    };

    applyTitle(item);

    NSMenuItem* wrapperItem = (NSMenuItem*)objc_getAssociatedObject(item, MENUBAR_ITEM_WRAPPER_KEY);
    if (wrapperItem)
    {
      applyTitle(wrapperItem);

      NSMenu* submenu = [wrapperItem submenu];
      if (submenu)
      {
        [submenu setTitle:ns_title];
      }
    }
  }
  else if ([handle respondsToSelector:@selector(setTitle:)])
  {
    [handle setTitle:ns_title];
  }

  if (title_str && title_str != value)
    free(title_str);
}

static int cocoaMenuItemSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  NSMenuItem* item = (NSMenuItem*)ih->handle;

  if (!item) return 1;

  char* title = iupAttribGet(ih, "TITLE");
  if (!title) title = "";

  cocoaMenuSetTitle(ih, item, title);

  return 1;
}

/*******************************************************************************************/
/* Driver Functions                                                                        */
/*******************************************************************************************/

int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  NSMenu* menu = (NSMenu*)ih->handle;
  if (!menu) {
    return IUP_ERROR;
  }

  NSPoint location = NSMakePoint(x, iupcocoaComputeCartesianScreenHeightFromIup(y));

  char* align_value = iupAttribGet(ih, "POPUPALIGN");
  if (align_value)
  {
    char value1[30], value2[30];
    iupStrToStrStr(align_value, value1, value2, ':');

    NSSize menu_size = [menu size];

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      location.x -= menu_size.width;
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      location.x -= menu_size.width / 2;

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      location.y += menu_size.height;
    else if (iupStrEqualNoCase(value2, "ACENTER"))
      location.y += menu_size.height / 2;
  }

  [menu popUpMenuPositioningItem:nil atLocation:location inView:nil];

  return IUP_NOERROR;
}

int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  (void)ih;
  /* Standard macOS menu bar height approximation. */
  return 24;
}


/*******************************************************************************************/
/* Application Menu Functions                                                              */
/*******************************************************************************************/

/* Creates the standard macOS "Application" menu (About, Services, Hide, Quit) */
static void cocoaMenuCreateAppMenu(NSMenu* main_menu)
{
  NSString* app_name = [[NSProcessInfo processInfo] processName];
  if (!app_name || [app_name length] == 0) app_name = @"Application";

  NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
  NSMenu* app_menu = [[NSMenu alloc] initWithTitle:app_name];

  [app_menu addItemWithTitle:[NSString stringWithFormat:@"About %@", app_name] action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

  [app_menu addItem:[NSMenuItem separatorItem]];

  NSMenuItem* services_item = [app_menu addItemWithTitle:@"Services" action:nil keyEquivalent:@""];
  NSMenu* services_menu = [[NSMenu alloc] initWithTitle:@"Services"];
  [services_item setSubmenu:services_menu];
  [[NSApplication sharedApplication] setServicesMenu:services_menu];
  [services_menu release];

  [app_menu addItem:[NSMenuItem separatorItem]];

  [app_menu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", app_name] action:@selector(hide:) keyEquivalent:@"h"];

  NSMenuItem* hide_others = [app_menu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
  [hide_others setKeyEquivalentModifierMask: NSEventModifierFlagCommand | NSEventModifierFlagOption];

  [app_menu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

  [app_menu addItem:[NSMenuItem separatorItem]];

  [app_menu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", app_name] action:@selector(terminate:) keyEquivalent:@"q"];


  /* Attach the submenu and insert into the main menu at the beginning */
  [app_item setSubmenu:app_menu];
  [main_menu insertItem:app_item atIndex:0];

  [app_menu release];
  [app_item release];
}

/* Creates a placeholder "File" menu. */
static void cocoaMenuCreateFileMenu(NSMenu* main_menu)
{
  NSMenuItem* file_item = [[NSMenuItem alloc] initWithTitle:@"File" action:nil keyEquivalent:@""];
  NSMenu* file_menu = [[NSMenu alloc] initWithTitle:@"File"];

  [file_menu addItemWithTitle:@"New" action:@selector(newDocument:) keyEquivalent:@"n"];
  [file_menu addItemWithTitle:@"Open..." action:@selector(openDocument:) keyEquivalent:@"o"];
  [file_menu addItem:[NSMenuItem separatorItem]];
  [file_menu addItemWithTitle:@"Close" action:@selector(performClose:) keyEquivalent:@"w"];
  [file_menu addItemWithTitle:@"Save..." action:@selector(saveDocument:) keyEquivalent:@"s"];

  [file_item setSubmenu:file_menu];
  [main_menu addItem:file_item];

  [file_menu release];
  [file_item release];
}

/* Creates a standard macOS "Edit" menu (Undo, Redo, Cut, Copy, Paste). */
static void cocoaMenuCreateEditMenu(NSMenu* main_menu)
{
  NSMenuItem* edit_item = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
  NSMenu* edit_menu = [[NSMenu alloc] initWithTitle:@"Edit"];

  [edit_menu addItemWithTitle:@"Undo" action:@selector(undo:) keyEquivalent:@"z"];
  [edit_menu addItemWithTitle:@"Redo" action:@selector(redo:) keyEquivalent:@"Z"];
  [edit_menu addItem:[NSMenuItem separatorItem]];
  [edit_menu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
  [edit_menu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
  [edit_menu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
  [edit_menu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];

  [edit_item setSubmenu:edit_menu];
  [main_menu addItem:edit_item];

  [edit_menu release];
  [edit_item release];
}

/* Creates the standard macOS "Window" menu (Minimize, Zoom, etc.) */
static void cocoaMenuCreateWindowMenu(NSMenu* main_menu)
{
  NSMenuItem* window_item = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
  NSMenu* window_menu = [[NSMenu alloc] initWithTitle:@"Window"];

  [window_menu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
  [window_menu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
  [window_menu addItem:[NSMenuItem separatorItem]];
  [window_menu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];

  [window_item setSubmenu:window_menu];
  [main_menu addItem:window_item];

  [window_menu release];
  [window_item release];
}

/* Creates a standard macOS "Help" menu. */
static void cocoaMenuCreateHelpMenu(NSMenu* main_menu)
{
  NSMenuItem* help_item = [[NSMenuItem alloc] initWithTitle:@"Help" action:nil keyEquivalent:@""];
  NSMenu* help_menu = [[NSMenu alloc] initWithTitle:@"Help"];

  NSString* app_name = [[NSProcessInfo processInfo] processName];
  if (!app_name || [app_name length] == 0) app_name = @"Application";
  [help_menu addItemWithTitle:[NSString stringWithFormat:@"%@ Help", app_name] action:@selector(showHelp:) keyEquivalent:@"?"];

  [help_item setSubmenu:help_menu];
  [main_menu addItem:help_item];

  [help_menu release];
  [help_item release];
}

/* Synchronizes the NSApplication's standard Window and Help menus with the provided menu bar. */
static void cocoaMenuSynchronizeStandardMenus(NSMenu* menuBar)
{
  if (!menuBar) {
    [[NSApplication sharedApplication] setWindowsMenu:nil];
    [[NSApplication sharedApplication] setHelpMenu:nil];
    return;
  }

  NSMenuItem* windowItem = cocoaMenuFindTitledItem(menuBar, @"Window");
  if (windowItem && [windowItem hasSubmenu]) {
    [[NSApplication sharedApplication] setWindowsMenu:[windowItem submenu]];
  } else {
    [[NSApplication sharedApplication] setWindowsMenu:nil];
  }

  NSMenuItem* helpItem = cocoaMenuFindTitledItem(menuBar, @"Help");
  if (helpItem && [helpItem hasSubmenu]) {
    [[NSApplication sharedApplication] setHelpMenu:[helpItem submenu]];
  } else {
    [[NSApplication sharedApplication] setHelpMenu:nil];
  }
}


/* Ensures that a default application menu exists and is set if no other menu is active. */
void iupcocoaEnsureDefaultApplicationMenu(void)
{
  if (s_defaultApplicationMenu == nil)
  {
    s_defaultApplicationMenu = [[NSMenu alloc] initWithTitle:@"DefaultMainMenu"];
    [s_defaultApplicationMenu setAutoenablesItems:NO];

    if (s_defaultApplicationMenu)
    {
      cocoaMenuCreateAppMenu(s_defaultApplicationMenu);
      cocoaMenuCreateFileMenu(s_defaultApplicationMenu);
      cocoaMenuCreateEditMenu(s_defaultApplicationMenu);
      cocoaMenuCreateWindowMenu(s_defaultApplicationMenu);
      cocoaMenuCreateHelpMenu(s_defaultApplicationMenu);
    }
  }

  if ([[NSApplication sharedApplication] mainMenu] == nil)
  {
    if (s_defaultApplicationMenu)
    {
      cocoaMenuSynchronizeStandardMenus(s_defaultApplicationMenu);
      [[NSApplication sharedApplication] setMainMenu:s_defaultApplicationMenu];
    }
  }
}

Ihandle* iupcocoaMenuGetApplicationMenu(void)
{
  return s_currentIupApplicationMenu;
}

/* Checks if the given Ihandle is the currently active application menu bar. */
int iupcocoaMenuIsApplicationBar(Ihandle* ih)
{
  int result = (ih != NULL && ih == s_currentIupApplicationMenu);
  return result;
}

/* Called during iupdrvClose to clean up resources. */
void iupcocoaMenuCleanupApplicationMenu(void)
{
  s_currentIupApplicationMenu = NULL;

  if (s_defaultApplicationMenu)
  {
    if ([[NSApplication sharedApplication] mainMenu] == s_defaultApplicationMenu)
    {
      [[NSApplication sharedApplication] setMainMenu:nil];
    }
    [s_defaultApplicationMenu release];
    s_defaultApplicationMenu = nil;
  }
}

/* Sets the given Ihandle (IupMenu) as the application's main menu bar.
   This is the central function for switching the application menu. */
void iupcocoaMenuSetApplicationMenu(Ihandle* ih)
{
  if (ih && s_currentIupApplicationMenu == ih) {
    return;
  }

  /* Restore the default menu. */
  if (!ih)
  {
    iupcocoaEnsureDefaultApplicationMenu(); /* Ensure it exists */
    if ([[NSApplication sharedApplication] mainMenu] != s_defaultApplicationMenu)
    {
      cocoaMenuSynchronizeStandardMenus(s_defaultApplicationMenu);
      [[NSApplication sharedApplication] setMainMenu:s_defaultApplicationMenu];
    }
    s_currentIupApplicationMenu = NULL; /* Update IUP tracking */
    return;
  }

  /* Set a new IUP menu. */
  if (!ih->handle) {
    IupMap(ih);
  }

  if (ih->handle && [(id)ih->handle isKindOfClass:[NSMenu class]])
  {
    NSMenu* menu = (NSMenu*)ih->handle;

    NSMenuItem* appMenuItem = ([menu numberOfItems] > 0) ? [menu itemAtIndex:0] : nil;
    if (!appMenuItem || ![appMenuItem hasSubmenu] || !cocoaMenuContainsQuitItem([appMenuItem submenu]))
    {
      cocoaMenuCreateAppMenu(menu);
    }

    if (!cocoaMenuFindTitledItem(menu, @"Window"))
    {
      cocoaMenuCreateWindowMenu(menu);
    }

    if (!cocoaMenuFindTitledItem(menu, @"Help"))
    {
      cocoaMenuCreateHelpMenu(menu);
    }

    cocoaMenuSynchronizeStandardMenus(menu);

    [[NSApplication sharedApplication] setMainMenu:menu];
    s_currentIupApplicationMenu = ih;
  }
  else {
    iupcocoaMenuSetApplicationMenu(NULL);
  }
}

/*******************************************************************************************/
/* MENU (IupMenu)                                                                          */
/*******************************************************************************************/

static int cocoaMenuMapMethod(Ihandle* ih)
{
  NSMenu* menu = nil;

  if (iupMenuIsMenuBar(ih))
  {
    menu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
    if (!menu) return IUP_ERROR;
    [menu setAutoenablesItems:NO];
    ih->handle = menu;

    if ([menu numberOfItems] == 0 || ![[menu itemAtIndex:0] hasSubmenu] || !cocoaMenuContainsQuitItem([[menu itemAtIndex:0] submenu]))
    {
      cocoaMenuCreateAppMenu(menu);
    }
  }
  else
  {
    menu = [[NSMenu alloc] initWithTitle:@""];
    if (!menu) return IUP_ERROR;
    ih->handle = menu;

    if (ih->parent)
    {
      if (!ih->parent->handle)
      {
        [menu release];
        ih->handle = NULL;
        return IUP_ERROR;
      }

      NSMenuItem* parent_item = (NSMenuItem*)ih->parent->handle;
      if ([parent_item isKindOfClass:[NSMenuItem class]])
      {
        [parent_item setSubmenu:menu];
        [menu setTitle:[parent_item title]];
      }
      else
      {
        [menu release];
        ih->handle = NULL;
        return IUP_ERROR;
      }
    }
  }

  IupCocoaMenuDelegate* delegate = [[IupCocoaMenuDelegate alloc] initWithIhandle:ih];
  [menu setDelegate:delegate];

  objc_setAssociatedObject(menu, MENU_DELEGATE_ASSOCIATED_OBJ_KEY, delegate, OBJC_ASSOCIATION_RETAIN);
  [delegate release];

  objc_setAssociatedObject(menu, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  return IUP_NOERROR;
}

static void cocoaMenuUnMapMethod(Ihandle* ih)
{
  if (iupcocoaMenuIsApplicationBar(ih))
  {
    iupcocoaMenuSetApplicationMenu(NULL);
  }

  NSMenu* menu = (NSMenu*)ih->handle;
  if (!menu) return;

  if (iupMenuIsMenuBar(ih))
  {
    ih->parent = NULL;
  }

  [menu setDelegate:nil];
  objc_setAssociatedObject(menu, MENU_DELEGATE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(menu, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

  if (!iupMenuIsMenuBar(ih) && ih->parent && ih->parent->handle)
  {
    NSMenuItem* parent_item = (NSMenuItem*)ih->parent->handle;
    if ([parent_item isKindOfClass:[NSMenuItem class]] && [parent_item submenu] == menu)
    {
      [parent_item setSubmenu:nil];
    }
  }

  [menu release];
  ih->handle = NULL;
}

void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = cocoaMenuMapMethod;
  ic->UnMap = cocoaMenuUnMapMethod;
  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaMenuSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
}

/*******************************************************************************************/
/* SEPARATOR (IupSeparator)                                                                */
/*******************************************************************************************/

static int cocoaSeparatorMapMethod(Ihandle* ih)
{
  if (!ih->parent || !ih->parent->handle)
    return IUP_ERROR;

  NSMenu* parent_menu = (NSMenu*)ih->parent->handle;
  if (![parent_menu isKindOfClass:[NSMenu class]])
    return IUP_ERROR;

  NSMenuItem* item = [NSMenuItem separatorItem];
  [item retain]; /* Retain it because ih->handle owns it. */
  ih->handle = item;
  ih->serial = iupMenuGetChildId(ih);
  [item setTag:ih->serial];

  int pos = IupGetChildPos(ih->parent, ih);
  if (iupMenuIsMenuBar(ih->parent))
  {
    pos++;
  }

  [parent_menu insertItem:item atIndex:pos];
  objc_setAssociatedObject(item, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
  return IUP_NOERROR;
}

static void cocoaSeparatorUnMapMethod(Ihandle* ih)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return;
  NSMenu* parent_menu = [item menu];
  if (parent_menu)
    [parent_menu removeItem:item];
  objc_setAssociatedObject(item, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
  [item release];
  ih->handle = NULL;
}

void iupdrvSeparatorInitClass(Iclass* ic)
{
  ic->Map = cocoaSeparatorMapMethod;
  ic->UnMap = cocoaSeparatorUnMapMethod;
}


/*******************************************************************************************/
/* ITEM (IupItem)                                                                          */
/*******************************************************************************************/

static void cocoaItemSetShortcutFromString(NSMenuItem* item, const char* shortcut_string)
{
    if (!item) return;

    if (!shortcut_string || shortcut_string[0] == '\0')
    {
        [item setKeyEquivalent:@""];
        [item setKeyEquivalentModifierMask:0];
        return;
    }

    NSUInteger mask = 0;
    char key_char = 0;
    char* current_part = iupStrDup(shortcut_string);
    char* next_token = current_part;

    while(next_token)
    {
        char* plus_pos = strchr(next_token, '+');
        if (plus_pos) {
            *plus_pos = '\0';
        }

        if (iupStrEqualNoCase(next_token, "Ctrl"))
            mask |= NSEventModifierFlagCommand;
        else if (iupStrEqualNoCase(next_token, "Shift"))
            mask |= NSEventModifierFlagShift;
        else if (iupStrEqualNoCase(next_token, "Alt"))
            mask |= NSEventModifierFlagOption;
        else if (iupStrEqualNoCase(next_token, "Sys"))
            mask |= NSEventModifierFlagCommand;
        else if (strlen(next_token) == 1) {
            key_char = next_token[0];
        }

        if (plus_pos) {
            next_token = plus_pos + 1;
        } else {
            next_token = NULL;
        }
    }
    free(current_part);

    if (key_char)
    {
        NSString* key_str = [[NSString stringWithFormat:@"%c", key_char] lowercaseString];
        [item setKeyEquivalent:key_str];
        [item setKeyEquivalentModifierMask:mask];
    }
    else
    {
        [item setKeyEquivalent:@""];
        [item setKeyEquivalentModifierMask:0];
    }
}

static int cocoaItemSetTitleAttrib(Ihandle* ih, const char* value)
{
    NSMenuItem* item = (NSMenuItem*)ih->handle;
    if (!item) return 0;

    if (!value) value = "";

    const char* tab_pos = strchr(value, '\t');
    char* title_part;
    const char* shortcut_part = NULL;

    if (tab_pos)
    {
        int len = tab_pos - value;
        title_part = (char*)malloc(len + 1);
        strncpy(title_part, value, len);
        title_part[len] = '\0';
        shortcut_part = tab_pos + 1;
    }
    else
    {
        title_part = iupStrDup(value);
    }

    cocoaMenuSetTitle(ih, item, title_part);
    free(title_part);

    if (tab_pos)
    {
        cocoaItemSetShortcutFromString(item, shortcut_part);
    }

    return 1;
}

static int cocoaItemSetValueAttrib(Ihandle* ih, const char* value)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return 0;

  if (iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    cocoaItemUpdateRadioGroup(ih);
  }
  else
  {
    BOOL desired_checked = iupStrBoolean(value);
    NSInteger desiredState = desired_checked ? NSControlStateValueOn : NSControlStateValueOff;
    [item setState:desiredState];
    iupAttribSet(ih, "VALUE", desired_checked ? "ON" : "OFF");
    cocoaMenuUpdateImage(ih);
  }

  return 1;
}

static char* cocoaItemGetValueAttrib(Ihandle* ih)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return "OFF";
  return iupStrReturnBoolean([item state] == NSControlStateValueOn);
}

static int cocoaItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return 0;

  iupBaseSetActiveAttrib(ih, value);

  BOOL enabled = (BOOL)iupStrBoolean(value);
  [item setEnabled:enabled];

  cocoaMenuUpdateImage(ih);
  return 1;
}

static char* cocoaItemGetActiveAttrib(Ihandle* ih)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean([item isEnabled]);
}

static int cocoaItemSetImageAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  cocoaMenuUpdateImage(ih);
  return 1;
}

static int cocoaItemSetHideMarkAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  cocoaMenuUpdateImage(ih);
  return 1;
}

static int cocoaItemSetKeyAttrib(Ihandle* ih, const char* value)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return 0;

  cocoaItemSetShortcutFromString(item, value);

  return 1;
}

static int cocoaItemMapMethod(Ihandle* ih)
{
  if (!ih->parent || !ih->parent->handle) return IUP_ERROR;
  NSMenu* parent_menu = (NSMenu*)ih->parent->handle;
  if (![parent_menu isKindOfClass:[NSMenu class]]) return IUP_ERROR;

  BOOL isMenuBar = iupMenuIsMenuBar(ih->parent);
  NSMenu* insertionMenu = parent_menu;
  NSMenuItem* menuBarWrapperItem = nil;

  if (isMenuBar)
  {
    menuBarWrapperItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
    [menuBarWrapperItem setSubmenu:submenu];
    [submenu release];
    insertionMenu = submenu;
  }

  NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:@selector(onMenuItemAction:) keyEquivalent:@""];
  ih->handle = item;
  ih->serial = iupMenuGetChildId(ih);

  IupCocoaMenuItemTarget* target = [[IupCocoaMenuItemTarget alloc] initWithIhandle:ih];
  [item setTarget:target];
  [item setTag:ih->serial];
  objc_setAssociatedObject(item, MENUITEM_TARGET_ASSOCIATED_OBJ_KEY, target, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(item, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
  [target release];

  if (isMenuBar)
  {
    objc_setAssociatedObject(item, MENUBAR_ITEM_WRAPPER_KEY, menuBarWrapperItem, OBJC_ASSOCIATION_RETAIN);
    int pos = IupGetChildPos(ih->parent, ih) + 1;
    [parent_menu insertItem:menuBarWrapperItem atIndex:pos];
    [menuBarWrapperItem release];
    [insertionMenu insertItem:item atIndex:0];
  }
  else
  {
    int pos = IupGetChildPos(ih->parent, ih);
    [insertionMenu insertItem:item atIndex:pos];
  }
  return IUP_NOERROR;
}

static void cocoaItemUnMapMethod(Ihandle* ih)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return;

  NSMenuItem* menuBarWrapperItem = (NSMenuItem*)objc_getAssociatedObject(item, MENUBAR_ITEM_WRAPPER_KEY);
  if (menuBarWrapperItem)
  {
    [[menuBarWrapperItem menu] removeItem:menuBarWrapperItem];
    [[item menu] removeItem:item];
    objc_setAssociatedObject(item, MENUBAR_ITEM_WRAPPER_KEY, nil, OBJC_ASSOCIATION_RETAIN);
  }
  else
  {
    [[item menu] removeItem:item];
  }

  objc_setAssociatedObject(item, MENUITEM_TARGET_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(item, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
  [item setTarget:nil];
  [item release];
  ih->handle = NULL;
}

void iupdrvItemInitClass(Iclass* ic)
{
  ic->Map = cocoaItemMapMethod;
  ic->UnMap = cocoaItemUnMapMethod;
  iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", cocoaItemGetValueAttrib, cocoaItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONT", NULL, cocoaMenuItemSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", cocoaItemGetActiveAttrib, cocoaItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "KEY", NULL, cocoaItemSetKeyAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, cocoaItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, cocoaItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, cocoaItemSetHideMarkAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

/*******************************************************************************************/
/* SUBMENU (IupSubmenu)                                                                    */
/*******************************************************************************************/

static int cocoaSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return 0;

  if (!value) value = "";

  const char* tab_pos = strchr(value, '\t');
  char* title_part;

  if (tab_pos)
  {
      int len = tab_pos - value;
      title_part = (char*)malloc(len + 1);
      strncpy(title_part, value, len);
      title_part[len] = '\0';
  }
  else
  {
      title_part = iupStrDup(value);
  }

  cocoaMenuSetTitle(ih, item, title_part);

  if ([item hasSubmenu])
  {
    NSString* ns_title = [item title];
    if (!ns_title) ns_title = @"";
    [[item submenu] setTitle:ns_title];
  }

  free(title_part);
  return 1;
}

static int cocoaSubmenuSetActiveAttrib(Ihandle* ih, const char* value)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return 0;

  iupBaseSetActiveAttrib(ih, value);

  BOOL enabled = (BOOL)iupStrBoolean(value);
  [item setEnabled:enabled];

  cocoaMenuUpdateImage(ih);
  return 1;
}

static char* cocoaSubmenuGetActiveAttrib(Ihandle* ih)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean([item isEnabled]);
}

static int cocoaSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  cocoaMenuUpdateImage(ih);
  return 1;
}

static int cocoaSubmenuMapMethod(Ihandle* ih)
{
  if (!ih->parent || !ih->parent->handle) return IUP_ERROR;
  NSMenu* parent_menu = (NSMenu*)ih->parent->handle;
  if (![parent_menu isKindOfClass:[NSMenu class]]) return IUP_ERROR;

  NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
  ih->handle = item;
  ih->serial = iupMenuGetChildId(ih);

  IupCocoaMenuItemTarget* target = [[IupCocoaMenuItemTarget alloc] initWithIhandle:ih];
  [item setTarget:target];
  [item setTag:ih->serial];
  objc_setAssociatedObject(item, MENUITEM_TARGET_ASSOCIATED_OBJ_KEY, target, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(item, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
  [target release];

  int pos = IupGetChildPos(ih->parent, ih);
  if (iupMenuIsMenuBar(ih->parent)) pos++;

  [parent_menu insertItem:item atIndex:pos];
  return IUP_NOERROR;
}

static void cocoaSubmenuUnMapMethod(Ihandle* ih)
{
  NSMenuItem* item = (NSMenuItem*)ih->handle;
  if (!item) return;
  [[item menu] removeItem:item];
  objc_setAssociatedObject(item, MENUITEM_TARGET_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(item, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
  [item setTarget:nil];
  [item setSubmenu:nil];
  [item release];
  ih->handle = NULL;
}

void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = cocoaSubmenuMapMethod;
  ic->UnMap = cocoaSubmenuUnMapMethod;
  iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaSubmenuSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONT", NULL, cocoaMenuItemSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", cocoaSubmenuGetActiveAttrib, cocoaSubmenuSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaSubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, cocoaSubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}


/*******************************************************************************************/
/* Recent Menu Support                                                                     */
/*******************************************************************************************/

static const void* RECENT_MENU_ASSOCIATED_OBJ_KEY = &RECENT_MENU_ASSOCIATED_OBJ_KEY;

@interface IupCocoaRecentMenuItemTarget : NSObject
{
  Ihandle* _menu;
  int _index;
}
@property (nonatomic, assign) Ihandle* menu;
@property (nonatomic, assign) int index;
- (instancetype) initWithMenu:(Ihandle*)menu index:(int)index;
- (void) onRecentItemAction:(id)sender;
@end

@implementation IupCocoaRecentMenuItemTarget
@synthesize menu = _menu;
@synthesize index = _index;

- (instancetype) initWithMenu:(Ihandle*)menu index:(int)index
{
  self = [super init];
  if (self)
  {
    _menu = menu;
    _index = index;
  }
  return self;
}

- (void) dealloc
{
  _menu = NULL;
  [super dealloc];
}

- (void) onRecentItemAction:(id)sender
{
  if (!_menu) return;

  Icallback recent_cb = (Icallback)iupAttribGet(_menu, "_IUP_RECENT_CB");
  Ihandle* config = (Ihandle*)iupAttribGet(_menu, "_IUP_CONFIG");

  if (recent_cb && config)
  {
    char attr_name[32];
    const char* filename;

    sprintf(attr_name, "_IUP_RECENT_FILE%d", _index);
    filename = iupAttribGet(_menu, attr_name);

    if (filename)
    {
      IupSetStrAttribute(config, "RECENTFILENAME", filename);
      IupSetStrAttribute(config, "TITLE", filename);
      config->parent = _menu;

      if (recent_cb(config) == IUP_CLOSE)
        IupExitLoop();

      config->parent = NULL;
      IupSetAttribute(config, "RECENTFILENAME", NULL);
      IupSetAttribute(config, "TITLE", NULL);
    }
  }
}

@end

int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  NSMenu* nsmenu;
  int max_recent, existing, i;

  if (!menu || !menu->handle)
    return -1;

  nsmenu = (NSMenu*)menu->handle;
  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  for (i = 0; i < count; i++)
  {
    char attr_name[32];
    NSString* title = [NSString stringWithUTF8String:filenames[i]];

    sprintf(attr_name, "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);

    if (i < existing)
    {
      NSMenuItem* item = [nsmenu itemAtIndex:i];
      [item setTitle:title];
    }
    else
    {
      IupCocoaRecentMenuItemTarget* target = [[IupCocoaRecentMenuItemTarget alloc] initWithMenu:menu index:i];
      NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
                                                    action:@selector(onRecentItemAction:)
                                             keyEquivalent:@""];
      [item setTarget:target];
      [item setTag:i];
      objc_setAssociatedObject(item, RECENT_MENU_ASSOCIATED_OBJ_KEY, target, OBJC_ASSOCIATION_RETAIN);
      [target release];

      [nsmenu insertItem:item atIndex:i];
      [item release];
    }
  }

  while ([nsmenu numberOfItems] > count && existing > count)
  {
    NSMenuItem* item = [nsmenu itemAtIndex:count];
    objc_setAssociatedObject(item, RECENT_MENU_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
    [nsmenu removeItemAtIndex:count];
    existing--;

    char attr_name[32];
    sprintf(attr_name, "_IUP_RECENT_FILE%d", existing);
    iupAttribSet(menu, attr_name, NULL);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}
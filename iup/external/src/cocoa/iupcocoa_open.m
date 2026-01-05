/** \file
 * \brief macOS Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
#include "iup_object.h"
#include "iup_str.h"

#include "iupcocoa_drv.h"

static NSAutoreleasePool* s_autoreleasePool = nil;

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return NULL;
}

static bool cocoaGetByteRGBAFromNSColor(NSColor* ns_color, unsigned char* red, unsigned char* green, unsigned char* blue, unsigned char* alpha)
{
  NSColor* rgb_color = [ns_color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
  if (rgb_color)
  {
    CGFloat rgba_components[4];
    [rgb_color getComponents:rgba_components];
    *red = (unsigned char)iupROUND(rgba_components[0] * 255.0);
    *green = (unsigned char)iupROUND(rgba_components[1] * 255.0);
    *blue = (unsigned char)iupROUND(rgba_components[2] * 255.0);
    *alpha = (unsigned char)iupROUND(rgba_components[3] * 255.0);
    return true;
  }
  else
  {
    return false;
  }
}

static void iupCocoaUpdateGlobalColors(void)
{
  unsigned char r, g, b, a;

  if (cocoaGetByteRGBAFromNSColor([NSColor windowBackgroundColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", r, g, b);
  if (cocoaGetByteRGBAFromNSColor([NSColor labelColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", r, g, b);

  if (cocoaGetByteRGBAFromNSColor([NSColor textBackgroundColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", r, g, b);
  if (cocoaGetByteRGBAFromNSColor([NSColor textColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", r, g, b);
  if (cocoaGetByteRGBAFromNSColor([NSColor selectedTextBackgroundColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", r, g, b);

  if (cocoaGetByteRGBAFromNSColor([NSColor linkColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", r, g, b);

  if (cocoaGetByteRGBAFromNSColor([NSColor windowBackgroundColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", r, g, b);
  if (cocoaGetByteRGBAFromNSColor([NSColor labelColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", r, g, b);
}

static const char* iupCocoaGetSystemLanguage(void)
{
  static char iupmac_language[20] = "en";
  NSString* language = [[NSLocale preferredLanguages] firstObject];
  if (language)
  {
    strncpy(iupmac_language, [language UTF8String], 19);
    iupmac_language[19] = 0;
  }
  return iupmac_language;
}

int iupdrvOpen(int* argc, char*** argv)
{
  (void)argc;
  (void)argv;

  if (nil == s_autoreleasePool)
  {
    s_autoreleasePool = [[NSAutoreleasePool alloc] init];
  }

  [NSApplication sharedApplication];

  /* Ensure the application is treated as a foreground application capable of showing UI. */
  if ([NSApp activationPolicy] == NSApplicationActivationPolicyProhibited)
  {
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  }

  /* Manually finish the launch process. This is normally handled by [NSApp run], but IUP uses a custom event loop. */
  [NSApp finishLaunching];

  /* This sets a default menu at startup. It will be replaced later if an IUP dialog with a menu is shown. */
  iupcocoaEnsureDefaultApplicationMenu();

  /* Disable automatic window tabbing */
  if ([NSWindow respondsToSelector:@selector(setAllowsAutomaticWindowTabbing:)])
  {
    [NSWindow setAllowsAutomaticWindowTabbing:NO];
  }

  IupSetGlobal("DRIVER", "Cocoa");
  IupSetGlobal("WINDOWING", "COCOA");
  IupSetGlobal("SYSTEMLANGUAGE", iupCocoaGetSystemLanguage());

  iupCocoaUpdateGlobalColors();
  IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");

  /* All NSStrings in this implementation use UTF-8. */
  IupSetInt(NULL, "UTF8MODE", 1);

  return IUP_NOERROR;
}

int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  NSString* appName = [NSString stringWithUTF8String:value];
  [[NSProcessInfo processInfo] setProcessName:appName];
  appname_set = 1;
  return 1;
}

void iupdrvClose(void)
{
  /* This cleans up the default menu instance and the IUP menu tracking. */
  iupcocoaMenuCleanupApplicationMenu();

  /*
   * Draining the autorelease pool here can cause a crash, especially when the app
   * is quit by closing the last window. The system seems to require the pool
   * to exist longer than the IupClose call. We accept a minor memory leak at
   * program termination, which is standard practice for modern applications,
   * rather than risk a crash. The pool is not set to nil to prevent re-allocation
   * if IupOpen/IupClose are called multiple times.
   */
  /* [s_autoreleasePool drain]; */
}

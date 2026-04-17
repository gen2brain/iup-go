/** \file
 * \brief macOS Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include "iup.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
#include "iup_object.h"

#include "iupcocoa_drv.h"

#ifdef GNUSTEP
#import <GNUstepGUI/GSTheme.h>
#endif


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

IUP_DRV_API void iupcocoaSetGlobalColors(void)
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

IUP_SDK_API int iupdrvOpen(int* argc, char*** argv)
{
  (void)argc;
  (void)argv;

  if (nil == s_autoreleasePool)
  {
    s_autoreleasePool = [[NSAutoreleasePool alloc] init];
  }

#ifdef GNUSTEP
  /* Seed NSFont defaults before sharedApplication. */
  {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults stringForKey:@"NSFont"])
    {
      NSArray *regular_keys = @[
        @"NSFont", @"NSUserFont", @"NSSystemFont", @"NSLabelFont",
        @"NSMessageFont", @"NSPaletteFont", @"NSTitleBarFont",
        @"NSToolTipsFont", @"NSControlContentFont", @"NSMenuFont"
      ];
      for (NSString *k in regular_keys)
      {
        [defaults setObject:@"DejaVuSans" forKey:k];
      }
      [defaults setObject:@"DejaVuSans-Bold" forKey:@"NSBoldFont"];
      [defaults setObject:@"DejaVuSans-Bold" forKey:@"NSBoldSystemFont"];
      [defaults setObject:@"DejaVuSansMono-Book" forKey:@"NSUserFixedPitchFont"];
    }
  }
#endif

  [NSApplication sharedApplication];

#ifdef GNUSTEP
  {
    const char* theme_env = getenv("IUP_GNUSTEPTHEME");
    if (theme_env && theme_env[0])
    {
      NSString* theme_name = [NSString stringWithUTF8String:theme_env];
      GSTheme* theme = [GSTheme loadThemeNamed:theme_name];
      if (theme)
      {
        [[NSUserDefaults standardUserDefaults] setObject:theme_name forKey:@"GSTheme"];
        [GSTheme setTheme:theme];
      }
    }
  }

  /* Backend is loaded now; verify and upgrade the seeds if Sans
     isn't actually installed. fontWithName:size: is silent on miss. */
  {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *current = [defaults stringForKey:@"NSFont"];
    if (!current || ![NSFont fontWithName:current size:12])
    {
      NSArray *regular_candidates = @[
        @"DejaVuSans", @"BitstreamVeraSans-Roman", @"LiberationSans",
        @"FreeSans", @"NimbusSans-Regular", @"NimbusSansL-Regu"
      ];
      NSArray *bold_candidates = @[
        @"DejaVuSans-Bold", @"BitstreamVeraSans-Bold", @"LiberationSans-Bold",
        @"FreeSansBold", @"NimbusSans-Bold", @"NimbusSansL-Bold"
      ];
      NSArray *mono_candidates = @[
        @"DejaVuSansMono", @"BitstreamVeraSansMono-Roman", @"LiberationMono",
        @"FreeMono", @"NimbusMono-Regular", @"NimbusMonoL-Regu", @"Courier"
      ];
      NSString *chosen_regular = nil;
      NSString *chosen_bold = nil;
      NSString *chosen_mono = nil;
      for (NSString *n in regular_candidates) { if ([NSFont fontWithName:n size:12]) { chosen_regular = n; break; } }
      for (NSString *n in bold_candidates)    { if ([NSFont fontWithName:n size:12]) { chosen_bold = n; break; } }
      for (NSString *n in mono_candidates)    { if ([NSFont fontWithName:n size:12]) { chosen_mono = n; break; } }
      if (chosen_regular)
      {
        [defaults setObject:chosen_regular forKey:@"NSFont"];
        [defaults setObject:chosen_regular forKey:@"NSUserFont"];
        [defaults setObject:chosen_regular forKey:@"NSSystemFont"];
        [defaults setObject:chosen_regular forKey:@"NSLabelFont"];
        [defaults setObject:chosen_regular forKey:@"NSMessageFont"];
        [defaults setObject:chosen_regular forKey:@"NSPaletteFont"];
        [defaults setObject:chosen_regular forKey:@"NSTitleBarFont"];
        [defaults setObject:chosen_regular forKey:@"NSToolTipsFont"];
        [defaults setObject:chosen_regular forKey:@"NSControlContentFont"];
        [defaults setObject:chosen_regular forKey:@"NSMenuFont"];
      }
      if (chosen_bold)
      {
        [defaults setObject:chosen_bold forKey:@"NSBoldFont"];
        [defaults setObject:chosen_bold forKey:@"NSBoldSystemFont"];
      }
      if (chosen_mono)
      {
        [defaults setObject:chosen_mono forKey:@"NSUserFixedPitchFont"];
      }
    }
  }
#endif

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
#ifdef GNUSTEP
  IupSetGlobal("WINDOWING", "X11");
#else
  IupSetGlobal("WINDOWING", "QUARTZ");
#endif
  IupSetGlobal("SYSTEMLANGUAGE", iupCocoaGetSystemLanguage());

  iupcocoaSetGlobalColors();
  IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");

  /* All NSStrings in this implementation use UTF-8. */
  IupSetInt(NULL, "UTF8MODE", 1);

  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  NSString* appName = [NSString stringWithUTF8String:value];
  [[NSProcessInfo processInfo] setProcessName:appName];
  appname_set = 1;
  return 1;
}

IUP_SDK_API void iupdrvClose(void)
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

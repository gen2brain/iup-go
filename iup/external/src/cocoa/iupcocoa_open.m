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
#include "iupunix_portal.h"
#endif

#ifdef GNUSTEP
@interface IupGnustepApplicationDelegate : NSObject
@end

@implementation IupGnustepApplicationDelegate
- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename
{
  (void)sender;
  (void)filename;
  return NO;
}

- (void)application:(NSApplication*)sender openFiles:(NSArray*)filenames
{
  (void)sender;
  (void)filenames;
}
@end

static IupGnustepApplicationDelegate* cocoa_gnustep_app_delegate = nil;

@interface IupGnustepPortalSettings : NSObject
{
  NSFileHandle* handle;
}
- (id)initWithFileDescriptor:(int)fd;
- (void)stop;
@end

@implementation IupGnustepPortalSettings
- (id)initWithFileDescriptor:(int)fd
{
  self = [super init];
  if (self)
  {
    handle = [[NSFileHandle alloc] initWithFileDescriptor:fd closeOnDealloc:NO];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(dataAvailable:) name:NSFileHandleDataAvailableNotification object:handle];
    [handle waitForDataInBackgroundAndNotify];
  }
  return self;
}

- (void)dataAvailable:(NSNotification*)notification
{
  (void)notification;
  if (iupUnixPortalSettingsDispatch())
    [handle waitForDataInBackgroundAndNotify];
}

- (void)stop
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [handle release];
  handle = nil;
}

- (void)dealloc
{
  [self stop];
  [super dealloc];
}
@end

static IupGnustepPortalSettings* cocoa_gnustep_portal_settings = nil;
#endif


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

static int cocoaIsSystemDarkMode(void)
{
#ifdef GNUSTEP
  unsigned char r, g, b, a;
  if (cocoaGetByteRGBAFromNSColor([NSColor windowBackgroundColor], &r, &g, &b, &a))
    return (0.2126 * r + 0.7152 * g + 0.0722 * b) < 128.0? 1: 0;
  return 0;
#else
  /* the NSApp appearance follows APPEARANCE, the user default does not */
  NSString* style = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
  return (style && [style rangeOfString:@"Dark"].location != NSNotFound)? 1: 0;
#endif
}

IUP_SDK_API int iupdrvIsSystemDarkMode(void)
{
  @autoreleasepool {
    return cocoaIsSystemDarkMode();
  }
}

static void cocoaSetAppearance(int appearance)
{
#ifndef GNUSTEP
  if (appearance == IUP_APPEARANCE_DARK)
    [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
  else if (appearance == IUP_APPEARANCE_LIGHT)
    [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameAqua]];
  else
    [NSApp setAppearance:nil];
#endif

  iupcocoaSetGlobalColors();

#ifdef GNUSTEP
  {
    int dark, forced;

    if (appearance == IUP_APPEARANCE_SYSTEM)
      dark = iupUnixPortalGetDarkMode(iupdrvIsSystemDarkMode());
    else
      dark = (appearance == IUP_APPEARANCE_DARK)? 1: 0;

    forced = (iupdrvIsSystemDarkMode() != dark);
    iupGlobalSetPaletteForced(forced);

    if (forced)
      iupGlobalSetAppearanceColors(dark);
  }
#endif
}

IUP_SDK_API void iupdrvSetAppearance(int appearance)
{
  @autoreleasepool {
    cocoaSetAppearance(appearance);
  }
}

static void cocoaUpdateGlobalColors(void)
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

  {
    NSColor* accent = nil;
    if ([NSColor respondsToSelector:@selector(controlAccentColor)])
      accent = [NSColor performSelector:@selector(controlAccentColor)];
    if (!accent)
      accent = [NSColor selectedControlColor];
    if (cocoaGetByteRGBAFromNSColor(accent, &r, &g, &b, &a))
      iupGlobalSetDefaultColorAttrib("ACCENTCOLOR", r, g, b);
  }
#ifdef GNUSTEP
  iupUnixPortalSetAccentColor();
#endif

  if (cocoaGetByteRGBAFromNSColor([NSColor linkColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", r, g, b);

  if (cocoaGetByteRGBAFromNSColor([NSColor windowBackgroundColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", r, g, b);
  if (cocoaGetByteRGBAFromNSColor([NSColor labelColor], &r, &g, &b, &a))
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", r, g, b);
}

IUP_DRV_API void iupcocoaSetGlobalColors(void)
{
#ifdef GNUSTEP
  cocoaUpdateGlobalColors();
#else
  /* the dynamic NSColors resolve against the drawing appearance, not the one set on NSApp */
  [[NSApp effectiveAppearance] performAsCurrentDrawingAppearance:^{ cocoaUpdateGlobalColors(); }];
#endif
}

static int cocoaOpen(void)
{

#ifdef GNUSTEP
  /* Seed NSFont defaults before sharedApplication. */
  {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:@{@"NSScrollViewInterfaceStyle": @"NSMacintoshInterfaceStyle"}];
    if (![defaults stringForKey:@"NSFont"])
    {
      NSArray* regular_keys = @[
        @"NSFont", @"NSUserFont", @"NSSystemFont", @"NSLabelFont",
        @"NSMessageFont", @"NSPaletteFont", @"NSTitleBarFont",
        @"NSToolTipsFont", @"NSControlContentFont", @"NSMenuFont"
      ];
      NSMutableDictionary* seeds = [NSMutableDictionary dictionary];
      for (NSString* k in regular_keys)
      {
        [seeds setObject:@"DejaVuSans" forKey:k];
      }
      [seeds setObject:@"DejaVuSans-Bold" forKey:@"NSBoldFont"];
      [seeds setObject:@"DejaVuSans-Bold" forKey:@"NSBoldSystemFont"];
      [seeds setObject:@"DejaVuSansMono-Book" forKey:@"NSUserFixedPitchFont"];
      [defaults registerDefaults:seeds];
    }
  }
#endif

  [NSApplication sharedApplication];

#ifdef GNUSTEP
  {
    const char* theme_env = getenv("IUP_GNUSTEPTHEME");
    if (theme_env && theme_env[0])
      iupcocoaGnustepSetTheme(theme_env);
  }

  /* fontWithName:size: is silent on a miss, so verify the seeds once the backend is up */
  {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* current = [defaults stringForKey:@"NSFont"];
    if (!current || ![NSFont fontWithName:current size:12])
    {
      NSArray* regular_candidates = @[
        @"DejaVuSans", @"BitstreamVeraSans-Roman", @"LiberationSans",
        @"FreeSans", @"NimbusSans-Regular", @"NimbusSansL-Regu"
      ];
      NSArray* bold_candidates = @[
        @"DejaVuSans-Bold", @"BitstreamVeraSans-Bold", @"LiberationSans-Bold",
        @"FreeSansBold", @"NimbusSans-Bold", @"NimbusSansL-Bold"
      ];
      NSArray* mono_candidates = @[
        @"DejaVuSansMono", @"BitstreamVeraSansMono-Roman", @"LiberationMono",
        @"FreeMono", @"NimbusMono-Regular", @"NimbusMonoL-Regu", @"Courier"
      ];
      NSString* chosen_regular = nil;
      NSString* chosen_bold = nil;
      NSString* chosen_mono = nil;
      for (NSString* n in regular_candidates) { if ([NSFont fontWithName:n size:12]) { chosen_regular = n; break; } }
      for (NSString* n in bold_candidates)    { if ([NSFont fontWithName:n size:12]) { chosen_bold = n; break; } }
      for (NSString* n in mono_candidates)    { if ([NSFont fontWithName:n size:12]) { chosen_mono = n; break; } }
      if (chosen_regular)
      {
        iupcocoaGnustepSetPrimaryDefault(@"NSFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSUserFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSSystemFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSLabelFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSMessageFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSPaletteFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSTitleBarFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSToolTipsFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSControlContentFont", chosen_regular);
        iupcocoaGnustepSetPrimaryDefault(@"NSMenuFont", chosen_regular);
      }
      if (chosen_bold)
      {
        iupcocoaGnustepSetPrimaryDefault(@"NSBoldFont", chosen_bold);
        iupcocoaGnustepSetPrimaryDefault(@"NSBoldSystemFont", chosen_bold);
      }
      if (chosen_mono)
      {
        iupcocoaGnustepSetPrimaryDefault(@"NSUserFixedPitchFont", chosen_mono);
      }
    }
  }
#endif

  if ([NSApp activationPolicy] == NSApplicationActivationPolicyProhibited)
  {
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  }

#ifdef GNUSTEP
  if (![NSApp delegate])
  {
    cocoa_gnustep_app_delegate = [[IupGnustepApplicationDelegate alloc] init];
    [NSApp setDelegate:cocoa_gnustep_app_delegate];
  }
#endif

  /* [NSApp run] would do this, but IUP runs its own event loop */
  [NSApp finishLaunching];

  iupcocoaEnsureDefaultApplicationMenu();

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

#ifdef GNUSTEP
  {
    int fd = iupUnixPortalSettingsOpen();
    if (fd >= 0)
      cocoa_gnustep_portal_settings = [[IupGnustepPortalSettings alloc] initWithFileDescriptor:fd];
  }

  cocoaSetAppearance(IUP_APPEARANCE_SYSTEM);
#else
  iupGlobalSetAppearanceNative(1);
  iupcocoaSetGlobalColors();
#endif
  IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");

  IupSetInt(NULL, "UTF8MODE", 1);

  return IUP_NOERROR;
}

static int cocoaSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  @autoreleasepool {
    return cocoaSetGlobalAppIDAttrib(value);
  }
}

IUP_SDK_API int iupdrvOpen(int* argc, char*** argv)
{
  (void)argc;
  (void)argv;

  @autoreleasepool {
    return cocoaOpen();
  }
}

static int cocoaSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  NSString* appName = [NSString stringWithUTF8String:value];
  [[NSProcessInfo processInfo] setProcessName:appName];
  appname_set = 1;
  return 1;
}

IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  @autoreleasepool {
    return cocoaSetGlobalAppNameAttrib(value);
  }
}

IUP_SDK_API void iupdrvClose(void)
{
  @autoreleasepool {
    iupcocoaMenuCleanupApplicationMenu();
  }

#ifdef GNUSTEP
  if (cocoa_gnustep_portal_settings)
  {
    [cocoa_gnustep_portal_settings stop];
    [cocoa_gnustep_portal_settings release];
    cocoa_gnustep_portal_settings = nil;
  }
  iupUnixPortalSettingsClose();

  if (cocoa_gnustep_app_delegate)
  {
    if ([NSApp delegate] == cocoa_gnustep_app_delegate)
      [NSApp setDelegate:nil];
    [cocoa_gnustep_app_delegate release];
    cocoa_gnustep_app_delegate = nil;
  }
#endif
}

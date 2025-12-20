/** \file
 * \brief macOS Desktop Notifications
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>
#import <ImageIO/ImageIO.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_notify.h"
#include "iup_tray.h"
#include "iup_image.h"

#define IUPCOCOA_NOTIFY_MAX_ICON_SIZE 256

@interface IupCocoaNotifyDelegate : NSObject <UNUserNotificationCenterDelegate>
@property (nonatomic, assign) Ihandle* ih;
@end

@implementation IupCocoaNotifyDelegate

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{
  UNNotificationPresentationOptions options = UNNotificationPresentationOptionSound;
  NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];

  if (version.majorVersion >= 11)
    options |= (1 << 4);  /* UNNotificationPresentationOptionBanner, macOS 11+ */
  else
    options |= (1 << 2);  /* UNNotificationPresentationOptionAlert, macOS 10.x */

  completionHandler(options);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void (^)(void))completionHandler
{
  NSString* actionId = response.actionIdentifier;
  Ihandle* ih = self.ih;
  int action = 0;

  if (!ih)
  {
    completionHandler();
    return;
  }

  if ([actionId isEqualToString:UNNotificationDefaultActionIdentifier])
  {
    action = 0;
  }
  else if ([actionId isEqualToString:UNNotificationDismissActionIdentifier])
  {
    IFni close_cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
    if (close_cb)
    {
      int ret = close_cb(ih, 2);
      if (ret == IUP_CLOSE)
        IupExitLoop();
    }
    completionHandler();
    return;
  }
  else if ([actionId isEqualToString:@"action1"])
  {
    action = 1;
  }
  else if ([actionId isEqualToString:@"action2"])
  {
    action = 2;
  }
  else if ([actionId isEqualToString:@"action3"])
  {
    action = 3;
  }
  else if ([actionId isEqualToString:@"action4"])
  {
    action = 4;
  }

  IFni notify_cb = (IFni)IupGetCallback(ih, "NOTIFY_CB");
  if (notify_cb)
  {
    int ret = notify_cb(ih, action);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  completionHandler();
}

@end

typedef struct _IupCocoaNotify {
  Ihandle* ih;
  NSString* identifier;
  IupCocoaNotifyDelegate* delegate;
  int permission_status;
  NSString* tempImagePath;
} IupCocoaNotify;

static NSString* cocoaNotifySaveImageToTemp(int width, int height, unsigned char* pixels)
{
  NSString* tempPath = nil;
  CFMutableDataRef data = NULL;
  CGImageDestinationRef dest = NULL;
  CGColorSpaceRef colorSpace = NULL;
  CGDataProviderRef provider = NULL;
  CGImageRef image = NULL;
  unsigned char* rgbaPixels = NULL;
  int x, y;

  rgbaPixels = (unsigned char*)malloc(width * height * 4);
  if (!rgbaPixels)
    return nil;

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      int offset = (y * width + x) * 4;
      unsigned char a = pixels[offset + 0];
      unsigned char r = pixels[offset + 1];
      unsigned char g = pixels[offset + 2];
      unsigned char b = pixels[offset + 3];

      rgbaPixels[offset + 0] = r;
      rgbaPixels[offset + 1] = g;
      rgbaPixels[offset + 2] = b;
      rgbaPixels[offset + 3] = a;
    }
  }

  colorSpace = CGColorSpaceCreateDeviceRGB();
  if (!colorSpace)
  {
    free(rgbaPixels);
    return nil;
  }

  provider = CGDataProviderCreateWithData(NULL, rgbaPixels, width * height * 4, NULL);
  if (!provider)
  {
    CGColorSpaceRelease(colorSpace);
    free(rgbaPixels);
    return nil;
  }

  image = CGImageCreate(width, height, 8, 32, width * 4, colorSpace,
      kCGBitmapByteOrderDefault | kCGImageAlphaLast,
      provider, NULL, false, kCGRenderingIntentDefault);

  CGDataProviderRelease(provider);
  CGColorSpaceRelease(colorSpace);

  if (!image)
  {
    free(rgbaPixels);
    return nil;
  }

  tempPath = [NSTemporaryDirectory() stringByAppendingPathComponent:
      [[NSUUID UUID] UUIDString]];
  tempPath = [tempPath stringByAppendingPathExtension:@"png"];

  data = CFDataCreateMutable(NULL, 0);
  if (!data)
  {
    CGImageRelease(image);
    free(rgbaPixels);
    return nil;
  }

  dest = CGImageDestinationCreateWithData(data, CFSTR("public.png"), 1, NULL);
  if (!dest)
  {
    CFRelease(data);
    CGImageRelease(image);
    free(rgbaPixels);
    return nil;
  }

  CGImageDestinationAddImage(dest, image, NULL);

  if (CGImageDestinationFinalize(dest))
  {
    if ([(__bridge NSData*)data writeToFile:tempPath atomically:YES])
    {
      tempPath = [tempPath retain];
    }
    else
    {
      tempPath = nil;
    }
  }
  else
  {
    tempPath = nil;
  }

  CFRelease(dest);
  CFRelease(data);
  CGImageRelease(image);
  free(rgbaPixels);

  return tempPath;
}

static int iupCocoaNotifyIsSupported(void)
{
  static int checked = 0;
  static int supported = 0;

  if (checked)
    return supported;

  checked = 1;

  NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
  if (version.majorVersion < 10 || (version.majorVersion == 10 && version.minorVersion < 15))
    return 0;

  if (NSClassFromString(@"UNUserNotificationCenter") == nil)
    return 0;

  supported = 1;
  return 1;
}

static void cocoaNotifyRegisterCategory(Ihandle* ih)
{
  NSMutableArray* actions = [NSMutableArray array];

  const char* action1 = IupGetAttribute(ih, "ACTION1");
  const char* action2 = IupGetAttribute(ih, "ACTION2");
  const char* action3 = IupGetAttribute(ih, "ACTION3");
  const char* action4 = IupGetAttribute(ih, "ACTION4");

  if (action1)
  {
    UNNotificationAction* act = [UNNotificationAction
        actionWithIdentifier:@"action1"
                       title:[NSString stringWithUTF8String:action1]
                     options:UNNotificationActionOptionForeground];
    [actions addObject:act];
  }
  if (action2)
  {
    UNNotificationAction* act = [UNNotificationAction
        actionWithIdentifier:@"action2"
                       title:[NSString stringWithUTF8String:action2]
                     options:UNNotificationActionOptionForeground];
    [actions addObject:act];
  }
  if (action3)
  {
    UNNotificationAction* act = [UNNotificationAction
        actionWithIdentifier:@"action3"
                       title:[NSString stringWithUTF8String:action3]
                     options:UNNotificationActionOptionForeground];
    [actions addObject:act];
  }
  if (action4)
  {
    UNNotificationAction* act = [UNNotificationAction
        actionWithIdentifier:@"action4"
                       title:[NSString stringWithUTF8String:action4]
                     options:UNNotificationActionOptionForeground];
    [actions addObject:act];
  }

  UNNotificationCategory* category = [UNNotificationCategory
      categoryWithIdentifier:@"IUP_NOTIFY_CATEGORY"
                     actions:actions
           intentIdentifiers:@[]
                     options:UNNotificationCategoryOptionCustomDismissAction];

  [[UNUserNotificationCenter currentNotificationCenter]
      setNotificationCategories:[NSSet setWithObject:category]];
}

static IupCocoaNotify* cocoaNotifyCreate(Ihandle* ih)
{
  IupCocoaNotify* notify = (IupCocoaNotify*)calloc(1, sizeof(IupCocoaNotify));
  notify->ih = ih;
  notify->delegate = [[IupCocoaNotifyDelegate alloc] init];
  notify->delegate.ih = ih;
  notify->permission_status = -1;

  UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
  center.delegate = notify->delegate;

  iupAttribSet(ih, "_IUPCOCOA_NOTIFY", (char*)notify);

  return notify;
}

static void cocoaNotifyDestroy(IupCocoaNotify* notify)
{
  if (!notify)
    return;

  if (notify->identifier)
  {
    [[UNUserNotificationCenter currentNotificationCenter]
        removeDeliveredNotificationsWithIdentifiers:@[notify->identifier]];
    [notify->identifier release];
    notify->identifier = nil;
  }

  if (notify->tempImagePath)
  {
    [[NSFileManager defaultManager] removeItemAtPath:notify->tempImagePath error:nil];
    [notify->tempImagePath release];
    notify->tempImagePath = nil;
  }

  if (notify->delegate)
  {
    [notify->delegate release];
    notify->delegate = nil;
  }

  free(notify);
}

static char* cocoaNotifyGetPermissionAttrib(Ihandle* ih)
{
  __block int status = -1;
  dispatch_semaphore_t sem;

  if (!iupCocoaNotifyIsSupported())
    return "DENIED";

  sem = dispatch_semaphore_create(0);

  [[UNUserNotificationCenter currentNotificationCenter]
      getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
    switch (settings.authorizationStatus)
    {
      case UNAuthorizationStatusAuthorized:
      case UNAuthorizationStatusProvisional:
        status = 1;
        break;
      case UNAuthorizationStatusDenied:
        status = 0;
        break;
      default:
        status = -1;
        break;
    }
    dispatch_semaphore_signal(sem);
  }];

  dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC));

  if (status == 1)
    return "GRANTED";
  else if (status == 0)
    return "DENIED";
  else
    return "NOTDETERMINED";
}

static int cocoaNotifySetRequestPermissionAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrBoolean(value))
    return 0;

  if (!iupCocoaNotifyIsSupported())
    return 0;

  [[UNUserNotificationCenter currentNotificationCenter]
      requestAuthorizationWithOptions:(UNAuthorizationOptionAlert |
                                       UNAuthorizationOptionSound |
                                       UNAuthorizationOptionBadge)
                    completionHandler:^(BOOL granted, NSError* error) {
    IupCocoaNotify* notify = (IupCocoaNotify*)iupAttribGet(ih, "_IUPCOCOA_NOTIFY");
    if (notify)
      notify->permission_status = granted ? 1 : 0;
  }];

  return 0;
}

int iupdrvNotifyShow(Ihandle* ih)
{
  IupCocoaNotify* notify;
  __block UNAuthorizationStatus authStatus = UNAuthorizationStatusNotDetermined;
  dispatch_semaphore_t sem;

  if (!iupCocoaNotifyIsSupported())
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, (char*)"Notifications require macOS 10.15 or later");
    return 0;
  }

  if ([[NSBundle mainBundle] bundleIdentifier] == nil)
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, (char*)"Notifications require a bundled application with bundle identifier");
    return 0;
  }

  notify = (IupCocoaNotify*)iupAttribGet(ih, "_IUPCOCOA_NOTIFY");

  if (!notify)
    notify = cocoaNotifyCreate(ih);

  sem = dispatch_semaphore_create(0);

  [[UNUserNotificationCenter currentNotificationCenter]
      getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
    authStatus = settings.authorizationStatus;
    dispatch_semaphore_signal(sem);
  }];

  dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC));

  if (authStatus == UNAuthorizationStatusNotDetermined)
  {
    __block BOOL granted = NO;
    __block NSString* errorMsg = nil;
    sem = dispatch_semaphore_create(0);

    [[UNUserNotificationCenter currentNotificationCenter]
        requestAuthorizationWithOptions:(UNAuthorizationOptionAlert |
                                         UNAuthorizationOptionSound |
                                         UNAuthorizationOptionBadge)
                      completionHandler:^(BOOL g, NSError* error) {
      granted = g;
      if (error)
        errorMsg = [[error localizedDescription] copy];
      dispatch_semaphore_signal(sem);
    }];

    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC));

    if (!granted)
    {
      IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
      if (error_cb)
      {
        if (errorMsg)
          error_cb(ih, (char*)[errorMsg UTF8String]);
        else
          error_cb(ih, (char*)"Notification permission denied by user");
      }
      [errorMsg release];
      return 0;
    }
  }
  else if (authStatus == UNAuthorizationStatusDenied)
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, (char*)"Notification permission denied. Enable in System Preferences.");
    return 0;
  }

  cocoaNotifyRegisterCategory(ih);

  UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];

  const char* title = IupGetAttribute(ih, "TITLE");
  const char* body = IupGetAttribute(ih, "BODY");
  const char* subtitle = IupGetAttribute(ih, "SUBTITLE");
  const char* icon = IupGetAttribute(ih, "ICON");
  const char* threadId = IupGetAttribute(ih, "THREADID");
  int silent = IupGetInt(ih, "SILENT");

  if (title)
    content.title = [NSString stringWithUTF8String:title];
  if (body)
    content.body = [NSString stringWithUTF8String:body];
  if (subtitle)
    content.subtitle = [NSString stringWithUTF8String:subtitle];
  if (threadId)
    content.threadIdentifier = [NSString stringWithUTF8String:threadId];

  if (!silent)
    content.sound = [UNNotificationSound defaultSound];

  if (notify->tempImagePath)
  {
    [[NSFileManager defaultManager] removeItemAtPath:notify->tempImagePath error:nil];
    [notify->tempImagePath release];
    notify->tempImagePath = nil;
  }

  if (icon && icon[0])
  {
    int img_width, img_height;
    unsigned char* pixels = NULL;

    if (iupdrvGetIconPixels(ih, icon, &img_width, &img_height, &pixels))
    {
      unsigned char* use_pixels = pixels;
      int use_width = img_width;
      int use_height = img_height;
      unsigned char* scaled_pixels = NULL;

      if (img_width > IUPCOCOA_NOTIFY_MAX_ICON_SIZE || img_height > IUPCOCOA_NOTIFY_MAX_ICON_SIZE)
      {
        double scale;
        if (img_width > img_height)
          scale = (double)IUPCOCOA_NOTIFY_MAX_ICON_SIZE / img_width;
        else
          scale = (double)IUPCOCOA_NOTIFY_MAX_ICON_SIZE / img_height;

        use_width = (int)(img_width * scale);
        use_height = (int)(img_height * scale);
        if (use_width < 1) use_width = 1;
        if (use_height < 1) use_height = 1;

        scaled_pixels = (unsigned char*)malloc(use_width * use_height * 4);
        if (scaled_pixels)
        {
          iupImageResizeRGBA(img_width, img_height, pixels, use_width, use_height, scaled_pixels, 4);
          use_pixels = scaled_pixels;
        }
        else
        {
          use_width = img_width;
          use_height = img_height;
        }
      }

      notify->tempImagePath = cocoaNotifySaveImageToTemp(use_width, use_height, use_pixels);

      if (scaled_pixels)
        free(scaled_pixels);
      free(pixels);

      if (notify->tempImagePath)
      {
        NSURL* imageURL = [NSURL fileURLWithPath:notify->tempImagePath];
        NSError* attachError = nil;
        UNNotificationAttachment* attachment =
            [UNNotificationAttachment attachmentWithIdentifier:@"image"
                                                           URL:imageURL
                                                       options:nil
                                                         error:&attachError];
        if (attachment && !attachError)
          content.attachments = @[attachment];
      }
    }
  }

  content.categoryIdentifier = @"IUP_NOTIFY_CATEGORY";

  if (notify->identifier)
  {
    [[UNUserNotificationCenter currentNotificationCenter]
        removeDeliveredNotificationsWithIdentifiers:@[notify->identifier]];
    [notify->identifier release];
  }

  notify->identifier = [[[NSUUID UUID] UUIDString] retain];

  UNNotificationRequest* request =
      [UNNotificationRequest requestWithIdentifier:notify->identifier
                                           content:content
                                           trigger:nil];

  __block BOOL success = NO;
  dispatch_semaphore_t showSem = dispatch_semaphore_create(0);

  [[UNUserNotificationCenter currentNotificationCenter]
      addNotificationRequest:request
       withCompletionHandler:^(NSError* error) {
    if (error)
    {
      IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
      if (error_cb)
        error_cb(ih, (char*)[[error localizedDescription] UTF8String]);
      success = NO;
    }
    else
    {
      success = YES;
    }
    dispatch_semaphore_signal(showSem);
  }];

  dispatch_semaphore_wait(showSem, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC));

  [content release];

  return success ? 1 : 0;
}

int iupdrvNotifyClose(Ihandle* ih)
{
  IupCocoaNotify* notify;

  if (!iupCocoaNotifyIsSupported())
    return 0;

  notify = (IupCocoaNotify*)iupAttribGet(ih, "_IUPCOCOA_NOTIFY");

  if (notify && notify->identifier)
  {
    [[UNUserNotificationCenter currentNotificationCenter]
        removeDeliveredNotificationsWithIdentifiers:@[notify->identifier]];
    return 1;
  }

  return 0;
}

void iupdrvNotifyDestroy(Ihandle* ih)
{
  IupCocoaNotify* notify = (IupCocoaNotify*)iupAttribGet(ih, "_IUPCOCOA_NOTIFY");

  if (notify)
  {
    cocoaNotifyDestroy(notify);
    iupAttribSet(ih, "_IUPCOCOA_NOTIFY", NULL);
  }
}

int iupdrvNotifyIsAvailable(void)
{
  if (!iupCocoaNotifyIsSupported())
    return 0;
  if ([[NSBundle mainBundle] bundleIdentifier] == nil)
    return 0;
  return 1;
}

void iupdrvNotifyInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "SUBTITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PERMISSION", cocoaNotifyGetPermissionAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REQUESTPERMISSION", NULL, cocoaNotifySetRequestPermissionAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "THREADID", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TIMEOUT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}

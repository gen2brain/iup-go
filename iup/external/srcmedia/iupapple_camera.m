/** \file
 * \brief Camera control, AVFoundation capture (macOS and iOS)
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#include "iupcocoatouch_drv.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_camera.h"

@interface IupAppleCameraDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
  Ihandle* ih;
  AVCaptureSession* session;
  dispatch_queue_t queue;
  unsigned char* rgb;
  unsigned char* rotated;
  int width, height, rotation;
}
- (id)initWithHandle:(Ihandle*)handle;
- (int)startDevice:(AVCaptureDevice*)device width:(int*)w height:(int*)h fps:(int*)f;
- (void)stop;
@end

#if !TARGET_OS_IPHONE
static void iappleCameraAddType(NSMutableArray* types, CFStringRef name)
{
  CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.avfoundation"));
  NSString* const* type = bundle ? (NSString* const*)CFBundleGetDataPointerForName(bundle, name) : NULL;
  if (type && *type)
    [types addObject:*type];
}
#endif

static NSArray* iappleCameraDevices(void)
{
  NSMutableArray* types = [NSMutableArray arrayWithObject:AVCaptureDeviceTypeBuiltInWideAngleCamera];
#if !TARGET_OS_IPHONE
  iappleCameraAddType(types, CFSTR("AVCaptureDeviceTypeExternal"));
  iappleCameraAddType(types, CFSTR("AVCaptureDeviceTypeContinuityCamera"));
#endif
  AVCaptureDeviceDiscoverySession* discovery = [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:types
                                                                                                    mediaType:AVMediaTypeVideo
                                                                                                     position:AVCaptureDevicePositionUnspecified];
  return discovery.devices;
}

int iupdrvCameraIsAvailable(void)
{
  return 1;
}

int iupdrvCameraGetDeviceCount(void)
{
  return (int)[iappleCameraDevices() count];
}

char* iupdrvCameraGetDeviceName(int index)
{
  NSArray* devices = iappleCameraDevices();
  if (index < 0 || index >= (int)[devices count])
    return NULL;
  return iupStrReturnStr([[[devices objectAtIndex:index] localizedName] UTF8String]);
}

char* iupdrvCameraGetPermission(Ihandle* ih)
{
  (void)ih;
  switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo])
  {
  case AVAuthorizationStatusAuthorized: return "GRANTED";
  case AVAuthorizationStatusNotDetermined: return "PROMPT";
  default: return "DENIED";
  }
}

static long iappleCameraScore(AVCaptureDeviceFormat* format, int req_width, int req_height, int req_fps)
{
  CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
  FourCharCode subtype = CMFormatDescriptionGetMediaSubType(format.formatDescription);
  double max_fps = 0;
  long score;

  for (AVFrameRateRange* range in format.videoSupportedFrameRateRanges)
  {
    if (range.maxFrameRate > max_fps)
      max_fps = range.maxFrameRate;
  }

  score = labs((long)dims.width * (long)dims.height - (long)req_width * (long)req_height) * 4;
  if (max_fps > 0 && max_fps < req_fps)
    score += (long)(req_fps - max_fps) * 100000;
  if (subtype != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange && subtype != kCVPixelFormatType_420YpCbCr8BiPlanarFullRange && subtype != kCVPixelFormatType_422YpCbCr8)
    score += 1;
  return score;
}

@implementation IupAppleCameraDelegate

- (id)initWithHandle:(Ihandle*)handle
{
  self = [super init];
  if (self)
  {
    ih = handle;
    queue = dispatch_queue_create("iup.camera", DISPATCH_QUEUE_SERIAL);
  }
  return self;
}

- (void)dealloc
{
  free(rgb);
  free(rotated);
  [queue release];
  [session release];
  [super dealloc];
}

- (int)startDevice:(AVCaptureDevice*)device width:(int*)w height:(int*)h fps:(int*)f
{
  NSError* error = nil;
  AVCaptureDeviceFormat* best = nil;
  long best_score = -1;
  AVCaptureDeviceInput* input;
  AVCaptureVideoDataOutput* output;
  CMVideoDimensions dims;

  for (AVCaptureDeviceFormat* format in device.formats)
  {
    long score = iappleCameraScore(format, *w, *h, *f);
    if (best_score < 0 || score < best_score)
    {
      best_score = score;
      best = format;
    }
  }
  if (!best)
    return 0;

  dims = CMVideoFormatDescriptionGetDimensions(best.formatDescription);
  width = dims.width;
  height = dims.height;

#if TARGET_OS_IPHONE
  {
    UIWindow* window = iupCocoaTouchFindCurrentWindow();
    UIInterfaceOrientation orientation = (window && window.windowScene) ? window.windowScene.interfaceOrientation : UIInterfaceOrientationPortrait;
    switch (orientation)
    {
    case UIInterfaceOrientationLandscapeRight: rotation = 0; break;
    case UIInterfaceOrientationLandscapeLeft: rotation = 180; break;
    case UIInterfaceOrientationPortraitUpsideDown: rotation = 270; break;
    default: rotation = 90; break;
    }
    if (device.position == AVCaptureDevicePositionFront && (rotation == 90 || rotation == 270))
      rotation = 360 - rotation;
  }
#endif

  input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
  if (!input)
    return 0;

  output = [[[AVCaptureVideoDataOutput alloc] init] autorelease];
  output.videoSettings = @{ (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA) };
  output.alwaysDiscardsLateVideoFrames = YES;
  [output setSampleBufferDelegate:self queue:queue];

  session = [[AVCaptureSession alloc] init];
  [session beginConfiguration];
  if (![session canAddInput:input] || ![session canAddOutput:output])
  {
    [session commitConfiguration];
    return 0;
  }
  [session addInput:input];
  [session addOutput:output];

  if ([device lockForConfiguration:&error])
  {
    AVFrameRateRange* chosen = nil;
    device.activeFormat = best;
    for (AVFrameRateRange* range in best.videoSupportedFrameRateRanges)
    {
      if (!chosen || fabs(range.maxFrameRate - *f) < fabs(chosen.maxFrameRate - *f))
        chosen = range;
    }
    if (chosen)
    {
      if (*f > chosen.minFrameRate + 0.01 && *f < chosen.maxFrameRate - 0.01)
      {
        device.activeVideoMinFrameDuration = CMTimeMake(1, *f);
        device.activeVideoMaxFrameDuration = CMTimeMake(1, *f);
      }
      else
      {
        device.activeVideoMinFrameDuration = chosen.minFrameDuration;
        device.activeVideoMaxFrameDuration = chosen.minFrameDuration;
        *f = (int)(chosen.maxFrameRate + 0.5);
      }
    }
    [session commitConfiguration];
    [session startRunning];
    [device unlockForConfiguration];
  }
  else
  {
    [session commitConfiguration];
    [session startRunning];
  }

  if (rotation == 90 || rotation == 270)
  {
    *w = height;
    *h = width;
  }
  else
  {
    *w = width;
    *h = height;
  }
  return 1;
}

- (void)stop
{
  [session stopRunning];
  dispatch_sync(queue, ^{});
}

- (void)captureOutput:(AVCaptureOutput*)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection*)connection
{
  CVImageBufferRef image = CMSampleBufferGetImageBuffer(sampleBuffer);
  size_t w, h, stride, x, y;
  const unsigned char* base;
  unsigned char* dst;
  (void)captureOutput; (void)connection;

  if (!image || CVPixelBufferGetPixelFormatType(image) != kCVPixelFormatType_32BGRA)
    return;

  CVPixelBufferLockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
  w = CVPixelBufferGetWidth(image);
  h = CVPixelBufferGetHeight(image);
  stride = CVPixelBufferGetBytesPerRow(image);
  base = (const unsigned char*)CVPixelBufferGetBaseAddress(image);

  if ((int)w != width || (int)h != height || !rgb)
  {
    free(rgb);
    free(rotated);
    rgb = (unsigned char*)malloc(w * h * 3);
    rotated = rotation ? (unsigned char*)malloc(w * h * 3) : NULL;
    width = (int)w;
    height = (int)h;
  }

  dst = rgb;
  for (y = 0; y < h; y++)
  {
    const unsigned char* row = base + y * stride;
    for (x = 0; x < w; x++)
    {
      dst[0] = row[2]; dst[1] = row[1]; dst[2] = row[0];
      row += 4;
      dst += 3;
    }
  }
  CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);

  if (!rotation)
    iupCameraFrame(ih, rgb, width, height);
  else
  {
    iupCameraRotate(rgb, width, height, rotation, rotated);
    if (rotation == 180)
      iupCameraFrame(ih, rotated, width, height);
    else
      iupCameraFrame(ih, rotated, height, width);
  }
}

@end

static int iappleCameraStart(Ihandle* ih, int device_index, int* width, int* height, int* fps)
{
  NSArray* devices = iappleCameraDevices();
  IupAppleCameraDelegate* delegate;

  if (device_index < 0 || device_index >= (int)[devices count])
  {
    iupCameraError(ih, "Camera not found");
    return 0;
  }

  delegate = [[IupAppleCameraDelegate alloc] initWithHandle:ih];
  if (![delegate startDevice:[devices objectAtIndex:device_index] width:width height:height fps:fps])
  {
    [delegate release];
    iupCameraError(ih, "Cannot open camera");
    return 0;
  }

  iupAttribSet(ih, "_IUP_CAMERA", (char*)delegate);
  return 1;
}

int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

  if (status == AVAuthorizationStatusNotDetermined)
  {
    int req_width = *width, req_height = *height, req_fps = *fps;
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
      dispatch_async(dispatch_get_main_queue(), ^{
        if (!iupObjectCheck(ih))
          return;
        iupCameraPermission(ih, granted ? 1 : 0);
        if (granted && iupAttribGet(ih, "_IUP_CAMERA_PENDING"))
        {
          int w = req_width, h = req_height, f = req_fps;
          iupAttribSet(ih, "_IUP_CAMERA_PENDING", NULL);
          iappleCameraStart(ih, device, &w, &h, &f);
        }
      });
    }];
    iupAttribSet(ih, "_IUP_CAMERA_PENDING", "1");
    return 1;
  }

  if (status != AVAuthorizationStatusAuthorized)
  {
    iupCameraError(ih, "Camera access denied");
    return 0;
  }

  return iappleCameraStart(ih, device, width, height, fps);
}

void iupdrvCameraStop(Ihandle* ih)
{
  IupAppleCameraDelegate* delegate = (IupAppleCameraDelegate*)iupAttribGet(ih, "_IUP_CAMERA");
  iupAttribSet(ih, "_IUP_CAMERA_PENDING", NULL);
  if (!delegate)
    return;

  [delegate stop];
  [delegate release];
  iupAttribSet(ih, "_IUP_CAMERA", NULL);
}

void iupdrvCameraInitClass(Iclass* ic)
{
  (void)ic;
}

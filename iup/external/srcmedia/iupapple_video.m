/** \file
 * \brief Video control, AVFoundation playback (macOS and iOS)
 *
 * See Copyright Notice in "iup.h"
 */

#import <TargetConditionals.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iup_video.h"
#include "iup_object.h"
#include "iup_str.h"

@interface IupAppleVideo : NSObject
{
@public
  IvideoSession* session;
  AVPlayer* player;
  AVPlayerItem* item;
  AVPlayerLayer* layer;
  AVPlayerItemVideoOutput* output;
  id endObserver;
  unsigned char* rgba;
  size_t capacity;
  int ended;
}
@end

@implementation IupAppleVideo
- (void)dealloc
{
  if (endObserver)
    [[NSNotificationCenter defaultCenter] removeObserver:endObserver];
  [endObserver release];
  [layer release];
  [output release];
  [item release];
  [player release];
  free(rgba);
  [super dealloc];
}
@end

static int iappleVideoAvailable(void)
{
  return NSClassFromString(@"AVPlayer") != Nil;
}

static CALayer* iappleVideoParent(Ihandle* ih)
{
#if TARGET_OS_IPHONE
  UIView* view = (UIView*)ih->handle;
  return view ? view.layer : nil;
#else
  NSView* view;
  if (!iupStrEqual(IupGetGlobal("DRIVER"), "Cocoa"))
    return nil;
  view = (NSView*)IupGetAttribute(ih, "NSVIEW");
  if (!view)
    return nil;
  [view setWantsLayer:YES];
  return view.layer;
#endif
}

static void iappleVideoLayout(IvideoSession* session, Ihandle* ih)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  CALayer* parent = iappleVideoParent(ih);
  if (context->layer && parent)
  {
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    context->layer.frame = parent.bounds;
    [CATransaction commit];
  }
}

static int iappleVideoOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  NSString* name = [NSString stringWithUTF8String:source];
  IupAppleVideo* context;
  CALayer* parent;
  NSURL* uri;
  if (!name)
  {
    iupVideoError(session, "Invalid source name");
    return 0;
  }
  uri = url ? [NSURL URLWithString:name] : [NSURL fileURLWithPath:[name stringByStandardizingPath]];
  if (!uri)
  {
    iupVideoError(session, "Invalid source URL");
    return 0;
  }
  context = [[IupAppleVideo alloc] init];
  if (!context)
    return 0;
  session->context = context;
  context->session = session;
  context->item = [[AVPlayerItem playerItemWithURL:uri] retain];
  context->player = [[AVPlayer playerWithPlayerItem:context->item] retain];
  if (!context->item || !context->player)
  {
    iupVideoError(session, "Cannot create AVPlayer");
    return 0;
  }
  context->player.actionAtItemEnd = AVPlayerActionAtItemEndPause;
  parent = iappleVideoParent(ih);
  if (parent)
  {
    context->layer = [[AVPlayerLayer playerLayerWithPlayer:context->player] retain];
    context->layer.videoGravity = AVLayerVideoGravityResizeAspect;
#if !TARGET_OS_IPHONE
    context->layer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
#endif
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    context->layer.frame = parent.bounds;
    [parent addSublayer:context->layer];
    [CATransaction commit];
  }
  else
  {
    NSDictionary* attributes = @{(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)};
    context->output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:attributes];
    if (!context->output)
    {
      iupVideoError(session, "Cannot create AVPlayer video output");
      return 0;
    }
    [context->item addOutput:context->output];
  }
  context->endObserver = [[[NSNotificationCenter defaultCenter]
    addObserverForName:AVPlayerItemDidPlayToEndTimeNotification object:context->item
    queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification* notification) {
      (void)notification;
      if (context->session)
        context->ended = 1;
    }] retain];
  return 1;
}

static void iappleVideoClose(IvideoSession* session)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  if (!context)
    return;
  context->session = NULL;
  if (context->endObserver)
    [[NSNotificationCenter defaultCenter] removeObserver:context->endObserver];
  [context->endObserver release];
  context->endObserver = nil;
  [context->player pause];
  [context->player replaceCurrentItemWithPlayerItem:nil];
  if (context->layer)
  {
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    context->layer.player = nil;
    [context->layer removeFromSuperlayer];
    [CATransaction commit];
  }
  [context release];
  session->context = NULL;
}

static void iappleVideoPlay(IvideoSession* session, int play)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  if (play)
    [context->player play];
  else
    [context->player pause];
}

static void iappleVideoSeek(IvideoSession* session, double position)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  if (position > (double)LLONG_MAX / 600)
    return;
  context->ended = 0;
  [context->player seekToTime:CMTimeMakeWithSeconds(position, 600) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

static void iappleVideoVolume(IvideoSession* session, int volume, int mute)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  context->player.volume = volume / 100.0f;
  context->player.muted = mute != 0;
}

static void iappleVideoCopy(IupAppleVideo* context, const unsigned char* pixels, size_t width, size_t height, size_t stride)
{
  size_t size = width * height * 4;
  size_t x, y;
  if (size > context->capacity)
  {
    unsigned char* allocation = (unsigned char*)realloc(context->rgba, size);
    if (!allocation)
    {
      iupVideoError(context->session, "Cannot allocate AVPlayer video frame");
      return;
    }
    context->rgba = allocation;
    context->capacity = size;
  }
  for (y = 0; y < height; y++)
  {
    const unsigned char* input = pixels + y * stride;
    unsigned char* output = context->rgba + y * width * 4;
    for (x = 0; x < width; x++)
    {
      output[x * 4] = input[x * 4 + 2];
      output[x * 4 + 1] = input[x * 4 + 1];
      output[x * 4 + 2] = input[x * 4];
      output[x * 4 + 3] = 255;
    }
  }
  iupVideoFrame(context->session, context->rgba, (int)width, (int)height, width * 4, 4);
}

static void iappleVideoFrame(IupAppleVideo* context)
{
  CMTime time = [context->output itemTimeForHostTime:CACurrentMediaTime()];
  CVPixelBufferRef buffer;
  if (![context->output hasNewPixelBufferForItemTime:time])
    return;
  buffer = [context->output copyPixelBufferForItemTime:time itemTimeForDisplay:NULL];
  if (!buffer)
    return;
  if (CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess)
  {
    size_t width = CVPixelBufferGetWidth(buffer);
    size_t height = CVPixelBufferGetHeight(buffer);
    size_t stride = CVPixelBufferGetBytesPerRow(buffer);
    const unsigned char* pixels = (const unsigned char*)CVPixelBufferGetBaseAddress(buffer);
    if (pixels && width && height && width <= INT_MAX / 4 && height <= INT_MAX / 4 / width && stride >= width * 4)
      iappleVideoCopy(context, pixels, width, height, stride);
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
  }
  CVPixelBufferRelease(buffer);
}

static void iappleVideoPoll(IvideoSession* session)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  IvideoInfo info;
  double position, duration;
  memset(&info, 0, sizeof(info));
  if (context->item.status == AVPlayerItemStatusFailed || context->player.status == AVPlayerStatusFailed)
  {
    NSError* error = context->item.error ?: context->player.error;
    iupVideoError(session, error ? error.localizedDescription.UTF8String : "AVPlayer playback failed");
    return;
  }
  info.ready = context->item.status == AVPlayerItemStatusReadyToPlay;
  if (context->player.timeControlStatus == AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate)
    info.state = IVIDEO_BUFFERING;
  else
    info.state = context->player.timeControlStatus == AVPlayerTimeControlStatusPlaying ? IVIDEO_PLAYING : IVIDEO_PAUSED;
  info.ended = context->ended;
  context->ended = 0;
  position = CMTimeGetSeconds(context->player.currentTime);
  duration = CMTimeGetSeconds(context->item.duration);
  info.position = isfinite(position) && position > 0 ? position : 0;
  info.duration = isfinite(duration) && duration > 0 ? duration : 0;
  if (info.ready)
  {
    CGSize size = context->item.presentationSize;
    info.seekable = context->item.seekableTimeRanges.count > 0;
    if (size.width > 0 && size.width <= INT_MAX && size.height > 0 && size.height <= INT_MAX)
    {
      info.width = (int)size.width;
      info.height = (int)size.height;
      info.aspect = size.width / size.height;
    }
    for (AVPlayerItemTrack* track in context->item.tracks)
    {
      if ([track.assetTrack.mediaType isEqualToString:AVMediaTypeVideo])
        info.video = 1;
      if ([track.assetTrack.mediaType isEqualToString:AVMediaTypeAudio])
        info.audio = 1;
    }
    if (context->output)
      iappleVideoFrame(context);
  }
  iupVideoInfo(session, &info);
}

static const IvideoBackend iappleVideoBackend = {"NATIVE", iappleVideoAvailable, iappleVideoOpen, iappleVideoClose, iappleVideoPlay, iappleVideoSeek, iappleVideoVolume, iappleVideoPoll, iappleVideoLayout};

const IvideoBackend* iupdrvVideoBackend(int index)
{
  return index == 0 ? &iappleVideoBackend : NULL;
}

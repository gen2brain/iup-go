/** \file
 * \brief Video control, AVFoundation playback (macOS and iOS)
 *
 * See Copyright Notice in "iup.h"
 */

#import <TargetConditionals.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreImage/CoreImage.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#import <CoreAudio/CoreAudio.h>
#endif

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iup_video.h"
#include "iup_object.h"
#include "iup_str.h"

@interface IupAppleVideo : NSObject <AVPlayerItemLegibleOutputPushDelegate>
{
@public
  IvideoSession* session;
  Ihandle* ih;
  AVPlayer* player;
  AVPlayerItem* item;
  AVPlayerLayer* layer;
  AVPlayerItemVideoOutput* output;
  int outputWidth, outputHeight;
  int rotate, rotateA, rotateB, rotateC, rotateD;
  unsigned char* rotated;
  size_t rotatedSize;
  AVPlayerItemLegibleOutput* legible;
  AVMediaSelectionGroup* subtitles;
  CATextLayer* caption;
  CGRect captionRect;
  id endObserver;
  int ended;
  NSMutableData* adjust;
  int adjustPending;
  int restoring;
  CMTime restoreTime;
}
@end

@implementation IupAppleVideo
- (void)legibleOutput:(AVPlayerItemLegibleOutput*)sender didOutputAttributedStrings:(NSArray<NSAttributedString*>*)strings nativeSampleBuffers:(NSArray*)samples forItemTime:(CMTime)time
{
  NSMutableArray* lines;
  (void)sender;
  (void)samples;
  (void)time;
  if (!session)
    return;
  lines = [NSMutableArray array];
  for (NSAttributedString* value in strings)
    [lines addObject:value.string];
  iupVideoSubtitleCue(session, [[lines componentsJoinedByString:@"\n"] UTF8String]);
}
- (void)outputSequenceWasFlushed:(AVPlayerItemOutput*)sender
{
  (void)sender;
  if (session)
    iupVideoSubtitleCue(session, NULL);
}
- (void)dealloc
{
  if (endObserver)
    [[NSNotificationCenter defaultCenter] removeObserver:endObserver];
  [endObserver release];
  [layer release];
  [caption release];
  [subtitles release];
  [legible release];
  [output release];
  [item release];
  [player release];
  [adjust release];
  free(rotated);
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

#if TARGET_OS_IPHONE
static UIFont* iappleVideoCaptionFont(CGFloat size)
{
  return [UIFont boldSystemFontOfSize:size];
}
#else
static NSFont* iappleVideoCaptionFont(CGFloat size)
{
  return [NSFont boldSystemFontOfSize:size];
}
#endif

static void iappleVideoLayout(IvideoSession* session, Ihandle* ih)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  CALayer* parent = iappleVideoParent(ih);
  if (context->layer && parent)
  {
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    context->layer.frame = parent.bounds;
    if (context->caption)
    {
      CGRect rect = context->layer.videoRect;
      NSString* text = (NSString*)context->caption.string;
      CGFloat width, height;
      CGRect bounds;
      context->captionRect = rect;
      if (CGRectIsEmpty(rect))
        rect = parent.bounds;
      width = MAX(0, rect.size.width - 20);
      bounds = [text boundingRectWithSize:CGSizeMake(width, rect.size.height)
        options:NSStringDrawingUsesLineFragmentOrigin attributes:@{NSFontAttributeName:iappleVideoCaptionFont(context->caption.fontSize)} context:nil];
      height = MIN(rect.size.height, ceil(bounds.size.height) + 4);
      context->caption.frame = CGRectMake(rect.origin.x + 10, CGRectGetMaxY(rect) - height - 10, width, height);
      context->caption.contentsScale = parent.contentsScale;
    }
    [CATransaction commit];
  }
}

static NSURL* iappleVideoURL(const char* source, int url)
{
  NSString* name = [NSString stringWithUTF8String:source];
  if (!name)
    return nil;
  return url ? [NSURL URLWithString:name] : [NSURL fileURLWithPath:[name stringByStandardizingPath]];
}

static int iappleVideoAttach(IupAppleVideo* context, AVPlayerItem* item)
{
  if (!item)
  {
    iupVideoError(context->session, "Cannot create AVPlayer item");
    return 0;
  }
  context->item = [item retain];
  [context->player replaceCurrentItemWithPlayerItem:item];
  if (context->output)
    [context->item addOutput:context->output];
  context->legible = [[AVPlayerItemLegibleOutput alloc] init];
  context->legible.suppressesPlayerRendering = context->output != nil;
  if (context->output)
    [context->legible setDelegate:context queue:dispatch_get_main_queue()];
  [context->item addOutput:context->legible];
  [context->item.asset loadValuesAsynchronouslyForKeys:@[@"availableMediaCharacteristicsWithMediaSelectionOptions", @"tracks"] completionHandler:^{
    dispatch_async(dispatch_get_main_queue(), ^{
      AVMediaSelectionGroup* group;
      if (!context->session)
        return;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      group = [context->item.asset mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicLegible];
      if (context->output)
      {
        AVAssetTrack* track = [context->item.asset tracksWithMediaType:AVMediaTypeVideo].firstObject;
        CGAffineTransform transform = track ? track.preferredTransform : CGAffineTransformIdentity;
        if (!CGAffineTransformIsIdentity(transform))
        {
          context->rotateA = (int)lround(transform.a);
          context->rotateB = (int)lround(transform.b);
          context->rotateC = (int)lround(transform.c);
          context->rotateD = (int)lround(transform.d);
          context->rotate = 1;
          context->outputWidth = context->outputHeight = -1;
        }
      }
#pragma clang diagnostic pop
      [context->subtitles release];
      context->subtitles = [group retain];
    });
  }];
  context->endObserver = [[[NSNotificationCenter defaultCenter]
    addObserverForName:AVPlayerItemDidPlayToEndTimeNotification object:context->item
    queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification* notification) {
      (void)notification;
      if (context->session)
        context->ended = 1;
    }] retain];
  return 1;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static AVPlayerItem* iappleVideoCompose(AVAsset* video, AVAsset* audio)
{
  AVMutableComposition* composition = [AVMutableComposition composition];
  CMTimeRange range = CMTimeRangeMake(kCMTimeZero, video.duration);
  int tracks = 0;
  for (AVAssetTrack* track in video.tracks)
  {
    AVMutableCompositionTrack* target;
    if ([track.mediaType isEqualToString:AVMediaTypeAudio])
      continue;
    target = [composition addMutableTrackWithMediaType:track.mediaType preferredTrackID:kCMPersistentTrackID_Invalid];
    if (target && [target insertTimeRange:range ofTrack:track atTime:kCMTimeZero error:nil])
    {
      target.preferredTransform = track.preferredTransform;
      tracks++;
    }
  }
  for (AVAssetTrack* track in [audio tracksWithMediaType:AVMediaTypeAudio])
  {
    AVMutableCompositionTrack* target = [composition addMutableTrackWithMediaType:AVMediaTypeAudio preferredTrackID:kCMPersistentTrackID_Invalid];
    CMTimeRange audio_range = CMTimeRangeMake(kCMTimeZero, CMTimeMinimum(audio.duration, video.duration));
    if (target && [target insertTimeRange:audio_range ofTrack:track atTime:kCMTimeZero error:nil])
      tracks++;
    break;
  }
  return tracks ? [AVPlayerItem playerItemWithAsset:composition] : nil;
}
#pragma clang diagnostic pop

static int iappleVideoOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  IupAppleVideo* context;
  CALayer* parent;
  NSURL* uri = iappleVideoURL(source, url);
  NSURL* audio_uri = nil;
  if (!uri)
  {
    iupVideoError(session, "Invalid source URL");
    return 0;
  }
  if (session->audio)
  {
    audio_uri = [NSURL URLWithString:[NSString stringWithUTF8String:session->audio]];
    if (!audio_uri)
    {
      iupVideoError(session, "Invalid AUDIOURL");
      return 0;
    }
  }
  context = [[IupAppleVideo alloc] init];
  if (!context)
    return 0;
  session->context = context;
  context->session = session;
  context->ih = ih;
  context->player = [[AVPlayer alloc] init];
  if (!context->player)
  {
    iupVideoError(session, "Cannot create AVPlayer");
    return 0;
  }
  context->player.actionAtItemEnd = AVPlayerActionAtItemEndPause;
#if !TARGET_OS_IPHONE
  if (session->audio_device)
    context->player.audioOutputDeviceUniqueID = [NSString stringWithUTF8String:session->audio_device];
#endif
  context->adjust = [[NSMutableData alloc] initWithLength:IVIDEO_ADJUST_COUNT * sizeof(int)];
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
  }
  if (audio_uri)
  {
    AVURLAsset* video = [AVURLAsset URLAssetWithURL:uri options:nil];
    AVURLAsset* audio = [AVURLAsset URLAssetWithURL:audio_uri options:nil];
    NSArray* keys = @[@"tracks", @"duration"];
    dispatch_group_t group = dispatch_group_create();
    dispatch_group_enter(group);
    [video loadValuesAsynchronouslyForKeys:keys completionHandler:^{ dispatch_group_leave(group); }];
    dispatch_group_enter(group);
    [audio loadValuesAsynchronouslyForKeys:keys completionHandler:^{ dispatch_group_leave(group); }];
    [context retain];
    dispatch_group_notify(group, dispatch_get_main_queue(), ^{
      if (context->session)
      {
        NSError* error = nil;
        AVPlayerItem* item = nil;
        if ([video statusOfValueForKey:@"tracks" error:&error] == AVKeyValueStatusLoaded &&
            [audio statusOfValueForKey:@"tracks" error:&error] == AVKeyValueStatusLoaded)
          item = iappleVideoCompose(video, audio);
        if (item)
          iappleVideoAttach(context, item);
        else
          iupVideoError(context->session, error ? error.localizedDescription.UTF8String : "Cannot combine the source with AUDIOURL");
      }
      [context release];
    });
    dispatch_release(group);
    return 1;
  }
  return iappleVideoAttach(context, [AVPlayerItem playerItemWithURL:uri]);
}

static void iappleVideoClose(IvideoSession* session)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  if (!context)
    return;
  context->session = NULL;
  [context->legible setDelegate:nil queue:NULL];
  [context->caption removeFromSuperlayer];
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

static void iappleVideoSeek(IvideoSession* session, double position, int fast)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  if (position > (double)LLONG_MAX / 600)
    return;
  context->ended = 0;
  context->restoring = 0;
  CMTime before = fast < 0 ? kCMTimePositiveInfinity : kCMTimeZero;
  CMTime after = fast > 0 ? kCMTimePositiveInfinity : kCMTimeZero;
  [context->player seekToTime:CMTimeMakeWithSeconds(position, 600) toleranceBefore:before toleranceAfter:after];
}

static void iappleVideoVolume(IvideoSession* session, int volume, int mute)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  context->player.volume = volume / 100.0f;
  context->player.muted = mute != 0;
}

static void iappleVideoResizeOutput(IupAppleVideo* context, int video_width, int video_height)
{
  NSMutableDictionary* attributes;
  AVPlayerItemVideoOutput* output;
  int width, height;
  iupVideoTargetSize(context->session, video_width, video_height, &width, &height);
  if (width == video_width && height == video_height)
    width = height = 0;
  else if (context->rotate && context->rotateA == 0)
  {
    int swap = width;
    width = height;
    height = swap;
  }
  if (width == context->outputWidth && height == context->outputHeight)
    return;
  attributes = [NSMutableDictionary dictionaryWithObject:@(kCVPixelFormatType_32BGRA) forKey:(id)kCVPixelBufferPixelFormatTypeKey];
  if (width)
  {
    attributes[(id)kCVPixelBufferWidthKey] = @(width);
    attributes[(id)kCVPixelBufferHeightKey] = @(height);
  }
  output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:attributes];
  if (!output)
    return;
  [context->item addOutput:output];
  [context->item removeOutput:context->output];
  [context->output release];
  context->output = output;
  context->outputWidth = width;
  context->outputHeight = height;
}

static void iappleVideoRotateFrame(IupAppleVideo* context, const unsigned char* pixels, int width, int height, size_t stride)
{
  int a = context->rotateA, b = context->rotateB, c = context->rotateC, d = context->rotateD;
  int out_width = a ? width : height, out_height = a ? height : width;
  int corners[4][2] = {{0, 0}, {width - 1, 0}, {0, height - 1}, {width - 1, height - 1}};
  int min_x = 0, min_y = 0, i, x, y;
  size_t size = (size_t)out_width * out_height * 4;
  uint32_t* target;
  for (i = 0; i < 4; i++)
  {
    int cx = a * corners[i][0] + c * corners[i][1];
    int cy = b * corners[i][0] + d * corners[i][1];
    if (cx < min_x)
      min_x = cx;
    if (cy < min_y)
      min_y = cy;
  }
  if (size > context->rotatedSize)
  {
    unsigned char* buffer = (unsigned char*)realloc(context->rotated, size);
    if (!buffer)
      return;
    context->rotated = buffer;
    context->rotatedSize = size;
  }
  target = (uint32_t*)context->rotated;
  for (y = 0; y < out_height; y++)
  {
    int dy = y + min_y;
    for (x = 0; x < out_width; x++)
    {
      int dx = x + min_x;
      int sx = a * dx + b * dy, sy = c * dx + d * dy;
      *target++ = *(const uint32_t*)(pixels + (size_t)sy * stride + (size_t)sx * 4);
    }
  }
  iupVideoFrame(context->session, context->rotated, out_width, out_height, (size_t)out_width * 4);
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
    {
      if (context->rotate)
        iappleVideoRotateFrame(context, pixels, (int)width, (int)height, stride);
      else
        iupVideoFrame(context->session, pixels, (int)width, (int)height, stride);
    }
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
  }
  CVPixelBufferRelease(buffer);
}

static CMTime iappleVideoKeptTime(IupAppleVideo* context)
{
  return context->restoring ? context->restoreTime : context->player.currentTime;
}

static void iappleVideoRestoreTime(IupAppleVideo* context, CMTime time)
{
  context->restoring = 1;
  context->restoreTime = time;
  [context->player seekToTime:time toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero completionHandler:^(BOOL finished) {
    if (finished)
      context->restoring = 0;
  }];
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static void iappleVideoApplyAdjust(IupAppleVideo* context)
{
  const int* values = (const int*)context->adjust.bytes;
  int any = values[IVIDEO_BRIGHTNESS] || values[IVIDEO_CONTRAST] || values[IVIDEO_SATURATION] || values[IVIDEO_GAMMA];
  int video = 0;
  NSMutableData* adjust = context->adjust;
  CMTime time;
  if (!context->adjustPending || context->item.status != AVPlayerItemStatusReadyToPlay)
    return;
  context->adjustPending = 0;
  if (!any)
  {
    if (context->item.videoComposition)
    {
      time = iappleVideoKeptTime(context);
      context->item.videoComposition = nil;
      iappleVideoRestoreTime(context, time);
    }
    return;
  }
  if (context->item.videoComposition)
    return;
  for (AVPlayerItemTrack* track in context->item.tracks)
  {
    if ([track.assetTrack.mediaType isEqualToString:AVMediaTypeVideo])
      video = 1;
  }
  if (!video)
    return;
  time = iappleVideoKeptTime(context);
  context->item.videoComposition = [AVVideoComposition videoCompositionWithAsset:context->item.asset applyingCIFiltersWithHandler:^(AVAsynchronousCIImageFilteringRequest* request) {
    const int* current = (const int*)adjust.bytes;
    CIImage* image = request.sourceImage.imageByClampingToExtent;
    CIFilter* color = [CIFilter filterWithName:@"CIColorControls"];
    CIFilter* gamma = [CIFilter filterWithName:@"CIGammaAdjust"];
    [color setValue:image forKey:kCIInputImageKey];
    [color setValue:@(current[IVIDEO_BRIGHTNESS] / 100.0) forKey:kCIInputBrightnessKey];
    [color setValue:@(1 + current[IVIDEO_CONTRAST] / 100.0) forKey:kCIInputContrastKey];
    [color setValue:@(1 + current[IVIDEO_SATURATION] / 100.0) forKey:kCIInputSaturationKey];
    [gamma setValue:color.outputImage forKey:kCIInputImageKey];
    [gamma setValue:@(pow(2, -current[IVIDEO_GAMMA] / 50.0)) forKey:@"inputPower"];
    [request finishWithImage:[gamma.outputImage imageByCroppingToRect:request.sourceImage.extent] context:nil];
  }];
  iappleVideoRestoreTime(context, time);
}
#pragma clang diagnostic pop

static void iappleVideoAdjust(IvideoSession* session, const int* values)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  memcpy(context->adjust.mutableBytes, values, IVIDEO_ADJUST_COUNT * sizeof(int));
  context->adjustPending = 1;
  iappleVideoApplyAdjust(context);
}

#if !TARGET_OS_IPHONE
static char* iappleVideoAudioString(AudioObjectID device, AudioObjectPropertySelector selector)
{
  AudioObjectPropertyAddress address = {selector, kAudioObjectPropertyScopeGlobal, 0};
  CFStringRef value = NULL;
  UInt32 size = sizeof(value);
  char* result = NULL;
  if (AudioObjectGetPropertyData(device, &address, 0, NULL, &size, &value) == noErr && value)
  {
    result = iupStrDup([(NSString*)value UTF8String]);
    CFRelease(value);
  }
  return result;
}

static int iappleVideoAudioDevices(IvideoAudioDevice** devices)
{
  AudioObjectPropertyAddress address = {kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, 0};
  AudioObjectID* ids;
  UInt32 size = 0, total, i;
  int count = 0;
  if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, NULL, &size) != noErr || size == 0)
    return 0;
  ids = (AudioObjectID*)malloc(size);
  if (!ids)
    return 0;
  if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, NULL, &size, ids) != noErr)
  {
    free(ids);
    return 0;
  }
  total = size / sizeof(AudioObjectID);
  for (i = 0; i < total; i++)
  {
    AudioObjectPropertyAddress streams = {kAudioDevicePropertyStreams, kAudioObjectPropertyScopeOutput, 0};
    UInt32 streams_size = 0;
    char* uid;
    char* name;
    if (AudioObjectGetPropertyDataSize(ids[i], &streams, 0, NULL, &streams_size) != noErr || streams_size == 0)
      continue;
    uid = iappleVideoAudioString(ids[i], kAudioDevicePropertyDeviceUID);
    name = iappleVideoAudioString(ids[i], kAudioObjectPropertyName);
    if (uid)
      count = iupVideoAddAudioDevice(devices, count, uid, name);
    free(uid);
    free(name);
  }
  free(ids);
  return count;
}

static void iappleVideoAudioDevice(IvideoSession* session, const char* id)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  CMTime time = iappleVideoKeptTime(context);
  context->player.audioOutputDeviceUniqueID = id ? [NSString stringWithUTF8String:id] : nil;
  if (context->item.status == AVPlayerItemStatusReadyToPlay)
    iappleVideoRestoreTime(context, time);
}
#endif

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
    iappleVideoApplyAdjust(context);
    if (context->output)
    {
      if (info.width > 0 && info.height > 0)
        iappleVideoResizeOutput(context, info.width, info.height);
      iappleVideoFrame(context);
    }
  }
  if (context->caption && context->layer && !CGRectEqualToRect(context->layer.videoRect, context->captionRect))
    iappleVideoLayout(session, context->ih);
  iupVideoInfo(session, &info);
}

static int iappleVideoSubtitleCount(IvideoSession* session)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  return (int)context->subtitles.options.count;
}

static char* iappleVideoSubtitleName(IvideoSession* session, int index, int language)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  AVMediaSelectionOption* option;
  if (index < 1 || index > (int)context->subtitles.options.count)
    return NULL;
  option = context->subtitles.options[index - 1];
  return iupStrReturnStr((language ? (option.extendedLanguageTag ?: option.locale.localeIdentifier) : option.displayName).UTF8String);
}

static int iappleVideoSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  CMTime position;
  BOOL suppress, changed;
  (void)delay;
  if (index > (int)context->subtitles.options.count)
    return 0;
  position = context->player.currentTime;
  suppress = index == 0 || context->output != nil;
  changed = context->legible.suppressesPlayerRendering != suppress;
  if (changed)
    context->legible.suppressesPlayerRendering = suppress;
  if (index == 0)
    iupVideoSubtitleCue(session, NULL);
  if (context->subtitles)
  {
    AVMediaSelectionOption* previous = [context->item.currentMediaSelection selectedMediaOptionInMediaSelectionGroup:context->subtitles];
    AVMediaSelectionOption* selected;
    if (index == -1)
    {
      [context->item selectMediaOptionAutomaticallyInMediaSelectionGroup:context->subtitles];
      selected = [context->item.currentMediaSelection selectedMediaOptionInMediaSelectionGroup:context->subtitles];
      if (previous != selected && ![previous isEqual:selected])
        changed = YES;
    }
    else
    {
      selected = index == 0 ? nil : context->subtitles.options[index - 1];
      if (previous != selected && ![previous isEqual:selected])
      {
        [context->item selectMediaOption:selected inMediaSelectionGroup:context->subtitles];
        changed = YES;
      }
    }
  }
  if (changed && CMTIME_IS_NUMERIC(position) && context->item.seekableTimeRanges.count)
    [context->player seekToTime:position toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
  return 1;
}

static int iappleVideoSubtitleText(IvideoSession* session, const char* text, int size, const char* color)
{
  IupAppleVideo* context = (IupAppleVideo*)session->context;
  unsigned char r = 255, g = 255, b = 255;
  CGFloat components[4];
  CGColorSpaceRef space;
  CGColorRef foreground;
  if (!context->layer)
    return 0;
  if (!text && !context->caption)
    return 1;
  if (!context->caption)
  {
    context->caption = [[CATextLayer alloc] init];
    context->caption.alignmentMode = kCAAlignmentCenter;
    context->caption.wrapped = YES;
    context->caption.shadowOpacity = 1;
    context->caption.shadowRadius = 2;
    context->caption.shadowOffset = CGSizeMake(0, 1);
    [context->layer addSublayer:context->caption];
  }
  iupStrToRGB(color, &r, &g, &b);
  components[0] = r / 255.0;
  components[1] = g / 255.0;
  components[2] = b / 255.0;
  components[3] = 1;
  space = CGColorSpaceCreateDeviceRGB();
  foreground = CGColorCreate(space, components);
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  context->caption.string = text ? [NSString stringWithUTF8String:text] : @"";
  context->caption.font = (CFTypeRef)iappleVideoCaptionFont(size);
  context->caption.fontSize = size;
  context->caption.foregroundColor = foreground;
  [CATransaction commit];
  CGColorRelease(foreground);
  CGColorSpaceRelease(space);
  iappleVideoLayout(session, context->ih);
  return 1;
}

#if !TARGET_OS_IPHONE
static id iapple_video_activity = nil;
#endif

void iupdrvVideoInhibitScreenSaver(int inhibit)
{
#if TARGET_OS_IPHONE
  [UIApplication sharedApplication].idleTimerDisabled = inhibit ? YES : NO;
#else
  if (inhibit && !iapple_video_activity)
    iapple_video_activity = [[[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityIdleDisplaySleepDisabled | NSActivityIdleSystemSleepDisabled reason:@"Playing video"] retain];
  else if (!inhibit && iapple_video_activity)
  {
    [[NSProcessInfo processInfo] endActivity:iapple_video_activity];
    [iapple_video_activity release];
    iapple_video_activity = nil;
  }
#endif
}

char* iupdrvVideoReadSubtitle(const char* filename)
{
  NSData* bytes = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:filename]];
  char* text;
  if (!bytes || bytes.length > 16 * 1024 * 1024 || memchr(bytes.bytes, 0, bytes.length))
    return NULL;
  text = (char*)malloc(bytes.length + 1);
  if (text)
  {
    memcpy(text, bytes.bytes, bytes.length);
    text[bytes.length] = 0;
  }
  return text;
}

static const IvideoBackend iappleVideoBackend = {"NATIVE", iappleVideoAvailable, iappleVideoOpen, iappleVideoClose, iappleVideoPlay, iappleVideoSeek, iappleVideoVolume, iappleVideoPoll, iappleVideoLayout, iappleVideoSubtitleCount, iappleVideoSubtitleName, iappleVideoSubtitleSelect, iappleVideoSubtitleText, 1,
#if TARGET_OS_IPHONE
  NULL, NULL,
#else
  iappleVideoAudioDevices, iappleVideoAudioDevice,
#endif
  iappleVideoAdjust};

#if TARGET_OS_IPHONE
const IvideoBackend* iupdrvVideoBackend(int index)
{
  return index == 0 ? &iappleVideoBackend : NULL;
}
#else
extern const IvideoBackend iupVideoMpvBackend;
extern const IvideoBackend iupVideoVlcBackend;
extern const IvideoBackend iupVideoGstreamerBackend;

const IvideoBackend* iupdrvVideoBackend(int index)
{
  static const IvideoBackend* backends[] = {&iappleVideoBackend, &iupVideoMpvBackend, &iupVideoVlcBackend, &iupVideoGstreamerBackend};
  if (index < 0 || index >= (int)(sizeof(backends) / sizeof(backends[0])))
    return NULL;
  return backends[index];
}
#endif

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

@interface IupAppleVideo : NSObject <AVPlayerItemLegibleOutputPushDelegate>
{
@public
  IvideoSession* session;
  Ihandle* ih;
  AVPlayer* player;
  AVPlayerItem* item;
  AVPlayerLayer* layer;
  AVPlayerItemVideoOutput* output;
  AVPlayerItemLegibleOutput* legible;
  AVMediaSelectionGroup* subtitles;
  CATextLayer* caption;
  CGRect captionRect;
  id endObserver;
  unsigned char* rgba;
  size_t capacity;
  int ended;
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
  [context->item.asset loadValuesAsynchronouslyForKeys:@[@"availableMediaCharacteristicsWithMediaSelectionOptions"] completionHandler:^{
    dispatch_async(dispatch_get_main_queue(), ^{
      AVMediaSelectionGroup* group;
      if (!context->session)
        return;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      group = [context->item.asset mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicLegible];
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

static const IvideoBackend iappleVideoBackend = {"NATIVE", iappleVideoAvailable, iappleVideoOpen, iappleVideoClose, iappleVideoPlay, iappleVideoSeek, iappleVideoVolume, iappleVideoPoll, iappleVideoLayout, iappleVideoSubtitleCount, iappleVideoSubtitleName, iappleVideoSubtitleSelect, iappleVideoSubtitleText, 1};

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

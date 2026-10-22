/** \file
 * \brief Video control, HTMLVideoElement playback
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

#include "iup.h"
#include "iup_str.h"
#include "iup_video.h"

#include "iupwasm_drv.h"

enum
{
  IWASMVIDEO_READY,
  IWASMVIDEO_STATE,
  IWASMVIDEO_ENDED,
  IWASMVIDEO_ERROR,
  IWASMVIDEO_POSITION,
  IWASMVIDEO_DURATION,
  IWASMVIDEO_SEEKABLE,
  IWASMVIDEO_WIDTH,
  IWASMVIDEO_HEIGHT,
  IWASMVIDEO_AUDIO,
  IWASMVIDEO_VIDEO,
  IWASMVIDEO_COUNT
};

typedef struct _IwasmVideo
{
  int vid;
  double ended;
  char background[32];
} IwasmVideo;

EM_JS(int, iupwasmJsVideoOpen, (int vid, int canvas, const char* source, int url, const char* background), {
  var src = UTF8ToString(source), data = null;
  if (!url) {
    try { data = FS.readFile(src); } catch (e) { return 0; }
  }
  var buffer = typeof SharedArrayBuffer !== 'undefined' ? new SharedArrayBuffer(8 * 11) : new ArrayBuffer(8 * 11);
  if (!globalThis.__iupVideo) globalThis.__iupVideo = {};
  globalThis.__iupVideo[vid] = new Float64Array(buffer);
  var subtitles = typeof SharedArrayBuffer !== 'undefined' ? new SharedArrayBuffer(32776) : new ArrayBuffer(32776);
  if (!globalThis.__iupVideoSubtitles) globalThis.__iupVideoSubtitles = {};
  globalThis.__iupVideoSubtitles[vid] = new Uint8Array(subtitles);
  var ext = src.split('?')[0].split('.').pop().toLowerCase();
  var types = { mp4: 'video/mp4', m4v: 'video/mp4', mov: 'video/quicktime', webm: 'video/webm', mkv: 'video/x-matroska', ogv: 'video/ogg', ogg: 'audio/ogg', mp3: 'audio/mpeg', m4a: 'audio/mp4', wav: 'audio/wav', flac: 'audio/flac' };
  globalThis.__iupApply({ op: 'vidopen', vid: vid, id: canvas, sab: buffer, subtitles: subtitles, url: url ? src : null, data: data ? data.buffer : null, type: data ? (types[ext] || "") : "", bg: UTF8ToString(background) });
  return 1;
})

EM_JS(void, iupwasmJsVideoRead, (int vid, double* values), {
  var state = globalThis.__iupVideo && globalThis.__iupVideo[vid];
  if (state) HEAPF64.set(state, values >> 3);
})

EM_JS(void, iupwasmJsVideoClose, (int vid), {
  globalThis.__iupApply({ op: 'vidclose', vid: vid });
  if (globalThis.__iupVideo) delete globalThis.__iupVideo[vid];
  if (globalThis.__iupVideoSubtitles) delete globalThis.__iupVideoSubtitles[vid];
})

EM_JS(void, iupwasmJsVideoPlay, (int vid, int play), {
  globalThis.__iupApply({ op: 'vidplay', vid: vid, play: play });
})

EM_JS(void, iupwasmJsVideoSeek, (int vid, double position, int fast), {
  globalThis.__iupApply({ op: 'vidseek', vid: vid, t: position, fast: fast });
})

EM_JS(void, iupwasmJsVideoVolume, (int vid, int volume, int mute), {
  globalThis.__iupApply({ op: 'vidvol', vid: vid, v: volume, m: mute });
})

EM_JS(char*, iupwasmJsVideoDevices, (void), {
  if (typeof document !== 'undefined') return 0;
  var s = globalThis.__iupReadSync({ op: 'viddevices' });
  if (!s) return 0;
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(void, iupwasmJsVideoSink, (int vid, const char* id), {
  globalThis.__iupApply({ op: 'vidsink', vid: vid, id: UTF8ToString(id) });
})

EM_JS(void, iupwasmJsVideoFilter, (int vid, int brightness, int contrast, int saturation, int gamma), {
  globalThis.__iupApply({ op: 'vidfilter', vid: vid, b: brightness, c: contrast, s: saturation, g: gamma });
})

EM_JS(void, iupwasmJsVideoWake, (int on), {
  globalThis.__iupApply({ op: 'vidwake', on: on });
})

EM_JS(void, iupwasmJsVideoBackground, (int vid, const char* background), {
  globalThis.__iupApply({ op: 'vidbg', vid: vid, bg: UTF8ToString(background) });
})

EM_JS(int, iupwasmJsVideoSubtitles, (int vid, int index, int language, char* text), {
  var bytes = globalThis.__iupVideoSubtitles && globalThis.__iupVideoSubtitles[vid];
  if (!bytes) return 0;
  var header = new Int32Array(bytes.buffer, 0, 2);
  for (;;) {
    var version = Atomics.load(header, 0);
    if (version & 1) continue;
    var count = Atomics.load(header, 1);
    if (text) {
      HEAPU8[text] = 0;
      if (index >= 1 && index <= count && index <= 64) {
        var offset = 8 + (index - 1) * 512 + (language ? 256 : 0);
        HEAPU8.set(bytes.subarray(offset, offset + 256), text);
      }
    }
    if (Atomics.load(header, 0) === version) return count;
  }
})

EM_JS(void, iupwasmJsVideoSubtitleSelect, (int vid, int index, double delay), {
  globalThis.__iupApply({ op: 'vidsubselect', vid: vid, index: index, delay: delay });
})

EM_JS(void, iupwasmJsVideoSubtitleText, (int vid, const char* text, int size, const char* color), {
  globalThis.__iupApply({ op: 'vidsubtext', vid: vid, text: text ? UTF8ToString(text) : "", size: size, color: UTF8ToString(color) });
})

static void wasmVideoBackground(Ihandle* ih, char* background, size_t size)
{
  unsigned char r = 0, g = 0, b = 0;
  iupStrToRGB(IupGetAttribute(ih, "BGCOLOR"), &r, &g, &b);
  snprintf(background, size, "rgb(%d,%d,%d)", r, g, b);
}

static int wasmVideoAvailable(void)
{
  return 1;
}

static void wasmVideoAudioDevice(IvideoSession* session, const char* id)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  iupwasmJsVideoSink(context->vid, id ? id : "");
}

static int wasmVideoAudioDevices(IvideoAudioDevice** devices)
{
  char* list = iupwasmJsVideoDevices();
  char* line;
  int count = 0;
  if (!list)
    return 0;
  for (line = strtok(list, "\n"); line; line = strtok(NULL, "\n"))
  {
    char* name = strchr(line, '\t');
    if (!name)
      continue;
    *name++ = 0;
    count = iupVideoAddAudioDevice(devices, count, line, name);
  }
  free(list);
  return count;
}

static void wasmVideoAdjust(IvideoSession* session, const int* values)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  iupwasmJsVideoFilter(context->vid, values[IVIDEO_BRIGHTNESS], values[IVIDEO_CONTRAST], values[IVIDEO_SATURATION], values[IVIDEO_GAMMA]);
}

static int wasmVideoOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  static int next_vid = 1;
  IwasmVideo* context;
  int canvas = iupwasmIdOf(ih);
  if (!canvas)
  {
    iupVideoError(session, "No video canvas element");
    return 0;
  }
  context = (IwasmVideo*)calloc(1, sizeof(IwasmVideo));
  if (!context)
    return 0;
  session->context = context;
  context->vid = next_vid++;
  wasmVideoBackground(ih, context->background, sizeof(context->background));
  if (!iupwasmJsVideoOpen(context->vid, canvas, source, url, context->background))
  {
    context->vid = 0;
    iupVideoError(session, "Cannot read the video file");
    return 0;
  }
  if (session->audio_device)
    wasmVideoAudioDevice(session, session->audio_device);
  return 1;
}

static void wasmVideoClose(IvideoSession* session)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  if (!context)
    return;
  if (context->vid)
    iupwasmJsVideoClose(context->vid);
  free(context);
  session->context = NULL;
}

static void wasmVideoPlay(IvideoSession* session, int play)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  iupwasmJsVideoPlay(context->vid, play);
}

static void wasmVideoSeek(IvideoSession* session, double position, int fast)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  iupwasmJsVideoSeek(context->vid, position, fast);
}

static void wasmVideoVolume(IvideoSession* session, int volume, int mute)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  iupwasmJsVideoVolume(context->vid, volume, mute);
}

static void wasmVideoLayout(IvideoSession* session, Ihandle* ih)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  char background[32];
  wasmVideoBackground(ih, background, sizeof(background));
  if (strcmp(background, context->background) != 0)
  {
    strcpy(context->background, background);
    iupwasmJsVideoBackground(context->vid, background);
  }
}

static const char* wasmVideoErrorMessage(int code)
{
  switch (code)
  {
  case 1:
    return "Video loading was aborted";
  case 2:
    return "A network error stopped the video download";
  case 3:
    return "The video could not be decoded";
  default:
    return "The video format or source is not supported";
  }
}

static void wasmVideoPoll(IvideoSession* session)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  double values[IWASMVIDEO_COUNT];
  IvideoInfo info;
  memset(values, 0, sizeof(values));
  memset(&info, 0, sizeof(info));
  iupwasmJsVideoRead(context->vid, values);
  if (values[IWASMVIDEO_ERROR] > 0)
  {
    iupVideoError(session, wasmVideoErrorMessage((int)values[IWASMVIDEO_ERROR]));
    return;
  }
  info.ready = values[IWASMVIDEO_READY] > 0;
  info.state = info.ready ? (int)values[IWASMVIDEO_STATE] : IVIDEO_LOADING;
  info.ended = values[IWASMVIDEO_ENDED] > context->ended;
  context->ended = values[IWASMVIDEO_ENDED];
  info.position = values[IWASMVIDEO_POSITION];
  info.duration = values[IWASMVIDEO_DURATION];
  info.seekable = values[IWASMVIDEO_SEEKABLE] > 0;
  info.width = (int)values[IWASMVIDEO_WIDTH];
  info.height = (int)values[IWASMVIDEO_HEIGHT];
  info.audio = values[IWASMVIDEO_AUDIO] > 0;
  info.video = values[IWASMVIDEO_VIDEO] > 0;
  info.aspect = info.height > 0 ? (double)info.width / info.height : 0;
  iupVideoInfo(session, &info);
}

static int wasmVideoSubtitleCount(IvideoSession* session)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  return iupwasmJsVideoSubtitles(context->vid, 0, 0, NULL);
}

static char* wasmVideoSubtitleName(IvideoSession* session, int index, int language)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  char text[256] = "";
  iupwasmJsVideoSubtitles(context->vid, index, language, text);
  return text[0] ? iupStrReturnStr(text) : NULL;
}

static int wasmVideoSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  if (index > wasmVideoSubtitleCount(session))
    return 0;
  iupwasmJsVideoSubtitleSelect(context->vid, index, delay);
  return 1;
}

static int wasmVideoSubtitleText(IvideoSession* session, const char* text, int size, const char* color)
{
  IwasmVideo* context = (IwasmVideo*)session->context;
  iupwasmJsVideoSubtitleText(context->vid, text, size, color);
  return 1;
}

char* iupdrvVideoReadSubtitle(const char* filename)
{
  return iupVideoReadSubtitleStream(fopen(filename, "rb"));
}

void iupdrvVideoInhibitScreenSaver(int inhibit)
{
  iupwasmJsVideoWake(inhibit);
}

static const IvideoBackend wasmVideoBackend = {"NATIVE", wasmVideoAvailable, wasmVideoOpen, wasmVideoClose, wasmVideoPlay, wasmVideoSeek, wasmVideoVolume, wasmVideoPoll, wasmVideoLayout, wasmVideoSubtitleCount, wasmVideoSubtitleName, wasmVideoSubtitleSelect, wasmVideoSubtitleText, 0, wasmVideoAudioDevices, wasmVideoAudioDevice, wasmVideoAdjust};

const IvideoBackend* iupdrvVideoBackend(int index)
{
  return index == 0 ? &wasmVideoBackend : NULL;
}

/** \file
 * \brief Video player
 *
 * See Copyright Notice in "iup.h"
 */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupdraw.h"
#include "iupmedia.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"
#include "iup_thread.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_video.h"

typedef struct
{
  Ihandle* ih;
  unsigned references;
  unsigned long generation;
} IvideoLifetime;

typedef int (*IvideoStringCallback)(Ihandle*, const char*);

#define IVIDEO_SUBTITLE_LIMIT (16 * 1024 * 1024)

typedef struct
{
  double start, end, maximum;
  int order;
  char* text;
} IvideoSubtitle;

struct _IcontrolData
{
  iupCanvas canvas;
  IvideoSession* session;
  IvideoLifetime* lifetime;
  Ihandle* timer;
  Ihandle* image;
  unsigned char* frame;
  int frame_width, frame_height;
  char image_name[48];
  char backend[16];
  char* source;
  char* audio;
  int url, intent, state, ready, ended, reported_error, reported_state, inhibit;
  int pending_seek;
  double seek_position;
  IvideoInfo info;
  IvideoSubtitle* subtitles;
  int subtitle_count, subtitle_loaded, subtitle, subtitle_size, subtitle_dirty, subtitle_native, subtitle_error_pending;
  int subtitle_tracks, subtitle_ticks;
  double subtitle_delay;
  char* subtitle_file;
  char* subtitle_text;
  char* subtitle_wrapped;
  int subtitle_wrap_width;
  char subtitle_color[32], subtitle_error[512];
};

static const char* iVideoStates[] = {"EMPTY", "LOADING", "STOPPED", "PLAYING", "PAUSED", "BUFFERING", "ENDED", "ERROR"};

char* iupVideoReadSubtitleStream(FILE* file)
{
  size_t limit = IVIDEO_SUBTITLE_LIMIT;
  size_t capacity = 65536;
  size_t length = 0;
  char* text;
  if (!file)
    return NULL;
  if (fseek(file, 0, SEEK_END) == 0)
  {
    long size = ftell(file);
    if (size >= 0)
      capacity = (unsigned long)size < limit ? (size_t)size + 2 : limit + 2;
    if (fseek(file, 0, SEEK_SET) != 0)
    {
      fclose(file);
      return NULL;
    }
  }
  text = (char*)malloc(capacity);
  if (!text)
  {
    fclose(file);
    return NULL;
  }
  while (length <= limit)
  {
    size_t count;
    if (length + 1 >= capacity)
    {
      size_t next = capacity * 2 > limit + 2 ? limit + 2 : capacity * 2;
      char* buffer = (char*)realloc(text, next);
      if (!buffer)
      {
        free(text);
        fclose(file);
        return NULL;
      }
      text = buffer;
      capacity = next;
    }
    count = fread(text + length, 1, capacity - 1 - length, file);
    if (!count)
      break;
    length += count;
  }
  if (ferror(file) || memchr(text, 0, length))
  {
    free(text);
    fclose(file);
    return NULL;
  }
  fclose(file);
  text[length] = 0;
  return text;
}

void iupVideoSubtitleCue(IvideoSession* session, const char* text)
{
  char* copy = text && *text ? iupStrDup(text) : NULL;
  iupdrvMutexLock(session->mutex);
  free(session->subtitle_cue);
  session->subtitle_cue = copy;
  iupdrvMutexUnlock(session->mutex);
}

void iupVideoSubtitleError(IvideoSession* session, const char* message)
{
  iupdrvMutexLock(session->mutex);
  if (!session->closing)
    iupStrCopyN(session->subtitle_error, sizeof(session->subtitle_error), message);
  iupdrvMutexUnlock(session->mutex);
}

static void iVideoSubtitleError(Ihandle* ih, const char* message)
{
  iupStrCopyN(ih->data->subtitle_error, sizeof(ih->data->subtitle_error), message);
  ih->data->subtitle_error_pending = 1;
  if (ih->handle)
    IupSetAttribute(ih->data->timer, "RUN", "YES");
}

static void iVideoFreeSubtitles(IvideoSubtitle* cues, int count)
{
  int i;
  for (i = 0; i < count; i++)
    free(cues[i].text);
  free(cues);
}

static int iVideoSubtitleTime(const char* text, double* value, const char** tail)
{
  unsigned parts[3] = {0, 0, 0}, fraction = 0;
  int count = 0, digits;
  while (*text == ' ' || *text == '\t')
    text++;
  for (;;)
  {
    unsigned n = 0;
    digits = 0;
    while (iup_isdigit(*text))
    {
      digits++;
      if (digits > 6)
        return 0;
      n = n * 10 + (unsigned)(*text - '0');
      text++;
    }
    if (!digits || count == 3)
      return 0;
    parts[count] = n;
    count++;
    if (*text != ':')
      break;
    text++;
  }
  if (count < 2 || parts[count - 1] >= 60 || (count == 3 && parts[1] >= 60) || (*text != ',' && *text != '.'))
    return 0;
  text++;
  for (digits = 0; digits < 3; digits++)
  {
    if (!iup_isdigit(*text))
      return 0;
    fraction = fraction * 10 + (unsigned)(*text - '0');
    text++;
  }
  if (iup_isdigit(*text))
    return 0;
  *value = count == 3 ? parts[0] * 3600.0 + parts[1] * 60.0 + parts[2] : parts[0] * 60.0 + parts[1];
  *value += fraction / 1000.0;
  *tail = text;
  return 1;
}

static int iVideoSubtitleUTF8(const char* text)
{
  const unsigned char* p = (const unsigned char*)text;
  while (*p)
  {
    unsigned value, minimum;
    int remaining;
    if (*p < 0x80)
    {
      p++;
      continue;
    }
    if (*p >= 0xc2 && *p <= 0xdf)
    {
      value = *p & 0x1f;
      remaining = 1;
      minimum = 0x80;
    }
    else if (*p >= 0xe0 && *p <= 0xef)
    {
      value = *p & 0x0f;
      remaining = 2;
      minimum = 0x800;
    }
    else if (*p >= 0xf0 && *p <= 0xf4)
    {
      value = *p & 0x07;
      remaining = 3;
      minimum = 0x10000;
    }
    else
      return 0;
    p++;
    while (remaining--)
    {
      if ((*p & 0xc0) != 0x80)
        return 0;
      value = (value << 6) | (*p & 0x3f);
      p++;
    }
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
      return 0;
  }
  return 1;
}

static size_t iVideoSubtitleEncode(char* output, unsigned value)
{
  if (value < 0x80)
  {
    output[0] = (char)value;
    return 1;
  }
  if (value < 0x800)
  {
    output[0] = (char)(0xc0 | (value >> 6));
    output[1] = (char)(0x80 | (value & 63));
    return 2;
  }
  if (value < 0x10000)
  {
    output[0] = (char)(0xe0 | (value >> 12));
    output[1] = (char)(0x80 | ((value >> 6) & 63));
    output[2] = (char)(0x80 | (value & 63));
    return 3;
  }
  output[0] = (char)(0xf0 | (value >> 18));
  output[1] = (char)(0x80 | ((value >> 12) & 63));
  output[2] = (char)(0x80 | ((value >> 6) & 63));
  output[3] = (char)(0x80 | (value & 63));
  return 4;
}

static char* iVideoSubtitlePlain(const char* text, size_t length)
{
  char* result = (char*)malloc(length + 1);
  size_t i = 0, n = 0;
  if (!result)
    return NULL;
  while (i < length)
  {
    if (text[i] == '<')
    {
      size_t end = i + 1;
      while (end < length && text[end] != '>')
        end++;
      if (end < length)
      {
        if (end - i >= 3 && text[i + 1] == 'b' && text[i + 2] == 'r' &&
            (end == i + 3 || text[i + 3] == '/' || text[i + 3] == ' '))
        {
          result[n] = '\n';
          n++;
        }
        i = end + 1;
        continue;
      }
    }
    if (text[i] == '&')
    {
      static const char* names[] = {"&amp;", "&lt;", "&gt;", "&quot;", "&apos;", "&nbsp;"};
      static const char values[] = {'&', '<', '>', '"', '\'', ' '};
      unsigned j;
      for (j = 0; j < sizeof(values); j++)
      {
        size_t size = strlen(names[j]);
        if (length - i >= size && memcmp(text + i, names[j], size) == 0)
        {
          result[n] = values[j];
          n++;
          i += size;
          break;
        }
      }
      if (j < sizeof(values))
        continue;
      if (length - i > 3 && text[i + 1] == '#')
      {
        size_t end = i + 2;
        unsigned value = 0, base = 10, digits = 0;
        if (text[end] == 'x' || text[end] == 'X')
        {
          base = 16;
          end++;
        }
        while (end < length && digits < 8)
        {
          unsigned digit;
          if (text[end] >= '0' && text[end] <= '9')
            digit = (unsigned)(text[end] - '0');
          else if (text[end] >= 'a' && text[end] <= 'f')
            digit = (unsigned)(text[end] - 'a' + 10);
          else if (text[end] >= 'A' && text[end] <= 'F')
            digit = (unsigned)(text[end] - 'A' + 10);
          else
            break;
          if (digit >= base || value > 0x10ffff / base)
            break;
          value = value * base + digit;
          end++;
          digits++;
        }
        if (digits && end < length && text[end] == ';' && value && value <= 0x10ffff && !(value >= 0xd800 && value <= 0xdfff))
        {
          n += iVideoSubtitleEncode(result + n, value);
          i = end + 1;
          continue;
        }
      }
    }
    result[n] = text[i];
    n++;
    i++;
  }
  while (n && (result[n - 1] == '\n' || result[n - 1] == ' '))
    n--;
  result[n] = 0;
  return result;
}

static int iVideoSubtitleCompare(const void* first, const void* second)
{
  const IvideoSubtitle* a = (const IvideoSubtitle*)first;
  const IvideoSubtitle* b = (const IvideoSubtitle*)second;
  if (a->start != b->start)
    return a->start < b->start ? -1 : 1;
  if (a->end != b->end)
    return a->end < b->end ? -1 : 1;
  return a->order < b->order ? -1 : a->order > b->order;
}

static int iVideoParseSubtitles(const char* input, IvideoSubtitle** output, int* count)
{
  char* data;
  char* p;
  char* write;
  IvideoSubtitle* cues = NULL;
  int size = 0, capacity = 0, parsed = 0, webvtt, i;
  double maximum = 0;
  size_t length = strlen(input);
  if (length > IVIDEO_SUBTITLE_LIMIT || !iVideoSubtitleUTF8(input))
    return 0;
  data = iupStrDup(input);
  if (!data)
    return 0;
  p = data;
  write = data;
  if (length >= 3 && memcmp(p, "\xef\xbb\xbf", 3) == 0)
    p += 3;
  while (*p)
  {
    if (*p == '\r')
    {
      *write = '\n';
      write++;
      p++;
      if (*p == '\n')
        p++;
    }
    else
    {
      *write = *p;
      write++;
      p++;
    }
  }
  *write = 0;
  p = data;
  write = data;
  while (*p)
  {
    char* end = strchr(p, '\n');
    char* space = p;
    if (!end)
      end = p + strlen(p);
    while (space < end && (*space == ' ' || *space == '\t'))
      space++;
    if (space != end)
    {
      size_t n = (size_t)(end - p);
      memmove(write, p, n);
      write += n;
    }
    if (!*end)
      break;
    *write = '\n';
    write++;
    p = end + 1;
  }
  *write = 0;
  p = data;
  webvtt = strncmp(p, "WEBVTT", 6) == 0 && (p[6] == 0 || p[6] == '\n' || p[6] == ' ' || p[6] == '\t');
  while (*p)
  {
    char* line;
    char* end;
    char* arrow;
    char* text;
    const char* tail;
    double start, finish;
    while (*p == '\n')
      p++;
    if (!*p)
      break;
    line = p;
    while (*p && !(p[0] == '\n' && p[1] == '\n'))
      p++;
    if (*p)
    {
      *p = 0;
      p += 2;
    }
    end = strchr(line, '\n');
    arrow = strstr(line, "-->");
    if (!arrow || (end && arrow > end))
    {
      if (!end)
        continue;
      line = end + 1;
      end = strchr(line, '\n');
      arrow = strstr(line, "-->");
      if (!arrow || (end && arrow > end))
        continue;
    }
    if (!iVideoSubtitleTime(line, &start, &tail))
      continue;
    while (*tail == ' ' || *tail == '\t')
      tail++;
    if (tail != arrow || !iVideoSubtitleTime(arrow + 3, &finish, &tail) ||
        (*tail && *tail != ' ' && *tail != '\t' && *tail != '\n'))
      continue;
    parsed++;
    if (finish <= start || !end)
      continue;
    text = iVideoSubtitlePlain(end + 1, strlen(end + 1));
    if (!text)
      goto failure;
    if (!*text)
    {
      free(text);
      continue;
    }
    if (size == capacity)
    {
      IvideoSubtitle* allocation;
      int next = capacity ? capacity * 2 : 64;
      if (next > 131072)
      {
        free(text);
        goto failure;
      }
      allocation = (IvideoSubtitle*)realloc(cues, (size_t)next * sizeof(IvideoSubtitle));
      if (!allocation)
      {
        free(text);
        goto failure;
      }
      cues = allocation;
      capacity = next;
    }
    cues[size].start = start;
    cues[size].end = finish;
    cues[size].order = size;
    cues[size].text = text;
    size++;
  }
  if (!parsed && !webvtt)
    goto failure;
  if (size > 1)
    qsort(cues, (size_t)size, sizeof(IvideoSubtitle), iVideoSubtitleCompare);
  for (i = 0; i < size; i++)
  {
    if (cues[i].end > maximum)
      maximum = cues[i].end;
    cues[i].maximum = maximum;
  }
  free(data);
  *output = cues;
  *count = size;
  return 1;
failure:
  free(data);
  iVideoFreeSubtitles(cues, size);
  return 0;
}

static void iVideoSelectSubtitle(Ihandle* ih)
{
  IvideoSession* session = ih->data->session;
  if (session && ih->data->ready && session->backend->subtitle_select)
  {
    int index = ih->data->subtitle == -2 ? 0 : ih->data->subtitle;
    if (!session->backend->subtitle_select(session, index, ih->data->subtitle_delay))
      iVideoSubtitleError(ih, "The subtitle track is not available");
  }
  ih->data->subtitle_dirty = 1;
}

static void iVideoUpdateSubtitle(Ihandle* ih)
{
  IvideoSession* session = ih->data->session;
  char* text = NULL;
  if (!session)
    return;
  if (ih->data->subtitle == -2 && ih->data->ready && ih->data->state != IVIDEO_ERROR &&
      ih->data->state != IVIDEO_STOPPED && !ih->data->pending_seek)
  {
    double time = ih->data->info.position - ih->data->subtitle_delay;
    int low = 0, high = ih->data->subtitle_count, i, first;
    size_t length = 0;
    while (low < high)
    {
      int mid = low + (high - low) / 2;
      if (ih->data->subtitles[mid].maximum <= time)
        low = mid + 1;
      else
        high = mid;
    }
    first = low;
    for (i = first; i < ih->data->subtitle_count && ih->data->subtitles[i].start <= time; i++)
    {
      if (time < ih->data->subtitles[i].end)
        length += strlen(ih->data->subtitles[i].text) + 1;
    }
    if (length)
    {
      size_t offset = 0;
      text = (char*)malloc(length);
      if (text)
      {
        for (i = first; i < ih->data->subtitle_count && ih->data->subtitles[i].start <= time; i++)
        {
          if (time < ih->data->subtitles[i].end)
          {
            size_t n = strlen(ih->data->subtitles[i].text);
            if (offset)
            {
              text[offset] = '\n';
              offset++;
            }
            memcpy(text + offset, ih->data->subtitles[i].text, n);
            offset += n;
          }
        }
        text[offset] = 0;
      }
    }
  }
  else if (ih->data->subtitle != 0 && ih->data->subtitle != -2)
  {
    iupdrvMutexLock(session->mutex);
    text = iupStrDup(session->subtitle_cue);
    iupdrvMutexUnlock(session->mutex);
  }
  if (!iupStrEqual(text, ih->data->subtitle_text) || ih->data->subtitle_dirty)
  {
    free(ih->data->subtitle_text);
    ih->data->subtitle_text = text;
    text = NULL;
    ih->data->subtitle_dirty = 0;
    free(ih->data->subtitle_wrapped);
    ih->data->subtitle_wrapped = NULL;
    if (session->backend->subtitle_text)
      ih->data->subtitle_native = session->backend->subtitle_text(session, ih->data->subtitle_text, ih->data->subtitle_size, ih->data->subtitle_color);
    IupUpdate(ih);
  }
  free(text);
}

void iupVideoInfo(IvideoSession* session, const IvideoInfo* info)
{
  iupdrvMutexLock(session->mutex);
  if (!session->closing && !session->info.error[0])
  {
    int ended = session->info.ended;
    session->info = *info;
    session->info.ended |= ended;
  }
  iupdrvMutexUnlock(session->mutex);
}

void iupVideoError(IvideoSession* session, const char* message)
{
  iupdrvMutexLock(session->mutex);
  if (!session->closing)
  {
    session->info.state = IVIDEO_ERROR;
    iupStrCopyN(session->info.error, sizeof(session->info.error), message ? message : "Video playback failed");
  }
  iupdrvMutexUnlock(session->mutex);
}

void iupVideoFrame(IvideoSession* session, const unsigned char* pixels, int width, int height, size_t stride, int channels)
{
  size_t size;
  int x, y;
  if (!pixels || width <= 0 || height <= 0 || (channels != 3 && channels != 4) ||
      width > INT_MAX / channels || height > INT_MAX / 3 / width || stride < (size_t)width * channels)
    return;
  size = (size_t)width * height * 3;
  iupdrvMutexLock(session->mutex);
  if (session->closing)
  {
    iupdrvMutexUnlock(session->mutex);
    return;
  }
  if (session->pending_width != width || session->pending_height != height)
  {
    unsigned char* buffer = (unsigned char*)realloc(session->pending, size);
    if (!buffer)
    {
      session->info.state = IVIDEO_ERROR;
      iupStrCopyN(session->info.error, sizeof(session->info.error), "Cannot allocate video frame");
      iupdrvMutexUnlock(session->mutex);
      return;
    }
    session->pending = buffer;
    session->pending_width = width;
    session->pending_height = height;
  }
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = pixels + (size_t)y * stride;
    unsigned char* output = session->pending + (size_t)y * width * 3;
    if (channels == 3)
      memcpy(output, row, (size_t)width * 3);
    else
    {
      for (x = 0; x < width; x++)
      {
        output[x * 3] = row[x * 4];
        output[x * 3 + 1] = row[x * 4 + 1];
        output[x * 3 + 2] = row[x * 4 + 2];
      }
    }
  }
  session->pending_ready = 1;
  iupdrvMutexUnlock(session->mutex);
}

static void iVideoReleaseImage(Ihandle* ih)
{
  if (ih->data->image)
  {
    IupSetHandle(ih->data->image_name, NULL);
    IupDestroy(ih->data->image);
    ih->data->image = NULL;
  }
  free(ih->data->frame);
  ih->data->frame = NULL;
  ih->data->frame_width = ih->data->frame_height = 0;
}

static int iVideoInhibitCount = 0;

static void iVideoInhibit(Ihandle* ih, int inhibit)
{
  if (ih->data->inhibit == inhibit)
    return;
  ih->data->inhibit = inhibit;
  if (inhibit)
  {
    if (iVideoInhibitCount++ == 0)
      iupdrvVideoInhibitScreenSaver(1);
  }
  else if (--iVideoInhibitCount == 0)
    iupdrvVideoInhibitScreenSaver(0);
}

static void iVideoClose(Ihandle* ih)
{
  IvideoSession* session = ih->data->session;
  iVideoInhibit(ih, 0);
  if (ih->data->lifetime)
    ih->data->lifetime->generation++;
  ih->data->session = NULL;
  if (session)
  {
    iupdrvMutexLock(session->mutex);
    session->closing = 1;
    iupdrvMutexUnlock(session->mutex);
    session->backend->close(session);
    free(session->pending);
    free(session->subtitle_cue);
    iupdrvMutexDestroy(session->mutex);
    free(session);
  }
  ih->data->ready = ih->data->ended = ih->data->reported_error = 0;
  memset(&ih->data->info, 0, sizeof(ih->data->info));
  iVideoReleaseImage(ih);
  free(ih->data->subtitle_text);
  ih->data->subtitle_text = NULL;
  free(ih->data->subtitle_wrapped);
  ih->data->subtitle_wrapped = NULL;
  ih->data->subtitle_dirty = 1;
  ih->data->subtitle_tracks = -1;
  ih->data->subtitle_ticks = 0;
}

static void iVideoVolume(Ihandle* ih, int volume, int mute)
{
  IvideoSession* session = ih->data->session;
  if (session)
    session->backend->volume(session, volume, mute);
}

static void iVideoSettings(Ihandle* ih)
{
  iVideoVolume(ih, iupAttribGetInt(ih, "VOLUME"), iupAttribGetBoolean(ih, "MUTE"));
}

static void iVideoStart(Ihandle* ih)
{
  const IvideoBackend* backend;
  char error[512] = "";
  int index;
  for (index = 0; (backend = iupdrvVideoBackend(index)) != NULL; index++)
  {
    IvideoSession* session;
    if (!iupStrEqualNoCase(ih->data->backend, "AUTO") && !iupStrEqualNoCase(ih->data->backend, backend->name))
      continue;
    if (!backend->available())
      continue;
    if (ih->data->audio && !backend->audio)
    {
      if (!error[0])
        iupStrCopyN(error, sizeof(error), "The video backend cannot play AUDIOURL");
      continue;
    }
    session = (IvideoSession*)calloc(1, sizeof(IvideoSession));
    if (!session)
      break;
    session->mutex = iupdrvMutexCreate();
    session->backend = backend;
    session->audio = ih->data->audio;
    session->info.state = IVIDEO_LOADING;
    ih->data->session = session;
    if (backend->open(session, ih, ih->data->source, ih->data->url))
    {
      session->audio = NULL;
      iVideoSettings(ih);
      return;
    }
    iupdrvMutexLock(session->mutex);
    iupStrCopyN(error, sizeof(error), session->info.error);
    iupdrvMutexUnlock(session->mutex);
    iVideoClose(ih);
  }
  ih->data->state = IVIDEO_ERROR;
  iupStrCopyN(ih->data->info.error, sizeof(ih->data->info.error), error[0] ? error : "No usable video backend");
}

static void iVideoOpen(Ihandle* ih)
{
  iVideoClose(ih);
  if (!ih->data->source)
    ih->data->state = IVIDEO_EMPTY;
  else
    ih->data->state = ih->handle ? IVIDEO_LOADING : IVIDEO_STOPPED;
  if (!ih->handle)
    return;
  if (ih->data->source)
    iVideoStart(ih);
  IupSetAttribute(ih->data->timer, "RUN", "YES");
}

static int iVideoCallback(Ihandle* ih, IvideoLifetime* lifetime, const char* name)
{
  unsigned long generation = lifetime->generation;
  Icallback cb = IupGetCallback(ih, name);
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
  return lifetime->ih != NULL && lifetime->generation == generation;
}

static void iVideoTakeFrame(Ihandle* ih, IvideoSession* session)
{
  unsigned char* old;
  int width, height, changed = 0;
  iupdrvMutexLock(session->mutex);
  if (session->pending_ready)
  {
    old = ih->data->frame;
    width = ih->data->frame_width;
    height = ih->data->frame_height;
    ih->data->frame = session->pending;
    ih->data->frame_width = session->pending_width;
    ih->data->frame_height = session->pending_height;
    session->pending = old;
    session->pending_width = width;
    session->pending_height = height;
    session->pending_ready = 0;
    changed = 1;
  }
  iupdrvMutexUnlock(session->mutex);
  if (!changed)
    return;
  width = ih->data->frame_width;
  height = ih->data->frame_height;
  if (ih->data->image && (IupGetInt(ih->data->image, "WIDTH") != width || IupGetInt(ih->data->image, "HEIGHT") != height))
  {
    IupSetHandle(ih->data->image_name, NULL);
    IupDestroy(ih->data->image);
    ih->data->image = NULL;
  }
  if (!ih->data->image)
  {
    ih->data->image = IupImageRGB(width, height, ih->data->frame);
    if (ih->data->image)
      IupSetHandle(ih->data->image_name, ih->data->image);
  }
  else
  {
    unsigned char* pixels = (unsigned char*)iupAttribGet(ih->data->image, "WID");
    memcpy(pixels, ih->data->frame, (size_t)width * height * 3);
    IupSetAttribute(ih->data->image, "CLEARCACHE", "YES");
  }
  IupUpdate(ih);
}

static int iVideoTick(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUPVIDEO");
  IvideoLifetime* lifetime = ih->data->lifetime;
  IvideoSession* session = ih->data->session;
  IvideoInfo info;
  unsigned long generation = lifetime->generation;
  int previous = ih->data->reported_state;
  lifetime->references++;
  if (session)
  {
    char subtitle_error[512] = "";
    session->backend->poll(session);
    iupdrvMutexLock(session->mutex);
    info = session->info;
    session->info.ended = 0;
    iupStrCopyN(subtitle_error, sizeof(subtitle_error), session->subtitle_error);
    session->subtitle_error[0] = 0;
    iupdrvMutexUnlock(session->mutex);
    if (subtitle_error[0])
      iVideoSubtitleError(ih, subtitle_error);
    ih->data->info = info;
    iVideoTakeFrame(ih, session);
    if (!lifetime->ih || lifetime->generation != generation)
      goto done;
    if (info.error[0])
      ih->data->state = IVIDEO_ERROR;
    else if (info.ready)
    {
      int first = !ih->data->ready;
      ih->data->ready = 1;
      if (first)
      {
        iVideoSettings(ih);
        iVideoSelectSubtitle(ih);
        if (session->backend->subtitle_count)
          ih->data->subtitle_tracks = session->backend->subtitle_count(session);
        ih->data->subtitle_ticks = 0;
        session->backend->play(session, ih->data->intent == 2);
        ih->data->state = ih->data->intent == 2 ? IVIDEO_BUFFERING : (ih->data->intent ? IVIDEO_PAUSED : IVIDEO_STOPPED);
        if (!iVideoCallback(ih, lifetime, "READY_CB"))
          goto done;
      }
      else if (!ih->data->ended)
        ih->data->state = ih->data->intent == 2 ? info.state : (ih->data->intent ? IVIDEO_PAUSED : IVIDEO_STOPPED);
      if (ih->data->pending_seek)
      {
        if (info.seekable)
          session->backend->seek(session, ih->data->seek_position);
        ih->data->pending_seek = 0;
        info.ended = 0;
      }
      if (info.ended && !ih->data->ended)
      {
        if (iupAttribGetBoolean(ih, "LOOP") && info.seekable)
        {
          session->backend->seek(session, 0);
          session->backend->play(session, ih->data->intent == 2);
        }
        else
        {
          ih->data->ended = 1;
          ih->data->state = IVIDEO_ENDED;
          if (!iVideoCallback(ih, lifetime, "PLAYEND_CB"))
            goto done;
        }
      }
    }
  }
  iVideoInhibit(ih, ih->data->session && ih->data->info.video && (ih->data->state == IVIDEO_PLAYING || ih->data->state == IVIDEO_BUFFERING));
  if (previous != ih->data->state)
  {
    IvideoStringCallback cb = (IvideoStringCallback)IupGetCallback(ih, "STATECHANGED_CB");
    ih->data->reported_state = ih->data->state;
    if (cb && cb(ih, iVideoStates[ih->data->state]) == IUP_CLOSE)
      IupExitLoop();
    if (!lifetime->ih || lifetime->generation != generation)
      goto done;
  }
  iVideoUpdateSubtitle(ih);
  if (session && ih->data->ready && session->backend->subtitle_count && ++ih->data->subtitle_ticks >= 16)
  {
    int count = session->backend->subtitle_count(session);
    ih->data->subtitle_ticks = 0;
    if (count != ih->data->subtitle_tracks)
    {
      ih->data->subtitle_tracks = count;
      iVideoSelectSubtitle(ih);
    }
  }
  if (ih->data->subtitle_error_pending)
  {
    IvideoStringCallback cb = (IvideoStringCallback)IupGetCallback(ih, "ERROR_CB");
    char message[512];
    iupStrCopyN(message, sizeof(message), ih->data->subtitle_error);
    ih->data->subtitle_error_pending = 0;
    if (cb && cb(ih, message) == IUP_CLOSE)
      IupExitLoop();
    if (!lifetime->ih || lifetime->generation != generation)
      goto done;
  }
  if (ih->data->state == IVIDEO_ERROR && !ih->data->reported_error)
  {
    IvideoStringCallback cb = (IvideoStringCallback)IupGetCallback(ih, "ERROR_CB");
    char message[512];
    iupStrCopyN(message, sizeof(message), ih->data->info.error);
    ih->data->reported_error = 1;
    if (cb && cb(ih, message) == IUP_CLOSE)
      IupExitLoop();
    if (!lifetime->ih || lifetime->generation != generation)
      goto done;
  }
  if (!ih->data->session)
    IupSetAttribute(timer, "RUN", "NO");
done:
  if (--lifetime->references == 0)
    free(lifetime);
  return IUP_DEFAULT;
}

static char* iVideoWrapSubtitle(Ihandle* ih, const char* text, int width)
{
  size_t length = strlen(text), used = 0;
  char* result = (char*)malloc(length * 2 + 1);
  if (!result)
    return NULL;
  while (*text)
  {
    const char* end = strchr(text, '\n');
    size_t count = end ? (size_t)(end - text) : strlen(text);
    while (count)
    {
      int w, h;
      size_t cut = count;
      IupDrawGetTextSize(ih, text, (int)count, &w, &h);
      if (w > width)
      {
        size_t low = 0, high = count;
        while (high > low + 1)
        {
          size_t mid = low + (high - low) / 2;
          while (mid > low && ((unsigned char)text[mid] & 0xc0) == 0x80)
            mid--;
          if (mid == low)
          {
            mid++;
            while (mid < count && ((unsigned char)text[mid] & 0xc0) == 0x80)
              mid++;
            if (mid >= high)
              break;
          }
          IupDrawGetTextSize(ih, text, (int)mid, &w, &h);
          if (w <= width)
            low = mid;
          else
            high = mid;
        }
        cut = low;
        if (!cut)
        {
          cut = 1;
          while (cut < count && ((unsigned char)text[cut] & 0xc0) == 0x80)
            cut++;
        }
        else
        {
          size_t space = cut;
          while (space && text[space - 1] != ' ' && text[space - 1] != '\t')
            space--;
          if (space)
            cut = space;
        }
      }
      memcpy(result + used, text, cut);
      used += cut;
      text += cut;
      count -= cut;
      while (used && (result[used - 1] == ' ' || result[used - 1] == '\t'))
        used--;
      while (count && (*text == ' ' || *text == '\t'))
      {
        text++;
        count--;
      }
      if (count)
      {
        result[used] = '\n';
        used++;
      }
    }
    if (!end)
      break;
    result[used] = '\n';
    used++;
    text = end + 1;
  }
  result[used] = 0;
  return result;
}

static int iVideoRedraw(Ihandle* ih)
{
  int w, h, x, y;
  int vx = 0, vy = 0, vw, vh;
  double aspect = ih->data->info.aspect;
  IupDrawBegin(ih);
  IupDrawGetSize(ih, &w, &h);
  iupAttribSetStr(ih, "DRAWCOLOR", iupAttribGetStr(ih, "BGCOLOR"));
  iupAttribSet(ih, "DRAWSTYLE", "FILL");
  IupDrawRectangle(ih, 0, 0, w - 1, h - 1);
  if ((!(aspect > 0) || !isfinite(aspect)) && ih->data->image)
    aspect = (double)ih->data->frame_width / ih->data->frame_height;
  vw = w;
  vh = h;
  if (aspect > 0 && isfinite(aspect) && w > 0 && h > 0)
  {
    if (aspect * h > w)
    {
      vh = (int)(w / aspect);
      vy = (h - vh) / 2;
    }
    else
    {
      vw = (int)(h * aspect);
      vx = (w - vw) / 2;
    }
  }
  if (ih->data->image && vw > 0 && vh > 0)
    IupDrawImage(ih, ih->data->image_name, vx, vy, vw, vh);
  if (ih->data->subtitle_text && !ih->data->subtitle_native && vw > 20 && vh > 20)
  {
    char font[80];
    const char* text;
    int tw, th, dx, dy;
    snprintf(font, sizeof(font), "Sans, Bold %d", ih->data->subtitle_size);
    iupAttribSetStr(ih, "DRAWFONT", font);
    iupAttribSet(ih, "DRAWTEXTALIGNMENT", "ACENTER");
    iupAttribSet(ih, "DRAWTEXTWRAP", "NO");
    if (!ih->data->subtitle_wrapped || ih->data->subtitle_wrap_width != vw - 20)
    {
      free(ih->data->subtitle_wrapped);
      ih->data->subtitle_wrapped = iVideoWrapSubtitle(ih, ih->data->subtitle_text, vw - 20);
      ih->data->subtitle_wrap_width = vw - 20;
    }
    text = ih->data->subtitle_wrapped ? ih->data->subtitle_wrapped : ih->data->subtitle_text;
    IupDrawGetTextSize(ih, text, -1, &tw, &th);
    tw = vw - 20;
    x = vx + 10;
    y = vy + vh - th - 10;
    IupDrawSetClipRect(ih, vx, vy, vx + vw - 1, vy + vh - 1);
    iupAttribSet(ih, "DRAWCOLOR", "0 0 0");
    for (dy = -1; dy <= 1; dy++)
    {
      for (dx = -1; dx <= 1; dx++)
      {
        if (dx || dy)
          IupDrawText(ih, text, -1, x + dx, y + dy, tw, th);
      }
    }
    iupAttribSetStr(ih, "DRAWCOLOR", ih->data->subtitle_color);
    IupDrawText(ih, text, -1, x, y, tw, th);
  }
  IupDrawEnd(ih);
  return IUP_DEFAULT;
}

static int iVideoSetSource(Ihandle* ih, const char* value, int url)
{
  char* source = value ? iupStrDup(value) : NULL;
  if (value && !source)
    return 0;
  iVideoFreeSubtitles(ih->data->subtitles, ih->data->subtitle_count);
  ih->data->subtitles = NULL;
  ih->data->subtitle_count = 0;
  ih->data->subtitle_loaded = 0;
  free(ih->data->subtitle_file);
  ih->data->subtitle_file = NULL;
  if (ih->data->subtitle > 0 || ih->data->subtitle == -2)
    ih->data->subtitle = -1;
  ih->data->subtitle_error[0] = 0;
  ih->data->subtitle_error_pending = 0;
  free(ih->data->source);
  ih->data->source = source;
  ih->data->url = url;
  ih->data->intent = 0;
  ih->data->pending_seek = 0;
  ih->data->seek_position = 0;
  iVideoOpen(ih);
  if (ih->handle)
    IupUpdate(ih);
  return 0;
}

static int iVideoSetFileAttrib(Ihandle* ih, const char* value)
{
  return iVideoSetSource(ih, value, 0);
}

static int iVideoSetUrlAttrib(Ihandle* ih, const char* value)
{
  return iVideoSetSource(ih, value, 1);
}

static char* iVideoGetFileAttrib(Ihandle* ih)
{
  return ih->data->url ? NULL : ih->data->source;
}

static char* iVideoGetUrlAttrib(Ihandle* ih)
{
  return ih->data->url ? ih->data->source : NULL;
}

static char* iVideoGetStateAttrib(Ihandle* ih)
{
  return (char*)iVideoStates[ih->data->state];
}

static char* iVideoGetBackendAttrib(Ihandle* ih)
{
  return ih->data->backend;
}

static char* iVideoGetBackendUsedAttrib(Ihandle* ih)
{
  return ih->data->session ? (char*)ih->data->session->backend->name : NULL;
}

static char* iVideoGetDurationAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->info.duration);
}

static char* iVideoGetPositionAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->info.position);
}

static char* iVideoGetSeekableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->info.seekable);
}

static char* iVideoGetHasVideoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->info.video);
}

static char* iVideoGetHasAudioAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->info.audio);
}

static char* iVideoGetVideoSizeAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->info.width, ih->data->info.height, 'x');
}

static int iVideoSetBackendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->session && value && (iupStrEqualNoCase(value, "AUTO") || iupStrEqualNoCase(value, "NATIVE") ||
      iupStrEqualNoCase(value, "VLC") || iupStrEqualNoCase(value, "GSTREAMER") || iupStrEqualNoCase(value, "MPV")))
  {
    iupStrCopyN(ih->data->backend, sizeof(ih->data->backend), value);
    iupStrUpper(ih->data->backend, ih->data->backend);
  }
  return 0;
}

static int iVideoSetAudioUrlAttrib(Ihandle* ih, const char* value)
{
  char* audio = value ? iupStrDup(value) : NULL;
  if (value && !audio)
    return 0;
  free(ih->data->audio);
  ih->data->audio = audio;
  return 0;
}

static char* iVideoGetAudioUrlAttrib(Ihandle* ih)
{
  return ih->data->audio;
}

static char* iVideoGetAvailableAttrib(Ihandle* ih)
{
  int index;
  const IvideoBackend* backend;
  for (index = 0; (backend = iupdrvVideoBackend(index)) != NULL; index++)
    if ((iupStrEqualNoCase(ih->data->backend, "AUTO") || iupStrEqualNoCase(ih->data->backend, backend->name)) &&
        (!ih->data->audio || backend->audio) && backend->available())
      return "YES";
  return "NO";
}

static int iVideoSetPlayAttrib(Ihandle* ih, const char* value)
{
  IvideoSession* session = ih->data->session;
  (void)value;
  if (!ih->data->source)
    return 0;
  ih->data->intent = 2;
  if (session && ih->data->ready && ih->data->state != IVIDEO_ERROR)
  {
    if (ih->data->ended)
      session->backend->seek(session, 0);
    ih->data->ended = 0;
    session->backend->play(session, 1);
  }
  return 0;
}

static int iVideoSetPauseAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (!ih->data->source)
    return 0;
  ih->data->intent = 1;
  if (ih->data->session && ih->data->ready)
    ih->data->session->backend->play(ih->data->session, 0);
  return 0;
}

static int iVideoSetStopAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  ih->data->intent = 0;
  ih->data->ended = 0;
  ih->data->pending_seek = 1;
  ih->data->seek_position = 0;
  if (ih->data->session && ih->data->ready)
  {
    ih->data->session->backend->play(ih->data->session, 0);
    if (ih->data->info.seekable)
      ih->data->session->backend->seek(ih->data->session, 0);
    ih->data->pending_seek = 0;
  }
  return 0;
}

static int iVideoSetPositionAttrib(Ihandle* ih, const char* value)
{
  double position;
  if (iupStrToDouble(value, &position) && isfinite(position) && position >= 0)
  {
    ih->data->pending_seek = 1;
    ih->data->seek_position = position;
    ih->data->ended = 0;
  }
  return 0;
}

static int iVideoSetVolumeAttrib(Ihandle* ih, const char* value)
{
  int volume;
  if (!iupStrToInt(value, &volume) || volume < 0)
    return 0;
  if (volume > 100)
    volume = 100;
  iVideoVolume(ih, volume, iupAttribGetBoolean(ih, "MUTE"));
  iupAttribSetInt(ih, "VOLUME", volume);
  return 0;
}

static int iVideoSetMuteAttrib(Ihandle* ih, const char* value)
{
  iVideoVolume(ih, iupAttribGetInt(ih, "VOLUME"), iupStrBoolean(value));
  return 1;
}

static int iVideoLoadSubtitle(Ihandle* ih, const char* value, const char* filename)
{
  IvideoSubtitle* cues = NULL;
  int count = 0;
  char* path = NULL;
  if (value && strlen(value) > IVIDEO_SUBTITLE_LIMIT)
  {
    iVideoSubtitleError(ih, "Subtitles exceed the maximum size of 16 MiB");
    return 0;
  }
  if (value && !iVideoParseSubtitles(value, &cues, &count))
  {
    iVideoSubtitleError(ih, "Cannot parse subtitles: expected UTF-8 SRT or WebVTT");
    return 0;
  }
  if (filename)
  {
    path = iupStrDup(filename);
    if (!path)
    {
      iVideoFreeSubtitles(cues, count);
      return 0;
    }
  }
  iVideoFreeSubtitles(ih->data->subtitles, ih->data->subtitle_count);
  free(ih->data->subtitle_file);
  ih->data->subtitle_file = path;
  ih->data->subtitles = cues;
  ih->data->subtitle_count = count;
  ih->data->subtitle_loaded = value != NULL;
  ih->data->subtitle_error[0] = 0;
  ih->data->subtitle_error_pending = 0;
  if (value)
    ih->data->subtitle = -2;
  else if (ih->data->subtitle == -2)
    ih->data->subtitle = -1;
  iVideoSelectSubtitle(ih);
  if (ih->handle)
    iVideoUpdateSubtitle(ih);
  return 0;
}

static int iVideoSetSubtitleFileAttrib(Ihandle* ih, const char* value)
{
  char* data;
  if (!value)
    return iVideoLoadSubtitle(ih, NULL, NULL);
  data = iupdrvVideoReadSubtitle(value);
  if (!data)
  {
    iVideoSubtitleError(ih, "Cannot read the subtitle file");
    return 0;
  }
  iVideoLoadSubtitle(ih, data, value);
  free(data);
  return 0;
}

static int iVideoSetSubtitleDataAttrib(Ihandle* ih, const char* value)
{
  return iVideoLoadSubtitle(ih, value, NULL);
}

static char* iVideoGetSubtitleFileAttrib(Ihandle* ih)
{
  return ih->data->subtitle_file;
}

static char* iVideoGetSubtitleTextAttrib(Ihandle* ih)
{
  return ih->data->subtitle_text;
}

static char* iVideoGetSubtitleErrorAttrib(Ihandle* ih)
{
  return ih->data->subtitle_error[0] ? ih->data->subtitle_error : NULL;
}

static char* iVideoGetSubtitleAttrib(Ihandle* ih)
{
  if (ih->data->subtitle == -1)
    return "AUTO";
  if (ih->data->subtitle == -2)
    return "EXTERNAL";
  if (!ih->data->subtitle)
    return "OFF";
  return iupStrReturnInt(ih->data->subtitle);
}

static int iVideoSetSubtitleAttrib(Ihandle* ih, const char* value)
{
  int index;
  if (!value || iupStrEqualNoCase(value, "AUTO"))
    index = -1;
  else if (iupStrEqualNoCase(value, "OFF"))
    index = 0;
  else if (iupStrEqualNoCase(value, "EXTERNAL"))
    index = -2;
  else if (!iupStrToInt(value, &index) || index <= 0)
    return 0;
  if (index == -2 && !ih->data->subtitle_loaded)
  {
    iVideoSubtitleError(ih, "No external subtitles are loaded");
    return 0;
  }
  if (index > 0 && ih->data->ready && ih->data->session && ih->data->session->backend->subtitle_count &&
      index > ih->data->session->backend->subtitle_count(ih->data->session))
  {
    iVideoSubtitleError(ih, "The subtitle track is not available");
    return 0;
  }
  ih->data->subtitle = index;
  ih->data->subtitle_error[0] = 0;
  ih->data->subtitle_error_pending = 0;
  iVideoSelectSubtitle(ih);
  if (ih->handle)
    iVideoUpdateSubtitle(ih);
  return 0;
}

static char* iVideoGetSubtitleCountAttrib(Ihandle* ih)
{
  IvideoSession* session = ih->data->session;
  return iupStrReturnInt(session && ih->data->ready && session->backend->subtitle_count ? session->backend->subtitle_count(session) : 0);
}

static char* iVideoGetSubtitleNameAttrib(Ihandle* ih, int id)
{
  IvideoSession* session = ih->data->session;
  return session && ih->data->ready && session->backend->subtitle_name ? session->backend->subtitle_name(session, id, 0) : NULL;
}

static char* iVideoGetSubtitleLanguageAttrib(Ihandle* ih, int id)
{
  IvideoSession* session = ih->data->session;
  return session && ih->data->ready && session->backend->subtitle_name ? session->backend->subtitle_name(session, id, 1) : NULL;
}

static char* iVideoGetSubtitleDelayAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->subtitle_delay);
}

static int iVideoSetSubtitleDelayAttrib(Ihandle* ih, const char* value)
{
  double delay = 0;
  if (value && (!iupStrToDouble(value, &delay) || !isfinite(delay) || fabs(delay) > 86400))
    return 0;
  ih->data->subtitle_delay = delay;
  iVideoSelectSubtitle(ih);
  if (ih->handle)
    iVideoUpdateSubtitle(ih);
  return 0;
}

static char* iVideoGetSubtitleFontSizeAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->subtitle_size);
}

static int iVideoSetSubtitleFontSizeAttrib(Ihandle* ih, const char* value)
{
  int size = 18;
  if (value && (!iupStrToInt(value, &size) || size < 6 || size > 96))
    return 0;
  ih->data->subtitle_size = size;
  ih->data->subtitle_dirty = 1;
  if (ih->handle)
    iVideoUpdateSubtitle(ih);
  return 0;
}

static char* iVideoGetSubtitleColorAttrib(Ihandle* ih)
{
  return ih->data->subtitle_color;
}

static int iVideoSetSubtitleColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r = 255, g = 255, b = 255;
  if (value && !iupStrToRGB(value, &r, &g, &b))
    return 0;
  snprintf(ih->data->subtitle_color, sizeof(ih->data->subtitle_color), "%u %u %u", r, g, b);
  ih->data->subtitle_dirty = 1;
  if (ih->handle)
    iVideoUpdateSubtitle(ih);
  return 0;
}

static int iVideoCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();
  if (!ih->data)
    return IUP_ERROR;
  ih->data->lifetime = (IvideoLifetime*)calloc(1, sizeof(IvideoLifetime));
  if (!ih->data->lifetime)
  {
    free(ih->data);
    ih->data = NULL;
    return IUP_ERROR;
  }
  ih->data->lifetime->ih = ih;
  ih->data->lifetime->references = 1;
  strcpy(ih->data->backend, "AUTO");
  ih->data->subtitle = -1;
  ih->data->subtitle_size = 18;
  strcpy(ih->data->subtitle_color, "255 255 255");
  snprintf(ih->data->image_name, sizeof(ih->data->image_name), "_IUP_VIDEO_%p", (void*)ih);
  ih->data->timer = IupTimer();
  if (!ih->data->timer)
  {
    free(ih->data->lifetime);
    free(ih->data);
    ih->data = NULL;
    return IUP_ERROR;
  }
  iupAttribSet(ih->data->timer, "_IUPVIDEO", (char*)ih);
  IupSetInt(ih->data->timer, "TIME", 16);
  IupSetCallback(ih->data->timer, "ACTION_CB", (Icallback)iVideoTick);
  IupSetCallback(ih, "ACTION", (Icallback)iVideoRedraw);
  return IUP_NOERROR;
}

static int iVideoMapMethod(Ihandle* ih)
{
  iVideoOpen(ih);
  return IUP_NOERROR;
}

static void iVideoUnMapMethod(Ihandle* ih)
{
  IupSetAttribute(ih->data->timer, "RUN", "NO");
  iVideoClose(ih);
  ih->data->intent = 0;
  ih->data->pending_seek = 0;
  ih->data->state = ih->data->source ? IVIDEO_STOPPED : IVIDEO_EMPTY;
}

static void iVideoDestroyMethod(Ihandle* ih)
{
  IvideoLifetime* lifetime = ih->data->lifetime;
  if (lifetime)
    lifetime->ih = NULL;
  iVideoClose(ih);
  if (ih->data->timer)
    IupDestroy(ih->data->timer);
  free(ih->data->source);
  free(ih->data->audio);
  free(ih->data->subtitle_file);
  iVideoFreeSubtitles(ih->data->subtitles, ih->data->subtitle_count);
  if (lifetime && --lifetime->references == 0)
    free(lifetime);
}

static void iVideoLayoutUpdateMethod(Ihandle* ih)
{
  IvideoSession* session = ih->data->session;
  if (session && session->backend->layout)
    session->backend->layout(session, ih);
}

static void iVideoComputeNaturalSizeMethod(Ihandle* ih, int* width, int* height, int* expand)
{
  int cw, ch;
  (void)expand;
  iupdrvFontGetCharSize(ih, &cw, &ch);
  *width = cw * 48;
  *height = *width * 9 / 16;
}

IUPMEDIA_API Ihandle* IupVideo(void)
{
  return IupCreate("video");
}

Iclass* iupVideoNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));
  int flags = IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT;
  ic->name = "video";
  ic->cons = "Video";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;
  ic->New = iupVideoNewClass;
  ic->Create = iVideoCreateMethod;
  ic->Map = iVideoMapMethod;
  ic->UnMap = iVideoUnMapMethod;
  ic->Destroy = iVideoDestroyMethod;
  ic->LayoutUpdate = iVideoLayoutUpdateMethod;
  ic->ComputeNaturalSize = iVideoComputeNaturalSizeMethod;

  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribDef(ic, "BGCOLOR", "0 0 0", NULL);
  iupClassRegisterReplaceAttribDef(ic, "CANFOCUS", "NO", NULL);

  iupClassRegisterCallback(ic, "READY_CB", "");
  iupClassRegisterCallback(ic, "STATECHANGED_CB", "s");
  iupClassRegisterCallback(ic, "PLAYEND_CB", "");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  iupClassRegisterAttribute(ic, "FILE", iVideoGetFileAttrib, iVideoSetFileAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "URL", iVideoGetUrlAttrib, iVideoSetUrlAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "BACKEND", iVideoGetBackendAttrib, iVideoSetBackendAttrib, IUPAF_SAMEASSYSTEM, "AUTO", flags);
  iupClassRegisterAttribute(ic, "AUDIOURL", iVideoGetAudioUrlAttrib, iVideoSetAudioUrlAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "MPVCONFIG", NULL, NULL, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "MPVOPTIONS", NULL, NULL, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "BACKENDUSED", iVideoGetBackendUsedAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "AVAILABLE", iVideoGetAvailableAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "STATE", iVideoGetStateAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "PLAY", NULL, iVideoSetPlayAttrib, NULL, NULL, flags | IUPAF_WRITEONLY);
  iupClassRegisterAttribute(ic, "PAUSE", NULL, iVideoSetPauseAttrib, NULL, NULL, flags | IUPAF_WRITEONLY);
  iupClassRegisterAttribute(ic, "STOP", NULL, iVideoSetStopAttrib, NULL, NULL, flags | IUPAF_WRITEONLY);
  iupClassRegisterAttribute(ic, "POSITION", iVideoGetPositionAttrib, iVideoSetPositionAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "DURATION", iVideoGetDurationAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "VIDEOSIZE", iVideoGetVideoSizeAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "SEEKABLE", iVideoGetSeekableAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "HASVIDEO", iVideoGetHasVideoAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "HASAUDIO", iVideoGetHasAudioAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "LOOP", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", flags);
  iupClassRegisterAttribute(ic, "VOLUME", NULL, iVideoSetVolumeAttrib, IUPAF_SAMEASSYSTEM, "100", flags);
  iupClassRegisterAttribute(ic, "MUTE", NULL, iVideoSetMuteAttrib, IUPAF_SAMEASSYSTEM, "NO", flags);
  iupClassRegisterAttribute(ic, "SUBTITLE", iVideoGetSubtitleAttrib, iVideoSetSubtitleAttrib, IUPAF_SAMEASSYSTEM, "AUTO", flags);
  iupClassRegisterAttribute(ic, "SUBTITLEFILE", iVideoGetSubtitleFileAttrib, iVideoSetSubtitleFileAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "SUBTITLEDATA", NULL, iVideoSetSubtitleDataAttrib, NULL, NULL, flags | IUPAF_WRITEONLY);
  iupClassRegisterAttribute(ic, "SUBTITLECOUNT", iVideoGetSubtitleCountAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttributeId(ic, "SUBTITLENAME", iVideoGetSubtitleNameAttrib, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttributeId(ic, "SUBTITLELANGUAGE", iVideoGetSubtitleLanguageAttrib, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "SUBTITLEDELAY", iVideoGetSubtitleDelayAttrib, iVideoSetSubtitleDelayAttrib, IUPAF_SAMEASSYSTEM, "0", flags);
  iupClassRegisterAttribute(ic, "SUBTITLEFONTSIZE", iVideoGetSubtitleFontSizeAttrib, iVideoSetSubtitleFontSizeAttrib, IUPAF_SAMEASSYSTEM, "18", flags);
  iupClassRegisterAttribute(ic, "SUBTITLECOLOR", iVideoGetSubtitleColorAttrib, iVideoSetSubtitleColorAttrib, IUPAF_SAMEASSYSTEM, "255 255 255", flags);
  iupClassRegisterAttribute(ic, "SUBTITLETEXT", iVideoGetSubtitleTextAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "SUBTITLEERROR", iVideoGetSubtitleErrorAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  return ic;
}

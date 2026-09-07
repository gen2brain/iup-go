/** \file
 * \brief Microphone control, Media Kit recorder on the audio input
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include <MediaDefs.h>
#include <MediaRecorder.h>
#include <MediaRoster.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_microphone.h"

class IhaikuRecorder : public BMediaRecorder
{
public:
  IhaikuRecorder(const char* name) : BMediaRecorder(name, B_MEDIA_RAW_AUDIO) {}
  using BMediaRecorder::MediaInput;
};

typedef struct _IhaikuMicrophone
{
  IhaikuRecorder* recorder;
  Ihandle* ih;
  media_raw_audio_format raw;
  short* convert;
  size_t convert_size;
  int stopping;
} IhaikuMicrophone;

static int haikuMicrophoneInput(media_node* node)
{
  BMediaRoster* roster = BMediaRoster::Roster();
  if (!roster)
    return 0;
  return roster->GetAudioInput(node) == B_OK;
}

int iupdrvMicrophoneIsAvailable(void)
{
  media_node node;
  if (!haikuMicrophoneInput(&node))
    return 0;
  BMediaRoster::Roster()->ReleaseNode(node);
  return 1;
}

int iupdrvMicrophoneGetDeviceCount(void)
{
  return iupdrvMicrophoneIsAvailable();
}

char* iupdrvMicrophoneGetDeviceName(int index)
{
  media_node node;
  live_node_info info;
  char* name = NULL;

  if (index != 0 || !haikuMicrophoneInput(&node))
    return NULL;
  if (BMediaRoster::Roster()->GetLiveNodeInfo(node, &info) == B_OK)
    name = iupStrReturnStr(info.name);
  BMediaRoster::Roster()->ReleaseNode(node);
  return name;
}

char* iupdrvMicrophoneGetPermission(Ihandle* ih)
{
  (void)ih;
  return iupdrvMicrophoneIsAvailable() ? (char*)"GRANTED" : (char*)"UNAVAILABLE";
}

static void haikuMicrophoneProcess(void* cookie, bigtime_t timestamp, void* data, size_t size, const media_format& format)
{
  IhaikuMicrophone* mic = (IhaikuMicrophone*)cookie;
  const media_raw_audio_format& raw = mic->raw;
  int sample_size = raw.format & 0xf;
  size_t samples = sample_size ? size / sample_size : 0;
  int channels = raw.channel_count ? raw.channel_count : 1;
  const short* pcm;
  (void)timestamp; (void)format;

  if (mic->stopping || samples == 0)
    return;

  if (raw.format == media_raw_audio_format::B_AUDIO_SHORT)
    pcm = (const short*)data;
  else
  {
    size_t k;
    if (mic->convert_size < samples)
    {
      short* buffer = (short*)realloc(mic->convert, samples * sizeof(short));
      if (!buffer)
        return;
      mic->convert = buffer;
      mic->convert_size = samples;
    }
    for (k = 0; k < samples; k++)
    {
      int v;
      switch (raw.format)
      {
      case media_raw_audio_format::B_AUDIO_FLOAT: v = (int)(((const float*)data)[k] * 32767.0f); break;
      case media_raw_audio_format::B_AUDIO_INT: v = ((const int*)data)[k] >> 16; break;
      case media_raw_audio_format::B_AUDIO_UCHAR: v = (((const unsigned char*)data)[k] - 128) << 8; break;
      case media_raw_audio_format::B_AUDIO_CHAR: v = ((const signed char*)data)[k] << 8; break;
      default: v = 0; break;
      }
      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;
      mic->convert[k] = (short)v;
    }
    pcm = mic->convert;
  }

  iupMicrophoneSamples(mic->ih, pcm, (int)(samples / channels));
}

static void haikuMicrophoneNotify(void* cookie, BMediaRecorder::notification what, ...)
{
  IhaikuMicrophone* mic = (IhaikuMicrophone*)cookie;
  if (what == BMediaRecorder::B_WILL_STOP && !mic->stopping)
    iupMicrophoneError(mic->ih, "Capture stopped");
}

static void haikuMicrophoneRelease(IhaikuMicrophone* mic)
{
  if (mic->recorder)
  {
    mic->recorder->SetHooks(NULL, NULL, NULL);
    mic->recorder->Disconnect();
    delete mic->recorder;
  }
  free(mic->convert);
  free(mic);
}

int iupdrvMicrophoneStart(Ihandle* ih, int device, int* channels, int* samplerate)
{
  IhaikuMicrophone* mic;
  media_node input;
  media_output output;
  media_format format;
  int32 count = 0;

  if (device != 0 || !haikuMicrophoneInput(&input))
  {
    iupMicrophoneError(ih, "Microphone not found");
    return 0;
  }

  mic = (IhaikuMicrophone*)calloc(1, sizeof(IhaikuMicrophone));
  if (!mic)
  {
    BMediaRoster::Roster()->ReleaseNode(input);
    return 0;
  }
  mic->ih = ih;

  mic->recorder = new IhaikuRecorder("IupMicrophone");
  if (mic->recorder->InitCheck() != B_OK)
  {
    BMediaRoster::Roster()->ReleaseNode(input);
    haikuMicrophoneRelease(mic);
    iupMicrophoneError(ih, "Cannot create recorder");
    return 0;
  }

  if (BMediaRoster::Roster()->GetFreeOutputsFor(input, &output, 1, &count, B_MEDIA_RAW_AUDIO) != B_OK || count < 1)
  {
    BMediaRoster::Roster()->ReleaseNode(input);
    haikuMicrophoneRelease(mic);
    iupMicrophoneError(ih, "Audio input is busy");
    return 0;
  }

  format.type = B_MEDIA_RAW_AUDIO;
  format.u.raw_audio = output.format.u.raw_audio;
  mic->recorder->SetAcceptedFormat(format);
  mic->recorder->SetHooks(haikuMicrophoneProcess, haikuMicrophoneNotify, mic);

  if (mic->recorder->Connect(input, &output, &format) != B_OK)
  {
    BMediaRoster::Roster()->ReleaseNode(input);
    haikuMicrophoneRelease(mic);
    iupMicrophoneError(ih, "Cannot connect to the audio input");
    return 0;
  }
  BMediaRoster::Roster()->ReleaseNode(input);

  mic->raw = mic->recorder->MediaInput().format.u.raw_audio;
  if (mic->raw.channel_count > 0)
    *channels = (int)mic->raw.channel_count;
  if (mic->raw.frame_rate > 0)
    *samplerate = (int)mic->raw.frame_rate;

  if (mic->recorder->Start() != B_OK)
  {
    haikuMicrophoneRelease(mic);
    iupMicrophoneError(ih, "Cannot start the recorder");
    return 0;
  }

  iupAttribSet(ih, "_IUP_MICROPHONE", (char*)mic);
  return 1;
}

void iupdrvMicrophoneStop(Ihandle* ih)
{
  IhaikuMicrophone* mic = (IhaikuMicrophone*)iupAttribGet(ih, "_IUP_MICROPHONE");
  if (!mic)
    return;

  mic->stopping = 1;
  mic->recorder->Stop(true);
  haikuMicrophoneRelease(mic);
  iupAttribSet(ih, "_IUP_MICROPHONE", NULL);
}

void iupdrvMicrophoneInitClass(Iclass* ic)
{
  (void)ic;
}

/** \file
 * \brief Camera control, Media Kit capture
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <BufferConsumer.h>
#include <Buffer.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <TimedEventQueue.h>
#include <TimeSource.h>
#include <scheduler.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_camera.h"

class IupHaikuCameraConsumer : public BMediaEventLooper, public BBufferConsumer
{
public:
  IupHaikuCameraConsumer(Ihandle* ih);
  virtual ~IupHaikuCameraConsumer();

  virtual BMediaAddOn* AddOn(int32* cookie) const;
  virtual void NodeRegistered();
  virtual status_t HandleMessage(int32 message, const void* data, size_t size);
  virtual void HandleEvent(const media_timed_event* event, bigtime_t lateness, bool realTimeEvent);
  virtual status_t AcceptFormat(const media_destination& dest, media_format* format);
  virtual status_t GetNextInput(int32* cookie, media_input* outInput);
  virtual void DisposeInputCookie(int32 cookie);
  virtual void BufferReceived(BBuffer* buffer);
  virtual void ProducerDataStatus(const media_destination& forWhom, int32 status, bigtime_t atMediaTime);
  virtual status_t GetLatencyFor(const media_destination& forWhom, bigtime_t* outLatency, media_node_id* outTimeSource);
  virtual status_t Connected(const media_source& producer, const media_destination& where, const media_format& withFormat, media_input* outInput);
  virtual void Disconnected(const media_source& producer, const media_destination& where);
  virtual status_t FormatChanged(const media_source& producer, const media_destination& consumer, int32 fromChangeCount, const media_format& format);

  media_input fInput;

private:
  void Deliver(BBuffer* buffer);

  Ihandle* fIh;
  unsigned char* fRgb;
  int fRgbSize;
};

typedef struct _IhaikuCamera
{
  BMediaRoster* roster;
  media_node producer;
  media_node timesource;
  media_output output;
  media_input input;
  IupHaikuCameraConsumer* consumer;
  int connected;
} IhaikuCamera;

IupHaikuCameraConsumer::IupHaikuCameraConsumer(Ihandle* ih)
  : BMediaNode("IupCamera"),
    BMediaEventLooper(),
    BBufferConsumer(B_MEDIA_RAW_VIDEO),
    fIh(ih),
    fRgb(NULL),
    fRgbSize(0)
{
  AddNodeKind(B_PHYSICAL_OUTPUT);
  SetEventLatency(0);
  SetPriority(B_DISPLAY_PRIORITY);
  memset(&fInput, 0, sizeof(fInput));
}

IupHaikuCameraConsumer::~IupHaikuCameraConsumer()
{
  Quit();
  free(fRgb);
}

BMediaAddOn* IupHaikuCameraConsumer::AddOn(int32* cookie) const
{
  *cookie = 0;
  return NULL;
}

void IupHaikuCameraConsumer::NodeRegistered()
{
  fInput.destination.port = ControlPort();
  fInput.destination.id = 0;
  fInput.source = media_source::null;
  fInput.format.type = B_MEDIA_RAW_VIDEO;
  fInput.format.u.raw_video = media_raw_video_format::wildcard;
  fInput.format.u.raw_video.display.format = B_RGB32;
  Run();
}

status_t IupHaikuCameraConsumer::HandleMessage(int32 message, const void* data, size_t size)
{
  (void)message; (void)data; (void)size;
  return B_OK;
}

status_t IupHaikuCameraConsumer::AcceptFormat(const media_destination& dest, media_format* format)
{
  if (dest != fInput.destination)
    return B_MEDIA_BAD_DESTINATION;

  if (format->type == B_MEDIA_NO_TYPE)
    format->type = B_MEDIA_RAW_VIDEO;
  if (format->type != B_MEDIA_RAW_VIDEO)
    return B_MEDIA_BAD_FORMAT;

  if (format->u.raw_video.display.format == media_raw_video_format::wildcard.display.format)
    format->u.raw_video.display.format = B_RGB32;
  if (format->u.raw_video.display.format != B_RGB32)
    return B_MEDIA_BAD_FORMAT;

  return B_OK;
}

status_t IupHaikuCameraConsumer::GetNextInput(int32* cookie, media_input* outInput)
{
  if (*cookie != 0)
    return B_MEDIA_BAD_DESTINATION;

  fInput.node = Node();
  fInput.destination.id = 0;
  strcpy(fInput.name, "IupCamera");
  *outInput = fInput;
  (*cookie)++;
  return B_OK;
}

void IupHaikuCameraConsumer::DisposeInputCookie(int32 cookie)
{
  (void)cookie;
}

void IupHaikuCameraConsumer::BufferReceived(BBuffer* buffer)
{
  if (RunState() == B_STOPPED)
  {
    buffer->Recycle();
    return;
  }

  media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
  EventQueue()->AddEvent(event);
}

void IupHaikuCameraConsumer::ProducerDataStatus(const media_destination& forWhom, int32 status, bigtime_t atMediaTime)
{
  (void)forWhom; (void)status; (void)atMediaTime;
}

status_t IupHaikuCameraConsumer::GetLatencyFor(const media_destination& forWhom, bigtime_t* outLatency, media_node_id* outTimeSource)
{
  if (forWhom != fInput.destination)
    return B_MEDIA_BAD_DESTINATION;
  *outLatency = 20000;
  *outTimeSource = TimeSource()->ID();
  return B_OK;
}

status_t IupHaikuCameraConsumer::Connected(const media_source& producer, const media_destination& where, const media_format& withFormat, media_input* outInput)
{
  (void)where;
  fInput.source = producer;
  fInput.format = withFormat;
  fInput.node = Node();
  strcpy(fInput.name, "IupCamera");
  *outInput = fInput;
  return B_OK;
}

void IupHaikuCameraConsumer::Disconnected(const media_source& producer, const media_destination& where)
{
  if (where == fInput.destination && producer == fInput.source)
    fInput.source = media_source::null;
}

status_t IupHaikuCameraConsumer::FormatChanged(const media_source& producer, const media_destination& consumer, int32 fromChangeCount, const media_format& format)
{
  (void)fromChangeCount;
  if (consumer != fInput.destination)
    return B_MEDIA_BAD_DESTINATION;
  if (producer != fInput.source)
    return B_MEDIA_BAD_SOURCE;
  fInput.format = format;
  return B_OK;
}

void IupHaikuCameraConsumer::Deliver(BBuffer* buffer)
{
  const media_raw_video_format& video = fInput.format.u.raw_video;
  int width = (int)video.display.line_width;
  int height = (int)video.display.line_count;
  int stride = (int)video.display.bytes_per_row;
  const unsigned char* src = (const unsigned char*)buffer->Data();
  unsigned char* dst;
  int x, y;

  if (stride <= 0)
    stride = width * 4;
  if (width <= 0 || height <= 0 || (size_t)(stride * height) > buffer->SizeUsed())
    return;

  if (fRgbSize != width * height * 3)
  {
    free(fRgb);
    fRgbSize = width * height * 3;
    fRgb = (unsigned char*)malloc(fRgbSize);
  }

  dst = fRgb;
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = src + y * stride;
    for (x = 0; x < width; x++)
    {
      dst[0] = row[2]; dst[1] = row[1]; dst[2] = row[0];
      row += 4;
      dst += 3;
    }
  }

  iupCameraFrame(fIh, fRgb, width, height);
}

void IupHaikuCameraConsumer::HandleEvent(const media_timed_event* event, bigtime_t lateness, bool realTimeEvent)
{
  (void)lateness; (void)realTimeEvent;

  switch (event->type)
  {
  case BTimedEventQueue::B_STOP:
    EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
    break;
  case BTimedEventQueue::B_HANDLE_BUFFER:
  {
    BBuffer* buffer = (BBuffer*)event->pointer;
    if (RunState() == B_STARTED && fInput.source != media_source::null)
      Deliver(buffer);
    buffer->Recycle();
    break;
  }
  default:
    break;
  }
}

int iupdrvCameraIsAvailable(void)
{
  return 1;
}

int iupdrvCameraGetDeviceCount(void)
{
  BMediaRoster* roster = BMediaRoster::Roster();
  media_node node;
  if (!roster || roster->GetVideoInput(&node) != B_OK)
    return 0;
  roster->ReleaseNode(node);
  return 1;
}

char* iupdrvCameraGetDeviceName(int index)
{
  BMediaRoster* roster = BMediaRoster::Roster();
  media_node node;
  live_node_info info;
  char* name;

  if (index != 0 || !roster || roster->GetVideoInput(&node) != B_OK)
    return NULL;
  if (roster->GetLiveNodeInfo(node, &info) == B_OK)
    name = iupStrReturnStr(info.name);
  else
    name = iupStrReturnStr("Video Input");
  roster->ReleaseNode(node);
  return name;
}

char* iupdrvCameraGetPermission(Ihandle* ih)
{
  (void)ih;
  return iupdrvCameraGetDeviceCount() ? (char*)"GRANTED" : (char*)"UNAVAILABLE";
}

static void haikuCameraRelease(IhaikuCamera* camera)
{
  if (camera->consumer)
  {
    if (camera->connected)
    {
      camera->roster->StopNode(camera->producer, 0, true);
      camera->roster->StopNode(camera->consumer->Node(), 0, true);
      camera->roster->Disconnect(camera->output.node.node, camera->output.source, camera->input.node.node, camera->input.destination);
    }
    camera->roster->ReleaseNode(camera->consumer->Node());
  }
  if (camera->producer != media_node::null)
    camera->roster->ReleaseNode(camera->producer);
  free(camera);
}

int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  IhaikuCamera* camera;
  media_format format;
  int32 count = 0;
  bigtime_t latency = 0, init_latency = 0, real, perf;
  BTimeSource* timesource;
  (void)fps;

  if (device != 0)
  {
    iupCameraError(ih, "Camera not found");
    return 0;
  }

  camera = (IhaikuCamera*)calloc(1, sizeof(IhaikuCamera));
  camera->producer = media_node::null;
  camera->roster = BMediaRoster::Roster();
  if (!camera->roster || camera->roster->GetVideoInput(&camera->producer) != B_OK)
  {
    camera->producer = media_node::null;
    haikuCameraRelease(camera);
    iupCameraError(ih, "Camera not found");
    return 0;
  }
  camera->roster->GetTimeSource(&camera->timesource);

  camera->consumer = new IupHaikuCameraConsumer(ih);
  if (camera->roster->RegisterNode(camera->consumer) != B_OK)
  {
    delete camera->consumer;
    camera->consumer = NULL;
    haikuCameraRelease(camera);
    iupCameraError(ih, "Cannot register the capture node");
    return 0;
  }

  if (camera->roster->GetFreeOutputsFor(camera->producer, &camera->output, 1, &count, B_MEDIA_RAW_VIDEO) != B_OK || count < 1 ||
      camera->roster->GetFreeInputsFor(camera->consumer->Node(), &camera->input, 1, &count, B_MEDIA_RAW_VIDEO) != B_OK || count < 1)
  {
    haikuCameraRelease(camera);
    iupCameraError(ih, "Camera is busy");
    return 0;
  }

  format.type = B_MEDIA_RAW_VIDEO;
  format.u.raw_video = media_raw_video_format::wildcard;
  format.u.raw_video.interlace = 1;
  format.u.raw_video.display.format = B_RGB32;
  format.u.raw_video.display.line_width = *width;
  format.u.raw_video.display.line_count = *height;
  if (camera->roster->Connect(camera->output.source, camera->input.destination, &format, &camera->output, &camera->input) != B_OK)
  {
    format.u.raw_video.display.line_width = 0;
    format.u.raw_video.display.line_count = 0;
    if (camera->roster->Connect(camera->output.source, camera->input.destination, &format, &camera->output, &camera->input) != B_OK)
    {
      haikuCameraRelease(camera);
      iupCameraError(ih, "Cannot connect the camera");
      return 0;
    }
  }
  camera->connected = 1;

  camera->roster->SetTimeSourceFor(camera->producer.node, camera->timesource.node);
  camera->roster->SetTimeSourceFor(camera->consumer->ID(), camera->timesource.node);
  camera->roster->GetLatencyFor(camera->producer, &latency);
  camera->roster->SetProducerRunModeDelay(camera->producer, latency);
  camera->roster->GetInitialLatencyFor(camera->producer, &init_latency);
  init_latency += estimate_max_scheduling_latency();

  timesource = camera->roster->MakeTimeSourceFor(camera->producer);
  real = BTimeSource::RealTime();
  if (!timesource->IsRunning())
  {
    camera->roster->StartTimeSource(camera->timesource, real);
    camera->roster->SeekTimeSource(camera->timesource, 0, real);
  }
  perf = timesource->PerformanceTimeFor(real + latency + init_latency);
  timesource->Release();

  if (camera->roster->StartNode(camera->producer, perf) != B_OK || camera->roster->StartNode(camera->consumer->Node(), perf) != B_OK)
  {
    haikuCameraRelease(camera);
    iupCameraError(ih, "Cannot start the camera");
    return 0;
  }

  *width = (int)camera->output.format.u.raw_video.display.line_width;
  *height = (int)camera->output.format.u.raw_video.display.line_count;
  if (camera->output.format.u.raw_video.field_rate > 0)
    *fps = (int)(camera->output.format.u.raw_video.field_rate + 0.5f);
  iupAttribSet(ih, "_IUP_CAMERA", (char*)camera);
  return 1;
}

void iupdrvCameraStop(Ihandle* ih)
{
  IhaikuCamera* camera = (IhaikuCamera*)iupAttribGet(ih, "_IUP_CAMERA");
  if (!camera)
    return;

  haikuCameraRelease(camera);
  iupAttribSet(ih, "_IUP_CAMERA", NULL);
}

void iupdrvCameraInitClass(Iclass* ic)
{
  (void)ic;
}

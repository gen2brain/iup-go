/** \file
 * \brief Camera control, Video4Linux2 capture
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_camera.h"

#define IUNIX_CAMERA_MAX_DEVICES 64
#define IUNIX_CAMERA_BUFFERS 4

typedef struct _IunixCamera
{
  Ihandle* ih;
  int fd;
  pthread_t thread;
  int quit;
  void* buffers[IUNIX_CAMERA_BUFFERS];
  size_t lengths[IUNIX_CAMERA_BUFFERS];
  int buffer_count;
  int width, height, stride;
  unsigned int fourcc;
  unsigned char* rgb;
} IunixCamera;

static const unsigned int iunix_camera_formats[] = {
  V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_UYVY, V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV21,
  V4L2_PIX_FMT_YUV420, V4L2_PIX_FMT_YVU420, V4L2_PIX_FMT_RGB24, V4L2_PIX_FMT_BGR24,
  V4L2_PIX_FMT_GREY, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_JPEG
};
#define IUNIX_CAMERA_FORMATS (int)(sizeof(iunix_camera_formats) / sizeof(iunix_camera_formats[0]))

static int iunixCameraIoctl(int fd, unsigned long request, void* arg)
{
  int ret;
  do
    ret = ioctl(fd, request, arg);
  while (ret == -1 && errno == EINTR);
  return ret;
}

static int iunixCameraOpenDevice(int index, char* name, size_t name_length)
{
  char path[32];
  struct v4l2_capability cap;
  struct v4l2_fmtdesc fmt;
  unsigned int caps;
  int fd;

  sprintf(path, "/dev/video%d", index);
  fd = open(path, O_RDWR | O_NONBLOCK);
  if (fd < 0)
    return -1;

  memset(&cap, 0, sizeof(cap));
  if (iunixCameraIoctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
  {
    close(fd);
    return -1;
  }

  caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
  if (!(caps & V4L2_CAP_VIDEO_CAPTURE) || !(caps & V4L2_CAP_STREAMING))
  {
    close(fd);
    return -1;
  }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (iunixCameraIoctl(fd, VIDIOC_ENUM_FMT, &fmt) < 0)
  {
    close(fd);
    return -1;
  }

  if (name)
  {
    strncpy(name, (const char*)cap.card, name_length - 1);
    name[name_length - 1] = 0;
  }
  return fd;
}

static int iunixCameraFindDevice(int device, char* name, size_t name_length)
{
  int index, found = 0;
  for (index = 0; index < IUNIX_CAMERA_MAX_DEVICES; index++)
  {
    int fd = iunixCameraOpenDevice(index, name, name_length);
    if (fd < 0)
      continue;
    if (found == device)
      return fd;
    close(fd);
    found++;
  }
  return -1;
}

int iupdrvCameraIsAvailable(void)
{
  return 1;
}

int iupdrvCameraGetDeviceCount(void)
{
  int index, count = 0;
  for (index = 0; index < IUNIX_CAMERA_MAX_DEVICES; index++)
  {
    int fd = iunixCameraOpenDevice(index, NULL, 0);
    if (fd < 0)
      continue;
    close(fd);
    count++;
  }
  return count;
}

char* iupdrvCameraGetDeviceName(int index)
{
  char name[64];
  int fd = iunixCameraFindDevice(index, name, sizeof(name));
  if (fd < 0)
    return NULL;
  close(fd);
  return iupStrReturnStr(name);
}

char* iupdrvCameraGetPermission(Ihandle* ih)
{
  int index;
  char path[32];
  (void)ih;

  for (index = 0; index < IUNIX_CAMERA_MAX_DEVICES; index++)
  {
    sprintf(path, "/dev/video%d", index);
    if (access(path, F_OK) != 0)
      continue;
    return access(path, R_OK | W_OK) == 0 ? "GRANTED" : "DENIED";
  }
  return "UNAVAILABLE";
}

static int iunixCameraFormatRank(unsigned int fourcc)
{
  int i;
  for (i = 0; i < IUNIX_CAMERA_FORMATS; i++)
  {
    if (iunix_camera_formats[i] == fourcc)
      return i;
  }
  return -1;
}

static int iunixCameraMaxFps(int fd, unsigned int fourcc, int width, int height)
{
  struct v4l2_frmivalenum ival;
  int fps = 0;

  memset(&ival, 0, sizeof(ival));
  ival.pixel_format = fourcc;
  ival.width = width;
  ival.height = height;
  while (iunixCameraIoctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival) == 0)
  {
    int value = 0;
    if (ival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
    {
      if (ival.discrete.numerator)
        value = (int)(ival.discrete.denominator / ival.discrete.numerator);
    }
    else
    {
      if (ival.stepwise.min.numerator)
        value = (int)(ival.stepwise.min.denominator / ival.stepwise.min.numerator);
      ival.index = 0xFFFF;
    }
    if (value > fps)
      fps = value;
    ival.index++;
  }
  return fps;
}

static void iunixCameraConsider(int fd, unsigned int fourcc, int width, int height, int req_width, int req_height, int req_fps,
                                unsigned int* best_fourcc, int* best_width, int* best_height, long* best_score)
{
  long score;
  int fps = iunixCameraMaxFps(fd, fourcc, width, height);

  score = labs((long)width * (long)height - (long)req_width * (long)req_height) * 4;
  if (fps && fps < req_fps)
    score += (long)(req_fps - fps) * 100000;
  score += iunixCameraFormatRank(fourcc);

  if (*best_score < 0 || score < *best_score)
  {
    *best_score = score;
    *best_fourcc = fourcc;
    *best_width = width;
    *best_height = height;
  }
}

static int iunixCameraChooseFormat(int fd, int req_width, int req_height, int req_fps, unsigned int* fourcc, int* width, int* height)
{
  struct v4l2_fmtdesc fmt;
  long best_score = -1;

  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  while (iunixCameraIoctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0)
  {
    struct v4l2_frmsizeenum size;

    fmt.index++;
    if (iunixCameraFormatRank(fmt.pixelformat) < 0)
      continue;

    memset(&size, 0, sizeof(size));
    size.pixel_format = fmt.pixelformat;
    while (iunixCameraIoctl(fd, VIDIOC_ENUM_FRAMESIZES, &size) == 0)
    {
      if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE)
        iunixCameraConsider(fd, fmt.pixelformat, (int)size.discrete.width, (int)size.discrete.height, req_width, req_height, req_fps, fourcc, width, height, &best_score);
      else
      {
        int w = req_width, h = req_height;
        if (w < (int)size.stepwise.min_width) w = (int)size.stepwise.min_width;
        if (w > (int)size.stepwise.max_width) w = (int)size.stepwise.max_width;
        if (h < (int)size.stepwise.min_height) h = (int)size.stepwise.min_height;
        if (h > (int)size.stepwise.max_height) h = (int)size.stepwise.max_height;
        iunixCameraConsider(fd, fmt.pixelformat, w, h, req_width, req_height, req_fps, fourcc, width, height, &best_score);
        break;
      }
      size.index++;
    }
  }

  return best_score >= 0;
}

static void iunixCameraRelease(IunixCamera* camera)
{
  int i;
  for (i = 0; i < camera->buffer_count; i++)
  {
    if (camera->buffers[i])
      munmap(camera->buffers[i], camera->lengths[i]);
  }
  if (camera->fd >= 0)
    close(camera->fd);
  free(camera->rgb);
  free(camera);
}

static unsigned char iunixCameraClamp(int value)
{
  return (unsigned char)(value < 0 ? 0 : value > 255 ? 255 : value);
}

static void iunixCameraYuvToRgb(int y, int u, int v, unsigned char* rgb)
{
  int c = y - 16, d = u - 128, e = v - 128;
  rgb[0] = iunixCameraClamp((298 * c + 409 * e + 128) >> 8);
  rgb[1] = iunixCameraClamp((298 * c - 100 * d - 208 * e + 128) >> 8);
  rgb[2] = iunixCameraClamp((298 * c + 516 * d + 128) >> 8);
}

static void iunixCameraConvertPacked422(const unsigned char* src, int stride, int width, int height, int y_first, unsigned char* rgb)
{
  int x, y;
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = src + y * stride;
    for (x = 0; x < width; x += 2)
    {
      int y0 = y_first ? row[0] : row[1];
      int u = y_first ? row[1] : row[0];
      int y1 = y_first ? row[2] : row[3];
      int v = y_first ? row[3] : row[2];
      iunixCameraYuvToRgb(y0, u, v, rgb);
      iunixCameraYuvToRgb(y1, u, v, rgb + 3);
      row += 4;
      rgb += 6;
    }
  }
}

static void iunixCameraConvertSemiPlanar(const unsigned char* src, int stride, int width, int height, int v_first, unsigned char* rgb)
{
  const unsigned char* uv = src + stride * height;
  int x, y;
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = src + y * stride;
    const unsigned char* chroma = uv + (y / 2) * stride;
    for (x = 0; x < width; x++)
    {
      int u = v_first ? chroma[(x & ~1) + 1] : chroma[x & ~1];
      int v = v_first ? chroma[x & ~1] : chroma[(x & ~1) + 1];
      iunixCameraYuvToRgb(row[x], u, v, rgb);
      rgb += 3;
    }
  }
}

static void iunixCameraConvertPlanar(const unsigned char* src, int stride, int width, int height, int v_first, unsigned char* rgb)
{
  int cstride = stride / 2;
  const unsigned char* plane1 = src + stride * height;
  const unsigned char* plane2 = plane1 + cstride * (height / 2);
  const unsigned char* uplane = v_first ? plane2 : plane1;
  const unsigned char* vplane = v_first ? plane1 : plane2;
  int x, y;
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = src + y * stride;
    const unsigned char* urow = uplane + (y / 2) * cstride;
    const unsigned char* vrow = vplane + (y / 2) * cstride;
    for (x = 0; x < width; x++)
    {
      iunixCameraYuvToRgb(row[x], urow[x / 2], vrow[x / 2], rgb);
      rgb += 3;
    }
  }
}

static void iunixCameraConvertRgb(const unsigned char* src, int stride, int width, int height, int bgr, unsigned char* rgb)
{
  int x, y;
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = src + y * stride;
    if (!bgr)
    {
      memcpy(rgb, row, width * 3);
      rgb += width * 3;
      continue;
    }
    for (x = 0; x < width; x++)
    {
      rgb[0] = row[2]; rgb[1] = row[1]; rgb[2] = row[0];
      row += 3;
      rgb += 3;
    }
  }
}

static void iunixCameraConvertGrey(const unsigned char* src, int stride, int width, int height, unsigned char* rgb)
{
  int x, y;
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = src + y * stride;
    for (x = 0; x < width; x++)
    {
      rgb[0] = rgb[1] = rgb[2] = row[x];
      rgb += 3;
    }
  }
}

static int iunixCameraConvert(IunixCamera* camera, const unsigned char* src, size_t size)
{
  int width = camera->width, height = camera->height, stride = camera->stride;

  switch (camera->fourcc)
  {
  case V4L2_PIX_FMT_YUYV: iunixCameraConvertPacked422(src, stride, width, height, 1, camera->rgb); break;
  case V4L2_PIX_FMT_UYVY: iunixCameraConvertPacked422(src, stride, width, height, 0, camera->rgb); break;
  case V4L2_PIX_FMT_NV12: iunixCameraConvertSemiPlanar(src, stride, width, height, 0, camera->rgb); break;
  case V4L2_PIX_FMT_NV21: iunixCameraConvertSemiPlanar(src, stride, width, height, 1, camera->rgb); break;
  case V4L2_PIX_FMT_YUV420: iunixCameraConvertPlanar(src, stride, width, height, 0, camera->rgb); break;
  case V4L2_PIX_FMT_YVU420: iunixCameraConvertPlanar(src, stride, width, height, 1, camera->rgb); break;
  case V4L2_PIX_FMT_RGB24: iunixCameraConvertRgb(src, stride, width, height, 0, camera->rgb); break;
  case V4L2_PIX_FMT_BGR24: iunixCameraConvertRgb(src, stride, width, height, 1, camera->rgb); break;
  case V4L2_PIX_FMT_GREY: iunixCameraConvertGrey(src, stride, width, height, camera->rgb); break;
  case V4L2_PIX_FMT_MJPEG:
  case V4L2_PIX_FMT_JPEG:
  {
    int w, h, channels;
    unsigned char* pixels = stbi_load_from_memory(src, (int)size, &w, &h, &channels, 3);
    if (!pixels)
      return 0;
    if (w == width && h == height)
      memcpy(camera->rgb, pixels, width * height * 3);
    stbi_image_free(pixels);
    if (w != width || h != height)
      return 0;
    break;
  }
  default:
    return 0;
  }
  return 1;
}

static void* iunixCameraThread(void* arg)
{
  IunixCamera* camera = (IunixCamera*)arg;

  while (!camera->quit)
  {
    struct pollfd pfd;
    struct v4l2_buffer buf;
    int ret;

    pfd.fd = camera->fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    ret = poll(&pfd, 1, 200);
    if (ret < 0 && errno != EINTR)
    {
      iupCameraError(camera->ih, strerror(errno));
      break;
    }
    if (ret <= 0 || camera->quit)
      continue;
    if (pfd.revents & POLLERR)
    {
      iupCameraError(camera->ih, "Camera stopped");
      break;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (iunixCameraIoctl(camera->fd, VIDIOC_DQBUF, &buf) < 0)
    {
      if (errno == EAGAIN)
        continue;
      iupCameraError(camera->ih, strerror(errno));
      break;
    }

    if (!(buf.flags & V4L2_BUF_FLAG_ERROR) && buf.bytesused > 0 && iunixCameraConvert(camera, (const unsigned char*)camera->buffers[buf.index], buf.bytesused))
      iupCameraFrame(camera->ih, camera->rgb, camera->width, camera->height);

    iunixCameraIoctl(camera->fd, VIDIOC_QBUF, &buf);
  }

  return NULL;
}

int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  IunixCamera* camera;
  struct v4l2_format fmt;
  struct v4l2_streamparm parm;
  struct v4l2_requestbuffers req;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  unsigned int fourcc = 0;
  int i, fd, w = *width, h = *height;

  fd = iunixCameraFindDevice(device, NULL, 0);
  if (fd < 0)
  {
    iupCameraError(ih, "Camera not found");
    return 0;
  }

  camera = (IunixCamera*)calloc(1, sizeof(IunixCamera));
  camera->ih = ih;
  camera->fd = fd;

  if (!iunixCameraChooseFormat(fd, *width, *height, *fps, &fourcc, &w, &h))
  {
    iunixCameraRelease(camera);
    iupCameraError(ih, "No supported pixel format");
    return 0;
  }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = w;
  fmt.fmt.pix.height = h;
  fmt.fmt.pix.pixelformat = fourcc;
  fmt.fmt.pix.field = V4L2_FIELD_NONE;
  if (iunixCameraIoctl(fd, VIDIOC_S_FMT, &fmt) < 0)
  {
    iunixCameraRelease(camera);
    iupCameraError(ih, strerror(errno));
    return 0;
  }
  camera->width = (int)fmt.fmt.pix.width;
  camera->height = (int)fmt.fmt.pix.height;
  camera->fourcc = fmt.fmt.pix.pixelformat;
  camera->stride = (int)fmt.fmt.pix.bytesperline;
  if (camera->stride <= 0)
    camera->stride = camera->width * (camera->fourcc == V4L2_PIX_FMT_RGB24 || camera->fourcc == V4L2_PIX_FMT_BGR24 ? 3 : camera->fourcc == V4L2_PIX_FMT_YUYV || camera->fourcc == V4L2_PIX_FMT_UYVY ? 2 : 1);

  memset(&parm, 0, sizeof(parm));
  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  parm.parm.capture.timeperframe.numerator = 1;
  parm.parm.capture.timeperframe.denominator = *fps > 0 ? *fps : 30;
  iunixCameraIoctl(fd, VIDIOC_S_PARM, &parm);
  memset(&parm, 0, sizeof(parm));
  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (iunixCameraIoctl(fd, VIDIOC_G_PARM, &parm) == 0 && parm.parm.capture.timeperframe.numerator)
    *fps = (int)(parm.parm.capture.timeperframe.denominator / parm.parm.capture.timeperframe.numerator);

  memset(&req, 0, sizeof(req));
  req.count = IUNIX_CAMERA_BUFFERS;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if (iunixCameraIoctl(fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2)
  {
    iunixCameraRelease(camera);
    iupCameraError(ih, "Cannot allocate capture buffers");
    return 0;
  }
  camera->buffer_count = (int)req.count;

  for (i = 0; i < camera->buffer_count; i++)
  {
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if (iunixCameraIoctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
    {
      iunixCameraRelease(camera);
      iupCameraError(ih, strerror(errno));
      return 0;
    }
    camera->lengths[i] = buf.length;
    camera->buffers[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (camera->buffers[i] == MAP_FAILED)
    {
      camera->buffers[i] = NULL;
      iunixCameraRelease(camera);
      iupCameraError(ih, strerror(errno));
      return 0;
    }
    if (iunixCameraIoctl(fd, VIDIOC_QBUF, &buf) < 0)
    {
      iunixCameraRelease(camera);
      iupCameraError(ih, strerror(errno));
      return 0;
    }
  }

  camera->rgb = (unsigned char*)malloc(camera->width * camera->height * 3);

  if (iunixCameraIoctl(fd, VIDIOC_STREAMON, &type) < 0)
  {
    iunixCameraRelease(camera);
    iupCameraError(ih, strerror(errno));
    return 0;
  }

  if (pthread_create(&camera->thread, NULL, iunixCameraThread, camera) != 0)
  {
    iunixCameraIoctl(fd, VIDIOC_STREAMOFF, &type);
    iunixCameraRelease(camera);
    iupCameraError(ih, "Cannot start capture thread");
    return 0;
  }

  *width = camera->width;
  *height = camera->height;
  iupAttribSet(ih, "_IUP_CAMERA", (char*)camera);
  return 1;
}

void iupdrvCameraStop(Ihandle* ih)
{
  IunixCamera* camera = (IunixCamera*)iupAttribGet(ih, "_IUP_CAMERA");
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (!camera)
    return;

  camera->quit = 1;
  pthread_join(camera->thread, NULL);
  iunixCameraIoctl(camera->fd, VIDIOC_STREAMOFF, &type);
  iunixCameraRelease(camera);
  iupAttribSet(ih, "_IUP_CAMERA", NULL);
}

void iupdrvCameraInitClass(Iclass* ic)
{
  (void)ic;
}

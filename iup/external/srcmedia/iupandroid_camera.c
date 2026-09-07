/** \file
 * \brief Camera control, Camera2 NDK capture
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCaptureRequest.h>
#include <camera/NdkCameraCaptureSession.h>
#include <media/NdkImageReader.h>
#include <android/native_window.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_media.h"
#include "iup_camera.h"

#include "iupandroid_drv.h"

typedef ACameraManager* (*ACameraManager_createFunc)(void);
typedef void (*ACameraManager_deleteFunc)(ACameraManager*);
typedef camera_status_t (*ACameraManager_getCameraIdListFunc)(ACameraManager*, ACameraIdList**);
typedef void (*ACameraManager_deleteCameraIdListFunc)(ACameraIdList*);
typedef camera_status_t (*ACameraManager_getCameraCharacteristicsFunc)(ACameraManager*, const char*, ACameraMetadata**);
typedef camera_status_t (*ACameraManager_openCameraFunc)(ACameraManager*, const char*, ACameraDevice_StateCallbacks*, ACameraDevice**);
typedef camera_status_t (*ACameraMetadata_getConstEntryFunc)(const ACameraMetadata*, uint32_t, ACameraMetadata_const_entry*);
typedef void (*ACameraMetadata_freeFunc)(ACameraMetadata*);
typedef camera_status_t (*ACameraDevice_closeFunc)(ACameraDevice*);
typedef camera_status_t (*ACameraDevice_createCaptureRequestFunc)(const ACameraDevice*, ACameraDevice_request_template, ACaptureRequest**);
typedef camera_status_t (*ACameraDevice_createCaptureSessionFunc)(ACameraDevice*, const ACaptureSessionOutputContainer*, const ACameraCaptureSession_stateCallbacks*, ACameraCaptureSession**);
typedef camera_status_t (*ACaptureSessionOutputContainer_createFunc)(ACaptureSessionOutputContainer**);
typedef void (*ACaptureSessionOutputContainer_freeFunc)(ACaptureSessionOutputContainer*);
typedef camera_status_t (*ACaptureSessionOutputContainer_addFunc)(ACaptureSessionOutputContainer*, const ACaptureSessionOutput*);
typedef camera_status_t (*ACaptureSessionOutput_createFunc)(ANativeWindow*, ACaptureSessionOutput**);
typedef void (*ACaptureSessionOutput_freeFunc)(ACaptureSessionOutput*);
typedef camera_status_t (*ACameraOutputTarget_createFunc)(ANativeWindow*, ACameraOutputTarget**);
typedef void (*ACameraOutputTarget_freeFunc)(ACameraOutputTarget*);
typedef camera_status_t (*ACaptureRequest_addTargetFunc)(ACaptureRequest*, const ACameraOutputTarget*);
typedef camera_status_t (*ACaptureRequest_setEntry_i32Func)(ACaptureRequest*, uint32_t, uint32_t, const int32_t*);
typedef void (*ACaptureRequest_freeFunc)(ACaptureRequest*);
typedef camera_status_t (*ACameraCaptureSession_setRepeatingRequestFunc)(ACameraCaptureSession*, ACameraCaptureSession_captureCallbacks*, int, ACaptureRequest**, int*);
typedef camera_status_t (*ACameraCaptureSession_stopRepeatingFunc)(ACameraCaptureSession*);
typedef void (*ACameraCaptureSession_closeFunc)(ACameraCaptureSession*);
typedef media_status_t (*AImageReader_newFunc)(int32_t, int32_t, int32_t, int32_t, AImageReader**);
typedef void (*AImageReader_deleteFunc)(AImageReader*);
typedef media_status_t (*AImageReader_getWindowFunc)(AImageReader*, ANativeWindow**);
typedef media_status_t (*AImageReader_setImageListenerFunc)(AImageReader*, AImageReader_ImageListener*);
typedef media_status_t (*AImageReader_acquireLatestImageFunc)(AImageReader*, AImage**);
typedef void (*AImage_deleteFunc)(AImage*);
typedef media_status_t (*AImage_getWidthFunc)(const AImage*, int32_t*);
typedef media_status_t (*AImage_getHeightFunc)(const AImage*, int32_t*);
typedef media_status_t (*AImage_getPlaneRowStrideFunc)(const AImage*, int, int32_t*);
typedef media_status_t (*AImage_getPlanePixelStrideFunc)(const AImage*, int, int32_t*);
typedef media_status_t (*AImage_getPlaneDataFunc)(const AImage*, int, uint8_t**, int*);

static struct
{
  int loaded;
  ACameraManager_createFunc ACameraManager_create;
  ACameraManager_deleteFunc ACameraManager_delete;
  ACameraManager_getCameraIdListFunc ACameraManager_getCameraIdList;
  ACameraManager_deleteCameraIdListFunc ACameraManager_deleteCameraIdList;
  ACameraManager_getCameraCharacteristicsFunc ACameraManager_getCameraCharacteristics;
  ACameraManager_openCameraFunc ACameraManager_openCamera;
  ACameraMetadata_getConstEntryFunc ACameraMetadata_getConstEntry;
  ACameraMetadata_freeFunc ACameraMetadata_free;
  ACameraDevice_closeFunc ACameraDevice_close;
  ACameraDevice_createCaptureRequestFunc ACameraDevice_createCaptureRequest;
  ACameraDevice_createCaptureSessionFunc ACameraDevice_createCaptureSession;
  ACaptureSessionOutputContainer_createFunc ACaptureSessionOutputContainer_create;
  ACaptureSessionOutputContainer_freeFunc ACaptureSessionOutputContainer_free;
  ACaptureSessionOutputContainer_addFunc ACaptureSessionOutputContainer_add;
  ACaptureSessionOutput_createFunc ACaptureSessionOutput_create;
  ACaptureSessionOutput_freeFunc ACaptureSessionOutput_free;
  ACameraOutputTarget_createFunc ACameraOutputTarget_create;
  ACameraOutputTarget_freeFunc ACameraOutputTarget_free;
  ACaptureRequest_addTargetFunc ACaptureRequest_addTarget;
  ACaptureRequest_setEntry_i32Func ACaptureRequest_setEntry_i32;
  ACaptureRequest_freeFunc ACaptureRequest_free;
  ACameraCaptureSession_setRepeatingRequestFunc ACameraCaptureSession_setRepeatingRequest;
  ACameraCaptureSession_stopRepeatingFunc ACameraCaptureSession_stopRepeating;
  ACameraCaptureSession_closeFunc ACameraCaptureSession_close;
  AImageReader_newFunc AImageReader_new;
  AImageReader_deleteFunc AImageReader_delete;
  AImageReader_getWindowFunc AImageReader_getWindow;
  AImageReader_setImageListenerFunc AImageReader_setImageListener;
  AImageReader_acquireLatestImageFunc AImageReader_acquireLatestImage;
  AImage_deleteFunc AImage_delete;
  AImage_getWidthFunc AImage_getWidth;
  AImage_getHeightFunc AImage_getHeight;
  AImage_getPlaneRowStrideFunc AImage_getPlaneRowStride;
  AImage_getPlanePixelStrideFunc AImage_getPlanePixelStride;
  AImage_getPlaneDataFunc AImage_getPlaneData;
} ndk = { -1 };

typedef struct _IandroidCamera
{
  Ihandle* ih;
  ACameraManager* manager;
  ACameraDevice* device;
  AImageReader* reader;
  ANativeWindow* window;
  ACaptureSessionOutputContainer* container;
  ACaptureSessionOutput* output;
  ACameraOutputTarget* target;
  ACaptureRequest* request;
  ACameraCaptureSession* session;
  int width, height, orientation;
  unsigned char* yuv_rgb;
  unsigned char* rgb;
  pthread_mutex_t lock;
  int closing;
} IandroidCamera;

#define IANDROID_CAMERA_SYM(lib, name) \
  ndk.name = (name##Func)dlsym(lib, #name); \
  if (!ndk.name) return 0

static int androidCameraLoad(void)
{
  void* camera;
  void* media;

  if (ndk.loaded >= 0)
    return ndk.loaded;
  ndk.loaded = 0;

  camera = dlopen("libcamera2ndk.so", RTLD_NOW | RTLD_LOCAL);
  media = dlopen("libmediandk.so", RTLD_NOW | RTLD_LOCAL);
  if (!camera || !media)
    return 0;

  IANDROID_CAMERA_SYM(camera, ACameraManager_create);
  IANDROID_CAMERA_SYM(camera, ACameraManager_delete);
  IANDROID_CAMERA_SYM(camera, ACameraManager_getCameraIdList);
  IANDROID_CAMERA_SYM(camera, ACameraManager_deleteCameraIdList);
  IANDROID_CAMERA_SYM(camera, ACameraManager_getCameraCharacteristics);
  IANDROID_CAMERA_SYM(camera, ACameraManager_openCamera);
  IANDROID_CAMERA_SYM(camera, ACameraMetadata_getConstEntry);
  IANDROID_CAMERA_SYM(camera, ACameraMetadata_free);
  IANDROID_CAMERA_SYM(camera, ACameraDevice_close);
  IANDROID_CAMERA_SYM(camera, ACameraDevice_createCaptureRequest);
  IANDROID_CAMERA_SYM(camera, ACameraDevice_createCaptureSession);
  IANDROID_CAMERA_SYM(camera, ACaptureSessionOutputContainer_create);
  IANDROID_CAMERA_SYM(camera, ACaptureSessionOutputContainer_free);
  IANDROID_CAMERA_SYM(camera, ACaptureSessionOutputContainer_add);
  IANDROID_CAMERA_SYM(camera, ACaptureSessionOutput_create);
  IANDROID_CAMERA_SYM(camera, ACaptureSessionOutput_free);
  IANDROID_CAMERA_SYM(camera, ACameraOutputTarget_create);
  IANDROID_CAMERA_SYM(camera, ACameraOutputTarget_free);
  IANDROID_CAMERA_SYM(camera, ACaptureRequest_addTarget);
  IANDROID_CAMERA_SYM(camera, ACaptureRequest_setEntry_i32);
  IANDROID_CAMERA_SYM(camera, ACaptureRequest_free);
  IANDROID_CAMERA_SYM(camera, ACameraCaptureSession_setRepeatingRequest);
  IANDROID_CAMERA_SYM(camera, ACameraCaptureSession_stopRepeating);
  IANDROID_CAMERA_SYM(camera, ACameraCaptureSession_close);
  IANDROID_CAMERA_SYM(media, AImageReader_new);
  IANDROID_CAMERA_SYM(media, AImageReader_delete);
  IANDROID_CAMERA_SYM(media, AImageReader_getWindow);
  IANDROID_CAMERA_SYM(media, AImageReader_setImageListener);
  IANDROID_CAMERA_SYM(media, AImageReader_acquireLatestImage);
  IANDROID_CAMERA_SYM(media, AImage_delete);
  IANDROID_CAMERA_SYM(media, AImage_getWidth);
  IANDROID_CAMERA_SYM(media, AImage_getHeight);
  IANDROID_CAMERA_SYM(media, AImage_getPlaneRowStride);
  IANDROID_CAMERA_SYM(media, AImage_getPlanePixelStride);
  IANDROID_CAMERA_SYM(media, AImage_getPlaneData);

  ndk.loaded = 1;
  return 1;
}

int iupdrvCameraIsAvailable(void)
{
  return androidCameraLoad();
}

int iupdrvCameraGetDeviceCount(void)
{
  ACameraManager* manager;
  ACameraIdList* list = NULL;
  int count = 0;

  if (!androidCameraLoad())
    return 0;

  manager = ndk.ACameraManager_create();
  if (!manager)
    return 0;
  if (ndk.ACameraManager_getCameraIdList(manager, &list) == ACAMERA_OK && list)
  {
    count = list->numCameras;
    ndk.ACameraManager_deleteCameraIdList(list);
  }
  ndk.ACameraManager_delete(manager);
  return count;
}

static int androidCameraFacing(ACameraManager* manager, const char* id)
{
  ACameraMetadata* metadata = NULL;
  ACameraMetadata_const_entry entry;
  int facing = -1;

  if (ndk.ACameraManager_getCameraCharacteristics(manager, id, &metadata) != ACAMERA_OK || !metadata)
    return -1;
  if (ndk.ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &entry) == ACAMERA_OK && entry.count > 0)
    facing = entry.data.u8[0];
  ndk.ACameraMetadata_free(metadata);
  return facing;
}

char* iupdrvCameraGetDeviceName(int index)
{
  ACameraManager* manager;
  ACameraIdList* list = NULL;
  char* name = NULL;

  if (!androidCameraLoad())
    return NULL;

  manager = ndk.ACameraManager_create();
  if (!manager)
    return NULL;
  if (ndk.ACameraManager_getCameraIdList(manager, &list) == ACAMERA_OK && list)
  {
    if (index >= 0 && index < list->numCameras)
    {
      int facing = androidCameraFacing(manager, list->cameraIds[index]);
      const char* kind = facing == ACAMERA_LENS_FACING_FRONT ? "Front" : facing == ACAMERA_LENS_FACING_BACK ? "Back" : "External";
      name = iupStrReturnStrf("%s camera %s", kind, list->cameraIds[index]);
    }
    ndk.ACameraManager_deleteCameraIdList(list);
  }
  ndk.ACameraManager_delete(manager);
  return name;
}

char* iupdrvCameraGetPermission(Ihandle* ih)
{
  int state;
  (void)ih;

  if (!androidCameraLoad())
    return "UNAVAILABLE";

  state = iupandroidMediaPermissionState("android.permission.CAMERA");
  return state == 1 ? "GRANTED" : state == 2 ? "DENIED" : state == 0 ? "PROMPT" : "UNAVAILABLE";
}

static unsigned char androidCameraClamp(int value)
{
  return (unsigned char)(value < 0 ? 0 : value > 255 ? 255 : value);
}

static void androidCameraImageAvailable(void* context, AImageReader* reader)
{
  IandroidCamera* camera = (IandroidCamera*)context;
  AImage* image = NULL;
  int32_t width = 0, height = 0, ystride = 0, ustride = 0, vstride = 0, upixel = 1, vpixel = 1;
  uint8_t *yplane = NULL, *uplane = NULL, *vplane = NULL;
  int ylen = 0, ulen = 0, vlen = 0, x, y;
  unsigned char* rgb;

  pthread_mutex_lock(&camera->lock);
  if (camera->closing || ndk.AImageReader_acquireLatestImage(reader, &image) != AMEDIA_OK || !image)
  {
    pthread_mutex_unlock(&camera->lock);
    return;
  }

  ndk.AImage_getWidth(image, &width);
  ndk.AImage_getHeight(image, &height);
  ndk.AImage_getPlaneRowStride(image, 0, &ystride);
  ndk.AImage_getPlaneRowStride(image, 1, &ustride);
  ndk.AImage_getPlaneRowStride(image, 2, &vstride);
  ndk.AImage_getPlanePixelStride(image, 1, &upixel);
  ndk.AImage_getPlanePixelStride(image, 2, &vpixel);
  ndk.AImage_getPlaneData(image, 0, &yplane, &ylen);
  ndk.AImage_getPlaneData(image, 1, &uplane, &ulen);
  ndk.AImage_getPlaneData(image, 2, &vplane, &vlen);

  if (width == camera->width && height == camera->height && yplane && uplane && vplane)
  {
    rgb = camera->orientation ? camera->yuv_rgb : camera->rgb;
    for (y = 0; y < height; y++)
    {
      const uint8_t* yrow = yplane + y * ystride;
      const uint8_t* urow = uplane + (y / 2) * ustride;
      const uint8_t* vrow = vplane + (y / 2) * vstride;
      for (x = 0; x < width; x++)
      {
        int c = yrow[x] - 16, d = urow[(x / 2) * upixel] - 128, e = vrow[(x / 2) * vpixel] - 128;
        rgb[0] = androidCameraClamp((298 * c + 409 * e + 128) >> 8);
        rgb[1] = androidCameraClamp((298 * c - 100 * d - 208 * e + 128) >> 8);
        rgb[2] = androidCameraClamp((298 * c + 516 * d + 128) >> 8);
        rgb += 3;
      }
    }

    if (camera->orientation)
      iupCameraRotate(camera->yuv_rgb, width, height, camera->orientation, camera->rgb);

    if (camera->orientation == 90 || camera->orientation == 270)
      iupCameraFrame(camera->ih, camera->rgb, height, width);
    else
      iupCameraFrame(camera->ih, camera->rgb, width, height);
  }

  ndk.AImage_delete(image);
  pthread_mutex_unlock(&camera->lock);
}

static void androidCameraDisconnected(void* context, ACameraDevice* device)
{
  IandroidCamera* camera = (IandroidCamera*)context;
  (void)device;
  iupCameraError(camera->ih, "Camera disconnected");
}

static void androidCameraDeviceError(void* context, ACameraDevice* device, int error)
{
  IandroidCamera* camera = (IandroidCamera*)context;
  (void)device; (void)error;
  iupCameraError(camera->ih, "Camera error");
}

static void androidCameraSessionState(void* context, ACameraCaptureSession* session)
{
  (void)context; (void)session;
}

static void androidCameraChooseSize(ACameraMetadata* metadata, int req_width, int req_height, int* width, int* height)
{
  ACameraMetadata_const_entry entry;
  long best_score = -1;
  uint32_t i;

  if (ndk.ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry) != ACAMERA_OK)
    return;

  for (i = 0; i + 3 < entry.count; i += 4)
  {
    const int32_t* config = entry.data.i32 + i;
    long score;
    if (config[0] != AIMAGE_FORMAT_YUV_420_888 || config[3] != ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT)
      continue;
    score = labs((long)config[1] * (long)config[2] - (long)req_width * (long)req_height);
    if (best_score < 0 || score < best_score)
    {
      best_score = score;
      *width = config[1];
      *height = config[2];
    }
  }
}

static void androidCameraChooseFps(ACameraMetadata* metadata, int req_fps, int32_t* range)
{
  ACameraMetadata_const_entry entry;
  int best = -1;
  uint32_t i;

  range[0] = range[1] = 0;
  if (ndk.ACameraMetadata_getConstEntry(metadata, ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, &entry) != ACAMERA_OK)
    return;

  for (i = 0; i + 1 < entry.count; i += 2)
  {
    int low = entry.data.i32[i], high = entry.data.i32[i + 1];
    int score = abs(high - req_fps) * 2 + (high - low);
    if (best < 0 || score < best)
    {
      best = score;
      range[0] = low;
      range[1] = high;
    }
  }
}

static void androidCameraRelease(IandroidCamera* camera)
{
  pthread_mutex_lock(&camera->lock);
  camera->closing = 1;
  pthread_mutex_unlock(&camera->lock);

  if (camera->session)
  {
    ndk.ACameraCaptureSession_stopRepeating(camera->session);
    ndk.ACameraCaptureSession_close(camera->session);
  }
  if (camera->request)
    ndk.ACaptureRequest_free(camera->request);
  if (camera->target)
    ndk.ACameraOutputTarget_free(camera->target);
  if (camera->device)
    ndk.ACameraDevice_close(camera->device);
  if (camera->output)
    ndk.ACaptureSessionOutput_free(camera->output);
  if (camera->container)
    ndk.ACaptureSessionOutputContainer_free(camera->container);
  if (camera->window)
    ANativeWindow_release(camera->window);
  if (camera->reader)
    ndk.AImageReader_delete(camera->reader);
  if (camera->manager)
    ndk.ACameraManager_delete(camera->manager);
  pthread_mutex_destroy(&camera->lock);
  free(camera->yuv_rgb);
  free(camera->rgb);
  free(camera);
}

static int androidCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  IandroidCamera* camera;
  ACameraIdList* list = NULL;
  ACameraMetadata* metadata = NULL;
  ACameraMetadata_const_entry entry;
  ACameraDevice_StateCallbacks device_callbacks;
  ACameraCaptureSession_stateCallbacks session_callbacks;
  AImageReader_ImageListener listener;
  int32_t range[2];
  const char* id;

  camera = (IandroidCamera*)calloc(1, sizeof(IandroidCamera));
  camera->ih = ih;
  pthread_mutex_init(&camera->lock, NULL);
  camera->manager = ndk.ACameraManager_create();
  if (!camera->manager || ndk.ACameraManager_getCameraIdList(camera->manager, &list) != ACAMERA_OK || !list || device < 0 || device >= list->numCameras)
  {
    if (list)
      ndk.ACameraManager_deleteCameraIdList(list);
    androidCameraRelease(camera);
    iupCameraError(ih, "Camera not found");
    return 0;
  }
  id = list->cameraIds[device];

  if (ndk.ACameraManager_getCameraCharacteristics(camera->manager, id, &metadata) != ACAMERA_OK || !metadata)
  {
    ndk.ACameraManager_deleteCameraIdList(list);
    androidCameraRelease(camera);
    iupCameraError(ih, "Cannot open camera");
    return 0;
  }
  camera->width = *width;
  camera->height = *height;
  androidCameraChooseSize(metadata, *width, *height, &camera->width, &camera->height);
  androidCameraChooseFps(metadata, *fps, range);
  if (ndk.ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &entry) == ACAMERA_OK && entry.count > 0)
    camera->orientation = entry.data.i32[0];
  ndk.ACameraMetadata_free(metadata);

  memset(&device_callbacks, 0, sizeof(device_callbacks));
  device_callbacks.context = camera;
  device_callbacks.onDisconnected = androidCameraDisconnected;
  device_callbacks.onError = androidCameraDeviceError;
  if (ndk.ACameraManager_openCamera(camera->manager, id, &device_callbacks, &camera->device) != ACAMERA_OK)
  {
    ndk.ACameraManager_deleteCameraIdList(list);
    androidCameraRelease(camera);
    iupCameraError(ih, "Cannot open camera");
    return 0;
  }
  ndk.ACameraManager_deleteCameraIdList(list);

  if (ndk.AImageReader_new(camera->width, camera->height, AIMAGE_FORMAT_YUV_420_888, 4, &camera->reader) != AMEDIA_OK ||
      ndk.AImageReader_getWindow(camera->reader, &camera->window) != AMEDIA_OK)
  {
    androidCameraRelease(camera);
    iupCameraError(ih, "Cannot create image reader");
    return 0;
  }
  ANativeWindow_acquire(camera->window);

  camera->rgb = (unsigned char*)malloc(camera->width * camera->height * 3);
  camera->yuv_rgb = (unsigned char*)malloc(camera->width * camera->height * 3);

  memset(&listener, 0, sizeof(listener));
  listener.context = camera;
  listener.onImageAvailable = androidCameraImageAvailable;
  ndk.AImageReader_setImageListener(camera->reader, &listener);

  if (ndk.ACaptureSessionOutputContainer_create(&camera->container) != ACAMERA_OK ||
      ndk.ACaptureSessionOutput_create(camera->window, &camera->output) != ACAMERA_OK ||
      ndk.ACaptureSessionOutputContainer_add(camera->container, camera->output) != ACAMERA_OK ||
      ndk.ACameraOutputTarget_create(camera->window, &camera->target) != ACAMERA_OK ||
      ndk.ACameraDevice_createCaptureRequest(camera->device, TEMPLATE_PREVIEW, &camera->request) != ACAMERA_OK ||
      ndk.ACaptureRequest_addTarget(camera->request, camera->target) != ACAMERA_OK)
  {
    androidCameraRelease(camera);
    iupCameraError(ih, "Cannot configure camera");
    return 0;
  }
  if (range[1] > 0)
    ndk.ACaptureRequest_setEntry_i32(camera->request, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, range);

  memset(&session_callbacks, 0, sizeof(session_callbacks));
  session_callbacks.context = camera;
  session_callbacks.onClosed = androidCameraSessionState;
  session_callbacks.onReady = androidCameraSessionState;
  session_callbacks.onActive = androidCameraSessionState;
  if (ndk.ACameraDevice_createCaptureSession(camera->device, camera->container, &session_callbacks, &camera->session) != ACAMERA_OK ||
      ndk.ACameraCaptureSession_setRepeatingRequest(camera->session, NULL, 1, &camera->request, NULL) != ACAMERA_OK)
  {
    androidCameraRelease(camera);
    iupCameraError(ih, "Cannot start camera");
    return 0;
  }

  if (camera->orientation == 90 || camera->orientation == 270)
  {
    *width = camera->height;
    *height = camera->width;
  }
  else
  {
    *width = camera->width;
    *height = camera->height;
  }
  if (range[1] > 0)
    *fps = range[1];
  iupAttribSet(ih, "_IUP_CAMERA", (char*)camera);
  return 1;
}

int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  int state;

  if (!androidCameraLoad())
  {
    iupCameraError(ih, "Camera not supported");
    return 0;
  }

  state = iupandroidMediaPermissionState("android.permission.CAMERA");
  if (state == 1)
    return androidCameraStart(ih, device, width, height, fps);

  if (state != 0)
  {
    iupCameraError(ih, "Camera access denied");
    return 0;
  }

  iupandroidMediaRequestPermission("android.permission.CAMERA", ih);
  iupAttribSetStrf(ih, "_IUP_CAMERA_PENDING", "%d %d %d %d", device, *width, *height, *fps);
  return 1;
}

void iupdrvCameraStop(Ihandle* ih)
{
  IandroidCamera* camera = (IandroidCamera*)iupAttribGet(ih, "_IUP_CAMERA");
  iupAttribSet(ih, "_IUP_CAMERA_PENDING", NULL);
  if (!camera)
    return;

  androidCameraRelease(camera);
  iupAttribSet(ih, "_IUP_CAMERA", NULL);
}

void iupandroidCameraPermission(Ihandle* ih, int granted)
{
  char* pending = iupAttribGet(ih, "_IUP_CAMERA_PENDING");
  int device = 0, width = 640, height = 480, fps = 30;

  iupCameraPermission(ih, granted);
  if (!pending)
    return;

  sscanf(pending, "%d %d %d %d", &device, &width, &height, &fps);
  iupAttribSet(ih, "_IUP_CAMERA_PENDING", NULL);
  if (granted)
    androidCameraStart(ih, device, &width, &height, &fps);
  else
    iupCameraError(ih, "Camera access denied");
}

void iupdrvCameraInitClass(Iclass* ic)
{
  (void)ic;
}

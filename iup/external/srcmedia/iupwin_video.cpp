/** \file
 * \brief Video control, Media Foundation engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <mfapi.h>
#include <mfmediaengine.h>
#include <dcomp.h>
#include <d3d11.h>
#include <d3d10.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_video.h"
#include "iup_thread.h"
#include "iup_str.h"

typedef HRESULT (WINAPI* MFStartupFunc)(ULONG, DWORD);
typedef HRESULT (WINAPI* MFCreateAttributesFunc)(IMFAttributes**, UINT32);
typedef HRESULT (WINAPI* MFCreateDXGIDeviceManagerFunc)(UINT*, IMFDXGIDeviceManager**);
typedef HRESULT (WINAPI* DCompositionCreateDeviceFunc)(IDXGIDevice*, REFIID, void**);
typedef HRESULT (WINAPI* UrlCreateFromPathWFunc)(LPCWSTR, LPWSTR, LPDWORD, DWORD);
typedef HRESULT (WINAPI* D3D11CreateDeviceFunc)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

static MFStartupFunc winVideoMFStartup;
static MFCreateAttributesFunc winVideoMFCreateAttributes;
static MFCreateDXGIDeviceManagerFunc winVideoMFCreateDXGIDeviceManager;
static DCompositionCreateDeviceFunc winVideoDCompositionCreateDevice;
static UrlCreateFromPathWFunc winVideoUrlCreateFromPathW;
static D3D11CreateDeviceFunc winVideoD3D11CreateDevice;
static int win_video_loaded = -1;

static int winVideoCheck(IvideoSession* session, HRESULT result, const char* operation)
{
  if (FAILED(result))
  {
    char message[256];
    snprintf(message, sizeof(message), "%s (0x%08lx)", operation, (unsigned long)result);
    iupVideoError(session, message);
    return 0;
  }
  return 1;
}

class IwinVideoNotify final : public IMFMediaEngineNotify
{
  LONG references;
  LONG format;
  CRITICAL_SECTION lock;
  IvideoSession* session;

public:
  explicit IwinVideoNotify(IvideoSession* value) : references(1), format(0), session(value)
  {
    InitializeCriticalSection(&lock);
  }

  ~IwinVideoNotify()
  {
    DeleteCriticalSection(&lock);
  }

  int takeFormat()
  {
    return InterlockedExchange(&format, 0) != 0;
  }

  void disconnect()
  {
    EnterCriticalSection(&lock);
    session = nullptr;
    LeaveCriticalSection(&lock);
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override
  {
    if (!value)
      return E_POINTER;
    *value = nullptr;
    if (iid == __uuidof(IUnknown) || iid == __uuidof(IMFMediaEngineNotify))
    {
      *value = static_cast<IMFMediaEngineNotify*>(this);
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef() override
  {
    return (ULONG)InterlockedIncrement(&references);
  }

  ULONG STDMETHODCALLTYPE Release() override
  {
    ULONG count = (ULONG)InterlockedDecrement(&references);
    if (!count)
      delete this;
    return count;
  }

  HRESULT STDMETHODCALLTYPE EventNotify(DWORD event, DWORD_PTR first, DWORD second) override
  {
    (void)first;
    if (event == MF_MEDIA_ENGINE_EVENT_FORMATCHANGE || event == MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY)
      InterlockedExchange(&format, 1);
    EnterCriticalSection(&lock);
    if (session)
    {
      if (event == MF_MEDIA_ENGINE_EVENT_ERROR)
      {
        char message[128];
        snprintf(message, sizeof(message), "Media Foundation playback error (0x%08lx)", (unsigned long)second);
        iupVideoError(session, message);
      }
      else if (event == MF_MEDIA_ENGINE_EVENT_ENDED)
      {
        iupdrvMutexLock(session->mutex);
        if (!session->closing)
          session->info.ended = 1;
        iupdrvMutexUnlock(session->mutex);
      }
    }
    LeaveCriticalSection(&lock);
    return S_OK;
  }
};

typedef struct _IwinVideo
{
  Ihandle* ih;
  IMFMediaEngineEx* player;
  IwinVideoNotify* notify;
  IDCompositionDevice* composition;
  IDCompositionTarget* target;
  IDCompositionVisual* visual;
  IUnknown* surface;
  ID3D11Device* device;
  IMFDXGIDeviceManager* manager;
  HANDLE swapchain;
  HWND window;
  HWND parent;
  RECT bounds, clip;
  DWORD color;
  int placed, visible, com;
} IwinVideo;

static int winVideoAvailable()
{
  HMODULE mfplat, dcomp, shlwapi, d3d11;

  if (win_video_loaded >= 0)
    return win_video_loaded;
  win_video_loaded = 0;

  mfplat = LoadLibraryW(L"mfplat.dll");
  dcomp = LoadLibraryW(L"dcomp.dll");
  shlwapi = LoadLibraryW(L"shlwapi.dll");
  d3d11 = LoadLibraryW(L"d3d11.dll");
  if (!mfplat || !dcomp || !shlwapi || !d3d11)
    return 0;

  winVideoMFStartup = reinterpret_cast<MFStartupFunc>(GetProcAddress(mfplat, "MFStartup"));
  winVideoMFCreateAttributes = reinterpret_cast<MFCreateAttributesFunc>(GetProcAddress(mfplat, "MFCreateAttributes"));
  winVideoMFCreateDXGIDeviceManager = reinterpret_cast<MFCreateDXGIDeviceManagerFunc>(GetProcAddress(mfplat, "MFCreateDXGIDeviceManager"));
  winVideoDCompositionCreateDevice = reinterpret_cast<DCompositionCreateDeviceFunc>(GetProcAddress(dcomp, "DCompositionCreateDevice"));
  winVideoUrlCreateFromPathW = reinterpret_cast<UrlCreateFromPathWFunc>(GetProcAddress(shlwapi, "UrlCreateFromPathW"));
  winVideoD3D11CreateDevice = reinterpret_cast<D3D11CreateDeviceFunc>(GetProcAddress(d3d11, "D3D11CreateDevice"));
  if (!winVideoMFStartup || !winVideoMFCreateAttributes || !winVideoMFCreateDXGIDeviceManager ||
      !winVideoDCompositionCreateDevice || !winVideoUrlCreateFromPathW || !winVideoD3D11CreateDevice)
    return 0;

  if (FAILED(winVideoMFStartup(MF_VERSION, MFSTARTUP_FULL)))
    return 0;

  win_video_loaded = 1;
  return 1;
}

static BSTR winVideoSource(const char* source, int url)
{
  int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1, nullptr, 0);
  wchar_t* name;
  BSTR result = nullptr;
  if (!count)
    return nullptr;
  name = (wchar_t*)malloc((size_t)count * sizeof(wchar_t));
  if (!name)
    return nullptr;
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1, name, count);
  if (url)
    result = SysAllocString(name);
  else
  {
    DWORD length = GetFullPathNameW(name, 0, nullptr, nullptr);
    wchar_t* path = length ? (wchar_t*)malloc((size_t)length * sizeof(wchar_t)) : nullptr;
    DWORD copied = path ? GetFullPathNameW(name, length, path, nullptr) : 0;
    if (copied > 0 && copied < length)
    {
      DWORD size = length * 3 + 16;
      wchar_t* uri = (wchar_t*)malloc((size_t)size * sizeof(wchar_t));
      if (uri && SUCCEEDED(winVideoUrlCreateFromPathW(path, uri, &size, 0)))
        result = SysAllocString(uri);
      free(uri);
    }
    free(path);
  }
  free(name);
  return result;
}

static void winVideoLayout(IvideoSession* session, Ihandle* ih)
{
  IwinVideo* context = (IwinVideo*)session->context;
  HWND parent = (HWND)IupGetAttribute(ih, "HWND");
  Ihandle* dialog = IupGetDialog(ih);
  int fallback = parent == nullptr;
  RECT bounds, clip;
  POINT point;
  int x, y, width, height, visible = 1;
  unsigned char red = 0, green = 0, blue = 0;
  DWORD color;
  if (fallback)
    parent = (HWND)IupGetAttribute(dialog, "HWND");
  if (!parent || !context->window)
    return;
  iupStrToRGB(IupGetAttribute(ih, "BGCOLOR"), &red, &green, &blue);
  color = ((DWORD)red << 16) | ((DWORD)green << 8) | blue;
  IupGetIntInt(ih, "SCREENPOSITION", &x, &y);
  IupGetIntInt(ih, "DRAWSIZE", &width, &height);
  bounds = {x, y, x + width, y + height};
  clip = bounds;
  for (Ihandle* ancestor = ih; ancestor && ancestor != dialog; ancestor = IupGetParent(ancestor))
  {
    if (!IupGetInt(ancestor, "VISIBLE"))
      visible = 0;
    if (fallback && ancestor != ih && !iupStrEqual(IupGetClassType(ancestor), "void"))
    {
      RECT rectangle;
      int ax, ay, aw, ah;
      IupGetIntInt(ancestor, "SCREENPOSITION", &ax, &ay);
      IupGetIntInt(ancestor, "RASTERSIZE", &aw, &ah);
      rectangle = {ax, ay, ax + aw, ay + ah};
      IntersectRect(&clip, &clip, &rectangle);
    }
  }
  point = {bounds.left, bounds.top};
  ScreenToClient(parent, &point);
  OffsetRect(&clip, -bounds.left, -bounds.top);
  bounds = {point.x, point.y, point.x + width, point.y + height};
  if (parent != context->parent)
  {
    SetParent(context->window, parent);
    context->parent = parent;
    context->placed = 0;
  }
  visible = visible && context->surface && width > 0 && height > 0 && !IsRectEmpty(&clip);
  if (!context->placed || !EqualRect(&bounds, &context->bounds) || !EqualRect(&clip, &context->clip) || context->color != color)
  {
    HRGN region = CreateRectRgn(clip.left, clip.top, clip.right, clip.bottom);
    if (fallback)
      SetWindowPos(context->window, HWND_TOP, bounds.left, bounds.top, width, height, SWP_NOACTIVATE);
    else
      SetWindowPos(context->window, nullptr, bounds.left, bounds.top, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
    if (region && !SetWindowRgn(context->window, region, TRUE))
      DeleteObject(region);
    if (context->player)
    {
      RECT output = {0, 0, width, height};
      MFARGB background = {blue, green, red, 255};
      context->player->UpdateVideoStream(nullptr, &output, &background);
    }
    context->bounds = bounds;
    context->clip = clip;
    context->color = color;
    context->placed = 1;
  }
  if (context->visible != visible)
  {
    ShowWindow(context->window, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    context->visible = visible;
  }
}

static void winVideoClose(IvideoSession* session)
{
  IwinVideo* context = (IwinVideo*)session->context;
  if (!context)
    return;
  if (context->notify)
    context->notify->disconnect();
  if (context->player)
  {
    context->player->Shutdown();
    context->player->Release();
  }
  if (context->target)
    context->target->SetRoot(nullptr);
  if (context->composition)
    context->composition->Commit();
  if (context->surface)
    context->surface->Release();
  if (context->visual)
    context->visual->Release();
  if (context->target)
    context->target->Release();
  if (context->composition)
    context->composition->Release();
  if (context->manager)
    context->manager->Release();
  if (context->device)
    context->device->Release();
  if (context->swapchain)
    CloseHandle(context->swapchain);
  if (context->notify)
    context->notify->Release();
  if (context->window)
    DestroyWindow(context->window);
  if (context->com)
    CoUninitialize();
  free(context);
  session->context = nullptr;
}

static int winVideoCreateDevice(IvideoSession* session, IwinVideo* context)
{
  ID3D10Multithread* multithread = nullptr;
  IDXGIDevice* dxgi = nullptr;
  UINT token;
  HRESULT result = winVideoD3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
                                             nullptr, 0, D3D11_SDK_VERSION, &context->device, nullptr, nullptr);
  if (FAILED(result))
    result = winVideoD3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                       nullptr, 0, D3D11_SDK_VERSION, &context->device, nullptr, nullptr);
  if (!winVideoCheck(session, result, "Cannot create video D3D device"))
    return 0;
  if (SUCCEEDED(context->device->QueryInterface(__uuidof(ID3D10Multithread), (void**)&multithread)))
  {
    multithread->SetMultithreadProtected(TRUE);
    multithread->Release();
  }
  result = winVideoMFCreateDXGIDeviceManager(&token, &context->manager);
  if (SUCCEEDED(result))
    result = context->manager->ResetDevice(context->device, token);
  if (!winVideoCheck(session, result, "Cannot create video DXGI manager"))
    return 0;
  result = context->device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi);
  if (SUCCEEDED(result))
    result = winVideoDCompositionCreateDevice(dxgi, __uuidof(IDCompositionDevice), (void**)&context->composition);
  if (dxgi)
    dxgi->Release();
  if (!winVideoCheck(session, result, "Cannot create DirectComposition device"))
    return 0;
  if (!winVideoCheck(session, context->composition->CreateTargetForHwnd(context->window, TRUE, &context->target), "Cannot create video composition target"))
    return 0;
  return winVideoCheck(session, context->composition->CreateVisual(&context->visual), "Cannot create video composition visual");
}

static int winVideoCreatePlayer(IvideoSession* session, IwinVideo* context)
{
  IMFAttributes* attributes = nullptr;
  IMFMediaEngineClassFactory* factory = nullptr;
  IMFMediaEngine* player = nullptr;
  HRESULT result;
  context->notify = new IwinVideoNotify(session);
  result = winVideoMFCreateAttributes(&attributes, 4);
  if (SUCCEEDED(result))
    result = attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, context->notify);
  if (SUCCEEDED(result))
    result = attributes->SetUINT64(MF_MEDIA_ENGINE_OPM_HWND, (UINT64)(uintptr_t)context->window);
  if (SUCCEEDED(result))
    result = attributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM);
  if (SUCCEEDED(result))
    result = attributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, context->manager);
  if (SUCCEEDED(result))
    result = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMFMediaEngineClassFactory), (void**)&factory);
  if (SUCCEEDED(result))
    result = factory->CreateInstance(0, attributes, &player);
  if (SUCCEEDED(result))
    result = player->QueryInterface(__uuidof(IMFMediaEngineEx), (void**)&context->player);
  if (FAILED(result) && player)
    player->Shutdown();
  if (player)
    player->Release();
  if (factory)
    factory->Release();
  if (attributes)
    attributes->Release();
  if (!winVideoCheck(session, result, "Cannot create Media Foundation engine"))
    return 0;
  return winVideoCheck(session, context->player->EnableWindowlessSwapchainMode(TRUE), "Cannot enable video swap chain");
}

static int winVideoOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  IwinVideo* context = (IwinVideo*)calloc(1, sizeof(IwinVideo));
  DWORD style = 0;
  HRESULT result;
  BSTR uri;
  if (!context)
    return 0;
  session->context = context;
  context->ih = ih;
  result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  context->com = SUCCEEDED(result);
  if (FAILED(result) && result != RPC_E_CHANGED_MODE)
    return winVideoCheck(session, result, "Cannot initialize COM");
  context->parent = (HWND)IupGetAttribute(ih, "HWND");
  if (!context->parent)
  {
    context->parent = (HWND)IupGetAttribute(IupGetDialog(ih), "HWND");
    style = WS_EX_TRANSPARENT;
  }
  if (!context->parent)
  {
    iupVideoError(session, "No native video parent window");
    return 0;
  }
  context->window = CreateWindowExW(style, L"STATIC", L"", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0, context->parent, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (!context->window)
    return winVideoCheck(session, HRESULT_FROM_WIN32(GetLastError()), "Cannot create video window");
  if (!winVideoCreateDevice(session, context) || !winVideoCreatePlayer(session, context))
    return 0;
  uri = winVideoSource(source, url);
  if (!uri)
  {
    iupVideoError(session, "Cannot create video source URI");
    return 0;
  }
  result = context->player->SetSource(uri);
  SysFreeString(uri);
  if (FAILED(result))
    iupVideoError(session, "Media Foundation could not load this source");
  winVideoLayout(session, ih);
  return 1;
}

static void winVideoPlay(IvideoSession* session, int play)
{
  IwinVideo* context = (IwinVideo*)session->context;
  HRESULT result = play ? context->player->Play() : context->player->Pause();
  if (FAILED(result))
    iupVideoError(session, "Media Foundation playback state change failed");
}

static void winVideoSeek(IvideoSession* session, double position)
{
  IwinVideo* context = (IwinVideo*)session->context;
  context->player->SetCurrentTime(position);
}

static void winVideoVolume(IvideoSession* session, int volume, int mute)
{
  IwinVideo* context = (IwinVideo*)session->context;
  context->player->SetVolume(volume / 100.0);
  context->player->SetMuted(mute != 0);
}

static void winVideoAttachSurface(IwinVideo* context)
{
  HANDLE swapchain = nullptr;
  if (context->surface)
  {
    context->visual->SetContent(nullptr);
    context->surface->Release();
    context->surface = nullptr;
  }
  if (context->swapchain)
  {
    CloseHandle(context->swapchain);
    context->swapchain = nullptr;
  }
  if (SUCCEEDED(context->player->GetVideoSwapchainHandle(&swapchain)) &&
      SUCCEEDED(context->composition->CreateSurfaceFromHandle(swapchain, &context->surface)))
  {
    context->swapchain = swapchain;
    context->visual->SetContent(context->surface);
    context->target->SetRoot(context->visual);
    context->composition->Commit();
    context->placed = 0;
  }
  else if (swapchain)
    CloseHandle(swapchain);
}

static void winVideoPoll(IvideoSession* session)
{
  IwinVideo* context = (IwinVideo*)session->context;
  IvideoInfo info;
  IMFMediaTimeRange* range = nullptr;
  DWORD width = 0, height = 0;
  double position, duration;
  iupdrvMutexLock(session->mutex);
  info = session->info;
  iupdrvMutexUnlock(session->mutex);
  info.ready = context->player->GetReadyState() >= MF_MEDIA_ENGINE_READY_HAVE_CURRENT_DATA;
  if (context->player->IsPaused())
    info.state = IVIDEO_PAUSED;
  else
    info.state = context->player->GetReadyState() < MF_MEDIA_ENGINE_READY_HAVE_FUTURE_DATA ? IVIDEO_BUFFERING : IVIDEO_PLAYING;
  info.video = context->player->HasVideo();
  info.audio = context->player->HasAudio();
  position = context->player->GetCurrentTime();
  duration = context->player->GetDuration();
  info.position = isfinite(position) && position > 0 ? position : 0;
  info.duration = isfinite(duration) && duration > 0 ? duration : 0;
  if (SUCCEEDED(context->player->GetSeekable(&range)))
  {
    info.seekable = range->GetLength() > 0;
    range->Release();
  }
  if (SUCCEEDED(context->player->GetNativeVideoSize(&width, &height)) && width <= INT_MAX && height <= INT_MAX)
  {
    info.width = (int)width;
    info.height = (int)height;
    context->player->GetVideoAspectRatio(&width, &height);
    info.aspect = height ? (double)width / height : 0;
  }
  if (context->notify->takeFormat())
    winVideoAttachSurface(context);
  winVideoLayout(session, context->ih);
  iupVideoInfo(session, &info);
}

static const IvideoBackend winVideoBackend = {"NATIVE", winVideoAvailable, winVideoOpen, winVideoClose, winVideoPlay, winVideoSeek, winVideoVolume, winVideoPoll, winVideoLayout};

extern "C" const IvideoBackend* iupdrvVideoBackend(int index)
{
  return index == 0 ? &winVideoBackend : nullptr;
}

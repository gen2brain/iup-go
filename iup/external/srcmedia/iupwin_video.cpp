/** \file
 * \brief Video control, Media Foundation engine
 *
 * See Copyright Notice in "iup.h"
 */

#if !defined(WINVER) || WINVER < 0x0602
#undef WINVER
#define WINVER 0x0602
#endif
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#if !defined(NTDDI_VERSION) || NTDDI_VERSION < 0x06020000
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x06020000
#endif

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <dcomp.h>
#include <d3d11.h>
#include <d3d10.h>
#include <d2d1.h>
#include <dwrite.h>
#include <mferror.h>
#include <mmdeviceapi.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_video.h"
#include "iup_thread.h"
#include "iup_str.h"
#include "iupwin_timedtext.h"

typedef HRESULT (WINAPI* MFStartupFunc)(ULONG, DWORD);
typedef HRESULT (WINAPI* MFCreateAttributesFunc)(IMFAttributes**, UINT32);
typedef HRESULT (WINAPI* MFCreateDXGIDeviceManagerFunc)(UINT*, IMFDXGIDeviceManager**);
typedef HRESULT (WINAPI* DCompositionCreateDeviceFunc)(IDXGIDevice*, REFIID, void**);
typedef HRESULT (WINAPI* UrlCreateFromPathWFunc)(LPCWSTR, LPWSTR, LPDWORD, DWORD);
typedef HRESULT (WINAPI* D3D11CreateDeviceFunc)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
typedef HRESULT (WINAPI* D2D1CreateFactoryFunc)(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS*, void**);
typedef HRESULT (WINAPI* DWriteCreateFactoryFunc)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);
typedef UINT (WINAPI* GetDpiForWindowFunc)(HWND);

static MFStartupFunc winVideoMFStartup;
static MFCreateAttributesFunc winVideoMFCreateAttributes;
static MFCreateDXGIDeviceManagerFunc winVideoMFCreateDXGIDeviceManager;
static DCompositionCreateDeviceFunc winVideoDCompositionCreateDevice;
static UrlCreateFromPathWFunc winVideoUrlCreateFromPathW;
static D3D11CreateDeviceFunc winVideoD3D11CreateDevice;
static D2D1CreateFactoryFunc winVideoD2D1CreateFactory;
static DWriteCreateFactoryFunc winVideoDWriteCreateFactory;
static GetDpiForWindowFunc winVideoGetDpiForWindow;
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

static int winVideoDeviceLost(HRESULT result)
{
  return result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET ||
         result == DXGI_ERROR_DEVICE_HUNG || result == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
}

class IwinVideoNotify final : public IMFMediaEngineNotify
{
  LONG references;
  LONG format;
  LONG lost;
  LONG seeked;
  CRITICAL_SECTION lock;
  IvideoSession* session;

public:
  explicit IwinVideoNotify(IvideoSession* value) : references(1), format(0), lost(0), seeked(0), session(value)
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

  int takeSeeked()
  {
    return InterlockedExchange(&seeked, 0) != 0;
  }

  int takeLost()
  {
    return InterlockedExchange(&lost, 0) != 0;
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
    if (event == MF_MEDIA_ENGINE_EVENT_SEEKED)
      InterlockedExchange(&seeked, 1);
    if (event == MF_MEDIA_ENGINE_EVENT_ERROR && winVideoDeviceLost((HRESULT)second))
    {
      InterlockedExchange(&lost, 1);
      return S_OK;
    }
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

static char* winVideoCueText(IwinVideoTextCue* cue)
{
  char* text = nullptr;
  size_t size = 0;
  for (DWORD i = 0; i < cue->GetLineCount(); i++)
  {
    IwinVideoTextLine* line = nullptr;
    LPWSTR value = nullptr;
    if (FAILED(cue->GetLine(i, &line)))
      continue;
    if (SUCCEEDED(line->GetText(&value)) && value)
    {
      int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
      if (length > 0 && size + length + 1 <= 1024 * 1024)
      {
        char* allocation = (char*)realloc(text, size + (size_t)length + 1);
        if (allocation)
        {
          text = allocation;
          if (size)
            text[size++] = '\n';
          WideCharToMultiByte(CP_UTF8, 0, value, -1, text + size, length, nullptr, nullptr);
          size += (size_t)length - 1;
        }
      }
      CoTaskMemFree(value);
    }
    line->Release();
  }
  return text;
}

typedef struct _IwinVideoCue
{
  DWORD track;
  DWORD id;
  char* text;
} IwinVideoCue;

class IwinVideoSubtitleNotify final : public IwinVideoTextNotify
{
  LONG references;
  CRITICAL_SECTION lock;
  IvideoSession* session;
  IwinVideoCue* cues;
  size_t count;
  size_t capacity;

  void remove(size_t index)
  {
    free(cues[index].text);
    cues[index] = cues[--count];
  }

  void removeMatching(DWORD track, int match_id, DWORD id)
  {
    size_t i = 0;
    while (i < count)
    {
      if (cues[i].track == track && (!match_id || cues[i].id == id))
        remove(i);
      else
        i++;
    }
  }

  void append(DWORD track, DWORD id, char* text)
  {
    if (count == capacity)
    {
      size_t next = capacity ? capacity * 2 : 8;
      IwinVideoCue* allocation = (IwinVideoCue*)realloc(cues, next * sizeof(IwinVideoCue));
      if (allocation)
      {
        cues = allocation;
        capacity = next;
      }
    }
    if (count < capacity)
    {
      cues[count].track = track;
      cues[count].id = id;
      cues[count].text = text;
      count++;
    }
    else
    {
      free(text);
      iupVideoSubtitleError(session, "Cannot allocate subtitle cues");
    }
  }

  void publish()
  {
    size_t length = 0;
    size_t offset = 0;
    char* text;
    if (!session)
      return;
    for (size_t i = 0; i < count; i++)
      length += strlen(cues[i].text) + 1;
    if (length > 1024 * 1024)
    {
      iupVideoSubtitleError(session, "Subtitle cues are too large");
      return;
    }
    text = length ? (char*)malloc(length) : nullptr;
    if (text)
    {
      for (size_t i = 0; i < count; i++)
      {
        size_t size = strlen(cues[i].text);
        if (offset)
          text[offset++] = '\n';
        memcpy(text + offset, cues[i].text, size);
        offset += size;
      }
      text[offset] = 0;
    }
    iupVideoSubtitleCue(session, text);
    free(text);
  }

public:
  explicit IwinVideoSubtitleNotify(IvideoSession* value) : references(1), session(value), cues(nullptr), count(0), capacity(0)
  {
    InitializeCriticalSection(&lock);
  }

  ~IwinVideoSubtitleNotify()
  {
    while (count)
      remove(count - 1);
    free(cues);
    DeleteCriticalSection(&lock);
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
    if (iid == __uuidof(IUnknown) || iid == iwinVideoTextNotifyIID)
    {
      *value = static_cast<IwinVideoTextNotify*>(this);
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
    ULONG value = (ULONG)InterlockedDecrement(&references);
    if (!value)
      delete this;
    return value;
  }

  void STDMETHODCALLTYPE TrackAdded(DWORD) override
  {
  }

  void STDMETHODCALLTYPE TrackReadyStateChanged(DWORD) override
  {
  }

  void STDMETHODCALLTYPE TrackRemoved(DWORD track) override
  {
    TrackSelected(track, FALSE);
  }

  void STDMETHODCALLTYPE TrackSelected(DWORD track, BOOL selected) override
  {
    if (selected)
      return;
    EnterCriticalSection(&lock);
    removeMatching(track, 0, 0);
    publish();
    LeaveCriticalSection(&lock);
  }

  void STDMETHODCALLTYPE Error(int, HRESULT result, DWORD) override
  {
    EnterCriticalSection(&lock);
    if (session)
    {
      char text[128];
      snprintf(text, sizeof(text), "Media Foundation subtitle error (0x%08lx)", (unsigned long)result);
      iupVideoSubtitleError(session, text);
    }
    LeaveCriticalSection(&lock);
  }

  void STDMETHODCALLTYPE Cue(int event, double, IwinVideoTextCue* cue) override
  {
    EnterCriticalSection(&lock);
    if (event == IWINVIDEO_CUE_CLEAR)
    {
      while (count)
        remove(count - 1);
    }
    else if (cue && session)
    {
      DWORD track = cue->GetTrackId();
      DWORD id = cue->GetId();
      removeMatching(track, 1, id);
      if (event == IWINVIDEO_CUE_ACTIVE)
      {
        char* text = winVideoCueText(cue);
        if (text)
          append(track, id, text);
      }
    }
    publish();
    LeaveCriticalSection(&lock);
  }

  void STDMETHODCALLTYPE Reset() override
  {
    Cue(IWINVIDEO_CUE_CLEAR, 0, nullptr);
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
  UINT token;
  BSTR source;
  double resume_position;
  int resume, resume_play, attach_pending;
  HANDLE swapchain;
  HWND window;
  HWND parent;
  RECT bounds, clip;
  DWORD color;
  int placed, visible, com;
  IwinVideoText* timed_text;
  IwinVideoSubtitleNotify* subtitle_notify;
  IDCompositionVisual* caption_visual;
  IDCompositionSurface* caption_surface;
  ID2D1Factory* caption_d2d;
  IDWriteFactory* caption_write;
  HANDLE caption_device;
  char* caption_text;
  int caption_size;
  int caption_width;
  int caption_height;
  int caption_dirty;
  unsigned char caption_red, caption_green, caption_blue;
  double subtitle_delay;
  double aspect;
} IwinVideo;

static int winVideoAvailable()
{
  HMODULE mfplat, dcomp, shlwapi, d3d11, d2d1, dwrite, user32;

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

  d2d1 = LoadLibraryW(L"d2d1.dll");
  dwrite = LoadLibraryW(L"dwrite.dll");
  user32 = GetModuleHandleW(L"user32.dll");
  if (d2d1)
    winVideoD2D1CreateFactory = reinterpret_cast<D2D1CreateFactoryFunc>(GetProcAddress(d2d1, "D2D1CreateFactory"));
  if (dwrite)
    winVideoDWriteCreateFactory = reinterpret_cast<DWriteCreateFactoryFunc>(GetProcAddress(dwrite, "DWriteCreateFactory"));
  if (user32)
    winVideoGetDpiForWindow = reinterpret_cast<GetDpiForWindowFunc>(GetProcAddress(user32, "GetDpiForWindow"));

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

static int winVideoCaptionDpi(IwinVideo* context)
{
  HDC dc;
  int dpi = 0;
  if (winVideoGetDpiForWindow)
    dpi = (int)winVideoGetDpiForWindow(context->window);
  if (dpi > 0)
    return dpi;
  dc = GetDC(context->window);
  if (dc)
  {
    dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(context->window, dc);
  }
  return dpi > 0 ? dpi : 96;
}

static int winVideoLockCaptionDevice(IwinVideo* context)
{
  ID3D11Device* device = nullptr;
  HANDLE handle = nullptr;
  HRESULT result;
  if (!context->caption_device)
  {
    if (FAILED(context->manager->OpenDeviceHandle(&handle)))
      return 0;
    context->caption_device = handle;
  }
  result = context->manager->LockDevice(context->caption_device, __uuidof(ID3D11Device), (void**)&device, TRUE);
  if (result == MF_E_DXGI_NEW_VIDEO_DEVICE)
  {
    context->manager->CloseDeviceHandle(context->caption_device);
    context->caption_device = nullptr;
    if (FAILED(context->manager->OpenDeviceHandle(&handle)))
      return 0;
    context->caption_device = handle;
    result = context->manager->LockDevice(context->caption_device, __uuidof(ID3D11Device), (void**)&device, TRUE);
  }
  if (FAILED(result))
    return 0;
  device->Release();
  return 1;
}

static void winVideoPaintSubtitle(IwinVideo* context, ID2D1RenderTarget* target, POINT offset, int width, int height)
{
  ID2D1SolidColorBrush* brush = nullptr;
  IDWriteTextFormat* format = nullptr;
  IDWriteTextLayout* layout = nullptr;
  const char* caption = context->caption_text ? context->caption_text : "";
  int length = MultiByteToWideChar(CP_UTF8, 0, caption, -1, nullptr, 0);
  wchar_t* text = length > 0 ? (wchar_t*)malloc((size_t)length * sizeof(wchar_t)) : nullptr;
  FLOAT size = (FLOAT)context->caption_size * winVideoCaptionDpi(context) / 72.0f;
  FLOAT picture_width = (FLOAT)width;
  FLOAT bottom = (FLOAT)height;
  double aspect = context->aspect;
  target->SetTransform(D2D1::Matrix3x2F::Translation((FLOAT)offset.x, (FLOAT)offset.y));
  target->PushAxisAlignedClip(D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height), D2D1_ANTIALIAS_MODE_ALIASED);
  target->Clear(D2D1::ColorF(0, 0, 0, 0));
  if (text)
    MultiByteToWideChar(CP_UTF8, 0, caption, -1, text, length);
  if (aspect > 0 && aspect * height > width)
    bottom = (FLOAT)((height + width / aspect) / 2);
  else if (aspect > 0)
    picture_width = (FLOAT)(height * aspect);
  if (text && SUCCEEDED(context->caption_write->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
                                                                  DWRITE_FONT_STRETCH_NORMAL, size, L"", &format)))
  {
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    if (SUCCEEDED(context->caption_write->CreateTextLayout(text, (UINT32)(length - 1), format, picture_width > 20 ? picture_width - 20 : picture_width,
                                                           (FLOAT)height, &layout)) &&
        SUCCEEDED(target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &brush)))
    {
      DWRITE_TEXT_METRICS metrics;
      D2D1_POINT_2F point;
      layout->GetMetrics(&metrics);
      point = D2D1::Point2F(((FLOAT)width - picture_width) / 2 + 10, bottom - metrics.height - 10);
      for (int y = -1; y <= 1; y++)
      {
        for (int x = -1; x <= 1; x++)
        {
          if (x || y)
            target->DrawTextLayout(D2D1::Point2F(point.x + x, point.y + y), layout, brush);
        }
      }
      brush->SetColor(D2D1::ColorF(context->caption_red / 255.0f, context->caption_green / 255.0f, context->caption_blue / 255.0f, 1));
      target->DrawTextLayout(point, layout, brush);
    }
  }
  target->PopAxisAlignedClip();
  if (brush)
    brush->Release();
  if (layout)
    layout->Release();
  if (format)
    format->Release();
  free(text);
}

static void winVideoDrawSubtitle(IwinVideo* context)
{
  RECT client;
  IDXGISurface* surface = nullptr;
  ID2D1RenderTarget* target = nullptr;
  POINT offset;
  int width, height, resize;
  if (!context->composition)
    return;
  GetClientRect(context->window, &client);
  width = client.right;
  height = client.bottom;
  if (width <= 0 || height <= 0 || (!context->caption_dirty && context->caption_width == width && context->caption_height == height))
    return;
  resize = !context->caption_surface || context->caption_width != width || context->caption_height != height;
  context->caption_width = width;
  context->caption_height = height;
  context->caption_dirty = 0;
  if (!winVideoD2D1CreateFactory || !winVideoDWriteCreateFactory)
    return;
  if (!context->caption_d2d && FAILED(winVideoD2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), nullptr, (void**)&context->caption_d2d)))
    return;
  if (!context->caption_write && FAILED(winVideoDWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&context->caption_write)))
    return;
  if (!context->caption_visual)
  {
    if (FAILED(context->composition->CreateVisual(&context->caption_visual)))
      return;
    context->visual->AddVisual(context->caption_visual, TRUE, nullptr);
  }
  if (resize)
  {
    context->caption_visual->SetContent(nullptr);
    if (context->caption_surface)
    {
      context->caption_surface->Release();
      context->caption_surface = nullptr;
    }
    if (FAILED(context->composition->CreateSurface((UINT)width, (UINT)height, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, &context->caption_surface)))
      return;
    context->caption_visual->SetContent(context->caption_surface);
  }
  if (!winVideoLockCaptionDevice(context))
    return;
  if (SUCCEEDED(context->caption_surface->BeginDraw(nullptr, __uuidof(IDXGISurface), (void**)&surface, &offset)))
  {
    D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                                                            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f, 96.0f);
    if (SUCCEEDED(context->caption_d2d->CreateDxgiSurfaceRenderTarget(surface, &properties, &target)))
    {
      target->BeginDraw();
      winVideoPaintSubtitle(context, target, offset, width, height);
      target->EndDraw();
      target->Release();
    }
    surface->Release();
    context->caption_surface->EndDraw();
  }
  context->manager->UnlockDevice(context->caption_device, FALSE);
  context->composition->Commit();
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
  if (context->caption_text || context->caption_surface)
    winVideoDrawSubtitle(context);
}

static void winVideoReleaseDevice(IwinVideo* context)
{
  if (context->caption_surface)
  {
    context->caption_surface->Release();
    context->caption_surface = nullptr;
  }
  if (context->caption_visual)
  {
    context->caption_visual->Release();
    context->caption_visual = nullptr;
  }
  if (context->surface)
  {
    context->surface->Release();
    context->surface = nullptr;
  }
  if (context->visual)
  {
    context->visual->Release();
    context->visual = nullptr;
  }
  if (context->target)
  {
    context->target->Release();
    context->target = nullptr;
  }
  if (context->composition)
  {
    context->composition->Release();
    context->composition = nullptr;
  }
  if (context->manager && context->caption_device)
  {
    context->manager->CloseDeviceHandle(context->caption_device);
    context->caption_device = nullptr;
  }
  if (context->device)
  {
    context->device->Release();
    context->device = nullptr;
  }
  if (context->swapchain)
  {
    CloseHandle(context->swapchain);
    context->swapchain = nullptr;
  }
}

static void winVideoClose(IvideoSession* session)
{
  IwinVideo* context = (IwinVideo*)session->context;
  if (!context)
    return;
  if (context->notify)
    context->notify->disconnect();
  if (context->subtitle_notify)
    context->subtitle_notify->disconnect();
  if (context->timed_text)
  {
    context->timed_text->RegisterNotifications(nullptr);
    context->timed_text->Release();
  }
  if (context->player)
  {
    context->player->Shutdown();
    context->player->Release();
  }
  if (context->target)
    context->target->SetRoot(nullptr);
  if (context->composition)
    context->composition->Commit();
  winVideoReleaseDevice(context);
  if (context->caption_write)
    context->caption_write->Release();
  if (context->caption_d2d)
    context->caption_d2d->Release();
  free(context->caption_text);
  if (context->manager)
    context->manager->Release();
  if (context->source)
    SysFreeString(context->source);
  if (context->notify)
    context->notify->Release();
  if (context->subtitle_notify)
    context->subtitle_notify->Release();
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
  result = context->manager ? S_OK : winVideoMFCreateDXGIDeviceManager(&context->token, &context->manager);
  if (SUCCEEDED(result))
    result = context->manager->ResetDevice(context->device, context->token);
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
  IMFGetService* services = nullptr;
  if (SUCCEEDED(context->player->QueryInterface(__uuidof(IMFGetService), (void**)&services)))
  {
    services->GetService(iwinVideoTextService, iwinVideoTextIID, (void**)&context->timed_text);
    services->Release();
  }
  if (context->timed_text)
  {
    context->subtitle_notify = new IwinVideoSubtitleNotify(session);
    context->timed_text->RegisterNotifications(context->subtitle_notify);
    context->timed_text->SetInBandEnabled(TRUE);
  }
  return winVideoCheck(session, context->player->EnableWindowlessSwapchainMode(TRUE), "Cannot enable video swap chain");
}

static wchar_t* winVideoWide(const char* value)
{
  int count = MultiByteToWideChar(CP_UTF8, 0, value, -1, nullptr, 0);
  wchar_t* result = count ? (wchar_t*)malloc((size_t)count * sizeof(wchar_t)) : nullptr;
  if (result)
    MultiByteToWideChar(CP_UTF8, 0, value, -1, result, count);
  return result;
}

static char* winVideoUTF8(LPCWSTR value)
{
  int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
  char* result = length > 0 ? (char*)malloc((size_t)length) : nullptr;
  if (result)
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result, length, nullptr, nullptr);
  return result;
}

static void winVideoAudioEndpoint(IvideoSession* session, const char* id)
{
  IwinVideo* context = (IwinVideo*)session->context;
  IMFMediaEngineAudioEndpointId* endpoint = nullptr;
  wchar_t* value = winVideoWide(id ? id : "");
  if (value && SUCCEEDED(context->player->QueryInterface(__uuidof(IMFMediaEngineAudioEndpointId), (void**)&endpoint)))
  {
    endpoint->SetAudioEndpointId(value);
    endpoint->Release();
  }
  free(value);
}

static int winVideoAudioDevices(IvideoAudioDevice** devices)
{
  static const PROPERTYKEY friendly_name = {{0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};
  IMMDeviceEnumerator* enumerator = nullptr;
  IMMDeviceCollection* collection = nullptr;
  UINT total = 0, i;
  int count = 0;
  HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator)) &&
      SUCCEEDED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)))
    collection->GetCount(&total);
  for (i = 0; i < total; i++)
  {
    IMMDevice* device = nullptr;
    IPropertyStore* store = nullptr;
    LPWSTR id = nullptr;
    PROPVARIANT name;
    PropVariantInit(&name);
    if (SUCCEEDED(collection->Item(i, &device)) && SUCCEEDED(device->GetId(&id)))
    {
      char* utf8_id = winVideoUTF8(id);
      char* utf8_name = nullptr;
      if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store)) && SUCCEEDED(store->GetValue(friendly_name, &name)) && name.vt == VT_LPWSTR)
        utf8_name = winVideoUTF8(name.pwszVal);
      if (utf8_id)
        count = iupVideoAddAudioDevice(devices, count, utf8_id, utf8_name);
      free(utf8_id);
      free(utf8_name);
    }
    PropVariantClear(&name);
    if (store)
      store->Release();
    if (id)
      CoTaskMemFree(id);
    if (device)
      device->Release();
  }
  if (collection)
    collection->Release();
  if (enumerator)
    enumerator->Release();
  if (SUCCEEDED(init))
    CoUninitialize();
  return count;
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
  if (session->audio_device)
    winVideoAudioEndpoint(session, session->audio_device);
  context->source = uri;
  result = context->player->SetSource(uri);
  if (FAILED(result))
    iupVideoError(session, "Media Foundation could not load this source");
  winVideoLayout(session, ih);
  return 1;
}

static void winVideoPlay(IvideoSession* session, int play)
{
  IwinVideo* context = (IwinVideo*)session->context;
  HRESULT result;
  if (context->resume)
  {
    context->resume_play = play;
    return;
  }
  result = play ? context->player->Play() : context->player->Pause();
  if (FAILED(result))
    iupVideoError(session, "Media Foundation playback state change failed");
}

static void winVideoSeek(IvideoSession* session, double position, int fast)
{
  IwinVideo* context = (IwinVideo*)session->context;
  if (context->resume)
  {
    context->resume_position = position;
    return;
  }
  (void)fast;
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
  if (!context->composition)
    return;
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
  context->attach_pending = 1;
  context->placed = 0;
  if (SUCCEEDED(context->player->GetVideoSwapchainHandle(&swapchain)) && swapchain &&
      SUCCEEDED(context->composition->CreateSurfaceFromHandle(swapchain, &context->surface)))
  {
    context->attach_pending = 0;
    context->swapchain = swapchain;
    context->visual->SetContent(context->surface);
    context->target->SetRoot(context->visual);
    context->composition->Commit();
    context->placed = 0;
  }
  else if (swapchain)
    CloseHandle(swapchain);
}

static void winVideoRecoverDevice(IvideoSession* session, IwinVideo* context)
{
  IMFMediaError* error = nullptr;
  double position = context->player->GetCurrentTime();
  int paused = context->player->IsPaused();
  int failed = SUCCEEDED(context->player->GetError(&error)) && error;
  if (error)
    error->Release();
  winVideoReleaseDevice(context);
  if (!winVideoCreateDevice(session, context))
    return;
  if (failed && context->source)
  {
    context->resume_position = isfinite(position) && position > 0 ? position : 0;
    context->resume_play = !paused;
    context->resume = 1;
    context->player->SetSource(context->source);
  }
  winVideoAttachSurface(context);
  context->caption_dirty = 1;
}

static void winVideoPoll(IvideoSession* session)
{
  IwinVideo* context = (IwinVideo*)session->context;
  IvideoInfo info;
  IMFMediaTimeRange* range = nullptr;
  DWORD width = 0, height = 0;
  double position, duration;
  if (context->notify->takeLost() || (context->device && context->device->GetDeviceRemovedReason() != S_OK))
    winVideoRecoverDevice(session, context);
  if (!context->device)
    return;
  if (context->resume && context->player->GetReadyState() >= MF_MEDIA_ENGINE_READY_HAVE_METADATA)
  {
    context->resume = 0;
    if (context->resume_position > 0)
      context->player->SetCurrentTime(context->resume_position);
    if (context->resume_play)
      context->player->Play();
  }
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
  if (context->resume)
  {
    info.state = IVIDEO_BUFFERING;
    info.position = context->resume_position;
  }
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
    if (context->aspect != info.aspect)
      context->caption_dirty = 1;
    context->aspect = info.aspect;
  }
  if (context->notify->takeSeeked() && context->player->IsPaused())
    context->player->FrameStep(TRUE);
  if (context->notify->takeFormat() || context->attach_pending)
    winVideoAttachSurface(context);
  winVideoLayout(session, context->ih);
  iupVideoInfo(session, &info);
}

static int winVideoSubtitleCount(IvideoSession* session)
{
  IwinVideo* context = (IwinVideo*)session->context;
  IwinVideoTextTrackList* tracks = nullptr;
  int count;
  if (!context->timed_text || FAILED(context->timed_text->GetTextTracks(&tracks)))
    return 0;
  count = (int)tracks->GetLength();
  tracks->Release();
  return count;
}

static char* winVideoSubtitleName(IvideoSession* session, int index, int language)
{
  IwinVideo* context = (IwinVideo*)session->context;
  IwinVideoTextTrackList* tracks = nullptr;
  IwinVideoTextTrack* track = nullptr;
  char* result = nullptr;
  if (index < 1 || !context->timed_text || FAILED(context->timed_text->GetTextTracks(&tracks)))
    return nullptr;
  if (SUCCEEDED(tracks->GetTrack((DWORD)(index - 1), &track)))
  {
    LPWSTR value = nullptr;
    if (language)
      track->GetLanguage(&value);
    else
      track->GetLabel(&value);
    if (value)
    {
      int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
      if (length > 0)
      {
        result = iupStrGetMemory(length);
        WideCharToMultiByte(CP_UTF8, 0, value, -1, result, length, nullptr, nullptr);
      }
      CoTaskMemFree(value);
    }
    track->Release();
  }
  tracks->Release();
  if (result && *result)
    return result;
  return language ? nullptr : iupStrReturnStrf("Track %d", index);
}

static int winVideoSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IwinVideo* context = (IwinVideo*)session->context;
  IwinVideoTextTrackList* tracks = nullptr;
  DWORD count;
  int success = 1;
  if (!context->timed_text || FAILED(context->timed_text->GetTextTracks(&tracks)))
    return index <= 0;
  count = tracks->GetLength();
  if (index > (int)count)
  {
    tracks->Release();
    return 0;
  }
  if (index == -1)
  {
    index = 0;
    for (DWORD i = 0; i < count; i++)
    {
      IwinVideoTextTrack* track = nullptr;
      if (SUCCEEDED(tracks->GetTrack(i, &track)))
      {
        if (!index && track->IsActive())
          index = (int)i + 1;
        track->Release();
      }
    }
  }
  for (DWORD i = 0; i < count; i++)
  {
    IwinVideoTextTrack* track = nullptr;
    if (SUCCEEDED(tracks->GetTrack(i, &track)))
    {
      BOOL selected = index == (int)i + 1;
      if (track->IsActive() != selected && FAILED(context->timed_text->SelectTrack(track->GetId(), selected)))
        success = 0;
      track->Release();
    }
  }
  if (context->subtitle_delay != delay)
  {
    if (SUCCEEDED(context->timed_text->SetCueTimeOffset(delay)))
      context->subtitle_delay = delay;
    else
      success = 0;
  }
  tracks->Release();
  return success;
}

static int winVideoSubtitleText(IvideoSession* session, const char* text, int size, const char* color)
{
  IwinVideo* context = (IwinVideo*)session->context;
  free(context->caption_text);
  context->caption_text = iupStrDup(text);
  context->caption_size = size;
  iupStrToRGB(color, &context->caption_red, &context->caption_green, &context->caption_blue);
  context->caption_dirty = 1;
  if (text || context->caption_surface)
    winVideoDrawSubtitle(context);
  return 1;
}

extern "C" void iupdrvVideoInhibitScreenSaver(int inhibit)
{
  SetThreadExecutionState(inhibit ? ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED : ES_CONTINUOUS);
}

extern "C" char* iupdrvVideoReadSubtitle(const char* filename)
{
  int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, nullptr, 0);
  wchar_t* path;
  FILE* file;
  if (!length)
    return nullptr;
  path = (wchar_t*)malloc((size_t)length * sizeof(wchar_t));
  if (!path)
    return nullptr;
  MultiByteToWideChar(CP_UTF8, 0, filename, -1, path, length);
  file = _wfopen(path, L"rb");
  free(path);
  return iupVideoReadSubtitleStream(file);
}

static const IvideoBackend winVideoBackend = {"NATIVE", winVideoAvailable, winVideoOpen, winVideoClose, winVideoPlay, winVideoSeek, winVideoVolume, winVideoPoll, winVideoLayout, winVideoSubtitleCount, winVideoSubtitleName, winVideoSubtitleSelect, winVideoSubtitleText, 0, winVideoAudioDevices, winVideoAudioEndpoint, nullptr};

extern "C" const IvideoBackend iupVideoVlcBackend;
extern "C" const IvideoBackend iupVideoGstreamerBackend;
extern "C" const IvideoBackend iupVideoMpvBackend;

extern "C" const IvideoBackend* iupdrvVideoBackend(int index)
{
  static const IvideoBackend* backends[] = {&winVideoBackend, &iupVideoMpvBackend, &iupVideoVlcBackend, &iupVideoGstreamerBackend};
  if (index < 0 || index >= (int)(sizeof(backends) / sizeof(backends[0])))
    return nullptr;
  return backends[index];
}

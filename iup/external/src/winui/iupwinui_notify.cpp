/** \file
 * \brief WinUI Driver - Notify Control
 *
 * Uses Windows App SDK AppNotificationManager for modern toast notifications.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <map>
#include <mutex>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_notify.h"
#include "iup_image.h"
#include "iup_tray.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.Windows.AppNotifications.h>
#include <winrt/Microsoft.Windows.AppNotifications.Builder.h>

using namespace winrt;
using namespace Microsoft::Windows::AppNotifications;
using namespace Microsoft::Windows::AppNotifications::Builder;


#define IUPWINUI_NOTIFY_AUX "_IUPWINUI_NOTIFY_AUX"
#define IUPWINUI_NOTIFY_MAX_ICON_SIZE 256

struct IupWinUINotifyAux
{
  uint32_t lastNotificationId;
  hstring tag;
  std::wstring tempImagePath;

  IupWinUINotifyAux() : lastNotificationId(0) {}
};

static bool winui_notify_registered = false;
static event_token winui_notify_invoked_token = {};
static std::map<std::wstring, Ihandle*> winui_notify_map;
static std::mutex winui_notify_map_mutex;
static int winui_notify_tag_counter = 0;

/****************************************************************************
 * Argument Parsing
 ****************************************************************************/

static void winuiNotifyParseArguments(const std::wstring& arg_str, int* action_id, std::wstring* tag_value)
{
  *action_id = 0;
  tag_value->clear();

  size_t pos = 0;
  while (pos < arg_str.size())
  {
    size_t sep = arg_str.find(L';', pos);
    std::wstring token = (sep == std::wstring::npos)
      ? arg_str.substr(pos)
      : arg_str.substr(pos, sep - pos);

    size_t eq = token.find(L'=');
    if (eq != std::wstring::npos)
    {
      std::wstring key = token.substr(0, eq);
      std::wstring val = token.substr(eq + 1);

      if (key == L"action")
        *action_id = _wtoi(val.c_str());
      else if (key == L"tag")
        *tag_value = val;
    }
    pos = (sep == std::wstring::npos) ? arg_str.size() : sep + 1;
  }
}

/****************************************************************************
 * Global Manager Registration
 ****************************************************************************/

static void winuiNotifyEnsureRegistered(void)
{
  if (winui_notify_registered)
    return;

  auto manager = AppNotificationManager::Default();

  winui_notify_invoked_token = manager.NotificationInvoked(
    [](const auto&, AppNotificationActivatedEventArgs const& args)
    {
      std::wstring arg_str(args.Argument().c_str());
      int action_id = 0;
      std::wstring tag_value;
      winuiNotifyParseArguments(arg_str, &action_id, &tag_value);

      Ihandle* ih = NULL;
      {
        std::lock_guard<std::mutex> lock(winui_notify_map_mutex);
        auto it = winui_notify_map.find(tag_value);
        if (it != winui_notify_map.end())
          ih = it->second;
      }

      if (!ih)
        return;

      void* dq_ptr = iupwinuiGetDispatcherQueue();
      if (!dq_ptr)
        return;

      Windows::Foundation::IInspectable dq_obj{nullptr};
      winrt::copy_from_abi(dq_obj, dq_ptr);
      auto dq = dq_obj.as<Microsoft::UI::Dispatching::DispatcherQueue>();

      dq.TryEnqueue([ih, action_id]()
      {
        if (!iupObjectCheck(ih))
          return;

        IFni notify_cb = (IFni)IupGetCallback(ih, "NOTIFY_CB");
        if (notify_cb)
        {
          int ret = notify_cb(ih, action_id);
          if (ret == IUP_CLOSE)
            IupExitLoop();
        }
      });
    });

  manager.Register();
  winui_notify_registered = true;
}

/****************************************************************************
 * Icon Temp File
 ****************************************************************************/

static std::wstring winuiNotifySaveTempBMP(int width, int height, unsigned char* pixels)
{
  WCHAR tempDir[MAX_PATH];
  if (!GetTempPathW(MAX_PATH, tempDir))
    return L"";

  WCHAR tempFile[MAX_PATH];
  swprintf_s(tempFile, MAX_PATH, L"%siup_notify_%u_%d.bmp",
             tempDir, GetCurrentProcessId(), winui_notify_tag_counter);

  HANDLE hFile = CreateFileW(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return L"";

  BITMAPV5HEADER bi;
  ZeroMemory(&bi, sizeof(bi));
  bi.bV5Size = sizeof(bi);
  bi.bV5Width = width;
  bi.bV5Height = -height;
  bi.bV5Planes = 1;
  bi.bV5BitCount = 32;
  bi.bV5Compression = BI_BITFIELDS;
  bi.bV5SizeImage = width * height * 4;
  bi.bV5RedMask   = 0x00FF0000;
  bi.bV5GreenMask = 0x0000FF00;
  bi.bV5BlueMask  = 0x000000FF;
  bi.bV5AlphaMask = 0xFF000000;
  bi.bV5CSType = LCS_sRGB;
  bi.bV5Intent = LCS_GM_IMAGES;

  BITMAPFILEHEADER bf;
  ZeroMemory(&bf, sizeof(bf));
  bf.bfType = 0x4D42;
  bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPV5HEADER);
  bf.bfSize = bf.bfOffBits + bi.bV5SizeImage;

  unsigned char* bmpData = (unsigned char*)malloc(width * height * 4);
  if (!bmpData)
  {
    CloseHandle(hFile);
    DeleteFileW(tempFile);
    return L"";
  }

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      int offset = (y * width + x) * 4;
      unsigned char a = pixels[offset + 0];
      unsigned char r = pixels[offset + 1];
      unsigned char g = pixels[offset + 2];
      unsigned char b = pixels[offset + 3];

      bmpData[offset + 0] = b;
      bmpData[offset + 1] = g;
      bmpData[offset + 2] = r;
      bmpData[offset + 3] = a;
    }
  }

  DWORD written;
  WriteFile(hFile, &bf, sizeof(bf), &written, NULL);
  WriteFile(hFile, &bi, sizeof(bi), &written, NULL);
  WriteFile(hFile, bmpData, width * height * 4, &written, NULL);

  free(bmpData);
  CloseHandle(hFile);

  return std::wstring(tempFile);
}

/****************************************************************************
 * Driver Interface
 ****************************************************************************/

extern "C" int iupdrvNotifyShow(Ihandle* ih)
{
  const char* title;
  const char* body;
  const char* icon;
  int silent;

  winuiNotifyEnsureRegistered();

  IupWinUINotifyAux* aux = winuiGetAux<IupWinUINotifyAux>(ih, IUPWINUI_NOTIFY_AUX);
  if (!aux)
  {
    aux = new IupWinUINotifyAux();
    winuiSetAux(ih, IUPWINUI_NOTIFY_AUX, aux);
  }

  if (!winui_notify_registered)
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, (char*)"Failed to register notification manager");
    return 0;
  }

  title = IupGetAttribute(ih, "TITLE");
  body = IupGetAttribute(ih, "BODY");
  icon = IupGetAttribute(ih, "ICON");
  silent = IupGetInt(ih, "SILENT");

  try
  {
    auto builder = AppNotificationBuilder();

    if (title)
    {
      auto titleProps = AppNotificationTextProperties();
      titleProps.MaxLines(1);
      builder.AddText(iupwinuiStringToHString(title), titleProps);
    }

    if (body)
      builder.AddText(iupwinuiStringToHString(body));

    if (silent)
      builder.MuteAudio();

    /* Icon */
    if (icon && icon[0])
    {
      int img_width, img_height;
      unsigned char* pixels = NULL;

      if (iupdrvGetIconPixels(ih, icon, &img_width, &img_height, &pixels))
      {
        unsigned char* use_pixels = pixels;
        int use_width = img_width;
        int use_height = img_height;
        unsigned char* scaled_pixels = NULL;

        if (img_width > IUPWINUI_NOTIFY_MAX_ICON_SIZE || img_height > IUPWINUI_NOTIFY_MAX_ICON_SIZE)
        {
          double scale;
          if (img_width > img_height)
            scale = (double)IUPWINUI_NOTIFY_MAX_ICON_SIZE / img_width;
          else
            scale = (double)IUPWINUI_NOTIFY_MAX_ICON_SIZE / img_height;

          use_width = (int)(img_width * scale);
          use_height = (int)(img_height * scale);
          if (use_width < 1) use_width = 1;
          if (use_height < 1) use_height = 1;

          scaled_pixels = (unsigned char*)malloc(use_width * use_height * 4);
          if (scaled_pixels)
          {
            iupImageResizeRGBA(img_width, img_height, pixels, use_width, use_height, scaled_pixels, 4);
            use_pixels = scaled_pixels;
          }
          else
          {
            use_width = img_width;
            use_height = img_height;
          }
        }

        std::wstring bmp_path = winuiNotifySaveTempBMP(use_width, use_height, use_pixels);

        if (scaled_pixels)
          free(scaled_pixels);
        free(pixels);

        if (!bmp_path.empty())
        {
          if (!aux->tempImagePath.empty())
            DeleteFileW(aux->tempImagePath.c_str());
          aux->tempImagePath = bmp_path;

          std::wstring uri_str = L"file:///" + bmp_path;
          for (auto& ch : uri_str)
          {
            if (ch == L'\\')
              ch = L'/';
          }

          builder.SetAppLogoOverride(winrt::Windows::Foundation::Uri(uri_str));
        }
      }
    }

    /* Tag */
    char tag_buf[64];
    snprintf(tag_buf, sizeof(tag_buf), "iup_notify_%d", winui_notify_tag_counter++);
    hstring tag(iupwinuiStringToHString(tag_buf));

    /* Default body-click arguments */
    builder.AddArgument(L"action", L"0");
    builder.AddArgument(L"tag", tag);

    /* Action buttons */
    const char* action_attrs[] = { "ACTION1", "ACTION2", "ACTION3", "ACTION4" };
    for (int i = 0; i < 4; i++)
    {
      const char* label = IupGetAttribute(ih, action_attrs[i]);
      if (label && label[0])
      {
        auto button = AppNotificationButton(iupwinuiStringToHString(label));
        wchar_t num[4];
        swprintf_s(num, 4, L"%d", i + 1);
        button.AddArgument(L"action", num);
        button.AddArgument(L"tag", tag);
        builder.AddButton(button);
      }
    }

    auto notification = builder.BuildNotification();
    notification.Tag(tag);

    /* Remove previous tag mapping for this Ihandle */
    if (!aux->tag.empty())
    {
      std::lock_guard<std::mutex> lock(winui_notify_map_mutex);
      winui_notify_map.erase(std::wstring(aux->tag.c_str()));
    }

    aux->tag = tag;
    aux->lastNotificationId = 0;

    {
      std::lock_guard<std::mutex> lock(winui_notify_map_mutex);
      winui_notify_map[std::wstring(tag.c_str())] = ih;
    }

    AppNotificationManager::Default().Show(notification);
    aux->lastNotificationId = notification.Id();

    return 1;
  }
  catch (...)
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, (char*)"Failed to show notification");
    return 0;
  }
}

extern "C" int iupdrvNotifyClose(Ihandle* ih)
{
  IupWinUINotifyAux* aux = winuiGetAux<IupWinUINotifyAux>(ih, IUPWINUI_NOTIFY_AUX);
  if (!aux)
    return 0;

  try
  {
    if (!aux->tag.empty())
    {
      {
        std::lock_guard<std::mutex> lock(winui_notify_map_mutex);
        winui_notify_map.erase(std::wstring(aux->tag.c_str()));
      }
      AppNotificationManager::Default().RemoveByTagAsync(aux->tag).get();
    }
    return 1;
  }
  catch (...)
  {
    return 0;
  }
}

extern "C" void iupdrvNotifyDestroy(Ihandle* ih)
{
  IupWinUINotifyAux* aux = winuiGetAux<IupWinUINotifyAux>(ih, IUPWINUI_NOTIFY_AUX);
  if (aux)
  {
    if (!aux->tag.empty())
    {
      std::lock_guard<std::mutex> lock(winui_notify_map_mutex);
      winui_notify_map.erase(std::wstring(aux->tag.c_str()));
    }

    if (!aux->tempImagePath.empty())
      DeleteFileW(aux->tempImagePath.c_str());

    winuiFreeAux<IupWinUINotifyAux>(ih, IUPWINUI_NOTIFY_AUX);
  }
}

extern "C" int iupdrvNotifyIsAvailable(void)
{
  return 1;
}

extern "C" void iupdrvNotifyInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIMEOUT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}

/** \file
 * \brief WinUI Driver - Recent Menu
 *
 * Uses WinUI MenuFlyoutItem for dynamic recent file menu items.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_menu.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;


static void winuiRecentItemActivate(Ihandle* menu, int index)
{
  Icallback recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  Ihandle* config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");

  if (!recent_cb || !config)
    return;

  char attr_name[32];
  sprintf(attr_name, "_IUP_RECENT_FILE%d", index);
  const char* filename = iupAttribGet(menu, attr_name);

  if (filename)
  {
    IupSetStrAttribute(config, "RECENTFILENAME", filename);
    IupSetStrAttribute(config, "TITLE", filename);
    config->parent = menu;

    if (recent_cb(config) == IUP_CLOSE)
      IupExitLoop();

    config->parent = NULL;
    IupSetAttribute(config, "RECENTFILENAME", NULL);
    IupSetAttribute(config, "TITLE", NULL);
  }
}

extern "C" int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

extern "C" int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  int max_recent, existing, i;

  if (!menu || !menu->handle)
    return -1;

  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  auto items = winuiMenuGetItemsCollection(menu);
  if (!items)
    return -1;

  for (i = 0; i < existing; i++)
  {
    char attr_name[32];
    sprintf(attr_name, "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, attr_name, NULL);
  }

  if (items.Size() > 0)
    items.Clear();

  for (i = 0; i < count; i++)
  {
    char attr_name[32];
    sprintf(attr_name, "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);

    MenuFlyoutItem item;
    item.Text(iupwinuiStringToHString(filenames[i]));

    int index = i;
    item.Click([menu, index](IInspectable const&, RoutedEventArgs const&) {
      winuiRecentItemActivate(menu, index);
    });

    items.Append(item);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);

  return 0;
}

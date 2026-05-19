/** \file
 * \brief Haiku IupSingleInstance (named-port rendezvous)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Application.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_dlglist.h"
#include "iup_singleinstance.h"
}

#define IUPHAIKU_SI_MSG 'IuSI'

static port_id si_port = -1;
static thread_id si_thread = -1;
static volatile bool si_thread_quit = false;

static void haikuSiBuildPortName(const char* name, char* out, size_t cap)
{
  size_t j = 0;
  const char prefix[] = "iup_si_";
  for (size_t i = 0; i < sizeof(prefix) - 1 && j < cap - 1; ++i) out[j++] = prefix[i];
  for (size_t i = 0; name[i] && j < cap - 1; ++i)
  {
    char c = name[i];
    out[j++] = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' ? c : '_';
  }
  out[j] = '\0';
}

static void haikuSiDeliverData(const char* data, int len)
{
  for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    IFnsi cb = (IFnsi)IupGetCallback(dlg, "COPYDATA_CB");
    if (cb) { cb(dlg, (char*)data, len); return; }
  }
}

static int32 haikuSiReaderThread(void* /*arg*/)
{
  while (!si_thread_quit)
  {
    ssize_t size = port_buffer_size_etc(si_port, B_RELATIVE_TIMEOUT, 200000);
    if (size == B_TIMED_OUT) continue;
    if (size < 0) break;

    char* buf = (char*)malloc(size + 1);
    if (!buf) { read_port(si_port, NULL, NULL, 0); continue; }
    int32 code = 0;
    ssize_t got = read_port(si_port, &code, buf, size);
    if (got < 0) { free(buf); break; }
    buf[got] = '\0';

    if (be_app)
    {
      BMessage m(IUPHAIKU_SI_MSG);
      m.AddData("data", B_RAW_TYPE, buf, got);
      BMessenger(be_app).SendMessage(&m);
    }
    free(buf);
  }
  return 0;
}

/* be_app delegates IUPHAIKU_SI_MSG to this via IupHaikuApp::MessageReceived. */
IUP_DRV_API void iuphaikuSingleInstanceDispatch(BMessage* msg)
{
  if (!msg) return;
  const void* data = NULL;
  ssize_t len = 0;
  if (msg->FindData("data", B_RAW_TYPE, &data, &len) == B_OK && data && len > 0)
    haikuSiDeliverData((const char*)data, (int)len);
}

extern "C" IUP_SDK_API int iupdrvSingleInstanceSet(const char* name)
{
  if (!name || !*name) return 0;

  char port_name[B_OS_NAME_LENGTH];
  haikuSiBuildPortName(name, port_name, sizeof(port_name));

  port_id existing = find_port(port_name);
  if (existing >= B_OK)
  {
    char* argv0 = IupGetGlobal("ARGV0");
    const char* data = argv0 ? argv0 : "";
    write_port_etc(existing, 0, data, (ssize_t)strlen(data) + 1, B_RELATIVE_TIMEOUT, 1000000);
    return 1;
  }

  si_port = create_port(64, port_name);
  if (si_port < B_OK) return 0;

  si_thread_quit = false;
  si_thread = spawn_thread(haikuSiReaderThread, "iup_si_reader", B_NORMAL_PRIORITY, NULL);
  if (si_thread < B_OK) { delete_port(si_port); si_port = -1; return 0; }
  resume_thread(si_thread);
  return 0;
}

extern "C" IUP_SDK_API void iupdrvSingleInstanceClose(void)
{
  if (si_port >= B_OK)
  {
    si_thread_quit = true;
    close_port(si_port);
    if (si_thread >= B_OK) { status_t r; wait_for_thread(si_thread, &r); si_thread = -1; }
    delete_port(si_port);
    si_port = -1;
  }
}

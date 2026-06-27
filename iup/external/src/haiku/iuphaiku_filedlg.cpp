/** \file
 * \brief Haiku FileDlg (BFilePanel)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <Application.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <Path.h>
#include <String.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
}


/* async BFilePanel bridged to IupPopup's blocking contract via sem on a private looper */

class IupHaikuFileDlgRecv : public BLooper
{
public:
  IupHaikuFileDlgRecv()
    : BLooper("iup_filedlg_recv"),
      fStatus(-1), fSem(create_sem(0, "iup_fdlg_done")),
      fIsMultiple(false), fDone(false) {}

  ~IupHaikuFileDlgRecv() override
  {
    if (fSem >= B_OK) delete_sem(fSem);
  }

  /* fDone latch: BFilePanel sends a trailing B_CANCEL after REFS_RECEIVED */
  void MessageReceived(BMessage* msg) override
  {
    if (!msg || fDone) { BLooper::MessageReceived(msg); return; }

    if (msg->what == B_REFS_RECEIVED)
    {
      entry_ref ref;
      for (int32 i = 0; msg->FindRef("refs", i, &ref) == B_OK; ++i)
      {
        BPath p(&ref);
        fPaths.push_back(BString(p.Path()));
      }
      if (!fPaths.empty()) fStatus = 0;  /* selected entries exist */
    }
    else if (msg->what == B_SAVE_REQUESTED)
    {
      entry_ref dir_ref;
      const char* name = NULL;
      if (msg->FindRef("directory", &dir_ref) == B_OK &&
          msg->FindString("name", &name) == B_OK)
      {
        BPath dir(&dir_ref);
        BPath full(dir.Path(), name);
        fPaths.push_back(BString(full.Path()));
        BEntry exists(full.Path());
        fStatus = exists.Exists() ? 0 : 1;  /* 0 overwrite, 1 new file */
      }
    }
    else if (msg->what == B_CANCEL)
    {
      fStatus = -1;
    }
    else
    {
      BLooper::MessageReceived(msg);
      return;
    }
    fDone = true;
    release_sem(fSem);
  }

  void WaitDone()
  {
    /* Blocked here, so repaint the caller's window like BAlert::Go does. */
    BWindow* window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
    for (;;)
    {
      status_t status = acquire_sem_etc(fSem, 1, B_RELATIVE_TIMEOUT, 50000);
      if (status == B_TIMED_OUT || status == B_INTERRUPTED)
      {
        if (window) window->UpdateIfNeeded();
        continue;
      }
      break;
    }
  }
  int Status() const { return fStatus; }
  const std::vector<BString>& Paths() const { return fPaths; }
  bool IsMultiple() const { return fIsMultiple; }
  void SetMultiple(bool m) { fIsMultiple = m; }

private:
  int fStatus;
  sem_id fSem;
  std::vector<BString> fPaths;
  bool fIsMultiple;
  bool fDone;
};

/* MULTIVALUE[0]=dir, [1..N]=names (full paths if MULTIVALUEPATH), count=N+1. */
static void haikuFileDlgSetResult(Ihandle* ih, const std::vector<BString>& paths, bool multiple)
{
  BPath parent;
  BPath(paths[0].String()).GetParent(&parent);
  BString dir(parent.Path());
  if (dir.Length() && dir[dir.Length() - 1] != '/') dir << "/";
  iupAttribSetStr(ih, "DIRECTORY", dir.String());

  if (!multiple)
  {
    iupAttribSetStr(ih, "VALUE", paths[0].String());
    return;
  }

  bool multipath = iupAttribGetBoolean(ih, "MULTIVALUEPATH");
  iupAttribSetStrId(ih, "MULTIVALUE", 0, dir.String());

  BString value;
  for (size_t i = 0; i < paths.size(); ++i)
  {
    const char* rel = paths[i].String();
    if (!multipath && paths[i].Compare(dir, dir.Length()) == 0)
      rel += dir.Length();
    iupAttribSetStrId(ih, "MULTIVALUE", (int)i + 1, rel);

    if (i > 0) value << "|";
    value << rel;
  }
  iupAttribSetInt(ih, "MULTIVALUECOUNT", (int)paths.size() + 1);
  iupAttribSetStr(ih, "VALUE", value.String());
}

static int haikuFileDlgPopup(Ihandle* ih, int /*x*/, int /*y*/)
{
  const char* dlg_type = iupAttribGet(ih, "DIALOGTYPE");
  bool is_save = dlg_type && iupStrEqualNoCase(dlg_type, "SAVE");
  bool is_dir  = dlg_type && iupStrEqualNoCase(dlg_type, "DIR");
  bool multiple = iupAttribGetBoolean(ih, "MULTIPLEFILES");
  const char* title = iupAttribGet(ih, "TITLE");
  const char* directory = iupAttribGet(ih, "DIRECTORY");
  const char* file = iupAttribGet(ih, "FILE");

  IupHaikuFileDlgRecv* recv = new IupHaikuFileDlgRecv();
  recv->SetMultiple(multiple && !is_save);
  recv->Run();

  /* DIR: open panel that lets you Choose a directory. */
  uint32 node_flavors = is_dir ? B_DIRECTORY_NODE : (B_FILE_NODE | B_SYMLINK_NODE);

  /* TFilePanel copies the messenger by value; stack is fine. */
  BMessenger msgr(recv);
  BFilePanel* panel = new BFilePanel(
      is_save ? B_SAVE_PANEL : B_OPEN_PANEL,
      &msgr,
      NULL,                           /* directory ref - set below */
      node_flavors,
      multiple && !is_save,
      NULL,                           /* msg (default action) */
      NULL,                            /* RefFilter */
      true,                             /* modal */
      true);                      /* hideWhenDone */

  if (title) panel->Window()->SetTitle(title);
  if (directory) panel->SetPanelDirectory(directory);
  if (is_save && file) panel->SetSaveText(file);
  if (is_dir) panel->SetButtonLabel(B_DEFAULT_BUTTON, "Choose");

  panel->Show();
  recv->WaitDone();

  int status = recv->Status();
  const std::vector<BString>& paths = recv->Paths();

  iupAttribSetInt(ih, "STATUS", status);
  if (status >= 0 && !paths.empty())
    haikuFileDlgSetResult(ih, paths, recv->IsMultiple());
  else
    iupAttribSet(ih, "VALUE", NULL);

  delete panel;
  if (recv->Lock()) recv->Quit();  /* Quit() destroys the looper */
  else              recv->PostMessage(B_QUIT_REQUESTED);

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = haikuFileDlgPopup;

  /* BFilePanel public API has no preview slot / widget insertion */
  iupClassRegisterAttribute(ic, "SHOWPREVIEW", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWGLCANVAS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

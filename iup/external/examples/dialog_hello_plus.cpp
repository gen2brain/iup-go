#include <cstdlib>
#include "iup_plus.h"

static int quit_cb(Ihandle *self)
{
  (void)self;
  return IUP_CLOSE;
}

int main(int argc, char **argv)
{
  Iup::Open(argc, argv);

  Iup::Label label("Very Long Text Label");
  label.SetAttribute("EXPAND", "YES");
  label.SetAttribute("ALIGNMENT", "ACENTER");

  Iup::Button button("Quit");
  button.SetAttribute("PADDING", "DEFAULTBUTTONPADDING");
  button.SetCallback("ACTION", (Icallback)quit_cb);
  IupSetHandle("quitBtName", button.GetHandle());

  Iup::Vbox vbox(label, button);
  vbox.SetAttribute("ALIGNMENT", "ACENTER");
  vbox.SetAttribute("MARGIN", "10x10");
  vbox.SetAttribute("GAP", "10");

  Iup::Dialog dlg(vbox);
  dlg.SetAttribute("TITLE", "Dialog");
  dlg.SetAttribute("DEFAULTESC", "quitBtName");

  dlg.Show();
  Iup::MainLoop();

  Iup::Close();
  return EXIT_SUCCESS;
}

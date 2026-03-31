#include <stdlib.h>
#include "iup.h"

static int quit_cb(Ihandle *self)
{
  (void)self;
  return IUP_CLOSE;
}

int main(int argc, char **argv)
{
  IupOpen(&argc, &argv);

  Ihandle *label = IupLabel("Very Long Text Label");
  IupSetAttribute(label, "EXPAND", "YES");
  IupSetAttribute(label, "ALIGNMENT", "ACENTER");

  Ihandle *button = IupButton("Quit", NULL);
  IupSetAttribute(button, "PADDING", "DEFAULTBUTTONPADDING");
  IupSetCallback(button, "ACTION", (Icallback)quit_cb);
  IupSetHandle("quitBtName", button);

  Ihandle *vbox = IupVbox(label, button, NULL);
  IupSetAttribute(vbox, "ALIGNMENT", "ACENTER");
  IupSetAttribute(vbox, "MARGIN", "10x10");
  IupSetAttribute(vbox, "GAP", "10");

  Ihandle *dlg = IupDialog(vbox);
  IupSetAttribute(dlg, "TITLE", "Dialog");
  IupSetAttribute(dlg, "DEFAULTESC", "quitBtName");

  IupShow(dlg);
  IupMainLoop();

  IupClose();
  return EXIT_SUCCESS;
}

import iup

iup.IupOpen(None, None)

label = iup.Label("Hello from Python", padding="10x10")
text = iup.Text(expand="HORIZONTAL", value="Type here")
greet = iup.Button("Greet")
quit_button = iup.Button("Quit")


def on_greet(ih):
    label.title = "Hello, %s" % text.value
    iup.IupRefresh(label)


def on_quit(ih):
    return iup.IUP_CLOSE


greet.action = on_greet
quit_button.action = on_quit

dialog = iup.Dialog(
    iup.Vbox(label, text, iup.Hbox(iup.Fill(), greet, quit_button, gap="5"), nmargin="10x10", gap="5"),
    title="IUP Python",
)


iup.IupShowXY(dialog, iup.IUP_CENTER, iup.IUP_CENTER)
iup.IupMainLoop()
iup.IupClose()

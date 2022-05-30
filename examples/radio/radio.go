package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	male := iup.Toggle("Male").SetHandle("male")
	female := iup.Toggle("Female").SetHandle("female")

	exclusive := iup.Radio(
		iup.Vbox(
			male,
			female,
		),
	).SetAttribute("VALUE", "female")

	male.SetAttribute("TIP", "Two state button - Exclusive - RADIO")
	female.SetAttribute("TIP", "Two state button - Exclusive - RADIO")

	frame := iup.Frame(exclusive).SetAttribute("TITLE", "Gender")

	dlg := iup.Dialog(
		iup.Hbox(
			iup.Fill(),
			frame,
			iup.Fill(),
		),
	).SetAttributes(`TITLE="Radio", SIZE=140, RESIZE=NO, MINBOX=NO, MAXBOX=NO`)

	iup.Show(dlg)
	iup.MainLoop()
}

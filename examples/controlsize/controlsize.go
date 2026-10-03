package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func column(size string) iup.Ihandle {
	list := iup.List().SetAttributes(`DROPDOWN=YES, 1="Dropdown", 2="Second", VALUE=1`)
	combo := iup.List().SetAttributes(`DROPDOWN=YES, EDITBOX=YES, 1="Combo", 2="Second", VALUE="Combo"`)
	radio := iup.Radio(iup.Hbox(iup.Toggle("One"), iup.Toggle("Two")))
	progress := iup.ProgressBar().SetAttributes(`VALUE=0.6, EXPAND=HORIZONTAL`)

	box := iup.Vbox(
		iup.Label("Label text"),
		iup.Button("Button"),
		iup.Toggle("Checkbox").SetAttribute("VALUE", "ON"),
		radio,
		iup.Toggle("Switch").SetAttributes(`SWITCH=YES, VALUE=ON`),
		iup.Text().SetAttributes(`VALUE="Text", EXPAND=HORIZONTAL`),
		iup.Text().SetAttributes(`SPIN=YES, VALUE=3`),
		list,
		combo,
		iup.Val("HORIZONTAL").SetAttributes(`VALUE=0.4, EXPAND=HORIZONTAL`),
		progress,
		iup.ProgressBar().SetAttributes(`CIRCULAR=YES, MARQUEE=YES`),
	).SetAttributes(`NGAP=6, NMARGIN=8x8`)
	box.SetAttribute("CONTROLSIZE", size)

	return iup.Frame(box).SetAttribute("TITLE", size)
}

func main() {
	iup.Open()
	defer iup.Close()

	dlg := iup.Dialog(
		iup.Hbox(column("MINI"), column("SMALL"), column("REGULAR"), column("LARGE")).SetAttributes(`NGAP=10, NMARGIN=10x10, ALIGNMENT=ATOP`),
	).SetAttribute("TITLE", "CONTROLSIZE")

	iup.Show(dlg)
	iup.MainLoop()
}

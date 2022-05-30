package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	text1 := iup.Text().SetAttributes(`VALUE="Apenas PARTE DESTE TEXTO deve ficar em highlight", SIZE=250, MULTILINE=YES`)
	text2 := iup.Text().SetAttributes(`VALUE="Nada deste texto deve ficar em highlight", SIZE=250`)

	tags := iup.User().SetHandle("tags")
	tags.SetAttributes(`SELECTIONPOS="7:24", BGCOLOR="255 183 115"`)

	text1.SetAttribute("READONLY", "YES")
	text1.SetAttribute("FORMATTING", "YES")
	text1.SetAttribute("ADDFORMATTAG", "tags")

	dlg := iup.Dialog(
		iup.Vbox(
			text1,
			text2,
		),
	).SetAttributes(`TITLE="Highlight"`)

	iup.Show(dlg)
	iup.MainLoop()
}

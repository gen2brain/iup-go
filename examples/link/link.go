package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	link1 := iup.Link("https://www.tecgraf.puc-rio.br/iup/", "IUP Documentation")
	link2 := iup.Link("https://github.com/gen2brain/iup-go", "IUP-Go on GitHub")
	link3 := iup.Link("https://example.com", "Example Link with Custom Handler")

	link3.SetCallback("ACTION", iup.LinkActionFunc(func(ih iup.Ihandle, url string) int {
		fmt.Println("Link clicked! URL:", url)
		iup.Message("Link Clicked", "You clicked the custom handler link.\nURL: "+url)
		return iup.IGNORE
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Click on the links below:"),
			iup.Space().SetAttribute("SIZE", "x10"),
			link1,
			link2,
			link3,
			iup.Space().SetAttribute("SIZE", "x20"),
			iup.Label("The first two links open in your browser."),
			iup.Label("The third link has a custom ACTION callback."),
		).SetAttributes("MARGIN=20x20, GAP=5"),
	).SetAttribute("TITLE", "Link Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

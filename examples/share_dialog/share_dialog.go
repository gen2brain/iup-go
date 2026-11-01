package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	text := iup.Text().SetAttributes("VALUE=\"Shared from IUP\", EXPAND=HORIZONTAL")
	url := iup.Text().SetAttributes("VALUE=https://github.com/gen2brain/iup-go, EXPAND=HORIZONTAL")
	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL")

	share := func(attr, value string) {
		dlg := iup.ShareDlg()
		dlg.SetAttribute("TITLE", "IUP Share")
		dlg.SetAttribute(attr, value)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		status.SetAttribute("TITLE", "STATUS: "+dlg.GetAttribute("STATUS"))
		dlg.Destroy()
	}

	btnText := iup.Button("Share Text").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		share("TEXT", text.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))

	btnURL := iup.Button("Share URL").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		share("URL", url.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))

	btnFile := iup.Button("Share File...").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		fd := iup.FileDlg().SetAttributes("DIALOGTYPE=OPEN, TITLE=\"Choose a File\"")
		iup.Popup(fd, iup.CENTERPARENT, iup.CENTERPARENT)
		if fd.GetInt("STATUS") != -1 {
			share("FILE", fd.GetAttribute("VALUE"))
		}
		fd.Destroy()
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Text:"),
			text,
			iup.Label("URL:"),
			url,
			iup.Vbox(btnText, btnURL, btnFile).SetAttributes("GAP=6, NORMALIZESIZE=HORIZONTAL"),
			status,
		).SetAttributes("MARGIN=10x10, GAP=6"),
	).SetAttribute("TITLE", "Share Dialog")

	iup.Show(dlg)
	iup.MainLoop()
}

package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	filedlg := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="File Save", FILTER="*.jpg", FILTERINFO="Jpeg Files"`)
	defer filedlg.Destroy()

	iup.Popup(filedlg, iup.CENTER, iup.CENTER)

	switch filedlg.GetInt("STATUS") {
	case 1:
		iup.Message("New file", filedlg.GetAttribute("VALUE"))
	case 0:
		iup.Message("File already exists", filedlg.GetAttribute("VALUE"))
	case -1:
		iup.Message("IupFileDlg", "Operation Canceled")
	}
}

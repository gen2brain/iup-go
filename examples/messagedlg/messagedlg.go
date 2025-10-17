package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	msgDlg := iup.MessageDlg().SetAttributes(map[string]interface{}{
		"DIALOGTYPE":    "ERROR",
		"BUTTONS":       "YESNOCANCEL",
		"BUTTONDEFAULT": "3",
		"TITLE":         "Critical",
		"VALUE":         "Something happened!",
	})
	defer msgDlg.Destroy()

	iup.Popup(msgDlg, iup.CENTER, iup.CENTER)
}

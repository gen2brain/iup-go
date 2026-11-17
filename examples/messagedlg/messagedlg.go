package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	response := iup.Label("Response:").SetAttribute("EXPAND", "HORIZONTAL")

	show := func(attrs map[string]interface{}) {
		msgDlg := iup.MessageDlg().SetAttributes(attrs)
		defer msgDlg.Destroy()

		iup.Popup(msgDlg, iup.CENTERPARENT, iup.CENTERPARENT)

		response.SetAttribute("TITLE", "Response: "+msgDlg.GetAttribute("BUTTONRESPONSE"))
	}

	standard := iup.Button("Yes, No, Cancel").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		show(map[string]interface{}{
			"DIALOGTYPE":    "ERROR",
			"BUTTONS":       "YESNOCANCEL",
			"BUTTONDEFAULT": "3",
			"TITLE":         "Critical",
			"VALUE":         "Something happened!",
		})
		return iup.DEFAULT
	}))

	custom := iup.Button("Custom").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		show(map[string]interface{}{
			"DIALOGTYPE":   "QUESTION",
			"BUTTONS":      "CUSTOM",
			"BUTTONTITLE1": "&Save",
			"BUTTONTITLE2": "&Discard",
			"BUTTONTITLE3": "Save &All",
			"BUTTONTITLE4": "&Cancel",
			"TITLE":        "Close",
			"VALUE":        "Save changes to \"notes.txt\"?",
		})
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(standard, custom).SetAttribute("GAP", "10"),
			response,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "MessageDlg")

	iup.Show(dlg)
	iup.MainLoop()
}

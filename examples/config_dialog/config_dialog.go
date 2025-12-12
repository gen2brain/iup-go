package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var config iup.Ihandle
var mainDlg iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("APPNAME", "ConfigDialogExample")

	config = iup.Config()
	config.SetAttribute("APP_NAME", "ConfigDialogExample")
	iup.ConfigLoad(config)

	fmt.Printf("Config file: %s\n", config.GetAttribute("FILENAME"))

	mainDlg = createMainDialog()

	// ConfigDialogShow restores the dialog position and size from config.
	// Position is saved in "X" and "Y" variables, size in "Width" and "Height".
	iup.ConfigDialogShow(config, mainDlg, "MainDialog")

	iup.MainLoop()

	// Save config on exit (CLOSE_CB already called ConfigDialogClosed)
	iup.ConfigSave(config)

	mainDlg.Destroy()
	config.Destroy()
}

func createMainDialog() iup.Ihandle {
	label := iup.Label("Resize and move this window, then close it.\nOn next run, position and size will be restored.\n\nClick 'Open Modal Dialog' to test modal dialog positioning.")
	label.SetAttribute("EXPAND", "YES")
	label.SetAttribute("ALIGNMENT", "ACENTER:ACENTER")

	infoBtn := iup.Button("Show Config Info").SetAttribute("PADDING", "10x5")
	infoBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filename := config.GetAttribute("FILENAME")

		// Read stored values for MainDialog
		x := iup.ConfigGetVariableIntDef(config, "MainDialog", "X", -1)
		y := iup.ConfigGetVariableIntDef(config, "MainDialog", "Y", -1)
		w := iup.ConfigGetVariableIntDef(config, "MainDialog", "Width", -1)
		h := iup.ConfigGetVariableIntDef(config, "MainDialog", "Height", -1)
		maximized := iup.ConfigGetVariableIntDef(config, "MainDialog", "Maximized", 0)

		// Read stored values for ModalDialog
		mx := iup.ConfigGetVariableIntDef(config, "ModalDialog", "X", -1)
		my := iup.ConfigGetVariableIntDef(config, "ModalDialog", "Y", -1)
		mw := iup.ConfigGetVariableIntDef(config, "ModalDialog", "Width", -1)
		mh := iup.ConfigGetVariableIntDef(config, "ModalDialog", "Height", -1)

		msg := fmt.Sprintf("Config File: %s\n\n"+
			"MainDialog:\n  X=%d, Y=%d\n  Width=%d, Height=%d\n  Maximized=%d\n\n"+
			"ModalDialog:\n  X=%d, Y=%d\n  Width=%d, Height=%d",
			filename, x, y, w, h, maximized, mx, my, mw, mh)

		iup.Message("Config Info", msg)
		return iup.DEFAULT
	}))

	modalBtn := iup.Button("Open Modal Dialog").SetAttribute("PADDING", "10x5")
	modalBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		showModalDialog()
		return iup.DEFAULT
	}))

	resetBtn := iup.Button("Reset All").SetAttribute("PADDING", "10x5")
	resetBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		// Clear MainDialog position/size
		iup.ConfigSetVariableStr(config, "MainDialog", "X", "")
		iup.ConfigSetVariableStr(config, "MainDialog", "Y", "")
		iup.ConfigSetVariableStr(config, "MainDialog", "Width", "")
		iup.ConfigSetVariableStr(config, "MainDialog", "Height", "")
		iup.ConfigSetVariableStr(config, "MainDialog", "Maximized", "")

		// Clear ModalDialog position/size
		iup.ConfigSetVariableStr(config, "ModalDialog", "X", "")
		iup.ConfigSetVariableStr(config, "ModalDialog", "Y", "")
		iup.ConfigSetVariableStr(config, "ModalDialog", "Width", "")
		iup.ConfigSetVariableStr(config, "ModalDialog", "Height", "")

		iup.ConfigSave(config)
		iup.Message("Reset", "All positions reset. Restart the app to see default placement.")
		return iup.DEFAULT
	}))

	btnBox1 := iup.Hbox(infoBtn, modalBtn, resetBtn).SetAttributes("GAP=10, ALIGNMENT=ACENTER")

	vbox := iup.Vbox(label, btnBox1).SetAttributes("MARGIN=20x20, GAP=15, ALIGNMENT=ACENTER")

	dlg := iup.Dialog(vbox)
	dlg.SetAttribute("TITLE", "Config Dialog Example - Main Window")
	dlg.SetAttribute("SIZE", "400x200")

	// Save position/size when dialog is closed
	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		// IupConfigDialogClosed saves position (X, Y) and size (Width, Height)
		iup.ConfigDialogClosed(config, ih, "MainDialog")
		return iup.CLOSE
	}))

	return dlg
}

func showModalDialog() {
	label := iup.Label("This is a modal dialog.\nIts position is also remembered.")
	label.SetAttribute("EXPAND", "YES")
	label.SetAttribute("ALIGNMENT", "ACENTER:ACENTER")

	okBtn := iup.Button("OK").SetAttribute("PADDING", "20x5")

	vbox := iup.Vbox(label, okBtn).SetAttributes("MARGIN=20x20, GAP=15, ALIGNMENT=ACENTER")

	dlg := iup.Dialog(vbox)
	dlg.SetAttribute("TITLE", "Modal Dialog")
	dlg.SetAttribute("SIZE", "250x100")
	dlg.SetAttribute("PARENTDIALOG", iup.GetName(mainDlg))

	okBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		// Save position before closing
		iup.ConfigDialogClosed(config, dlg, "ModalDialog")
		return iup.CLOSE
	}))

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		iup.ConfigDialogClosed(config, ih, "ModalDialog")
		return iup.CLOSE
	}))

	// For modal dialogs, call ConfigDialogShow before Popup with IUP_CURRENT
	iup.ConfigDialogShow(config, dlg, "ModalDialog")
	iup.Popup(dlg, iup.CURRENT, iup.CURRENT)

	dlg.Destroy()
}

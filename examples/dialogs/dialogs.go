package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Message Dialog
	btnMessage := iup.Button("Message Dialog")
	btnMessage.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.Message("Information", "This is a simple message dialog.")
		return iup.DEFAULT
	}))

	// Message Dialog with types
	btnMessageDlg := iup.Button("MessageDlg (Warning)")
	btnMessageDlg.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.MessageDlg()
		dlg.SetAttributes(`DIALOGTYPE=WARNING, TITLE="Warning", VALUE="This is a warning message!", BUTTONS=OK`)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		iup.Destroy(dlg)
		return iup.DEFAULT
	}))

	// Alarm Dialog (Yes/No/Cancel)
	btnAlarm := iup.Button("Alarm Dialog")
	btnAlarm.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		ret := iup.Alarm("Confirm", "Do you want to save changes?", "Yes", "No", "Cancel")
		fmt.Printf("Alarm returned: %d\n", ret)
		return iup.DEFAULT
	}))

	// File Dialog (Open)
	btnFileOpen := iup.Button("File Dialog (Open)")
	btnFileOpen.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.FileDlg()
		dlg.SetAttributes(`DIALOGTYPE=OPEN, TITLE="Open File"`)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		if dlg.GetAttribute("STATUS") != "-1" {
			fmt.Printf("Selected file: %s\n", dlg.GetAttribute("VALUE"))
		}
		iup.Destroy(dlg)
		return iup.DEFAULT
	}))

	// File Dialog (Save)
	btnFileSave := iup.Button("File Dialog (Save)")
	btnFileSave.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.FileDlg()
		dlg.SetAttributes(`DIALOGTYPE=SAVE, TITLE="Save File"`)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		if dlg.GetAttribute("STATUS") != "-1" {
			fmt.Printf("Save to: %s\n", dlg.GetAttribute("VALUE"))
		}
		iup.Destroy(dlg)
		return iup.DEFAULT
	}))

	// File Dialog (Directory)
	btnFileDir := iup.Button("File Dialog (Directory)")
	btnFileDir.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.FileDlg()
		dlg.SetAttributes(`DIALOGTYPE=DIR, TITLE="Select Directory"`)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		if dlg.GetAttribute("STATUS") != "-1" {
			fmt.Printf("Selected directory: %s\n", dlg.GetAttribute("VALUE"))
		}
		iup.Destroy(dlg)
		return iup.DEFAULT
	}))

	// Color Dialog
	btnColor := iup.Button("Color Dialog")
	btnColor.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.ColorDlg()
		dlg.SetAttributes(`TITLE="Pick a Color", VALUE="255 128 0"`)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		if dlg.GetAttribute("STATUS") == "1" {
			fmt.Printf("Selected color: %s\n", dlg.GetAttribute("VALUE"))
		}
		iup.Destroy(dlg)
		return iup.DEFAULT
	}))

	// Font Dialog
	btnFont := iup.Button("Font Dialog")
	btnFont.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.FontDlg()
		dlg.SetAttributes(`TITLE="Pick a Font"`)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		if dlg.GetAttribute("STATUS") == "1" {
			fmt.Printf("Selected font: %s\n", dlg.GetAttribute("VALUE"))
		}
		iup.Destroy(dlg)
		return iup.DEFAULT
	}))

	// GetFile convenience function
	btnGetFile := iup.Button("GetFile (convenience)")
	btnGetFile.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		file, ret := iup.GetFile("./*.go")
		if ret >= 0 {
			fmt.Printf("GetFile returned: %s\n", file)
		}
		return iup.DEFAULT
	}))

	// GetColor convenience function
	btnGetColor := iup.Button("GetColor (convenience)")
	btnGetColor.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		col, ret := iup.GetColor(iup.CENTERPARENT, iup.CENTERPARENT)
		if ret == 1 {
			fmt.Printf("GetColor returned: R=%d G=%d B=%d\n", col.R, col.G, col.B)
		}
		return iup.DEFAULT
	}))

	// GetText convenience function
	btnGetText := iup.Button("GetText (convenience)")
	btnGetText.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		text, ret := iup.GetText("Enter Text", "Default value", 256)
		if ret == 1 {
			fmt.Printf("GetText returned: %s\n", text)
		}
		return iup.DEFAULT
	}))

	// ListDialog convenience function
	btnListDialog := iup.Button("ListDialog (convenience)")
	btnListDialog.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		items := []string{"Apple", "Banana", "Cherry", "Date", "Elderberry"}
		marks := make([]bool, len(items))
		ret := iup.ListDialog(2, "Select Fruits", items, 0, 15, 5, &marks)
		if ret != -1 {
			for i, mark := range marks {
				if mark {
					fmt.Printf("ListDialog selected: %s\n", items[i])
				}
			}
		}
		return iup.DEFAULT
	}))

	// Progress Dialog
	btnProgress := iup.Button("Progress Dialog")
	btnProgress.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.ProgressDlg()
		dlg.SetAttributes(`TITLE="Processing", DESCRIPTION="Please wait...", TOTALCOUNT=100`)
		iup.Show(dlg)

		// Simulate progress with delay
		for i := 0; i <= 100; i += 5 {
			dlg.SetAttribute("COUNT", fmt.Sprintf("%d", i))
			iup.LoopStep()
			iup.Flush()
			time.Sleep(50 * time.Millisecond)
		}

		iup.Destroy(dlg)
		return iup.DEFAULT
	}))

	// Create main dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Click buttons to open different dialogs:"),
			iup.Frame(
				iup.Vbox(
					iup.Hbox(btnMessage, btnMessageDlg, btnAlarm).SetAttributes("GAP=5"),
				).SetAttributes("MARGIN=5x5"),
			).SetAttribute("TITLE", "Message Dialogs"),
			iup.Frame(
				iup.Vbox(
					iup.Hbox(btnFileOpen, btnFileSave, btnFileDir).SetAttributes("GAP=5"),
				).SetAttributes("MARGIN=5x5"),
			).SetAttribute("TITLE", "File Dialogs"),
			iup.Frame(
				iup.Vbox(
					iup.Hbox(btnColor, btnFont).SetAttributes("GAP=5"),
				).SetAttributes("MARGIN=5x5"),
			).SetAttribute("TITLE", "Picker Dialogs"),
			iup.Frame(
				iup.Vbox(
					iup.Hbox(btnGetFile, btnGetColor, btnGetText, btnListDialog).SetAttributes("GAP=5"),
				).SetAttributes("MARGIN=5x5"),
			).SetAttribute("TITLE", "Convenience Functions"),
			iup.Frame(
				iup.Vbox(
					iup.Hbox(btnProgress).SetAttributes("GAP=5"),
				).SetAttributes("MARGIN=5x5"),
			).SetAttribute("TITLE", "Other Dialogs"),
			iup.Fill(),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttributes("TITLE=Dialog Examples, SIZE=500x350")

	iup.Show(dlg)
	iup.MainLoop()
}

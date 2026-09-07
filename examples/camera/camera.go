//go:build media

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func setStatus(text string) {
	iup.GetHandle("status").SetAttribute("TITLE", text)
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.MediaOpen()

	camera := iup.Camera().SetAttributes(`EXPAND=YES`)
	camera.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, message string) int {
		setStatus("Error: " + message)
		return iup.DEFAULT
	}))
	camera.SetCallback("PERMISSION_CB", iup.PermissionFunc(func(ih iup.Ihandle, granted int) int {
		if granted == 1 {
			setStatus("Permission granted")
		} else {
			setStatus("Permission denied")
		}
		return iup.DEFAULT
	}))

	frames := 0
	camera.SetCallback("FRAME_CB", iup.FrameFunc(func(ih iup.Ihandle, width, height int, data []byte) int {
		frames++
		if frames%30 == 0 {
			setStatus(fmt.Sprintf("%dx%d at %s fps, %d frames", width, height, ih.GetAttribute("FPS"), frames))
		}
		return iup.DEFAULT
	}))

	devices := iup.List().SetAttributes(`DROPDOWN=YES, EXPAND=HORIZONTAL`)
	count := camera.GetInt("DEVICECOUNT")
	for i := 0; i < count; i++ {
		devices.SetAttribute(fmt.Sprintf("%d", i+1), camera.GetAttribute("DEVICENAME", i))
	}
	if count > 0 {
		devices.SetAttribute("VALUE", "1")
	}
	devices.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			camera.SetAttribute("DEVICE", item-1)
		}
		return iup.DEFAULT
	}))

	run := iup.Toggle("Run")
	run.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		camera.SetAttribute("RUN", state)
		ih.SetAttribute("VALUE", camera.GetAttribute("RUN"))
		setStatus(camera.GetAttribute("RUN"))
		return iup.DEFAULT
	}))

	mirror := iup.Toggle("Mirror")
	mirror.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		camera.SetAttribute("MIRROR", state)
		return iup.DEFAULT
	}))

	snapshot := iup.Button("Snapshot...")
	snapshot.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "SAVE")
		filedlg.SetAttribute("TITLE", "Save Snapshot")
		filedlg.SetAttribute("EXTFILTER", "PNG Files|*.png|All Files|*.*")
		iup.Popup(filedlg, iup.CENTER, iup.CENTER)
		if filedlg.GetInt("STATUS") != -1 {
			camera.SetAttribute("SNAPSHOT", filedlg.GetAttribute("VALUE"))
			setStatus("Saved " + filedlg.GetAttribute("VALUE"))
		}
		filedlg.Destroy()
		return iup.DEFAULT
	}))

	status := iup.Label("Available: " + camera.GetAttribute("AVAILABLE") + ", permission: " + camera.GetAttribute("PERMISSION")).SetAttributes(`EXPAND=HORIZONTAL`)
	status.SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(devices, run, mirror, snapshot).SetAttributes(`GAP=5, ALIGNMENT=ACENTER`),
			camera,
			status,
		).SetAttributes(`GAP=5, MARGIN=10x10`),
	).SetAttributes(`TITLE="Camera", SIZE=HALFxHALF`)

	iup.Show(dlg)
	iup.MainLoop()
}

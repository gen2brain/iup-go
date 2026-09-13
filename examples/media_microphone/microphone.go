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

	microphone := iup.Microphone()
	microphone.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, message string) int {
		setStatus("Error: " + message)
		return iup.DEFAULT
	}))
	microphone.SetCallback("PERMISSION_CB", iup.PermissionFunc(func(ih iup.Ihandle, granted int) int {
		if granted == 1 {
			setStatus("Permission granted")
		} else {
			setStatus("Permission denied")
		}
		return iup.DEFAULT
	}))

	total := 0
	microphone.SetCallback("SAMPLES_CB", iup.SamplesFunc(func(ih iup.Ihandle, frames, channels int, samples []int16) int {
		total += frames
		setStatus(fmt.Sprintf("%d channels at %s Hz, %d frames", channels, ih.GetAttribute("SAMPLERATE"), total))
		return iup.DEFAULT
	}))

	devices := iup.List().SetAttributes(`DROPDOWN=YES, EXPAND=HORIZONTAL`)
	count := microphone.GetInt("DEVICECOUNT")
	for i := 0; i < count; i++ {
		devices.SetAttributeId("", i+1, microphone.GetAttribute("DEVICENAME", i))
	}
	if count > 0 {
		devices.SetAttribute("VALUE", "1")
	}
	devices.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			microphone.SetAttribute("DEVICE", item-1)
		}
		return iup.DEFAULT
	}))

	run := iup.Toggle("Run")
	run.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		total = 0
		microphone.SetAttribute("RUN", state)
		ih.SetAttribute("VALUE", microphone.GetAttribute("RUN"))
		setStatus(microphone.GetAttribute("RUN"))
		return iup.DEFAULT
	}))

	record := iup.Toggle("Record...")
	record.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		if state == 0 {
			microphone.SetAttribute("FILE", nil)
			return iup.DEFAULT
		}
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "SAVE")
		filedlg.SetAttribute("TITLE", "Record To")
		filedlg.SetAttribute("EXTFILTER", "WAV Files|*.wav|All Files|*.*")
		iup.Popup(filedlg, iup.CENTER, iup.CENTER)
		if filedlg.GetInt("STATUS") != -1 {
			microphone.SetAttribute("FILE", filedlg.GetAttribute("VALUE"))
		} else {
			ih.SetAttribute("VALUE", "OFF")
		}
		filedlg.Destroy()
		return iup.DEFAULT
	}))

	level := iup.ProgressBar().SetAttributes(`MIN=0, MAX=100, EXPAND=HORIZONTAL`)

	timer := iup.Timer().SetAttributes("TIME=50, RUN=YES")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		level.SetAttribute("VALUE", microphone.GetAttribute("LEVEL"))
		return iup.DEFAULT
	}))

	status := iup.Label("Available: " + microphone.GetAttribute("AVAILABLE") + ", permission: " + microphone.GetAttribute("PERMISSION")).SetAttributes(`EXPAND=HORIZONTAL`)
	status.SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(devices, run, record).SetAttributes(`GAP=5, ALIGNMENT=ACENTER`),
			iup.Hbox(iup.Label("Level"), level).SetAttributes(`GAP=5, ALIGNMENT=ACENTER`),
			status,
		).SetAttributes(`GAP=5, MARGIN=10x10`),
	).SetAttributes(`TITLE="Microphone", SIZE=HALFx`)

	iup.Show(dlg)
	iup.MainLoop()
}

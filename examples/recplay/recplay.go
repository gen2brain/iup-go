package main

import (
	"fmt"
	"os"
	"path/filepath"
	"sync/atomic"

	"github.com/gen2brain/iup-go/iup"
)

const recFileName = "iup_recplay.rec"

var (
	recording atomic.Bool
	playing   atomic.Bool
	clicks    atomic.Int64
	recPath   string
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	recPath = filepath.Join(iup.GetGlobal("TMPDIR"), recFileName)

	status := iup.Label("Idle.").SetAttribute("EXPAND", "HORIZONTAL")
	pathLbl := iup.Label("File: " + recPath).SetAttributes("EXPAND=HORIZONTAL, SELECTABLE=YES")

	clickBtn := iup.Button("Click me").SetAttribute("PADDING", "DEFAULTBUTTONPADDING")
	clickInfo := iup.Label("Clicks: 0").SetAttribute("EXPAND", "HORIZONTAL")
	clickBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		n := clicks.Add(1)
		iup.GetHandle("clickInfo").SetAttribute("TITLE", fmt.Sprintf("Clicks: %d", n))
		return iup.DEFAULT
	}))

	textInput := iup.Text().SetAttribute("EXPAND", "HORIZONTAL")
	textInfo := iup.Label("Text: ").SetAttribute("EXPAND", "HORIZONTAL")
	textInput.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("textInfo").SetAttribute("TITLE", "Text: "+ih.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))

	slider := iup.Val("HORIZONTAL").SetAttribute("EXPAND", "HORIZONTAL")
	sliderInfo := iup.Label("Slider: 0.00").SetAttribute("EXPAND", "HORIZONTAL")
	slider.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("sliderInfo").SetAttribute("TITLE",
			fmt.Sprintf("Slider: %.2f", ih.GetFloat("VALUE")))
		return iup.DEFAULT
	}))

	toggle := iup.Toggle("Toggle me")
	toggleInfo := iup.Label("Toggle: OFF").SetAttribute("EXPAND", "HORIZONTAL")
	toggle.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		s := "OFF"
		if state == 1 {
			s = "ON"
		}
		iup.GetHandle("toggleInfo").SetAttribute("TITLE", "Toggle: "+s)
		return iup.DEFAULT
	}))

	list := iup.List().SetAttributes(map[string]string{
		"1":            "Apple",
		"2":            "Banana",
		"3":            "Cherry",
		"4":            "Date",
		"5":            "Elderberry",
		"DROPDOWN":     "YES",
		"VISIBLEITEMS": "5",
		"EXPAND":       "HORIZONTAL",
	})
	listInfo := iup.Label("List: (none)").SetAttribute("EXPAND", "HORIZONTAL")
	list.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		pos := ih.GetInt("VALUE")
		text := ""
		if pos > 0 {
			text = ih.GetAttribute(fmt.Sprintf("%d", pos))
		}
		iup.GetHandle("listInfo").SetAttribute("TITLE", "List: "+text)
		return iup.DEFAULT
	}))

	recordBtn := iup.Button("&Record").SetAttribute("PADDING", "DEFAULTBUTTONPADDING")
	stopBtn := iup.Button("&Stop").SetAttribute("PADDING", "DEFAULTBUTTONPADDING").
		SetAttribute("ACTIVE", "NO")
	playBtn := iup.Button("&Play").SetAttribute("PADDING", "DEFAULTBUTTONPADDING").
		SetAttribute("ACTIVE", "NO")
	resetBtn := iup.Button("R&eset").SetAttribute("PADDING", "DEFAULTBUTTONPADDING")

	iup.SetHandle("status", status)
	iup.SetHandle("clickBtn", clickBtn)
	iup.SetHandle("clickInfo", clickInfo)
	iup.SetHandle("textInput", textInput)
	iup.SetHandle("textInfo", textInfo)
	iup.SetHandle("slider", slider)
	iup.SetHandle("sliderInfo", sliderInfo)
	iup.SetHandle("toggle", toggle)
	iup.SetHandle("toggleInfo", toggleInfo)
	iup.SetHandle("list", list)
	iup.SetHandle("listInfo", listInfo)
	iup.SetHandle("recordBtn", recordBtn)
	iup.SetHandle("stopBtn", stopBtn)
	iup.SetHandle("playBtn", playBtn)
	iup.SetHandle("resetBtn", resetBtn)

	recordBtn.SetCallback("ACTION", iup.ActionFunc(doRecord))
	stopBtn.SetCallback("ACTION", iup.ActionFunc(doStop))
	playBtn.SetCallback("ACTION", iup.ActionFunc(doPlay))
	resetBtn.SetCallback("ACTION", iup.ActionFunc(doReset))

	controls := iup.Frame(iup.Vbox(
		iup.Hbox(clickBtn, clickInfo).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Hbox(textInput, textInfo).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Hbox(slider, sliderInfo).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Hbox(toggle, toggleInfo).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Hbox(list, listInfo).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
	).SetAttributes("NMARGIN=10x10, NGAP=6")).SetAttribute("TITLE", "Interact with these")

	transport := iup.Hbox(recordBtn, stopBtn, playBtn, resetBtn, iup.Fill()).
		SetAttributes("NGAP=8")

	dlg := iup.Dialog(iup.Vbox(
		status, controls, transport, pathLbl,
	).SetAttributes("NMARGIN=10x10, NGAP=8")).SetAttribute("TITLE", "Record / Play Input")

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		if recording.Load() {
			iup.RecordInput("", 0)
		}
		if playing.Load() {
			iup.PlayInput("")
		}
		return iup.DEFAULT
	}))

	if _, err := os.Stat(recPath); err == nil {
		iup.GetHandle("playBtn").SetAttribute("ACTIVE", "YES")
		iup.GetHandle("status").SetAttribute("TITLE", "Existing recording found.")
	}

	driver := iup.GetGlobal("DRIVER")
	if !playbackSupported(driver) {
		iup.GetHandle("status").SetAttribute("TITLE",
			"Warning: "+driver+" driver does not synthesize input events. Recording works, but Play will not dispatch events.")
	}

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func isActive(name string) bool {
	return iup.GetHandle(name).GetAttribute("ACTIVE") != "NO"
}

func doRecord(ih iup.Ihandle) int {
	if !isActive("recordBtn") {
		return iup.DEFAULT
	}
	if iup.RecordInput(recPath, iup.RECTEXT) != iup.NOERROR {
		iup.GetHandle("status").SetAttribute("TITLE", "Error: could not open file for recording")
		return iup.DEFAULT
	}
	recording.Store(true)
	setRecording("Recording...")
	return iup.DEFAULT
}

func doStop(ih iup.Ihandle) int {
	if !isActive("stopBtn") {
		return iup.DEFAULT
	}
	switch {
	case recording.Load():
		iup.RecordInput("", 0)
		recording.Store(false)
		setIdle("Saved to " + recPath)
	case playing.Load():
		iup.PlayInput("")
		playing.Store(false)
		setIdle("Playback stopped.")
	}
	return iup.DEFAULT
}

func doPlay(ih iup.Ihandle) int {
	if !isActive("playBtn") {
		return iup.DEFAULT
	}
	if iup.PlayInput(recPath) != iup.NOERROR {
		iup.GetHandle("status").SetAttribute("TITLE", "Error: could not open recording file")
		return iup.DEFAULT
	}
	playing.Store(true)
	setPlaying("Playing back. Don't move the mouse, the recorded events are firing.")
	return iup.DEFAULT
}

func doReset(ih iup.Ihandle) int {
	if !isActive("resetBtn") {
		return iup.DEFAULT
	}
	resetControls()
	return iup.DEFAULT
}

func playbackSupported(driver string) bool {
	switch driver {
	case "Win32", "WinUI", "Cocoa", "GTK", "Motif", "Qt", "FLTK", "Android", "Haiku":
		return true
	}
	return false
}

func resetControls() {
	clicks.Store(0)
	iup.GetHandle("clickInfo").SetAttribute("TITLE", "Clicks: 0")
	iup.GetHandle("textInput").SetAttribute("VALUE", "")
	iup.GetHandle("textInfo").SetAttribute("TITLE", "Text: ")
	iup.GetHandle("slider").SetAttribute("VALUE", "0")
	iup.GetHandle("sliderInfo").SetAttribute("TITLE", "Slider: 0.00")
	iup.GetHandle("toggle").SetAttribute("VALUE", "OFF")
	iup.GetHandle("toggleInfo").SetAttribute("TITLE", "Toggle: OFF")
	iup.GetHandle("list").SetAttribute("VALUE", "0")
	iup.GetHandle("listInfo").SetAttribute("TITLE", "List: (none)")
	iup.GetHandle("status").SetAttribute("TITLE", "Reset. Ready to Record or Play.")
}

func setRecording(msg string) {
	iup.GetHandle("status").SetAttribute("TITLE", msg)
	iup.GetHandle("recordBtn").SetAttribute("ACTIVE", "NO")
	iup.GetHandle("stopBtn").SetAttribute("ACTIVE", "YES")
	iup.GetHandle("playBtn").SetAttribute("ACTIVE", "NO")
	iup.GetHandle("resetBtn").SetAttribute("ACTIVE", "NO")
}

func setPlaying(msg string) {
	iup.GetHandle("status").SetAttribute("TITLE", msg)
	iup.GetHandle("recordBtn").SetAttribute("ACTIVE", "NO")
	iup.GetHandle("stopBtn").SetAttribute("ACTIVE", "YES")
	iup.GetHandle("playBtn").SetAttribute("ACTIVE", "NO")
	iup.GetHandle("resetBtn").SetAttribute("ACTIVE", "NO")
}

func setIdle(msg string) {
	iup.GetHandle("status").SetAttribute("TITLE", msg)
	iup.GetHandle("recordBtn").SetAttribute("ACTIVE", "YES")
	iup.GetHandle("stopBtn").SetAttribute("ACTIVE", "NO")
	iup.GetHandle("playBtn").SetAttribute("ACTIVE", "YES")
	iup.GetHandle("resetBtn").SetAttribute("ACTIVE", "YES")
}

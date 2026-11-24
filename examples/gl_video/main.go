//go:build media && gl

package main

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/gen2brain/iup-go/iup"
)

func clock(seconds float64) string {
	s := int(seconds + 0.5)
	return fmt.Sprintf("%d:%02d", s/60, s%60)
}

func main() {
	iup.Open()
	defer iup.Close()
	iup.GLCanvasOpen()
	iup.MediaOpen()

	video := iup.GLVideo()
	if video == 0 || video.GetAttribute("AVAILABLE") != "YES" {
		iup.Message("IupGLVideo", "libmpv is not installed.")
		return
	}

	status := iup.Label(video.GetAttribute("STATE")).SetAttributes("EXPAND=HORIZONTAL, ELLIPSIS=YES")
	elapsed := iup.Label("0:00 / 0:00")
	position := iup.Val("HORIZONTAL").SetAttributes("EXPAND=HORIZONTAL, MIN=0, MAX=1")

	load := func(file string) {
		video.SetAttribute("FILE", file)
		video.SetAttribute("PLAY", "YES")
		iup.GetDialog(video).SetAttribute("TITLE", filepath.Base(file))
	}

	video.SetCallback("STATECHANGED_CB", iup.StateChangedFunc(func(_ iup.Ihandle, state string) int {
		status.SetAttribute("TITLE", state)
		return iup.DEFAULT
	}))
	video.SetCallback("ERROR_CB", iup.ErrorFunc(func(_ iup.Ihandle, message string) int {
		status.SetAttribute("TITLE", "ERROR: "+message)
		return iup.DEFAULT
	}))

	open := iup.Button("Open...").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		file := iup.FileDlg().SetAttributes(`DIALOGTYPE=OPEN, TITLE="Open Video"`)
		file.SetAttribute("EXTFILTER", "Video Files|*.mp4;*.m4v;*.mov;*.mkv;*.webm;*.avi|All Files|*.*|")
		iup.Popup(file, iup.CENTER, iup.CENTER)
		if file.GetInt("STATUS") != -1 {
			load(file.GetAttribute("VALUE"))
		}
		file.Destroy()
		return iup.DEFAULT
	}))
	play := iup.Button("Play / Pause").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		if video.GetAttribute("STATE") == "PLAYING" {
			video.SetAttribute("PAUSE", "YES")
		} else {
			video.SetAttribute("PLAY", "YES")
		}
		return iup.DEFAULT
	}))

	position.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		video.SetAttribute("POSITION", ih.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))
	timer := iup.Timer().SetAttributes("TIME=250, RUN=YES")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(iup.Ihandle) int {
		duration := video.GetDouble("DURATION")
		if duration > 0 {
			position.SetAttribute("MAX", fmt.Sprintf("%f", duration))
			position.SetAttribute("VALUE", video.GetAttribute("POSITION"))
		}
		elapsed.SetAttribute("TITLE", clock(video.GetDouble("POSITION"))+" / "+clock(duration))
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(
		video,
		iup.Hbox(open, play, position, elapsed).SetAttributes("ALIGNMENT=ACENTER, GAP=6, NMARGIN=6x4"),
		status,
	)).SetAttribute("TITLE", "IupGLVideo")
	iup.Show(dlg)
	if len(os.Args) > 1 {
		load(os.Args[1])
	}
	iup.MainLoop()
}

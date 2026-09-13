//go:build media

package main

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func setStatus(text string) {
	iup.GetHandle("status").SetAttribute("TITLE", text)
}

func clock(seconds float64) string {
	s := int(seconds + 0.5)
	return fmt.Sprintf("%d:%02d", s/60, s%60)
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.MediaOpen()

	audio := iup.Audio()
	audio.SetCallback("PLAYEND_CB", iup.PlayEndFunc(func(ih iup.Ihandle) int {
		setStatus("Finished")
		return iup.DEFAULT
	}))
	audio.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, message string) int {
		setStatus("Error: " + message)
		return iup.DEFAULT
	}))

	load := func(path string) {
		audio.SetAttribute("FILE", path)
		if audio.GetAttribute("FILE") == "" {
			return
		}
		iup.GetHandle("file").SetAttribute("TITLE", filepath.Base(path))
		iup.GetHandle("info").SetAttribute("TITLE", fmt.Sprintf("%s channels, %s Hz, %s", audio.GetAttribute("CHANNELS"), audio.GetAttribute("SAMPLERATE"), clock(audio.GetDouble("DURATION"))))
		setStatus(audio.GetAttribute("STATE"))
	}

	open := iup.Button("Open...")
	open.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "OPEN")
		filedlg.SetAttribute("TITLE", "Open Audio File")
		filedlg.SetAttribute("EXTFILTER", "Audio Files|*.wav;*.flac;*.mp3;*.ogg|All Files|*.*")
		iup.Popup(filedlg, iup.CENTER, iup.CENTER)
		if filedlg.GetInt("STATUS") != -1 {
			load(filedlg.GetAttribute("VALUE"))
		}
		filedlg.Destroy()
		return iup.DEFAULT
	}))

	action := func(title, attribute string) iup.Ihandle {
		return iup.Button(title).SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			audio.SetAttribute(attribute, "YES")
			setStatus(audio.GetAttribute("STATE"))
			return iup.DEFAULT
		}))
	}

	loop := iup.Toggle("Loop")
	loop.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		audio.SetAttribute("LOOP", state)
		return iup.DEFAULT
	}))

	volume := iup.Val("HORIZONTAL").SetAttributes(`MIN=0, MAX=100, VALUE=100, EXPAND=HORIZONTAL`)
	volume.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		audio.SetAttribute("VOLUME", ih.GetInt("VALUE"))
		return iup.DEFAULT
	}))

	pan := iup.Val("HORIZONTAL").SetAttributes(`MIN=-100, MAX=100, VALUE=0, EXPAND=HORIZONTAL`)
	pan.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		audio.SetAttribute("PAN", ih.GetInt("VALUE"))
		return iup.DEFAULT
	}))

	pitch := iup.Val("HORIZONTAL").SetAttributes(`MIN=0.5, MAX=2.0, VALUE=1.0, EXPAND=HORIZONTAL`)
	pitch.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		audio.SetAttribute("PITCH", ih.GetDouble("VALUE"))
		return iup.DEFAULT
	}))

	position := iup.Val("HORIZONTAL").SetAttributes(`MIN=0, MAX=1, VALUE=0, EXPAND=HORIZONTAL`)
	position.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		audio.SetAttribute("POSITION", ih.GetDouble("VALUE")*audio.GetDouble("DURATION"))
		return iup.DEFAULT
	}))

	timer := iup.Timer().SetAttributes("TIME=250, RUN=YES")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		duration := audio.GetDouble("DURATION")
		current := audio.GetDouble("POSITION")
		iup.GetHandle("time").SetAttribute("TITLE", clock(current)+" / "+clock(duration))
		if duration > 0 && audio.GetAttribute("STATE") == "PLAYING" {
			position.SetAttribute("VALUE", current/duration)
		}
		return iup.DEFAULT
	}))

	value := func(handle string) iup.Ihandle {
		label := iup.Label("-").SetAttributes(`EXPAND=HORIZONTAL`)
		label.SetHandle(handle)
		return label
	}

	status := iup.Label("Stopped").SetAttributes(`EXPAND=HORIZONTAL`)
	status.SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(open, action("Play", "PLAY"), action("Pause", "PAUSE"), action("Stop", "STOP"), loop).SetAttributes(`GAP=5, ALIGNMENT=ACENTER`),
			iup.GridBox(
				iup.Label("File"), value("file"),
				iup.Label("Format"), value("info"),
				iup.Label("Time"), value("time"),
				iup.Label("Position"), position,
				iup.Label("Volume"), volume,
				iup.Label("Pan"), pan,
				iup.Label("Pitch"), pitch,
			).SetAttributes(`NUMDIV=2, SIZELIN=-1, SIZECOL=-1, GAPLIN=5, GAPCOL=10, ALIGNMENTLIN=ACENTER`),
			status,
		).SetAttributes(`GAP=5, MARGIN=10x10`),
	).SetAttributes(`TITLE="Audio", SIZE=HALFx`)

	iup.Show(dlg)

	if len(os.Args) > 1 {
		load(os.Args[1])
		audio.SetAttribute("PLAY", "YES")
		setStatus(audio.GetAttribute("STATE"))
	}

	iup.MainLoop()

	timer.Destroy()
	audio.Destroy()
}

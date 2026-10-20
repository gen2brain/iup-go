//go:build media

package main

import (
	"fmt"
	"os"
	"path/filepath"
	"slices"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func clock(seconds float64) string {
	s := int(seconds + 0.5)
	return fmt.Sprintf("%d:%02d", s/60, s%60)
}

func main() {
	iup.Open()
	defer iup.Close()
	iup.MediaOpen()

	video := iup.Video().SetAttribute("CANFOCUS", "YES")
	value := func() iup.Ihandle {
		return iup.Label("-").SetAttributes("EXPAND=HORIZONTAL, ELLIPSIS=YES")
	}
	name, format, elapsed, status := value(), value(), value(), value()
	status.SetAttribute("TITLE", video.GetAttribute("STATE"))

	video.SetCallback("READY_CB", iup.ReadyFunc(func(ih iup.Ihandle) int {
		format.SetAttribute("TITLE", fmt.Sprintf("%s, %s, %s", ih.GetAttribute("BACKENDUSED"), ih.GetAttribute("VIDEOSIZE"), clock(ih.GetDouble("DURATION"))))
		return iup.DEFAULT
	}))
	video.SetCallback("STATECHANGED_CB", iup.StateChangedFunc(func(_ iup.Ihandle, state string) int {
		status.SetAttribute("TITLE", state)
		return iup.DEFAULT
	}))
	video.SetCallback("ERROR_CB", iup.ErrorFunc(func(_ iup.Ihandle, message string) int {
		status.SetAttribute("TITLE", "ERROR: "+message)
		return iup.DEFAULT
	}))

	source, sourceURL := "", false
	sourceVersion := 0
	load := func(text string, url bool) {
		source, sourceURL = text, url
		sourceVersion++
		if url {
			video.SetAttribute("URL", text)
			name.SetAttribute("TITLE", text)
		} else {
			video.SetAttribute("FILE", text)
			name.SetAttribute("TITLE", filepath.Base(text))
		}
		format.SetAttribute("TITLE", "-")
		video.SetAttribute("PLAY", "YES")
	}

	open := iup.Button("Open...").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		file := iup.FileDlg().SetAttributes(`DIALOGTYPE=OPEN, TITLE="Open Media"`)
		file.SetAttribute("EXTFILTER", "Media Files|*.mp4;*.m4v;*.mov;*.mkv;*.webm;*.avi;*.mp3;*.flac;*.ogg;*.wav;*.m4a|All Files|*.*|")
		iup.Popup(file, iup.CENTER, iup.CENTER)
		if file.GetInt("STATUS") != -1 {
			load(file.GetAttribute("VALUE"), false)
		}
		file.Destroy()
		return iup.DEFAULT
	}))
	action := func(title, attribute string) iup.Ihandle {
		return iup.Button(title).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			video.SetAttribute(attribute, "YES")
			return iup.DEFAULT
		}))
	}

	output := iup.List().SetAttributes("DROPDOWN=YES, EXPAND=HORIZONTAL, VISIBLECOLUMNS=10")
	updateOutputs := func() {
		output.SetAttribute("REMOVEITEM", "ALL")
		for index := 0; index < video.GetInt("AUDIODEVICECOUNT"); index++ {
			output.SetAttribute(strconv.Itoa(index+1), video.GetAttribute("AUDIODEVICENAME"+strconv.Itoa(index)))
		}
		output.SetAttribute("VALUE", 1)
		video.SetAttribute("AUDIODEVICE", 0)
	}
	output.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, index, selected int) int {
		if selected != 0 {
			video.SetAttribute("AUDIODEVICE", index-1)
		}
		return iup.DEFAULT
	}))
	adjustment := func(attribute string) iup.Ihandle {
		return iup.Val("HORIZONTAL").SetAttributes("MIN=-100, MAX=100, VALUE=0, EXPAND=HORIZONTAL").SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
			video.SetAttribute(attribute, ih.GetInt("VALUE"))
			return iup.DEFAULT
		}))
	}

	engines := []string{"AUTO"}
	for _, engine := range []string{"NATIVE", "VLC", "GSTREAMER", "MPV"} {
		video.SetAttribute("BACKEND", engine)
		if video.GetInt("AVAILABLE") != 0 {
			engines = append(engines, engine)
		}
	}
	video.SetAttribute("BACKEND", "AUTO")
	backend := iup.List().SetAttributes("DROPDOWN=YES, VALUE=1")
	for index, engine := range engines {
		backend.SetAttribute(strconv.Itoa(index+1), engine)
	}
	backend.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, text string, _ int, selected int) int {
		if selected != 0 {
			video.SetAttribute("FILE", nil)
			video.SetAttribute("BACKEND", text)
			updateOutputs()
			if source != "" {
				load(source, sourceURL)
			}
		}
		return iup.DEFAULT
	}))

	subtitleFiles := []string{}
	subtitles := iup.List().SetAttributes("DROPDOWN=YES, EXPAND=HORIZONTAL, VISIBLECOLUMNS=6")
	subtitleCount := 0
	subtitleSourceVersion := -1
	updateSubtitles := func(force bool) {
		count := video.GetInt("SUBTITLECOUNT")
		if !force && count == subtitleCount && sourceVersion == subtitleSourceVersion {
			return
		}
		subtitleCount = count
		subtitleSourceVersion = sourceVersion
		subtitles.SetAttribute("REMOVEITEM", "ALL")
		for index, label := range []string{"Auto", "Off"} {
			subtitles.SetAttribute(strconv.Itoa(index+1), label)
		}
		for index := 1; index <= count; index++ {
			label := video.GetAttribute("SUBTITLENAME" + strconv.Itoa(index))
			if label == "" {
				label = fmt.Sprintf("Track %d", index)
			}
			if language := video.GetAttribute("SUBTITLELANGUAGE" + strconv.Itoa(index)); language != "" {
				label += " (" + language + ")"
			}
			subtitles.SetAttribute(strconv.Itoa(index+2), label)
		}
		for index, path := range subtitleFiles {
			subtitles.SetAttribute(strconv.Itoa(count+index+3), filepath.Base(path))
		}
		selection := 1
		switch video.GetAttribute("SUBTITLE") {
		case "OFF":
			selection = 2
		case "EXTERNAL":
			for index, path := range subtitleFiles {
				if path == video.GetAttribute("SUBTITLEFILE") {
					selection = count + index + 3
				}
			}
		default:
			if index := video.GetInt("SUBTITLE"); index > 0 {
				selection = index + 2
			}
		}
		subtitles.SetAttribute("VALUE", selection)
	}
	subtitles.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, index, selected int) int {
		if selected == 0 {
			return iup.DEFAULT
		}
		switch {
		case index == 1:
			video.SetAttribute("SUBTITLE", "AUTO")
		case index == 2:
			video.SetAttribute("SUBTITLE", "OFF")
		case index <= subtitleCount+2:
			video.SetAttribute("SUBTITLE", index-2)
		case index-subtitleCount-3 < len(subtitleFiles):
			video.SetAttribute("SUBTITLEFILE", subtitleFiles[index-subtitleCount-3])
		}
		return iup.DEFAULT
	}))
	addSubtitleFile := func(path string) {
		video.SetAttribute("SUBTITLEFILE", path)
		if video.GetAttribute("SUBTITLEERROR") != "" {
			return
		}
		if !slices.Contains(subtitleFiles, path) {
			subtitleFiles = append(subtitleFiles, path)
		}
		updateSubtitles(true)
	}
	openSubtitles := iup.Button("Subtitles...").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		file := iup.FileDlg().SetAttributes(`DIALOGTYPE=OPEN, TITLE="Open Subtitles", EXTFILTER="Subtitles|*.srt;*.vtt|All Files|*.*|"`)
		iup.Popup(file, iup.CENTER, iup.CENTER)
		if file.GetInt("STATUS") != -1 {
			addSubtitleFile(file.GetAttribute("VALUE"))
		}
		file.Destroy()
		return iup.DEFAULT
	}))
	delay := iup.Text().SetAttributes("VISIBLECOLUMNS=6, VALUE=0")
	delay.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		video.SetAttribute("SUBTITLEDELAY", ih.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))
	updateSubtitles(true)

	loop := iup.Toggle("Loop").SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		video.SetAttribute("LOOP", state)
		return iup.DEFAULT
	}))
	mute := iup.Toggle("Mute").SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		video.SetAttribute("MUTE", state)
		return iup.DEFAULT
	}))

	url := iup.Text().SetAttribute("EXPAND", "HORIZONTAL")
	loadURL := func() {
		if text := url.GetAttribute("VALUE"); text != "" {
			load(text, true)
		}
	}
	url.SetCallback("K_ANY", iup.KAnyFunc(func(_ iup.Ihandle, key int) int {
		if key == iup.K_CR {
			loadURL()
			return iup.IGNORE
		}
		return iup.CONTINUE
	}))
	urlButton := iup.Button("Load URL").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		loadURL()
		return iup.DEFAULT
	}))

	seek, hold := -1.0, 0
	position := iup.Val("HORIZONTAL").SetAttributes("MIN=0, MAX=1, EXPAND=HORIZONTAL, ACTIVE=NO")
	position.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		seek = ih.GetDouble("VALUE") * video.GetDouble("DURATION")
		hold = 2
		return iup.DEFAULT
	}))
	volume := iup.Val("HORIZONTAL").SetAttributes("MIN=0, MAX=100, VALUE=100, EXPAND=HORIZONTAL")
	volume.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		video.SetAttribute("VOLUME", ih.GetInt("VALUE"))
		return iup.DEFAULT
	}))

	timer := iup.Timer().SetAttributes("TIME=250, RUN=YES")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(iup.Ihandle) int {
		updateSubtitles(false)
		if seek >= 0 {
			video.SetAttribute("POSITION", seek)
			seek = -1
		}
		current, duration := video.GetDouble("POSITION"), video.GetDouble("DURATION")
		seekable := video.GetInt("SEEKABLE") != 0 && duration > 0
		if position.GetBool("ACTIVE") != seekable {
			position.SetAttribute("ACTIVE", seekable)
		}
		elapsed.SetAttribute("TITLE", clock(current)+" / "+clock(duration))
		if hold > 0 {
			hold--
		} else if seekable {
			position.SetAttribute("VALUE", current/duration)
		}
		return iup.DEFAULT
	}))

	top := iup.Vbox(
		iup.Hbox(action("Play", "PLAY"), action("Pause", "PAUSE"), action("Stop", "STOP")).SetAttributes("GAP=5, ALIGNMENT=ACENTER"),
		iup.Hbox(loop, mute, iup.Label("Engine"), backend).SetAttributes("GAP=5, ALIGNMENT=ACENTER"),
		iup.Hbox(url, urlButton, open).SetAttributes("GAP=5, ALIGNMENT=ACENTER"),
		iup.Hbox(openSubtitles, subtitles, iup.Label("Delay (s)"), delay).SetAttributes("GAP=5, ALIGNMENT=ACENTER"),
	).SetAttribute("GAP", 5)
	bottom := iup.GridBox(
		iup.Label("Source"), name,
		iup.Label("Format"), format,
		iup.Label("Time"), elapsed,
		iup.Label("Position"), position,
		iup.Label("Volume"), volume,
		iup.Label("Output"), output,
		iup.Label("Brightness"), adjustment("BRIGHTNESS"),
		iup.Label("Contrast"), adjustment("CONTRAST"),
		iup.Label("Saturation"), adjustment("SATURATION"),
		iup.Label("Gamma"), adjustment("GAMMA"),
		iup.Label("State"), status,
	).SetAttributes("NUMDIV=2, SIZECOL=-1, SIZELIN=-1, GAPLIN=5, GAPCOL=10, ALIGNMENTLIN=ACENTER")
	layout := iup.Vbox(top, video, bottom).SetAttributes("MARGIN=10x10, GAP=5")
	dialog := iup.Dialog(layout).SetAttributes(`TITLE="Video", SHRINK=YES`)

	fullscreen := func(on bool) {
		for _, ih := range []iup.Ihandle{top, bottom} {
			ih.SetAttribute("FLOATING", on)
			ih.SetAttribute("VISIBLE", !on)
		}
		if on {
			layout.SetAttributes("MARGIN=0x0, GAP=0")
		} else {
			layout.SetAttributes("MARGIN=10x10, GAP=5")
		}
		dialog.SetAttribute("FULLSCREEN", on)
		iup.Refresh(dialog)
		if on {
			iup.SetFocus(video)
		}
	}
	video.SetCallback("BUTTON_CB", iup.ButtonFunc(func(_ iup.Ihandle, button, pressed, _, _ int, state string) int {
		if button == iup.BUTTON1 && pressed != 0 && iup.IsDouble(state) {
			fullscreen(dialog.GetAttribute("FULLSCREEN") != "YES")
		}
		return iup.DEFAULT
	}))
	dialog.SetCallback("K_ANY", iup.KAnyFunc(func(_ iup.Ihandle, key int) int {
		if key == iup.K_ESC && dialog.GetAttribute("FULLSCREEN") == "YES" {
			fullscreen(false)
			return iup.IGNORE
		}
		return iup.CONTINUE
	}))

	if len(os.Args) > 2 {
		for index, engine := range engines {
			if strings.EqualFold(os.Args[2], engine) {
				video.SetAttribute("BACKEND", engine)
				backend.SetAttribute("VALUE", index+1)
			}
		}
	}
	updateOutputs()
	if len(os.Args) > 1 {
		load(os.Args[1], strings.Contains(os.Args[1], "://"))
	}
	if len(os.Args) > 3 {
		addSubtitleFile(os.Args[3])
	}
	iup.Show(dialog)
	iup.MainLoop()
	if driver := iup.GetGlobal("DRIVER"); driver == "Android" || driver == "CocoaTouch" {
		return
	}
	timer.Destroy()
	dialog.Destroy()
}

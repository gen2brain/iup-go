//go:build media

package main

import (
	"fmt"
	"io"
	"math"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type take struct {
	id, frames, channels, sampleRate int
	name, path                       string
	started                          time.Time
	duration                         float64
	size                             int64
}

type studio struct {
	dialog, tabs, capturePage, takesPage iup.Ihandle
	camera, microphone, player, timer    iup.Ihandle
	cameraDevices, microphoneDevices     iup.Ihandle
	cameraRun, microphoneRun             iup.Ihandle
	recordButton, destinationText        iup.Ihandle
	waveform, level, recordingTime       iup.Ihandle
	table, playbackSeek, playbackTime    iup.Ihandle
	activity                             iup.Ihandle

	destination                    string
	cameraWanted, microphoneWanted bool
	recording, visible, closing    bool
	frameCount                     int
	pendingLog                     []string
	current                        take
	takes                          []take
	selectedTake, loadedTake       int
	nextTake, nextSnapshot         int
	waveSamples                    []int16
	seeking                        bool
	cameraState, microphoneState   string
}

func main() {
	iup.Open()
	iup.MediaOpen()
	iup.SetGlobal("UTF8MODE", "YES")
	iup.SetGlobal("APPID", "com.example.CaptureStudio")
	iup.SetGlobal("APPNAME", "Capture Studio")

	app := &studio{visible: true, selectedTake: -1, loadedTake: -1, nextTake: 1, nextSnapshot: 1}
	app.destination = defaultDestination()
	app.build()
	app.refreshCameraDevices()
	app.refreshMicrophoneDevices()

	iup.Show(app.dialog)
	for _, message := range app.pendingLog {
		app.activity.SetAttribute("APPEND", message)
	}
	app.pendingLog = nil
	if runtime.GOOS == "js" {
		app.log("Recording, snapshots and export need a filesystem and are disabled in the browser")
	}
	app.updatePermissionStatus()
	app.timer.SetAttribute("RUN", "YES")
	iup.MainLoop()

	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		return
	}
	app.stopAll()
	app.timer.Destroy()
	app.player.Destroy()
	app.microphone.Destroy()
	app.dialog.Destroy()
	iup.Close()
}

func (app *studio) build() {
	app.camera = iup.Camera().SetAttributes(`EXPAND=YES, BGCOLOR="24 27 31"`)
	app.microphone = iup.Microphone()
	app.player = iup.Audio()
	app.timer = iup.Timer().SetAttribute("TIME", 50)

	app.camera.SetCallback("FRAME_CB", iup.FrameFunc(func(_ iup.Ihandle, width, height int, _ []byte) int {
		app.frameCount++
		if app.frameCount == 1 {
			app.log(fmt.Sprintf("Camera streaming at %dx%d, %s fps", width, height, app.camera.GetAttribute("FPS")))
		}
		return iup.DEFAULT
	}))
	app.camera.SetCallback("PERMISSION_CB", iup.PermissionFunc(func(_ iup.Ihandle, granted int) int {
		if granted == 0 {
			app.cameraWanted = false
			app.camera.SetAttribute("RUN", "NO")
			app.cameraRun.SetAttribute("VALUE", "OFF")
		}
		app.populateCameraDevices()
		app.updatePermissionStatus()
		return iup.DEFAULT
	}))
	app.camera.SetCallback("ERROR_CB", iup.ErrorFunc(func(_ iup.Ihandle, message string) int {
		app.cameraWanted = false
		app.cameraRun.SetAttribute("VALUE", "OFF")
		app.log("Camera error: " + message)
		return iup.DEFAULT
	}))

	app.microphone.SetCallback("SAMPLES_CB", iup.SamplesFunc(func(ih iup.Ihandle, frames, channels int, samples []int16) int {
		if app.recording {
			app.current.frames += frames
			app.current.channels = channels
			app.current.sampleRate = ih.GetInt("SAMPLERATE")
		}
		app.appendWaveform(frames, channels, samples)
		return iup.DEFAULT
	}))
	app.microphone.SetCallback("PERMISSION_CB", iup.PermissionFunc(func(_ iup.Ihandle, granted int) int {
		if granted == 0 {
			app.microphoneWanted = false
			app.microphoneRun.SetAttribute("VALUE", "OFF")
			app.finishRecording(false)
		}
		app.populateMicrophoneDevices()
		app.updatePermissionStatus()
		return iup.DEFAULT
	}))
	app.microphone.SetCallback("ERROR_CB", iup.ErrorFunc(func(_ iup.Ihandle, message string) int {
		app.finishRecording(true)
		if !app.microphone.GetBool("RUN") {
			app.microphoneWanted = false
			app.microphoneRun.SetAttribute("VALUE", "OFF")
		}
		app.log("Microphone error: " + message)
		return iup.DEFAULT
	}))

	app.player.SetCallback("PLAYEND_CB", iup.PlayEndFunc(func(iup.Ihandle) int {
		app.log("Playback finished")
		return iup.DEFAULT
	}))
	app.player.SetCallback("ERROR_CB", iup.ErrorFunc(func(_ iup.Ihandle, message string) int {
		app.log("Playback error: " + message)
		return iup.DEFAULT
	}))

	app.capturePage = app.capturePanel().SetAttribute("TABTITLE", "Capture")
	app.takesPage = app.takesPanel().SetAttribute("TABTITLE", "Takes")
	app.tabs = iup.Tabs(app.capturePage, app.takesPage).SetAttribute("EXPAND", "YES")
	app.tabs.SetCallback("TABCHANGE_CB", iup.TabChangeFunc(func(_ iup.Ihandle, newTab, oldTab iup.Ihandle) int {
		app.leavePage(oldTab)
		app.enterPage(newTab)
		return iup.DEFAULT
	}))

	app.dialog = iup.Dialog(iup.Vbox(app.header(), app.tabs, app.footer()).SetAttributes("NGAP=0")).SetHandle("capture_dlg")
	app.dialog.SetAttributes(map[string]string{"TITLE": "Capture Studio", "PLACEMENT": "MAXIMIZED", "SHRINK": "YES"})
	app.dialog.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int {
		app.closing = true
		app.stopAll()
		return iup.CLOSE
	}))
	app.dialog.SetCallback("SHOW_CB", iup.ShowFunc(func(_ iup.Ihandle, state int) int {
		switch state {
		case iup.HIDE, iup.MINIMIZE:
			app.visible = false
			if app.recording {
				app.camera.SetAttribute("RUN", "NO")
				app.cameraRun.SetAttribute("VALUE", "OFF")
			} else {
				app.stopCapture()
			}
			app.timer.SetAttribute("RUN", "NO")
		case iup.SHOW, iup.RESTORE, iup.MAXIMIZE:
			app.visible = true
			app.timer.SetAttribute("RUN", "YES")
			if app.currentPage() == app.capturePage {
				app.resumeCapture()
			}
		}
		return iup.DEFAULT
	}))
	app.dialog.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(iup.Ihandle, int) int {
		iup.Update(app.waveform)
		return iup.DEFAULT
	}))

	app.timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(iup.Ihandle) int {
		app.updateTimer()
		return iup.DEFAULT
	}))
}

func (app *studio) header() iup.Ihandle {
	theme := button("Theme", func() {
		if iup.GetGlobalBool("DARKMODE") {
			iup.SetGlobal("APPEARANCE", "LIGHT")
		} else {
			iup.SetGlobal("APPEARANCE", "DARK")
		}
	})
	if phone() {
		return iup.Hbox(iup.Fill(), theme).SetAttributes("NMARGIN=8x3")
	}
	return iup.Hbox(
		iup.Vbox(
			iup.Label("Capture Studio").SetAttributes("FONTSTYLE=Bold, FONTSIZE=15"),
			iup.Label("Camera, microphone and recorded takes in one session").SetAttribute("FGCOLOR", "128 128 128"),
		).SetAttributes("NGAP=2, EXPAND=HORIZONTAL"),
		theme,
	).SetAttributes("NGAP=8, NMARGIN=9x7, ALIGNMENT=ACENTER")
}

func (app *studio) footer() iup.Ihandle {
	app.activity = iup.Text().SetAttributes("MULTILINE=YES, READONLY=YES, WORDWRAP=YES, APPENDNEWLINE=YES, APPENDSCROLL=YES, CANFOCUS=NO, EXPAND=HORIZONTAL, VISIBLELINES=2")
	return app.activity
}

func (app *studio) capturePanel() iup.Ihandle {
	cameraPanel := app.cameraPanel()
	microphonePanel := app.microphonePanel()
	if phone() {
		cameraPanel.SetAttribute("TABTITLE", "Camera")
		microphonePanel.SetAttribute("TABTITLE", "Microphone")
		tabs := iup.Tabs(cameraPanel, microphonePanel).SetAttribute("EXPAND", "YES")
		tabs.SetCallback("TABCHANGE_CB", iup.TabChangeFunc(func(_, current, _ iup.Ihandle) int {
			if current == cameraPanel {
				if app.cameraWanted {
					app.camera.SetAttribute("RUN", "YES")
					app.cameraRun.SetAttribute("VALUE", app.camera.GetAttribute("RUN"))
				}
			} else {
				app.camera.SetAttribute("RUN", "NO")
			}
			return iup.DEFAULT
		}))
		return tabs
	}
	return iup.Split(cameraPanel, microphonePanel).SetAttributes("ORIENTATION=VERTICAL, VALUE=610, MINMAX=420:760, SHOWGRIP=YES")
}

func (app *studio) cameraPanel() iup.Ihandle {
	app.cameraDevices = iup.List().SetAttributes("DROPDOWN=YES, VISIBLECOLUMNS=14, EXPAND=HORIZONTAL")
	app.cameraDevices.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, item, state int) int {
		if state == 1 && item > 0 {
			app.camera.SetAttribute("DEVICE", item-1)
			app.frameCount = 0
		}
		return iup.DEFAULT
	}))
	app.cameraRun = iup.Toggle("Camera on")
	app.cameraRun.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		app.cameraWanted = state == 1
		if app.cameraWanted && app.canCapture() {
			app.camera.SetAttribute("RUN", "YES")
		} else {
			app.camera.SetAttribute("RUN", "NO")
		}
		ih.SetAttribute("VALUE", app.camera.GetAttribute("RUN"))
		app.updatePermissionStatus()
		return iup.DEFAULT
	}))
	mirror := iup.Toggle("Mirror")
	mirror.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		app.camera.SetAttribute("MIRROR", state)
		return iup.DEFAULT
	}))
	var deviceRow iup.Ihandle
	if phone() {
		app.cameraDevices.SetAttribute("VISIBLECOLUMNS", 8)
		deviceRow = iup.Hbox(app.cameraDevices, button("Refresh", app.refreshCameraDevices)).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	} else {
		deviceRow = iup.Hbox(iup.Label("Camera"), app.cameraDevices, button("Refresh", app.refreshCameraDevices)).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	}
	actionRow := iup.Hbox(app.cameraRun, mirror, button("Snapshot", app.snapshot).SetAttribute("ACTIVE", yesNo(runtime.GOOS != "js")), iup.Fill()).SetAttributes("NGAP=7, ALIGNMENT=ACENTER")
	return iup.Vbox(deviceRow, actionRow, app.camera).SetAttributes("NGAP=6, NMARGIN=8x8")
}

func (app *studio) microphonePanel() iup.Ihandle {
	app.microphoneDevices = iup.List().SetAttributes("DROPDOWN=YES, VISIBLECOLUMNS=14, EXPAND=HORIZONTAL")
	app.microphoneDevices.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, item, state int) int {
		if state == 1 && item > 0 {
			if app.recording {
				app.finishRecording(true)
			}
			app.microphone.SetAttribute("DEVICE", item-1)
		}
		return iup.DEFAULT
	}))
	app.microphoneRun = iup.Toggle("Monitor")
	app.microphoneRun.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		app.microphoneWanted = state == 1
		if !app.microphoneWanted {
			app.finishRecording(true)
		}
		if app.microphoneWanted && app.canCapture() {
			app.microphone.SetAttribute("RUN", "YES")
		} else {
			app.microphone.SetAttribute("RUN", "NO")
		}
		ih.SetAttribute("VALUE", app.microphone.GetAttribute("RUN"))
		app.updatePermissionStatus()
		return iup.DEFAULT
	}))
	app.recordButton = button("Record", app.toggleRecording).SetAttribute("ACTIVE", yesNo(runtime.GOOS != "js"))
	app.destinationText = iup.Text().SetAttributes("VISIBLECOLUMNS=28, EXPAND=HORIZONTAL").SetAttribute("VALUE", app.destination)
	if phone() {
		app.destinationText.SetAttribute("READONLY", "YES")
	}
	app.level = iup.ProgressBar().SetAttributes("MIN=0, MAX=100, EXPAND=HORIZONTAL")
	app.recordingTime = iup.Label("0:00").SetHandle("capture_recording_time")
	app.waveform = iup.Canvas().SetAttributes("BORDER=NO, EXPAND=YES").SetHandle("capture_waveform")
	app.waveform.SetCallback("ACTION", iup.ActionFunc(app.drawWaveform))
	var deviceRow iup.Ihandle
	if phone() {
		app.microphoneDevices.SetAttribute("VISIBLECOLUMNS", 8)
		deviceRow = iup.Hbox(app.microphoneDevices, button("Refresh", app.refreshMicrophoneDevices)).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	} else {
		deviceRow = iup.Hbox(iup.Label("Microphone"), app.microphoneDevices, button("Refresh", app.refreshMicrophoneDevices)).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	}
	actionRow := iup.Hbox(app.microphoneRun, app.recordButton, iup.Fill()).SetAttributes("NGAP=7, ALIGNMENT=ACENTER")
	destinationRow := iup.Hbox(
		iup.Label("Destination"), app.destinationText,
		button("Browse", app.chooseDestination).SetAttribute("ACTIVE", yesNo(!phone() && runtime.GOOS != "js")),
	).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	levelRow := iup.Hbox(iup.Label("Level"), app.level, app.recordingTime).SetAttributes("NGAP=7, ALIGNMENT=ACENTER")
	return iup.Vbox(deviceRow, actionRow, destinationRow, levelRow, app.waveform).SetAttributes("NGAP=6, NMARGIN=8x8")
}

func (app *studio) takesPanel() iup.Ihandle {
	app.table = iup.Table().SetAttributes("NUMCOL=5, VISIBLECOLUMNS=4, VISIBLELINES=9, EXPAND=YES, ALTERNATECOLOR=YES, USERRESIZE=YES, ALLOWREORDER=YES, STRETCHLAST=YES, FOCUSRECT=NO")
	for col, title := range []string{"Take", "Recorded", "Duration", "Format", "File"} {
		iup.SetAttributeId(app.table, "TITLE", col+1, title)
	}
	app.table.SetAttributes("ALIGNMENT3=ARIGHT, ALIGNMENT4=ACENTER")
	app.table.SetCallback("ENTERITEM_CB", iup.EnterItemFunc(func(_ iup.Ihandle, lin, _ int) int {
		if lin >= 1 && lin <= len(app.takes) {
			app.selectedTake = lin - 1
			app.showSelectedTake()
		}
		return iup.DEFAULT
	}))
	app.table.SetCallback("CLICK_CB", iup.ClickFunc(func(_ iup.Ihandle, lin, _ int, status string) int {
		if iup.IsDouble(status) && lin >= 1 && lin <= len(app.takes) {
			app.selectedTake = lin - 1
			app.play()
		}
		return iup.DEFAULT
	}))

	app.playbackSeek = iup.Val("HORIZONTAL").SetAttributes("MIN=0, MAX=1, VALUE=0, EXPAND=HORIZONTAL")
	app.playbackSeek.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		if !app.seeking {
			app.player.SetAttribute("POSITION", ih.GetDouble("VALUE")*app.player.GetDouble("DURATION"))
		}
		return iup.DEFAULT
	}))
	app.playbackTime = iup.Label("0:00 / 0:00")
	buttons := iup.Hbox(
		button("Play", app.play), button("Pause", func() { app.player.SetAttribute("PAUSE", "YES") }),
		button("Stop", app.stopPlayback), button("Export", app.exportTake).SetAttribute("ACTIVE", yesNo(runtime.GOOS != "js")), button("Delete", app.deleteTake),
	).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	controls := iup.Hbox(buttons, iup.Fill()).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	position := iup.Hbox(iup.Label("Position"), app.playbackSeek, app.playbackTime).SetAttributes("NGAP=7, ALIGNMENT=ACENTER")
	return iup.Vbox(controls, app.table, position).SetAttributes("NGAP=7, NMARGIN=8x8")
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttributes("PADDING=5x3")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func (app *studio) log(message string) {
	line := fmt.Sprintf("[%s] %s", time.Now().Format("15:04:05"), message)
	if app.activity == 0 || app.activity.GetAttribute("WID") == "" {
		app.pendingLog = append(app.pendingLog, line)
		return
	}
	app.activity.SetAttribute("APPEND", line)
}

func phone() bool {
	return runtime.GOOS == "android" || runtime.GOOS == "ios"
}

func (app *studio) currentPage() iup.Ihandle {
	if app.tabs.GetInt("VALUEPOS") == 1 {
		return app.takesPage
	}
	return app.capturePage
}

func (app *studio) canCapture() bool {
	return !app.closing && app.visible && app.currentPage() == app.capturePage
}

func (app *studio) leavePage(page iup.Ihandle) {
	if page == app.capturePage {
		app.stopCapture()
	}
	if page == app.takesPage {
		app.stopPlayback()
	}
}

func (app *studio) enterPage(page iup.Ihandle) {
	if page == app.capturePage && app.visible {
		app.resumeCapture()
	}
}

func (app *studio) resumeCapture() {
	if app.cameraWanted {
		app.camera.SetAttribute("RUN", "YES")
		app.cameraRun.SetAttribute("VALUE", app.camera.GetAttribute("RUN"))
	}
	if app.microphoneWanted {
		app.microphone.SetAttribute("RUN", "YES")
		app.microphoneRun.SetAttribute("VALUE", app.microphone.GetAttribute("RUN"))
	}
	app.updatePermissionStatus()
}

func (app *studio) stopCapture() {
	app.frameCount = 0
	app.finishRecording(true)
	app.microphone.SetAttribute("RUN", "NO")
	app.camera.SetAttribute("RUN", "NO")
	app.cameraRun.SetAttribute("VALUE", "OFF")
	app.microphoneRun.SetAttribute("VALUE", "OFF")
	app.level.SetAttribute("VALUE", 0)
	app.updatePermissionStatus()
}

func (app *studio) stopAll() {
	app.timer.SetAttribute("RUN", "NO")
	app.finishRecording(true)
	app.microphone.SetAttribute("RUN", "NO")
	app.camera.SetAttribute("RUN", "NO")
	app.stopPlayback()
	app.player.SetAttribute("FILE", nil)
}

func (app *studio) toggleRecording() {
	if app.recording {
		app.finishRecording(true)
		return
	}
	app.startRecording()
}

func (app *studio) startRecording() {
	destination := strings.TrimSpace(app.destinationText.GetAttribute("VALUE"))
	if destination == "" {
		app.log("Choose a recording destination first")
		return
	}
	if err := os.MkdirAll(destination, 0o755); err != nil {
		app.log("Cannot use destination: " + err.Error())
		return
	}
	app.destination = destination
	started := time.Now()
	name := fmt.Sprintf("Take %02d", app.nextTake)
	path := filepath.Join(destination, fmt.Sprintf("capture-%s-%02d.wav", started.Format("20060102-150405"), app.nextTake))
	app.current = take{id: app.nextTake, name: name, path: path, started: started, sampleRate: app.microphone.GetInt("SAMPLERATE"), channels: app.microphone.GetInt("CHANNELS")}
	app.waveSamples = nil
	app.recording = true
	app.microphoneWanted = true
	app.microphoneRun.SetAttribute("VALUE", "ON")
	app.microphone.SetAttribute("FILE", path)
	app.microphone.SetAttribute("RUN", "YES")
	if !app.microphone.GetBool("RUN") {
		app.finishRecording(false)
		app.log("Microphone could not start")
		return
	}
	app.nextTake++
	app.recordButton.SetAttribute("TITLE", "Stop recording")
	app.destinationText.SetAttribute("ACTIVE", "NO")
	app.log("Recording " + filepath.Base(path))
}

func (app *studio) finishRecording(keep bool) {
	if !app.recording {
		return
	}
	app.microphone.SetAttribute("FILE", nil)
	app.recording = false
	app.recordButton.SetAttribute("TITLE", "Record")
	app.destinationText.SetAttribute("ACTIVE", "YES")
	if app.current.sampleRate > 0 {
		app.current.duration = float64(app.current.frames) / float64(app.current.sampleRate)
	}
	if info, err := os.Stat(app.current.path); err == nil {
		app.current.size = info.Size()
	}
	if keep && app.current.frames > 0 && app.current.size > 44 {
		app.takes = append(app.takes, app.current)
		app.selectedTake = len(app.takes) - 1
		app.refreshTakes()
		app.log(fmt.Sprintf("Saved %s, %s", app.current.name, clock(app.current.duration)))
	} else {
		os.Remove(app.current.path)
		if keep {
			app.log("No audio samples were recorded")
		}
	}
	app.current = take{}
	app.recordingTime.SetAttribute("TITLE", "0:00")
}

func (app *studio) appendWaveform(frames, channels int, samples []int16) {
	if frames <= 0 || channels <= 0 {
		return
	}
	step := max(1, frames/240)
	for frame := 0; frame < frames; frame += step {
		var sum int
		for channel := 0; channel < channels; channel++ {
			sum += int(samples[frame*channels+channel])
		}
		app.waveSamples = append(app.waveSamples, int16(sum/channels))
	}
	const capacity = 1200
	if len(app.waveSamples) > capacity {
		copy(app.waveSamples, app.waveSamples[len(app.waveSamples)-capacity:])
		app.waveSamples = app.waveSamples[:capacity]
	}
}

func (app *studio) drawWaveform(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, h := iup.DrawGetSize(ih)
	ih.SetAttribute("DRAWCOLOR", global("TXTBGCOLOR", "245 246 248"))
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)
	center := h / 2
	ih.SetAttribute("DRAWCOLOR", global("DLGBGCOLOR", "210 212 216"))
	iup.DrawLine(ih, 0, center, w-1, center)
	if len(app.waveSamples) > 1 && w > 1 {
		ih.SetAttribute("DRAWCOLOR", global("ACCENTCOLOR", "47 119 218"))
		previousX, previousY := 0, center
		for x := 0; x < w; x++ {
			index := x * (len(app.waveSamples) - 1) / max(1, w-1)
			y := center - int(app.waveSamples[index])*max(1, h-8)/(2*32768)
			if x > 0 {
				iup.DrawLine(ih, previousX, previousY, x, y)
			}
			previousX, previousY = x, y
		}
	}
	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func (app *studio) snapshot() {
	if app.frameCount == 0 {
		app.log("Start the camera and wait for a frame before taking a snapshot")
		return
	}
	if err := os.MkdirAll(app.destination, 0o755); err != nil {
		app.log("Cannot use destination: " + err.Error())
		return
	}
	path := filepath.Join(app.destination, fmt.Sprintf("snapshot-%s-%02d.png", time.Now().Format("20060102-150405"), app.nextSnapshot))
	app.nextSnapshot++
	app.camera.SetAttribute("SNAPSHOT", path)
	if info, err := os.Stat(path); err == nil && info.Size() > 0 {
		app.log("Saved " + filepath.Base(path))
	} else {
		app.log("The snapshot could not be saved")
	}
}

func (app *studio) refreshCameraDevices() {
	if app.cameraDevices == 0 {
		return
	}
	wasRunning := app.camera.GetBool("RUN")
	app.camera.SetAttribute("RUN", "NO")
	app.populateCameraDevices()
	count := app.camera.GetInt("DEVICECOUNT")
	if count > 0 {
		device := min(app.camera.GetInt("DEVICE"), count-1)
		app.camera.SetAttribute("DEVICE", device)
	}
	if wasRunning && app.canCapture() {
		app.camera.SetAttribute("RUN", "YES")
	}
	app.updatePermissionStatus()
}

func (app *studio) populateCameraDevices() {
	app.cameraDevices.SetAttribute("REMOVEITEM", "ALL")
	count := app.camera.GetInt("DEVICECOUNT")
	for index := 0; index < count; index++ {
		name := app.camera.GetAttribute("DEVICENAME", index)
		if name == "" {
			name = fmt.Sprintf("Camera %d", index+1)
		}
		app.cameraDevices.SetAttributeId("", index+1, name)
	}
	if count == 0 {
		app.cameraDevices.SetAttribute("1", "No camera found")
		app.cameraDevices.SetAttributes("VALUE=1, ACTIVE=NO")
	} else {
		device := min(app.camera.GetInt("DEVICE"), count-1)
		app.cameraDevices.SetAttribute("ACTIVE", "YES")
		app.cameraDevices.SetAttribute("VALUE", device+1)
	}
}

func (app *studio) refreshMicrophoneDevices() {
	if app.microphoneDevices == 0 {
		return
	}
	wasRunning := app.microphone.GetBool("RUN")
	if app.recording {
		app.finishRecording(true)
	}
	app.microphone.SetAttribute("RUN", "NO")
	app.populateMicrophoneDevices()
	count := app.microphone.GetInt("DEVICECOUNT")
	if count > 0 {
		device := min(app.microphone.GetInt("DEVICE"), count-1)
		app.microphone.SetAttribute("DEVICE", device)
	}
	if wasRunning && app.canCapture() {
		app.microphone.SetAttribute("RUN", "YES")
	}
	app.updatePermissionStatus()
}

func (app *studio) populateMicrophoneDevices() {
	app.microphoneDevices.SetAttribute("REMOVEITEM", "ALL")
	count := app.microphone.GetInt("DEVICECOUNT")
	for index := 0; index < count; index++ {
		name := app.microphone.GetAttribute("DEVICENAME", index)
		if name == "" {
			name = fmt.Sprintf("Microphone %d", index+1)
		}
		app.microphoneDevices.SetAttributeId("", index+1, name)
	}
	if count == 0 {
		app.microphoneDevices.SetAttribute("1", "No microphone found")
		app.microphoneDevices.SetAttributes("VALUE=1, ACTIVE=NO")
	} else {
		device := min(app.microphone.GetInt("DEVICE"), count-1)
		app.microphoneDevices.SetAttribute("ACTIVE", "YES")
		app.microphoneDevices.SetAttribute("VALUE", device+1)
	}
}

func (app *studio) updatePermissionStatus() {
	cameraState := "stopped"
	if app.camera.GetBool("RUN") {
		cameraState = "running"
	}
	cameraState = fmt.Sprintf("Camera %s, permission %s", cameraState, strings.ToLower(app.camera.GetAttribute("PERMISSION")))
	if cameraState != app.cameraState {
		app.cameraState = cameraState
		app.log(cameraState)
	}
	if app.recording {
		return
	}
	microphoneState := "stopped"
	if app.microphone.GetBool("RUN") {
		microphoneState = "running"
	}
	microphoneState = fmt.Sprintf("Microphone %s, permission %s", microphoneState, strings.ToLower(app.microphone.GetAttribute("PERMISSION")))
	if microphoneState != app.microphoneState {
		app.microphoneState = microphoneState
		app.log(microphoneState)
	}
}

func (app *studio) chooseDestination() {
	if app.recording {
		return
	}
	dialog := iup.FileDlg().SetAttributes(map[string]string{"DIALOGTYPE": "DIR", "TITLE": "Recording Destination", "DIRECTORY": app.destination})
	iup.SetAttributeHandle(dialog, "PARENTDIALOG", app.dialog)
	iup.Popup(dialog, iup.CENTERPARENT, iup.CENTERPARENT)
	if dialog.GetInt("STATUS") != -1 {
		value := dialog.GetAttribute("VALUE")
		if value != "" {
			app.destination = value
			app.destinationText.SetAttribute("VALUE", value)
			app.log("Destination set to " + value)
		}
	}
	dialog.Destroy()
}

func (app *studio) refreshTakes() {
	app.table.SetAttribute("NUMLIN", len(app.takes))
	for index, item := range app.takes {
		lin := index + 1
		values := []string{
			item.name,
			item.started.Format("Jan 02  15:04:05"),
			clock(item.duration),
			fmt.Sprintf("%d ch, %d Hz", item.channels, item.sampleRate),
			filepath.Base(item.path),
		}
		for col, value := range values {
			iup.SetAttributeId2(app.table, "", lin, col+1, value)
		}
	}
	if app.selectedTake >= 0 && app.selectedTake < len(app.takes) {
		app.table.SetAttribute("FOCUSCELL", fmt.Sprintf("%d:1", app.selectedTake+1))
		app.table.SetAttributeId("SELECTED", app.selectedTake+1, "YES")
		app.showSelectedTake()
	}
}

func (app *studio) showSelectedTake() {
	if app.selectedTake < 0 || app.selectedTake >= len(app.takes) {
		return
	}
	item := app.takes[app.selectedTake]
	app.log(fmt.Sprintf("Selected %s, %s, %s", item.name, clock(item.duration), fileSize(item.size)))
}

func (app *studio) loadSelectedTake() bool {
	if app.selectedTake < 0 || app.selectedTake >= len(app.takes) {
		app.log("Select a recorded take first")
		return false
	}
	if app.loadedTake == app.takes[app.selectedTake].id && app.player.GetAttribute("FILE") != "" {
		return true
	}
	app.player.SetAttribute("STOP", "YES")
	app.player.SetAttribute("FILE", nil)
	app.player.SetAttribute("FILE", app.takes[app.selectedTake].path)
	if app.player.GetAttribute("FILE") == "" {
		app.loadedTake = -1
		return false
	}
	app.loadedTake = app.takes[app.selectedTake].id
	app.playbackSeek.SetAttribute("VALUE", 0)
	return true
}

func (app *studio) play() {
	if app.loadSelectedTake() {
		app.player.SetAttribute("PLAY", "YES")
		app.log("Playing " + app.takes[app.selectedTake].name)
	}
}

func (app *studio) stopPlayback() {
	app.player.SetAttribute("STOP", "YES")
	app.seeking = true
	app.playbackSeek.SetAttribute("VALUE", 0)
	app.seeking = false
}

func (app *studio) exportTake() {
	if app.selectedTake < 0 || app.selectedTake >= len(app.takes) {
		app.log("Select a recorded take first")
		return
	}
	item := app.takes[app.selectedTake]
	dialog := iup.FileDlg().SetAttributes(map[string]string{
		"DIALOGTYPE": "SAVE", "TITLE": "Export Recorded Take", "EXTFILTER": "WAV Files|*.wav|All Files|*.*", "EXTDEFAULT": "wav", "FILE": filepath.Base(item.path),
	})
	iup.SetAttributeHandle(dialog, "PARENTDIALOG", app.dialog)
	iup.Popup(dialog, iup.CENTERPARENT, iup.CENTERPARENT)
	if dialog.GetInt("STATUS") != -1 {
		destination := dialog.GetAttribute("VALUE")
		if same(item.path, destination) {
			app.log("Choose a different file than the recorded take")
		} else if err := copyFile(item.path, destination); err != nil {
			app.log("Export failed: " + err.Error())
		} else {
			app.log("Exported " + filepath.Base(destination))
		}
	}
	dialog.Destroy()
}

func (app *studio) deleteTake() {
	if app.selectedTake < 0 || app.selectedTake >= len(app.takes) {
		return
	}
	item := app.takes[app.selectedTake]
	if app.loadedTake == item.id {
		app.player.SetAttribute("STOP", "YES")
		app.player.SetAttribute("FILE", nil)
		app.loadedTake = -1
	}
	if err := os.Remove(item.path); err != nil && !os.IsNotExist(err) {
		app.log("Delete failed: " + err.Error())
		return
	}
	app.log("Deleted " + item.name)
	app.takes = append(app.takes[:app.selectedTake], app.takes[app.selectedTake+1:]...)
	if app.selectedTake >= len(app.takes) {
		app.selectedTake = len(app.takes) - 1
	}
	app.refreshTakes()
}

func (app *studio) updateTimer() {
	if app.closing || !app.visible {
		return
	}
	if app.currentPage() == app.capturePage {
		app.level.SetAttribute("VALUE", app.microphone.GetInt("LEVEL"))
		iup.Update(app.waveform)
		if app.recording && app.current.sampleRate > 0 {
			app.recordingTime.SetAttribute("TITLE", clock(float64(app.current.frames)/float64(app.current.sampleRate)))
		}
		if app.frameCount > 0 && app.frameCount%30 == 0 {
			app.updatePermissionStatus()
		}
		return
	}
	duration := app.player.GetDouble("DURATION")
	position := app.player.GetDouble("POSITION")
	app.playbackTime.SetAttribute("TITLE", clock(position)+" / "+clock(duration))
	if duration > 0 && app.player.GetAttribute("STATE") == "PLAYING" {
		app.seeking = true
		app.playbackSeek.SetAttribute("VALUE", position/duration)
		app.seeking = false
	}
}

func defaultDestination() string {
	base := iup.GetGlobal("DATADIR")
	if base == "" {
		base = iup.GetGlobal("TMPDIR")
	}
	if base == "" {
		base = os.TempDir()
	}
	destination := filepath.Join(base, "Capture Studio")
	if err := os.MkdirAll(destination, 0o755); err != nil {
		destination = filepath.Join(os.TempDir(), "Capture Studio")
		os.MkdirAll(destination, 0o755)
	}
	return destination
}

func same(a, b string) bool {
	ia, errA := os.Stat(a)
	ib, errB := os.Stat(b)
	return errA == nil && errB == nil && os.SameFile(ia, ib)
}

func copyFile(source, destination string) error {
	in, err := os.Open(source)
	if err != nil {
		return err
	}
	defer in.Close()
	out, err := os.Create(destination)
	if err != nil {
		return err
	}
	_, copyErr := io.Copy(out, in)
	closeErr := out.Close()
	if copyErr != nil {
		return copyErr
	}
	return closeErr
}

func clock(seconds float64) string {
	value := int(math.Max(0, seconds) + 0.5)
	return fmt.Sprintf("%d:%02d", value/60, value%60)
}

func fileSize(size int64) string {
	if size >= 1<<20 {
		return fmt.Sprintf("%.1f MB", float64(size)/(1<<20))
	}
	return fmt.Sprintf("%.0f KB", float64(size)/(1<<10))
}

func global(name, fallback string) string {
	if value := iup.GetGlobal(name); value != "" {
		return value
	}
	return fallback
}

func yesNo(value bool) string {
	if value {
		return "YES"
	}
	return "NO"
}

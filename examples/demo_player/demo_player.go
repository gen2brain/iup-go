//go:build media

package main

import (
	"fmt"
	"image"
	"image/color"
	_ "image/jpeg"
	_ "image/png"
	"os"
	"path/filepath"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func clock(seconds float64) string {
	s := int(seconds + 0.5)
	return fmt.Sprintf("%d:%02d", s/60, s%60)
}

var coverNames = []string{"cover", "folder", "front", "album", "albumart"}
var coverExts = []string{".jpg", ".jpeg", ".png"}

func findCover(audioPath string) string {
	entries, err := os.ReadDir(filepath.Dir(audioPath))
	if err != nil {
		return ""
	}
	byName := make(map[string]string, len(entries))
	for _, e := range entries {
		if !e.IsDir() {
			byName[strings.ToLower(e.Name())] = filepath.Join(filepath.Dir(audioPath), e.Name())
		}
	}
	base := strings.ToLower(strings.TrimSuffix(filepath.Base(audioPath), filepath.Ext(audioPath)))
	for _, ext := range coverExts {
		if p, ok := byName[base+ext]; ok {
			return p
		}
	}
	for _, name := range coverNames {
		for _, ext := range coverExts {
			if p, ok := byName[name+ext]; ok {
				return p
			}
		}
	}
	return ""
}

func thumbnail(src image.Image, max int) image.Image {
	b := src.Bounds()
	w, h := b.Dx(), b.Dy()
	if w < 1 || h < 1 || (w <= max && h <= max) {
		return src
	}
	nw, nh := max, h*max/w
	if h > w {
		nw, nh = w*max/h, max
	}
	dst := image.NewRGBA(image.Rect(0, 0, nw, nh))
	for y := 0; y < nh; y++ {
		sy := b.Min.Y + y*h/nh
		for x := 0; x < nw; x++ {
			dst.Set(x, y, src.At(b.Min.X+x*w/nw, sy))
		}
	}
	return dst
}

func placeholder(size int) image.Image {
	img := image.NewRGBA(image.Rect(0, 0, size, size))
	fill := color.RGBA{0xd8, 0xd8, 0xd8, 0xff}
	edge := color.RGBA{0xa0, 0xa0, 0xa0, 0xff}
	for y := 0; y < size; y++ {
		for x := 0; x < size; x++ {
			c := fill
			if x == 0 || y == 0 || x == size-1 || y == size-1 {
				c = edge
			}
			img.SetRGBA(x, y, c)
		}
	}
	return img
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.MediaOpen()

	audio := iup.Audio()

	var paths []string
	var coverImg iup.Ihandle
	var curDur float64
	current := -1

	placeholderImg := iup.ImageFromImage(placeholder(200))
	cover := iup.Label("")
	iup.SetAttributeHandle(cover, "IMAGE", placeholderImg)
	list := iup.List().SetAttributes(`EXPAND=YES, VISIBLELINES=8`)
	nowPlaying := iup.Label("No file loaded").SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER, FONT="Sans, Bold 13"`)
	format := iup.Label("").SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER`)
	elapsed := iup.Label("0:00")
	duration := iup.Label("0:00")
	status := iup.Label("Stopped").SetAttributes(`EXPAND=HORIZONTAL`)

	seek := iup.Val("HORIZONTAL").SetAttributes(`MIN=0, MAX=1, VALUE=0, EXPAND=HORIZONTAL`)
	volume := iup.Val("HORIZONTAL").SetAttributes(`MIN=0, MAX=100, VALUE=100, EXPAND=HORIZONTAL`)

	openBtn := iup.Button("Open")
	removeBtn := iup.Button("Del")
	clearBtn := iup.Button("Clr")
	prevBtn := iup.Button("⏮").SetAttributes(`PADDING=8x5`)
	playBtn := iup.Button("▶").SetAttributes(`PADDING=14x5`)
	stopBtn := iup.Button("⏹").SetAttributes(`PADDING=8x5`)
	nextBtn := iup.Button("⏭").SetAttributes(`PADDING=8x5`)
	repeat := iup.Toggle("Repeat")
	timer := iup.Timer().SetAttributes("TIME=250, RUN=YES")

	clearCover := func() {
		iup.SetAttributeHandle(cover, "IMAGE", placeholderImg)
		if coverImg != 0 {
			coverImg.Destroy()
			coverImg = 0
		}
	}

	setCover := func(audioPath string) {
		clearCover()
		path := findCover(audioPath)
		if path == "" {
			return
		}
		f, err := os.Open(path)
		if err != nil {
			return
		}
		img, _, err := image.Decode(f)
		f.Close()
		if err != nil {
			return
		}
		coverImg = iup.ImageFromImage(thumbnail(img, 200))
		iup.SetAttributeHandle(cover, "IMAGE", coverImg)
	}

	showTrack := func(i int) bool {
		if i < 0 || i >= len(paths) {
			return false
		}
		audio.SetAttribute("FILE", paths[i])
		if audio.GetAttribute("FILE") == "" {
			return false
		}
		current = i
		list.SetAttribute("VALUE", i+1)
		nowPlaying.SetAttribute("TITLE", filepath.Base(paths[i]))
		format.SetAttribute("TITLE", fmt.Sprintf("%s ch, %s Hz", audio.GetAttribute("CHANNELS"), audio.GetAttribute("SAMPLERATE")))
		seek.SetAttribute("VALUE", 0)
		curDur = audio.GetDouble("DURATION")
		setCover(paths[i])
		return true
	}

	play := func(i int) {
		if showTrack(i) {
			audio.SetAttribute("PLAY", "YES")
		}
	}

	playPause := func() {
		switch audio.GetAttribute("STATE") {
		case "PLAYING":
			audio.SetAttribute("PAUSE", "YES")
		case "PAUSED":
			audio.SetAttribute("PLAY", "YES")
		default:
			if current >= 0 {
				audio.SetAttribute("PLAY", "YES")
			} else if sel := list.GetInt("VALUE"); sel > 0 {
				play(sel - 1)
			} else if len(paths) > 0 {
				play(0)
			}
		}
	}

	next := func() {
		if current+1 < len(paths) {
			play(current + 1)
		} else {
			audio.SetAttribute("STOP", "YES")
		}
	}

	prev := func() {
		if current > 0 {
			play(current - 1)
		}
	}

	addFiles := func(more ...string) {
		wasEmpty := len(paths) == 0
		for _, p := range more {
			paths = append(paths, p)
			list.SetAttribute("APPENDITEM", filepath.Base(p))
		}
		if wasEmpty && len(paths) > 0 {
			list.SetAttribute("VALUE", 1)
		}
	}

	openBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttributes(map[string]string{
			"DIALOGTYPE":    "OPEN",
			"TITLE":         "Open Audio Files",
			"MULTIPLEFILES": "YES",
			"EXTFILTER":     "Audio Files|*.wav;*.flac;*.mp3;*.ogg|All Files|*.*",
		})
		iup.Popup(filedlg, iup.CENTER, iup.CENTER)
		if filedlg.GetInt("STATUS") != -1 {
			value := filedlg.GetAttribute("VALUE")
			if strings.Contains(value, "|") {
				parts := strings.Split(value, "|")
				for _, name := range parts[1:] {
					if name != "" {
						addFiles(filepath.Join(parts[0], name))
					}
				}
			} else if value != "" {
				addFiles(value)
			}
		}
		filedlg.Destroy()
		return iup.DEFAULT
	}))

	removeBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		sel := list.GetInt("VALUE")
		if sel <= 0 {
			return iup.DEFAULT
		}
		i := sel - 1
		list.SetAttribute("REMOVEITEM", sel)
		paths = append(paths[:i], paths[i+1:]...)
		switch {
		case i == current:
			audio.SetAttribute("STOP", "YES")
			audio.SetAttribute("FILE", "")
			current = -1
			curDur = 0
			nowPlaying.SetAttribute("TITLE", "No file loaded")
			format.SetAttribute("TITLE", "")
			clearCover()
		case i < current:
			current--
		}
		return iup.DEFAULT
	}))

	clearBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		audio.SetAttribute("STOP", "YES")
		audio.SetAttribute("FILE", "")
		list.SetAttribute("REMOVEITEM", "ALL")
		paths = nil
		current = -1
		curDur = 0
		nowPlaying.SetAttribute("TITLE", "No file loaded")
		format.SetAttribute("TITLE", "")
		clearCover()
		return iup.DEFAULT
	}))

	playBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		playPause()
		return iup.DEFAULT
	}))
	stopBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		audio.SetAttribute("STOP", "YES")
		return iup.DEFAULT
	}))
	prevBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		prev()
		return iup.DEFAULT
	}))
	nextBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		next()
		return iup.DEFAULT
	}))

	list.SetCallback("DBLCLICK_CB", iup.DblclickFunc(func(ih iup.Ihandle, item int, text string) int {
		play(item - 1)
		return iup.DEFAULT
	}))

	repeat.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		audio.SetAttribute("LOOP", state)
		return iup.DEFAULT
	}))

	seek.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		if curDur > 0 {
			audio.SetAttribute("POSITION", ih.GetDouble("VALUE")*curDur)
		}
		return iup.DEFAULT
	}))
	volume.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		audio.SetAttribute("VOLUME", ih.GetInt("VALUE"))
		return iup.DEFAULT
	}))

	audio.SetCallback("PLAYEND_CB", iup.PlayEndFunc(func(ih iup.Ihandle) int {
		next()
		return iup.DEFAULT
	}))
	audio.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, message string) int {
		status.SetAttribute("TITLE", "Error: "+message)
		return iup.DEFAULT
	}))

	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		dur := curDur
		pos := audio.GetDouble("POSITION")
		elapsed.SetAttribute("TITLE", clock(pos))
		duration.SetAttribute("TITLE", clock(dur))
		state := audio.GetAttribute("STATE")
		status.SetAttribute("TITLE", map[string]string{"PLAYING": "Playing", "PAUSED": "Paused", "STOPPED": "Stopped"}[state])
		if state == "PLAYING" {
			playBtn.SetAttribute("TITLE", "⏸")
		} else {
			playBtn.SetAttribute("TITLE", "▶")
		}
		if dur > 0 && state == "PLAYING" {
			seek.SetAttribute("VALUE", pos/dur)
		}
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(iup.Fill(), cover, iup.Fill()),
			nowPlaying,
			format,
			iup.Hbox(elapsed, seek, duration).SetAttributes(`GAP=8, ALIGNMENT=ACENTER`),
			iup.Hbox(
				iup.Fill(),
				prevBtn, playBtn, stopBtn, nextBtn, repeat,
				iup.Fill(),
			).SetAttributes(`GAP=6, ALIGNMENT=ACENTER`),
			iup.Hbox(iup.Label("Volume"), volume).SetAttributes(`GAP=8, ALIGNMENT=ACENTER`),
			iup.Hbox(
				iup.Label("Playlist").SetAttributes(`FONT="Sans, Bold 9"`),
				iup.Fill(),
				openBtn, removeBtn, clearBtn,
			).SetAttributes(`GAP=5, ALIGNMENT=ACENTER`),
			list,
			status,
		).SetAttributes(`GAP=8, MARGIN=14x14`),
	).SetAttributes(`TITLE="Media Player"`)

	dlg.SetCallback("K_ANY", iup.KAnyFunc(func(ih iup.Ihandle, c int) int {
		if c == iup.K_SP {
			playPause()
			return iup.IGNORE
		}
		return iup.CONTINUE
	}))
	dlg.SetCallback("DROPFILES_CB", iup.DropFilesFunc(func(ih iup.Ihandle, path string, num, x, y int) int {
		addFiles(path)
		return iup.DEFAULT
	}))

	iup.Show(dlg)

	if len(os.Args) > 1 {
		addFiles(os.Args[1:]...)
		play(0)
	}

	iup.MainLoop()

	timer.Destroy()
	audio.Destroy()
}

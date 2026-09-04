package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var status iup.Ihandle

func report(msg string) {
	fmt.Println(msg)
	status.SetAttribute("TITLE", msg)
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetHandle("tip_icon", makeImage(60, 140, 220))

	status = iup.Label("Hover the controls; TIPS_CB reports here").SetAttribute("EXPAND", "HORIZONTAL")

	plain := iup.Label("Plain TIP").SetAttribute("TIP", "A plain tip")

	styled := iup.Button("TIPBGCOLOR, TIPFGCOLOR, TIPFONT and TIPDELAY")
	styled.SetAttributes(`TIP="Dark tip in a bold font, shown after one second", TIPBGCOLOR="40 40 60", TIPFGCOLOR="255 230 120", TIPFONT="Sans, Bold 12", TIPDELAY=1000`)

	markup := iup.Text()
	markup.SetAttributes(`VALUE="TIPMARKUP and TIPICON", EXPAND=HORIZONTAL, TIPMARKUP=YES, TIP="<b>Bold</b> and <i>italic</i> Pango markup with an icon", TIPICON=tip_icon`)

	balloon := iup.Button("TIPBALLOON with a title and an icon")
	balloon.SetAttributes(`TIP="Balloon body text", TIPBALLOON=YES, TIPBALLOONTITLE="Balloon title", TIPBALLOONTITLEICON=1`)

	half := iup.Label("TIPRECT: only the left half of this label has a tip")
	half.SetAttributes(`BGCOLOR="220 235 250", PADDING=10x10, TIP="You are over the left half"`)

	hideTimer := iup.Timer().SetAttribute("TIME", "2000")
	show := iup.Button("TIPVISIBLE=YES shows this button's tip for two seconds")
	show.SetAttribute("TIP", "Forced with TIPVISIBLE=YES, hidden with TIPVISIBLE=NO")
	show.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		show.SetAttribute("TIPVISIBLE", "YES")
		hideTimer.SetAttribute("RUN", "YES")
		return iup.DEFAULT
	}))
	hideTimer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		ih.SetAttribute("RUN", "NO")
		visible := show.GetAttribute("TIPVISIBLE")
		show.SetAttribute("TIPVISIBLE", "NO")
		report("TIPVISIBLE was " + visible + ", now " + show.GetAttribute("TIPVISIBLE"))
		return iup.DEFAULT
	}))

	for _, ih := range []iup.Ihandle{plain, styled, markup, balloon, half, show} {
		ih.SetCallback("TIPS_CB", iup.TipsFunc(func(ih iup.Ihandle, x, y int) int {
			report(fmt.Sprintf("TIPS_CB at %d,%d: %s", x, y, ih.GetAttribute("TIP")))
			return iup.DEFAULT
		}))
	}

	dlg := iup.Dialog(iup.Vbox(
		plain,
		styled,
		markup,
		balloon,
		half,
		show,
		status,
	).SetAttributes("NMARGIN=10x10, NGAP=8"))
	dlg.SetAttribute("TITLE", "Tips")

	iup.Map(dlg)
	var w, h int
	fmt.Sscanf(half.GetAttribute("RASTERSIZE"), "%dx%d", &w, &h)
	half.SetAttribute("TIPRECT", fmt.Sprintf("0 0 %d %d", w/2, h))

	iup.Show(dlg)
	iup.MainLoop()

	iup.Destroy(hideTimer)
}

func makeImage(r, g, b byte) iup.Ihandle {
	const n = 16
	pixels := make([]byte, n*n*4)
	for y := 0; y < n; y++ {
		for x := 0; x < n; x++ {
			i := (y*n + x) * 4
			dx, dy := float64(x-8)+0.5, float64(y-8)+0.5
			if dx*dx+dy*dy <= 49 {
				pixels[i+0], pixels[i+1], pixels[i+2], pixels[i+3] = r, g, b, 255
			}
		}
	}
	return iup.ImageRGBA(n, n, pixels)
}

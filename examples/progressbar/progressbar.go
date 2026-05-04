package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var increment float32 = 0.01

func setBars(value float32) {
	v := fmt.Sprintf("%g", value)
	iup.GetHandle("progContin").SetAttribute("VALUE", v)
	iup.GetHandle("progDashed").SetAttribute("VALUE", v)
	iup.GetHandle("progColored").SetAttribute("VALUE", v)
	iup.GetHandle("progVert").SetAttribute("VALUE", fmt.Sprintf("%g", value*50))
}

func timeCb(ih iup.Ihandle) int {
	v := iup.GetHandle("progContin").GetFloat("VALUE") + increment
	if v > 1 {
		v = 0
	}
	setBars(v)
	return iup.DEFAULT
}

func btnPauseCb(ih iup.Ihandle) int {
	timer := iup.GetHandle("timer")
	btn := iup.GetHandle("btnPause")
	if timer.GetInt("RUN") != 0 {
		timer.SetAttribute("RUN", "NO")
		btn.SetAttribute("TITLE", "Resume")
	} else {
		timer.SetAttribute("RUN", "YES")
		btn.SetAttribute("TITLE", "Pause")
	}
	return iup.DEFAULT
}

func btnRestartCb(ih iup.Ihandle) int { setBars(0); return iup.DEFAULT }
func btnFasterCb(ih iup.Ihandle) int  { increment *= 2; return iup.DEFAULT }
func btnSlowerCb(ih iup.Ihandle) int  { increment /= 2; return iup.DEFAULT }

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.Timer().
		SetHandle("timer").
		SetAttribute("TIME", "100").
		SetCallback("ACTION_CB", iup.TimerActionFunc(timeCb))

	/* Default continuous bar. */
	iup.ProgressBar().
		SetHandle("progContin").
		SetAttribute("EXPAND", "HORIZONTAL")

	/* DASHED is creation-only on Windows, so we configure the variant up front. */
	iup.ProgressBar().
		SetHandle("progDashed").
		SetAttribute("EXPAND", "HORIZONTAL").
		SetAttribute("DASHED", "YES")

	/* MARQUEE is also creation-only; once enabled the bar self-animates. */
	iup.ProgressBar().
		SetHandle("progMarquee").
		SetAttribute("EXPAND", "HORIZONTAL").
		SetAttribute("MARQUEE", "YES")

	/* BGCOLOR and FGCOLOR support varies by driver (Win Classic, Motif, Qt, Android, iOS toast). */
	iup.ProgressBar().
		SetHandle("progColored").
		SetAttribute("EXPAND", "HORIZONTAL").
		SetAttribute("BGCOLOR", "230 230 230").
		SetAttribute("FGCOLOR", "60 160 80")

	/* ORIENTATION is creation-only. */
	iup.ProgressBar().
		SetHandle("progVert").
		SetAttribute("ORIENTATION", "VERTICAL").
		SetAttribute("RASTERSIZE", "40x").
		SetAttribute("EXPAND", "VERTICAL").
		SetAttribute("MIN", "0").
		SetAttribute("MAX", "50").
		SetAttribute("VALUE", "0")

	/* CIRCULAR is honored only on Android (Material CircularProgressIndicator) and iOS (UIActivityIndicatorView). */
	driver := iup.GetGlobal("DRIVER")
	hasCircular := driver == "Android" || driver == "CocoaTouch"
	if hasCircular {
		iup.ProgressBar().
			SetHandle("progCircular").
			SetAttribute("CIRCULAR", "YES").
			SetAttribute("MARQUEE", "YES")
	}

	iup.Button("Pause").
		SetHandle("btnPause").
		SetCallback("ACTION", iup.ActionFunc(btnPauseCb)).
		SetAttribute("EXPAND", "HORIZONTAL")
	iup.Button("Restart").
		SetHandle("btnRestart").
		SetCallback("ACTION", iup.ActionFunc(btnRestartCb)).
		SetAttribute("EXPAND", "HORIZONTAL")
	iup.Button("Faster").
		SetHandle("btnFaster").
		SetCallback("ACTION", iup.ActionFunc(btnFasterCb)).
		SetAttribute("EXPAND", "HORIZONTAL")
	iup.Button("Slower").
		SetHandle("btnSlower").
		SetCallback("ACTION", iup.ActionFunc(btnSlowerCb)).
		SetAttribute("EXPAND", "HORIZONTAL")

	wrap := func(title string, name string) iup.Ihandle {
		return iup.Frame(
			iup.Vbox(iup.GetHandle(name)).SetAttributes("NMARGIN=10x10"),
		).SetAttribute("TITLE", title)
	}

	horizontalChildren := []iup.Ihandle{
		wrap("Continuous", "progContin"),
		wrap("Dashed", "progDashed"),
		wrap("Marquee", "progMarquee"),
		wrap("Colored (MIN/MAX/FGCOLOR/BGCOLOR)", "progColored"),
	}
	if hasCircular {
		horizontalChildren = append(horizontalChildren, wrap("Circular", "progCircular"))
	}
	horizontalStack := iup.Vbox(horizontalChildren...).SetAttributes("NGAP=10")

	controlsFrame := iup.Frame(
		iup.Vbox(
			iup.Hbox(iup.GetHandle("btnPause"), iup.GetHandle("btnRestart")).SetAttributes("NGAP=10"),
			iup.Hbox(iup.GetHandle("btnSlower"), iup.GetHandle("btnFaster")).SetAttributes("NGAP=10"),
		).SetAttributes("NMARGIN=10x10, NGAP=10"),
	).SetAttribute("TITLE", "Controls")

	verticalFrame := iup.Frame(
		iup.Vbox(iup.GetHandle("progVert")).SetAttributes("ALIGNMENT=ACENTER, NMARGIN=10x10"),
	).SetAttribute("TITLE", "Vertical")

	dlg := iup.Dialog(
		iup.Vbox(
			horizontalStack,
			iup.Hbox(controlsFrame, verticalFrame).SetAttributes("NGAP=10"),
		).SetAttributes("NMARGIN=10x10, NGAP=10"),
	)
	dlg.SetAttributes("TITLE=IupProgressBar Test, RASTERSIZE=460x")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.GetHandle("timer").SetAttribute("RUN", "YES")
	iup.MainLoop()
}

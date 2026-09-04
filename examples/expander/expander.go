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

	status = iup.Label("Expander is open")
	status.SetAttribute("EXPAND", "HORIZONTAL")

	iup.SetHandle("arrow_close", makeImage(60, 140, 220))
	iup.SetHandle("arrow_open", makeImage(200, 60, 60))
	iup.SetHandle("arrow_close_high", makeImage(120, 190, 255))
	iup.SetHandle("arrow_open_high", makeImage(255, 120, 120))
	iup.SetHandle("title_close", makeImage(120, 120, 120))
	iup.SetHandle("title_open", makeImage(40, 160, 60))
	iup.SetHandle("title_close_high", makeImage(170, 170, 170))
	iup.SetHandle("title_open_high", makeImage(90, 210, 110))
	iup.SetHandle("extra_a", makeImage(230, 170, 40))
	iup.SetHandle("extra_a_press", makeImage(180, 120, 0))
	iup.SetHandle("extra_a_high", makeImage(255, 210, 90))
	iup.SetHandle("extra_b", makeImage(150, 90, 200))
	iup.SetHandle("extra_b_press", makeImage(100, 40, 150))
	iup.SetHandle("extra_b_high", makeImage(190, 140, 240))

	openClose := func(name string) iup.OpenCloseFunc {
		return func(ih iup.Ihandle, state int) int {
			action := "closing"
			if state != 0 {
				action = "opening"
			}
			report(name + " " + action)
			iup.GetDialog(ih).SetAttribute("RASTERSIZE", nil)
			return iup.DEFAULT
		}
	}

	plain := iup.Expander(iup.Vbox(
		iup.Button("Button ONE").SetAttribute("EXPAND", "HORIZONTAL"),
		iup.Button("Button TWO").SetAttribute("EXPAND", "HORIZONTAL"),
	).SetAttribute("NGAP", "4"))
	plain.SetAttributes(`TITLE="Plain expander", FORECOLOR="0 0 255", OPENCOLOR="0 120 0", HIGHCOLOR="220 40 40", TITLEEXPAND=YES, STATE=OPEN`)
	plain.SetCallback("OPENCLOSE_CB", openClose("Plain expander"))

	animated := iup.Expander(iup.Frame(iup.Vbox(
		iup.Label("The child is a native container so the"),
		iup.Label("SLIDE animation runs when opening and closing."),
		iup.Toggle("A toggle"),
	).SetAttributes("NMARGIN=5x5, NGAP=4")))
	animated.SetAttributes(`TITLE="Animated, framed, custom images", ANIMATION=SLIDE, NUMFRAMES=20, FRAMETIME=20`)
	animated.SetAttributes(`FRAME=YES, FRAMEWIDTH=2, BACKCOLOR="235 235 245", STATE=CLOSE`)
	animated.SetAttributes("IMAGE=arrow_close, IMAGEOPEN=arrow_open, IMAGEHIGHLIGHT=arrow_close_high, IMAGEOPENHIGHLIGHT=arrow_open_high")
	animated.SetAttributes("TITLEIMAGE=title_close, TITLEIMAGEOPEN=title_open, TITLEIMAGEHIGHLIGHT=title_close_high, TITLEIMAGEOPENHIGHLIGHT=title_open_high")
	animated.SetCallback("OPENCLOSE_CB", openClose("Animated expander"))

	extras := iup.Expander(iup.Vbox(
		iup.Label("Two extra buttons sit at the right of the bar."),
		iup.Label("Button 1 is the rightmost one."),
	).SetAttributes("NMARGIN=5x5, NGAP=4"))
	extras.SetAttributes(`TITLE="Extra buttons", EXTRABUTTONS=2, STATE=OPEN`)
	extras.SetAttributes("IMAGEEXTRA1=extra_a, IMAGEEXTRAPRESS1=extra_a_press, IMAGEEXTRAHIGHLIGHT1=extra_a_high")
	extras.SetAttributes("IMAGEEXTRA2=extra_b, IMAGEEXTRAPRESS2=extra_b_press, IMAGEEXTRAHIGHLIGHT2=extra_b_high")
	extras.SetCallback("OPENCLOSE_CB", openClose("Extra buttons expander"))
	extras.SetCallback("EXTRABUTTON_CB", iup.ExtraButtonFunc(func(_ iup.Ihandle, button, pressed int) int {
		report(fmt.Sprintf("EXTRABUTTON_CB button=%d pressed=%d", button, pressed))
		return iup.DEFAULT
	}))

	autoShow := iup.Expander(iup.Vbox(
		iup.Label("Shown while the mouse rests over the bar for a second."),
		iup.Label("STATEREFRESH=NO keeps the dialog layout untouched."),
	).SetAttributes("NMARGIN=5x5, NGAP=4"))
	autoShow.SetAttributes(`TITLE="Auto show", AUTOSHOW=YES, STATEREFRESH=NO, BARSIZE=30, STATE=CLOSE`)
	autoShow.SetCallback("OPENCLOSE_CB", openClose("Auto show expander"))

	left := iup.Expander(iup.Label("Bar at the left").SetAttribute("NMARGIN", "5x5"))
	left.SetAttributes(`BARPOSITION=LEFT, TITLE="Left", STATE=OPEN`)
	left.SetCallback("OPENCLOSE_CB", openClose("Left expander"))

	right := iup.Expander(iup.Label("Bar at the right").SetAttribute("NMARGIN", "5x5"))
	right.SetAttributes(`BARPOSITION=RIGHT, TITLE="Right", STATE=OPEN`)
	right.SetCallback("OPENCLOSE_CB", openClose("Right expander"))

	bottom := iup.Expander(iup.Label("Bar at the bottom").SetAttribute("NMARGIN", "5x5"))
	bottom.SetAttributes(`BARPOSITION=BOTTOM, TITLE="Bottom", STATE=OPEN`)
	bottom.SetCallback("OPENCLOSE_CB", openClose("Bottom expander"))

	toggleAll := iup.Button("Toggle all")
	toggleAll.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		for _, ih := range []iup.Ihandle{plain, animated, extras, autoShow, left, right, bottom} {
			state := "OPEN"
			if ih.GetAttribute("STATE") == "OPEN" {
				state = "CLOSE"
			}
			ih.SetAttribute("STATE", state)
		}
		dlg := iup.GetDialog(plain)
		dlg.SetAttribute("RASTERSIZE", nil)
		iup.Refresh(dlg)
		return iup.DEFAULT
	}))

	column := iup.Vbox(
		plain,
		extras,
		autoShow,
		iup.Hbox(left, right, bottom).SetAttribute("NGAP", "10"),
		toggleAll,
		status,
	).SetAttributes("NGAP=10")

	hbox := iup.Hbox(
		column,
		iup.Vbox(animated, iup.Fill()),
	).SetAttributes("NMARGIN=10x10, NGAP=10")

	dlg := iup.Dialog(hbox).SetAttributes(`TITLE="IupExpander"`)

	iup.Show(dlg)
	iup.MainLoop()
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

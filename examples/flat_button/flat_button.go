//go:build ctrl

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
	iup.ControlsOpen()

	status = iup.Label("Click a button to show its name here")
	status.SetAttribute("EXPAND", "HORIZONTAL")

	action := func(name string) iup.FlatActionFunc {
		return func(iup.Ihandle) int {
			report("Clicked: " + name)
			return iup.DEFAULT
		}
	}

	button := func(title, grad string) iup.Ihandle {
		b := iup.FlatButton(title)
		b.SetAttributes(`EXPAND=HORIZONTAL, PADDING=12x8, CORNERRADIUS=6, FGCOLOR="255 255 255", GRADIENTANGLE=90`)
		if grad != "" {
			b.SetAttribute("GRADIENT", grad)
		}
		iup.SetCallback(b, "FLAT_ACTION", action(title))
		return b
	}

	solid := iup.FlatButton("Solid color")
	solid.SetAttributes(`EXPAND=HORIZONTAL, PADDING=12x8, CORNERRADIUS=6, BGCOLOR="40 120 210", FGCOLOR="255 255 255"`)
	iup.SetCallback(solid, "FLAT_ACTION", action("Solid color"))

	gradients := iup.Vbox(
		solid,
		button("Two-stop gradient", "60 130 220:20 60 130"),
		button("Three-stop gradient", "200 40 40:230 200 40:40 160 60"),
		button("Five-stop gradient", "200 0 0:230 130 0:230 230 0:0 170 0:0 0 210"),
		button("Sunset (four stops)", "40 20 80:180 60 90:240 150 70:250 220 120"),
	)
	gradients.SetAttributes("NGAP=8")

	vbox := iup.Vbox(
		iup.Hbox(
			iup.Frame(gradients).SetAttribute("TITLE", "Gradients"),
			iup.Frame(imageStates()).SetAttribute("TITLE", "Image states"),
			iup.Frame(textLayout()).SetAttribute("TITLE", "Text"),
		).SetAttributes("NGAP=10, NORMALIZESIZE=VERTICAL"),
		iup.Hbox(
			iup.Frame(radioToggles()).SetAttribute("TITLE", "Toggles in a radio"),
			iup.Frame(stateColors()).SetAttribute("TITLE", "State colors and callbacks"),
		).SetAttributes("NGAP=10, NORMALIZESIZE=VERTICAL"),
		status,
	)
	vbox.SetAttributes("NMARGIN=10x10, NGAP=8")

	dlg := iup.Dialog(vbox)
	dlg.SetAttributes("TITLE=IupFlatButton, SHRINK=YES")

	iup.Show(dlg)
	iup.MainLoop()
}

func imageStates() iup.Ihandle {
	iup.SetHandle("img_normal", makeImage(60, 140, 220, 24))
	iup.SetHandle("img_press", makeImage(20, 70, 140, 24))
	iup.SetHandle("img_high", makeImage(120, 190, 255, 24))
	iup.SetHandle("img_inactive", makeImage(170, 170, 170, 24))
	iup.SetHandle("back_normal", makeImage(230, 230, 200, 48))
	iup.SetHandle("back_press", makeImage(200, 200, 150, 48))
	iup.SetHandle("back_high", makeImage(250, 250, 220, 48))
	iup.SetHandle("back_inactive", makeImage(215, 215, 215, 48))
	iup.SetHandle("front_normal", makeImage(200, 60, 60, 16))
	iup.SetHandle("front_press", makeImage(140, 20, 20, 16))
	iup.SetHandle("front_high", makeImage(255, 120, 120, 16))
	iup.SetHandle("front_inactive", makeImage(190, 190, 190, 16))

	states := iup.FlatButton("Image states")
	states.SetAttributes(`IMAGE=img_normal, IMAGEPRESS=img_press, IMAGEHIGHLIGHT=img_high, IMAGEINACTIVE=img_inactive, IMAGEPOSITION=TOP, PADDING=8x6, SPACING=4`)
	iup.SetCallback(states, "FLAT_ACTION", iup.FlatActionFunc(func(iup.Ihandle) int {
		report("Clicked: Image states")
		return iup.DEFAULT
	}))

	back := iup.FlatButton("")
	back.SetAttributes(`BACKIMAGE=back_normal, BACKIMAGEPRESS=back_press, BACKIMAGEHIGHLIGHT=back_high, BACKIMAGEINACTIVE=back_inactive, FITTOBACKIMAGE=YES`)
	back.SetAttributes(`FRONTIMAGE=front_normal, FRONTIMAGEPRESS=front_press, FRONTIMAGEHIGHLIGHT=front_high, FRONTIMAGEINACTIVE=front_inactive`)
	iup.SetCallback(back, "FLAT_ACTION", iup.FlatActionFunc(func(iup.Ihandle) int {
		report("Clicked: back and front images")
		return iup.DEFAULT
	}))

	left := iup.FlatButton("Left")
	left.SetAttributes("IMAGE=img_normal, IMAGEPOSITION=LEFT, CPADDING=2x1, CSPACING=1")
	right := iup.FlatButton("Right")
	right.SetAttributes("IMAGE=img_normal, IMAGEPOSITION=RIGHT, CPADDING=2x1, CSPACING=1")
	bottom := iup.FlatButton("Bottom")
	bottom.SetAttributes("IMAGE=img_normal, IMAGEPOSITION=BOTTOM, CPADDING=2x1, CSPACING=1")

	active := iup.Toggle("Active")
	active.SetAttribute("VALUE", "ON")
	iup.SetCallback(active, "ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		value := "NO"
		if state != 0 {
			value = "YES"
		}
		for _, ih := range []iup.Ihandle{states, back, left, right, bottom} {
			ih.SetAttribute("ACTIVE", value)
		}
		report("ACTIVE=" + value + " shows the INACTIVE images")
		return iup.DEFAULT
	}))

	return iup.Vbox(
		iup.Hbox(states, back).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Hbox(left, right, bottom).SetAttributes("NGAP=4, ALIGNMENT=ACENTER"),
		active,
	).SetAttributes("NGAP=8")
}

func textLayout() iup.Ihandle {
	aligned := iup.FlatButton("Right aligned\nsecond line\nthird")
	aligned.SetAttributes("TEXTALIGNMENT=ARIGHT, PADDING=8x4, EXPAND=HORIZONTAL")

	rotated := iup.FlatButton("Rotated 90")
	rotated.SetAttributes("TEXTORIENTATION=90, PADDING=4x8")

	ellipsis := iup.FlatButton("This long title is cut with an ellipsis when the button shrinks")
	ellipsis.SetAttributes("TEXTELLIPSIS=YES, EXPAND=HORIZONTAL, PADDING=8x4")

	wrap := iup.FlatButton("This long title wraps onto several lines when the button shrinks")
	wrap.SetAttributes("TEXTWRAP=YES, TEXTCLIP=YES, EXPAND=HORIZONTAL, PADDING=8x4")

	split := iup.Split(
		iup.Vbox(ellipsis, wrap).SetAttributes("NGAP=8"),
		iup.Label("Drag the bar to the left").SetAttribute("EXPAND", "HORIZONTAL"),
	)
	split.SetAttributes("ORIENTATION=VERTICAL, VALUE=500, SHOWGRIP=NO")

	return iup.Vbox(
		iup.Hbox(aligned, rotated).SetAttributes("NGAP=8"),
		split,
	).SetAttributes("NGAP=8")
}

func radioToggles() iup.Ihandle {
	toggle := func(title string) iup.Ihandle {
		t := iup.FlatButton(title)
		t.SetAttributes(`TOGGLE=YES, PADDING=10x6, BORDERWIDTH=1, PSCOLOR="120 180 240", HLCOLOR="200 225 250"`)
		iup.SetCallback(t, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
			report(fmt.Sprintf("Toggle %s VALUE=%s", title, ih.GetAttribute("VALUE")))
			return iup.DEFAULT
		}))
		iup.SetCallback(t, "VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
			fmt.Printf("VALUECHANGED_CB %s VALUE=%s\n", title, ih.GetAttribute("VALUE"))
			return iup.DEFAULT
		}))
		return t
	}

	one, two, three := toggle("One"), toggle("Two"), toggle("Three")
	free := toggle("Independent")
	free.SetAttribute("IGNORERADIO", "YES")
	one.SetAttribute("VALUE", "ON")

	radio := iup.Radio(iup.Hbox(one, two, three, free).SetAttribute("NGAP", "4"))

	return iup.Vbox(
		radio,
		iup.Label("The last toggle sets IGNORERADIO and keeps its own state"),
	).SetAttributes("NGAP=8")
}

func stateColors() iup.Ihandle {
	colored := iup.FlatButton("Hover, press and focus me")
	colored.SetAttributes(`PADDING=12x8, SHOWBORDER=YES, BORDERWIDTH=2, BORDERCOLOR="120 120 120", BORDERHLCOLOR="40 120 210", BORDERPSCOLOR="200 40 40"`)
	colored.SetAttributes(`TEXTHLCOLOR="40 120 210", TEXTPSCOLOR="200 40 40", FOCUSFEEDBACK=YES, CANFOCUS=YES`)

	nofocus := iup.FlatButton("No focus feedback")
	nofocus.SetAttributes(`PADDING=12x8, SHOWBORDER=YES, BORDERWIDTH=2, BORDERCOLOR="120 120 120", FOCUSFEEDBACK=NO, CANFOCUS=YES`)

	for _, b := range []iup.Ihandle{colored, nofocus} {
		iup.SetCallback(b, "FLAT_BUTTON_CB", iup.ButtonFunc(func(ih iup.Ihandle, button, pressed, x, y int, st string) int {
			report(fmt.Sprintf("FLAT_BUTTON_CB %s button=%d pressed=%d at %d,%d [%s]", ih.GetAttribute("TITLE"), button, pressed, x, y, st))
			return iup.DEFAULT
		}))
		iup.SetCallback(b, "FLAT_MOTION_CB", iup.MotionFunc(func(ih iup.Ihandle, x, y int, st string) int {
			fmt.Printf("FLAT_MOTION_CB %s %d,%d PRESSED=%s\n", ih.GetAttribute("TITLE"), x, y, ih.GetAttribute("PRESSED"))
			return iup.DEFAULT
		}))
		iup.SetCallback(b, "FLAT_ENTERWINDOW_CB", iup.EnterWindowFunc(func(ih iup.Ihandle) int {
			report(fmt.Sprintf("FLAT_ENTERWINDOW_CB %s HIGHLIGHTED=%s", ih.GetAttribute("TITLE"), ih.GetAttribute("HIGHLIGHTED")))
			return iup.DEFAULT
		}))
		iup.SetCallback(b, "FLAT_LEAVEWINDOW_CB", iup.LeaveWindowFunc(func(ih iup.Ihandle) int {
			report(fmt.Sprintf("FLAT_LEAVEWINDOW_CB %s", ih.GetAttribute("TITLE")))
			return iup.DEFAULT
		}))
		iup.SetCallback(b, "FLAT_FOCUS_CB", iup.FocusFunc(func(ih iup.Ihandle, focus int) int {
			report(fmt.Sprintf("FLAT_FOCUS_CB %s focus=%d HASFOCUS=%s", ih.GetAttribute("TITLE"), focus, ih.GetAttribute("HASFOCUS")))
			return iup.DEFAULT
		}))
	}

	return iup.Vbox(colored, nofocus).SetAttributes("NGAP=8")
}

func makeImage(r, g, b byte, n int) iup.Ihandle {
	pixels := make([]byte, n*n*4)
	c := float64(n) / 2
	for y := 0; y < n; y++ {
		for x := 0; x < n; x++ {
			i := (y*n + x) * 4
			dx, dy := float64(x)-c+0.5, float64(y)-c+0.5
			if dx*dx+dy*dy <= (c-1)*(c-1) {
				pixels[i+0], pixels[i+1], pixels[i+2], pixels[i+3] = r, g, b, 255
			}
		}
	}
	return iup.ImageRGBA(n, n, pixels)
}

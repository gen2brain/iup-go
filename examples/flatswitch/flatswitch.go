package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Default switch (pill shape, white thumb, blue ON, gray OFF)
	sw1 := iup.FlatToggle("Default OFF")
	sw1.SetAttribute("SWITCH", "YES")
	iup.SetCallback(sw1, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Switch initially ON
	sw2 := iup.FlatToggle("Default ON")
	sw2.SetAttributes("SWITCH=YES, VALUE=ON")
	iup.SetCallback(sw2, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Switch without text - OFF
	sw2a := iup.FlatToggle("")
	sw2a.SetAttributes("SWITCH=YES")

	// Switch without text - ON
	sw2b := iup.FlatToggle("")
	sw2b.SetAttributes("SWITCH=YES, VALUE=ON")

	// Switch on right side
	sw3 := iup.FlatToggle("Check Right")
	sw3.SetAttributes("SWITCH=YES, CHECKRIGHT=YES, VALUE=ON")
	iup.SetCallback(sw3, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Custom track ON color (green)
	sw4 := iup.FlatToggle("Green ON")
	sw4.SetAttributes(`SWITCH=YES, VALUE=ON, SWITCHONCOLOR="0 180 0"`)
	iup.SetCallback(sw4, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Custom thumb color
	sw5 := iup.FlatToggle("Yellow Thumb")
	sw5.SetAttributes(`SWITCH=YES, VALUE=ON, SWITCHTHUMBCOLOR="255 220 50"`)
	iup.SetCallback(sw5, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Custom track OFF color
	sw6 := iup.FlatToggle("Red OFF Track")
	sw6.SetAttributes(`SWITCH=YES, SWITCHOFFCOLOR="200 80 80"`)
	iup.SetCallback(sw6, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Highlight colors
	sw7 := iup.FlatToggle("Highlight Colors")
	sw7.SetAttributes(`SWITCH=YES, SWITCHONHLCOLOR="100 200 255", SWITCHTHUMBHLCOLOR="220 220 255"`)
	iup.SetCallback(sw7, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Large switch (CHECKSIZE=32)
	sw8 := iup.FlatToggle("Large (32px)")
	sw8.SetAttributes("SWITCH=YES, CHECKSIZE=32, VALUE=ON")
	iup.SetCallback(sw8, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Custom track width
	sw9 := iup.FlatToggle("Wide Track (80px)")
	sw9.SetAttributes("SWITCH=YES, SWITCHTRACKWIDTH=80, VALUE=ON")
	iup.SetCallback(sw9, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Square track (corner radius = 0)
	sw10 := iup.FlatToggle("Square Track")
	sw10.SetAttributes("SWITCH=YES, SWITCHCORNERRADIUS=0, VALUE=ON")
	iup.SetCallback(sw10, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Square thumb (corner radius = 0)
	sw11 := iup.FlatToggle("Square Thumb")
	sw11.SetAttributes("SWITCH=YES, SWITCHTHUMBCORNERRADIUS=0")
	iup.SetCallback(sw11, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Both square
	sw12 := iup.FlatToggle("All Square")
	sw12.SetAttributes("SWITCH=YES, SWITCHCORNERRADIUS=0, SWITCHTHUMBCORNERRADIUS=0, VALUE=ON")
	iup.SetCallback(sw12, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Both square, large
	sw12a := iup.FlatToggle("All Square Large")
	sw12a.SetAttributes("SWITCH=YES, SWITCHCORNERRADIUS=0, SWITCHTHUMBCORNERRADIUS=0, CHECKSIZE=32, VALUE=ON")
	iup.SetCallback(sw12a, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Rounded rect (partial radius)
	sw13 := iup.FlatToggle("Rounded Rect")
	sw13.SetAttributes("SWITCH=YES, SWITCHCORNERRADIUS=4, SWITCHTHUMBCORNERRADIUS=3")
	iup.SetCallback(sw13, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Gradient track OFF
	sw14 := iup.FlatToggle("Gradient OFF")
	sw14.SetAttributes(`SWITCH=YES, SWITCHTRACKGRADIENT="220 220 220:170 170 170"`)
	iup.SetCallback(sw14, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Gradient track ON
	sw15 := iup.FlatToggle("Gradient ON")
	sw15.SetAttributes(`SWITCH=YES, VALUE=ON, SWITCHTRACKONGRADIENT="0 180 255:0 100 200"`)
	iup.SetCallback(sw15, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Gradient both + angle
	sw16 := iup.FlatToggle("Gradient Both 45deg")
	sw16.SetAttributes(`SWITCH=YES, VALUE=ON, SWITCHTRACKGRADIENT="230 230 230:180 180 180", SWITCHTRACKONGRADIENT="80 200 120:30 140 60", SWITCHTRACKGRADIENTANGLE=45`)
	iup.SetCallback(sw16, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Custom thumb size
	sw17 := iup.FlatToggle("Small Thumb")
	sw17.SetAttributes("SWITCH=YES, SWITCHTHUMBSIZE=8")
	iup.SetCallback(sw17, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	// Disabled OFF
	sw18 := iup.FlatToggle("Disabled OFF")
	sw18.SetAttributes("SWITCH=YES, ACTIVE=NO")

	// Disabled ON
	sw19 := iup.FlatToggle("Disabled ON")
	sw19.SetAttributes("SWITCH=YES, VALUE=ON, ACTIVE=NO")

	// Disabled without text
	sw19a := iup.FlatToggle("")
	sw19a.SetAttributes("SWITCH=YES, ACTIVE=NO")

	sw19b := iup.FlatToggle("")
	sw19b.SetAttributes("SWITCH=YES, VALUE=ON, ACTIVE=NO")

	// Top alignment
	sw20 := iup.FlatToggle("Align Top")
	sw20.SetAttributes("SWITCH=YES, CHECKALIGN=ATOP, RASTERSIZE=x50, VALUE=ON")

	// Bottom alignment
	sw21 := iup.FlatToggle("Align Bottom")
	sw21.SetAttributes("SWITCH=YES, CHECKALIGN=ABOTTOM, RASTERSIZE=x50")

	// Combine everything: large, green, gradient, rounded rect
	sw22 := iup.FlatToggle("Full Custom")
	sw22.SetAttributes(`SWITCH=YES, VALUE=ON, CHECKSIZE=28, SWITCHTRACKWIDTH=56`)
	sw22.SetAttributes(`SWITCHONCOLOR="0 160 80", SWITCHCORNERRADIUS=6, SWITCHTHUMBCORNERRADIUS=4`)
	sw22.SetAttributes(`SWITCHTRACKONGRADIENT="0 200 100:0 130 60"`)
	sw22.SetAttributes(`SWITCHTHUMBCOLOR="240 240 240", SWITCHTHUMBHLCOLOR="255 255 255"`)
	iup.SetCallback(sw22, "FLAT_ACTION", iup.FlatToggleActionFunc(actionCb))

	col1 := iup.Vbox(
		iup.FlatLabel("Basic:").SetAttributes("FONTBOLD=YES"),
		sw1, sw2, iup.Hbox(sw2a, sw2b).SetAttributes("GAP=5"), sw3,
		iup.FlatSeparator(),
		iup.FlatLabel("Track Colors:").SetAttributes("FONTBOLD=YES"),
		sw4, sw5, sw6, sw7,
	).SetAttributes("GAP=4")

	col2 := iup.Vbox(
		iup.FlatLabel("Sizes:").SetAttributes("FONTBOLD=YES"),
		sw8, sw9, sw17,
		iup.FlatSeparator(),
		iup.FlatLabel("Corner Radius:").SetAttributes("FONTBOLD=YES"),
		sw10, sw11, sw12, sw12a, sw13,
	).SetAttributes("GAP=4")

	col3 := iup.Vbox(
		iup.FlatLabel("Gradients:").SetAttributes("FONTBOLD=YES"),
		sw14, sw15, sw16,
		iup.FlatSeparator(),
		iup.FlatLabel("States:").SetAttributes("FONTBOLD=YES"),
		sw18, sw19, iup.Hbox(sw19a, sw19b).SetAttributes("GAP=5"),
		iup.FlatSeparator(),
		iup.FlatLabel("Alignment & Custom:").SetAttributes("FONTBOLD=YES"),
		sw20, sw21, sw22,
	).SetAttributes("GAP=4")

	hbox := iup.Hbox(col1, col2, col3).SetAttributes("GAP=20")

	mainVbox := iup.Vbox(
		iup.FlatLabel("FlatToggle SWITCH Examples").SetAttributes("FONTSIZE=14, FONTBOLD=YES, ALIGNMENT=ACENTER"),
		iup.FlatSeparator(),
		hbox,
	).SetAttributes("MARGIN=10x10, GAP=8")

	dlg := iup.Dialog(mainVbox)
	dlg.SetAttributes(`TITLE="FlatToggle Switch", RESIZE=NO`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func actionCb(ih iup.Ihandle, state int) int {
	title := ih.GetAttribute("TITLE")
	fmt.Printf("%s: state=%d\n", title, state)
	return iup.DEFAULT
}

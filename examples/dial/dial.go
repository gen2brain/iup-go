package main

import (
	"fmt"
	"math"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	lblH := iup.Label("0.0 degrees").SetHandle("lblH").SetAttributes("ALIGNMENT=ACENTER, EXPAND=HORIZONTAL")
	lblV := iup.Label("0 radians").SetHandle("lblV").SetAttributes("ALIGNMENT=ACENTER, EXPAND=HORIZONTAL")

	dialV := iup.Dial("VERTICAL").SetAttributes(`FLAT=YES, FLATCOLOR="200 40 40"`)
	dialH := iup.Dial("HORIZONTAL").SetAttributes("DENSITY=0.3")

	lblC := iup.Label("VALUE -0.00 rad, -180.0 degrees (Home resets)").SetHandle("lblC").SetAttributes("ALIGNMENT=ACENTER, EXPAND=HORIZONTAL")
	dialC := iup.Dial("CIRCULAR").SetAttributes("DENSITY=0.5")

	dialV.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(dialVCb))
	dialC.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(dialCCb))
	dialH.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("lblH").SetAttribute("TITLE", fmt.Sprintf("%.1f degrees", ih.GetFloat("VALUE")*180/math.Pi))
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Hbox(
			iup.Vbox(
				iup.Vbox(
					dialV,
					lblV,
				),
				iup.Vbox(
					dialH,
					lblH,
				),
			).SetAttribute("NGAP", "5"),
			iup.Vbox(
				dialC,
				lblC,
			).SetAttributes("NGAP=5, ALIGNMENT=ACENTER"),
		).SetAttributes("NMARGIN=10x10, NGAP=15"),
	).SetAttributes(`TITLE="Dial"`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func dialVCb(ih iup.Ihandle) int {
	iup.GetHandle("lblV").SetAttribute("TITLE", fmt.Sprintf("%.2f radians", ih.GetFloat("VALUE")))
	return iup.DEFAULT
}

func dialCCb(ih iup.Ihandle) int {
	v := ih.GetFloat("VALUE")
	iup.GetHandle("lblC").SetAttribute("TITLE", fmt.Sprintf("VALUE %.2f rad, %.1f degrees (Home resets)", v, v*180/math.Pi))
	return iup.DEFAULT
}

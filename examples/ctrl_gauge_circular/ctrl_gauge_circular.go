//go:build ctrl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.ControlsOpen()

	plain := iup.Gauge().SetAttributes(`CIRCULAR=YES`)
	dashed := iup.Gauge().SetAttributes(`CIRCULAR=YES, DASHED=YES, SHOWTEXT=NO, FGCOLOR="60 160 90"`)
	thin := iup.Gauge().SetAttributes(`CIRCULAR=YES, RINGWIDTH=3, FGCOLOR="200 60 60"`)
	thick := iup.Gauge().SetAttributes(`CIRCULAR=YES, RINGWIDTH=14, SHOWTEXT=NO, FGCOLOR="230 150 30", BACKCOLOR="225 225 225"`)
	count := iup.Gauge().SetAttributes(`CIRCULAR=YES, MAX=60, TEXT=0s, FGCOLOR="120 80 200"`)
	bar := iup.Gauge().SetAttributes(`CORNERRADIUS=8, EXPAND=HORIZONTAL`)

	gauges := []iup.Ihandle{plain, dashed, thin, thick, bar}

	value := 0.0
	timer := iup.Timer().SetAttributes("TIME=40")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		value += 0.005
		if value > 1 {
			value = 0
		}
		for _, g := range gauges {
			g.SetAttribute("VALUE", value)
		}
		count.SetAttribute("TEXT", fmt.Sprintf("%ds", int(value*60)))
		count.SetAttribute("VALUE", value*60)
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(plain, dashed, thin, thick, count).SetAttributes("GAP=16, ALIGNMENT=ACENTER"),
			bar,
		).SetAttributes("GAP=16, MARGIN=16x16"),
	).SetAttribute("TITLE", "Circular Gauge")

	iup.Show(dlg)
	timer.SetAttribute("RUN", "YES")
	iup.MainLoop()
}

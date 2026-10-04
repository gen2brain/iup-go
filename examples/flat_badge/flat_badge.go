//go:build ctrl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

const accent = "60 120 200"

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.ControlsOpen()

	count := 3
	inbox := iup.FlatButton("Inbox").SetAttributes(map[string]string{
		"BADGE": "3", "SHOWBORDER": "YES", "BORDERCOLOR": accent, "CORNERRADIUS": "8", "PADDING": "18x10",
	})
	inbox.SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		count++
		ih.SetAttribute("BADGE", fmt.Sprintf("%d", count))
		return iup.DEFAULT
	}))
	updates := iup.FlatButton("Updates").SetAttributes(map[string]string{
		"BADGE": "new", "BADGECOLOR": "60 160 90", "SHOWBORDER": "YES", "BORDERCOLOR": accent, "CORNERRADIUS": "8", "PADDING": "18x10",
	})

	tabs := iup.FlatTabs(
		iup.Vbox(iup.Label("Messages").SetAttribute("EXPAND", "HORIZONTAL")).SetAttributes("MARGIN=10x10"),
		iup.Vbox(iup.Label("Calls").SetAttribute("EXPAND", "HORIZONTAL")).SetAttributes("MARGIN=10x10"),
		iup.Vbox(iup.Label("Alerts").SetAttribute("EXPAND", "HORIZONTAL")).SetAttributes("MARGIN=10x10"),
	).SetAttributes(map[string]string{
		"TABTITLE0": "Messages", "TABTITLE1": "Calls", "TABTITLE2": "Alerts",
		"TABBADGE0": "12", "TABBADGE2": "128", "TABSPADDING": "18x10", "TABSCORNERRADIUS": "8",
	})

	group := iup.Vbox(
		iup.Hbox(
			iup.FlatButton("Button").SetAttributes(map[string]string{
				"SHOWBORDER": "YES", "BORDERCOLOR": accent, "BGCOLOR": "230 240 255", "CORNERRADIUS": "8", "PADDING": "10x6",
			}),
			iup.FlatToggle("Check box").SetAttributes(map[string]string{"VALUE": "ON", "CHECKFGCOLOR": accent}),
			iup.Gauge().SetAttributes(`CIRCULAR=YES, VALUE=0.6, FGCOLOR="60 120 200"`),
		).SetAttributes("GAP=10, ALIGNMENT=ACENTER"),
		iup.FlatVal("HORIZONTAL").SetAttributes(map[string]string{
			"VALUE": "0.4", "EXPAND": "HORIZONTAL", "SLIDERFILLCOLOR": accent, "BORDERCOLOR": accent, "CORNERRADIUS": "8",
		}),
	).SetAttributes("GAP=10, INACTIVEOPACITY=90")

	enabled := iup.Toggle("Enabled").SetAttribute("VALUE", "ON")
	enabled.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		if state == 1 {
			group.SetAttribute("ACTIVE", "YES")
		} else {
			group.SetAttribute("ACTIVE", "NO")
		}
		return iup.DEFAULT
	}))

	sep := func(style string) iup.Ihandle {
		return iup.Separator().SetAttributes("ORIENTATION=HORIZONTAL, STYLE=" + style)
	}

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(inbox, updates).SetAttributes("GAP=10"),
			tabs,
			sep("DASHED"),
			enabled,
			group,
			sep("DOTTED"),
		).SetAttributes("GAP=12, MARGIN=12x12"),
	).SetAttribute("TITLE", "Flat badge")

	iup.Show(dlg)
	iup.MainLoop()
}

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func setStatus(text string) {
	iup.GetHandle("status").SetAttribute("TITLE", text)
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("APPID", "com.example.Location")

	loc := iup.Location()
	loc.SetCallback("LOCATION_CB", iup.LocationFunc(func(ih iup.Ihandle, lat, lon float64) int {
		iup.GetHandle("latitude").SetAttribute("TITLE", fmt.Sprintf("%.6f", lat))
		iup.GetHandle("longitude").SetAttribute("TITLE", fmt.Sprintf("%.6f", lon))
		iup.GetHandle("accuracy").SetAttribute("TITLE", ih.GetAttribute("HORIZONTALACCURACY")+" m")
		iup.GetHandle("altitude").SetAttribute("TITLE", ih.GetAttribute("ALTITUDE"))
		iup.GetHandle("speed").SetAttribute("TITLE", ih.GetAttribute("SPEED"))
		setStatus("Fix at " + ih.GetAttribute("TIMESTAMP"))
		return iup.DEFAULT
	}))
	loc.SetCallback("PERMISSION_CB", iup.PermissionFunc(func(ih iup.Ihandle, granted int) int {
		if granted == 1 {
			setStatus("Permission granted")
		} else {
			setStatus("Permission denied")
		}
		return iup.DEFAULT
	}))
	loc.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, message string) int {
		setStatus("Error: " + message)
		return iup.DEFAULT
	}))

	active := iup.Toggle("Active")
	active.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		if state == 1 {
			loc.SetAttribute("ACTIVE", "YES")
			ih.SetAttribute("VALUE", loc.GetAttribute("ACTIVE"))
		} else {
			loc.SetAttribute("ACTIVE", "NO")
		}
		return iup.DEFAULT
	}))

	fine := iup.Toggle("Fine accuracy")
	fine.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		if state == 1 {
			loc.SetAttribute("ACCURACY", "FINE")
		} else {
			loc.SetAttribute("ACCURACY", "COARSE")
		}
		return iup.DEFAULT
	}))

	value := func(handle string) iup.Ihandle {
		label := iup.Label("-").SetAttributes(`EXPAND=HORIZONTAL`)
		label.SetHandle(handle)
		return label
	}

	status := iup.Label("Available: " + loc.GetAttribute("AVAILABLE") + ", permission: " + loc.GetAttribute("PERMISSION"))
	status.SetAttribute("EXPAND", "HORIZONTAL")
	status.SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(active, fine).SetAttributes(`GAP=10`),
			iup.GridBox(
				iup.Label("Latitude"), value("latitude"),
				iup.Label("Longitude"), value("longitude"),
				iup.Label("Accuracy"), value("accuracy"),
				iup.Label("Altitude"), value("altitude"),
				iup.Label("Speed"), value("speed"),
			).SetAttributes(`NUMDIV=2, GAPLIN=5, GAPCOL=10, ALIGNMENTLIN=ACENTER, EXPANDCHILDREN=HORIZONTAL`),
			status,
		).SetAttributes(`GAP=5, MARGIN=10x10`),
	).SetAttributes(`TITLE="Location", SIZE=HALFx`)

	iup.Show(dlg)
	iup.MainLoop()
}

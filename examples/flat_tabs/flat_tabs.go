//go:build ctrl

package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func logMsg(msg string) {
	log := iup.GetHandle("log")
	if log != 0 {
		log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s", time.Now().Format("15:04:05"), msg))
	}
	fmt.Println(msg)
}

func init() { iup.EntryPoint(main) }

func page(title, rgb string) iup.Ihandle {
	vbox := iup.Vbox(
		iup.Label("Content of the \""+title+"\" tab.").SetAttribute("EXPAND", "YES"),
		iup.Button("Button "+title),
	).SetAttributes("NMARGIN=15x15, NGAP=8, EXPAND=YES")
	vbox.SetAttribute("TABTITLE", title)
	if rgb != "" {
		vbox.SetAttribute("TABBACKCOLOR", rgb)
	}
	return vbox
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.ControlsOpen()

	iup.SetHandle("close", makeImage(120, 120, 120))
	iup.SetHandle("close_high", makeImage(220, 60, 60))
	iup.SetHandle("close_press", makeImage(140, 20, 20))
	iup.SetHandle("close_inactive", makeImage(200, 200, 200))
	iup.SetHandle("extra", makeImage(60, 140, 220))
	iup.SetHandle("extra_high", makeImage(120, 190, 255))
	iup.SetHandle("extra_press", makeImage(20, 70, 140))
	iup.SetHandle("extra_inactive", makeImage(170, 170, 170))
	iup.SetHandle("tab_icon", makeImage(230, 170, 40))

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=YES, READONLY=YES, VISIBLELINES=6")
	txtLog.SetAttribute("VALUE", "Drag a tab title onto another to reorder it.\n"+
		"An insertion line shows where it will land; colors follow the tab.")
	iup.SetHandle("log", txtLog)

	red, green, blue, amber := page("Red", "230 120 120"), page("Green", "120 200 120"), page("Blue", "120 150 230"), page("Amber", "235 190 100")
	green.SetAttribute("TABIMAGE", "tab_icon")

	tabs := iup.FlatTabs(red, green, blue, amber)
	tabs.SetAttributes("ALLOWREORDER=YES, EXPAND=YES, SHOWLINES=YES, SHOWCLOSE=YES, TABCHANGEONCHECK=YES, CHILDSIZEALL=YES")
	tabs.SetAttributes(`TABSPADDING=12x6, TABSFORECOLOR="60 60 60", TABSBACKCOLOR="225 225 225", TABSHIGHCOLOR="245 245 245", HIGHCOLOR="0 90 200"`)
	tabs.SetAttributes(`TABSLINECOLOR="120 120 120", TABSFONTSTYLE=Italic, TABSIMAGEPOSITION=RIGHT, TABSIMAGESPACING=6, TABSALIGNMENT="ACENTER:ACENTER"`)
	tabs.SetAttributes(`TABSTEXTALIGNMENT=ACENTER, TABFONTSTYLE0=Bold, TABFONTSIZE3=14, TABFORECOLOR3="150 60 0"`)
	tabs.SetAttributes("TABTIP0=The red tab, TABTIP1=The green tab has an image, TABTIP2=The blue tab, TABTIP3=The amber tab is bigger")
	tabs.SetAttributes("CLOSEIMAGE=close, CLOSEIMAGEHIGHLIGHT=close_high, CLOSEIMAGEPRESS=close_press, CLOSEIMAGEINACTIVE=close_inactive")
	tabs.SetAttributes(`CLOSEHIGHCOLOR="255 220 220", CLOSEPRESSCOLOR="255 180 180"`)
	tabs.SetAttributes(`EXTRABUTTONS=2, EXTRATITLE1=Pin, EXTRATOGGLE1=YES, EXTRATIP1=Toggle button, EXTRABORDERWIDTH1=1, EXTRASHOWBORDER1=YES`)
	tabs.SetAttributes(`EXTRABORDERCOLOR1="60 140 220", EXTRAFORECOLOR1="0 60 120", EXTRAPRESSCOLOR1="200 225 250", EXTRAHIGHCOLOR1="225 240 255", EXTRAFONT1="Sans, Bold 9"`)
	tabs.SetAttributes("EXTRAIMAGE2=extra, EXTRAIMAGEHIGHLIGHT2=extra_high, EXTRAIMAGEPRESS2=extra_press, EXTRAIMAGEINACTIVE2=extra_inactive, EXTRATIP2=Image button, EXTRAALIGNMENT2=ACENTER:ATOP")
	tabs.SetAttribute("EXPANDBUTTON", "YES")

	tabs.SetCallback("REORDER_CB", iup.ReorderFunc(func(ih iup.Ihandle, oldPos, newPos int) int {
		logMsg(fmt.Sprintf("REORDER_CB: tab moved from %d to %d", oldPos, newPos))
		return iup.DEFAULT
	}))
	tabs.SetCallback("TABCHANGEPOS_CB", iup.TabChangePosFunc(func(ih iup.Ihandle, newPos, oldPos int) int {
		logMsg(fmt.Sprintf("TABCHANGEPOS_CB: position %d -> %d", oldPos, newPos))
		return iup.DEFAULT
	}))
	tabs.SetCallback("TABCHANGE_CB", iup.TabChangeFunc(func(ih, newTab, oldTab iup.Ihandle) int {
		logMsg(fmt.Sprintf("TABCHANGE_CB: %s -> %s", oldTab.GetAttribute("TABTITLE"), newTab.GetAttribute("TABTITLE")))
		return iup.DEFAULT
	}))
	tabs.SetCallback("TABCLOSE_CB", iup.TabCloseFunc(func(ih iup.Ihandle, pos int) int {
		logMsg(fmt.Sprintf("TABCLOSE_CB: tab %d hidden instead of closed", pos))
		return iup.IGNORE
	}))
	tabs.SetCallback("RIGHTCLICK_CB", iup.RightClickFunc(func(ih iup.Ihandle, pos int) int {
		logMsg(fmt.Sprintf("RIGHTCLICK_CB: tab %d", pos))
		return iup.DEFAULT
	}))
	tabs.SetCallback("EXTRABUTTON_CB", iup.ExtraButtonFunc(func(ih iup.Ihandle, button, pressed int) int {
		logMsg(fmt.Sprintf("EXTRABUTTON_CB: button=%d pressed=%d EXTRAVALUE1=%s EXPANDBUTTONSTATE=%s EXPANDBUTTONPOS=%s EXTRABOX%d=%s",
			button, pressed, ih.GetAttribute("EXTRAVALUE1"), ih.GetAttribute("EXPANDBUTTONSTATE"), ih.GetAttribute("EXPANDBUTTONPOS"),
			button, iup.GetAttributeId(ih, "EXTRABOX", button)))
		return iup.DEFAULT
	}))
	tabs.SetCallback("FLAT_BUTTON_CB", iup.ButtonFunc(func(ih iup.Ihandle, button, pressed, x, y int, st string) int {
		logMsg(fmt.Sprintf("FLAT_BUTTON_CB: button=%d pressed=%d at %d,%d [%s] tab=%d", button, pressed, x, y, st, iup.ConvertXYToPos(ih, x, y)))
		return iup.DEFAULT
	}))
	tabs.SetCallback("FLAT_MOTION_CB", iup.MotionFunc(func(ih iup.Ihandle, x, y int, st string) int {
		fmt.Printf("FLAT_MOTION_CB: %d,%d tab=%d", x, y, iup.ConvertXYToPos(ih, x, y))
		return iup.DEFAULT
	}))
	tabs.SetCallback("FLAT_LEAVEWINDOW_CB", iup.LeaveWindowFunc(func(ih iup.Ihandle) int {
		fmt.Println("FLAT_LEAVEWINDOW_CB")
		return iup.DEFAULT
	}))
	tabs.SetCallback("FLAT_GETFOCUS_CB", iup.GetFocusFunc(func(ih iup.Ihandle) int {
		logMsg("FLAT_GETFOCUS_CB")
		return iup.DEFAULT
	}))
	tabs.SetCallback("FLAT_KILLFOCUS_CB", iup.KillFocusFunc(func(ih iup.Ihandle) int {
		logMsg(fmt.Sprintf("FLAT_KILLFOCUS_CB HASFOCUS=%s", ih.GetAttribute("HASFOCUS")))
		return iup.DEFAULT
	}))

	showAll := iup.Button("Show all tabs")
	showAll.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		for i := 0; i < 4; i++ {
			iup.SetAttributeId(tabs, "TABVISIBLE", i, "YES")
		}
		return iup.DEFAULT
	}))

	selectAmber := iup.Button("Select Amber by handle")
	selectAmber.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		iup.SetAttributeHandle(tabs, "VALUE_HANDLE", amber)
		return iup.DEFAULT
	}))

	fixedWidth := iup.Toggle("FIXEDWIDTH")
	fixedWidth.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		if state != 0 {
			tabs.SetAttribute("FIXEDWIDTH", "110")
		} else {
			tabs.SetAttribute("FIXEDWIDTH", "0")
		}
		return iup.DEFAULT
	}))

	blueActive := iup.Toggle("TABACTIVE2")
	blueActive.SetAttribute("VALUE", "ON")
	blueActive.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		value := "NO"
		if state != 0 {
			value = "YES"
		}
		tabs.SetAttribute("TABACTIVE2", value)
		return iup.DEFAULT
	}))

	tabType := iup.List()
	tabType.SetAttributes("DROPDOWN=YES, 1=TOP, 2=BOTTOM, 3=LEFT, 4=RIGHT, VALUE=1")
	tabType.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			tabs.SetAttribute("TABTYPE", text)
			iup.Refresh(tabs)
		}
		return iup.DEFAULT
	}))

	vboxMain := iup.Vbox(
		tabs,
		iup.Hbox(showAll, selectAmber, fixedWidth, blueActive, iup.Label("TABTYPE"), tabType).SetAttributes("NGAP=5, ALIGNMENT=ACENTER"),
		iup.Frame(txtLog).SetAttributes(`TITLE="Event Log"`),
	).SetAttributes("NMARGIN=10x10, NGAP=10")

	dlg := iup.Dialog(vboxMain).SetAttributes(`TITLE="IupFlatTabs"`)

	iup.Show(dlg)
	iup.MainLoop()
}

func makeImage(r, g, b byte) iup.Ihandle {
	const n = 12
	pixels := make([]byte, n*n*4)
	for y := 0; y < n; y++ {
		for x := 0; x < n; x++ {
			i := (y*n + x) * 4
			dx, dy := float64(x-6)+0.5, float64(y-6)+0.5
			if dx*dx+dy*dy <= 25 {
				pixels[i+0], pixels[i+1], pixels[i+2], pixels[i+3] = r, g, b, 255
			}
		}
	}
	return iup.ImageRGBA(n, n, pixels)
}

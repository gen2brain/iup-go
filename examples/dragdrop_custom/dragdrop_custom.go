package main

import (
	"fmt"
	"strings"
	"unsafe"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func setStatus(s string) { iup.GetHandle("status").SetAttribute("TITLE", s) }

type color struct {
	name, rgb string
}

var colors = []color{
	{"Red", "220 40 40"},
	{"Green", "40 180 60"},
	{"Blue", "50 90 220"},
	{"Orange", "230 140 30"},
	{"Purple", "150 60 200"},
}

type dropped struct {
	rgb  string
	x, y int
}

var drops []dropped

// swatch is a draggable canvas carrying an app-defined color payload.
func swatch(c color) iup.Ihandle {
	cv := iup.Canvas().SetAttributes(fmt.Sprintf(`RASTERSIZE=110x28, BGCOLOR="%s", DRAGSOURCE=YES, DRAGTYPES=org.iup-go.color`, c.rgb))
	cv.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.DrawBegin(ih)
		w, h := iup.DrawGetSize(ih)
		ih.SetAttributes(fmt.Sprintf(`DRAWSTYLE=FILL, DRAWCOLOR="%s"`, c.rgb))
		iup.DrawRectangle(ih, 0, 0, w, h)
		ih.SetAttribute("DRAWCOLOR", "255 255 255")
		iup.DrawText(ih, c.name, 8, 6, 0, 0)
		iup.DrawEnd(ih)
		return iup.DEFAULT
	}))
	cv.SetCallback("DRAGBEGIN_CB", iup.DragBeginFunc(func(iup.Ihandle, int, int) int {
		setStatus("dragging " + c.name)
		return iup.DEFAULT
	}))
	cv.SetCallback("DRAGDATASIZE_CB", iup.DragDataSizeFunc(func(iup.Ihandle, string) int {
		return len(c.rgb) + 1
	}))
	cv.SetCallback("DRAGDATA_CB", iup.DragDataFunc(func(_ iup.Ihandle, _ string, data unsafe.Pointer, size int) int {
		buf := unsafe.Slice((*byte)(data), size)
		if n := copy(buf, c.rgb); n < size {
			buf[n] = 0
		}
		return iup.DEFAULT
	}))
	cv.SetCallback("DRAGEND_CB", iup.DragEndFunc(func(_ iup.Ihandle, action int) int {
		setStatus(fmt.Sprintf("drag ended (action=%d)", action))
		return iup.DEFAULT
	}))
	return cv
}

// target is a canvas that draws a square of the dropped color at the drop point.
func target() iup.Ihandle {
	canvas := iup.Canvas().SetAttributes(`EXPAND=YES, DROPTARGET=YES, DROPTYPES=org.iup-go.color`)
	canvas.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.DrawBegin(ih)
		w, h := iup.DrawGetSize(ih)
		ih.SetAttributes(`DRAWSTYLE=FILL, DRAWCOLOR="250 250 250"`)
		iup.DrawRectangle(ih, 0, 0, w, h)
		for _, d := range drops {
			ih.SetAttributes(fmt.Sprintf(`DRAWSTYLE=FILL, DRAWCOLOR="%s"`, d.rgb))
			iup.DrawRectangle(ih, d.x-16, d.y-16, d.x+16, d.y+16)
		}
		iup.DrawEnd(ih)
		return iup.DEFAULT
	}))
	canvas.SetCallback("DROPDATA_CB", iup.DropDataFunc(func(ih iup.Ihandle, _ string, data unsafe.Pointer, size, x, y int) int {
		rgb := string(unsafe.Slice((*byte)(data), size))
		if i := strings.IndexByte(rgb, 0); i >= 0 {
			rgb = rgb[:i]
		}
		drops = append(drops, dropped{rgb, x, y})
		iup.Update(ih)
		setStatus(fmt.Sprintf("dropped %q at %d,%d", rgb, x, y))
		return iup.DEFAULT
	}))
	canvas.SetCallback("DROPMOTION_CB", iup.DropMotionFunc(func(iup.Ihandle, int, int, string) int {
		return iup.DEFAULT
	}))
	return canvas
}

func main() {
	iup.Open()
	defer iup.Close()

	var swatches []iup.Ihandle
	for _, c := range colors {
		swatches = append(swatches, swatch(c))
	}
	sources := iup.Vbox(swatches...).SetAttributes(`NMARGIN=8x8, NGAP=8`)

	status := iup.Label("Drag a color onto the canvas.").SetAttribute("EXPAND", "HORIZONTAL")
	iup.SetHandle("status", status)

	row := iup.Hbox(
		iup.Frame(sources).SetAttribute("TITLE", "Drag sources"),
		iup.Frame(target()).SetAttributes(`TITLE="Drop target (canvas)", EXPAND=YES`),
	).SetAttributes(`NMARGIN=8x8, NGAP=8`)

	dlg := iup.Dialog(
		iup.Vbox(row, status).SetAttributes(`NMARGIN=8x8, NGAP=6`),
	).SetAttributes(`TITLE="Custom drag & drop", SIZE=HALFxHALF`)

	iup.Show(dlg)
	iup.MainLoop()
}

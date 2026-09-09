package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var names = []string{
	"ARROW", "BUSY", "CROSS", "HAND",
	"HELP", "MOVE", "NO", "PEN",
	"RESIZE_N", "RESIZE_S", "RESIZE_NS", "RESIZE_W",
	"RESIZE_E", "RESIZE_WE", "RESIZE_NE", "RESIZE_SW",
	"RESIZE_NW", "RESIZE_SE", "SPLITTER_HORIZ", "SPLITTER_VERT",
	"TEXT", "UPARROW", "NONE", "custom",
}

const columns = 4

var (
	hovered = -1
	status  iup.Ihandle
)

func main() {
	iup.Open()
	defer iup.Close()

	customCursor().SetHandle("custom")

	rows := (len(names) + columns - 1) / columns
	canvas := iup.Canvas().SetAttributes(map[string]string{
		"SIZE":   fmt.Sprintf("%dx%d", columns*80, rows*24),
		"EXPAND": "YES",
		"BORDER": "NO",
	})
	canvas.SetCallback("ACTION", iup.ActionFunc(draw))
	canvas.SetCallback("MOTION_CB", iup.MotionFunc(motion))
	canvas.SetCallback("LEAVEWINDOW_CB", iup.LeaveWindowFunc(leave))

	status = iup.Label("Move the mouse over a cell").SetAttribute("EXPAND", "HORIZONTAL")

	dlg := iup.Dialog(iup.Vbox(canvas, status).SetAttributes("MARGIN=10x10, GAP=6"))
	dlg.SetAttribute("TITLE", "Cursor")

	iup.Show(dlg)
	iup.MainLoop()
}

func customCursor() iup.Ihandle {
	const size = 32
	pixels := make([]byte, size*size)
	for y := 0; y < size; y++ {
		for x := 0; x < size; x++ {
			dx, dy := x-size/2, y-size/2
			switch {
			case dx*dx+dy*dy < 5*5:
				pixels[y*size+x] = 2
			case dx == 0 || dy == 0 || dx*dx+dy*dy < 14*14 && dx*dx+dy*dy > 12*12:
				pixels[y*size+x] = 1
			}
		}
	}
	return iup.Image(size, size, pixels).SetAttributes(map[string]string{
		"0":       "BGCOLOR",
		"1":       "0 0 0",
		"2":       "220 40 40",
		"HOTSPOT": fmt.Sprintf("%d:%d", size/2, size/2),
	})
}

func cellAt(ih iup.Ihandle, x, y int) int {
	_, w, h := ih.GetIntInt("DRAWSIZE")
	rows := (len(names) + columns - 1) / columns
	if w <= 0 || h <= 0 || x < 0 || y < 0 || x >= w || y >= h {
		return -1
	}
	i := y*rows/h*columns + x*columns/w
	if i >= len(names) {
		return -1
	}
	return i
}

func motion(ih iup.Ihandle, x, y int, status string) int {
	i := cellAt(ih, x, y)
	if i == hovered {
		return iup.DEFAULT
	}
	hovered = i
	if i >= 0 {
		ih.SetAttribute("CURSOR", names[i])
		setStatus(names[i])
	} else {
		ih.SetAttribute("CURSOR", "ARROW")
	}
	iup.Update(ih)
	return iup.DEFAULT
}

func leave(ih iup.Ihandle) int {
	hovered = -1
	ih.SetAttribute("CURSOR", "ARROW")
	iup.Update(ih)
	return iup.DEFAULT
}

func setStatus(name string) {
	if name == "custom" {
		status.SetAttribute("TITLE", "CURSOR=custom, an IupImage with HOTSPOT")
	} else {
		status.SetAttribute("TITLE", "CURSOR="+name)
	}
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	iup.DrawParentBackground(ih)

	w, h := iup.DrawGetSize(ih)
	rows := (len(names) + columns - 1) / columns
	cw, ch := w/columns, h/rows

	for i, name := range names {
		x, y := (i%columns)*cw, (i/columns)*ch
		if i == hovered {
			ih.SetAttribute("DRAWCOLOR", iup.GetGlobal("TXTHLCOLOR"))
			ih.SetAttribute("DRAWSTYLE", "FILL")
			iup.DrawRectangle(ih, x, y, x+cw-1, y+ch-1)
		}
		ih.SetAttribute("DRAWCOLOR", iup.GetGlobal("DLGFGCOLOR"))
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		iup.DrawRectangle(ih, x, y, x+cw-1, y+ch-1)
		tw, th := iup.DrawGetTextSize(ih, name)
		iup.DrawText(ih, name, x+(cw-tw)/2, y+(ch-th)/2, tw, th)
	}
	return iup.DEFAULT
}

package main

import (
	"fmt"
	"math"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type shape struct {
	name  string
	path  iup.Ihandle
	x, y  float64
	rot   float64
	scale float64
	color string
	rule  int
}

var (
	shapes  []*shape
	hovered *shape
	picked  *shape
	dragX   float64
	dragY   float64
	status  = "Move over a shape to pick it, drag to move"
)

func star(cx, cy, outer, inner float64, points int) iup.Ihandle {
	p := iup.DrawPathCreate()
	for i := 0; i < points*2; i++ {
		r := outer
		if i%2 == 1 {
			r = inner
		}
		a := float64(i)*math.Pi/float64(points) - math.Pi/2
		x, y := cx+r*math.Cos(a), cy+r*math.Sin(a)
		if i == 0 {
			iup.DrawPathMoveToF(p, x, y)
		} else {
			iup.DrawPathLineToF(p, x, y)
		}
	}
	iup.DrawPathClose(p)
	return p
}

func build() {
	plate := iup.DrawPathCreate()
	iup.DrawPathSetSvg(plate, "M 0 20 Q 0 0 20 0 L 180 0 Q 200 0 200 20 L 200 100 Q 200 120 180 120 L 20 120 Q 0 120 0 100 Z")

	donut := iup.DrawPathCreate()
	iup.DrawPathSetSvg(donut, "M 60 0 A 60 60 0 1 1 59.9 0 Z M 60 25 A 35 35 0 1 0 60.1 25 Z")

	leaf := iup.DrawPathCreate()
	iup.DrawPathMoveToF(leaf, 0, 45)
	iup.DrawPathCurveToF(leaf, 0, 12, 35, -12, 80, 0)
	iup.DrawPathCurveToF(leaf, 68, 45, 35, 62, 0, 45)
	iup.DrawPathClose(leaf)

	shapes = []*shape{
		{name: "plate (behind the donut)", path: plate, x: 60, y: 60, scale: 1, color: "180 200 220", rule: iup.DRAW_RULE_WINDING},
		{name: "donut (even-odd: the hole is not pickable)", path: donut, x: 90, y: 40, scale: 1, color: "230 150 60", rule: iup.DRAW_RULE_EVENODD},
		{name: "star (rotated)", path: star(0, 0, 70, 28, 5), x: 400, y: 120, rot: 18, scale: 1, color: "120 110 220", rule: iup.DRAW_RULE_WINDING},
		{name: "leaf (scaled)", path: leaf, x: 330, y: 250, rot: -12, scale: 1.4, color: "90 170 90", rule: iup.DRAW_RULE_WINDING},
	}
}

func (s *shape) local(x, y float64) (float64, float64) {
	dx, dy := x-s.x, y-s.y
	a := -s.rot * math.Pi / 180
	rx := dx*math.Cos(a) - dy*math.Sin(a)
	ry := dx*math.Sin(a) + dy*math.Cos(a)
	return rx / s.scale, ry / s.scale
}

func pick(x, y float64) *shape {
	for i := len(shapes) - 1; i >= 0; i-- {
		lx, ly := shapes[i].local(x, y)
		if iup.DrawPathContains(shapes[i].path, int(lx), int(ly), shapes[i].rule) {
			return shapes[i]
		}
	}
	return nil
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, h := iup.DrawGetSize(ih)
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawSetSourceSolid(ih, "250 250 252")
	iup.DrawRectangle(ih, 0, 0, w, h)

	for _, s := range shapes {
		iup.DrawSave(ih)
		iup.DrawTranslate(ih, s.x, s.y)
		iup.DrawRotate(ih, -s.rot)
		iup.DrawScale(ih, s.scale, s.scale)

		iup.DrawSetPath(ih, s.path)
		iup.DrawSetSourceSolid(ih, s.color)
		iup.DrawPathFill(ih, s.rule)

		if s == hovered || s == picked {
			iup.DrawSetPath(ih, s.path)
			iup.DrawSetSourceSolid(ih, "40 40 60")
			ih.SetAttributes("DRAWSTYLE=STROKE, DRAWLINEWIDTH=3, DRAWLINEJOIN=ROUND")
			if s == picked {
				ih.SetAttribute("DRAWDASH", "8 4")
			}
			iup.DrawPathStroke(ih)
			ih.SetAttribute("DRAWDASH", "")
			ih.SetAttribute("DRAWSTYLE", "FILL")
		}
		iup.DrawRestore(ih)
	}

	iup.DrawSetSourceSolid(ih, "40 40 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 10")
	iup.DrawText(ih, status, 12, h-24, -1, -1)

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func button(ih iup.Ihandle, b, pressed, x, y int, s string) int {
	if b != iup.BUTTON1 {
		return iup.DEFAULT
	}
	if pressed == 1 {
		picked = pick(float64(x), float64(y))
		if picked != nil {
			dragX, dragY = float64(x)-picked.x, float64(y)-picked.y
			status = "picked: " + picked.name
		} else {
			status = "nothing under the pointer"
		}
	} else if picked != nil {
		status = "moved: " + picked.name
		picked = nil
	}
	iup.Update(ih)
	return iup.DEFAULT
}

func motion(ih iup.Ihandle, x, y int, s string) int {
	if picked != nil && iup.IsButton1(s) {
		picked.x, picked.y = float64(x)-dragX, float64(y)-dragY
		iup.Update(ih)
		return iup.DEFAULT
	}

	was := hovered
	hovered = pick(float64(x), float64(y))
	if hovered != was {
		if hovered != nil {
			status = "over: " + hovered.name
			ih.SetAttribute("CURSOR", "HAND")
		} else {
			status = fmt.Sprintf("empty canvas at %d,%d", x, y)
			ih.SetAttribute("CURSOR", "ARROW")
		}
		iup.Update(ih)
	}
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	build()

	cv := iup.Canvas().SetAttributes("RASTERSIZE=560x380, BORDER=NO")
	cv.SetCallback("ACTION", iup.ActionFunc(draw))
	cv.SetCallback("BUTTON_CB", iup.ButtonFunc(button))
	cv.SetCallback("MOTION_CB", iup.MotionFunc(motion))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "Shape Picking")
	iup.Show(dlg)

	iup.MainLoop()
}

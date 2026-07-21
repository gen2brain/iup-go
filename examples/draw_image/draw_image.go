package main

import (
	"fmt"
	"math"

	"github.com/gen2brain/iup-go/iup"
)

var tick int

func card(ih iup.Ihandle, x1, y1, x2, y2 int, title string) {
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "255 255 255 18")
	iup.DrawRoundedRectangle(ih, x1, y1, x2, y2, 12)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWCOLOR", "255 255 255 60")
	iup.DrawRoundedRectangle(ih, x1, y1, x2, y2, 12)
	ih.SetAttribute("DRAWCOLOR", "255 255 255 170")
	ih.SetAttribute("DRAWFONT", "Helvetica, 9")
	iup.DrawText(ih, title, x1+14, y2-22, x2-x1-28, 16)
}

func label(ih iup.Ihandle, text string, x, y, w int) {
	ih.SetAttribute("DRAWCOLOR", "255 255 255 130")
	ih.SetAttribute("DRAWFONT", "Helvetica, 8")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ACENTER")
	iup.DrawText(ih, text, x, y, w, 14)
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, _ := iup.DrawGetSize(ih)

	iup.DrawLinearGradient(ih, 0, 0, 719, 519, 90, "24 26 48", "58 34 78")

	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	ih.SetAttribute("DRAWFONT", "Helvetica, Bold 13")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ACENTER")
	iup.DrawText(ih, "IupDrawImage: SRCRECT, TINT, OPACITY, QUALITY", 0, 14, w, 22)
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")

	card(ih, 20, 50, 350, 270, "DRAWIMAGEQUALITY: LINEAR vs NEAREST upscale")
	iup.DrawImage(ih, "sunset", 40, 90, -1, -1)
	label(ih, "48 px", 30, 142, 70)
	ih.SetAttribute("DRAWIMAGEQUALITY", "LINEAR")
	iup.DrawImage(ih, "sunset", 100, 70, 115, 115)
	label(ih, "LINEAR", 100, 190, 115)
	ih.SetAttribute("DRAWIMAGEQUALITY", "NEAREST")
	iup.DrawImage(ih, "sunset", 225, 70, 115, 115)
	label(ih, "NEAREST", 225, 190, 115)

	card(ih, 370, 50, 700, 270, "DRAWIMAGESRCRECT: animated sprite sheet frames")
	frame := tick % 2
	ih.SetAttribute("DRAWIMAGEQUALITY", "NEAREST")
	iup.DrawImage(ih, "alien", 395, 100, 88, 32)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWCOLOR", "255 220 80")
	iup.DrawRectangle(ih, 395+frame*44-2, 98, 395+frame*44+45, 133)
	label(ih, "sheet 22x8", 395, 142, 88)
	ih.SetAttribute("DRAWIMAGESRCRECT", fmt.Sprintf("%d 0 11 8", frame*11))
	iup.DrawImage(ih, "alien", 525, 75, 143, 104)
	ih.SetAttribute("DRAWIMAGESRCRECT", "")
	label(ih, "one frame, 13x, NEAREST", 510, 190, 170)

	card(ih, 20, 290, 350, 500, "DRAWIMAGETINT: one sprite, any color")
	tints := []string{"", "255 90 90", "255 190 70", "250 250 110", "120 200 255", "220 140 255", "255 255 255", "255 255 255 90"}
	for i, tint := range tints {
		if tint != "" {
			ih.SetAttribute("DRAWIMAGETINT", tint)
		}
		iup.DrawImage(ih, "alien", 42+(i%4)*72, 330+(i/4)*70, 44, 32)
		ih.SetAttribute("DRAWIMAGETINT", "")
	}

	card(ih, 370, 290, 700, 500, "DRAWIMAGEOPACITY: fade cascade and pulse")
	ih.SetAttribute("DRAWIMAGEQUALITY", "LINEAR")
	for i, opacity := range []int{255, 170, 100, 45} {
		ih.SetAttribute("DRAWIMAGEOPACITY", fmt.Sprintf("%d", opacity))
		iup.DrawImage(ih, "sunset", 390+i*58, 315, 80, 80)
	}
	label(ih, "255 .. 45", 390, 400, 222)
	pulse := 40 + int(215*(0.5+0.5*math.Sin(float64(tick)*0.35)))
	ih.SetAttribute("DRAWIMAGEOPACITY", fmt.Sprintf("%d", pulse))
	ih.SetAttribute("DRAWIMAGEQUALITY", "NEAREST")
	iup.DrawImage(ih, "alien", 585, 415, 66, 48)
	ih.SetAttribute("DRAWIMAGEOPACITY", "255")
	label(ih, "opacity pulse", 390, 435, 180)

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetHandle("alien", iup.ImageRGBA(22, 8, makeInvaderSheet()))
	iup.SetHandle("sunset", iup.ImageRGBA(48, 48, makeSunset()))

	cv := iup.Canvas().SetAttributes(`RASTERSIZE=720x520, BORDER=NO`)
	cv.SetCallback("ACTION", iup.ActionFunc(draw))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "IupDrawImage")
	iup.Show(dlg)

	timer := iup.Timer()
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(t iup.Ihandle) int {
		tick++
		iup.Update(cv)
		return iup.DEFAULT
	}))
	timer.SetAttributes("TIME=160, RUN=YES")

	iup.MainLoop()
}

var invader = [2][]string{
	{
		"..X.....X..",
		"...X...X...",
		"..XXXXXXX..",
		".XX.XXX.XX.",
		"XXXXXXXXXXX",
		"X.XXXXXXX.X",
		"X.X.....X.X",
		"...XX.XX...",
	},
	{
		"..X.....X..",
		"X..X...X..X",
		"X.XXXXXXX.X",
		"XXX.XXX.XXX",
		".XXXXXXXXX.",
		"..XXXXXXX..",
		"..X.....X..",
		".X.......X.",
	},
}

func makeInvaderSheet() []byte {
	data := make([]byte, 22*8*4)
	for f := 0; f < 2; f++ {
		for y := 0; y < 8; y++ {
			for x := 0; x < 11; x++ {
				if invader[f][y][x] == 'X' {
					i := (y*22 + f*11 + x) * 4
					data[i+0] = 120
					data[i+1] = 255
					data[i+2] = 160
					data[i+3] = 255
				}
			}
		}
	}
	return data
}

func makeSunset() []byte {
	data := make([]byte, 48*48*4)
	for y := 0; y < 48; y++ {
		for x := 0; x < 48; x++ {
			t := float64(y) / 47
			var r, g, b float64
			if t < 0.5 {
				u := t / 0.5
				r, g, b = 26+u*(178-26), 42+u*(31-42), 108+u*(31-108)
			} else {
				u := (t - 0.5) / 0.5
				r, g, b = 178+u*(253-178), 31+u*(187-31), 31+u*(45-31)
			}
			dx, dy := float64(x-34), float64(y-18)
			if d := math.Sqrt(dx*dx + dy*dy); d < 7 {
				k := 1 - d/7
				r += (255 - r) * k
				g += (250 - g) * k
				b += (210 - b) * k
			}
			if float64(y) > 38+4*math.Sin(float64(x)/5) {
				r, g, b = 24, 18, 34
			}
			i := (y*48 + x) * 4
			data[i+0] = byte(r)
			data[i+1] = byte(g)
			data[i+2] = byte(b)
			data[i+3] = 255
		}
	}
	return data
}

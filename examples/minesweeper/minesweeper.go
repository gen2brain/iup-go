package main

import (
	"bytes"
	_ "embed"
	"fmt"
	"image"
	"image/draw"
	"image/png"
	"log"
	"strconv"

	"github.com/gen2brain/iup-go/iup"
)

//go:embed minesweeper.png
var sheetPNG []byte

const (
	cellPx = 17
	smiley = 26
	digW   = 13
	digH   = 23
	outerM = 10
	panelH = 34
	gap    = 8
)

type difficulty struct {
	name        string
	w, h, mines int
}

var (
	beginner     = difficulty{"Beginner", 9, 9, 10}
	intermediate = difficulty{"Intermediate", 16, 16, 40}
	expert       = difficulty{"Expert", 30, 16, 99}
)

var (
	board        *Board
	cur          = beginner
	cv           iup.Ihandle
	dlg          iup.Ihandle
	timer        iup.Ihandle
	elapsed      int
	started      bool
	mouseDown    bool
	marksEnabled = true
	pressX       = -1
	pressY       = -1
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	loadSheet()

	cv = iup.Canvas().SetAttributes("BORDER=NO, EXPAND=NO")
	cv.SetCallback("ACTION", iup.ActionFunc(render))
	cv.SetCallback("BUTTON_CB", iup.ButtonFunc(button))
	cv.SetCallback("MOTION_CB", iup.MotionFunc(motion))

	timer = iup.Timer().SetAttribute("TIME", "1000")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(tick))

	dlg = iup.Dialog(iup.Vbox(cv)).SetAttributes(`TITLE="Minesweeper", RESIZE=NO, MAXBOX=NO`)
	dlg.SetHandle("dlg")
	iup.SetAttributeHandle(dlg, "MENU", buildMenu())
	dlg.SetCallback("K_ANY", iup.KAnyFunc(func(ih iup.Ihandle, c int) int {
		if c == iup.K_F2 {
			newGame(cur)
			return iup.IGNORE
		}
		return iup.CONTINUE
	}))

	newGame(cur)

	iup.MainLoop()
}

func loadSheet() {
	img, err := png.Decode(bytes.NewReader(sheetPNG))
	if err != nil {
		log.Fatal(err)
	}
	put := func(name string, x, y, w, h int) {
		dst := image.NewRGBA(image.Rect(0, 0, w, h))
		draw.Draw(dst, dst.Bounds(), img, image.Pt(x, y), draw.Src)
		iup.ImageFromImage(dst).SetHandle(name)
	}
	cells := []string{"covered", "num0", "num1", "num2", "num3", "num4", "num5", "num6", "num7", "num8", "flag", "question", "mine", "explode", "wrong"}
	for i, n := range cells {
		put(n, i*cellPx, 0, cellPx, cellPx)
	}
	for i, n := range []string{"sm_smile", "sm_open", "sm_dead", "sm_cool"} {
		put(n, i*smiley, cellPx, smiley, smiley)
	}
	for d := 0; d <= 9; d++ {
		put("led"+strconv.Itoa(d), d*digW, cellPx+smiley, digW, digH)
	}
	put("ledminus", 10*digW, cellPx+smiley, digW, digH)
}

func newGame(d difficulty) {
	cur = d
	board = NewBoard(d.w, d.h, d.mines)
	board.MarksEnabled = marksEnabled
	started, elapsed, mouseDown = false, 0, false
	pressX, pressY = -1, -1
	timer.SetAttribute("RUN", "NO")

	cw := outerM*2 + board.W*cellPx
	ch := outerM*2 + panelH + gap + board.H*cellPx
	cv.SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", cw, ch))
	dlg.SetAttribute("RASTERSIZE", "")
	iup.Refresh(dlg)
	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.Update(cv)
}

func boardOrigin() (int, int)  { return outerM, outerM + panelH + gap }
func smileyOrigin() (int, int) { return outerM + (board.W*cellPx-smiley)/2, outerM + (panelH-smiley)/2 }
func mineOrigin() (int, int)   { return outerM + 4, outerM + (panelH-digH)/2 }
func timerOrigin() (int, int)  { return outerM + board.W*cellPx - 4 - 3*digW, outerM + (panelH-digH)/2 }

func render(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	fill(0, 0, w, h, "192 192 192")

	ox, oy := boardOrigin()
	bevel(ox-3, oy-3, board.W*cellPx+6, board.H*cellPx+6, true)
	for y := 0; y < board.H; y++ {
		for x := 0; x < board.W; x++ {
			iup.DrawImage(ih, cellSprite(x, y), ox+x*cellPx, oy+y*cellPx, cellPx, cellPx)
		}
	}

	mx, my := mineOrigin()
	tx, ty := timerOrigin()
	bevel(mx-2, my-2, 3*digW+4, digH+4, true)
	bevel(tx-2, ty-2, 3*digW+4, digH+4, true)
	drawLED(board.MinesRemaining(), mx, my)
	drawLED(elapsed, tx, ty)

	sx, sy := smileyOrigin()
	bevel(sx-3, sy-3, smiley+6, smiley+6, !mouseDown || pressX < 0)
	iup.DrawImage(ih, smileyName(), sx, sy, smiley, smiley)

	return iup.DEFAULT
}

func fill(x, y, w, h int, color string) {
	cv.SetAttribute("DRAWCOLOR", color)
	cv.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(cv, x, y, x+w-1, y+h-1)
}

func bevel(x, y, w, h int, sunken bool) {
	tl, br := "255 255 255", "128 128 128"
	if sunken {
		tl, br = "128 128 128", "255 255 255"
	}
	fill(x, y, w, 2, tl)
	fill(x, y, 2, h, tl)
	fill(x, y+h-2, w, 2, br)
	fill(x+w-2, y, 2, h, br)
}

func drawLED(v, x, y int) {
	for i, ch := range ledDigits(v) {
		name := "led" + string(ch)
		if ch == '-' {
			name = "ledminus"
		}
		iup.DrawImage(cv, name, x+i*digW, y, digW, digH)
	}
}

func ledDigits(v int) string {
	if v < 0 {
		if v < -99 {
			v = -99
		}
		return fmt.Sprintf("-%02d", -v)
	}
	if v > 999 {
		v = 999
	}
	return fmt.Sprintf("%03d", v)
}

func cellSprite(x, y int) string {
	c := board.At(x, y)
	if board.State == Lost {
		switch {
		case x == board.ExplodedX && y == board.ExplodedY:
			return "explode"
		case c.Mine && c.Mark != MarkFlag:
			return "mine"
		case c.Mark == MarkFlag && !c.Mine:
			return "wrong"
		case c.Mark == MarkFlag:
			return "flag"
		case c.Revealed:
			return "num" + strconv.Itoa(c.Adjacent)
		default:
			return "covered"
		}
	}
	if !c.Revealed {
		switch c.Mark {
		case MarkFlag:
			return "flag"
		case MarkQuestion:
			return "question"
		}
		if x == pressX && y == pressY {
			return "num0"
		}
		return "covered"
	}
	return "num" + strconv.Itoa(c.Adjacent)
}

func smileyName() string {
	switch {
	case board.State == Won:
		return "sm_cool"
	case board.State == Lost:
		return "sm_dead"
	case mouseDown:
		return "sm_open"
	default:
		return "sm_smile"
	}
}

func cellAt(px, py int) (int, int, bool) {
	ox, oy := boardOrigin()
	if px < ox || py < oy {
		return 0, 0, false
	}
	cx, cy := (px-ox)/cellPx, (py-oy)/cellPx
	if cx < 0 || cx >= board.W || cy < 0 || cy >= board.H {
		return 0, 0, false
	}
	return cx, cy, true
}

func inSmiley(px, py int) bool {
	sx, sy := smileyOrigin()
	return px >= sx && px < sx+smiley && py >= sy && py < sy+smiley
}

func button(ih iup.Ihandle, but, pressed, x, y int, status string) int {
	switch but {
	case iup.BUTTON1:
		if inSmiley(x, y) {
			if pressed == 0 {
				newGame(cur)
			}
			return iup.DEFAULT
		}
		if board.State != Playing {
			return iup.DEFAULT
		}
		cx, cy, ok := cellAt(x, y)
		if pressed == 1 {
			mouseDown = true
			if ok && (iup.IsButton3(status) || iup.IsDouble(status)) {
				chord(cx, cy)
			} else if ok {
				pressX, pressY = cx, cy
			}
		} else {
			if ok && cx == pressX && cy == pressY && !iup.IsButton3(status) {
				board.Reveal(cx, cy)
				afterMove()
			}
			mouseDown, pressX, pressY = false, -1, -1
		}
		iup.Update(cv)
	case iup.BUTTON3:
		if board.State != Playing || pressed != 1 {
			return iup.DEFAULT
		}
		if cx, cy, ok := cellAt(x, y); ok {
			if iup.IsButton1(status) {
				chord(cx, cy)
			} else {
				board.ToggleMark(cx, cy)
			}
			iup.Update(cv)
		}
	}
	return iup.DEFAULT
}

func motion(ih iup.Ihandle, x, y int, status string) int {
	if !iup.IsButton1(status) || board.State != Playing {
		return iup.DEFAULT
	}
	nx, ny := -1, -1
	if cx, cy, ok := cellAt(x, y); ok && !board.At(cx, cy).Revealed && board.At(cx, cy).Mark == MarkNone {
		nx, ny = cx, cy
	}
	if nx != pressX || ny != pressY {
		pressX, pressY = nx, ny
		iup.Update(cv)
	}
	return iup.DEFAULT
}

func chord(x, y int) {
	board.Chord(x, y)
	pressX, pressY = -1, -1
	afterMove()
}

func afterMove() {
	if !started && board.placed {
		started, elapsed = true, 0
		timer.SetAttribute("RUN", "YES")
	}
	if board.State != Playing {
		timer.SetAttribute("RUN", "NO")
	}
}

func tick(ih iup.Ihandle) int {
	if board.State == Playing && started {
		if elapsed < 999 {
			elapsed++
		}
		iup.Update(cv)
	}
	return iup.DEFAULT
}

func buildMenu() iup.Ihandle {
	diffItem := func(d difficulty, on bool) iup.Ihandle {
		it := iup.MenuItem(d.name).SetAttribute("AUTOTOGGLE", "YES")
		if on {
			it.SetAttribute("VALUE", "ON")
		}
		return it.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { newGame(d); return iup.DEFAULT }))
	}
	return iup.Menu(
		iup.Submenu("&Game", iup.Menu(
			iup.MenuItem("&New\tF2").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { newGame(cur); return iup.DEFAULT })),
			iup.Separator(),
			iup.Submenu("&Difficulty", iup.Menu(
				diffItem(beginner, true),
				diffItem(intermediate, false),
				diffItem(expert, false),
			).SetAttribute("RADIO", "YES")),
			iup.MenuItem("&Marks (?)").SetAttributes("AUTOTOGGLE=YES, VALUE=ON").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
				marksEnabled = ih.GetAttribute("VALUE") == "ON"
				board.MarksEnabled = marksEnabled
				return iup.DEFAULT
			})),
			iup.Separator(),
			iup.MenuItem("E&xit").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return iup.CLOSE })),
		)),
		iup.Submenu("&Help", iup.Menu(
			iup.MenuItem("&About").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
				iup.Message("About", "Minesweeper\n\nClassic Minesweeper built with IUP-Go.\nLeft-click reveals, right-click flags,\nleft+right (or double-click) chords.")
				return iup.DEFAULT
			})),
		)),
	)
}

//go:build ctrl

package main

import (
	"fmt"
	"math"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const (
	numLin = 24
	numCol = 7
)

type cell struct{ lin, col int }

type kind int

const (
	empty kind = iota
	number
	text
	fail
)

type value struct {
	kind kind
	num  float64
	str  string
}

var (
	sheet   = map[cell]string{}
	marked  = map[cell]bool{}
	cached  = map[cell]value{}
	running = map[cell]bool{}
	current = cell{1, 1}
)

func main() {
	iup.Open()
	iup.ControlsOpen()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	load()

	mat := iup.Matrix().SetHandle("sheet")
	mat.SetAttributes(fmt.Sprintf(`NUMLIN=%d, NUMCOL=%d, NUMLIN_VISIBLE=12, NUMCOL_VISIBLE=%d,
		ALIGNMENT=ARIGHT, ALIGNMENT1=ALEFT, WIDTH0=14, WIDTH1=52,
		USETITLESIZE=YES, RESIZEMATRIX=YES, MARKMODE=CELL, MARKMULTIPLE=YES, EXPAND=YES`, numLin, numCol, numCol))
	for col := 2; col <= numCol; col++ {
		iup.SetAttributeId(mat, "WIDTH", col, "34")
	}

	mat.SetCallback("VALUE_CB", iup.MatrixValueFunc(valueCb))
	mat.SetCallback("VALUE_EDIT_CB", iup.ValueEditFunc(valueEditCb))
	mat.SetCallback("MARK_CB", iup.MarkFunc(markCb))
	mat.SetCallback("MARKEDIT_CB", iup.MarkEditFunc(markEditCb))
	mat.SetCallback("BGCOLOR_CB", iup.BgColorFunc(bgColorCb))
	mat.SetCallback("FGCOLOR_CB", iup.FgColorFunc(fgColorCb))
	mat.SetCallback("ENTERITEM_CB", iup.EnterItemFunc(enterItemCb))

	where := iup.Label("A1").SetAttributes("VISIBLECOLUMNS=5, ALIGNMENT=ACENTER").SetHandle("where")
	formula := iup.Text().SetAttributes("EXPAND=HORIZONTAL").SetHandle("formula")
	formula.SetCallback("K_ANY", iup.KAnyFunc(formulaKey))

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=6x4").SetHandle("status")

	bar := iup.Hbox(where, formula, button("Sum", insertSum), button("Clear", clear)).
		SetAttributes("NGAP=6, ALIGNMENT=ACENTER")

	dlg := iup.Dialog(iup.Vbox(bar, mat, status).SetAttributes("NGAP=6, NMARGIN=8x8"))
	dlg.SetAttribute("TITLE", "Spreadsheet")

	iup.Show(dlg)
	show(current)

	iup.MainLoop()
}

func load() {
	for c, v := range map[cell]string{
		{1, 1}: "Region", {1, 2}: "Q1", {1, 3}: "Q2", {1, 4}: "Q3", {1, 5}: "Q4",
		{1, 6}: "Total", {1, 7}: "Share",

		{2, 1}: "North", {2, 2}: "120", {2, 3}: "135", {2, 4}: "148", {2, 5}: "160",
		{3, 1}: "South", {3, 2}: "98", {3, 3}: "104", {3, 4}: "99", {3, 5}: "121",
		{4, 1}: "East", {4, 2}: "210", {4, 3}: "198", {4, 4}: "225", {4, 5}: "240",
		{5, 1}: "West", {5, 2}: "75", {5, 3}: "88", {5, 4}: "91", {5, 5}: "86",

		{6, 1}: "Total", {6, 2}: "=SUM(B2:B5)", {6, 3}: "=SUM(C2:C5)",
		{6, 4}: "=SUM(D2:D5)", {6, 5}: "=SUM(E2:E5)", {6, 6}: "=SUM(F2:F5)",

		{8, 1}: "Average", {8, 2}: "=AVG(B2:B5)", {8, 3}: "=AVG(C2:C5)",
		{9, 1}: "Best", {9, 2}: "=MAX(F2:F5)",
		{10, 1}: "Growth", {10, 2}: "=(E6-B6)/B6*100",
	} {
		sheet[c] = v
	}
	for lin := 2; lin <= 5; lin++ {
		sheet[cell{lin, 6}] = fmt.Sprintf("=SUM(B%d:E%d)", lin, lin)
		sheet[cell{lin, 7}] = fmt.Sprintf("=F%d/F6*100", lin)
	}
}

func valueCb(ih iup.Ihandle, lin, col int) string {
	switch {
	case lin == 0 && col == 0:
		return ""
	case lin == 0:
		return colName(col)
	case col == 0:
		return strconv.Itoa(lin)
	}
	c := cell{lin, col}
	if ih.GetAttribute("EDITVALUE") != "" {
		return sheet[c]
	}
	return display(c)
}

func valueEditCb(ih iup.Ihandle, lin, col int, newval string) int {
	set(cell{lin, col}, newval)
	ih.SetAttribute("REDRAW", "ALL")
	show(cell{lin, col})
	return iup.DEFAULT
}

func markCb(ih iup.Ihandle, lin, col int) int {
	if marked[cell{lin, col}] {
		return 1
	}
	return 0
}

func markEditCb(ih iup.Ihandle, lin, col, mark int) int {
	marked[cell{lin, col}] = mark == 1
	if !summarize() {
		show(current)
	}
	return iup.DEFAULT
}

func bgColorCb(ih iup.Ihandle, lin, col int) (int, int, int, int) {
	if lin > 0 && col > 0 && (lin == current.lin || col == current.col) {
		r, g, b := highlight(ih)
		return r, g, b, iup.DEFAULT
	}
	return 0, 0, 0, iup.IGNORE
}

func highlight(ih iup.Ihandle) (int, int, int) {
	br, bg, bb := ih.GetRGB("BGCOLOR")
	fr, fg, fb := ih.GetRGB("FGCOLOR")
	mix := func(b, f uint8) int { return int(b) + (int(f)-int(b))*12/100 }
	return mix(br, fr), mix(bg, fg), mix(bb, fb)
}

func fgColorCb(ih iup.Ihandle, lin, col int) (int, int, int, int) {
	if lin == 0 || col == 0 {
		return 0, 0, 0, iup.IGNORE
	}
	c := cell{lin, col}
	v := eval(c)
	switch {
	case v.kind == fail || (v.kind == number && v.num < 0):
		return 220, 60, 60, iup.DEFAULT
	case strings.HasPrefix(sheet[c], "="):
		return 90, 130, 220, iup.DEFAULT
	}
	return 0, 0, 0, iup.IGNORE
}

func enterItemCb(ih iup.Ihandle, lin, col int) int {
	current = cell{lin, col}
	ih.SetAttribute("REDRAW", "ALL")
	show(current)
	return iup.DEFAULT
}

func formulaKey(ih iup.Ihandle, c int) int {
	if c != iup.K_CR {
		return iup.DEFAULT
	}
	set(current, ih.GetAttribute("VALUE"))
	mat := iup.GetHandle("sheet")
	mat.SetAttribute("REDRAW", "ALL")
	iup.SetFocus(mat)
	show(current)
	return iup.DEFAULT
}

func insertSum() {
	first, last, count := bounds()
	if count == 0 {
		setStatus("Select the cells to add up first")
		return
	}
	at := cell{last.lin + 1, first.col}
	if at.lin > numLin {
		at = current
	}

	set(at, fmt.Sprintf("=SUM(%s:%s)", name(first), name(last)))
	iup.GetHandle("sheet").SetAttribute("REDRAW", "ALL")
	show(at)
}

func clear() {
	_, _, count := bounds()
	if count == 0 {
		set(current, "")
	}
	for c, on := range marked {
		if on {
			set(c, "")
		}
	}
	iup.GetHandle("sheet").SetAttribute("REDRAW", "ALL")
	show(current)
}

func bounds() (cell, cell, int) {
	first, last, count := cell{numLin, numCol}, cell{1, 1}, 0
	for c, on := range marked {
		if !on {
			continue
		}
		count++
		first.lin, first.col = min(first.lin, c.lin), min(first.col, c.col)
		last.lin, last.col = max(last.lin, c.lin), max(last.col, c.col)
	}
	return first, last, count
}

func summarize() bool {
	sum, count := 0.0, 0
	for c, on := range marked {
		if !on {
			continue
		}
		if v := eval(c); v.kind == number {
			sum += v.num
			count++
		}
	}
	if count < 2 {
		return false
	}

	setStatus(fmt.Sprintf("Sum %s   Count %d   Average %s", format(sum), count, format(sum/float64(count))))
	return true
}

func show(c cell) {
	iup.GetHandle("where").SetAttribute("TITLE", name(c))
	iup.GetHandle("formula").SetAttribute("VALUE", sheet[c])
	if summarize() {
		return
	}
	if raw := sheet[c]; strings.HasPrefix(raw, "=") {
		setStatus(name(c) + " " + raw + " = " + display(c))
		return
	}
	setStatus("Type a value, or =SUM(B2:B5)")
}

func setStatus(s string) {
	iup.GetHandle("status").SetAttribute("TITLE", s)
}

func set(c cell, raw string) {
	if raw == "" {
		delete(sheet, c)
	} else {
		sheet[c] = raw
	}
	cached = map[cell]value{}
}

func name(c cell) string {
	return colName(c.col) + strconv.Itoa(c.lin)
}

func colName(col int) string {
	name := ""
	for col > 0 {
		col--
		name = string(rune('A'+col%26)) + name
		col /= 26
	}
	return name
}

func display(c cell) string {
	v := eval(c)
	switch v.kind {
	case number:
		return format(v.num)
	case text, fail:
		return v.str
	}
	return ""
}

func format(n float64) string {
	if n == math.Trunc(n) && math.Abs(n) < 1e15 {
		return strconv.FormatFloat(n, 'f', 0, 64)
	}
	return strconv.FormatFloat(n, 'f', 2, 64)
}

func eval(c cell) value {
	if v, ok := cached[c]; ok {
		return v
	}
	if running[c] {
		return value{kind: fail, str: "#CYCLE"}
	}

	raw := strings.TrimSpace(sheet[c])
	var v value
	switch {
	case raw == "":
	case strings.HasPrefix(raw, "="):
		running[c] = true
		v = formula(raw[1:])
		delete(running, c)
	default:
		if n, err := strconv.ParseFloat(raw, 64); err == nil {
			v = value{kind: number, num: n}
		} else {
			v = value{kind: text, str: raw}
		}
	}

	cached[c] = v
	return v
}

type parser struct {
	src []rune
	pos int
	bad bool
}

func formula(src string) value {
	p := &parser{src: []rune(strings.ToUpper(src))}
	v := p.expr()
	p.space()
	if p.bad || p.pos != len(p.src) {
		return value{kind: fail, str: "#ERROR"}
	}
	return v
}

func (p *parser) space() {
	for p.pos < len(p.src) && p.src[p.pos] == ' ' {
		p.pos++
	}
}

func (p *parser) peek() rune {
	if p.pos < len(p.src) {
		return p.src[p.pos]
	}
	return 0
}

func (p *parser) expr() value {
	v := p.term()
	for {
		p.space()
		op := p.peek()
		if op != '+' && op != '-' {
			return v
		}
		p.pos++
		v = arith(v, p.term(), op)
	}
}

func (p *parser) term() value {
	v := p.factor()
	for {
		p.space()
		op := p.peek()
		if op != '*' && op != '/' {
			return v
		}
		p.pos++
		v = arith(v, p.factor(), op)
	}
}

func (p *parser) factor() value {
	p.space()
	switch c := p.peek(); {
	case c == '-':
		p.pos++
		return arith(value{kind: number}, p.factor(), '-')
	case c == '+':
		p.pos++
		return p.factor()
	case c == '(':
		p.pos++
		v := p.expr()
		p.space()
		if p.peek() != ')' {
			p.bad = true
			return value{kind: fail, str: "#ERROR"}
		}
		p.pos++
		return v
	case c >= '0' && c <= '9', c == '.':
		return p.literal()
	case c >= 'A' && c <= 'Z':
		return p.word()
	}
	p.bad = true
	return value{kind: fail, str: "#ERROR"}
}

func (p *parser) literal() value {
	start := p.pos
	for p.pos < len(p.src) && (p.src[p.pos] >= '0' && p.src[p.pos] <= '9' || p.src[p.pos] == '.') {
		p.pos++
	}
	n, err := strconv.ParseFloat(string(p.src[start:p.pos]), 64)
	if err != nil {
		p.bad = true
		return value{kind: fail, str: "#ERROR"}
	}
	return value{kind: number, num: n}
}

func (p *parser) word() value {
	start := p.pos
	if c, ok := p.ref(); ok {
		if c.lin > numLin || c.col > numCol {
			return value{kind: fail, str: "#REF"}
		}
		return eval(c)
	}

	p.pos = start
	for p.pos < len(p.src) && p.src[p.pos] >= 'A' && p.src[p.pos] <= 'Z' {
		p.pos++
	}
	fn := string(p.src[start:p.pos])
	p.space()
	if p.peek() != '(' {
		p.bad = true
		return value{kind: fail, str: "#NAME"}
	}
	p.pos++

	nums, v := p.arguments()
	if v.kind == fail {
		return v
	}
	if p.peek() != ')' {
		p.bad = true
		return value{kind: fail, str: "#ERROR"}
	}
	p.pos++

	return apply(fn, nums)
}

func (p *parser) arguments() ([]float64, value) {
	var nums []float64
	for {
		p.space()
		start := p.pos
		if !p.pushRange(&nums) {
			p.pos = start
			v := p.expr()
			if v.kind == fail {
				return nil, v
			}
			n, ok := numeric(v)
			if !ok {
				return nil, value{kind: fail, str: "#VALUE"}
			}
			nums = append(nums, n)
		}
		p.space()
		if p.peek() != ',' && p.peek() != ';' {
			return nums, value{}
		}
		p.pos++
	}
}

func (p *parser) pushRange(nums *[]float64) bool {
	from, ok := p.ref()
	if !ok {
		return false
	}
	p.space()
	if p.peek() != ':' {
		return false
	}
	p.pos++
	p.space()
	to, ok := p.ref()
	if !ok {
		return false
	}

	for lin := min(from.lin, to.lin); lin <= max(from.lin, to.lin); lin++ {
		for col := min(from.col, to.col); col <= max(from.col, to.col); col++ {
			if v := eval(cell{lin, col}); v.kind == number {
				*nums = append(*nums, v.num)
			}
		}
	}
	return true
}

func (p *parser) ref() (cell, bool) {
	i, col := p.pos, 0
	for i < len(p.src) && p.src[i] >= 'A' && p.src[i] <= 'Z' {
		col = col*26 + int(p.src[i]-'A') + 1
		i++
	}
	if col == 0 {
		return cell{}, false
	}

	digits, lin := i, 0
	for i < len(p.src) && p.src[i] >= '0' && p.src[i] <= '9' {
		lin = lin*10 + int(p.src[i]-'0')
		i++
	}
	if i == digits || lin == 0 {
		return cell{}, false
	}

	p.pos = i
	return cell{lin, col}, true
}

func apply(fn string, nums []float64) value {
	if len(nums) == 0 && fn != "COUNT" {
		return value{kind: fail, str: "#VALUE"}
	}
	switch fn {
	case "SUM", "AVG", "AVERAGE":
		total := 0.0
		for _, n := range nums {
			total += n
		}
		if fn == "SUM" {
			return value{kind: number, num: total}
		}
		return value{kind: number, num: total / float64(len(nums))}
	case "MIN", "MAX":
		best := nums[0]
		for _, n := range nums[1:] {
			if fn == "MIN" && n < best || fn == "MAX" && n > best {
				best = n
			}
		}
		return value{kind: number, num: best}
	case "COUNT":
		return value{kind: number, num: float64(len(nums))}
	}
	return value{kind: fail, str: "#NAME"}
}

func arith(a, b value, op rune) value {
	if a.kind == fail {
		return a
	}
	if b.kind == fail {
		return b
	}

	x, ok := numeric(a)
	y, ok2 := numeric(b)
	if !ok || !ok2 {
		return value{kind: fail, str: "#VALUE"}
	}

	switch op {
	case '+':
		return value{kind: number, num: x + y}
	case '-':
		return value{kind: number, num: x - y}
	case '*':
		return value{kind: number, num: x * y}
	case '/':
		if y == 0 {
			return value{kind: fail, str: "#DIV/0"}
		}
		return value{kind: number, num: x / y}
	}
	return value{kind: fail, str: "#ERROR"}
}

func numeric(v value) (float64, bool) {
	switch v.kind {
	case empty:
		return 0, true
	case number:
		return v.num, true
	}
	return 0, false
}

func button(title string, action func()) iup.Ihandle {
	return iup.Button(title).SetAttributes("PADDING=10x4").
		SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			action()
			return iup.DEFAULT
		}))
}

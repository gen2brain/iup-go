package main

import (
	"fmt"
	"math"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const visible = 16

type row struct {
	mark  byte
	left  string
	right string
}

var (
	rows  []row
	hunks []int
	top   int
)

const before = `package store

import "errors"

type Store struct {
	items map[string]int
}

func New() *Store {
	return &Store{items: make(map[string]int)}
}

func (s *Store) Get(key string) (int, error) {
	v, ok := s.items[key]
	if !ok {
		return 0, errors.New("not found")
	}
	return v, nil
}

func (s *Store) Put(key string, value int) {
	s.items[key] = value
}

func (s *Store) Len() int {
	return len(s.items)
}`

const after = `package store

import (
	"errors"
	"sync"
)

var ErrNotFound = errors.New("not found")

type Store struct {
	mu    sync.RWMutex
	items map[string]int
}

func New() *Store {
	return &Store{items: make(map[string]int)}
}

func (s *Store) Get(key string) (int, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	v, ok := s.items[key]
	if !ok {
		return 0, ErrNotFound
	}
	return v, nil
}

func (s *Store) Put(key string, value int) {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.items[key] = value
}

func (s *Store) Len() int {
	s.mu.RLock()
	defer s.mu.RUnlock()

	return len(s.items)
}`

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	compare(strings.Split(before, "\n"), strings.Split(after, "\n"))

	left := panelOf("store.go", "left")
	right := panelOf("store.go (working copy)", "right")

	bar := iup.Scrollbar("VERTICAL").SetHandle("bar")
	bar.SetAttributes(fmt.Sprintf("MIN=0, MAX=%d, VALUE=0, PAGESIZE=%d, LINESTEP=%f, PAGESTEP=%f, EXPAND=VERTICAL",
		len(rows), visible, 1.0/float64(len(rows)), float64(visible)/float64(len(rows))))
	bar.SetCallback("SCROLL_CB", iup.ScrollFunc(scrolled))
	bar.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		goTo(round(float64(ih.GetFloat("VALUE"))))
		return iup.DEFAULT
	}))

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=8x4").SetHandle("status")

	buttons := iup.Hbox(
		button("Previous change", func() { jump(-1) }),
		button("Next change", func() { jump(1) }),
		iup.Fill(),
		iup.Label(fmt.Sprintf("%d changed lines", len(hunks))),
	).SetAttributes("NGAP=6, ALIGNMENT=ACENTER")

	dlg := iup.Dialog(iup.Vbox(
		iup.Hbox(left, right, bar).SetAttributes("NGAP=6"),
		buttons,
		status,
	).SetAttributes("NGAP=6, NMARGIN=8x8")).SetHandle("diffviewer")
	dlg.SetAttribute("TITLE", "Diff Viewer")

	iup.Show(dlg)

	fill()
	goTo(0)
	setStatus("Scroll with the bar on the right, both sides follow")

	iup.MainLoop()
}

func panelOf(title, name string) iup.Ihandle {
	columns := 34
	if phone() {
		columns = 18
	}

	text := iup.Text().SetAttributes(fmt.Sprintf(
		"MULTILINE=YES, READONLY=YES, SCROLLBAR=YES, CANFOCUS=NO, VISIBLELINES=%d, VISIBLECOLUMNS=%d, EXPAND=YES",
		visible, columns)).SetHandle(name)
	text.SetAttribute("FONT", "Courier, 10")

	return iup.Vbox(
		iup.Label(title).SetAttributes("FONTSTYLE=Bold, PADDING=2x2"),
		text,
	).SetAttributes("NGAP=4")
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

func compare(old, new []string) {
	common := make([][]int, len(old)+1)
	for i := range common {
		common[i] = make([]int, len(new)+1)
	}
	for i := len(old) - 1; i >= 0; i-- {
		for j := len(new) - 1; j >= 0; j-- {
			if old[i] == new[j] {
				common[i][j] = common[i+1][j+1] + 1
			} else if common[i+1][j] >= common[i][j+1] {
				common[i][j] = common[i+1][j]
			} else {
				common[i][j] = common[i][j+1]
			}
		}
	}

	i, j := 0, 0
	for i < len(old) && j < len(new) {
		switch {
		case old[i] == new[j]:
			rows = append(rows, row{' ', old[i], new[j]})
			i, j = i+1, j+1
		case common[i+1][j] >= common[i][j+1]:
			rows = append(rows, row{'-', old[i], ""})
			i++
		default:
			rows = append(rows, row{'+', "", new[j]})
			j++
		}
	}
	for ; i < len(old); i++ {
		rows = append(rows, row{'-', old[i], ""})
	}
	for ; j < len(new); j++ {
		rows = append(rows, row{'+', "", new[j]})
	}

	for at, r := range rows {
		if r.mark != ' ' && (at == 0 || rows[at-1].mark == ' ') {
			hunks = append(hunks, at)
		}
	}
}

func fill() {
	var left, right strings.Builder
	for _, r := range rows {
		if r.mark == '-' {
			fmt.Fprintf(&left, "- %s\n", r.left)
			right.WriteString("\n")
			continue
		}
		if r.mark == '+' {
			left.WriteString("\n")
			fmt.Fprintf(&right, "+ %s\n", r.right)
			continue
		}
		fmt.Fprintf(&left, "  %s\n", r.left)
		fmt.Fprintf(&right, "  %s\n", r.right)
	}

	iup.GetHandle("left").SetAttribute("VALUE", left.String())
	iup.GetHandle("right").SetAttribute("VALUE", right.String())
}

func scrolled(ih iup.Ihandle, op int, posx, posy float64) int {
	goTo(round(posy))
	setStatus(fmt.Sprintf("SCROLL_CB %s at %.0f, top line %d", opName(op), posy, top+1))
	return iup.DEFAULT
}

func round(pos float64) int { return int(math.Round(pos)) }

func goTo(line int) {
	if line > len(rows)-visible {
		line = len(rows) - visible
	}
	if line < 0 {
		line = 0
	}
	top = line

	for _, name := range []string{"left", "right"} {
		text := iup.GetHandle(name)
		text.SetAttribute("SCROLLTO", fmt.Sprintf("%d,1", min(top+visible, len(rows))))
		text.SetAttribute("SCROLLTO", fmt.Sprintf("%d,1", top+1))
	}
}

func jump(dir int) {
	target := -1
	for _, at := range hunks {
		if dir > 0 && at > top {
			target = at
			break
		}
		if dir < 0 && at < top {
			target = at
		}
	}
	if target < 0 {
		setStatus("No more changes in that direction")
		return
	}

	iup.GetHandle("bar").SetAttribute("VALUE", target)
	goTo(target)
	setStatus(fmt.Sprintf("Change at line %d", target+1))
}

func opName(op int) string {
	switch op {
	case iup.SBUP:
		return "SBUP"
	case iup.SBDN:
		return "SBDN"
	case iup.SBPGUP:
		return "SBPGUP"
	case iup.SBPGDN:
		return "SBPGDN"
	case iup.SBPOSV:
		return "SBPOSV"
	case iup.SBDRAGV:
		return "SBDRAGV"
	}
	return fmt.Sprintf("op %d", op)
}

func setStatus(s string) {
	iup.GetHandle("status").SetAttribute("TITLE", s)
}

func button(title string, action func()) iup.Ihandle {
	return iup.Button(title).SetAttributes("PADDING=10x4").
		SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			action()
			return iup.DEFAULT
		}))
}

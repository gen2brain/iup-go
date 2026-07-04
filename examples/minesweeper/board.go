package main

import "math/rand"

// Classic Minesweeper logic.

type Mark int

const (
	MarkNone Mark = iota
	MarkFlag
	MarkQuestion
)

type State int

const (
	Playing State = iota
	Won
	Lost
)

type Cell struct {
	Mine     bool
	Revealed bool
	Mark     Mark
	Adjacent int
}

type Board struct {
	W, H, Mines  int
	Cells        []Cell
	State        State
	MarksEnabled bool
	ExplodedX    int // clicked mine on loss, else -1
	ExplodedY    int

	placed    bool
	revealedN int
}

func NewBoard(w, h, mines int) *Board {
	if mines > w*h-9 {
		mines = w*h - 9
	}
	return &Board{
		W: w, H: h, Mines: mines,
		Cells:     make([]Cell, w*h),
		State:     Playing,
		ExplodedX: -1, ExplodedY: -1,
		MarksEnabled: true,
	}
}

func (b *Board) idx(x, y int) int       { return y*b.W + x }
func (b *Board) At(x, y int) *Cell      { return &b.Cells[b.idx(x, y)] }
func (b *Board) inBounds(x, y int) bool { return x >= 0 && x < b.W && y >= 0 && y < b.H }

func (b *Board) forNeighbors(x, y int, fn func(nx, ny int)) {
	for dy := -1; dy <= 1; dy++ {
		for dx := -1; dx <= 1; dx++ {
			if dx == 0 && dy == 0 {
				continue
			}
			nx, ny := x+dx, y+dy
			if b.inBounds(nx, ny) {
				fn(nx, ny)
			}
		}
	}
}

// Mines are placed on the first reveal, skipping the clicked cell and its neighbors (safe first click).
func (b *Board) placeMines(safeX, safeY int) {
	safe := map[int]bool{b.idx(safeX, safeY): true}
	b.forNeighbors(safeX, safeY, func(nx, ny int) { safe[b.idx(nx, ny)] = true })

	spots := make([]int, 0, len(b.Cells))
	for i := range b.Cells {
		if !safe[i] {
			spots = append(spots, i)
		}
	}
	rand.Shuffle(len(spots), func(i, j int) { spots[i], spots[j] = spots[j], spots[i] })
	for i := 0; i < b.Mines && i < len(spots); i++ {
		b.Cells[spots[i]].Mine = true
	}
	for y := 0; y < b.H; y++ {
		for x := 0; x < b.W; x++ {
			n := 0
			b.forNeighbors(x, y, func(nx, ny int) {
				if b.At(nx, ny).Mine {
					n++
				}
			})
			b.At(x, y).Adjacent = n
		}
	}
	b.placed = true
}

func (b *Board) Reveal(x, y int) {
	if b.State != Playing || !b.inBounds(x, y) {
		return
	}
	if !b.placed {
		b.placeMines(x, y)
	}
	c := b.At(x, y)
	if c.Revealed || c.Mark == MarkFlag {
		return
	}
	if c.Mine {
		c.Revealed = true
		b.ExplodedX, b.ExplodedY = x, y
		b.lose()
		return
	}
	b.floodReveal(x, y)
	b.checkWin()
}

func (b *Board) floodReveal(x, y int) {
	stack := []int{b.idx(x, y)}
	for len(stack) > 0 {
		i := stack[len(stack)-1]
		stack = stack[:len(stack)-1]
		c := &b.Cells[i]
		if c.Revealed || c.Mine || c.Mark == MarkFlag {
			continue
		}
		c.Revealed = true
		b.revealedN++
		if c.Adjacent == 0 {
			cx, cy := i%b.W, i/b.W
			b.forNeighbors(cx, cy, func(nx, ny int) {
				n := b.At(nx, ny)
				if !n.Revealed && !n.Mine {
					stack = append(stack, b.idx(nx, ny))
				}
			})
		}
	}
}

func (b *Board) ToggleMark(x, y int) {
	if b.State != Playing || !b.inBounds(x, y) {
		return
	}
	c := b.At(x, y)
	if c.Revealed {
		return
	}
	switch c.Mark {
	case MarkNone:
		c.Mark = MarkFlag
	case MarkFlag:
		if b.MarksEnabled {
			c.Mark = MarkQuestion
		} else {
			c.Mark = MarkNone
		}
	default:
		c.Mark = MarkNone
	}
}

// Chord reveals a number's neighbors when its flag count matches.
func (b *Board) Chord(x, y int) {
	if b.State != Playing || !b.inBounds(x, y) {
		return
	}
	c := b.At(x, y)
	if !c.Revealed || c.Adjacent == 0 {
		return
	}
	flags := 0
	b.forNeighbors(x, y, func(nx, ny int) {
		if b.At(nx, ny).Mark == MarkFlag {
			flags++
		}
	})
	if flags != c.Adjacent {
		return
	}
	var targets []int
	b.forNeighbors(x, y, func(nx, ny int) {
		n := b.At(nx, ny)
		if !n.Revealed && n.Mark != MarkFlag {
			targets = append(targets, b.idx(nx, ny))
		}
	})
	for _, i := range targets {
		if b.State != Playing {
			break
		}
		b.Reveal(i%b.W, i/b.W)
	}
}

func (b *Board) MinesRemaining() int {
	flags := 0
	for i := range b.Cells {
		if b.Cells[i].Mark == MarkFlag {
			flags++
		}
	}
	return b.Mines - flags
}

func (b *Board) lose() {
	b.State = Lost
	for i := range b.Cells {
		if b.Cells[i].Mine {
			b.Cells[i].Revealed = true
		}
	}
}

func (b *Board) checkWin() {
	if b.revealedN == b.W*b.H-b.Mines {
		b.State = Won
		for i := range b.Cells {
			if b.Cells[i].Mine {
				b.Cells[i].Mark = MarkFlag
			}
		}
	}
}

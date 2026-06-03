//go:build js && wasm

// Package iup WebAssembly binding. Instead of cgo, it drives an
// Emscripten-compiled IUP module (exposed on the global object as IupModule)
// through syscall/js. The C API is reached via Module.ccall; callbacks cross
// back C -> JS -> Go through the global iupGoDispatch function.
package iup

import (
	"sync"
	"syscall/js"
)

// Ihandle is an opaque handle to an IUP element. Under WebAssembly it holds the
// element's pointer within the IUP module's address space, as a plain number.
type Ihandle uintptr

var iupMod js.Value

func module() js.Value {
	if iupMod.IsUndefined() {
		iupMod = js.Global().Get("IupModule")
		if iupMod.IsUndefined() || iupMod.IsNull() {
			panic("iup: IupModule is not loaded; the Emscripten IUP module must be instantiated before the Go program runs")
		}
	}
	return iupMod
}

// ccall invokes Module.ccall. ret "" means a void return (JS null).
func ccall(name, ret string, argTypes, args []interface{}) js.Value {
	var retArg interface{} = ret
	if ret == "" {
		retArg = nil
	}
	return module().Call("ccall", name, retArg, js.ValueOf(argTypes), js.ValueOf(args))
}

func ccallHandle(name string, argTypes, args []interface{}) Ihandle {
	return Ihandle(ccall(name, "number", argTypes, args).Int())
}

// --- module heap helpers (for out-params and int arrays) ------------------

func wasmMalloc(n int) int { return module().Call("_malloc", n).Int() }
func wasmFree(ptr int)     { module().Call("_free", ptr) }

func wasmGetI32(ptr int) int { return module().Call("getValue", ptr, "i32").Int() }
func wasmSetI32(ptr, v int)  { module().Call("setValue", ptr, v, "i32") }

// --- callback registry and dispatch --------------------------------------

type cbKey struct {
	ih   Ihandle
	name string
}

var (
	callbacks = make(map[cbKey]interface{})
	exitCh    chan struct{}
	exitOnce  sync.Once
)

// removeCallbacks drops the map entries for ih and its subtree. IupDestroy frees
// the subtree and the Ihandle pointers may later be reused, so stale entries must go.
func removeCallbacks(ih Ihandle) {
	victims := map[Ihandle]bool{ih: true}
	var walk func(Ihandle)
	walk = func(p Ihandle) {
		for i, n := 0, GetChildCount(p); i < n; i++ {
			if c := GetChild(p, i); c != 0 && !victims[c] {
				victims[c] = true
				walk(c)
			}
		}
	}
	walk(ih)
	for k := range callbacks {
		if victims[k.ih] {
			delete(callbacks, k)
		}
	}
}

func init() {
	js.Global().Set("iupGoDispatch", js.FuncOf(func(this js.Value, args []js.Value) interface{} {
		ih := Ihandle(args[0].Int())
		name := args[1].String()
		i1 := args[2].Int()
		i2 := args[3].Int()
		i3 := args[4].Int()
		i4 := args[5].Int()
		sarg := args[6].String()
		return dispatch(ih, name, i1, i2, i3, i4, sarg)
	}))
	js.Global().Set("iupGoDispatchF", js.FuncOf(func(this js.Value, args []js.Value) interface{} {
		return dispatchF(Ihandle(args[0].Int()), args[1].String(), args[2].Int(), args[3].Float(), args[4].Float())
	}))
	js.Global().Set("iupGoDispatch6", js.FuncOf(func(this js.Value, args []js.Value) interface{} {
		return dispatch6(Ihandle(args[0].Int()), args[1].String(),
			args[2].Int(), args[3].Int(), args[4].Int(), args[5].Int(), args[6].Int(), args[7].Int())
	}))
	js.Global().Set("iupGoDispatchStr", js.FuncOf(func(this js.Value, args []js.Value) interface{} {
		return dispatchStr(Ihandle(args[0].Int()), args[1].String(), args[2].Int(), args[3].Int())
	}))
	js.Global().Set("iupGoDispatchGesture", js.FuncOf(func(this js.Value, args []js.Value) interface{} {
		return dispatchGesture(Ihandle(args[0].Int()), args[1].Int(), args[2].Int(), args[3].Int(), args[4].Int(), args[5].Float(), args[6].Float())
	}))
	js.Global().Set("iupGoIdleDispatch", js.FuncOf(func(js.Value, []js.Value) interface{} {
		if fn, ok := globalFuncs["IDLE_ACTION"].(IdleFunc); ok && fn != nil {
			return fn()
		}
		return IGNORE
	}))
	js.Global().Set("iupGoExitLoop", js.FuncOf(func(js.Value, []js.Value) interface{} {
		exitOnce.Do(func() {
			if exitCh != nil {
				close(exitCh)
			}
		})
		return nil
	}))
}

// dispatchStr handles callbacks that return a string (table VALUE_CB/IMAGE_CB).
func dispatchStr(ih Ihandle, name string, i1, i2 int) string {
	fn := callbacks[cbKey{ih, name}]
	if fn == nil {
		return ""
	}
	switch f := fn.(type) {
	case TableValueFunc:
		return f(ih, i1, i2)
	case TableImageFunc:
		return f(ih, i1, i2)
	case ListValueFunc:
		return f(ih, i1)
	case ListImageFunc:
		return f(ih, i1)
	}
	return ""
}

func dispatch6(ih Ihandle, name string, i1, i2, i3, i4, i5, i6 int) int {
	fn := callbacks[cbKey{ih, name}]
	if fn == nil {
		return DEFAULT
	}
	if f, ok := fn.(CellsDrawFunc); ok {
		return f(ih, i1, i2, i3, i4, i5, i6)
	}
	return DEFAULT
}

func dispatchF(ih Ihandle, name string, op int, posx, posy float64) int {
	fn := callbacks[cbKey{ih, name}]
	if fn == nil {
		return DEFAULT
	}
	if f, ok := fn.(ScrollFunc); ok {
		return f(ih, op, posx, posy)
	}
	return DEFAULT
}

func dispatchGesture(ih Ihandle, gesture, state, x, y int, v1, v2 float64) int {
	if f, ok := callbacks[cbKey{ih, "GESTURE_CB"}].(GestureFunc); ok {
		return f(ih, gesture, state, x, y, v1, v2)
	}
	return DEFAULT
}

func dispatch(ih Ihandle, name string, i1, i2, i3, i4 int, sarg string) int {
	if name == "GETPARAM_ACTION_CB" {
		if curGetParamAction != nil {
			return curGetParamAction(ih, i1)
		}
		return DEFAULT
	}

	fn := callbacks[cbKey{ih, name}]
	if fn == nil {
		return DEFAULT
	}

	ret := DEFAULT
	switch f := fn.(type) {
	case ActionFunc:
		ret = f(ih)
	case func(Ihandle) int:
		ret = f(ih)
	case ToggleActionFunc:
		ret = f(ih, i1)
	case KAnyFunc:
		ret = f(ih, i1)
	case TextActionFunc:
		ret = f(ih, i1, sarg)
	case ListActionFunc:
		ret = f(ih, sarg, i1, i2)
	case LinkActionFunc:
		ret = f(ih, sarg)
	case ChangeFunc:
		ret = f(ih, uint8(i1), uint8(i2), uint8(i3))
	case DragFunc:
		ret = f(ih, uint8(i1), uint8(i2), uint8(i3))
	case DblclickFunc:
		ret = f(ih, i1, sarg)
	case MultiselectFunc:
		ret = f(ih, sarg)
	case CaretFunc:
		ret = f(ih, i1, i2, i3)
	case ValueChangedFunc:
		ret = f(ih)
	case TimerActionFunc:
		ret = f(ih)
	case ResizeFunc:
		ret = f(ih, i1, i2)
	case MotionFunc:
		ret = f(ih, i1, i2, sarg)
	case EnterWindowFunc:
		ret = f(ih)
	case LeaveWindowFunc:
		ret = f(ih)
	case HelpFunc:
		ret = f(ih)
	case WheelFunc:
		ret = f(ih, float32(i1), i2, i3, sarg)
	case CloseFunc:
		ret = f(ih)
	case NotifyFunc:
		ret = f(ih, i1)
	case NotifyCloseFunc:
		ret = f(ih, i1)
	case ErrorFunc:
		ret = f(ih, sarg)
	case CompletedFunc:
		ret = f(ih, sarg)
	case NavigateFunc:
		ret = f(ih, sarg)
	case NewWindowFunc:
		ret = f(ih, sarg)
	case UpdateFunc:
		ret = f(ih)
	case ClickFunc:
		ret = f(ih, i1, i2, sarg)
	case EnterItemFunc:
		ret = f(ih, i1, i2)
	case TableEditionFunc:
		ret = f(ih, i1, i2, sarg)
	case TableValueChangedFunc:
		ret = f(ih, i1, i2)
	case TableSortFunc:
		ret = f(ih, i1)
	case ThemeChangedFunc:
		ret = f(ih, i1)
	case ShowFunc:
		ret = f(ih, i1)
	case DragDropFunc:
		ret = f(ih, i1, i2, i3, i4)
	case DragBeginFunc:
		ret = f(ih, i1, i2)
	case DragEndFunc:
		ret = f(ih, i1)
	case DropMotionFunc:
		ret = f(ih, i1, i2, sarg)
	case DropFilesFunc:
		ret = f(ih, sarg, i1, i2, i3)
	case DropDataFunc:
		// the payload lives in the module heap, not Go's address space
		ret = f(ih, sarg, nil, i2, i3, i4)
	case SelectionFunc:
		ret = f(ih, i1, i2)
	case BranchOpenFunc:
		ret = f(ih, i1)
	case BranchCloseFunc:
		ret = f(ih, i1)
	case ExecuteLeafFunc:
		ret = f(ih, i1)
	case ExecuteBranchFunc:
		ret = f(ih, i1)
	case ShowRenameFunc:
		ret = f(ih, i1)
	case RenameFunc:
		ret = f(ih, i1, sarg)
	case NodeRemovedFunc:
		ret = f(ih, uintptr(uint32(i1)))
	case MultiSelectionFunc:
		ids := parseIDList(sarg)
		ret = f(ih, ids, i1)
	case MultiUnselectionFunc:
		ids := parseIDList(sarg)
		ret = f(ih, ids, i1)
	case MapFunc:
		ret = f(ih)
	case UnmapFunc:
		ret = f(ih)
	case HighlightFunc:
		ret = f(ih)
	case MenuOpenFunc:
		ret = f(ih)
	case MenuCloseFunc:
		ret = f(ih)
	case GetFocusFunc:
		ret = f(ih)
	case KillFocusFunc:
		ret = f(ih)
	case DropDownFunc:
		ret = f(ih, i1)
	case EditFunc:
		ret = f(ih, i1, sarg)
	case ButtonFunc:
		ret = f(ih, i1, i2, i3, i4, sarg)
	case TabChangeFunc:
		ret = f(ih, Ihandle(i1), Ihandle(i2))
	case TabChangePosFunc:
		ret = f(ih, i1, i2)
	case ReorderFunc:
		ret = f(ih, i1, i2)
	case TabCloseFunc:
		ret = f(ih, i1)
	case RightClickFunc:
		ret = f(ih, i1)
	case SpinFunc:
		ret = f(ih, i1)
	case NLinesFunc:
		ret = f(ih)
	case NColsFunc:
		ret = f(ih)
	case WidthFunc:
		ret = f(ih, i1)
	case HeightFunc:
		ret = f(ih, i1)
	case TouchFunc:
		ret = f(ih, i1, i2, i3, sarg)
	case MultiTouchFunc:
		pid, px, py, pstate := parseTouchPoints(sarg)
		ret = f(ih, i1, pid, px, py, pstate)
	case func(Ihandle, int) int:
		ret = f(ih, i1)
	case func(Ihandle, int, string) int:
		ret = f(ih, i1, sarg)
	}

	return ret
}

// parseTouchPoints decodes the "id,x,y,state;..." serialization MULTITOUCH_CB carries.
func parseTouchPoints(s string) (pid, px, py, pstate []int) {
	for len(s) > 0 {
		var rec string
		if i := indexByte(s, ';'); i >= 0 {
			rec, s = s[:i], s[i+1:]
		} else {
			rec, s = s, ""
		}
		if rec == "" {
			continue
		}
		var a, b, c, d int
		if n, _ := scanFour(rec, &a, &b, &c, &d); n == 4 {
			pid = append(pid, a)
			px = append(px, b)
			py = append(py, c)
			pstate = append(pstate, d)
		}
	}
	return
}

// parseIDList decodes the "id;id;..." serialization the tree multi-selection callbacks carry.
func parseIDList(s string) []int {
	var ids []int
	for len(s) > 0 {
		var rec string
		if i := indexByte(s, ';'); i >= 0 {
			rec, s = s[:i], s[i+1:]
		} else {
			rec, s = s, ""
		}
		if rec == "" {
			continue
		}
		var v, dummy1, dummy2, dummy3 int
		if n, _ := scanFour(rec, &v, &dummy1, &dummy2, &dummy3); n >= 1 {
			ids = append(ids, v)
		}
	}
	return ids
}

func indexByte(s string, ch byte) int {
	for i := 0; i < len(s); i++ {
		if s[i] == ch {
			return i
		}
	}
	return -1
}

func scanFour(rec string, a, b, c, d *int) (int, error) {
	vals := [4]*int{a, b, c, d}
	n, cur, started := 0, 0, false
	neg := false
	for i := 0; i <= len(rec); i++ {
		if i < len(rec) && rec[i] >= '0' && rec[i] <= '9' {
			cur = cur*10 + int(rec[i]-'0')
			started = true
		} else if i < len(rec) && rec[i] == '-' && !started {
			neg = true
		} else if i == len(rec) || rec[i] == ',' {
			if started && n < 4 {
				if neg {
					cur = -cur
				}
				*vals[n] = cur
				n++
			}
			cur, started, neg = 0, false, false
		}
	}
	return n, nil
}

// heap out-parameter helpers
func wasmGetF64(ptr int) float64    { return module().Call("getValue", ptr, "double").Float() }
func wasmSetF64(ptr int, v float64) { module().Call("setValue", ptr, v, "double") }
func wasmGetF32(ptr int) float32    { return float32(module().Call("getValue", ptr, "float").Float()) }
func wasmSetF32(ptr int, v float32) { module().Call("setValue", ptr, float64(v), "float") }

func wasmF64Array(xs []float64) int {
	ptr := wasmMalloc(len(xs) * 8)
	for i, v := range xs {
		module().Call("setValue", ptr+i*8, v, "double")
	}
	return ptr
}

func wasmStrArray(ss []string) (int, func()) {
	arr := wasmMalloc(len(ss) * 4)
	ptrs := make([]int, len(ss))
	for i, s := range ss {
		ptrs[i] = wasmStrToPtr(s)
		wasmSetI32(arr+i*4, ptrs[i])
	}
	return arr, func() {
		for _, p := range ptrs {
			wasmFree(p)
		}
		wasmFree(arr)
	}
}

func b2i(b bool) int {
	if b {
		return 1
	}
	return 0
}

func wasmGetStrAt(ptr int) string { return wasmPtrToStr(wasmGetI32(ptr)) }

func wasmStrToPtr(s string) int {
	n := module().Call("lengthBytesUTF8", s).Int() + 1
	ptr := wasmMalloc(n)
	module().Call("stringToUTF8", s, ptr, n)
	return ptr
}

func wasmBufWrite(ptr int, s string, max int) { module().Call("stringToUTF8", s, ptr, max) }
func wasmPtrToStr(ptr int) string             { return module().Call("UTF8ToString", ptr).String() }
func wasmGetU8(ptr int) uint8 {
	return uint8(module().Call("getValue", ptr, "i8").Int() & 0xFF)
}

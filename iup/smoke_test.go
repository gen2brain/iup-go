//go:build !js

package iup

import (
	"os"
	"testing"
	"time"
	"unsafe"
)

// GTK cannot reinit in-process, so open once for all tests.
func TestMain(m *testing.M) {
	if Open() == ERROR {
		os.Exit(1)
	}
	code := m.Run()
	Close()
	os.Exit(code)
}

// Public API end-to-end; runs under both the cgo and nocgo backend.
func TestSmoke(t *testing.T) {
	if v := Version(); v == "" {
		t.Fatal("empty Version()")
	} else {
		t.Logf("IUP %s (%d)", v, VersionNumber())
	}

	btn := Button("Click me")
	if btn == 0 {
		t.Fatal("Button returned nil handle")
	}

	btn.SetAttribute("NAME", "SMOKE_BTN")
	if got := btn.GetAttribute("NAME"); got != "SMOKE_BTN" {
		t.Fatalf("attribute round-trip failed: got %q", got)
	}

	// int round-trip (IupSetInt/IupGetInt)
	SetAttribute(btn, "GAP", 7)
	if got := GetInt(btn, "GAP"); got != 7 {
		t.Fatalf("int round-trip failed: got %d", got)
	}

	// RGB out-param round-trip (IupSetRGB / IupGetRGB writes through *uint8)
	SetRGB(btn, "BGCOLOR", 10, 20, 30)
	if r, g, b := GetRGB(btn, "BGCOLOR"); r != 10 || g != 20 || b != 30 {
		t.Fatalf("RGB out-param round-trip failed: got %d,%d,%d", r, g, b)
	}

	// char** out-array (IupGetAllAttributes)
	if attrs := GetAllAttributes(btn); len(attrs) == 0 {
		t.Fatal("GetAllAttributes returned nothing")
	}

	// global round-trip
	SetGlobal("SMOKE_GLOBAL", "on")
	if got := GetGlobal("SMOKE_GLOBAL"); got != "on" {
		t.Fatalf("global round-trip failed: got %q", got)
	}

	SetCallback(btn, "ACTION", ActionFunc(func(Ihandle) int { return DEFAULT }))
	if GetCallback(btn, "ACTION") == 0 {
		t.Fatal("ACTION callback not installed on native side")
	}

	// Multi-arg callbacks install their native trampolines without panicking.
	SetCallback(btn, "BUTTON_CB", ButtonFunc(func(Ihandle, int, int, int, int, string) int { return DEFAULT }))
	SetCallback(btn, "MOTION_CB", MotionFunc(func(Ihandle, int, int, string) int { return DEFAULT }))
	SetCallback(btn, "K_ANY", KAnyFunc(func(Ihandle, int) int { return CONTINUE }))
	for _, cb := range []string{"BUTTON_CB", "MOTION_CB", "K_ANY"} {
		if GetCallback(btn, cb) == 0 {
			t.Fatalf("%s not installed on native side", cb)
		}
	}

	// Every constructor must return a non-nil handle.
	for name, h := range map[string]Ihandle{
		"Label":       Label("x"),
		"Toggle":      Toggle("t"),
		"Text":        Text(),
		"List":        List(),
		"Tree":        Tree(),
		"Canvas":      Canvas(),
		"ProgressBar": ProgressBar(),
		"Val":         Val("HORIZONTAL"),
		"Frame":       Frame(Fill()),
		"Tabs":        Tabs(Vbox(Label("a")), Vbox(Label("b"))),
		"Zbox":        Zbox(Fill(), Fill()),
		"ScrollBox":   ScrollBox(Fill()),
		"Split":       Split(Fill(), Fill()),
		"FileDlg":     FileDlg(),
		"ColorDlg":    ColorDlg(),
	} {
		if h == 0 {
			t.Fatalf("%s returned nil handle", name)
		}
	}

	dlg := Dialog(Vbox(btn, Label("hello"), Hbox(Fill(), Toggle("ok"))))
	if dlg == 0 {
		t.Fatal("Dialog returned nil handle")
	}
	Destroy(dlg)
}

// Runtime element callback (Timer ACTION_CB) delivered through the loop.
func TestTimerCallback(t *testing.T) {
	ticks := 0
	tmr := Timer()
	tmr.SetAttribute("TIME", 20)
	SetCallback(tmr, "ACTION_CB", TimerActionFunc(func(Ihandle) int {
		ticks++
		return DEFAULT
	}))
	tmr.SetAttribute("RUN", "YES")

	deadline := time.Now().Add(3 * time.Second)
	for ticks < 3 && time.Now().Before(deadline) {
		LoopStep()
		time.Sleep(5 * time.Millisecond)
	}
	tmr.SetAttribute("RUN", "NO")
	Destroy(tmr)

	if ticks < 3 {
		t.Fatalf("Timer ACTION_CB fired %d times, want >= 3", ticks)
	}
	t.Logf("Timer ACTION_CB fired %d times through the loop", ticks)
}

// Multi-arg callback (RESIZE_CB) delivered with correct args on map.
func TestResizeCallback(t *testing.T) {
	var gotW, gotH int
	fired := false

	cv := Canvas()
	cv.SetAttribute("RASTERSIZE", "150x80")
	SetCallback(cv, "RESIZE_CB", ResizeFunc(func(ih Ihandle, w, h int) int {
		fired, gotW, gotH = true, w, h
		return DEFAULT
	}))

	dlg := Dialog(cv)
	dlg.SetAttribute("TITLE", "resize test")
	Show(dlg)

	deadline := time.Now().Add(2 * time.Second)
	for !fired && time.Now().Before(deadline) {
		LoopStep()
		time.Sleep(5 * time.Millisecond)
	}
	Hide(dlg)
	Destroy(dlg)

	if !fired {
		t.Fatal("RESIZE_CB never fired")
	}
	if gotW <= 0 || gotH <= 0 {
		t.Fatalf("RESIZE_CB delivered bad args: %dx%d", gotW, gotH)
	}
	t.Logf("RESIZE_CB fired with %dx%d", gotW, gotH)
}

// Global callback (IDLE_ACTION via SetFunction) delivered through the loop.
func TestIdleGlobal(t *testing.T) {
	ticks := 0
	SetFunction("IDLE_ACTION", IdleFunc(func() int {
		ticks++
		return DEFAULT
	}))
	defer SetFunction("IDLE_ACTION", nil)

	deadline := time.Now().Add(2 * time.Second)
	for ticks < 3 && time.Now().Before(deadline) {
		LoopStep()
		time.Sleep(2 * time.Millisecond)
	}
	if ticks < 3 {
		t.Fatalf("IDLE_ACTION fired %d times, want >= 3", ticks)
	}
	t.Logf("IDLE_ACTION fired %d times through the loop", ticks)
}

// Layout tree navigation.
func TestLayout(t *testing.T) {
	a, b := Label("a"), Label("b")
	box := Vbox(a, b)
	if n := GetChildCount(box); n != 2 {
		t.Fatalf("GetChildCount = %d, want 2", n)
	}
	if GetChild(box, 0) != a {
		t.Fatal("GetChild(box,0) != a")
	}
	if GetParent(a) != box {
		t.Fatal("GetParent(a) != box")
	}
	if p := GetChildPos(box, b); p != 1 {
		t.Fatalf("GetChildPos(box,b) = %d, want 1", p)
	}
	Append(box, Label("c"))
	if n := GetChildCount(box); n != 3 {
		t.Fatalf("after Append GetChildCount = %d, want 3", n)
	}
	dlg := Dialog(box)
	if GetDialog(a) != dlg {
		t.Fatal("GetDialog(a) != dlg")
	}
	Destroy(dlg)
}

// Image pixel-array marshaling + menu construction.
func TestResources(t *testing.T) {
	img := ImageRGB(2, 2, []byte{255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0})
	if img == 0 {
		t.Fatal("ImageRGB returned nil")
	}
	menu := Menu(MenuItem("File"), MenuSeparator(), Submenu("Edit", Menu(MenuItem("Copy"))))
	if menu == 0 {
		t.Fatal("Menu returned nil")
	}
	if n := GetChildCount(menu); n != 3 {
		t.Fatalf("menu child count = %d, want 3", n)
	}
	Destroy(menu)
	Destroy(img)
}

// Config variable round-trips (str/int/double + default).
func TestConfig(t *testing.T) {
	cfg := Config()
	if cfg == 0 {
		t.Fatal("Config returned nil")
	}
	ConfigSetVariableStr(cfg, "MainWindow", "Title", "hello")
	ConfigSetVariableInt(cfg, "MainWindow", "Width", 800)
	ConfigSetVariableDouble(cfg, "MainWindow", "Zoom", 1.5)
	if v := ConfigGetVariableStr(cfg, "MainWindow", "Title"); v != "hello" {
		t.Fatalf("str = %q, want hello", v)
	}
	if v := ConfigGetVariableInt(cfg, "MainWindow", "Width"); v != 800 {
		t.Fatalf("int = %d, want 800", v)
	}
	if v := ConfigGetVariableDouble(cfg, "MainWindow", "Zoom"); v != 1.5 {
		t.Fatalf("double = %v, want 1.5", v)
	}
	if v := ConfigGetVariableStrDef(cfg, "MainWindow", "Missing", "def"); v != "def" {
		t.Fatalf("default = %q, want def", v)
	}
	Destroy(cfg)
}

// Registry introspection (char** arrays at scale + class lookup).
func TestIntrospect(t *testing.T) {
	classes := GetAllClasses()
	if len(classes) < 20 {
		t.Fatalf("GetAllClasses = %d, want many", len(classes))
	}
	globals := GetAllGlobals()
	if len(globals) < 20 {
		t.Fatalf("GetAllGlobals = %d, want many", len(globals))
	}
	attrs := GetClassAttributes("button")
	if len(attrs) == 0 {
		t.Fatal("button reported no attributes")
	}
	if cn := GetClassName(Button("x")); cn != "button" {
		t.Fatalf("GetClassName = %q, want button", cn)
	}
	t.Logf("%d classes, %d globals, button has %d attributes", len(classes), len(globals), len(attrs))
}

// Tree + drag-drop callbacks dispatch and install their trampolines.
func TestTreeCallbacks(t *testing.T) {
	tree := Tree()
	SetCallback(tree, "SELECTION_CB", SelectionFunc(func(Ihandle, int, int) int { return DEFAULT }))
	SetCallback(tree, "MULTISELECTION_CB", MultiSelectionFunc(func(Ihandle, []int, int) int { return DEFAULT }))
	SetCallback(tree, "BRANCHOPEN_CB", BranchOpenFunc(func(Ihandle, int) int { return DEFAULT }))
	SetCallback(tree, "EXECUTELEAF_CB", ExecuteLeafFunc(func(Ihandle, int) int { return DEFAULT }))
	SetCallback(tree, "RENAME_CB", RenameFunc(func(Ihandle, int, string) int { return DEFAULT }))
	SetCallback(tree, "DRAGDROP_CB", DragDropFunc(func(Ihandle, int, int, int, int) int { return DEFAULT }))
	SetCallback(tree, "DROPDATA_CB", DropDataFunc(func(Ihandle, string, unsafe.Pointer, int, int, int) int { return DEFAULT }))
	for _, cb := range []string{"SELECTION_CB", "MULTISELECTION_CB", "BRANCHOPEN_CB", "EXECUTELEAF_CB", "RENAME_CB", "DRAGDROP_CB", "DROPDATA_CB"} {
		if GetCallback(tree, cb) == 0 {
			t.Fatalf("%s not installed", cb)
		}
	}
	Destroy(tree)
}

// Attribute typing, multi-set, empty-clears, reset (same hash semantics both backends).
func TestAttributeTypes(t *testing.T) {
	e := Text()

	e.SetAttribute("VALUE", "hello")
	if got := e.GetAttribute("VALUE"); got != "hello" {
		t.Fatalf("VALUE = %q", got)
	}

	SetAttribute(e, "TI", 42)
	if GetInt(e, "TI") != 42 {
		t.Fatalf("GetInt = %d", GetInt(e, "TI"))
	}
	SetAttribute(e, "TD", "1.5")
	if GetDouble(e, "TD") != 1.5 || GetFloat(e, "TD") != 1.5 {
		t.Fatalf("GetDouble/Float = %v / %v", GetDouble(e, "TD"), GetFloat(e, "TD"))
	}
	SetAttribute(e, "TB", "YES")
	if !GetBool(e, "TB") {
		t.Fatal("GetBool = false, want true")
	}

	SetAttributes(e, "CUEBANNER=hint, NC=5")
	if e.GetAttribute("CUEBANNER") != "hint" || GetInt(e, "NC") != 5 {
		t.Fatalf("SetAttributes: CUEBANNER=%q NC=%d", e.GetAttribute("CUEBANNER"), GetInt(e, "NC"))
	}

	// "" clears the attribute (cgo cStrOrNull and nocgo both route ""->NULL).
	SetAttribute(e, "TMP", "x")
	SetAttribute(e, "TMP", "")
	if got := e.GetAttribute("TMP"); got != "" {
		t.Fatalf("empty-set did not clear: %q", got)
	}

	SetAttribute(e, "R", "y")
	ResetAttribute(e, "R")
	if got := e.GetAttribute("R"); got != "" {
		t.Fatalf("ResetAttribute did not clear: %q", got)
	}
	Destroy(e)
}

// Named-handle registry round-trip.
func TestHandleName(t *testing.T) {
	b := Button("h")
	SetHandle("SMOKE_HANDLE", b)
	if GetHandle("SMOKE_HANDLE") != b {
		t.Fatal("GetHandle != set handle")
	}
	if got := GetName(b); got != "SMOKE_HANDLE" {
		t.Fatalf("GetName = %q", got)
	}
	SetHandle("SMOKE_HANDLE", 0)
	Destroy(b)
}

// Built-in globals are populated after Open.
func TestGlobals(t *testing.T) {
	if GetGlobal("DRIVER") == "" {
		t.Fatal("DRIVER global empty")
	}
	if GetGlobal("DEFAULTFONT") == "" {
		t.Fatal("DEFAULTFONT global empty")
	}
	t.Logf("DRIVER=%s", GetGlobal("DRIVER"))
}

// List model: items via id attributes, COUNT, VALUE selection.
func TestListItems(t *testing.T) {
	l := List()
	l.SetAttribute("1", "Apple")
	l.SetAttribute("2", "Banana")
	l.SetAttribute("3", "Cherry")

	dlg := Dialog(l)
	Show(dlg)

	if n := GetInt(l, "COUNT"); n != 3 {
		t.Fatalf("COUNT = %d, want 3", n)
	}
	if got := l.GetAttribute("2"); got != "Banana" {
		t.Fatalf("item 2 = %q", got)
	}
	l.SetAttribute("VALUE", "2")
	if got := l.GetAttribute("VALUE"); got != "2" {
		t.Fatalf("VALUE = %q, want 2", got)
	}
	Hide(dlg)
	Destroy(dlg)
}

// Widget-lifecycle callback (MAP_CB) fires when the element is mapped.
func TestMapCallback(t *testing.T) {
	mapped := false
	b := Button("m")
	SetCallback(b, "MAP_CB", MapFunc(func(Ihandle) int {
		mapped = true
		return DEFAULT
	}))
	dlg := Dialog(b)
	Show(dlg)
	Hide(dlg)
	Destroy(dlg)
	if !mapped {
		t.Fatal("MAP_CB did not fire on Show")
	}
}

//go:build js && wasm

package iup

import (
	"fmt"
	"strconv"
	"syscall/js"
	"unsafe"
)

// Callback return values.
const (
	IGNORE   = -1
	DEFAULT  = -2
	CLOSE    = -3
	CONTINUE = -4
)

// Special position values for ShowXY/PopupXY.
const (
	CENTER       = 0xFFFF
	LEFT         = 0xFFFE
	RIGHT        = 0xFFFD
	MOUSEPOS     = 0xFFFC
	CURRENT      = 0xFFFB
	CENTERPARENT = 0xFFFA
	TOP          = LEFT
	BOTTOM       = RIGHT
)

// Scrollbar operation codes (SCROLL_CB op), matching the IUP_SB* enum order.
const (
	SBUP = iota
	SBDN
	SBPGUP
	SBPGDN
	SBPOSV
	SBDRAGV
	SBLEFT
	SBRIGHT
	SBPGLEFT
	SBPGRIGHT
	SBPOSH
	SBDRAGH
)

// Mouse button identifiers (BUTTON_CB, matching IUP_BUTTON1..5 = '1'..'5').
const (
	BUTTON1 = '1'
	BUTTON2 = '2'
	BUTTON3 = '3'
	BUTTON4 = '4'
	BUTTON5 = '5'
)

// ActionFunc is the type for the ACTION callback.
type ActionFunc func(Ihandle) int

// ToggleActionFunc is the type for the toggle ACTION callback (with state).
type ToggleActionFunc func(ih Ihandle, state int) int

// TextActionFunc is the type for the text ACTION callback.
type TextActionFunc func(ih Ihandle, ch int, newValue string) int

// ValueChangedFunc is the type for the VALUECHANGED_CB callback.
type ValueChangedFunc func(ih Ihandle) int

// KAnyFunc is the type for the K_ANY keyboard callback.
type KAnyFunc func(ih Ihandle, c int) int

// ListActionFunc is the type for the list ACTION callback.
type ListActionFunc func(ih Ihandle, text string, item, state int) int

// DblclickFunc is the type for the DBLCLICK_CB callback.
type DblclickFunc func(ih Ihandle, item int, text string) int

// MultiselectFunc is the type for the MULTISELECT_CB callback.
type MultiselectFunc func(ih Ihandle, text string) int

// GetFocusFunc is the type for the GETFOCUS_CB callback.
type GetFocusFunc func(ih Ihandle) int

// KillFocusFunc is the type for the KILLFOCUS_CB callback.
type KillFocusFunc func(ih Ihandle) int

// DropDownFunc is the type for the DROPDOWN_CB callback.
type DropDownFunc func(ih Ihandle, state int) int

// EditFunc is the type for the EDIT_CB callback.
type EditFunc func(ih Ihandle, c int, after string) int

// ButtonFunc is the type for the BUTTON_CB callback.
type ButtonFunc func(ih Ihandle, button, pressed, x, y int, status string) int

// TabChangeFunc is the type for the TABCHANGE_CB callback.
type TabChangeFunc func(ih, newTab, oldTab Ihandle) int

// TabChangePosFunc is the type for the TABCHANGEPOS_CB callback.
type TabChangePosFunc func(ih Ihandle, newPos, oldPos int) int

// ReorderFunc is the type for the REORDER_CB callback.
type ReorderFunc func(ih Ihandle, oldPos, newPos int) int

// TabCloseFunc is the type for the TABCLOSE_CB callback.
type TabCloseFunc func(ih Ihandle, pos int) int

// RightClickFunc is the type for the RIGHTCLICK_CB callback.
type RightClickFunc func(ih Ihandle, pos int) int

// TimerActionFunc is the type for the timer ACTION_CB callback.
type TimerActionFunc func(ih Ihandle) int

// ResizeFunc is the type for the RESIZE_CB callback.
type ResizeFunc func(ih Ihandle, width, height int) int

// MotionFunc is the type for the MOTION_CB callback.
type MotionFunc func(ih Ihandle, x, y int, status string) int

// WheelFunc is the type for the WHEEL_CB callback.
type WheelFunc func(ih Ihandle, delta float32, x, y int, status string) int

// CloseFunc is the type for the dialog CLOSE_CB callback.
type CloseFunc func(Ihandle) int

// NotifyFunc is the type for the IupNotify NOTIFY_CB callback.
type NotifyFunc func(ih Ihandle, actionId int) int

// NotifyCloseFunc is the type for the IupNotify CLOSE_CB callback.
type NotifyCloseFunc func(ih Ihandle, reason int) int

// ErrorFunc is the type for the ERROR_CB callback.
type ErrorFunc func(ih Ihandle, url string) int

// CompletedFunc is the type for the IupWebBrowser COMPLETED_CB callback.
type CompletedFunc func(ih Ihandle, url string) int

// NavigateFunc is the type for the IupWebBrowser NAVIGATE_CB callback.
type NavigateFunc func(ih Ihandle, url string) int

// NewWindowFunc is the type for the IupWebBrowser NEWWINDOW_CB callback.
type NewWindowFunc func(ih Ihandle, url string) int

// UpdateFunc is the type for the IupWebBrowser UPDATE_CB callback.
type UpdateFunc func(ih Ihandle) int

// Open initializes the IUP toolkit.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_open.md
func Open() int {
	return ccall("IupOpen", "number", []interface{}{"number", "number"}, []interface{}{0, 0}).Int()
}

// Destroy destroys an element and its children.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_destroy.md
func Destroy(ih Ihandle) {
	removeCallbacks(ih)
	ccall("IupDestroy", "", []interface{}{"number"}, []interface{}{int(ih)})
}

// Close ends the IUP toolkit and releases internal memory.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_close.md
func Close() {
	ccall("IupClose", "", nil, nil)
}

// EntryPoint is a portability shim. Under WebAssembly the Go main runs
// directly, so this is a no-op (matching the desktop behavior).
func EntryPoint(entry func()) {}

// MainLoop runs the application main loop. The Go top-level stays on the async
// JS event loop (a synchronous wait would freeze Go's scheduler) until a
// callback returns CLOSE. Modals block in C via IupPopup.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_mainloop.md
func MainLoop() int {
	exitCh = make(chan struct{})
	<-exitCh
	return 0
}

// Show displays a dialog in the current position, or changes a control VISIBLE state.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_show.md
func Show(ih Ihandle) int {
	return ccall("IupShow", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
}

// Popup shows a menu or dialog as a popup at the given position. A dialog blocks
// modally until closed.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_popup.md
func Popup(ih Ihandle, x, y int) int {
	return ccall("IupPopup", "number", []interface{}{"number", "number", "number"}, []interface{}{int(ih), x, y}).Int()
}

// ShowXY displays a dialog at the given position (use CENTER, LEFT, etc.).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_showxy.md
func ShowXY(ih Ihandle, x, y int) int {
	return ccall("IupShowXY", "number", []interface{}{"number", "number", "number"}, []interface{}{int(ih), x, y}).Int()
}

// Label creates a label interface element which displays a separator, a text or an image.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_label.md
func Label(title string) Ihandle {
	return ccallHandle("IupLabel", []interface{}{"string"}, []interface{}{title})
}

// Button creates an interface element that is a button.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_button.md
func Button(title string) Ihandle {
	return ccallHandle("IupButton", []interface{}{"string"}, []interface{}{title})
}

// Vbox creates a void container for composing elements vertically.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_vbox.md
func Vbox(children ...Ihandle) Ihandle {
	box := ccallHandle("iupwasmVbox0", nil, nil)
	for _, c := range children {
		ccall("IupAppend", "number", []interface{}{"number", "number"}, []interface{}{int(box), int(c)})
	}
	return box
}

// Hbox creates a void container for composing elements horizontally.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_hbox.md
func Hbox(children ...Ihandle) Ihandle {
	box := ccallHandle("iupwasmHbox0", nil, nil)
	for _, c := range children {
		ccall("IupAppend", "number", []interface{}{"number", "number"}, []interface{}{int(box), int(c)})
	}
	return box
}

// Tabs creates a container for grouping children into tabbed pages.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_tabs.md
func Tabs(children ...Ihandle) Ihandle {
	tabs := ccallHandle("iupwasmTabs0", nil, nil)
	for _, c := range children {
		ccall("IupAppend", "number", []interface{}{"number", "number"}, []interface{}{int(tabs), int(c)})
	}
	return tabs
}

// Frame creates a frame container that draws a border around its child, with an optional title.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_frame.md
func Frame(child Ihandle) Ihandle {
	return ccallHandle("IupFrame", []interface{}{"number"}, []interface{}{int(child)})
}

// Fill creates a void element that dynamically occupies empty spaces in a box.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_fill.md
func Fill() Ihandle {
	return ccallHandle("IupFill", nil, nil)
}

// Text creates an editable text field (single line).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_text.md
func Text() Ihandle {
	return ccallHandle("IupText", nil, nil)
}

// MultiLine creates an editable text field with multiple lines (Text with MULTILINE=YES).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_multiline.md
func MultiLine() Ihandle {
	return ccallHandle("IupMultiLine", nil, nil)
}

// ProgressBar creates a progress bar to show a percentage of a task.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_progressbar.md
func ProgressBar() Ihandle {
	return ccallHandle("IupProgressBar", nil, nil)
}

// Timer creates a timer that periodically invokes ACTION_CB while RUN=YES.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_timer.md
func Timer() Ihandle {
	return ccallHandle("IupTimer", nil, nil)
}

// Val creates a slider; _type is "HORIZONTAL" or "VERTICAL".
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_val.md
func Val(_type string) Ihandle {
	return ccallHandle("IupVal", []interface{}{"string"}, []interface{}{_type})
}

// Toggle creates the toggle interface element (checkbox, or radio inside a Radio).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_toggle.md
func Toggle(title string) Ihandle {
	return ccallHandle("IupToggle", []interface{}{"string"}, []interface{}{title})
}

// Radio creates a void container for grouping mutually exclusive toggles.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_radio.md
func Radio(child Ihandle) Ihandle {
	return ccallHandle("IupRadio", []interface{}{"number"}, []interface{}{int(child)})
}

// List creates a list (listbox, dropdown, or with an edit box).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_list.md
func List() Ihandle {
	return ccallHandle("IupList", nil, nil)
}

func imageBytes(width, height int, pixMap []byte, fn string) Ihandle {
	// heap copy, not ccall "array" (which stack-allocates and overflows for large images)
	ptr := wasmMalloc(len(pixMap))
	view := js.Global().Get("Uint8Array").New(module().Get("HEAPU8").Get("buffer"), ptr, len(pixMap))
	js.CopyBytesToJS(view, pixMap)
	ih := ccallHandle(fn, []interface{}{"number", "number", "number"}, []interface{}{width, height, ptr})
	wasmFree(ptr)
	return ih
}

// Image creates an image to be shown on labels, buttons, toggles, etc. (8bpp indexed).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_image.md
func Image(width, height int, pixMap []byte) Ihandle {
	return imageBytes(width, height, pixMap, "IupImage")
}

// ImageRGB creates a 24bpp RGB image.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_image.md
func ImageRGB(width, height int, pixMap []byte) Ihandle {
	return imageBytes(width, height, pixMap, "IupImageRGB")
}

// ImageRGBA creates a 32bpp RGBA image.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_image.md
func ImageRGBA(width, height int, pixMap []byte) Ihandle {
	return imageBytes(width, height, pixMap, "IupImageRGBA")
}

// Menu creates a menu (menu bar when set as a dialog MENU, else a popup).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_menu.md
func Menu(children ...Ihandle) Ihandle {
	menu := ccallHandle("iupwasmMenu0", nil, nil)
	for _, c := range children {
		ccall("IupAppend", "number", []interface{}{"number", "number"}, []interface{}{int(menu), int(c)})
	}
	return menu
}

// Submenu creates a submenu entry with the given title and child menu.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_submenu.md
func Submenu(title string, child Ihandle) Ihandle {
	return ccallHandle("IupSubmenu", []interface{}{"string", "number"}, []interface{}{title, int(child)})
}

// MenuItem creates a menu item with the given title.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_item.md
func MenuItem(title string) Ihandle {
	return ccallHandle("IupMenuItem", []interface{}{"string"}, []interface{}{title})
}

// MenuSeparator creates a menu separator line.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_menuseparator.md
func MenuSeparator() Ihandle {
	return ccallHandle("IupMenuSeparator", nil, nil)
}

// ClickFunc is the type for the table CLICK_CB callback.
type ClickFunc func(ih Ihandle, lin, col int, status string) int

// EnterItemFunc is the type for the table ENTERITEM_CB callback.
type EnterItemFunc func(ih Ihandle, lin, col int) int

// TableEditionFunc is the type for the table EDITION_CB callback.
type TableEditionFunc func(ih Ihandle, lin, col int, update string) int

// TableValueChangedFunc is the type for the table VALUECHANGED_CB callback.
type TableValueChangedFunc func(ih Ihandle, lin, col int) int

// ThemeChangedFunc is the type for the THEMECHANGED_CB callback.
type ThemeChangedFunc func(ih Ihandle, darkMode int) int

// ScrollFunc is the type for the SCROLL_CB callback.
type ScrollFunc func(ih Ihandle, op int, posx, posy float64) int

// ShowFunc is the type for the SHOW_CB callback.
type ShowFunc func(ih Ihandle, state int) int

// DragDropFunc is the type for the list/tree DRAGDROP_CB callback.
type DragDropFunc func(ih Ihandle, dragId, dropId, isShift, isControl int) int

// DragBeginFunc is the type for the DRAGBEGIN_CB callback.
type DragBeginFunc func(ih Ihandle, x, y int) int

// DragDataSizeFunc is the type for the DRAGDATASIZE_CB callback.
type DragDataSizeFunc func(ih Ihandle, dragType string) int

// DragDataFunc is the type for the DRAGDATA_CB callback.
type DragDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size int) int

// DragEndFunc is the type for the DRAGEND_CB callback.
type DragEndFunc func(ih Ihandle, action int) int

// DropDataFunc is the type for the DROPDATA_CB callback.
type DropDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size, x, y int) int

// DropMotionFunc is the type for the DROPMOTION_CB callback.
type DropMotionFunc func(ih Ihandle, x, y int, status string) int

// DropFilesFunc is the type for the DROPFILES_CB callback.
type DropFilesFunc func(ih Ihandle, fileName string, num, x, y int) int

// SelectionFunc is the type for the tree SELECTION_CB callback.
type SelectionFunc func(ih Ihandle, id, status int) int

// BranchOpenFunc is the type for the tree BRANCHOPEN_CB callback.
type BranchOpenFunc func(ih Ihandle, id int) int

// BranchCloseFunc is the type for the tree BRANCHCLOSE_CB callback.
type BranchCloseFunc func(ih Ihandle, id int) int

// ExecuteLeafFunc is the type for the tree EXECUTELEAF_CB callback.
type ExecuteLeafFunc func(ih Ihandle, id int) int

// ExecuteBranchFunc is the type for the tree EXECUTEBRANCH_CB callback.
type ExecuteBranchFunc func(ih Ihandle, id int) int

// ShowRenameFunc is the type for the tree SHOWRENAME_CB callback.
type ShowRenameFunc func(ih Ihandle, id int) int

// RenameFunc is the type for the tree RENAME_CB callback.
type RenameFunc func(ih Ihandle, id int, title string) int

// HighlightFunc is the type for the menu item HIGHLIGHT_CB callback.
type HighlightFunc func(ih Ihandle) int

// MenuOpenFunc is the type for the MENUOPEN_CB callback.
type MenuOpenFunc func(ih Ihandle) int

// MenuCloseFunc is the type for the MENUCLOSE_CB callback.
type MenuCloseFunc func(ih Ihandle) int

// Calendar creates a calendar (date picker) element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_calendar.md
func Calendar() Ihandle {
	return ccallHandle("IupCalendar", nil, nil)
}

// Table creates a grid/table element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_table.md
func Table() Ihandle {
	return ccallHandle("IupTable", nil, nil)
}

// Tree creates a tree-view element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_tree.md
func Tree() Ihandle {
	return ccallHandle("IupTree", nil, nil)
}

// Scrollbar creates a standalone scrollbar; orientation is "HORIZONTAL" or "VERTICAL".
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_scrollbar.md
func Scrollbar(orientation string) Ihandle {
	return ccallHandle("IupScrollbar", []interface{}{"string"}, []interface{}{orientation})
}

// SetAttributeId sets an id-numbered attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattribute.md
func SetAttributeId(ih Ihandle, name string, id int, value interface{}) {
	switch v := value.(type) {
	case nil:
		ccall("IupSetAttributeId", "", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, id, 0})
	case Ihandle:
		ccall("IupSetAttributeId", "", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, id, int(v)})
	case uintptr:
		ccall("IupSetAttributeId", "", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, id, int(v)})
	case string:
		ccall("IupSetStrAttributeId", "", []interface{}{"number", "string", "number", "string"}, []interface{}{int(ih), name, id, v})
	case int:
		SetAttributeId(ih, name, id, strconv.Itoa(v))
	default:
		SetAttributeId(ih, name, id, fmt.Sprintf("%v", v))
	}
}

// GetAttributeId returns an id-numbered attribute value.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattribute.md
func GetAttributeId(ih Ihandle, name string, id int) string {
	return ccall("IupGetAttributeId", "string", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, id}).String()
}

// SetAttributeId2 sets a lin:col-numbered attribute (e.g. a table cell).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattribute.md
func SetAttributeId2(ih Ihandle, name string, lin, col int, value interface{}) {
	switch v := value.(type) {
	case nil:
		ccall("IupSetAttributeId2", "", []interface{}{"number", "string", "number", "number", "number"}, []interface{}{int(ih), name, lin, col, 0})
	case Ihandle:
		ccall("IupSetAttributeId2", "", []interface{}{"number", "string", "number", "number", "number"}, []interface{}{int(ih), name, lin, col, int(v)})
	case uintptr:
		ccall("IupSetAttributeId2", "", []interface{}{"number", "string", "number", "number", "number"}, []interface{}{int(ih), name, lin, col, int(v)})
	case string:
		ccall("IupSetStrAttributeId2", "", []interface{}{"number", "string", "number", "number", "string"}, []interface{}{int(ih), name, lin, col, v})
	case int:
		SetAttributeId2(ih, name, lin, col, strconv.Itoa(v))
	default:
		SetAttributeId2(ih, name, lin, col, fmt.Sprintf("%v", v))
	}
}

// GetAttributeId2 returns a lin:col-numbered attribute value (e.g. a table cell).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattribute.md
func GetAttributeId2(ih Ihandle, name string, lin, col int) string {
	return ccall("IupGetAttributeId2", "string", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, lin, col}).String()
}

// Canvas creates a canvas for custom drawing through the IupDraw API.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_canvas.md
func Canvas() Ihandle {
	return ccallHandle("IupCanvas", nil, nil)
}

// FontDlg creates a font selection dialog (custom IUP-controls dialog).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_fontdlg.md
func FontDlg() Ihandle {
	return ccallHandle("IupFontDlg", nil, nil)
}

// Popover creates a floating container anchored to another element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_popover.md
func Popover(child Ihandle) Ihandle {
	return ccallHandle("IupPopover", []interface{}{"number"}, []interface{}{int(child)})
}

// Dialog creates a dialog element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_dialog.md
func Dialog(child Ihandle) Ihandle {
	return ccallHandle("IupDialog", []interface{}{"number"}, []interface{}{int(child)})
}

// GetParent returns the parent of an element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getparent.md
func GetParent(ih Ihandle) Ihandle {
	return ccallHandle("IupGetParent", []interface{}{"number"}, []interface{}{int(ih)})
}

// GetChildPos returns the position of a child in its parent's child list.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getchildpos.md
func GetChildPos(ih, child Ihandle) int {
	return ccall("IupGetChildPos", "number", []interface{}{"number", "number"}, []interface{}{int(ih), int(child)}).Int()
}

// SetAttribute sets an interface element attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattribute.md
func SetAttribute(ih Ihandle, name string, value interface{}) {
	if wasmThreadSetAttr(ih, name, value) {
		return
	}
	switch v := value.(type) {
	case nil:
		ccall("IupSetAttribute", "", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, 0})
	case Ihandle:
		ccall("IupSetAttribute", "", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, int(v)})
	case uintptr:
		ccall("IupSetAttribute", "", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, int(v)})
	case string:
		ccall("IupSetStrAttribute", "", []interface{}{"number", "string", "string"}, []interface{}{int(ih), name, v})
	case bool:
		s := "NO"
		if v {
			s = "YES"
		}
		SetAttribute(ih, name, s)
	case int:
		SetAttribute(ih, name, strconv.Itoa(v))
	default:
		SetAttribute(ih, name, fmt.Sprintf("%v", v))
	}
}

// SetAttributes sets several attributes at once from a string.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattributes.md
func SetAttributes(ih Ihandle, str string) Ihandle {
	ccall("IupSetAttributes", "number", []interface{}{"number", "string"}, []interface{}{int(ih), str})
	return ih
}

// GetAttribute returns an interface element attribute value.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattribute.md
func GetAttribute(ih Ihandle, name string) string {
	return ccall("IupGetAttribute", "string", []interface{}{"number", "string"}, []interface{}{int(ih), name}).String()
}

// SetAttributeHandle associates a named element with an attribute (e.g. IMAGE handles).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattributehandle.md
func SetAttributeHandle(ih Ihandle, name string, ihNamed Ihandle) {
	ccall("IupSetAttributeHandle", "", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, int(ihNamed)})
}

// SetHandle associates a name with an interface element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_sethandle.md
func SetHandle(name string, ih Ihandle) Ihandle {
	ccall("IupSetHandle", "", []interface{}{"string", "number"}, []interface{}{name, int(ih)})
	return ih
}

// GetHandle returns the element associated with a name.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_gethandle.md
func GetHandle(name string) Ihandle {
	return ccallHandle("IupGetHandle", []interface{}{"string"}, []interface{}{name})
}

// GetGlobal returns a global attribute value.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getglobal.md
func GetGlobal(name string) string {
	return ccall("IupGetGlobal", "string", []interface{}{"string"}, []interface{}{name}).String()
}

// GetChild returns the child at the given position (0-based).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getchild.md
func GetChild(ih Ihandle, pos int) Ihandle {
	return ccallHandle("IupGetChild", []interface{}{"number", "number"}, []interface{}{int(ih), pos})
}

// GetFloat returns an attribute value as a float32.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattribute.md
func GetFloat(ih Ihandle, name string) float32 {
	return float32(ccall("IupGetFloat", "number", []interface{}{"number", "string"}, []interface{}{int(ih), name}).Float())
}

// GetName returns the name of an element previously associated by SetHandle.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getname.md
func GetName(ih Ihandle) string {
	return ccall("IupGetName", "string", []interface{}{"number"}, []interface{}{int(ih)}).String()
}

// GetInt returns an attribute value as an integer.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattribute.md
func GetInt(ih Ihandle, name string) int {
	return ccall("IupGetInt", "number", []interface{}{"number", "string"}, []interface{}{int(ih), name}).Int()
}

// SetCallback associates a callback with an event.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setcallback.md
func SetCallback(ih Ihandle, name string, fn interface{}) {
	if fn == nil {
		delete(callbacks, cbKey{ih, name})
		ccall("IupSetCallback", "", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, 0})
		return
	}
	callbacks[cbKey{ih, name}] = fn
	ccall("iupwasmGoSetCallback", "", []interface{}{"number", "string"}, []interface{}{int(ih), name})
}

// Message shows a modal message dialog (browser alert).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_messagedlg.md
func Message(title, msg string) {
	ccall("IupMessage", "", []interface{}{"string", "string"}, []interface{}{title, msg})
}

// MessageError shows a modal error message dialog.
func MessageError(parent Ihandle, msg string) {
	ccall("IupMessageError", "", []interface{}{"number", "string"}, []interface{}{int(parent), msg})
}

// MessageAlarm shows a modal message dialog with the given buttons.
func MessageAlarm(parent Ihandle, title, msg, buttons string) {
	ccall("IupMessageAlarm", "", []interface{}{"number", "string", "string", "string"}, []interface{}{int(parent), title, msg, buttons})
}

// Alarm shows a modal dialog with up to three buttons and returns the pressed
// one (1, 2 or 3). b2/b3 may be empty to omit those buttons.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_alarm.md
func Alarm(title, msg, b1, b2, b3 string) int {
	return ccall("IupAlarm", "number",
		[]interface{}{"string", "string", "string", "string", "string"},
		[]interface{}{title, msg, b1, b2, b3}).Int()
}

// Notify creates a desktop notification element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_notify.md
func Notify() Ihandle {
	return ccallHandle("IupNotify", nil, nil)
}

// Clipboard creates a clipboard access element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_clipboard.md
func Clipboard() Ihandle {
	return ccallHandle("IupClipboard", nil, nil)
}

// Help opens a URL in a new browser tab.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_help.md
func Help(url string) int {
	return ccall("IupHelp", "number", []interface{}{"string"}, []interface{}{url}).Int()
}

// SetGlobal sets a global attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setglobal.md
func SetGlobal(name string, value interface{}) {
	switch v := value.(type) {
	case nil:
		ccall("IupSetGlobal", "", []interface{}{"string", "number"}, []interface{}{name, 0})
	case Ihandle:
		ccall("IupSetGlobal", "", []interface{}{"string", "number"}, []interface{}{name, int(v)})
	case uintptr:
		ccall("IupSetGlobal", "", []interface{}{"string", "number"}, []interface{}{name, int(v)})
	case string:
		ccall("IupSetStrGlobal", "", []interface{}{"string", "string"}, []interface{}{name, v})
	case bool:
		s := "NO"
		if v {
			s = "YES"
		}
		SetGlobal(name, s)
	case int:
		SetGlobal(name, strconv.Itoa(v))
	default:
		SetGlobal(name, fmt.Sprintf("%v", v))
	}
}

// --- IupDraw API ----------------------------------------------------------

// DrawBegin initializes the drawing process.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawBegin(ih Ihandle) {
	ccall("IupDrawBegin", "", []interface{}{"number"}, []interface{}{int(ih)})
}

// DrawEnd terminates the drawing process and flushes to the screen.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawEnd(ih Ihandle) {
	ccall("IupDrawEnd", "", []interface{}{"number"}, []interface{}{int(ih)})
}

// DrawGetSize returns the drawing area size.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetSize(ih Ihandle) (w, h int) {
	pw := wasmMalloc(8)
	ph := pw + 4
	ccall("IupDrawGetSize", "", []interface{}{"number", "number", "number"}, []interface{}{int(ih), pw, ph})
	w, h = wasmGetI32(pw), wasmGetI32(ph)
	wasmFree(pw)
	return
}

// DrawParentBackground fills the canvas with the native parent background color.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawParentBackground(ih Ihandle) {
	ccall("IupDrawParentBackground", "", []interface{}{"number"}, []interface{}{int(ih)})
}

// DrawLine draws a line including start and end points.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawLine(ih Ihandle, x1, y1, x2, y2 int) {
	ccall("IupDrawLine", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2})
}

// DrawRectangle draws a rectangle including start and end points.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRectangle(ih Ihandle, x1, y1, x2, y2 int) {
	ccall("IupDrawRectangle", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2})
}

// DrawArc draws an arc inside a rectangle between the two angles in degrees.
// When filled it draws a pie shape. Angles are counter-clockwise from 3 o'clock.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawArc(ih Ihandle, x1, y1, x2, y2 int, a1, a2 float64) {
	ccall("IupDrawArc", "", []interface{}{"number", "number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2, a1, a2})
}

// DrawEllipse draws an ellipse inscribed in the rectangle (x1,y1)-(x2,y2).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawEllipse(ih Ihandle, x1, y1, x2, y2 int) {
	ccall("IupDrawEllipse", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2})
}

// DrawPolygon draws a polygon. Coordinates are x1, y1, x2, y2, ...
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPolygon(ih Ihandle, points []int, count int) {
	if len(points) == 0 {
		return
	}
	ptr := wasmMalloc(len(points) * 4)
	for i, v := range points {
		wasmSetI32(ptr+i*4, v)
	}
	ccall("IupDrawPolygon", "", []interface{}{"number", "number", "number"}, []interface{}{int(ih), ptr, count})
	wasmFree(ptr)
}

// DrawPixel draws a single pixel.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPixel(ih Ihandle, x, y int) {
	ccall("IupDrawPixel", "", []interface{}{"number", "number", "number"}, []interface{}{int(ih), x, y})
}

// DrawRoundedRectangle draws a rectangle with rounded corners.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRoundedRectangle(ih Ihandle, x1, y1, x2, y2, cornerRadius int) {
	ccall("IupDrawRoundedRectangle", "", []interface{}{"number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2, cornerRadius})
}

// DrawBezier draws a cubic Bezier curve.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawBezier(ih Ihandle, x1, y1, x2, y2, x3, y3, x4, y4 int) {
	ccall("IupDrawBezier", "", []interface{}{"number", "number", "number", "number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2, x3, y3, x4, y4})
}

// DrawQuadraticBezier draws a quadratic Bezier curve.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawQuadraticBezier(ih Ihandle, x1, y1, x2, y2, x3, y3 int) {
	ccall("IupDrawQuadraticBezier", "", []interface{}{"number", "number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2, x3, y3})
}

// DrawText draws text using the font defined by DRAWFONT, or FONT.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawText(ih Ihandle, str string, x, y, w, h int) {
	ccall("IupDrawText", "", []interface{}{"number", "string", "number", "number", "number", "number", "number"}, []interface{}{int(ih), str, len(str), x, y, w, h})
}

// DrawImage draws an image given its name; w/h of -1 use the image size.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawImage(ih Ihandle, name string, x, y, w, h int) {
	ccall("IupDrawImage", "", []interface{}{"number", "string", "number", "number", "number", "number"}, []interface{}{int(ih), name, x, y, w, h})
}

// DrawSelectRect draws a selection rectangle.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSelectRect(ih Ihandle, x1, y1, x2, y2 int) {
	ccall("IupDrawSelectRect", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2})
}

// DrawFocusRect draws a focus rectangle.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawFocusRect(ih Ihandle, x1, y1, x2, y2 int) {
	ccall("IupDrawFocusRect", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2})
}

// DrawSetClipRect defines a rectangular clipping region.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetClipRect(ih Ihandle, x1, y1, x2, y2 int) {
	ccall("IupDrawSetClipRect", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2})
}

// DrawSetClipRoundedRect defines a rounded rectangular clipping region.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetClipRoundedRect(ih Ihandle, x1, y1, x2, y2, cornerRadius int) {
	ccall("IupDrawSetClipRoundedRect", "", []interface{}{"number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), x1, y1, x2, y2, cornerRadius})
}

// DrawResetClip resets the clipping area to none.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawResetClip(ih Ihandle) {
	ccall("IupDrawResetClip", "", []interface{}{"number"}, []interface{}{int(ih)})
}

// DrawLinearGradient draws a linear gradient between two colors.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawLinearGradient(ih Ihandle, x1, y1, x2, y2 int, angle float32, color1, color2 string) {
	ccall("IupDrawLinearGradient", "", []interface{}{"number", "number", "number", "number", "number", "number", "string", "string"}, []interface{}{int(ih), x1, y1, x2, y2, float64(angle), color1, color2})
}

// DrawRadialGradient draws a radial gradient from center to edge.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRadialGradient(ih Ihandle, cx, cy, radius int, colorCenter, colorEdge string) {
	ccall("IupDrawRadialGradient", "", []interface{}{"number", "number", "number", "number", "string", "string"}, []interface{}{int(ih), cx, cy, radius, colorCenter, colorEdge})
}

// DrawGetTextSize returns the given text size using DRAWFONT, or FONT.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetTextSize(ih Ihandle, str string) (w, h int) {
	pw := wasmMalloc(8)
	ph := pw + 4
	ccall("IupDrawGetTextSize", "", []interface{}{"number", "string", "number", "number", "number"}, []interface{}{int(ih), str, len(str), pw, ph})
	w, h = wasmGetI32(pw), wasmGetI32(ph)
	wasmFree(pw)
	return
}

// DrawGetTextMetrics returns the font metrics using DRAWFONT, or FONT.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetTextMetrics(ih Ihandle) (ascent, descent, lineHeight int) {
	pa := wasmMalloc(12)
	pd := pa + 4
	pl := pa + 8
	ccall("IupDrawGetTextMetrics", "", []interface{}{"number", "number", "number", "number"}, []interface{}{int(ih), pa, pd, pl})
	ascent, descent, lineHeight = wasmGetI32(pa), wasmGetI32(pd), wasmGetI32(pl)
	wasmFree(pa)
	return
}

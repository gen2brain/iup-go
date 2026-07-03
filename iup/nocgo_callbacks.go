//go:build !cgo && !js

package iup

import (
	"slices"
	"strconv"
	"strings"
	"sync"
	"unsafe"

	"github.com/ebitengine/purego"
)

// Go callbacks live in a process-wide registry keyed by an int id; the base36 id
// is stored as an _IUPGO_<CB> attribute so one trampoline per name serves all elements.
var cbReg = struct {
	sync.Mutex
	m    map[uint64]any
	next uint64
}{m: make(map[uint64]any)}

func cbStore(f any) uint64 {
	cbReg.Lock()
	defer cbReg.Unlock()
	cbReg.next++
	if cbReg.next == 0 {
		cbReg.next++
	}
	id := cbReg.next
	cbReg.m[id] = f
	return id
}

func cbLoad(id uint64) any {
	if id == 0 {
		return nil
	}
	cbReg.Lock()
	defer cbReg.Unlock()
	return cbReg.m[id]
}

func cbDelete(id uint64) {
	cbReg.Lock()
	defer cbReg.Unlock()
	delete(cbReg.m, id)
}

func idToStr(id uint64) string {
	return strconv.FormatUint(id, 36)
}

func strToID(s string) uint64 {
	if s == "" {
		return 0
	}
	n, err := strconv.ParseUint(s, 36, 64)
	if err != nil {
		return 0
	}
	return n
}

// iupgoRegistryAttr lists the _IUPGO_* keys set on an element so LDESTROY can
// free them; _IUP* names are skipped by IupGetAllAttributes.
const iupgoRegistryAttr = "_IUPGO_REGISTRY"

func storeCallback(ih Ihandle, key string, f any) {
	if old := strToID(ih.GetAttribute(key)); old != 0 {
		cbDelete(old)
	} else {
		list := ih.GetAttribute(iupgoRegistryAttr)
		switch {
		case list == "":
			ih.SetAttribute(iupgoRegistryAttr, key)
			iupSetCallback(uintptr(ih), "LDESTROY_CB", ldestroyCB)
		case !slices.Contains(strings.Split(list, ","), key):
			ih.SetAttribute(iupgoRegistryAttr, list+","+key)
		}
	}
	ih.SetAttribute(key, idToStr(cbStore(f)))
}

func loadCallback(ih Ihandle, key string) any {
	return cbLoad(strToID(ih.GetAttribute(key)))
}

func clearCallback(ih Ihandle, key string) {
	if old := strToID(ih.GetAttribute(key)); old != 0 {
		cbDelete(old)
		ih.SetAttribute(key, "")
	}
}

var ldestroyCB = purego.NewCallback(func(ih uintptr) int {
	h := Ihandle(ih)
	list := h.GetAttribute(iupgoRegistryAttr)
	if list == "" {
		return 0
	}
	for _, key := range strings.Split(list, ",") {
		if id := strToID(h.GetAttribute(key)); id != 0 {
			cbDelete(id)
			h.SetAttribute(key, "")
		}
	}
	h.SetAttribute(iupgoRegistryAttr, "")
	return 0
})

func GetCallback(ih Ihandle, name string) uintptr {
	return iupGetCallback(uintptr(ih), name)
}

func SetCallback(ih Ihandle, name string, fn interface{}) {
	if fn == nil {
		clearCallback(ih, "_IUPGO_"+name)
		iupSetCallback(uintptr(ih), name, 0)
		return
	}

	switch name {
	case "ACTION":
		switch v := fn.(type) {
		case ActionFunc:
			setActionFunc(ih, v)
		case ListActionFunc:
			setListActionFunc(ih, v)
		case TextActionFunc:
			setTextActionFunc(ih, v)
		case ToggleActionFunc:
			setToggleActionFunc(ih, v)
		case LinkActionFunc:
			setLinkActionFunc(ih, v)
		}
	case "DROPDOWN_CB":
		setDropDownFunc(ih, fn.(DropDownFunc))
	case "MULTISELECT_CB":
		setMultiselectFunc(ih, fn.(MultiselectFunc))
	case "EDIT_CB":
		setEditFunc(ih, fn.(EditFunc))
	case "DETACHED_CB":
		setDetachedFunc(ih, fn.(DetachedFunc))
	case "MAP_CB":
		setMapFunc(ih, fn.(MapFunc))
	case "UNMAP_CB":
		setUnmapFunc(ih, fn.(UnmapFunc))
	case "DESTROY_CB":
		setDestroyFunc(ih, fn.(DestroyFunc))
	case "GETFOCUS_CB":
		setGetFocusFunc(ih, fn.(GetFocusFunc))
	case "KILLFOCUS_CB":
		setKillFocusFunc(ih, fn.(KillFocusFunc))
	case "ENTERWINDOW_CB":
		setEnterWindowFunc(ih, fn.(EnterWindowFunc))
	case "LEAVEWINDOW_CB":
		setLeaveWindowFunc(ih, fn.(LeaveWindowFunc))
	case "VALUECHANGED_CB":
		switch v := fn.(type) {
		case ValueChangedFunc:
			setValueChangedFunc(ih, v)
		case TableValueChangedFunc:
			setTableValueChangedFunc(ih, v)
		}
	case "CLOSE_CB":
		switch v := fn.(type) {
		case CloseFunc:
			setCloseFunc(ih, v)
		case NotifyCloseFunc:
			setNotifyCloseFunc(ih, v)
		}
	case "HELP_CB":
		setHelpFunc(ih, fn.(HelpFunc))
	case "CANCEL_CB":
		setCancelFunc(ih, fn.(CancelFunc))
	case "COLORUPDATE_CB":
		setColorUpdateFunc(ih, fn.(ColorUpdateFunc))
	case "HIGHLIGHT_CB":
		setHighlightFunc(ih, fn.(HighlightFunc))
	case "LAYOUTUPDATE_CB":
		setLayoutUpdateFunc(ih, fn.(LayoutUpdateFunc))
	case "MENUCLOSE_CB":
		setMenuCloseFunc(ih, fn.(MenuCloseFunc))
	case "MENUOPEN_CB":
		setMenuOpenFunc(ih, fn.(MenuOpenFunc))
	case "SWAPBUFFERS_CB":
		setSwapBuffersFunc(ih, fn.(SwapBuffersFunc))
	case "THREAD_CB":
		setThreadFunc(ih, fn.(ThreadFunc))
	case "UPDATE_CB":
		setUpdateFunc(ih, fn.(UpdateFunc))
	case "CELL_CB":
		setCellFunc(ih, fn.(CellFunc))
	case "CHANGE_CB":
		setChangeFunc(ih, fn.(ChangeFunc))
	case "DRAG_CB":
		setDragFunc(ih, fn.(DragFunc))
	case "DROPMOTION_CB":
		setDropMotionFunc(ih, fn.(DropMotionFunc))
	case "DROPSHOW_CB":
		setDropShowFunc(ih, fn.(DropShowFunc))
	case "EXTENDED_CB":
		setExtendedFunc(ih, fn.(ExtendedFunc))
	case "EXTRABUTTON_CB":
		setExtraButtonFunc(ih, fn.(ExtraButtonFunc))
	case "FILE_CB":
		setFileFunc(ih, fn.(FileFunc))
	case "FOCUS_CB":
		setFocusFunc(ih, fn.(FocusFunc))
	case "GESTURE_CB":
		setGestureFunc(ih, fn.(GestureFunc))
	case "MASKFAIL_CB":
		setMaskFailFunc(ih, fn.(MaskFailFunc))
	case "MULTITOUCH_CB":
		setMultiTouchFunc(ih, fn.(MultiTouchFunc))
	case "MULTIUNSELECTION_CB":
		setMultiUnselectionFunc(ih, fn.(MultiUnselectionFunc))
	case "NOTIFY_CB":
		setNotifyFunc(ih, fn.(NotifyFunc))
	case "OPENCLOSE_CB":
		setOpenCloseFunc(ih, fn.(OpenCloseFunc))
	case "POSTMESSAGE_CB":
		setPostMessageFunc(ih, fn.(PostMessageFunc))
	case "REORDER_CB":
		setReorderFunc(ih, fn.(ReorderFunc))
	case "RESTORED_CB":
		setRestoredFunc(ih, fn.(RestoredFunc))
	case "RIGHTCLICK_CB":
		setRightClickFunc(ih, fn.(RightClickFunc))
	case "SELECT_CB":
		switch v := fn.(type) {
		case SelectFunc:
			setSelectFunc(ih, v)
		case PlotSelectFunc:
			setPlotSelectFunc(ih, v)
		}
	case "SHOW_CB":
		setShowFunc(ih, fn.(ShowFunc))
	case "SWITCH_CB":
		setSwitchFunc(ih, fn.(SwitchFunc))
	case "TABCLOSE_CB":
		setTabCloseFunc(ih, fn.(TabCloseFunc))
	case "EDITION_CB":
		switch v := fn.(type) {
		case TableEditionFunc:
			setTableEditionFunc(ih, v)
		case EditionFunc:
			setEditionFunc(ih, v)
		}
	case "SORT_CB":
		setTableSortFunc(ih, fn.(TableSortFunc))
	case "LINK_CB":
		setTextLinkFunc(ih, fn.(TextLinkFunc))
	case "THEMECHANGED_CB":
		setThemeChangedFunc(ih, fn.(ThemeChangedFunc))
	case "TOGGLEVALUE_CB":
		switch v := fn.(type) {
		case ToggleValueFunc:
			setToggleValueFunc(ih, v)
		case MatrixToggleValueFunc:
			setMatrixToggleValueFunc(ih, v)
		}
	case "TOUCH_CB":
		setTouchFunc(ih, fn.(TouchFunc))
	case "TRAYCLICK_CB":
		setTrayClickFunc(ih, fn.(TrayClickFunc))
	case "VALUECHANGING_CB":
		setValueChangingFunc(ih, fn.(ValueChangingFunc))
	case "K_ANY":
		setKAnyFunc(ih, fn.(KAnyFunc))
	case "TIPS_CB":
		setTipsFunc(ih, fn.(TipsFunc))
	case "BUTTON_CB":
		setButtonFunc(ih, fn.(ButtonFunc))
	case "MOTION_CB":
		setMotionFunc(ih, fn.(MotionFunc))
	case "WHEEL_CB":
		setWheelFunc(ih, fn.(WheelFunc))
	case "RESIZE_CB":
		setResizeFunc(ih, fn.(ResizeFunc))
	case "MOVE_CB":
		setMoveFunc(ih, fn.(MoveFunc))
	case "DBLCLICK_CB":
		setDblclickFunc(ih, fn.(DblclickFunc))
	case "CARET_CB":
		setCaretFunc(ih, fn.(CaretFunc))
	case "SPIN_CB":
		setSpinFunc(ih, fn.(SpinFunc))
	case "TABCHANGE_CB":
		setTabChangeFunc(ih, fn.(TabChangeFunc))
	case "TABCHANGEPOS_CB":
		setTabChangePosFunc(ih, fn.(TabChangePosFunc))
	case "DROPFILES_CB":
		setDropFilesFunc(ih, fn.(DropFilesFunc))
	case "SCROLL_CB":
		setScrollFunc(ih, fn.(ScrollFunc))
	case "KEYPRESS_CB":
		setKeyPressFunc(ih, fn.(KeyPressFunc))
	case "SELECTION_CB":
		setSelectionFunc(ih, fn.(SelectionFunc))
	case "MULTISELECTION_CB":
		setMultiSelectionFunc(ih, fn.(MultiSelectionFunc))
	case "BRANCHOPEN_CB":
		setBranchOpenFunc(ih, fn.(BranchOpenFunc))
	case "BRANCHCLOSE_CB":
		setBranchCloseFunc(ih, fn.(BranchCloseFunc))
	case "EXECUTELEAF_CB":
		setExecuteLeafFunc(ih, fn.(ExecuteLeafFunc))
	case "EXECUTEBRANCH_CB":
		setExecuteBranchFunc(ih, fn.(ExecuteBranchFunc))
	case "SHOWRENAME_CB":
		setShowRenameFunc(ih, fn.(ShowRenameFunc))
	case "RENAME_CB":
		setRenameFunc(ih, fn.(RenameFunc))
	case "NODEREMOVED_CB":
		setNodeRemovedFunc(ih, fn.(NodeRemovedFunc))
	case "DRAGDROP_CB":
		setDragDropFunc(ih, fn.(DragDropFunc))
	case "DRAGBEGIN_CB":
		setDragBeginFunc(ih, fn.(DragBeginFunc))
	case "DRAGDATASIZE_CB":
		setDragDataSizeFunc(ih, fn.(DragDataSizeFunc))
	case "DRAGDATA_CB":
		setDragDataFunc(ih, fn.(DragDataFunc))
	case "DRAGEND_CB":
		setDragEndFunc(ih, fn.(DragEndFunc))
	case "DROPDATA_CB":
		setDropDataFunc(ih, fn.(DropDataFunc))
	case "BUSY_CB":
		setBusyFunc(ih, fn.(BusyFunc))
	case "CLICKSAMPLE_CB":
		setPlotClickSampleFunc(ih, fn.(PlotClickSampleFunc))
	case "CLICKSEGMENT_CB":
		setPlotClickSegmentFunc(ih, fn.(PlotClickSegmentFunc))
	case "CLICK_CB":
		setClickFunc(ih, fn.(ClickFunc))
	case "COLRESIZE_CB":
		setColResizeFunc(ih, fn.(ColResizeFunc))
	case "COMPLETED_CB":
		setCompletedFunc(ih, fn.(CompletedFunc))
	case "DELETEBEGIN_CB":
		setPlotDeleteBeginFunc(ih, fn.(PlotDeleteBeginFunc))
	case "DELETEEND_CB":
		setPlotDeleteEndFunc(ih, fn.(PlotDeleteEndFunc))
	case "DELETE_CB":
		setPlotDeleteFunc(ih, fn.(PlotDeleteFunc))
	case "DRAWSAMPLE_CB":
		setPlotDrawSampleFunc(ih, fn.(PlotDrawSampleFunc))
	case "DRAW_CB":
		switch v := fn.(type) {
		case CellsDrawFunc:
			setCellsDrawFunc(ih, v)
		case MatrixDrawFunc:
			setMatrixDrawFunc(ih, v)
		}
	case "DROPCHECK_CB":
		setDropCheckFunc(ih, fn.(DropCheckFunc))
	case "DROPSELECT_CB":
		setDropSelectFunc(ih, fn.(DropSelectFunc))
	case "DROP_CB":
		setMatrixDropFunc(ih, fn.(MatrixDropFunc))
	case "DSPROPERTIESCHANGED_CB":
		setPlotDSPropertiesChangedFunc(ih, fn.(PlotDSPropertiesChangedFunc))
	case "DSPROPERTIESVALIDATE_CB":
		setPlotDSPropertiesValidateFunc(ih, fn.(PlotDSPropertiesValidateFunc))
	case "EDITBEGIN_CB":
		setEditBeginFunc(ih, fn.(EditBeginFunc))
	case "EDITCLICK_CB":
		setEditClickFunc(ih, fn.(EditClickFunc))
	case "EDITEND_CB":
		setEditEndFunc(ih, fn.(EditEndFunc))
	case "EDITMOUSEMOVE_CB":
		setEditMouseMoveFunc(ih, fn.(EditMouseMoveFunc))
	case "EDITRELEASE_CB":
		setEditReleaseFunc(ih, fn.(EditReleaseFunc))
	case "EDITSAMPLE_CB":
		setPlotEditSampleFunc(ih, fn.(PlotEditSampleFunc))
	case "ENTERITEM_CB":
		setEnterItemFunc(ih, fn.(EnterItemFunc))
	case "ERROR_CB":
		setErrorFunc(ih, fn.(ErrorFunc))
	case "FLAT_ACTION":
		switch v := fn.(type) {
		case FlatActionFunc:
			setFlatActionFunc(ih, v)
		case FlatListActionFunc:
			setFlatListActionFunc(ih, v)
		case FlatToggleActionFunc:
			setFlatToggleActionFunc(ih, v)
		}
	case "HEIGHT_CB":
		setHeightFunc(ih, fn.(HeightFunc))
	case "HSPAN_CB":
		setHSpanFunc(ih, fn.(HSpanFunc))
	case "LEAVEITEM_CB":
		setLeaveItemFunc(ih, fn.(LeaveItemFunc))
	case "LISTDRAW_CB":
		setListDrawFunc(ih, fn.(ListDrawFunc))
	case "LISTEDITION_CB":
		setListEditionFunc(ih, fn.(ListEditionFunc))
	case "LISTINSERT_CB":
		setListInsertFunc(ih, fn.(ListInsertFunc))
	case "LISTRELEASE_CB":
		setListReleaseFunc(ih, fn.(ListReleaseFunc))
	case "LISTREMOVE_CB":
		setListRemoveFunc(ih, fn.(ListRemoveFunc))
	case "MARKEDIT_CB":
		setMarkEditFunc(ih, fn.(MarkEditFunc))
	case "MARK_CB":
		setMarkFunc(ih, fn.(MarkFunc))
	case "MENUCONTEXTCLOSE_CB":
		switch v := fn.(type) {
		case MenuContextCloseFunc:
			setMenuContextCloseFunc(ih, v)
		case PlotMenuContextCloseFunc:
			setPlotMenuContextCloseFunc(ih, v)
		}
	case "MENUCONTEXT_CB":
		switch v := fn.(type) {
		case MenuContextFunc:
			setMenuContextFunc(ih, v)
		case PlotMenuContextFunc:
			setPlotMenuContextFunc(ih, v)
		}
	case "MENUDROP_CB":
		setMenuDropFunc(ih, fn.(MenuDropFunc))
	case "MOUSECLICK_CB":
		setMouseClickFunc(ih, fn.(MouseClickFunc))
	case "MOUSEMOTION_CB":
		setMouseMotionFunc(ih, fn.(MouseMotionFunc))
	case "MOUSEMOVE_CB":
		setMatrixMouseMoveFunc(ih, fn.(MatrixMouseMoveFunc))
	case "NAVIGATE_CB":
		setNavigateFunc(ih, fn.(NavigateFunc))
	case "NCOLS_CB":
		setNColsFunc(ih, fn.(NColsFunc))
	case "NEWWINDOW_CB":
		setNewWindowFunc(ih, fn.(NewWindowFunc))
	case "NLINES_CB":
		setNLinesFunc(ih, fn.(NLinesFunc))
	case "NUMERICSETVALUE_CB":
		setNumericSetValueFunc(ih, fn.(NumericSetValueFunc))
	case "PASTESIZE_CB":
		setPasteSizeFunc(ih, fn.(PasteSizeFunc))
	case "PLOTBUTTON_CB":
		setPlotButtonFunc(ih, fn.(PlotButtonFunc))
	case "PLOTMOTION_CB":
		setPlotMotionFunc(ih, fn.(PlotMotionFunc))
	case "PROPERTIESCHANGED_CB":
		setPlotPropertiesChangedFunc(ih, fn.(PlotPropertiesChangedFunc))
	case "PROPERTIESVALIDATE_CB":
		setPlotPropertiesValidateFunc(ih, fn.(PlotPropertiesValidateFunc))
	case "RELEASE_CB":
		setReleaseFunc(ih, fn.(ReleaseFunc))
	case "RESIZEMATRIX_CB":
		setResizeMatrixFunc(ih, fn.(ResizeMatrixFunc))
	case "SCROLLING_CB":
		setScrollingFunc(ih, fn.(ScrollingFunc))
	case "SCROLLTOP_CB":
		setScrollTopFunc(ih, fn.(ScrollTopFunc))
	case "SELECTBEGIN_CB":
		setPlotSelectBeginFunc(ih, fn.(PlotSelectBeginFunc))
	case "SELECTEND_CB":
		setPlotSelectEndFunc(ih, fn.(PlotSelectEndFunc))
	case "SORTCOLUMNCOMPARE_CB":
		setSortColumnCompareFunc(ih, fn.(SortColumnCompareFunc))
	case "VALUE_EDIT_CB":
		setValueEditFunc(ih, fn.(ValueEditFunc))
	case "VSPAN_CB":
		setVSpanFunc(ih, fn.(VSpanFunc))
	case "WIDTH_CB":
		setWidthFunc(ih, fn.(WidthFunc))
	case "BGCOLOR_CB":
		setBgColorFunc(ih, fn.(BgColorFunc))
	case "FGCOLOR_CB":
		setFgColorFunc(ih, fn.(FgColorFunc))
	case "TYPE_CB":
		setMatrixTypeFunc(ih, fn.(MatrixTypeFunc))
	case "FONT_CB":
		setMatrixFontFunc(ih, fn.(MatrixFontFunc))
	case "TRANSLATEVALUE_CB":
		setTranslateValueFunc(ih, fn.(TranslateValueFunc))
	case "NUMERICGETVALUE_CB":
		setNumericGetValueFunc(ih, fn.(NumericGetValueFunc))
	case "PREDRAW_CB":
		setPlotPreDrawFunc(ih, fn.(PlotDrawFunc))
	case "POSTDRAW_CB":
		setPlotPostDrawFunc(ih, fn.(PlotDrawFunc))
	case "XTICKFORMATNUMBER_CB":
		setPlotXTickFormatNumberFunc(ih, fn.(PlotTickFormatNumberFunc))
	case "YTICKFORMATNUMBER_CB":
		setPlotYTickFormatNumberFunc(ih, fn.(PlotTickFormatNumberFunc))
	case "ACTION_CB":
		switch v := fn.(type) {
		case TimerActionFunc:
			setTimerActionFunc(ih, v)
		case MatrixActionFunc:
			setMatrixActionFunc(ih, v)
		case MatrixListActionFunc:
			setMatrixListActionFunc(ih, v)
		}
	case "VALUE_CB":
		switch v := fn.(type) {
		case ListValueFunc:
			setListValueFunc(ih, v)
		case TableValueFunc:
			setTableValueFunc(ih, v)
		case MatrixValueFunc:
			setMatrixValueFunc(ih, v)
		}
	case "IMAGE_CB":
		switch v := fn.(type) {
		case ListImageFunc:
			setListImageFunc(ih, v)
		case TableImageFunc:
			setTableImageFunc(ih, v)
		}
	}
}

// Global (IupSetFunction) callbacks are per-process; plain package vars hold them (no cgo.Handle without cgo).
var (
	globalIdle  IdleFunc
	globalEntry EntryPointFunc
	globalExit  ExitFunc
)

type IdleFunc func() int

type EntryPointFunc func()

type ExitFunc func()

var idleCB = purego.NewCallback(func() int {
	if globalIdle != nil {
		return globalIdle()
	}
	return 0
})

var entryPointCB = purego.NewCallback(func() {
	if globalEntry != nil {
		globalEntry()
	}
})

var exitCB = purego.NewCallback(func() {
	if globalExit != nil {
		globalExit()
	}
})

func GetFunction(name string) uintptr {
	return iupGetFunction(name)
}

func SetFunction(name string, fn interface{}) {
	if fn == nil {
		switch name {
		case "IDLE_ACTION":
			globalIdle = nil
		case "ENTRY_POINT":
			globalEntry = nil
		case "EXIT_CB":
			globalExit = nil
		}
		iupSetFunction(name, 0)
		return
	}

	switch name {
	case "IDLE_ACTION":
		globalIdle = fn.(IdleFunc)
		iupSetFunction("IDLE_ACTION", idleCB)
	case "ENTRY_POINT":
		globalEntry = fn.(EntryPointFunc)
		iupSetFunction("ENTRY_POINT", entryPointCB)
	case "EXIT_CB":
		globalExit = fn.(ExitFunc)
		iupSetFunction("EXIT_CB", exitCB)
	}
}

// One trampoline per func(Ihandle) int name; callbacks store the base func type
// (not the named type) so the shared trampoline can dispatch.
var ihIntCB = map[string]uintptr{}

func init() {
	for _, name := range []string{
		"ACTION", "MAP_CB", "UNMAP_CB", "DESTROY_CB",
		"GETFOCUS_CB", "KILLFOCUS_CB", "ENTERWINDOW_CB", "LEAVEWINDOW_CB",
		"VALUECHANGED_CB", "CLOSE_CB", "HELP_CB",
		"CANCEL_CB", "COLORUPDATE_CB", "HIGHLIGHT_CB", "LAYOUTUPDATE_CB",
		"MENUCLOSE_CB", "MENUOPEN_CB", "SWAPBUFFERS_CB", "THREAD_CB", "UPDATE_CB",
	} {
		key := "_IUPGO_" + name
		ihIntCB[name] = purego.NewCallback(func(ih uintptr) int {
			if f, ok := loadCallback(Ihandle(ih), key).(func(Ihandle) int); ok {
				return f(Ihandle(ih))
			}
			return 0
		})
	}
}

func setIhIntFunc(ih Ihandle, name string, f func(Ihandle) int) {
	storeCallback(ih, "_IUPGO_"+name, f)
	iupSetCallback(uintptr(ih), name, ihIntCB[name])
}

type ActionFunc func(Ihandle) int

func setActionFunc(ih Ihandle, f ActionFunc) { setIhIntFunc(ih, "ACTION", (func(Ihandle) int)(f)) }

type MapFunc func(Ihandle) int

func setMapFunc(ih Ihandle, f MapFunc) { setIhIntFunc(ih, "MAP_CB", (func(Ihandle) int)(f)) }

type UnmapFunc func(Ihandle) int

func setUnmapFunc(ih Ihandle, f UnmapFunc) { setIhIntFunc(ih, "UNMAP_CB", (func(Ihandle) int)(f)) }

type DestroyFunc func(Ihandle) int

func setDestroyFunc(ih Ihandle, f DestroyFunc) {
	setIhIntFunc(ih, "DESTROY_CB", (func(Ihandle) int)(f))
}

type GetFocusFunc func(Ihandle) int

func setGetFocusFunc(ih Ihandle, f GetFocusFunc) {
	setIhIntFunc(ih, "GETFOCUS_CB", (func(Ihandle) int)(f))
}

type KillFocusFunc func(Ihandle) int

func setKillFocusFunc(ih Ihandle, f KillFocusFunc) {
	setIhIntFunc(ih, "KILLFOCUS_CB", (func(Ihandle) int)(f))
}

type EnterWindowFunc func(Ihandle) int

func setEnterWindowFunc(ih Ihandle, f EnterWindowFunc) {
	setIhIntFunc(ih, "ENTERWINDOW_CB", (func(Ihandle) int)(f))
}

type LeaveWindowFunc func(Ihandle) int

func setLeaveWindowFunc(ih Ihandle, f LeaveWindowFunc) {
	setIhIntFunc(ih, "LEAVEWINDOW_CB", (func(Ihandle) int)(f))
}

type ValueChangedFunc func(Ihandle) int

func setValueChangedFunc(ih Ihandle, f ValueChangedFunc) {
	setIhIntFunc(ih, "VALUECHANGED_CB", (func(Ihandle) int)(f))
}

type CloseFunc func(Ihandle) int

func setCloseFunc(ih Ihandle, f CloseFunc) { setIhIntFunc(ih, "CLOSE_CB", (func(Ihandle) int)(f)) }

type HelpFunc func(Ihandle) int

func setHelpFunc(ih Ihandle, f HelpFunc) { setIhIntFunc(ih, "HELP_CB", (func(Ihandle) int)(f)) }

type CancelFunc func(Ihandle) int

func setCancelFunc(ih Ihandle, f CancelFunc) { setIhIntFunc(ih, "CANCEL_CB", (func(Ihandle) int)(f)) }

type ColorUpdateFunc func(Ihandle) int

func setColorUpdateFunc(ih Ihandle, f ColorUpdateFunc) {
	setIhIntFunc(ih, "COLORUPDATE_CB", (func(Ihandle) int)(f))
}

type HighlightFunc func(Ihandle) int

func setHighlightFunc(ih Ihandle, f HighlightFunc) {
	setIhIntFunc(ih, "HIGHLIGHT_CB", (func(Ihandle) int)(f))
}

type LayoutUpdateFunc func(Ihandle) int

func setLayoutUpdateFunc(ih Ihandle, f LayoutUpdateFunc) {
	setIhIntFunc(ih, "LAYOUTUPDATE_CB", (func(Ihandle) int)(f))
}

type MenuCloseFunc func(Ihandle) int

func setMenuCloseFunc(ih Ihandle, f MenuCloseFunc) {
	setIhIntFunc(ih, "MENUCLOSE_CB", (func(Ihandle) int)(f))
}

type MenuOpenFunc func(Ihandle) int

func setMenuOpenFunc(ih Ihandle, f MenuOpenFunc) {
	setIhIntFunc(ih, "MENUOPEN_CB", (func(Ihandle) int)(f))
}

type SwapBuffersFunc func(Ihandle) int

func setSwapBuffersFunc(ih Ihandle, f SwapBuffersFunc) {
	setIhIntFunc(ih, "SWAPBUFFERS_CB", (func(Ihandle) int)(f))
}

type ThreadFunc func(Ihandle) int

func setThreadFunc(ih Ihandle, f ThreadFunc) { setIhIntFunc(ih, "THREAD_CB", (func(Ihandle) int)(f)) }

type UpdateFunc func(Ihandle) int

func setUpdateFunc(ih Ihandle, f UpdateFunc) { setIhIntFunc(ih, "UPDATE_CB", (func(Ihandle) int)(f)) }

type TimerActionFunc func(Ihandle) int

var timerActionCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TIMER_ACTION").(func(Ihandle) int); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setTimerActionFunc(ih Ihandle, f TimerActionFunc) {
	storeCallback(ih, "_IUPGO_TIMER_ACTION", (func(Ihandle) int)(f))
	iupSetCallback(uintptr(ih), "ACTION_CB", timerActionCB)
}

// cReturnStr returns a C pointer to s from a rotating 16-buffer pool: IUP reads a
// callback's char* result synchronously and never frees it, so we keep the last few alive.
const cReturnPoolSize = 16

var (
	cReturnPool [cReturnPoolSize][]byte
	cReturnIdx  int
)

func cReturnStr(s string) uintptr {
	if s == "" {
		return 0
	}
	b := append([]byte(s), 0)
	cReturnPool[cReturnIdx] = b
	cReturnIdx = (cReturnIdx + 1) % cReturnPoolSize
	return uintptr(unsafe.Pointer(&b[0]))
}

type ListValueFunc func(ih Ihandle, pos int) string

var listValueCB = purego.NewCallback(func(ih uintptr, pos int32) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LIST_VALUE_CB").(ListValueFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(pos)))
	}
	return 0
})

func setListValueFunc(ih Ihandle, f ListValueFunc) {
	storeCallback(ih, "_IUPGO_LIST_VALUE_CB", f)
	iupSetCallback(uintptr(ih), "VALUE_CB", listValueCB)
}

type ListImageFunc func(ih Ihandle, pos int) string

var listImageCB = purego.NewCallback(func(ih uintptr, pos int32) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LIST_IMAGE_CB").(ListImageFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(pos)))
	}
	return 0
})

func setListImageFunc(ih Ihandle, f ListImageFunc) {
	storeCallback(ih, "_IUPGO_LIST_IMAGE_CB", f)
	iupSetCallback(uintptr(ih), "IMAGE_CB", listImageCB)
}

type TableValueFunc func(ih Ihandle, lin, col int) string

var tableValueCB = purego.NewCallback(func(ih uintptr, lin, col int32) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TABLE_VALUE_CB").(TableValueFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(lin), int(col)))
	}
	return 0
})

func setTableValueFunc(ih Ihandle, f TableValueFunc) {
	storeCallback(ih, "_IUPGO_TABLE_VALUE_CB", f)
	iupSetCallback(uintptr(ih), "VALUE_CB", tableValueCB)
}

type TableImageFunc func(ih Ihandle, lin, col int) string

var tableImageCB = purego.NewCallback(func(ih uintptr, lin, col int32) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TABLE_IMAGE_CB").(TableImageFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(lin), int(col)))
	}
	return 0
})

func setTableImageFunc(ih Ihandle, f TableImageFunc) {
	storeCallback(ih, "_IUPGO_TABLE_IMAGE_CB", f)
	iupSetCallback(uintptr(ih), "IMAGE_CB", tableImageCB)
}

type KAnyFunc func(Ihandle, int) int

var kAnyCB = purego.NewCallback(func(ih uintptr, c int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_K_ANY").(KAnyFunc); ok {
		return f(Ihandle(ih), int(c))
	}
	return 0
})

func setKAnyFunc(ih Ihandle, f KAnyFunc) {
	storeCallback(ih, "_IUPGO_K_ANY", f)
	iupSetCallback(uintptr(ih), "K_ANY", kAnyCB)
}

type KeyPressFunc func(ih Ihandle, c, press int) int

var keyPressCB = purego.NewCallback(func(ih uintptr, c, press int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_KEYPRESS_CB").(KeyPressFunc); ok {
		return f(Ihandle(ih), int(c), int(press))
	}
	return 0
})

func setKeyPressFunc(ih Ihandle, f KeyPressFunc) {
	storeCallback(ih, "_IUPGO_KEYPRESS_CB", f)
	iupSetCallback(uintptr(ih), "KEYPRESS_CB", keyPressCB)
}

type TipsFunc func(ih Ihandle, x, y int) int

var tipsCB = purego.NewCallback(func(ih uintptr, x, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TIPS_CB").(TipsFunc); ok {
		return f(Ihandle(ih), int(x), int(y))
	}
	return 0
})

func setTipsFunc(ih Ihandle, f TipsFunc) {
	storeCallback(ih, "_IUPGO_TIPS_CB", f)
	iupSetCallback(uintptr(ih), "TIPS_CB", tipsCB)
}

type MoveFunc func(ih Ihandle, x, y int) int

var moveCB = purego.NewCallback(func(ih uintptr, x, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MOVE_CB").(MoveFunc); ok {
		return f(Ihandle(ih), int(x), int(y))
	}
	return 0
})

func setMoveFunc(ih Ihandle, f MoveFunc) {
	storeCallback(ih, "_IUPGO_MOVE_CB", f)
	iupSetCallback(uintptr(ih), "MOVE_CB", moveCB)
}

type ResizeFunc func(ih Ihandle, width, height int) int

var resizeCB = purego.NewCallback(func(ih uintptr, width, height int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_RESIZE_CB").(ResizeFunc); ok {
		return f(Ihandle(ih), int(width), int(height))
	}
	return 0
})

func setResizeFunc(ih Ihandle, f ResizeFunc) {
	storeCallback(ih, "_IUPGO_RESIZE_CB", f)
	iupSetCallback(uintptr(ih), "RESIZE_CB", resizeCB)
}

type SpinFunc func(ih Ihandle, inc int) int

var spinCB = purego.NewCallback(func(ih uintptr, inc int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SPIN_CB").(SpinFunc); ok {
		return f(Ihandle(ih), int(inc))
	}
	return 0
})

func setSpinFunc(ih Ihandle, f SpinFunc) {
	storeCallback(ih, "_IUPGO_SPIN_CB", f)
	iupSetCallback(uintptr(ih), "SPIN_CB", spinCB)
}

type CaretFunc func(ih Ihandle, lin, col, pos int) int

var caretCB = purego.NewCallback(func(ih uintptr, lin, col, pos int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_CARET_CB").(CaretFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), int(pos))
	}
	return 0
})

func setCaretFunc(ih Ihandle, f CaretFunc) {
	storeCallback(ih, "_IUPGO_CARET_CB", f)
	iupSetCallback(uintptr(ih), "CARET_CB", caretCB)
}

type TabChangePosFunc func(ih Ihandle, newPos, oldPos int) int

var tabChangePosCB = purego.NewCallback(func(ih uintptr, newPos, oldPos int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TABCHANGEPOS_CB").(TabChangePosFunc); ok {
		return f(Ihandle(ih), int(newPos), int(oldPos))
	}
	return 0
})

func setTabChangePosFunc(ih Ihandle, f TabChangePosFunc) {
	storeCallback(ih, "_IUPGO_TABCHANGEPOS_CB", f)
	iupSetCallback(uintptr(ih), "TABCHANGEPOS_CB", tabChangePosCB)
}

type TabChangeFunc func(ih, newTab, oldTab Ihandle) int

var tabChangeCB = purego.NewCallback(func(ih, newTab, oldTab uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TABCHANGE_CB").(TabChangeFunc); ok {
		return f(Ihandle(ih), Ihandle(newTab), Ihandle(oldTab))
	}
	return 0
})

func setTabChangeFunc(ih Ihandle, f TabChangeFunc) {
	storeCallback(ih, "_IUPGO_TABCHANGE_CB", f)
	iupSetCallback(uintptr(ih), "TABCHANGE_CB", tabChangeCB)
}

type DblclickFunc func(ih Ihandle, item int, text string) int

var dblclickCB = purego.NewCallback(func(ih uintptr, item int32, text uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DBLCLICK_CB").(DblclickFunc); ok {
		return f(Ihandle(ih), int(item), goString(text))
	}
	return 0
})

func setDblclickFunc(ih Ihandle, f DblclickFunc) {
	storeCallback(ih, "_IUPGO_DBLCLICK_CB", f)
	iupSetCallback(uintptr(ih), "DBLCLICK_CB", dblclickCB)
}

type MotionFunc func(ih Ihandle, x, y int, status string) int

var motionCB = purego.NewCallback(func(ih uintptr, x, y int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MOTION_CB").(MotionFunc); ok {
		return f(Ihandle(ih), int(x), int(y), goString(status))
	}
	return 0
})

func setMotionFunc(ih Ihandle, f MotionFunc) {
	storeCallback(ih, "_IUPGO_MOTION_CB", f)
	iupSetCallback(uintptr(ih), "MOTION_CB", motionCB)
}

type ButtonFunc func(Ihandle, int, int, int, int, string) int

var buttonCB = purego.NewCallback(func(ih uintptr, button, pressed, x, y int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_BUTTON_CB").(ButtonFunc); ok {
		return f(Ihandle(ih), int(button), int(pressed), int(x), int(y), goString(status))
	}
	return 0
})

func setButtonFunc(ih Ihandle, f ButtonFunc) {
	storeCallback(ih, "_IUPGO_BUTTON_CB", f)
	iupSetCallback(uintptr(ih), "BUTTON_CB", buttonCB)
}

type DropFilesFunc func(Ihandle, string, int, int, int) int

var dropFilesCB = purego.NewCallback(func(ih, filename uintptr, num, x, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROPFILES_CB").(DropFilesFunc); ok {
		return f(Ihandle(ih), goString(filename), int(num), int(x), int(y))
	}
	return 0
})

func setDropFilesFunc(ih Ihandle, f DropFilesFunc) {
	storeCallback(ih, "_IUPGO_DROPFILES_CB", f)
	iupSetCallback(uintptr(ih), "DROPFILES_CB", dropFilesCB)
}

type WheelFunc func(ih Ihandle, delta float64, x, y int, status string) int

var wheelCB = purego.NewCallback(func(ih uintptr, delta float32, x, y int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_WHEEL_CB").(WheelFunc); ok {
		return f(Ihandle(ih), float64(delta), int(x), int(y), goString(status))
	}
	return 0
})

func setWheelFunc(ih Ihandle, f WheelFunc) {
	storeCallback(ih, "_IUPGO_WHEEL_CB", f)
	iupSetCallback(uintptr(ih), "WHEEL_CB", wheelCB)
}

type ScrollFunc func(ih Ihandle, op int, posx, posy float64) int

var scrollCB = purego.NewCallback(func(ih uintptr, op int32, posx, posy float32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SCROLL_CB").(ScrollFunc); ok {
		return f(Ihandle(ih), int(op), float64(posx), float64(posy))
	}
	return 0
})

func setScrollFunc(ih Ihandle, f ScrollFunc) {
	storeCallback(ih, "_IUPGO_SCROLL_CB", f)
	iupSetCallback(uintptr(ih), "SCROLL_CB", scrollCB)
}

type ListActionFunc func(ih Ihandle, text string, item, state int) int

var listActionCB = purego.NewCallback(func(ih, text uintptr, item, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LIST_ACTION").(ListActionFunc); ok {
		return f(Ihandle(ih), goString(text), int(item), int(state))
	}
	return 0
})

func setListActionFunc(ih Ihandle, f ListActionFunc) {
	storeCallback(ih, "_IUPGO_LIST_ACTION", f)
	iupSetCallback(uintptr(ih), "ACTION", listActionCB)
}

type TextActionFunc func(ih Ihandle, ch int, newValue string) int

var textActionCB = purego.NewCallback(func(ih uintptr, c int32, newValue uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TEXT_ACTION").(TextActionFunc); ok {
		return f(Ihandle(ih), int(c), goString(newValue))
	}
	return 0
})

func setTextActionFunc(ih Ihandle, f TextActionFunc) {
	storeCallback(ih, "_IUPGO_TEXT_ACTION", f)
	iupSetCallback(uintptr(ih), "ACTION", textActionCB)
}

type ToggleActionFunc func(ih Ihandle, state int) int

var toggleActionCB = purego.NewCallback(func(ih uintptr, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TOGGLE_ACTION").(ToggleActionFunc); ok {
		return f(Ihandle(ih), int(state))
	}
	return 0
})

func setToggleActionFunc(ih Ihandle, f ToggleActionFunc) {
	storeCallback(ih, "_IUPGO_TOGGLE_ACTION", f)
	iupSetCallback(uintptr(ih), "ACTION", toggleActionCB)
}

type LinkActionFunc func(ih Ihandle, url string) int

var linkActionCB = purego.NewCallback(func(ih, url uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LINK_ACTION").(LinkActionFunc); ok {
		return f(Ihandle(ih), goString(url))
	}
	return 0
})

func setLinkActionFunc(ih Ihandle, f LinkActionFunc) {
	storeCallback(ih, "_IUPGO_LINK_ACTION", f)
	iupSetCallback(uintptr(ih), "ACTION", linkActionCB)
}

type DropDownFunc func(ih Ihandle, state int) int

var dropDownCB = purego.NewCallback(func(ih uintptr, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROPDOWN_CB").(DropDownFunc); ok {
		return f(Ihandle(ih), int(state))
	}
	return 0
})

func setDropDownFunc(ih Ihandle, f DropDownFunc) {
	storeCallback(ih, "_IUPGO_DROPDOWN_CB", f)
	iupSetCallback(uintptr(ih), "DROPDOWN_CB", dropDownCB)
}

type MultiselectFunc func(ih Ihandle, text string) int

var multiselectCB = purego.NewCallback(func(ih, text uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MULTISELECT_CB").(MultiselectFunc); ok {
		return f(Ihandle(ih), goString(text))
	}
	return 0
})

func setMultiselectFunc(ih Ihandle, f MultiselectFunc) {
	storeCallback(ih, "_IUPGO_MULTISELECT_CB", f)
	iupSetCallback(uintptr(ih), "MULTISELECT_CB", multiselectCB)
}

type EditFunc func(ih Ihandle, item int, text string) int

var editCB = purego.NewCallback(func(ih uintptr, item int32, text uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDIT_CB").(EditFunc); ok {
		return f(Ihandle(ih), int(item), goString(text))
	}
	return 0
})

func setEditFunc(ih Ihandle, f EditFunc) {
	storeCallback(ih, "_IUPGO_EDIT_CB", f)
	iupSetCallback(uintptr(ih), "EDIT_CB", editCB)
}

type DetachedFunc func(Ihandle, Ihandle, int, int) int

var detachedCB = purego.NewCallback(func(ih, newParent uintptr, x, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DETACHED_CB").(DetachedFunc); ok {
		return f(Ihandle(ih), Ihandle(newParent), int(x), int(y))
	}
	return 0
})

func setDetachedFunc(ih Ihandle, f DetachedFunc) {
	storeCallback(ih, "_IUPGO_DETACHED_CB", f)
	iupSetCallback(uintptr(ih), "DETACHED_CB", detachedCB)
}

type SelectionFunc func(ih Ihandle, id, status int) int

var selectionCB = purego.NewCallback(func(ih uintptr, id, status int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SELECTION_CB").(SelectionFunc); ok {
		return f(Ihandle(ih), int(id), int(status))
	}
	return 0
})

func setSelectionFunc(ih Ihandle, f SelectionFunc) {
	storeCallback(ih, "_IUPGO_SELECTION_CB", f)
	iupSetCallback(uintptr(ih), "SELECTION_CB", selectionCB)
}

type MultiSelectionFunc func(ih Ihandle, ids []int, n int) int

var multiSelectionCB = purego.NewCallback(func(ih, ids uintptr, n int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MULTISELECTION_CB").(MultiSelectionFunc); ok {
		cints := unsafe.Slice((*int32)(unsafe.Pointer(ids)), int(n))
		goIds := make([]int, n)
		for i, v := range cints {
			goIds[i] = int(v)
		}
		return f(Ihandle(ih), goIds, int(n))
	}
	return 0
})

func setMultiSelectionFunc(ih Ihandle, f MultiSelectionFunc) {
	storeCallback(ih, "_IUPGO_MULTISELECTION_CB", f)
	iupSetCallback(uintptr(ih), "MULTISELECTION_CB", multiSelectionCB)
}

type BranchOpenFunc func(ih Ihandle, id int) int

var branchOpenCB = purego.NewCallback(func(ih uintptr, id int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_BRANCHOPEN_CB").(BranchOpenFunc); ok {
		return f(Ihandle(ih), int(id))
	}
	return 0
})

func setBranchOpenFunc(ih Ihandle, f BranchOpenFunc) {
	storeCallback(ih, "_IUPGO_BRANCHOPEN_CB", f)
	iupSetCallback(uintptr(ih), "BRANCHOPEN_CB", branchOpenCB)
}

type BranchCloseFunc func(ih Ihandle, id int) int

var branchCloseCB = purego.NewCallback(func(ih uintptr, id int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_BRANCHCLOSE_CB").(BranchCloseFunc); ok {
		return f(Ihandle(ih), int(id))
	}
	return 0
})

func setBranchCloseFunc(ih Ihandle, f BranchCloseFunc) {
	storeCallback(ih, "_IUPGO_BRANCHCLOSE_CB", f)
	iupSetCallback(uintptr(ih), "BRANCHCLOSE_CB", branchCloseCB)
}

type ExecuteLeafFunc func(ih Ihandle, id int) int

var executeLeafCB = purego.NewCallback(func(ih uintptr, id int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EXECUTELEAF_CB").(ExecuteLeafFunc); ok {
		return f(Ihandle(ih), int(id))
	}
	return 0
})

func setExecuteLeafFunc(ih Ihandle, f ExecuteLeafFunc) {
	storeCallback(ih, "_IUPGO_EXECUTELEAF_CB", f)
	iupSetCallback(uintptr(ih), "EXECUTELEAF_CB", executeLeafCB)
}

type ExecuteBranchFunc func(ih Ihandle, id int) int

var executeBranchCB = purego.NewCallback(func(ih uintptr, id int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EXECUTEBRANCH_CB").(ExecuteBranchFunc); ok {
		return f(Ihandle(ih), int(id))
	}
	return 0
})

func setExecuteBranchFunc(ih Ihandle, f ExecuteBranchFunc) {
	storeCallback(ih, "_IUPGO_EXECUTEBRANCH_CB", f)
	iupSetCallback(uintptr(ih), "EXECUTEBRANCH_CB", executeBranchCB)
}

type ShowRenameFunc func(ih Ihandle, id int) int

var showRenameCB = purego.NewCallback(func(ih uintptr, id int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SHOWRENAME_CB").(ShowRenameFunc); ok {
		return f(Ihandle(ih), int(id))
	}
	return 0
})

func setShowRenameFunc(ih Ihandle, f ShowRenameFunc) {
	storeCallback(ih, "_IUPGO_SHOWRENAME_CB", f)
	iupSetCallback(uintptr(ih), "SHOWRENAME_CB", showRenameCB)
}

type RenameFunc func(ih Ihandle, id int, title string) int

var renameCB = purego.NewCallback(func(ih uintptr, id int32, title uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_RENAME_CB").(RenameFunc); ok {
		return f(Ihandle(ih), int(id), goString(title))
	}
	return 0
})

func setRenameFunc(ih Ihandle, f RenameFunc) {
	storeCallback(ih, "_IUPGO_RENAME_CB", f)
	iupSetCallback(uintptr(ih), "RENAME_CB", renameCB)
}

type NodeRemovedFunc func(ih Ihandle, userId uintptr) int

var nodeRemovedCB = purego.NewCallback(func(ih, userData uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NODEREMOVED_CB").(NodeRemovedFunc); ok {
		return f(Ihandle(ih), userData)
	}
	return 0
})

func setNodeRemovedFunc(ih Ihandle, f NodeRemovedFunc) {
	storeCallback(ih, "_IUPGO_NODEREMOVED_CB", f)
	iupSetCallback(uintptr(ih), "NODEREMOVED_CB", nodeRemovedCB)
}

type DragDropFunc func(ih Ihandle, dragId, dropId, isShift, isControl int) int

var dragDropCB = purego.NewCallback(func(ih uintptr, dragId, dropId, isShift, isControl int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DRAGDROP_CB").(DragDropFunc); ok {
		return f(Ihandle(ih), int(dragId), int(dropId), int(isShift), int(isControl))
	}
	return 0
})

func setDragDropFunc(ih Ihandle, f DragDropFunc) {
	storeCallback(ih, "_IUPGO_DRAGDROP_CB", f)
	iupSetCallback(uintptr(ih), "DRAGDROP_CB", dragDropCB)
}

type DragBeginFunc func(ih Ihandle, x, y int) int

var dragBeginCB = purego.NewCallback(func(ih uintptr, x, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DRAGBEGIN_CB").(DragBeginFunc); ok {
		return f(Ihandle(ih), int(x), int(y))
	}
	return 0
})

func setDragBeginFunc(ih Ihandle, f DragBeginFunc) {
	storeCallback(ih, "_IUPGO_DRAGBEGIN_CB", f)
	iupSetCallback(uintptr(ih), "DRAGBEGIN_CB", dragBeginCB)
}

type DragDataSizeFunc func(ih Ihandle, dragType string) int

var dragDataSizeCB = purego.NewCallback(func(ih, dragType uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DRAGDATASIZE_CB").(DragDataSizeFunc); ok {
		return f(Ihandle(ih), goString(dragType))
	}
	return 0
})

func setDragDataSizeFunc(ih Ihandle, f DragDataSizeFunc) {
	storeCallback(ih, "_IUPGO_DRAGDATASIZE_CB", f)
	iupSetCallback(uintptr(ih), "DRAGDATASIZE_CB", dragDataSizeCB)
}

type DragDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size int) int

var dragDataCB = purego.NewCallback(func(ih, dragType, data uintptr, size int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DRAGDATA_CB").(DragDataFunc); ok {
		return f(Ihandle(ih), goString(dragType), unsafe.Pointer(data), int(size))
	}
	return 0
})

func setDragDataFunc(ih Ihandle, f DragDataFunc) {
	storeCallback(ih, "_IUPGO_DRAGDATA_CB", f)
	iupSetCallback(uintptr(ih), "DRAGDATA_CB", dragDataCB)
}

type DragEndFunc func(ih Ihandle, action int) int

var dragEndCB = purego.NewCallback(func(ih uintptr, action int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DRAGEND_CB").(DragEndFunc); ok {
		return f(Ihandle(ih), int(action))
	}
	return 0
})

func setDragEndFunc(ih Ihandle, f DragEndFunc) {
	storeCallback(ih, "_IUPGO_DRAGEND_CB", f)
	iupSetCallback(uintptr(ih), "DRAGEND_CB", dragEndCB)
}

type DropDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size, x, y int) int

var dropDataCB = purego.NewCallback(func(ih, dragType, data uintptr, size, x, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROPDATA_CB").(DropDataFunc); ok {
		return f(Ihandle(ih), goString(dragType), unsafe.Pointer(data), int(size), int(x), int(y))
	}
	return 0
})

func setDropDataFunc(ih Ihandle, f DropDataFunc) {
	storeCallback(ih, "_IUPGO_DROPDATA_CB", f)
	iupSetCallback(uintptr(ih), "DROPDATA_CB", dropDataCB)
}

func intSlice(p uintptr, n int32) []int {
	if p == 0 || n <= 0 {
		return nil
	}
	src := unsafe.Slice((*int32)(unsafe.Pointer(p)), int(n))
	out := make([]int, n)
	for i, v := range src {
		out[i] = int(v)
	}
	return out
}

type CellFunc func(ih Ihandle, cell int) int

var cellCB = purego.NewCallback(func(ih uintptr, cell int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_CELL_CB").(CellFunc); ok {
		return f(Ihandle(ih), int(cell))
	}
	return 0
})

func setCellFunc(ih Ihandle, f CellFunc) {
	storeCallback(ih, "_IUPGO_CELL_CB", f)
	iupSetCallback(uintptr(ih), "CELL_CB", cellCB)
}

type ExtendedFunc func(ih Ihandle, cell int) int

var extendedCB = purego.NewCallback(func(ih uintptr, cell int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EXTENDED_CB").(ExtendedFunc); ok {
		return f(Ihandle(ih), int(cell))
	}
	return 0
})

func setExtendedFunc(ih Ihandle, f ExtendedFunc) {
	storeCallback(ih, "_IUPGO_EXTENDED_CB", f)
	iupSetCallback(uintptr(ih), "EXTENDED_CB", extendedCB)
}

type ChangeFunc func(ih Ihandle, r, g, b uint8) int

var changeCB = purego.NewCallback(func(ih uintptr, r, g, b uint8) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_CHANGE_CB").(ChangeFunc); ok {
		return f(Ihandle(ih), r, g, b)
	}
	return 0
})

func setChangeFunc(ih Ihandle, f ChangeFunc) {
	storeCallback(ih, "_IUPGO_CHANGE_CB", f)
	iupSetCallback(uintptr(ih), "CHANGE_CB", changeCB)
}

type DragFunc func(ih Ihandle, r, g, b uint8) int

var dragCB = purego.NewCallback(func(ih uintptr, r, g, b uint8) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DRAG_CB").(DragFunc); ok {
		return f(Ihandle(ih), r, g, b)
	}
	return 0
})

func setDragFunc(ih Ihandle, f DragFunc) {
	storeCallback(ih, "_IUPGO_DRAG_CB", f)
	iupSetCallback(uintptr(ih), "DRAG_CB", dragCB)
}

type DropMotionFunc func(ih Ihandle, x, y int, status string) int

var dropMotionCB = purego.NewCallback(func(ih uintptr, x, y int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROPMOTION_CB").(DropMotionFunc); ok {
		return f(Ihandle(ih), int(x), int(y), goString(status))
	}
	return 0
})

func setDropMotionFunc(ih Ihandle, f DropMotionFunc) {
	storeCallback(ih, "_IUPGO_DROPMOTION_CB", f)
	iupSetCallback(uintptr(ih), "DROPMOTION_CB", dropMotionCB)
}

type DropShowFunc func(ih Ihandle, state int) int

var dropShowCB = purego.NewCallback(func(ih uintptr, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROPSHOW_CB").(DropShowFunc); ok {
		return f(Ihandle(ih), int(state))
	}
	return 0
})

func setDropShowFunc(ih Ihandle, f DropShowFunc) {
	storeCallback(ih, "_IUPGO_DROPSHOW_CB", f)
	iupSetCallback(uintptr(ih), "DROPSHOW_CB", dropShowCB)
}

type ExtraButtonFunc func(Ihandle, int, int) int

var extraButtonCB = purego.NewCallback(func(ih uintptr, button, pressed int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EXTRABUTTON_CB").(ExtraButtonFunc); ok {
		return f(Ihandle(ih), int(button), int(pressed))
	}
	return 0
})

func setExtraButtonFunc(ih Ihandle, f ExtraButtonFunc) {
	storeCallback(ih, "_IUPGO_EXTRABUTTON_CB", f)
	iupSetCallback(uintptr(ih), "EXTRABUTTON_CB", extraButtonCB)
}

type FileFunc func(ih Ihandle, filename, status string) int

var fileCB = purego.NewCallback(func(ih, filename, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FILE_CB").(FileFunc); ok {
		return f(Ihandle(ih), goString(filename), goString(status))
	}
	return 0
})

func setFileFunc(ih Ihandle, f FileFunc) {
	storeCallback(ih, "_IUPGO_FILE_CB", f)
	iupSetCallback(uintptr(ih), "FILE_CB", fileCB)
}

type FocusFunc func(Ihandle, int) int

var focusCB = purego.NewCallback(func(ih uintptr, c int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FOCUS_CB").(FocusFunc); ok {
		return f(Ihandle(ih), int(c))
	}
	return 0
})

func setFocusFunc(ih Ihandle, f FocusFunc) {
	storeCallback(ih, "_IUPGO_FOCUS_CB", f)
	iupSetCallback(uintptr(ih), "FOCUS_CB", focusCB)
}

type GestureFunc func(ih Ihandle, gesture, state, x, y int, v1, v2 float64) int

var gestureCB = purego.NewCallback(func(ih uintptr, gesture, state, x, y int32, v1, v2 float64) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_GESTURE_CB").(GestureFunc); ok {
		return f(Ihandle(ih), int(gesture), int(state), int(x), int(y), v1, v2)
	}
	return 0
})

func setGestureFunc(ih Ihandle, f GestureFunc) {
	storeCallback(ih, "_IUPGO_GESTURE_CB", f)
	iupSetCallback(uintptr(ih), "GESTURE_CB", gestureCB)
}

type MaskFailFunc func(ih Ihandle, newValue string) int

var maskFailCB = purego.NewCallback(func(ih, newValue uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MASKFAIL_CB").(MaskFailFunc); ok {
		return f(Ihandle(ih), goString(newValue))
	}
	return 0
})

func setMaskFailFunc(ih Ihandle, f MaskFailFunc) {
	storeCallback(ih, "_IUPGO_MASKFAIL_CB", f)
	iupSetCallback(uintptr(ih), "MASKFAIL_CB", maskFailCB)
}

type MultiTouchFunc func(ih Ihandle, count int, pid, px, py, pstate []int) int

var multiTouchCB = purego.NewCallback(func(ih uintptr, count int32, pid, px, py, pstate uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MULTITOUCH_CB").(MultiTouchFunc); ok {
		return f(Ihandle(ih), int(count), intSlice(pid, count), intSlice(px, count), intSlice(py, count), intSlice(pstate, count))
	}
	return 0
})

func setMultiTouchFunc(ih Ihandle, f MultiTouchFunc) {
	storeCallback(ih, "_IUPGO_MULTITOUCH_CB", f)
	iupSetCallback(uintptr(ih), "MULTITOUCH_CB", multiTouchCB)
}

type MultiUnselectionFunc func(ih Ihandle, ids []int, n int) int

var multiUnselectionCB = purego.NewCallback(func(ih, ids uintptr, n int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MULTIUNSELECTION_CB").(MultiUnselectionFunc); ok {
		return f(Ihandle(ih), intSlice(ids, n), int(n))
	}
	return 0
})

func setMultiUnselectionFunc(ih Ihandle, f MultiUnselectionFunc) {
	storeCallback(ih, "_IUPGO_MULTIUNSELECTION_CB", f)
	iupSetCallback(uintptr(ih), "MULTIUNSELECTION_CB", multiUnselectionCB)
}

type NotifyFunc func(ih Ihandle, actionId int) int

var notifyCB = purego.NewCallback(func(ih uintptr, actionId int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NOTIFY_CB").(NotifyFunc); ok {
		return f(Ihandle(ih), int(actionId))
	}
	return 0
})

func setNotifyFunc(ih Ihandle, f NotifyFunc) {
	storeCallback(ih, "_IUPGO_NOTIFY_CB", f)
	iupSetCallback(uintptr(ih), "NOTIFY_CB", notifyCB)
}

type NotifyCloseFunc func(ih Ihandle, reason int) int

var notifyCloseCB = purego.NewCallback(func(ih uintptr, reason int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NOTIFY_CLOSE_CB").(NotifyCloseFunc); ok {
		return f(Ihandle(ih), int(reason))
	}
	return 0
})

func setNotifyCloseFunc(ih Ihandle, f NotifyCloseFunc) {
	storeCallback(ih, "_IUPGO_NOTIFY_CLOSE_CB", f)
	iupSetCallback(uintptr(ih), "CLOSE_CB", notifyCloseCB)
}

type OpenCloseFunc func(ih Ihandle, state int) int

var openCloseCB = purego.NewCallback(func(ih uintptr, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_OPENCLOSE_CB").(OpenCloseFunc); ok {
		return f(Ihandle(ih), int(state))
	}
	return 0
})

func setOpenCloseFunc(ih Ihandle, f OpenCloseFunc) {
	storeCallback(ih, "_IUPGO_OPENCLOSE_CB", f)
	iupSetCallback(uintptr(ih), "OPENCLOSE_CB", openCloseCB)
}

type ReorderFunc func(ih Ihandle, oldPos, newPos int) int

var reorderCB = purego.NewCallback(func(ih uintptr, oldPos, newPos int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_REORDER_CB").(ReorderFunc); ok {
		return f(Ihandle(ih), int(oldPos), int(newPos))
	}
	return 0
})

func setReorderFunc(ih Ihandle, f ReorderFunc) {
	storeCallback(ih, "_IUPGO_REORDER_CB", f)
	iupSetCallback(uintptr(ih), "REORDER_CB", reorderCB)
}

type RestoredFunc func(Ihandle, Ihandle, int, int) int

var restoredCB = purego.NewCallback(func(ih, oldParent uintptr, x, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_RESTORED_CB").(RestoredFunc); ok {
		return f(Ihandle(ih), Ihandle(oldParent), int(x), int(y))
	}
	return 0
})

func setRestoredFunc(ih Ihandle, f RestoredFunc) {
	storeCallback(ih, "_IUPGO_RESTORED_CB", f)
	iupSetCallback(uintptr(ih), "RESTORED_CB", restoredCB)
}

type RightClickFunc func(Ihandle, int) int

var rightClickCB = purego.NewCallback(func(ih uintptr, pos int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_RIGHTCLICK_CB").(RightClickFunc); ok {
		return f(Ihandle(ih), int(pos))
	}
	return 0
})

func setRightClickFunc(ih Ihandle, f RightClickFunc) {
	storeCallback(ih, "_IUPGO_RIGHTCLICK_CB", f)
	iupSetCallback(uintptr(ih), "RIGHTCLICK_CB", rightClickCB)
}

type SelectFunc func(ih Ihandle, cell, _type int) int

var selectCB = purego.NewCallback(func(ih uintptr, cell, typ int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SELECT_CB").(SelectFunc); ok {
		return f(Ihandle(ih), int(cell), int(typ))
	}
	return 0
})

func setSelectFunc(ih Ihandle, f SelectFunc) {
	storeCallback(ih, "_IUPGO_SELECT_CB", f)
	iupSetCallback(uintptr(ih), "SELECT_CB", selectCB)
}

type ShowFunc func(ih Ihandle, state int) int

var showCB = purego.NewCallback(func(ih uintptr, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SHOW_CB").(ShowFunc); ok {
		return f(Ihandle(ih), int(state))
	}
	return 0
})

func setShowFunc(ih Ihandle, f ShowFunc) {
	storeCallback(ih, "_IUPGO_SHOW_CB", f)
	iupSetCallback(uintptr(ih), "SHOW_CB", showCB)
}

type SwitchFunc func(ih Ihandle, primCell, secCell int) int

var switchCB = purego.NewCallback(func(ih uintptr, primCell, secCell int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SWITCH_CB").(SwitchFunc); ok {
		return f(Ihandle(ih), int(primCell), int(secCell))
	}
	return 0
})

func setSwitchFunc(ih Ihandle, f SwitchFunc) {
	storeCallback(ih, "_IUPGO_SWITCH_CB", f)
	iupSetCallback(uintptr(ih), "SWITCH_CB", switchCB)
}

type TabCloseFunc func(Ihandle, int) int

var tabCloseCB = purego.NewCallback(func(ih uintptr, pos int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TABCLOSE_CB").(TabCloseFunc); ok {
		return f(Ihandle(ih), int(pos))
	}
	return 0
})

func setTabCloseFunc(ih Ihandle, f TabCloseFunc) {
	storeCallback(ih, "_IUPGO_TABCLOSE_CB", f)
	iupSetCallback(uintptr(ih), "TABCLOSE_CB", tabCloseCB)
}

type TableEditionFunc func(ih Ihandle, lin, col int, update string) int

var tableEditionCB = purego.NewCallback(func(ih uintptr, lin, col int32, update uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITION_CB").(TableEditionFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(update))
	}
	return 0
})

func setTableEditionFunc(ih Ihandle, f TableEditionFunc) {
	storeCallback(ih, "_IUPGO_EDITION_CB", f)
	iupSetCallback(uintptr(ih), "EDITION_CB", tableEditionCB)
}

type TableSortFunc func(ih Ihandle, col int) int

var tableSortCB = purego.NewCallback(func(ih uintptr, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SORT_CB").(TableSortFunc); ok {
		return f(Ihandle(ih), int(col))
	}
	return 0
})

func setTableSortFunc(ih Ihandle, f TableSortFunc) {
	storeCallback(ih, "_IUPGO_SORT_CB", f)
	iupSetCallback(uintptr(ih), "SORT_CB", tableSortCB)
}

type TableValueChangedFunc func(ih Ihandle, lin, col int) int

var tableValueChangedCB = purego.NewCallback(func(ih uintptr, lin, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TABLE_VALUECHANGED_CB").(TableValueChangedFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setTableValueChangedFunc(ih Ihandle, f TableValueChangedFunc) {
	storeCallback(ih, "_IUPGO_TABLE_VALUECHANGED_CB", f)
	iupSetCallback(uintptr(ih), "VALUECHANGED_CB", tableValueChangedCB)
}

type TextLinkFunc func(ih Ihandle, url string) int

var textLinkCB = purego.NewCallback(func(ih, url uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LINK_CB").(TextLinkFunc); ok {
		return f(Ihandle(ih), goString(url))
	}
	return 0
})

func setTextLinkFunc(ih Ihandle, f TextLinkFunc) {
	storeCallback(ih, "_IUPGO_LINK_CB", f)
	iupSetCallback(uintptr(ih), "LINK_CB", textLinkCB)
}

type ThemeChangedFunc func(ih Ihandle, darkMode int) int

var themeChangedCB = purego.NewCallback(func(ih uintptr, darkMode int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_THEMECHANGED_CB").(ThemeChangedFunc); ok {
		return f(Ihandle(ih), int(darkMode))
	}
	return 0
})

func setThemeChangedFunc(ih Ihandle, f ThemeChangedFunc) {
	storeCallback(ih, "_IUPGO_THEMECHANGED_CB", f)
	iupSetCallback(uintptr(ih), "THEMECHANGED_CB", themeChangedCB)
}

type ToggleValueFunc func(ih Ihandle, id, state int) int

var toggleValueCB = purego.NewCallback(func(ih uintptr, id, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TOGGLEVALUE_CB").(ToggleValueFunc); ok {
		return f(Ihandle(ih), int(id), int(state))
	}
	return 0
})

func setToggleValueFunc(ih Ihandle, f ToggleValueFunc) {
	storeCallback(ih, "_IUPGO_TOGGLEVALUE_CB", f)
	iupSetCallback(uintptr(ih), "TOGGLEVALUE_CB", toggleValueCB)
}

type TouchFunc func(ih Ihandle, id, x, y int, state string) int

var touchCB = purego.NewCallback(func(ih uintptr, id, x, y int32, state uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TOUCH_CB").(TouchFunc); ok {
		return f(Ihandle(ih), int(id), int(x), int(y), goString(state))
	}
	return 0
})

func setTouchFunc(ih Ihandle, f TouchFunc) {
	storeCallback(ih, "_IUPGO_TOUCH_CB", f)
	iupSetCallback(uintptr(ih), "TOUCH_CB", touchCB)
}

type TrayClickFunc func(Ihandle, int, int, int) int

var trayClickCB = purego.NewCallback(func(ih uintptr, but, pressed, dclick int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TRAYCLICK_CB").(TrayClickFunc); ok {
		return f(Ihandle(ih), int(but), int(pressed), int(dclick))
	}
	return 0
})

func setTrayClickFunc(ih Ihandle, f TrayClickFunc) {
	storeCallback(ih, "_IUPGO_TRAYCLICK_CB", f)
	iupSetCallback(uintptr(ih), "TRAYCLICK_CB", trayClickCB)
}

type ValueChangingFunc func(ih Ihandle, start int) int

var valueChangingCB = purego.NewCallback(func(ih uintptr, start int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_VALUECHANGING_CB").(ValueChangingFunc); ok {
		return f(Ihandle(ih), int(start))
	}
	return 0
})

func setValueChangingFunc(ih Ihandle, f ValueChangingFunc) {
	storeCallback(ih, "_IUPGO_VALUECHANGING_CB", f)
	iupSetCallback(uintptr(ih), "VALUECHANGING_CB", valueChangingCB)
}

type PostMessageFunc func(Ihandle, string, int, any) int

var postMessageCB = purego.NewCallback(func(ih, s uintptr, i int32, d float64, p uintptr) int {
	_ = d
	f, ok := loadCallback(Ihandle(ih), "_IUPGO_POSTMESSAGE_CB").(PostMessageFunc)
	if !ok {
		return 0
	}
	var payload any
	if p != 0 {
		payload = cbLoad(uint64(p))
		cbDelete(uint64(p))
	}
	return f(Ihandle(ih), goString(s), int(i), payload)
})

func setPostMessageFunc(ih Ihandle, f PostMessageFunc) {
	storeCallback(ih, "_IUPGO_POSTMESSAGE_CB", f)
	iupSetCallback(uintptr(ih), "POSTMESSAGE_CB", postMessageCB)
}

func PostMessage(ih Ihandle, s string, i int, p any) {
	var id uint64
	if p != nil {
		id = cbStore(p)
	}
	iupPostMessage(uintptr(ih), s, int32(i), 0.0, uintptr(id))
}

type GetParamFunc func(dialog Ihandle, paramIndex int) int

var getParamCB = purego.NewCallback(func(dialog uintptr, paramIndex int32, userData uintptr) int {
	if f, ok := cbLoad(uint64(userData)).(GetParamFunc); ok {
		return f(Ihandle(dialog), int(paramIndex))
	}
	return 1
})

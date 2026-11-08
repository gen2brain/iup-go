package iup

import "unsafe"

// ClassAttributeInfo is the metadata returned by [GetClassAttributeInfo].
type ClassAttributeInfo struct {
	DefaultValue  string
	SystemDefault string
	Flags         int
}

// ClassInfo is the metadata returned by [GetClassInfo].
type ClassInfo struct {
	Parent        string
	NativeType    string
	ChildType     int
	IsInteractive bool
	HasAttribID   int
}

// ClassConstructorInfo is the metadata returned by [GetClassConstructor].
type ClassConstructorInfo struct {
	Format     string // creation parameter format ("h", "g", "s", "i" + "c", ...) or "" if NULL
	FormatAttr string // attribute name receiving the first parameter when Format starts with "s" or "a"
}

// GlobalInfo is the metadata returned by [GetGlobalInfo].
type GlobalInfo struct {
	Flags   int // OR of GF_* flag bits
	Drivers int // OR of DRV_* driver bits
}

// IdleFunc for IDLE_ACTION callback.
// Generated when there are no events or messages to be processed. Often used to perform background operations.
//
// https://gen2brain.github.io/iup-go/call/iup_idle_action.html
type IdleFunc func() int

// EntryPointFunc for ENTRY_POINT callback.
// Called once the system is ready to run the application, on Android and iOS; see [EntryPoint].
//
// https://gen2brain.github.io/iup-go/call/iup_entry_point.html
type EntryPointFunc func()

// ExitFunc for EXIT_CB callback.
// Global callback for an exit. Used when main is not possible, such as in iOS and Android systems.
//
// https://gen2brain.github.io/iup-go/call/iup_exit_cb.html
type ExitFunc func()

// GlobalKeyPressFunc for GLOBALKEYPRESS_CB callback.
// Same as KeyPressFunc, without the Ihandle. Needs the INPUTCALLBACKS global attribute.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
type GlobalKeyPressFunc func(c, press int) int

// GlobalButtonFunc for GLOBALBUTTON_CB callback.
// Same as ButtonFunc, without the Ihandle. Needs the INPUTCALLBACKS global attribute.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
type GlobalButtonFunc func(button, pressed, x, y int, status string) int

// GlobalMotionFunc for GLOBALMOTION_CB callback.
// Same as MotionFunc, without the Ihandle. Needs the INPUTCALLBACKS global attribute.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
type GlobalMotionFunc func(x, y int, status string) int

// GlobalWheelFunc for GLOBALWHEEL_CB callback.
// Same as WheelFunc, without the Ihandle. Needs the INPUTCALLBACKS global attribute.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
type GlobalWheelFunc func(delta float64, x, y int, status string) int

// GlobalEnterModalFunc for GLOBALENTERMODAL_CB callback.
// Called when a dialog becomes modal, with the modal level.
type GlobalEnterModalFunc func(level int) int

// GlobalLeaveModalFunc for GLOBALLEAVEMODAL_CB callback.
// Called when a modal dialog is closed, with the modal level.
type GlobalLeaveModalFunc func(level int) int

// MapFunc for MAP_CB callback.
// Called right after an element is mapped and its attributes updated in Map.
// When the element is a dialog, it is called after the layout is updated. For all other elements is called before the layout is updated.
//
// https://gen2brain.github.io/iup-go/call/iup_map_cb.html
type MapFunc func(Ihandle) int

// UnmapFunc for UNMAP_CB callback.
// Called right before an element is unmapped.
//
// https://gen2brain.github.io/iup-go/call/iup_unmap_cb.html
type UnmapFunc func(Ihandle) int

// DestroyFunc for DESTROY_CB callback.
// Called right before an element is destroyed.
//
// https://gen2brain.github.io/iup-go/call/iup_destroy_cb.html
type DestroyFunc func(Ihandle) int

// GetFocusFunc for GETFOCUS_CB callback.
// Action generated when an element is given keyboard focus.
//
// https://gen2brain.github.io/iup-go/call/iup_getfocus_cb.html
type GetFocusFunc func(Ihandle) int

// KillFocusFunc for KILLFOCUS_CB callback.
// Action generated when an element loses keyboard focus.
//
// https://gen2brain.github.io/iup-go/call/iup_killfocus_cb.html
type KillFocusFunc func(Ihandle) int

// EnterWindowFunc for ENTERWINDOW_CB callback.
// Action generated when the mouse enters the native element.
//
// https://gen2brain.github.io/iup-go/call/iup_enterwindow_cb.html
type EnterWindowFunc func(Ihandle) int

// LeaveWindowFunc for LEAVEWINDOW_CB callback.
// Action generated when the mouse leaves the native element.
//
// https://gen2brain.github.io/iup-go/call/iup_leavewindow_cb.html
type LeaveWindowFunc func(Ihandle) int

// TipsFunc for TIPS_CB callback.
// Action before a tip is displayed.
//
// https://gen2brain.github.io/iup-go/attrib/iup_tip.html
type TipsFunc func(ih Ihandle, x, y int) int

// KAnyFunc for K_ANY callback.
// Action generated when a keyboard event occurs.
//
// https://gen2brain.github.io/iup-go/call/iup_k_any.html
type KAnyFunc func(Ihandle, int) int

// HelpFunc for HELP_CB callback.
// Action generated when the user presses F1 at a control.
//
// https://gen2brain.github.io/iup-go/call/iup_help_cb.html
type HelpFunc func(Ihandle) int

// ActionFunc for ACTION callback.
// Action generated when the element is activated. Affects each element differently.
//
// https://gen2brain.github.io/iup-go/call/iup_action.html
type ActionFunc func(Ihandle) int

// ButtonFunc for BUTTON_CB callback.
// Action generated when a mouse button is pressed or released.
//
// https://gen2brain.github.io/iup-go/call/iup_button_cb.html
type ButtonFunc func(Ihandle, int, int, int, int, string) int

// TouchFunc for TOUCH_CB callback.
// Action generated when a touch event occurred.
//
// https://gen2brain.github.io/iup-go/elem/iup_canvas.html
type TouchFunc func(ih Ihandle, id, x, y int, state string) int

// MultiTouchFunc for MULTITOUCH_CB callback.
// Action generated when multiple touch events occurred.
//
// https://gen2brain.github.io/iup-go/elem/iup_canvas.html
type MultiTouchFunc func(ih Ihandle, count int, pid, px, py, pstate []int) int

// GestureFunc for GESTURE_CB callback.
// Action generated when a pinch, rotate, pan, swipe, tap or long-press gesture is recognized.
//
// https://gen2brain.github.io/iup-go/elem/iup_canvas.html
type GestureFunc func(ih Ihandle, gesture, state, x, y int, v1, v2 float64) int

// TerminalInputFunc for Terminal INPUT_CB callback.
type TerminalInputFunc func(ih Ihandle, bytes []byte) int

// TerminalTitleChangedFunc for Terminal TITLECHANGED_CB callback.
type TerminalTitleChangedFunc func(ih Ihandle, title string) int

// TerminalBellFunc for Terminal BELL_CB callback.
type TerminalBellFunc func(ih Ihandle) int

// TerminalSizeFunc for Terminal TERMSIZE_CB callback.
type TerminalSizeFunc func(ih Ihandle, cols, lines int) int

// TerminalExecExitFunc for Terminal EXECEXIT_CB callback.
type TerminalExecExitFunc func(ih Ihandle, status int) int

// TextInputFunc for TEXTINPUT_CB callback.
type TextInputFunc func(ih Ihandle, text string) int

// DropFilesFunc for DROPFILES_CB callback.
// Action called when a file is "dropped" into control.
//
// https://gen2brain.github.io/iup-go/call/iup_dropfiles_cb.html
type DropFilesFunc func(Ihandle, string, int, int, int) int

// CopyDataFunc for COPYDATA_CB callback.
// Called at the first instance when a second instance is running, with the command line of the second instance.
// Requires the global attribute SINGLEINSTANCE.
//
// https://gen2brain.github.io/iup-go/dlg/iup_dialog.html
type CopyDataFunc func(ih Ihandle, cmdLine string, size int) int

// PaginateFunc for PAGINATE_CB callback.
// Called by the PrintDlg when the size of the printable area is known, before the pages are drawn; returns the number of pages.
//
// https://gen2brain.github.io/iup-go/dlg/iup_printdlg.html
type PaginateFunc func(ih Ihandle, width, height int) int

// PrintPageFunc for PRINTPAGE_CB callback.
// Called by the PrintDlg for each page to print; draw the page with the Draw functions between DrawBegin and DrawEnd.
//
// https://gen2brain.github.io/iup-go/dlg/iup_printdlg.html
type PrintPageFunc func(ih Ihandle, page, width, height int) int

// ListActionFunc for List ACTION callback.
// Action generated when the state of an item in the list is changed.
type ListActionFunc func(ih Ihandle, text string, item, state int) int

// ListValueFunc for List VALUE_CB callback in virtual mode.
// Called to get the text value of an item when VIRTUALMODE=YES.
// pos is 1-based item index.
// Returns the string to display for the item.
type ListValueFunc func(ih Ihandle, pos int) string

// ListImageFunc for List IMAGE_CB callback in virtual mode.
// Called to get the image name for an item when VIRTUALMODE=YES and SHOWIMAGE=YES.
// Returns the image name to display for the item, or empty string for no image.
type ListImageFunc func(ih Ihandle, pos int) string

// CaretFunc for CARET_CB callback.
// Action generated when the caret/cursor position is changed.
type CaretFunc func(ih Ihandle, lin, col, pos int) int

// DblclickFunc for DBLCLICK_CB callback.
// Action generated when the user double click an item.
type DblclickFunc func(ih Ihandle, item int, text string) int

// EditFunc for EDIT_CB callback.
type EditFunc func(ih Ihandle, c int, newValue string) int

// MotionFunc for MOTION_CB callback.
// Action generated when the mouse is moved over the list.
type MotionFunc func(ih Ihandle, x, y int, status string) int

// MultiselectFunc for MULTISELECT_CB callback.
// Action generated when the state of an item in the multiple selection list is changed.
type MultiselectFunc func(ih Ihandle, text string) int

// ValueChangedFunc for VALUECHANGED_CB callback.
// Called after the value was interactively changed by the user.
type ValueChangedFunc func(ih Ihandle) int

// TextActionFunc for Text ACTION callback.
// Action generated when the text is edited, but before its value is actually changed.
type TextActionFunc func(ih Ihandle, ch int, newValue string) int

// ToggleActionFunc for Toggle ACTION callback.
// Action generated when the toggle's state (on/off) was changed.
type ToggleActionFunc func(ih Ihandle, state int) int

// TabChangeFunc for TABCHANGE_CB callback.
type TabChangeFunc func(ih, newTab, oldTab Ihandle) int

// TabChangePosFunc for TABCHANGEPOS_CB callback.
type TabChangePosFunc func(ih Ihandle, newPos, oldPos int) int

// ReorderFunc for REORDER_CB callback.
type ReorderFunc func(ih Ihandle, oldPos, newPos int) int

// SpinFunc for SPIN_CB callback.
type SpinFunc func(ih Ihandle, inc int) int

// PostMessageFunc for POSTMESSAGE_CB callback.
type PostMessageFunc func(Ihandle, string, int, any) int

// CloseFunc for CLOSE_CB callback.
// Called just before a dialog is closed when the user clicks the close button
// of the title bar or an equivalent action.
type CloseFunc func(Ihandle) int

// FocusFunc for FOCUS_CB callback.
// Called when the dialog or any of its children gets the focus, or when
// another dialog or any control in another dialog gets the focus.
type FocusFunc func(Ihandle, int) int

// MoveFunc for MOVE_CB callback.
// Called after the dialog was moved on screen.
type MoveFunc func(ih Ihandle, x, y int) int

// ResizeFunc for RESIZE_CB callback.
// Action generated when the canvas or dialog size is changed.
type ResizeFunc func(ih Ihandle, width, height int) int

// ShowFunc for SHOW_CB callback.
// Called right after the dialog is shown, hidden, maximized, minimized or restored from minimized/maximized.
type ShowFunc func(ih Ihandle, state int) int

// ChangeFunc for CHANGE_CB callback.
// Called when the user releases the left mouse button over the control, defining the selected color.
type ChangeFunc func(ih Ihandle, r, g, b uint8) int

// DragFunc for DRAG_CB callback.
// Called several times while the color is being changed by dragging the mouse over the control.
type DragFunc func(ih Ihandle, r, g, b uint8) int

// DetachedFunc for DETACHED_CB callback.
type DetachedFunc func(Ihandle, Ihandle, int, int) int

// RestoredFunc for RESTORED_CB callback.
// Called when the box is restored if RESTOREWHENCLOSED=Yes.
type RestoredFunc func(Ihandle, Ihandle, int, int) int

// SwapBuffersFunc for SWAPBUFFERS_CB callback.
// Action generated when GLSwapBuffers is called.
type SwapBuffersFunc func(Ihandle) int

// CancelFunc for CANCEL_CB callback.
// Action generated when the user clicked on the Cancel button.
type CancelFunc func(Ihandle) int

// TimerActionFunc for ACTION_CB callback.
// Called every time the defined time interval is reached.
// To stop the callback from being called, stop the timer with RUN=NO.
type TimerActionFunc func(ih Ihandle) int

// ThreadFunc for THREAD_CB callback.
// Action generated when the thread is started.
// If this callback returns or does not exist, the thread is terminated.
type ThreadFunc func(ih Ihandle) int

// ScrollFunc for SCROLL_CB callback.
// Called when the scrollbar is manipulated.
type ScrollFunc func(ih Ihandle, op int, posx, posy float64) int

// TrayClickFunc for TRAYCLICK_CB callback.
// Called right after the mouse button is pressed or released over the tray icon.
type TrayClickFunc func(Ihandle, int, int, int) int

// TabCloseFunc for TABCLOSE_CB callback.
// Called when the user clicks on the close button.
type TabCloseFunc func(Ihandle, int) int

// RightClickFunc for RIGHTCLICK_CB callback.
// Called when the user clicks on some tab using the right mouse button.
type RightClickFunc func(Ihandle, int) int

// ExtraButtonFunc for EXTRABUTTON_CB callback.
// Action generated when any mouse button is pressed or released.
type ExtraButtonFunc func(Ihandle, int, int) int

// OpenCloseFunc for OPENCLOSE_CB callback.
// Action generated before the expander state is interactively changed.
type OpenCloseFunc func(ih Ihandle, state int) int

// ValueChangingFunc for VALUECHANGING_CB callback.
// Called when the value starts or ends to be interactively changed by the user.
type ValueChangingFunc func(ih Ihandle, start int) int

// DropDownFunc for DROPDOWN_CB callback.
// Action generated right before the drop child is shown or hidden.
type DropDownFunc func(ih Ihandle, state int) int

// DropShowFunc for DROPSHOW_CB callback.
// Action generated right after the drop child is shown or hidden.
type DropShowFunc func(ih Ihandle, state int) int

// KeyPressFunc for KEYPRESS_CB callback.
// Action generated when a key is pressed or released.
type KeyPressFunc func(ih Ihandle, c, press int) int

// CellFunc for CELL_CB callback.
// Called when the user double clicks a color cell to change its value.
// Returns the new color, or an empty string to ignore the change.
type CellFunc func(ih Ihandle, cell int) string

// ExtendedFunc for EXTENDED_CB callback.
// Called when the user right click a cell with the Shift key pressed.
type ExtendedFunc func(ih Ihandle, cell int) int

// SelectFunc for SELECT_CB callback.
// Called when a color is selected.
type SelectFunc func(ih Ihandle, cell, _type int) int

// SwitchFunc for SWITCH_CB callback.
// Called when the user double clicks the preview area outside the preview cells
// to switch the primary and secondary selections.
type SwitchFunc func(ih Ihandle, primCell, secCell int) int

// LinkActionFunc for Link ACTION callback.
type LinkActionFunc func(ih Ihandle, url string) int

// TextLinkFunc for LINK_CB callback on multiline text controls.
// Action generated when a hyperlink in formatted text is clicked.
type TextLinkFunc func(ih Ihandle, url string) int

// MaskFailFunc for MASKFAIL_CB callback.
// Action generated when the new text fails at the mask check.
type MaskFailFunc func(ih Ihandle, newValue string) int

// WheelFunc for WHEEL_CB callback.
// Action generated when the mouse wheel is rotated.
type WheelFunc func(ih Ihandle, delta float64, x, y int, status string) int

// DragDropFunc for DRAGDROP_CB callback.
// Action generated when an internal drag and drop is executed.
type DragDropFunc func(ih Ihandle, dragId, dropId, isShift, isControl int) int

// DragBeginFunc for DRAGBEGIN_CB callback.
// Called when drag operation begins.
type DragBeginFunc func(ih Ihandle, x, y int) int

// DragDataSizeFunc for DRAGDATASIZE_CB callback.
// Called to query the size of drag data.
type DragDataSizeFunc func(ih Ihandle, dragType string) int

// DragDataFunc for DRAGDATA_CB callback.
// Called to get drag data.
type DragDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size int) int

// DragEndFunc for DRAGEND_CB callback.
// Called when drag operation ends.
type DragEndFunc func(ih Ihandle, action int) int

// DropDataFunc for DROPDATA_CB callback.
// Called when data is dropped.
type DropDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size, x, y int) int

// DropMotionFunc for DROPMOTION_CB callback.
// Called during drag motion over drop target.
type DropMotionFunc func(ih Ihandle, x, y int, status string) int

// SelectionFunc for SELECTION_CB callback.
// Action generated when a node is selected or deselected.
type SelectionFunc func(ih Ihandle, id, status int) int

// BranchOpenFunc for BRANCHOPEN_CB callback.
// Action generated when a branch is expanded.
type BranchOpenFunc func(ih Ihandle, id int) int

// BranchCloseFunc for BRANCHCLOSE_CB callback.
// Action generated when a branch is collapsed.
type BranchCloseFunc func(ih Ihandle, id int) int

// ExecuteLeafFunc for EXECUTELEAF_CB callback.
// Action generated when a leaf is executed.
type ExecuteLeafFunc func(ih Ihandle, id int) int

// ExecuteBranchFunc for EXECUTEBRANCH_CB callback.
// Action generated when a branch is executed.
type ExecuteBranchFunc func(ih Ihandle, id int) int

// ShowRenameFunc for SHOWRENAME_CB callback.
// Action generated when a node is about to be renamed.
type ShowRenameFunc func(ih Ihandle, id int) int

// RenameFunc for RENAME_CB callback.
// Action generated after a node was renamed in place.
type RenameFunc func(ih Ihandle, id int, title string) int

// ToggleValueFunc for TOGGLEVALUE_CB callback.
// Action generated when the toggle's state was changed.
type ToggleValueFunc func(ih Ihandle, id, state int) int

// NodeRemovedFunc for NODEREMOVED_CB callback. userId is the USERDATA pointer
// that was set on the node via TreeSetUserId; useful for releasing whatever it
// pointed at (e.g. cgo.Handle(userId).Delete()).
type NodeRemovedFunc func(ih Ihandle, userId uintptr) int

// MultiSelectionFunc for MULTISELECTION_CB callback.
// Action generated after a continuous range of nodes is selected in one single operation.
type MultiSelectionFunc func(ih Ihandle, ids []int, n int) int

// MultiUnselectionFunc for MULTIUNSELECTION_CB callback.
// Action generated before multiple nodes are unselected in one single operation.
type MultiUnselectionFunc func(ih Ihandle, ids []int, n int) int

// MenuOpenFunc for MENUOPEN_CB callback.
// Called just before the menu is opened.
//
// https://gen2brain.github.io/iup-go/call/iup_menuopen_cb.html
type MenuOpenFunc func(ih Ihandle) int

// AppearanceChangedFunc for APPEARANCECHANGED_CB callback.
// Called when the appearance changes between light and dark.
type AppearanceChangedFunc func(ih Ihandle, darkMode int) int

// UpdateFunc for UPDATE_CB callback.
// Action generated when the selection was changed and the editor interface needs an update.
type UpdateFunc func(ih Ihandle) int

// TableEditChangedFunc for EDITCHANGED_CB callback.
// Called when a cell is being edited in a table/matrix control.
// update: The new value entered by the user.
type TableEditChangedFunc func(ih Ihandle, lin, col int, update string) int

// TableRightClickFunc for RIGHTCLICK_CB callback.
// Called when the right mouse button is pressed over a table cell.
type TableRightClickFunc func(ih Ihandle, lin, col int) int

// CellSelectionFunc for CELLSELECTION_CB callback.
// Called when the selected cell range of a table changes in CELLS selection mode.
type CellSelectionFunc func(ih Ihandle, lin1, col1, lin2, col2 int) int

// TableValueChangedFunc for VALUECHANGED_CB callback.
// Called after a cell value has been changed in a table/matrix control.
// Triggered by edits, paste operations, or programmatic value changes.
type TableValueChangedFunc func(ih Ihandle, lin, col int) int

// TableValueFunc for VALUE_CB callback.
// Called to get the value of a cell in virtual mode.
// Returns the string value to display in the cell.
type TableValueFunc func(ih Ihandle, lin, col int) string

// TableImageFunc for IMAGE_CB callback.
// Called to get the image name for a cell in virtual mode when SHOWIMAGE=YES.
// Returns the image name to display in the cell, or empty string for no image.
type TableImageFunc func(ih Ihandle, lin, col int) string

// TableSortFunc for SORT_CB callback.
// Called when user clicks a column header to sort.
// Returns DEFAULT to proceed with sorting or IGNORE to cancel.
type TableSortFunc func(ih Ihandle, col int) int

// FileFunc for FILE_CB callback.
// Called when a file is selected in FileDlg.
// status: "OK", "NEW", or "CANCEL"
type FileFunc func(ih Ihandle, filename, status string) int

// LayoutUpdateFunc for LAYOUTUPDATE_CB callback.
// Called when layout is updated in FlatScrollBox or ScrollBox.
type LayoutUpdateFunc func(ih Ihandle) int

// HighlightFunc for HIGHLIGHT_CB callback.
// Called when a menu item is highlighted.
type HighlightFunc func(ih Ihandle) int

// MenuCloseFunc for MENUCLOSE_CB callback.
// Called when a menu is closed.
type MenuCloseFunc func(ih Ihandle) int

// ColorUpdateFunc for COLORUPDATE_CB callback.
// Called when color is updated in ColorDlg.
type ColorUpdateFunc func(ih Ihandle) int

// NotifyFunc for NOTIFY_CB callback.
// Called when the notification is activated (clicked or action button pressed).
// action_id: 0=body clicked, 1-4=action buttons
type NotifyFunc func(ih Ihandle, actionId int) int

// NotifyDismissFunc for DISMISS_CB callback on notification.
// Called when the notification is dismissed.
// reason: 0=unknown, 1=expired, 2=user dismissed, 3=closed programmatically
type NotifyDismissFunc func(ih Ihandle, reason int) int

// LocationFunc for LOCATION_CB callback.
// Called on every position update of an IupLocation element.
type LocationFunc func(ih Ihandle, latitude, longitude float64) int

// SensorFunc for SENSOR_CB callback.
// Called on every reading of an IupSensor element.
type SensorFunc func(ih Ihandle, x, y, z float64) int

// PermissionFunc for PERMISSION_CB callback.
// Called when the user answers a permission request; granted is 1 or 0.
type PermissionFunc func(ih Ihandle, granted int) int

// GetParamFunc is the callback for GetParam dialog.
// It is called when a parameter value changes, a button is pressed, or the dialog is mapped/initialized.
// The paramIndex is >= 0 for parameter changes, or one of the GETPARAM_* constants for events.
// Return 1 to accept the change or 0 to reject it.
//
// https://gen2brain.github.io/iup-go/dlg/iup_getparam.html
type GetParamFunc func(dialog Ihandle, paramIndex int) int

// ParamFunc for PARAM_CB callback.
// Called when a parameter value changes or a button is pressed.
//
// https://gen2brain.github.io/iup-go/elem/iup_parambox.html
type ParamFunc func(paramBox Ihandle, paramIndex int) int

// CustomFrameDrawFunc for CUSTOMFRAMEDRAW_CB callback.
// Called when a dialog with a custom frame must be redrawn.
type CustomFrameDrawFunc func(ih Ihandle) int

// CustomFrameActivateFunc for CUSTOMFRAMEACTIVATE_CB callback.
// Called when the active state of a dialog with a custom frame changes.
type CustomFrameActivateFunc func(ih Ihandle, active int) int

// WomFunc for WOM_CB callback.
// Called when a waveform audio output device opened with the canvas window sends an event.
type WomFunc func(ih Ihandle, state int) int

// ListReleaseFunc for LISTRELEASE_CB callback in MatrixList.
// Called when mouse button is released over a list item.
type ListReleaseFunc func(ih Ihandle, lin, col int, status string) int

// ListInsertFunc for LISTINSERT_CB callback in MatrixList.
// Called when an item is inserted.
type ListInsertFunc func(ih Ihandle, pos int) int

// ListRemoveFunc for LISTREMOVE_CB callback in MatrixList.
// Called when an item is removed.
type ListRemoveFunc func(ih Ihandle, pos int) int

// ListEditionFunc for LISTEDITION_CB callback in MatrixList.
// Called during list item editing.
type ListEditionFunc func(ih Ihandle, lin, col, mode, update int) int

// ListDrawFunc for LISTDRAW_CB callback in MatrixList.
// Called to draw a list item (custom drawing).
type ListDrawFunc func(ih Ihandle, lin, col, x1, x2, y1, y2 int) int

// BusyFunc for BUSY_CB callback in MatrixEx.
// Called during busy/long operations.
type BusyFunc func(ih Ihandle, lin, col int, status string) int

// MenuContextFunc for MENUCONTEXT_CB callback in MatrixEx.
// Called to show context menu for a cell.
type MenuContextFunc func(ih, menu Ihandle, lin, col int) int

// MenuContextCloseFunc for MENUCONTEXTCLOSE_CB callback in MatrixEx.
// Called when context menu is closed.
type MenuContextCloseFunc func(ih, menu Ihandle, lin, col int) int

// PasteSizeFunc for PASTESIZE_CB callback in MatrixEx.
// Called to determine paste area size.
type PasteSizeFunc func(ih Ihandle, numlin, numcol int) int

// NumericGetValueFunc for NUMERICGETVALUE_CB callback in Matrix.
// Called to get numeric value from a cell (returns double).
type NumericGetValueFunc func(ih Ihandle, lin, col int) float64

// NumericSetValueFunc for NUMERICSETVALUE_CB callback in Matrix.
// Called to set numeric value in a cell.
type NumericSetValueFunc func(ih Ihandle, lin, col int, value float64) int

// SortColumnCompareFunc for SORTCOLUMNCOMPARE_CB callback in Matrix.
// Called to compare two rows during column sort.
type SortColumnCompareFunc func(ih Ihandle, col, lin1, lin2 int) int

// CellsDrawFunc for DRAW_CB callback in Cells control.
// Called to draw a cell.
type CellsDrawFunc func(ih Ihandle, i, j, xmin, xmax, ymin, ymax int) int

// MouseClickFunc for MOUSECLICK_CB callback.
// Called when mouse is clicked in a cell.
type MouseClickFunc func(ih Ihandle, button, pressed, i, j, x, y int, status string) int

// MouseMotionFunc for MOUSEMOTION_CB callback.
// Called when mouse moves over a cell.
type MouseMotionFunc func(ih Ihandle, i, j, x, y int, status string) int

// ScrollingFunc for SCROLLING_CB callback.
// Called when cells are scrolled.
type ScrollingFunc func(ih Ihandle, i, j int) int

// NColsFunc for NCOLS_CB callback.
// Called to get the number of columns.
type NColsFunc func(ih Ihandle) int

// NLinesFunc for NLINES_CB callback.
// Called to get the number of lines.
type NLinesFunc func(ih Ihandle) int

// HSpanFunc for HSPAN_CB callback.
// Called to get horizontal span of a cell.
type HSpanFunc func(ih Ihandle, i, j int) int

// VSpanFunc for VSPAN_CB callback.
// Called to get vertical span of a cell.
type VSpanFunc func(ih Ihandle, i, j int) int

// HeightFunc for HEIGHT_CB callback.
// Called to get the height of a line.
type HeightFunc func(ih Ihandle, i int) int

// WidthFunc for WIDTH_CB callback.
// Called to get the width of a column.
type WidthFunc func(ih Ihandle, j int) int

// BgColorFunc for BGCOLOR_CB callback.
// Called to determine the background color of a cell.
type BgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

// ClickFunc for CLICK_CB callback.
// Called when a cell is clicked.
type ClickFunc func(ih Ihandle, lin, col int, status string) int

// ColResizeFunc for COLRESIZE_CB callback.
// Called when a column is resized.
type ColResizeFunc func(ih Ihandle, col int) int

// DropCheckFunc for DROPCHECK_CB callback.
// Called to check if dropdown should be shown.
type DropCheckFunc func(ih Ihandle, lin, col int) int

// DropSelectFunc for DROPSELECT_CB callback.
// Called when dropdown item is selected.
type DropSelectFunc func(ih Ihandle, lin, col int, drop Ihandle, text string, item, col2 int) int

// EditBeginFunc for EDITBEGIN_CB callback.
// Called when cell editing begins.
type EditBeginFunc func(ih Ihandle, lin, col int) int

// EditClickFunc for EDITCLICK_CB callback.
// Called when edit field is clicked.
type EditClickFunc func(ih Ihandle, lin, col int, status string) int

// EditEndFunc for EDITEND_CB callback.
// Called when cell editing ends.
type EditEndFunc func(ih Ihandle, lin, col int, newValue string, apply int) int

// EditMouseMoveFunc for EDITMOUSEMOVE_CB callback.
// Called when mouse moves in edit field.
type EditMouseMoveFunc func(ih Ihandle, lin, col int) int

// EditReleaseFunc for EDITRELEASE_CB callback.
// Called when mouse button is released in edit field.
type EditReleaseFunc func(ih Ihandle, lin, col int, status string) int

// EditionFunc for EDITION_CB callback.
// Called during cell editing.
type EditionFunc func(ih Ihandle, lin, col, mode, update int) int

// EnterItemFunc for ENTERITEM_CB callback.
// Called when a cell is entered.
type EnterItemFunc func(ih Ihandle, lin, col int) int

// FgColorFunc for FGCOLOR_CB callback.
// Called to determine the foreground color of a cell.
type FgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

// LeaveItemFunc for LEAVEITEM_CB callback.
// Called when a cell is left.
type LeaveItemFunc func(ih Ihandle, lin, col int) int

// MarkEditFunc for MARKEDIT_CB callback.
// Called when cell mark is edited.
type MarkEditFunc func(ih Ihandle, lin, col, marked int) int

// MarkFunc for MARK_CB callback.
// Called when a cell is marked.
type MarkFunc func(ih Ihandle, lin, col int) int

// MatrixActionFunc for ACTION_CB callback in Matrix control.
// Called for keyboard actions in a cell.
type MatrixActionFunc func(ih Ihandle, key, lin, col, edition int, status string) int

// MatrixDrawFunc for DRAW_CB callback in Matrix control.
// Called to draw a cell.
type MatrixDrawFunc func(ih Ihandle, lin, col, x1, x2, y1, y2 int) int

// MatrixDropFunc for DROP_CB callback in Matrix control.
// Called when a dropdown is shown.
type MatrixDropFunc func(ih, drop Ihandle, lin, col int) int

// MatrixFontFunc for FONT_CB callback.
// Called to get the font for a cell.
// Returns font string.
type MatrixFontFunc func(ih Ihandle, lin, col int) string

// MatrixMouseMoveFunc for MOUSEMOVE_CB callback in Matrix.
// Called when mouse moves over a cell.
type MatrixMouseMoveFunc func(ih Ihandle, lin, col int) int

// MatrixToggleValueFunc for TOGGLEVALUE_CB callback in Matrix control.
// Called when toggle value changes.
type MatrixToggleValueFunc func(ih Ihandle, lin, col, value int) int

// MatrixTypeFunc for TYPE_CB callback.
// Called to get the type of a cell.
// Returns type string.
type MatrixTypeFunc func(ih Ihandle, lin, col int) string

// MatrixValueFunc for VALUE_CB callback.
// Called to get the value of a cell.
// Returns value string.
type MatrixValueFunc func(ih Ihandle, lin, col int) string

// MenuDropFunc for MENUDROP_CB callback.
// Called to show dropdown menu for a cell.
type MenuDropFunc func(ih, menu Ihandle, lin, col int) int

// ReleaseFunc for RELEASE_CB callback.
// Called when mouse button is released over a cell.
type ReleaseFunc func(ih Ihandle, lin, col int, status string) int

// ResizeMatrixFunc for RESIZEMATRIX_CB callback.
// Called when the matrix is resized.
type ResizeMatrixFunc func(ih Ihandle, width, height int) int

// ScrollTopFunc for SCROLLTOP_CB callback.
// Called when scrolling to top.
type ScrollTopFunc func(ih Ihandle, lin, col int) int

// TranslateValueFunc for TRANSLATEVALUE_CB callback.
// Called to translate cell value for display.
// Returns translated string.
type TranslateValueFunc func(ih Ihandle, lin, col int, value string) string

// ValueEditFunc for VALUE_EDIT_CB callback.
// Called to edit cell value.
type ValueEditFunc func(ih Ihandle, lin, col int, newval string) int

// MatrixListActionFunc for ACTION_CB and IMAGEVALUECHANGED_CB callbacks in MatrixList.
// Called when item selection or image value changes.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_matrixlist.html
type MatrixListActionFunc func(ih Ihandle, item, state int) int

// FlatActionFunc for FLAT_ACTION callback.
// Action generated when the button 1 (usually left) is selected.
type FlatActionFunc func(ih Ihandle) int

// FlatListActionFunc for FlatList FLAT_ACTION callback.
// Action generated when the state of an item in the list is interactively changed.
type FlatListActionFunc func(ih Ihandle, text string, item, state int) int

// FlatToggleActionFunc for FlatToggle FLAT_ACTION callback.
// Action generated when the toggle's state (on/off) was changed.
type FlatToggleActionFunc func(ih Ihandle, state int) int

// PlayEndFunc for PLAYEND_CB callback.
// Called when playback reaches the end of the file.
type PlayEndFunc func(ih Ihandle) int

// ReadyFunc for READY_CB callback.
// Called when the source is ready to play.
type ReadyFunc func(ih Ihandle) int

// StateChangedFunc for STATECHANGED_CB callback.
// Called when the playback state changes.
type StateChangedFunc func(ih Ihandle, state string) int

// FrameFunc for FRAME_CB callback.
// Called for every captured frame, before it is drawn. The data slice holds packed RGB pixels and is valid only during the callback.
type FrameFunc func(ih Ihandle, width, height int, data []byte) int

// SamplesFunc for SAMPLES_CB callback.
// Called for every block of captured audio. The samples slice holds 16-bit interleaved frames and is valid only during the callback.
type SamplesFunc func(ih Ihandle, frames, channels int, samples []int16) int

// PlotDrawFunc for PREDRAW_CB and POSTDRAW_CB callbacks in IupPlot.
type PlotDrawFunc func(ih Ihandle) int

// PlotClickSampleFunc for CLICKSAMPLE_CB callback in IupPlot.
type PlotClickSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, button int) int

// PlotClickSegmentFunc for CLICKSEGMENT_CB callback in IupPlot.
type PlotClickSegmentFunc func(ih Ihandle, dsIndex, sampleIndex1 int, x1, y1 float64, sampleIndex2 int, x2, y2 float64, button int) int

// PlotDrawSampleFunc for DRAWSAMPLE_CB callback in IupPlot.
type PlotDrawSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, state int) int

// PlotMotionFunc for PLOTMOTION_CB callback in IupPlot.
type PlotMotionFunc func(ih Ihandle, x, y float64, status string) int

// PlotButtonFunc for PLOTBUTTON_CB callback in IupPlot.
type PlotButtonFunc func(ih Ihandle, button, pressed int, x, y float64, status string) int

// PlotEditSampleFunc for EDITSAMPLE_CB callback in IupPlot.
type PlotEditSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

// PlotDeleteFunc for DELETE_CB callback in IupPlot.
type PlotDeleteFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

// PlotDeleteBeginFunc for DELETEBEGIN_CB callback in IupPlot.
type PlotDeleteBeginFunc func(ih Ihandle) int

// PlotDeleteEndFunc for DELETEEND_CB callback in IupPlot.
type PlotDeleteEndFunc func(ih Ihandle) int

// PlotSelectFunc for SELECT_CB callback in IupPlot.
type PlotSelectFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, selected int) int

// PlotSelectBeginFunc for SELECTBEGIN_CB callback in IupPlot.
type PlotSelectBeginFunc func(ih Ihandle) int

// PlotSelectEndFunc for SELECTEND_CB callback in IupPlot.
type PlotSelectEndFunc func(ih Ihandle) int

// PlotMenuContextFunc for MENUCONTEXT_CB callback in IupPlot.
type PlotMenuContextFunc func(ih, menu Ihandle, x, y int) int

// PlotMenuContextCloseFunc for MENUCONTEXTCLOSE_CB callback in IupPlot.
type PlotMenuContextCloseFunc func(ih, menu Ihandle, x, y int) int

// PlotPropertiesChangedFunc for PROPERTIESCHANGED_CB callback in IupPlot.
type PlotPropertiesChangedFunc func(ih Ihandle) int

// PlotPropertiesValidateFunc for PROPERTIESVALIDATE_CB callback in IupPlot.
type PlotPropertiesValidateFunc func(ih Ihandle, name, value string) int

// PlotDSPropertiesChangedFunc for DSPROPERTIESCHANGED_CB callback in IupPlot.
type PlotDSPropertiesChangedFunc func(ih Ihandle, dsIndex int) int

// PlotDSPropertiesValidateFunc for DSPROPERTIESVALIDATE_CB callback in IupPlot.
type PlotDSPropertiesValidateFunc func(ih, dialog Ihandle, dsIndex int) int

// PlotTickFormatNumberFunc for XTICKFORMATNUMBER_CB and YTICKFORMATNUMBER_CB callbacks in IupPlot.
// Returns the tick text and DEFAULT to use it, CONTINUE for the internal formatter, or IGNORE.
type PlotTickFormatNumberFunc func(ih Ihandle, format string, value float64, decimalSymbol string) (string, int)

// CompletedFunc for COMPLETED_CB callback.
// Action generated when a page is successfully completed.
type CompletedFunc func(ih Ihandle, url string) int

// ErrorFunc for ERROR_CB callback.
// Called when a page load or media playback fails.
type ErrorFunc func(ih Ihandle, url string) int

// NavigateFunc for NAVIGATE_CB callback.
// Action generated when the browser requests a navigation to another page.
type NavigateFunc func(ih Ihandle, url string) int

// NewWindowFunc for NEWWINDOW_CB callback.
// Action generated when the browser requests a new window.
type NewWindowFunc func(ih Ihandle, url string) int

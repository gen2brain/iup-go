package iup

import (
	"fmt"
	"reflect"
	"strings"
)

// Callback is an IUP callback name typed with the function it takes, for use with [On].
type Callback[F any] struct{ name string }

// Name returns the IUP name of the callback, as used by [SetCallback].
func (cb Callback[F]) Name() string { return cb.name }

// On sets the callback cb of ih to fn and returns ih. SetCallback(ih, name, nil) removes it.
//
//	iup.On(btn, iup.ActionCB, func(ih iup.Ihandle) int { return iup.DEFAULT })
//
// https://gen2brain.github.io/iup-go/func/iup_setcallback.html
func On[F any](ih Ihandle, cb Callback[F], fn F) Ihandle {
	SetCallback(ih, cb.name, fn)
	return ih
}

var callbackTypes = map[string][]reflect.Type{}

func newCallback[F any](name string) Callback[F] {
	callbackTypes[name] = append(callbackTypes[name], reflect.TypeOf((*F)(nil)).Elem())
	return Callback[F]{name}
}

func checkCallback(name string, fn any) any {
	v := reflect.ValueOf(fn)
	if v.Kind() == reflect.Func && v.IsNil() {
		return nil
	}

	types, ok := callbackTypes[name]
	if !ok {
		panic(fmt.Sprintf("iup: unknown callback %q", name))
	}

	t := v.Type()
	for _, ct := range types {
		if t == ct {
			return fn
		}
	}

	var match []reflect.Type
	if t.Name() == "" {
		for _, ct := range types {
			if t.ConvertibleTo(ct) {
				match = append(match, ct)
			}
		}
	}

	if len(match) == 1 {
		return v.Convert(match[0]).Interface()
	}

	names := make([]string, len(types))
	for i, ct := range types {
		names[i] = ct.String()
	}

	switch len(match) {
	case 0:
		panic(fmt.Sprintf("iup: callback %s takes %s, not %s", name, strings.Join(names, " or "), t))
	default:
		panic(fmt.Sprintf("iup: callback %s is ambiguous for %s, convert it to %s or use On", name, t, strings.Join(names, " or ")))
	}
}

// Callbacks for [On], one per function type of a name, such as [ActionCB] and [ListActionCB].
var (
	ActionCB                      = newCallback[ActionFunc]("ACTION")
	BgColorCB                     = newCallback[BgColorFunc]("BGCOLOR_CB")
	BranchCloseCB                 = newCallback[BranchCloseFunc]("BRANCHCLOSE_CB")
	BranchOpenCB                  = newCallback[BranchOpenFunc]("BRANCHOPEN_CB")
	BusyCB                        = newCallback[BusyFunc]("BUSY_CB")
	ButtonCB                      = newCallback[ButtonFunc]("BUTTON_CB")
	CancelCB                      = newCallback[CancelFunc]("CANCEL_CB")
	CaretCB                       = newCallback[CaretFunc]("CARET_CB")
	CellCB                        = newCallback[CellFunc]("CELL_CB")
	CellsDrawCB                   = newCallback[CellsDrawFunc]("DRAW_CB")
	ChangeCB                      = newCallback[ChangeFunc]("CHANGE_CB")
	ClickCB                       = newCallback[ClickFunc]("CLICK_CB")
	CloseCB                       = newCallback[CloseFunc]("CLOSE_CB")
	ColResizeCB                   = newCallback[ColResizeFunc]("COLRESIZE_CB")
	ColorUpdateCB                 = newCallback[ColorUpdateFunc]("COLORUPDATE_CB")
	CompletedCB                   = newCallback[CompletedFunc]("COMPLETED_CB")
	DblclickCB                    = newCallback[DblclickFunc]("DBLCLICK_CB")
	DestroyCB                     = newCallback[DestroyFunc]("DESTROY_CB")
	DetachedCB                    = newCallback[DetachedFunc]("DETACHED_CB")
	DragBeginCB                   = newCallback[DragBeginFunc]("DRAGBEGIN_CB")
	DragCB                        = newCallback[DragFunc]("DRAG_CB")
	DragDataCB                    = newCallback[DragDataFunc]("DRAGDATA_CB")
	DragDataSizeCB                = newCallback[DragDataSizeFunc]("DRAGDATASIZE_CB")
	DragDropCB                    = newCallback[DragDropFunc]("DRAGDROP_CB")
	DragEndCB                     = newCallback[DragEndFunc]("DRAGEND_CB")
	DropCheckCB                   = newCallback[DropCheckFunc]("DROPCHECK_CB")
	DropDataCB                    = newCallback[DropDataFunc]("DROPDATA_CB")
	DropDownCB                    = newCallback[DropDownFunc]("DROPDOWN_CB")
	DropFilesCB                   = newCallback[DropFilesFunc]("DROPFILES_CB")
	DropMotionCB                  = newCallback[DropMotionFunc]("DROPMOTION_CB")
	DropSelectCB                  = newCallback[DropSelectFunc]("DROPSELECT_CB")
	DropShowCB                    = newCallback[DropShowFunc]("DROPSHOW_CB")
	EditBeginCB                   = newCallback[EditBeginFunc]("EDITBEGIN_CB")
	EditCB                        = newCallback[EditFunc]("EDIT_CB")
	EditClickCB                   = newCallback[EditClickFunc]("EDITCLICK_CB")
	EditEndCB                     = newCallback[EditEndFunc]("EDITEND_CB")
	EditMouseMoveCB               = newCallback[EditMouseMoveFunc]("EDITMOUSEMOVE_CB")
	EditReleaseCB                 = newCallback[EditReleaseFunc]("EDITRELEASE_CB")
	EditionCB                     = newCallback[EditionFunc]("EDITION_CB")
	EnterItemCB                   = newCallback[EnterItemFunc]("ENTERITEM_CB")
	EnterWindowCB                 = newCallback[EnterWindowFunc]("ENTERWINDOW_CB")
	ErrorCB                       = newCallback[ErrorFunc]("ERROR_CB")
	ExecuteBranchCB               = newCallback[ExecuteBranchFunc]("EXECUTEBRANCH_CB")
	ExecuteLeafCB                 = newCallback[ExecuteLeafFunc]("EXECUTELEAF_CB")
	ExtendedCB                    = newCallback[ExtendedFunc]("EXTENDED_CB")
	ExtraButtonCB                 = newCallback[ExtraButtonFunc]("EXTRABUTTON_CB")
	FgColorCB                     = newCallback[FgColorFunc]("FGCOLOR_CB")
	FileCB                        = newCallback[FileFunc]("FILE_CB")
	FlatActionCB                  = newCallback[FlatActionFunc]("FLAT_ACTION")
	FlatButtonCB                  = newCallback[ButtonFunc]("FLAT_BUTTON_CB")
	FlatEnterWindowCB             = newCallback[EnterWindowFunc]("FLAT_ENTERWINDOW_CB")
	FlatFocusCB                   = newCallback[FocusFunc]("FLAT_FOCUS_CB")
	FlatGetFocusCB                = newCallback[GetFocusFunc]("FLAT_GETFOCUS_CB")
	FlatKillFocusCB               = newCallback[KillFocusFunc]("FLAT_KILLFOCUS_CB")
	FlatLeaveWindowCB             = newCallback[LeaveWindowFunc]("FLAT_LEAVEWINDOW_CB")
	FlatListActionCB              = newCallback[FlatListActionFunc]("FLAT_ACTION")
	FlatMotionCB                  = newCallback[MotionFunc]("FLAT_MOTION_CB")
	FlatToggleActionCB            = newCallback[FlatToggleActionFunc]("FLAT_ACTION")
	FlatWheelCB                   = newCallback[WheelFunc]("FLAT_WHEEL_CB")
	FocusCB                       = newCallback[FocusFunc]("FOCUS_CB")
	FrameCB                       = newCallback[FrameFunc]("FRAME_CB")
	GestureCB                     = newCallback[GestureFunc]("GESTURE_CB")
	GetFocusCB                    = newCallback[GetFocusFunc]("GETFOCUS_CB")
	HSpanCB                       = newCallback[HSpanFunc]("HSPAN_CB")
	HeightCB                      = newCallback[HeightFunc]("HEIGHT_CB")
	HelpCB                        = newCallback[HelpFunc]("HELP_CB")
	HighlightCB                   = newCallback[HighlightFunc]("HIGHLIGHT_CB")
	KAnyCB                        = newCallback[KAnyFunc]("K_ANY")
	KeyPressCB                    = newCallback[KeyPressFunc]("KEYPRESS_CB")
	KillFocusCB                   = newCallback[KillFocusFunc]("KILLFOCUS_CB")
	LayoutUpdateCB                = newCallback[LayoutUpdateFunc]("LAYOUTUPDATE_CB")
	LeaveItemCB                   = newCallback[LeaveItemFunc]("LEAVEITEM_CB")
	LeaveWindowCB                 = newCallback[LeaveWindowFunc]("LEAVEWINDOW_CB")
	LinkActionCB                  = newCallback[LinkActionFunc]("ACTION")
	ListActionCB                  = newCallback[ListActionFunc]("ACTION")
	ListClickCB                   = newCallback[ClickFunc]("LISTCLICK_CB")
	ListDrawCB                    = newCallback[ListDrawFunc]("LISTDRAW_CB")
	ListEditionCB                 = newCallback[ListEditionFunc]("LISTEDITION_CB")
	ListImageCB                   = newCallback[ListImageFunc]("IMAGE_CB")
	ListInsertCB                  = newCallback[ListInsertFunc]("LISTINSERT_CB")
	ListReleaseCB                 = newCallback[ListReleaseFunc]("LISTRELEASE_CB")
	ListRemoveCB                  = newCallback[ListRemoveFunc]("LISTREMOVE_CB")
	ListValueCB                   = newCallback[ListValueFunc]("VALUE_CB")
	LocationCB                    = newCallback[LocationFunc]("LOCATION_CB")
	MapCB                         = newCallback[MapFunc]("MAP_CB")
	MarkCB                        = newCallback[MarkFunc]("MARK_CB")
	MarkEditCB                    = newCallback[MarkEditFunc]("MARKEDIT_CB")
	MaskFailCB                    = newCallback[MaskFailFunc]("MASKFAIL_CB")
	MatrixActionCB                = newCallback[MatrixActionFunc]("ACTION_CB")
	MatrixDrawCB                  = newCallback[MatrixDrawFunc]("DRAW_CB")
	MatrixDropCB                  = newCallback[MatrixDropFunc]("DROP_CB")
	MatrixFontCB                  = newCallback[MatrixFontFunc]("FONT_CB")
	MatrixListActionCB            = newCallback[MatrixListActionFunc]("ACTION_CB")
	MatrixListImageValueChangedCB = newCallback[MatrixListActionFunc]("IMAGEVALUECHANGED_CB")
	MatrixMouseMoveCB             = newCallback[MatrixMouseMoveFunc]("MOUSEMOVE_CB")
	MatrixToggleValueCB           = newCallback[MatrixToggleValueFunc]("TOGGLEVALUE_CB")
	MatrixTypeCB                  = newCallback[MatrixTypeFunc]("TYPE_CB")
	MatrixValueCB                 = newCallback[MatrixValueFunc]("VALUE_CB")
	MenuCloseCB                   = newCallback[MenuCloseFunc]("MENUCLOSE_CB")
	MenuContextCB                 = newCallback[MenuContextFunc]("MENUCONTEXT_CB")
	MenuContextCloseCB            = newCallback[MenuContextCloseFunc]("MENUCONTEXTCLOSE_CB")
	MenuDropCB                    = newCallback[MenuDropFunc]("MENUDROP_CB")
	MenuOpenCB                    = newCallback[MenuOpenFunc]("MENUOPEN_CB")
	MotionCB                      = newCallback[MotionFunc]("MOTION_CB")
	MouseClickCB                  = newCallback[MouseClickFunc]("MOUSECLICK_CB")
	MouseMotionCB                 = newCallback[MouseMotionFunc]("MOUSEMOTION_CB")
	MoveCB                        = newCallback[MoveFunc]("MOVE_CB")
	MultiSelectionCB              = newCallback[MultiSelectionFunc]("MULTISELECTION_CB")
	MultiTouchCB                  = newCallback[MultiTouchFunc]("MULTITOUCH_CB")
	MultiUnselectionCB            = newCallback[MultiUnselectionFunc]("MULTIUNSELECTION_CB")
	MultiselectCB                 = newCallback[MultiselectFunc]("MULTISELECT_CB")
	NColsCB                       = newCallback[NColsFunc]("NCOLS_CB")
	NLinesCB                      = newCallback[NLinesFunc]("NLINES_CB")
	NavigateCB                    = newCallback[NavigateFunc]("NAVIGATE_CB")
	NewWindowCB                   = newCallback[NewWindowFunc]("NEWWINDOW_CB")
	NodeRemovedCB                 = newCallback[NodeRemovedFunc]("NODEREMOVED_CB")
	NotifyCB                      = newCallback[NotifyFunc]("NOTIFY_CB")
	NotifyCloseCB                 = newCallback[NotifyCloseFunc]("CLOSE_CB")
	NumericGetValueCB             = newCallback[NumericGetValueFunc]("NUMERICGETVALUE_CB")
	NumericSetValueCB             = newCallback[NumericSetValueFunc]("NUMERICSETVALUE_CB")
	OpenCloseCB                   = newCallback[OpenCloseFunc]("OPENCLOSE_CB")
	ParamCB                       = newCallback[ParamFunc]("PARAM_CB")
	PasteSizeCB                   = newCallback[PasteSizeFunc]("PASTESIZE_CB")
	PermissionCB                  = newCallback[PermissionFunc]("PERMISSION_CB")
	PlayEndCB                     = newCallback[PlayEndFunc]("PLAYEND_CB")
	PlotButtonCB                  = newCallback[PlotButtonFunc]("PLOTBUTTON_CB")
	PlotClickSampleCB             = newCallback[PlotClickSampleFunc]("CLICKSAMPLE_CB")
	PlotClickSegmentCB            = newCallback[PlotClickSegmentFunc]("CLICKSEGMENT_CB")
	PlotDSPropertiesChangedCB     = newCallback[PlotDSPropertiesChangedFunc]("DSPROPERTIESCHANGED_CB")
	PlotDSPropertiesValidateCB    = newCallback[PlotDSPropertiesValidateFunc]("DSPROPERTIESVALIDATE_CB")
	PlotDeleteBeginCB             = newCallback[PlotDeleteBeginFunc]("DELETEBEGIN_CB")
	PlotDeleteCB                  = newCallback[PlotDeleteFunc]("DELETE_CB")
	PlotDeleteEndCB               = newCallback[PlotDeleteEndFunc]("DELETEEND_CB")
	PlotDrawSampleCB              = newCallback[PlotDrawSampleFunc]("DRAWSAMPLE_CB")
	PlotEditSampleCB              = newCallback[PlotEditSampleFunc]("EDITSAMPLE_CB")
	PlotMenuContextCB             = newCallback[PlotMenuContextFunc]("MENUCONTEXT_CB")
	PlotMenuContextCloseCB        = newCallback[PlotMenuContextCloseFunc]("MENUCONTEXTCLOSE_CB")
	PlotMotionCB                  = newCallback[PlotMotionFunc]("PLOTMOTION_CB")
	PlotPostDrawCB                = newCallback[PlotDrawFunc]("POSTDRAW_CB")
	PlotPreDrawCB                 = newCallback[PlotDrawFunc]("PREDRAW_CB")
	PlotPropertiesChangedCB       = newCallback[PlotPropertiesChangedFunc]("PROPERTIESCHANGED_CB")
	PlotPropertiesValidateCB      = newCallback[PlotPropertiesValidateFunc]("PROPERTIESVALIDATE_CB")
	PlotSelectBeginCB             = newCallback[PlotSelectBeginFunc]("SELECTBEGIN_CB")
	PlotSelectCB                  = newCallback[PlotSelectFunc]("SELECT_CB")
	PlotSelectEndCB               = newCallback[PlotSelectEndFunc]("SELECTEND_CB")
	PlotXTickFormatNumberCB       = newCallback[PlotTickFormatNumberFunc]("XTICKFORMATNUMBER_CB")
	PlotYTickFormatNumberCB       = newCallback[PlotTickFormatNumberFunc]("YTICKFORMATNUMBER_CB")
	PostMessageCB                 = newCallback[PostMessageFunc]("POSTMESSAGE_CB")
	ReleaseCB                     = newCallback[ReleaseFunc]("RELEASE_CB")
	RenameCB                      = newCallback[RenameFunc]("RENAME_CB")
	ReorderCB                     = newCallback[ReorderFunc]("REORDER_CB")
	ResizeCB                      = newCallback[ResizeFunc]("RESIZE_CB")
	ResizeMatrixCB                = newCallback[ResizeMatrixFunc]("RESIZEMATRIX_CB")
	RestoredCB                    = newCallback[RestoredFunc]("RESTORED_CB")
	RightClickCB                  = newCallback[RightClickFunc]("RIGHTCLICK_CB")
	SamplesCB                     = newCallback[SamplesFunc]("SAMPLES_CB")
	ScrollCB                      = newCallback[ScrollFunc]("SCROLL_CB")
	ScrollTopCB                   = newCallback[ScrollTopFunc]("SCROLLTOP_CB")
	ScrollingCB                   = newCallback[ScrollingFunc]("SCROLLING_CB")
	SelectCB                      = newCallback[SelectFunc]("SELECT_CB")
	SelectionCB                   = newCallback[SelectionFunc]("SELECTION_CB")
	SensorCB                      = newCallback[SensorFunc]("SENSOR_CB")
	ShowCB                        = newCallback[ShowFunc]("SHOW_CB")
	ShowRenameCB                  = newCallback[ShowRenameFunc]("SHOWRENAME_CB")
	SortColumnCompareCB           = newCallback[SortColumnCompareFunc]("SORTCOLUMNCOMPARE_CB")
	SpinCB                        = newCallback[SpinFunc]("SPIN_CB")
	SwapBuffersCB                 = newCallback[SwapBuffersFunc]("SWAPBUFFERS_CB")
	SwitchCB                      = newCallback[SwitchFunc]("SWITCH_CB")
	TabChangeCB                   = newCallback[TabChangeFunc]("TABCHANGE_CB")
	TabChangePosCB                = newCallback[TabChangePosFunc]("TABCHANGEPOS_CB")
	TabCloseCB                    = newCallback[TabCloseFunc]("TABCLOSE_CB")
	TableEditionCB                = newCallback[TableEditionFunc]("EDITION_CB")
	TableImageCB                  = newCallback[TableImageFunc]("IMAGE_CB")
	TableSortCB                   = newCallback[TableSortFunc]("SORT_CB")
	TableValueCB                  = newCallback[TableValueFunc]("VALUE_CB")
	TableValueChangedCB           = newCallback[TableValueChangedFunc]("VALUECHANGED_CB")
	TerminalBellCB                = newCallback[TerminalBellFunc]("BELL_CB")
	TerminalExitCB                = newCallback[TerminalExitFunc]("EXIT_CB")
	TerminalInputCB               = newCallback[TerminalInputFunc]("INPUT_CB")
	TerminalSizeCB                = newCallback[TerminalSizeFunc]("TERMSIZE_CB")
	TerminalTitleCB               = newCallback[TerminalTitleFunc]("TITLE_CB")
	TextActionCB                  = newCallback[TextActionFunc]("ACTION")
	TextInputCB                   = newCallback[TextInputFunc]("TEXTINPUT_CB")
	TextLinkCB                    = newCallback[TextLinkFunc]("LINK_CB")
	ThemeChangedCB                = newCallback[ThemeChangedFunc]("THEMECHANGED_CB")
	ThreadCB                      = newCallback[ThreadFunc]("THREAD_CB")
	TimerActionCB                 = newCallback[TimerActionFunc]("ACTION_CB")
	TipsCB                        = newCallback[TipsFunc]("TIPS_CB")
	ToggleActionCB                = newCallback[ToggleActionFunc]("ACTION")
	ToggleValueCB                 = newCallback[ToggleValueFunc]("TOGGLEVALUE_CB")
	TouchCB                       = newCallback[TouchFunc]("TOUCH_CB")
	TranslateValueCB              = newCallback[TranslateValueFunc]("TRANSLATEVALUE_CB")
	TrayClickCB                   = newCallback[TrayClickFunc]("TRAYCLICK_CB")
	UnmapCB                       = newCallback[UnmapFunc]("UNMAP_CB")
	UpdateCB                      = newCallback[UpdateFunc]("UPDATE_CB")
	VSpanCB                       = newCallback[VSpanFunc]("VSPAN_CB")
	ValueChangedCB                = newCallback[ValueChangedFunc]("VALUECHANGED_CB")
	ValueChangingCB               = newCallback[ValueChangingFunc]("VALUECHANGING_CB")
	ValueEditCB                   = newCallback[ValueEditFunc]("VALUE_EDIT_CB")
	WheelCB                       = newCallback[WheelFunc]("WHEEL_CB")
	WidthCB                       = newCallback[WidthFunc]("WIDTH_CB")
)

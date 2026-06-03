//go:build js && wasm

package iup

type BgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)
type BusyFunc func(ih Ihandle, lin, col int, status string) int
type CancelFunc func(Ihandle) int
type CaretFunc func(ih Ihandle, lin, col, pos int) int
type CellFunc func(ih Ihandle, cell int) int
type CellsDrawFunc func(ih Ihandle, i, j, xmin, xmax, ymin, ymax int) int
type ChangeFunc func(ih Ihandle, r, g, b uint8) int
type ColorUpdateFunc func(ih Ihandle) int
type ColResizeFunc func(ih Ihandle, col int) int
type DestroyFunc func(Ihandle) int
type DetachedFunc func(Ihandle, Ihandle, int, int) int
type DragFunc func(ih Ihandle, r, g, b uint8) int
type DropCheckFunc func(ih Ihandle, lin, col int) int
type DropSelectFunc func(ih Ihandle, lin, col int, drop Ihandle, text string, item, col2 int) int
type DropShowFunc func(ih Ihandle, state int) int
type EditBeginFunc func(ih Ihandle, lin, col int) int
type EditClickFunc func(ih Ihandle, lin, col int, status string) int
type EditEndFunc func(ih Ihandle, lin, col int, newValue string, apply int) int
type EditionFunc func(ih Ihandle, lin, col, mode, update int) int
type EditMouseMoveFunc func(ih Ihandle, lin, col int) int
type EditReleaseFunc func(ih Ihandle, lin, col int, status string) int
type EnterWindowFunc func(Ihandle) int
type EntryPointFunc func()
type ExitFunc func()
type ExtendedFunc func(ih Ihandle, cell int) int
type ExtraButtonFunc func(Ihandle, int, int) int
type FgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)
type FileFunc func(ih Ihandle, filename, status string) int
type FlatActionFunc func(ih Ihandle) int
type FlatListActionFunc func(ih Ihandle, text string, item, state int) int
type FlatToggleActionFunc func(ih Ihandle, state int) int
type FocusFunc func(Ihandle, int) int
type GestureFunc func(ih Ihandle, gesture, state, x, y int, v1, v2 float64) int
type GetParamFunc func(dialog Ihandle, paramIndex int) int
type HeightFunc func(ih Ihandle, i int) int
type HelpFunc func(Ihandle) int
type HSpanFunc func(ih Ihandle, i, j int) int
type IdleFunc func() int
type KeyPressFunc func(ih Ihandle, c, press int) int
type LayoutUpdateFunc func(ih Ihandle) int
type LeaveItemFunc func(ih Ihandle, lin, col int) int
type LeaveWindowFunc func(Ihandle) int
type LinkActionFunc func(ih Ihandle, url string) int
type ListDrawFunc func(ih Ihandle, lin, x1, x2, y1, y2, canvas int) int
type ListEditionFunc func(ih Ihandle, lin, col, mode, update int) int
type ListImageFunc func(ih Ihandle, pos int) string
type ListInsertFunc func(ih Ihandle, pos int) int
type ListReleaseFunc func(ih Ihandle, lin, col int, status string) int
type ListRemoveFunc func(ih Ihandle, pos int) int
type ListValueFunc func(ih Ihandle, pos int) string
type MapFunc func(Ihandle) int
type MarkEditFunc func(ih Ihandle, lin, col, marked int) int
type MarkFunc func(ih Ihandle, lin, col int) int
type MaskFailFunc func(ih Ihandle, newValue string) int
type MatrixActionFunc func(ih Ihandle, key, lin, col, edition int, status string) int
type MatrixDrawFunc func(ih Ihandle, lin, col, x1, x2, y1, y2 int) int
type MatrixDropFunc func(ih, drop Ihandle, lin, col int) int
type MatrixFontFunc func(ih Ihandle, lin, col int) string
type MatrixListActionFunc func(ih Ihandle, item, state int) int
type MatrixMouseMoveFunc func(ih Ihandle, lin, col int) int
type MatrixToggleValueFunc func(ih Ihandle, lin, col, value int) int
type MatrixTypeFunc func(ih Ihandle, lin, col int) string
type MatrixValueFunc func(ih Ihandle, lin, col int) string
type MenuContextCloseFunc func(ih, menu Ihandle, lin, col int) int
type MenuContextFunc func(ih, menu Ihandle, lin, col int) int
type MenuDropFunc func(ih, menu Ihandle, lin, col int) int
type MouseClickFunc func(ih Ihandle, button, pressed, i, j, x, y int, status string) int
type MouseMotionFunc func(ih Ihandle, i, j, x, y int, status string) int
type MoveFunc func(ih Ihandle, x, y int) int
type MultiSelectionFunc func(ih Ihandle, ids []int, n int) int
type MultiTouchFunc func(ih Ihandle, count int, pid, px, py, pstate []int) int
type MultiUnselectionFunc func(ih Ihandle, ids []int, n int) int
type NColsFunc func(ih Ihandle) int
type NLinesFunc func(ih Ihandle) int
type NodeRemovedFunc func(ih Ihandle, userId uintptr) int
type NumericGetValueFunc func(ih Ihandle, lin, col int) float64
type NumericSetValueFunc func(ih Ihandle, lin, col int, value float64) int
type OpenCloseFunc func(ih Ihandle, state int) int
type PasteSizeFunc func(ih Ihandle, numlin, numcol int) int
type PlotButtonFunc func(ih Ihandle, button, pressed int, x, y float64, status string) int
type PlotClickSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, button int) int
type PlotClickSegmentFunc func(ih Ihandle, dsIndex, sampleIndex1 int, x1, y1 float64, sampleIndex2 int, x2, y2 float64, button int) int
type PlotDeleteBeginFunc func(ih Ihandle) int
type PlotDeleteEndFunc func(ih Ihandle) int
type PlotDeleteFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int
type PlotDrawFunc func(ih Ihandle) int
type PlotDrawSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, state int) int
type PlotDSPropertiesChangedFunc func(ih Ihandle, dsIndex int) int
type PlotDSPropertiesValidateFunc func(ih, param1, param2 Ihandle, dsIndex int) int
type PlotEditSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int
type PlotMenuContextCloseFunc func(ih, menu Ihandle, x, y int) int
type PlotMenuContextFunc func(ih, menu Ihandle, x, y int) int
type PlotMotionFunc func(ih Ihandle, x, y float64, status string) int
type PlotPropertiesChangedFunc func(ih Ihandle) int
type PlotPropertiesValidateFunc func(ih Ihandle, name, value string) int
type PlotSelectBeginFunc func(ih Ihandle) int
type PlotSelectEndFunc func(ih Ihandle) int
type PlotSelectFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, selected int) int
type PlotTickFormatNumberFunc func(ih Ihandle, format, outStr string, value float64, status string) int
type PostMessageFunc func(Ihandle, string, int, any) int
type ReleaseFunc func(ih Ihandle, lin, col int, status string) int
type ResizeMatrixFunc func(ih Ihandle, width, height int) int
type RestoredFunc func(Ihandle, Ihandle, int, int) int
type ScrollingFunc func(ih Ihandle, i, j int) int
type ScrollTopFunc func(ih Ihandle, lin, col int) int
type SelectFunc func(ih Ihandle, cell, _type int) int
type SortColumnCompareFunc func(ih Ihandle, lin1, lin2, col int) int
type SpinFunc func(ih Ihandle, inc int) int
type SwapBuffersFunc func(Ihandle) int
type SwitchFunc func(ih Ihandle, primCell, secCell int) int
type TableImageFunc func(ih Ihandle, lin, col int) string
type TableSortFunc func(ih Ihandle, col int) int
type TableValueFunc func(ih Ihandle, lin, col int) string
type TextLinkFunc func(ih Ihandle, url string) int
type ThreadFunc func(ih Ihandle) int
type TipsFunc func(ih Ihandle, x, y int) int
type ToggleValueFunc func(ih Ihandle, id, state int) int
type TouchFunc func(ih Ihandle, id, x, y int, state string) int
type TranslateValueFunc func(ih Ihandle, lin, col int, value string) string
type TrayClickFunc func(Ihandle, int, int, int) int
type UnmapFunc func(Ihandle) int
type ValueChangingFunc func(ih Ihandle, start int) int
type ValueEditFunc func(ih Ihandle, lin, col int, newval string) int
type VSpanFunc func(ih Ihandle, i, j int) int
type WidthFunc func(ih Ihandle, j int) int

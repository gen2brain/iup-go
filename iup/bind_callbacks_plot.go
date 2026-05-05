package iup

import (
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

// PlotDrawFunc for PREDRAW_CB and POSTDRAW_CB callbacks in IupPlot.
type PlotDrawFunc func(ih Ihandle) int

//export goIupPlotPreDrawCB
func goIupPlotPreDrawCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PREDRAW_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDrawFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotPreDrawFunc(ih Ihandle, f PlotDrawFunc) {
	storeCallback(ih, "_IUPGO_PREDRAW_CB", f)
	C.goIupSetPlotPreDrawFunc(ih.ptr())
}

//export goIupPlotPostDrawCB
func goIupPlotPostDrawCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_POSTDRAW_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDrawFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotPostDrawFunc(ih Ihandle, f PlotDrawFunc) {
	storeCallback(ih, "_IUPGO_POSTDRAW_CB", f)
	C.goIupSetPlotPostDrawFunc(ih.ptr())
}

//--------------------

// PlotClickSampleFunc for CLICKSAMPLE_CB callback in IupPlot.
type PlotClickSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, button int) int

//export goIupPlotClickSampleCB
func goIupPlotClickSampleCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double, button C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_CLICKSAMPLE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotClickSampleFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y), int(button)))
}

func setPlotClickSampleFunc(ih Ihandle, f PlotClickSampleFunc) {
	storeCallback(ih, "_IUPGO_CLICKSAMPLE_CB", f)
	C.goIupSetPlotClickSampleFunc(ih.ptr())
}

//--------------------

// PlotClickSegmentFunc for CLICKSEGMENT_CB callback in IupPlot.
type PlotClickSegmentFunc func(ih Ihandle, dsIndex, sampleIndex1 int, x1, y1 float64, sampleIndex2 int, x2, y2 float64, button int) int

//export goIupPlotClickSegmentCB
func goIupPlotClickSegmentCB(ih unsafe.Pointer, dsIndex, sampleIndex1 C.int, x1, y1 C.double, sampleIndex2 C.int, x2, y2 C.double, button C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_CLICKSEGMENT_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotClickSegmentFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex1), float64(x1), float64(y1), int(sampleIndex2), float64(x2), float64(y2), int(button)))
}

func setPlotClickSegmentFunc(ih Ihandle, f PlotClickSegmentFunc) {
	storeCallback(ih, "_IUPGO_CLICKSEGMENT_CB", f)
	C.goIupSetPlotClickSegmentFunc(ih.ptr())
}

//--------------------

// PlotDrawSampleFunc for DRAWSAMPLE_CB callback in IupPlot.
type PlotDrawSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, state int) int

//export goIupPlotDrawSampleCB
func goIupPlotDrawSampleCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double, state C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAWSAMPLE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDrawSampleFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y), int(state)))
}

func setPlotDrawSampleFunc(ih Ihandle, f PlotDrawSampleFunc) {
	storeCallback(ih, "_IUPGO_DRAWSAMPLE_CB", f)
	C.goIupSetPlotDrawSampleFunc(ih.ptr())
}

//--------------------

// PlotMotionFunc for PLOTMOTION_CB callback in IupPlot.
type PlotMotionFunc func(ih Ihandle, x, y float64, status string) int

//export goIupPlotMotionCB
func goIupPlotMotionCB(ih unsafe.Pointer, x, y C.double, status unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PLOTMOTION_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotMotionFunc)
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), float64(x), float64(y), goStatus))
}

func setPlotMotionFunc(ih Ihandle, f PlotMotionFunc) {
	storeCallback(ih, "_IUPGO_PLOTMOTION_CB", f)
	C.goIupSetPlotMotionFunc(ih.ptr())
}

//--------------------

// PlotButtonFunc for PLOTBUTTON_CB callback in IupPlot.
type PlotButtonFunc func(ih Ihandle, button, pressed int, x, y float64, status string) int

//export goIupPlotButtonCB
func goIupPlotButtonCB(ih unsafe.Pointer, button, pressed C.int, x, y C.double, status unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PLOTBUTTON_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotButtonFunc)
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(button), int(pressed), float64(x), float64(y), goStatus))
}

func setPlotButtonFunc(ih Ihandle, f PlotButtonFunc) {
	storeCallback(ih, "_IUPGO_PLOTBUTTON_CB", f)
	C.goIupSetPlotButtonFunc(ih.ptr())
}

//--------------------

// PlotEditSampleFunc for EDITSAMPLE_CB callback in IupPlot.
type PlotEditSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

//export goIupPlotEditSampleCB
func goIupPlotEditSampleCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_EDITSAMPLE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotEditSampleFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y)))
}

func setPlotEditSampleFunc(ih Ihandle, f PlotEditSampleFunc) {
	storeCallback(ih, "_IUPGO_EDITSAMPLE_CB", f)
	C.goIupSetPlotEditSampleFunc(ih.ptr())
}

//--------------------

// PlotDeleteFunc for DELETE_CB callback in IupPlot.
type PlotDeleteFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

//export goIupPlotDeleteCB
func goIupPlotDeleteCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PLOT_DELETE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDeleteFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y)))
}

func setPlotDeleteFunc(ih Ihandle, f PlotDeleteFunc) {
	storeCallback(ih, "_IUPGO_PLOT_DELETE_CB", f)
	C.goIupSetPlotDeleteFunc(ih.ptr())
}

//--------------------

// PlotDeleteBeginFunc for DELETEBEGIN_CB callback in IupPlot.
type PlotDeleteBeginFunc func(ih Ihandle) int

//export goIupPlotDeleteBeginCB
func goIupPlotDeleteBeginCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DELETEBEGIN_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDeleteBeginFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotDeleteBeginFunc(ih Ihandle, f PlotDeleteBeginFunc) {
	storeCallback(ih, "_IUPGO_DELETEBEGIN_CB", f)
	C.goIupSetPlotDeleteBeginFunc(ih.ptr())
}

//--------------------

// PlotDeleteEndFunc for DELETEEND_CB callback in IupPlot.
type PlotDeleteEndFunc func(ih Ihandle) int

//export goIupPlotDeleteEndCB
func goIupPlotDeleteEndCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DELETEEND_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDeleteEndFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotDeleteEndFunc(ih Ihandle, f PlotDeleteEndFunc) {
	storeCallback(ih, "_IUPGO_DELETEEND_CB", f)
	C.goIupSetPlotDeleteEndFunc(ih.ptr())
}

//--------------------

// PlotSelectFunc for SELECT_CB callback in IupPlot.
type PlotSelectFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, selected int) int

//export goIupPlotSelectCB
func goIupPlotSelectCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double, selected C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PLOT_SELECT_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotSelectFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y), int(selected)))
}

func setPlotSelectFunc(ih Ihandle, f PlotSelectFunc) {
	storeCallback(ih, "_IUPGO_PLOT_SELECT_CB", f)
	C.goIupSetPlotSelectFunc(ih.ptr())
}

//--------------------

// PlotSelectBeginFunc for SELECTBEGIN_CB callback in IupPlot.
type PlotSelectBeginFunc func(ih Ihandle) int

//export goIupPlotSelectBeginCB
func goIupPlotSelectBeginCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_SELECTBEGIN_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotSelectBeginFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotSelectBeginFunc(ih Ihandle, f PlotSelectBeginFunc) {
	storeCallback(ih, "_IUPGO_SELECTBEGIN_CB", f)
	C.goIupSetPlotSelectBeginFunc(ih.ptr())
}

//--------------------

// PlotSelectEndFunc for SELECTEND_CB callback in IupPlot.
type PlotSelectEndFunc func(ih Ihandle) int

//export goIupPlotSelectEndCB
func goIupPlotSelectEndCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_SELECTEND_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotSelectEndFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotSelectEndFunc(ih Ihandle, f PlotSelectEndFunc) {
	storeCallback(ih, "_IUPGO_SELECTEND_CB", f)
	C.goIupSetPlotSelectEndFunc(ih.ptr())
}

//--------------------

// PlotMenuContextFunc for MENUCONTEXT_CB callback in IupPlot.
type PlotMenuContextFunc func(ih, menu Ihandle, x, y int) int

//export goIupPlotMenuContextCB
func goIupPlotMenuContextCB(ih, menu unsafe.Pointer, x, y C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PLOT_MENUCONTEXT_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotMenuContextFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(x), int(y)))
}

func setPlotMenuContextFunc(ih Ihandle, f PlotMenuContextFunc) {
	storeCallback(ih, "_IUPGO_PLOT_MENUCONTEXT_CB", f)
	C.goIupSetPlotMenuContextFunc(ih.ptr())
}

//--------------------

// PlotMenuContextCloseFunc for MENUCONTEXTCLOSE_CB callback in IupPlot.
type PlotMenuContextCloseFunc func(ih, menu Ihandle, x, y int) int

//export goIupPlotMenuContextCloseCB
func goIupPlotMenuContextCloseCB(ih, menu unsafe.Pointer, x, y C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PLOT_MENUCONTEXTCLOSE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotMenuContextCloseFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(x), int(y)))
}

func setPlotMenuContextCloseFunc(ih Ihandle, f PlotMenuContextCloseFunc) {
	storeCallback(ih, "_IUPGO_PLOT_MENUCONTEXTCLOSE_CB", f)
	C.goIupSetPlotMenuContextCloseFunc(ih.ptr())
}

//--------------------

// PlotPropertiesChangedFunc for PROPERTIESCHANGED_CB callback in IupPlot.
type PlotPropertiesChangedFunc func(ih Ihandle) int

//export goIupPlotPropertiesChangedCB
func goIupPlotPropertiesChangedCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PROPERTIESCHANGED_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotPropertiesChangedFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotPropertiesChangedFunc(ih Ihandle, f PlotPropertiesChangedFunc) {
	storeCallback(ih, "_IUPGO_PROPERTIESCHANGED_CB", f)
	C.goIupSetPlotPropertiesChangedFunc(ih.ptr())
}

//--------------------

// PlotPropertiesValidateFunc for PROPERTIESVALIDATE_CB callback in IupPlot.
type PlotPropertiesValidateFunc func(ih Ihandle, name, value string) int

//export goIupPlotPropertiesValidateCB
func goIupPlotPropertiesValidateCB(ih unsafe.Pointer, name, value unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PROPERTIESVALIDATE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotPropertiesValidateFunc)
	goName := C.GoString((*C.char)(name))
	goValue := C.GoString((*C.char)(value))
	return C.int(f((Ihandle)(ih), goName, goValue))
}

func setPlotPropertiesValidateFunc(ih Ihandle, f PlotPropertiesValidateFunc) {
	storeCallback(ih, "_IUPGO_PROPERTIESVALIDATE_CB", f)
	C.goIupSetPlotPropertiesValidateFunc(ih.ptr())
}

//--------------------

// PlotDSPropertiesChangedFunc for DSPROPERTIESCHANGED_CB callback in IupPlot.
type PlotDSPropertiesChangedFunc func(ih Ihandle, dsIndex int) int

//export goIupPlotDSPropertiesChangedCB
func goIupPlotDSPropertiesChangedCB(ih unsafe.Pointer, dsIndex C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DSPROPERTIESCHANGED_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDSPropertiesChangedFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex)))
}

func setPlotDSPropertiesChangedFunc(ih Ihandle, f PlotDSPropertiesChangedFunc) {
	storeCallback(ih, "_IUPGO_DSPROPERTIESCHANGED_CB", f)
	C.goIupSetPlotDSPropertiesChangedFunc(ih.ptr())
}

//--------------------

// PlotDSPropertiesValidateFunc for DSPROPERTIESVALIDATE_CB callback in IupPlot.
type PlotDSPropertiesValidateFunc func(ih, param1, param2 Ihandle, dsIndex int) int

//export goIupPlotDSPropertiesValidateCB
func goIupPlotDSPropertiesValidateCB(ih, param1, param2 unsafe.Pointer, dsIndex C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DSPROPERTIESVALIDATE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotDSPropertiesValidateFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(param1), (Ihandle)(param2), int(dsIndex)))
}

func setPlotDSPropertiesValidateFunc(ih Ihandle, f PlotDSPropertiesValidateFunc) {
	storeCallback(ih, "_IUPGO_DSPROPERTIESVALIDATE_CB", f)
	C.goIupSetPlotDSPropertiesValidateFunc(ih.ptr())
}

//--------------------

// PlotTickFormatNumberFunc for XTICKFORMATNUMBER_CB and YTICKFORMATNUMBER_CB callbacks in IupPlot.
type PlotTickFormatNumberFunc func(ih Ihandle, format, outStr string, value float64, status string) int

//export goIupPlotXTickFormatNumberCB
func goIupPlotXTickFormatNumberCB(ih unsafe.Pointer, format, outStr unsafe.Pointer, value C.double, status unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_XTICKFORMATNUMBER_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotTickFormatNumberFunc)
	goFormat := C.GoString((*C.char)(format))
	goOutStr := C.GoString((*C.char)(outStr))
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), goFormat, goOutStr, float64(value), goStatus))
}

func setPlotXTickFormatNumberFunc(ih Ihandle, f PlotTickFormatNumberFunc) {
	storeCallback(ih, "_IUPGO_XTICKFORMATNUMBER_CB", f)
	C.goIupSetPlotXTickFormatNumberFunc(ih.ptr())
}

//export goIupPlotYTickFormatNumberCB
func goIupPlotYTickFormatNumberCB(ih unsafe.Pointer, format, outStr unsafe.Pointer, value C.double, status unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_YTICKFORMATNUMBER_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PlotTickFormatNumberFunc)
	goFormat := C.GoString((*C.char)(format))
	goOutStr := C.GoString((*C.char)(outStr))
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), goFormat, goOutStr, float64(value), goStatus))
}

func setPlotYTickFormatNumberFunc(ih Ihandle, f PlotTickFormatNumberFunc) {
	storeCallback(ih, "_IUPGO_YTICKFORMATNUMBER_CB", f)
	C.goIupSetPlotYTickFormatNumberFunc(ih.ptr())
}

package iup

import (
	"runtime/cgo"
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
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PREDRAW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PREDRAW_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDrawFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotPreDrawFunc(ih Ihandle, f PlotDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PREDRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotPreDrawFunc(ih.ptr())
}

//export goIupPlotPostDrawCB
func goIupPlotPostDrawCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("POSTDRAW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "POSTDRAW_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDrawFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotPostDrawFunc(ih Ihandle, f PlotDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("POSTDRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotPostDrawFunc(ih.ptr())
}

//--------------------

// PlotClickSampleFunc for CLICKSAMPLE_CB callback in IupPlot.
type PlotClickSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, button int) int

//export goIupPlotClickSampleCB
func goIupPlotClickSampleCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double, button C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CLICKSAMPLE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CLICKSAMPLE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotClickSampleFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y), int(button)))
}

func setPlotClickSampleFunc(ih Ihandle, f PlotClickSampleFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CLICKSAMPLE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotClickSampleFunc(ih.ptr())
}

//--------------------

// PlotClickSegmentFunc for CLICKSEGMENT_CB callback in IupPlot.
type PlotClickSegmentFunc func(ih Ihandle, dsIndex, sampleIndex1 int, x1, y1 float64, sampleIndex2 int, x2, y2 float64, button int) int

//export goIupPlotClickSegmentCB
func goIupPlotClickSegmentCB(ih unsafe.Pointer, dsIndex, sampleIndex1 C.int, x1, y1 C.double, sampleIndex2 C.int, x2, y2 C.double, button C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CLICKSEGMENT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CLICKSEGMENT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotClickSegmentFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex1), float64(x1), float64(y1), int(sampleIndex2), float64(x2), float64(y2), int(button)))
}

func setPlotClickSegmentFunc(ih Ihandle, f PlotClickSegmentFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CLICKSEGMENT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotClickSegmentFunc(ih.ptr())
}

//--------------------

// PlotDrawSampleFunc for DRAWSAMPLE_CB callback in IupPlot.
type PlotDrawSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, state int) int

//export goIupPlotDrawSampleCB
func goIupPlotDrawSampleCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DRAWSAMPLE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DRAWSAMPLE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDrawSampleFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y), int(state)))
}

func setPlotDrawSampleFunc(ih Ihandle, f PlotDrawSampleFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DRAWSAMPLE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotDrawSampleFunc(ih.ptr())
}

//--------------------

// PlotMotionFunc for PLOTMOTION_CB callback in IupPlot.
type PlotMotionFunc func(ih Ihandle, x, y float64, status string) int

//export goIupPlotMotionCB
func goIupPlotMotionCB(ih unsafe.Pointer, x, y C.double, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PLOTMOTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PLOTMOTION_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotMotionFunc)
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), float64(x), float64(y), goStatus))
}

func setPlotMotionFunc(ih Ihandle, f PlotMotionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PLOTMOTION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotMotionFunc(ih.ptr())
}

//--------------------

// PlotButtonFunc for PLOTBUTTON_CB callback in IupPlot.
type PlotButtonFunc func(ih Ihandle, button, pressed int, x, y float64, status string) int

//export goIupPlotButtonCB
func goIupPlotButtonCB(ih unsafe.Pointer, button, pressed C.int, x, y C.double, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PLOTBUTTON_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PLOTBUTTON_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotButtonFunc)
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(button), int(pressed), float64(x), float64(y), goStatus))
}

func setPlotButtonFunc(ih Ihandle, f PlotButtonFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PLOTBUTTON_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotButtonFunc(ih.ptr())
}

//--------------------

// PlotEditSampleFunc for EDITSAMPLE_CB callback in IupPlot.
type PlotEditSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

//export goIupPlotEditSampleCB
func goIupPlotEditSampleCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITSAMPLE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITSAMPLE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotEditSampleFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y)))
}

func setPlotEditSampleFunc(ih Ihandle, f PlotEditSampleFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITSAMPLE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotEditSampleFunc(ih.ptr())
}

//--------------------

// PlotDeleteFunc for DELETE_CB callback in IupPlot.
type PlotDeleteFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

//export goIupPlotDeleteCB
func goIupPlotDeleteCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PLOT_DELETE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PLOT_DELETE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDeleteFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y)))
}

func setPlotDeleteFunc(ih Ihandle, f PlotDeleteFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PLOT_DELETE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotDeleteFunc(ih.ptr())
}

//--------------------

// PlotDeleteBeginFunc for DELETEBEGIN_CB callback in IupPlot.
type PlotDeleteBeginFunc func(ih Ihandle) int

//export goIupPlotDeleteBeginCB
func goIupPlotDeleteBeginCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DELETEBEGIN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DELETEBEGIN_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDeleteBeginFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotDeleteBeginFunc(ih Ihandle, f PlotDeleteBeginFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DELETEBEGIN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotDeleteBeginFunc(ih.ptr())
}

//--------------------

// PlotDeleteEndFunc for DELETEEND_CB callback in IupPlot.
type PlotDeleteEndFunc func(ih Ihandle) int

//export goIupPlotDeleteEndCB
func goIupPlotDeleteEndCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DELETEEND_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DELETEEND_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDeleteEndFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotDeleteEndFunc(ih Ihandle, f PlotDeleteEndFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DELETEEND_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotDeleteEndFunc(ih.ptr())
}

//--------------------

// PlotSelectFunc for SELECT_CB callback in IupPlot.
type PlotSelectFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, selected int) int

//export goIupPlotSelectCB
func goIupPlotSelectCB(ih unsafe.Pointer, dsIndex, sampleIndex C.int, x, y C.double, selected C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PLOT_SELECT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PLOT_SELECT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotSelectFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex), int(sampleIndex), float64(x), float64(y), int(selected)))
}

func setPlotSelectFunc(ih Ihandle, f PlotSelectFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PLOT_SELECT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotSelectFunc(ih.ptr())
}

//--------------------

// PlotSelectBeginFunc for SELECTBEGIN_CB callback in IupPlot.
type PlotSelectBeginFunc func(ih Ihandle) int

//export goIupPlotSelectBeginCB
func goIupPlotSelectBeginCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SELECTBEGIN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SELECTBEGIN_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotSelectBeginFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotSelectBeginFunc(ih Ihandle, f PlotSelectBeginFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SELECTBEGIN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotSelectBeginFunc(ih.ptr())
}

//--------------------

// PlotSelectEndFunc for SELECTEND_CB callback in IupPlot.
type PlotSelectEndFunc func(ih Ihandle) int

//export goIupPlotSelectEndCB
func goIupPlotSelectEndCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SELECTEND_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SELECTEND_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotSelectEndFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotSelectEndFunc(ih Ihandle, f PlotSelectEndFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SELECTEND_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotSelectEndFunc(ih.ptr())
}

//--------------------

// PlotMenuContextFunc for MENUCONTEXT_CB callback in IupPlot.
type PlotMenuContextFunc func(ih, menu Ihandle, x, y int) int

//export goIupPlotMenuContextCB
func goIupPlotMenuContextCB(ih, menu unsafe.Pointer, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PLOT_MENUCONTEXT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PLOT_MENUCONTEXT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotMenuContextFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(x), int(y)))
}

func setPlotMenuContextFunc(ih Ihandle, f PlotMenuContextFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PLOT_MENUCONTEXT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotMenuContextFunc(ih.ptr())
}

//--------------------

// PlotMenuContextCloseFunc for MENUCONTEXTCLOSE_CB callback in IupPlot.
type PlotMenuContextCloseFunc func(ih, menu Ihandle, x, y int) int

//export goIupPlotMenuContextCloseCB
func goIupPlotMenuContextCloseCB(ih, menu unsafe.Pointer, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PLOT_MENUCONTEXTCLOSE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PLOT_MENUCONTEXTCLOSE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotMenuContextCloseFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(x), int(y)))
}

func setPlotMenuContextCloseFunc(ih Ihandle, f PlotMenuContextCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PLOT_MENUCONTEXTCLOSE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotMenuContextCloseFunc(ih.ptr())
}

//--------------------

// PlotPropertiesChangedFunc for PROPERTIESCHANGED_CB callback in IupPlot.
type PlotPropertiesChangedFunc func(ih Ihandle) int

//export goIupPlotPropertiesChangedCB
func goIupPlotPropertiesChangedCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PROPERTIESCHANGED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PROPERTIESCHANGED_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotPropertiesChangedFunc)
	return C.int(f((Ihandle)(ih)))
}

func setPlotPropertiesChangedFunc(ih Ihandle, f PlotPropertiesChangedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PROPERTIESCHANGED_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotPropertiesChangedFunc(ih.ptr())
}

//--------------------

// PlotPropertiesValidateFunc for PROPERTIESVALIDATE_CB callback in IupPlot.
type PlotPropertiesValidateFunc func(ih Ihandle, name, value string) int

//export goIupPlotPropertiesValidateCB
func goIupPlotPropertiesValidateCB(ih unsafe.Pointer, name, value unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PROPERTIESVALIDATE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "PROPERTIESVALIDATE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotPropertiesValidateFunc)
	goName := C.GoString((*C.char)(name))
	goValue := C.GoString((*C.char)(value))
	return C.int(f((Ihandle)(ih), goName, goValue))
}

func setPlotPropertiesValidateFunc(ih Ihandle, f PlotPropertiesValidateFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PROPERTIESVALIDATE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotPropertiesValidateFunc(ih.ptr())
}

//--------------------

// PlotDSPropertiesChangedFunc for DSPROPERTIESCHANGED_CB callback in IupPlot.
type PlotDSPropertiesChangedFunc func(ih Ihandle, dsIndex int) int

//export goIupPlotDSPropertiesChangedCB
func goIupPlotDSPropertiesChangedCB(ih unsafe.Pointer, dsIndex C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DSPROPERTIESCHANGED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DSPROPERTIESCHANGED_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDSPropertiesChangedFunc)
	return C.int(f((Ihandle)(ih), int(dsIndex)))
}

func setPlotDSPropertiesChangedFunc(ih Ihandle, f PlotDSPropertiesChangedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DSPROPERTIESCHANGED_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotDSPropertiesChangedFunc(ih.ptr())
}

//--------------------

// PlotDSPropertiesValidateFunc for DSPROPERTIESVALIDATE_CB callback in IupPlot.
type PlotDSPropertiesValidateFunc func(ih, param1, param2 Ihandle, dsIndex int) int

//export goIupPlotDSPropertiesValidateCB
func goIupPlotDSPropertiesValidateCB(ih, param1, param2 unsafe.Pointer, dsIndex C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DSPROPERTIESVALIDATE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DSPROPERTIESVALIDATE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotDSPropertiesValidateFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(param1), (Ihandle)(param2), int(dsIndex)))
}

func setPlotDSPropertiesValidateFunc(ih Ihandle, f PlotDSPropertiesValidateFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DSPROPERTIESVALIDATE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotDSPropertiesValidateFunc(ih.ptr())
}

//--------------------

// PlotTickFormatNumberFunc for XTICKFORMATNUMBER_CB and YTICKFORMATNUMBER_CB callbacks in IupPlot.
type PlotTickFormatNumberFunc func(ih Ihandle, format, outStr string, value float64, status string) int

//export goIupPlotXTickFormatNumberCB
func goIupPlotXTickFormatNumberCB(ih unsafe.Pointer, format, outStr unsafe.Pointer, value C.double, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("XTICKFORMATNUMBER_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "XTICKFORMATNUMBER_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotTickFormatNumberFunc)
	goFormat := C.GoString((*C.char)(format))
	goOutStr := C.GoString((*C.char)(outStr))
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), goFormat, goOutStr, float64(value), goStatus))
}

func setPlotXTickFormatNumberFunc(ih Ihandle, f PlotTickFormatNumberFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("XTICKFORMATNUMBER_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotXTickFormatNumberFunc(ih.ptr())
}

//export goIupPlotYTickFormatNumberCB
func goIupPlotYTickFormatNumberCB(ih unsafe.Pointer, format, outStr unsafe.Pointer, value C.double, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("YTICKFORMATNUMBER_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "YTICKFORMATNUMBER_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(PlotTickFormatNumberFunc)
	goFormat := C.GoString((*C.char)(format))
	goOutStr := C.GoString((*C.char)(outStr))
	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), goFormat, goOutStr, float64(value), goStatus))
}

func setPlotYTickFormatNumberFunc(ih Ihandle, f PlotTickFormatNumberFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("YTICKFORMATNUMBER_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetPlotYTickFormatNumberFunc(ih.ptr())
}

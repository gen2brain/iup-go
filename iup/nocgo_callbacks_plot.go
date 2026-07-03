//go:build !cgo && !js

package iup

import "github.com/ebitengine/purego"

type PlotButtonFunc func(ih Ihandle, button, pressed int, x, y float64, status string) int

var plotButtonFuncCB = purego.NewCallback(func(ih uintptr, button int32, pressed int32, x float64, y float64, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PLOTBUTTON_CB").(PlotButtonFunc); ok {
		return f(Ihandle(ih), int(button), int(pressed), x, y, goString(status))
	}
	return 0
})

func setPlotButtonFunc(ih Ihandle, f PlotButtonFunc) {
	storeCallback(ih, "_IUPGO_PLOTBUTTON_CB", f)
	iupSetCallback(uintptr(ih), "PLOTBUTTON_CB", plotButtonFuncCB)
}

type PlotClickSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, button int) int

var plotClickSampleFuncCB = purego.NewCallback(func(ih uintptr, dsIndex int32, sampleIndex int32, x float64, y float64, button int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_CLICKSAMPLE_CB").(PlotClickSampleFunc); ok {
		return f(Ihandle(ih), int(dsIndex), int(sampleIndex), x, y, int(button))
	}
	return 0
})

func setPlotClickSampleFunc(ih Ihandle, f PlotClickSampleFunc) {
	storeCallback(ih, "_IUPGO_CLICKSAMPLE_CB", f)
	iupSetCallback(uintptr(ih), "CLICKSAMPLE_CB", plotClickSampleFuncCB)
}

type PlotClickSegmentFunc func(ih Ihandle, dsIndex, sampleIndex1 int, x1, y1 float64, sampleIndex2 int, x2, y2 float64, button int) int

var plotClickSegmentFuncCB = purego.NewCallback(func(ih uintptr, dsIndex int32, sampleIndex1 int32, x1 float64, y1 float64, sampleIndex2 int32, x2 float64, y2 float64, button int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_CLICKSEGMENT_CB").(PlotClickSegmentFunc); ok {
		return f(Ihandle(ih), int(dsIndex), int(sampleIndex1), x1, y1, int(sampleIndex2), x2, y2, int(button))
	}
	return 0
})

func setPlotClickSegmentFunc(ih Ihandle, f PlotClickSegmentFunc) {
	storeCallback(ih, "_IUPGO_CLICKSEGMENT_CB", f)
	iupSetCallback(uintptr(ih), "CLICKSEGMENT_CB", plotClickSegmentFuncCB)
}

type PlotDeleteBeginFunc func(ih Ihandle) int

var plotDeleteBeginFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DELETEBEGIN_CB").(PlotDeleteBeginFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setPlotDeleteBeginFunc(ih Ihandle, f PlotDeleteBeginFunc) {
	storeCallback(ih, "_IUPGO_DELETEBEGIN_CB", f)
	iupSetCallback(uintptr(ih), "DELETEBEGIN_CB", plotDeleteBeginFuncCB)
}

type PlotDeleteEndFunc func(ih Ihandle) int

var plotDeleteEndFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DELETEEND_CB").(PlotDeleteEndFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setPlotDeleteEndFunc(ih Ihandle, f PlotDeleteEndFunc) {
	storeCallback(ih, "_IUPGO_DELETEEND_CB", f)
	iupSetCallback(uintptr(ih), "DELETEEND_CB", plotDeleteEndFuncCB)
}

type PlotDeleteFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

var plotDeleteFuncCB = purego.NewCallback(func(ih uintptr, dsIndex int32, sampleIndex int32, x float64, y float64) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PLOT_DELETE_CB").(PlotDeleteFunc); ok {
		return f(Ihandle(ih), int(dsIndex), int(sampleIndex), x, y)
	}
	return 0
})

func setPlotDeleteFunc(ih Ihandle, f PlotDeleteFunc) {
	storeCallback(ih, "_IUPGO_PLOT_DELETE_CB", f)
	iupSetCallback(uintptr(ih), "DELETE_CB", plotDeleteFuncCB)
}

type PlotDrawSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, state int) int

var plotDrawSampleFuncCB = purego.NewCallback(func(ih uintptr, dsIndex int32, sampleIndex int32, x float64, y float64, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DRAWSAMPLE_CB").(PlotDrawSampleFunc); ok {
		return f(Ihandle(ih), int(dsIndex), int(sampleIndex), x, y, int(state))
	}
	return 0
})

func setPlotDrawSampleFunc(ih Ihandle, f PlotDrawSampleFunc) {
	storeCallback(ih, "_IUPGO_DRAWSAMPLE_CB", f)
	iupSetCallback(uintptr(ih), "DRAWSAMPLE_CB", plotDrawSampleFuncCB)
}

type PlotDSPropertiesChangedFunc func(ih Ihandle, dsIndex int) int

var plotDSPropertiesChangedFuncCB = purego.NewCallback(func(ih uintptr, dsIndex int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DSPROPERTIESCHANGED_CB").(PlotDSPropertiesChangedFunc); ok {
		return f(Ihandle(ih), int(dsIndex))
	}
	return 0
})

func setPlotDSPropertiesChangedFunc(ih Ihandle, f PlotDSPropertiesChangedFunc) {
	storeCallback(ih, "_IUPGO_DSPROPERTIESCHANGED_CB", f)
	iupSetCallback(uintptr(ih), "DSPROPERTIESCHANGED_CB", plotDSPropertiesChangedFuncCB)
}

type PlotDSPropertiesValidateFunc func(ih, param1, param2 Ihandle, dsIndex int) int

var plotDSPropertiesValidateFuncCB = purego.NewCallback(func(ih uintptr, param1 uintptr, param2 uintptr, dsIndex int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DSPROPERTIESVALIDATE_CB").(PlotDSPropertiesValidateFunc); ok {
		return f(Ihandle(ih), Ihandle(param1), Ihandle(param2), int(dsIndex))
	}
	return 0
})

func setPlotDSPropertiesValidateFunc(ih Ihandle, f PlotDSPropertiesValidateFunc) {
	storeCallback(ih, "_IUPGO_DSPROPERTIESVALIDATE_CB", f)
	iupSetCallback(uintptr(ih), "DSPROPERTIESVALIDATE_CB", plotDSPropertiesValidateFuncCB)
}

type PlotEditSampleFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64) int

var plotEditSampleFuncCB = purego.NewCallback(func(ih uintptr, dsIndex int32, sampleIndex int32, x float64, y float64) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITSAMPLE_CB").(PlotEditSampleFunc); ok {
		return f(Ihandle(ih), int(dsIndex), int(sampleIndex), x, y)
	}
	return 0
})

func setPlotEditSampleFunc(ih Ihandle, f PlotEditSampleFunc) {
	storeCallback(ih, "_IUPGO_EDITSAMPLE_CB", f)
	iupSetCallback(uintptr(ih), "EDITSAMPLE_CB", plotEditSampleFuncCB)
}

type PlotMenuContextCloseFunc func(ih, menu Ihandle, x, y int) int

var plotMenuContextCloseFuncCB = purego.NewCallback(func(ih uintptr, menu uintptr, x int32, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PLOT_MENUCONTEXTCLOSE_CB").(PlotMenuContextCloseFunc); ok {
		return f(Ihandle(ih), Ihandle(menu), int(x), int(y))
	}
	return 0
})

func setPlotMenuContextCloseFunc(ih Ihandle, f PlotMenuContextCloseFunc) {
	storeCallback(ih, "_IUPGO_PLOT_MENUCONTEXTCLOSE_CB", f)
	iupSetCallback(uintptr(ih), "MENUCONTEXTCLOSE_CB", plotMenuContextCloseFuncCB)
}

type PlotMenuContextFunc func(ih, menu Ihandle, x, y int) int

var plotMenuContextFuncCB = purego.NewCallback(func(ih uintptr, menu uintptr, x int32, y int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PLOT_MENUCONTEXT_CB").(PlotMenuContextFunc); ok {
		return f(Ihandle(ih), Ihandle(menu), int(x), int(y))
	}
	return 0
})

func setPlotMenuContextFunc(ih Ihandle, f PlotMenuContextFunc) {
	storeCallback(ih, "_IUPGO_PLOT_MENUCONTEXT_CB", f)
	iupSetCallback(uintptr(ih), "MENUCONTEXT_CB", plotMenuContextFuncCB)
}

type PlotMotionFunc func(ih Ihandle, x, y float64, status string) int

var plotMotionFuncCB = purego.NewCallback(func(ih uintptr, x float64, y float64, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PLOTMOTION_CB").(PlotMotionFunc); ok {
		return f(Ihandle(ih), x, y, goString(status))
	}
	return 0
})

func setPlotMotionFunc(ih Ihandle, f PlotMotionFunc) {
	storeCallback(ih, "_IUPGO_PLOTMOTION_CB", f)
	iupSetCallback(uintptr(ih), "PLOTMOTION_CB", plotMotionFuncCB)
}

type PlotPropertiesChangedFunc func(ih Ihandle) int

var plotPropertiesChangedFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PROPERTIESCHANGED_CB").(PlotPropertiesChangedFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setPlotPropertiesChangedFunc(ih Ihandle, f PlotPropertiesChangedFunc) {
	storeCallback(ih, "_IUPGO_PROPERTIESCHANGED_CB", f)
	iupSetCallback(uintptr(ih), "PROPERTIESCHANGED_CB", plotPropertiesChangedFuncCB)
}

type PlotPropertiesValidateFunc func(ih Ihandle, name, value string) int

var plotPropertiesValidateFuncCB = purego.NewCallback(func(ih uintptr, name uintptr, value uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PROPERTIESVALIDATE_CB").(PlotPropertiesValidateFunc); ok {
		return f(Ihandle(ih), goString(name), goString(value))
	}
	return 0
})

func setPlotPropertiesValidateFunc(ih Ihandle, f PlotPropertiesValidateFunc) {
	storeCallback(ih, "_IUPGO_PROPERTIESVALIDATE_CB", f)
	iupSetCallback(uintptr(ih), "PROPERTIESVALIDATE_CB", plotPropertiesValidateFuncCB)
}

type PlotSelectBeginFunc func(ih Ihandle) int

var plotSelectBeginFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SELECTBEGIN_CB").(PlotSelectBeginFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setPlotSelectBeginFunc(ih Ihandle, f PlotSelectBeginFunc) {
	storeCallback(ih, "_IUPGO_SELECTBEGIN_CB", f)
	iupSetCallback(uintptr(ih), "SELECTBEGIN_CB", plotSelectBeginFuncCB)
}

type PlotSelectEndFunc func(ih Ihandle) int

var plotSelectEndFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SELECTEND_CB").(PlotSelectEndFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setPlotSelectEndFunc(ih Ihandle, f PlotSelectEndFunc) {
	storeCallback(ih, "_IUPGO_SELECTEND_CB", f)
	iupSetCallback(uintptr(ih), "SELECTEND_CB", plotSelectEndFuncCB)
}

type PlotSelectFunc func(ih Ihandle, dsIndex, sampleIndex int, x, y float64, selected int) int

var plotSelectFuncCB = purego.NewCallback(func(ih uintptr, dsIndex int32, sampleIndex int32, x float64, y float64, selected int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PLOT_SELECT_CB").(PlotSelectFunc); ok {
		return f(Ihandle(ih), int(dsIndex), int(sampleIndex), x, y, int(selected))
	}
	return 0
})

func setPlotSelectFunc(ih Ihandle, f PlotSelectFunc) {
	storeCallback(ih, "_IUPGO_PLOT_SELECT_CB", f)
	iupSetCallback(uintptr(ih), "SELECT_CB", plotSelectFuncCB)
}

type PlotDrawFunc func(ih Ihandle) int

var plotPreDrawCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PREDRAW_CB").(PlotDrawFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

var plotPostDrawCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_POSTDRAW_CB").(PlotDrawFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setPlotPreDrawFunc(ih Ihandle, f PlotDrawFunc) {
	storeCallback(ih, "_IUPGO_PREDRAW_CB", f)
	iupSetCallback(uintptr(ih), "PREDRAW_CB", plotPreDrawCB)
}

func setPlotPostDrawFunc(ih Ihandle, f PlotDrawFunc) {
	storeCallback(ih, "_IUPGO_POSTDRAW_CB", f)
	iupSetCallback(uintptr(ih), "POSTDRAW_CB", plotPostDrawCB)
}

type PlotTickFormatNumberFunc func(ih Ihandle, format, outStr string, value float64, status string) int

var plotXTickFormatCB = purego.NewCallback(func(ih, format, outStr uintptr, value float64, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_XTICKFORMATNUMBER_CB").(PlotTickFormatNumberFunc); ok {
		return f(Ihandle(ih), goString(format), goString(outStr), value, goString(status))
	}
	return 0
})

var plotYTickFormatCB = purego.NewCallback(func(ih, format, outStr uintptr, value float64, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_YTICKFORMATNUMBER_CB").(PlotTickFormatNumberFunc); ok {
		return f(Ihandle(ih), goString(format), goString(outStr), value, goString(status))
	}
	return 0
})

func setPlotXTickFormatNumberFunc(ih Ihandle, f PlotTickFormatNumberFunc) {
	storeCallback(ih, "_IUPGO_XTICKFORMATNUMBER_CB", f)
	iupSetCallback(uintptr(ih), "XTICKFORMATNUMBER_CB", plotXTickFormatCB)
}

func setPlotYTickFormatNumberFunc(ih Ihandle, f PlotTickFormatNumberFunc) {
	storeCallback(ih, "_IUPGO_YTICKFORMATNUMBER_CB", f)
	iupSetCallback(uintptr(ih), "YTICKFORMATNUMBER_CB", plotYTickFormatCB)
}

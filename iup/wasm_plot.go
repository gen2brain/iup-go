//go:build js && wasm && plot

package iup

func Plot() Ihandle {
	return ccallHandle("IupPlot", nil, nil)
}

func PlotAdd(ih Ihandle, x, y float64) {
	ccall("IupPlotAdd", "", []interface{}{"number", "number", "number"}, []interface{}{int(ih), float64(x), float64(y)})
}

func PlotAddSamples(ih Ihandle, dsIndex int, x, y []float64) {
	if len(x) == 0 {
		return
	}
	px, py := wasmF64Array(x), wasmF64Array(y)
	defer wasmFree(px)
	defer wasmFree(py)
	ccall("IupPlotAddSamples", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, px, py, len(x)})
}

func PlotAddSegment(ih Ihandle, x, y float64) {
	ccall("IupPlotAddSegment", "", []interface{}{"number", "number", "number"}, []interface{}{int(ih), float64(x), float64(y)})
}

func PlotAddStr(ih Ihandle, x string, y float64) {
	ccall("IupPlotAddStr", "", []interface{}{"number", "string", "number"}, []interface{}{int(ih), x, float64(y)})
}

func PlotAddStrSamples(ih Ihandle, dsIndex int, x []string, y []float64) {
	if len(x) == 0 {
		return
	}
	px, free := wasmStrArray(x)
	defer free()
	py := wasmF64Array(y)
	defer wasmFree(py)
	ccall("IupPlotAddStrSamples", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, px, py, len(x)})
}

func PlotBegin(ih Ihandle, strXdata int) {
	ccall("IupPlotBegin", "", []interface{}{"number", "number"}, []interface{}{int(ih), strXdata})
}

func PlotEnd(ih Ihandle) int {
	return ccall("IupPlotEnd", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
}

func PlotFindSample(ih Ihandle, cnvX, cnvY float64) (dsIndex, sampleIndex int, found bool) {
	pd, ps := wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pd)
	defer wasmFree(ps)
	rc := ccall("IupPlotFindSample", "number", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), cnvX, cnvY, pd, ps}).Int()
	return wasmGetI32(pd), wasmGetI32(ps), rc != 0
}

func PlotFindSegment(ih Ihandle, cnvX, cnvY float64) (dsIndex, sampleIndex1, sampleIndex2 int, found bool) {
	pd, p1, p2 := wasmMalloc(4), wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pd)
	defer wasmFree(p1)
	defer wasmFree(p2)
	rc := ccall("IupPlotFindSegment", "number", []interface{}{"number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), cnvX, cnvY, pd, p1, p2}).Int()
	return wasmGetI32(pd), wasmGetI32(p1), wasmGetI32(p2), rc != 0
}

func PlotGetSample(ih Ihandle, dsIndex, sampleIndex int) (x, y float64) {
	px, py := wasmMalloc(8), wasmMalloc(8)
	defer wasmFree(px)
	defer wasmFree(py)
	ccall("IupPlotGetSample", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, px, py})
	return wasmGetF64(px), wasmGetF64(py)
}

func PlotGetSampleExtra(ih Ihandle, dsIndex, sampleIndex int) float64 {
	return ccall("IupPlotGetSampleExtra", "number", []interface{}{"number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex}).Float()
}

func PlotGetSampleSelection(ih Ihandle, dsIndex, sampleIndex int) int {
	return ccall("IupPlotGetSampleSelection", "number", []interface{}{"number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex}).Int()
}

func PlotGetSampleStr(ih Ihandle, dsIndex, sampleIndex int) (x string, y float64) {
	px, py := wasmMalloc(4), wasmMalloc(8)
	defer wasmFree(px)
	defer wasmFree(py)
	ccall("IupPlotGetSampleStr", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, px, py})
	return wasmGetStrAt(px), wasmGetF64(py)
}

func PlotInsert(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	ccall("IupPlotInsert", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, float64(x), float64(y)})
}

func PlotInsertSamples(ih Ihandle, dsIndex, sampleIndex int, x, y []float64) {
	if len(x) == 0 {
		return
	}
	px, py := wasmF64Array(x), wasmF64Array(y)
	defer wasmFree(px)
	defer wasmFree(py)
	ccall("IupPlotInsertSamples", "", []interface{}{"number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, px, py, len(x)})
}

func PlotInsertSegment(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	ccall("IupPlotInsertSegment", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, float64(x), float64(y)})
}

func PlotInsertStr(ih Ihandle, dsIndex, sampleIndex int, x string, y float64) {
	ccall("IupPlotInsertStr", "", []interface{}{"number", "number", "number", "string", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, x, float64(y)})
}

func PlotInsertStrSamples(ih Ihandle, dsIndex, sampleIndex int, x []string, y []float64) {
	if len(x) == 0 {
		return
	}
	px, free := wasmStrArray(x)
	defer free()
	py := wasmF64Array(y)
	defer wasmFree(py)
	ccall("IupPlotInsertStrSamples", "", []interface{}{"number", "number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, px, py, len(x)})
}

func PlotLoadData(ih Ihandle, filename string, strXdata int) int {
	return ccall("IupPlotLoadData", "number", []interface{}{"number", "string", "number"}, []interface{}{int(ih), filename, strXdata}).Int()
}

func PlotOpen() {
	ccall("IupPlotOpen", "", nil, nil)
}

func PlotSetSample(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	ccall("IupPlotSetSample", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, float64(x), float64(y)})
}

func PlotSetSampleExtra(ih Ihandle, dsIndex, sampleIndex int, extra float64) {
	ccall("IupPlotSetSampleExtra", "", []interface{}{"number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, float64(extra)})
}

func PlotSetSampleSelection(ih Ihandle, dsIndex, sampleIndex int, selected int) {
	ccall("IupPlotSetSampleSelection", "", []interface{}{"number", "number", "number", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, selected})
}

func PlotSetSampleStr(ih Ihandle, dsIndex, sampleIndex int, x string, y float64) {
	ccall("IupPlotSetSampleStr", "", []interface{}{"number", "number", "number", "string", "number"}, []interface{}{int(ih), dsIndex, sampleIndex, x, float64(y)})
}

func PlotTransform(ih Ihandle, x, y float64) (cnvX, cnvY float64) {
	px, py := wasmMalloc(8), wasmMalloc(8)
	defer wasmFree(px)
	defer wasmFree(py)
	ccall("IupPlotTransform", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), x, y, px, py})
	return wasmGetF64(px), wasmGetF64(py)
}

func PlotTransformTo(ih Ihandle, cnvX, cnvY float64) (x, y float64) {
	px, py := wasmMalloc(8), wasmMalloc(8)
	defer wasmFree(px)
	defer wasmFree(py)
	ccall("IupPlotTransformTo", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), cnvX, cnvY, px, py})
	return wasmGetF64(px), wasmGetF64(py)
}

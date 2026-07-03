//go:build !cgo && !js && plot

package iup

import (
	"runtime"
	"unsafe"

	"github.com/ebitengine/purego"
)

var (
	iupPlotOpen               func()
	iupPlot                   func() uintptr
	iupPlotBegin              func(ih uintptr, strXdata int32)
	iupPlotAdd                func(ih uintptr, x, y float64)
	iupPlotAddStr             func(ih uintptr, x string, y float64)
	iupPlotAddSegment         func(ih uintptr, x, y float64)
	iupPlotEnd                func(ih uintptr) int32
	iupPlotLoadData           func(ih uintptr, filename string, strXdata int32) int32
	iupPlotInsert             func(ih uintptr, ds, sample int32, x, y float64)
	iupPlotInsertStr          func(ih uintptr, ds, sample int32, x string, y float64)
	iupPlotInsertSegment      func(ih uintptr, ds, sample int32, x, y float64)
	iupPlotInsertSamples      func(ih uintptr, ds, sample int32, x, y []float64, count int32)
	iupPlotInsertStrSamples   func(ih uintptr, ds, sample int32, x *uintptr, y []float64, count int32)
	iupPlotAddSamples         func(ih uintptr, ds int32, x, y []float64, count int32)
	iupPlotAddStrSamples      func(ih uintptr, ds int32, x *uintptr, y []float64, count int32)
	iupPlotGetSample          func(ih uintptr, ds, sample int32, x, y *float64)
	iupPlotGetSampleStr       func(ih uintptr, ds, sample int32, x *uintptr, y *float64)
	iupPlotGetSampleSelection func(ih uintptr, ds, sample int32) int32
	iupPlotGetSampleExtra     func(ih uintptr, ds, sample int32) float64
	iupPlotSetSample          func(ih uintptr, ds, sample int32, x, y float64)
	iupPlotSetSampleStr       func(ih uintptr, ds, sample int32, x string, y float64)
	iupPlotSetSampleSelection func(ih uintptr, ds, sample, selected int32)
	iupPlotSetSampleExtra     func(ih uintptr, ds, sample int32, extra float64)
	iupPlotTransform          func(ih uintptr, x, y float64, cnvX, cnvY *float64)
	iupPlotTransformTo        func(ih uintptr, cnvX, cnvY float64, x, y *float64)
	iupPlotFindSample         func(ih uintptr, cnvX, cnvY float64, ds, sample *int32) int32
	iupPlotFindSegment        func(ih uintptr, cnvX, cnvY float64, ds, sample1, sample2 *int32) int32
)

func init() {
	ensureBase()
	lib := openLib("iupplot")
	if lib == 0 {
		panic("iup: cannot load libiupplot")
	}
	reg := func(fptr any, name string) { purego.RegisterLibFunc(fptr, lib, name) }
	reg(&iupPlotOpen, "IupPlotOpen")
	reg(&iupPlot, "IupPlot")
	reg(&iupPlotBegin, "IupPlotBegin")
	reg(&iupPlotAdd, "IupPlotAdd")
	reg(&iupPlotAddStr, "IupPlotAddStr")
	reg(&iupPlotAddSegment, "IupPlotAddSegment")
	reg(&iupPlotEnd, "IupPlotEnd")
	reg(&iupPlotLoadData, "IupPlotLoadData")
	reg(&iupPlotInsert, "IupPlotInsert")
	reg(&iupPlotInsertStr, "IupPlotInsertStr")
	reg(&iupPlotInsertSegment, "IupPlotInsertSegment")
	reg(&iupPlotInsertSamples, "IupPlotInsertSamples")
	reg(&iupPlotInsertStrSamples, "IupPlotInsertStrSamples")
	reg(&iupPlotAddSamples, "IupPlotAddSamples")
	reg(&iupPlotAddStrSamples, "IupPlotAddStrSamples")
	reg(&iupPlotGetSample, "IupPlotGetSample")
	reg(&iupPlotGetSampleStr, "IupPlotGetSampleStr")
	reg(&iupPlotGetSampleSelection, "IupPlotGetSampleSelection")
	reg(&iupPlotGetSampleExtra, "IupPlotGetSampleExtra")
	reg(&iupPlotSetSample, "IupPlotSetSample")
	reg(&iupPlotSetSampleStr, "IupPlotSetSampleStr")
	reg(&iupPlotSetSampleSelection, "IupPlotSetSampleSelection")
	reg(&iupPlotSetSampleExtra, "IupPlotSetSampleExtra")
	reg(&iupPlotTransform, "IupPlotTransform")
	reg(&iupPlotTransformTo, "IupPlotTransformTo")
	reg(&iupPlotFindSample, "IupPlotFindSample")
	reg(&iupPlotFindSegment, "IupPlotFindSegment")
}

// cStrArray builds a char* array (as []uintptr into Go buffers) plus the keep
// slice that must stay referenced across the call.
func cStrArray(ss []string) (arr []uintptr, keep [][]byte) {
	arr = make([]uintptr, len(ss))
	keep = make([][]byte, len(ss))
	for i, s := range ss {
		keep[i] = append([]byte(s), 0)
		arr[i] = uintptr(unsafe.Pointer(&keep[i][0]))
	}
	return
}

func PlotOpen() { iupPlotOpen() }

func Plot() Ihandle { return mkih(iupPlot()) }

func PlotBegin(ih Ihandle, strXdata int) { iupPlotBegin(uintptr(ih), int32(strXdata)) }

func PlotAdd(ih Ihandle, x, y float64) { iupPlotAdd(uintptr(ih), x, y) }

func PlotAddStr(ih Ihandle, x string, y float64) { iupPlotAddStr(uintptr(ih), x, y) }

func PlotAddSegment(ih Ihandle, x, y float64) { iupPlotAddSegment(uintptr(ih), x, y) }

func PlotEnd(ih Ihandle) int { return int(iupPlotEnd(uintptr(ih))) }

func PlotLoadData(ih Ihandle, filename string, strXdata int) int {
	return int(iupPlotLoadData(uintptr(ih), filename, int32(strXdata)))
}

func PlotInsert(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	iupPlotInsert(uintptr(ih), int32(dsIndex), int32(sampleIndex), x, y)
}

func PlotInsertStr(ih Ihandle, dsIndex, sampleIndex int, x string, y float64) {
	iupPlotInsertStr(uintptr(ih), int32(dsIndex), int32(sampleIndex), x, y)
}

func PlotInsertSegment(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	iupPlotInsertSegment(uintptr(ih), int32(dsIndex), int32(sampleIndex), x, y)
}

func PlotInsertSamples(ih Ihandle, dsIndex, sampleIndex int, x, y []float64) {
	iupPlotInsertSamples(uintptr(ih), int32(dsIndex), int32(sampleIndex), x, y, int32(len(x)))
}

func PlotInsertStrSamples(ih Ihandle, dsIndex, sampleIndex int, x []string, y []float64) {
	arr, keep := cStrArray(x)
	iupPlotInsertStrSamples(uintptr(ih), int32(dsIndex), int32(sampleIndex), &arr[0], y, int32(len(x)))
	runtime.KeepAlive(keep)
}

func PlotAddSamples(ih Ihandle, dsIndex int, x, y []float64) {
	iupPlotAddSamples(uintptr(ih), int32(dsIndex), x, y, int32(len(x)))
}

func PlotAddStrSamples(ih Ihandle, dsIndex int, x []string, y []float64) {
	arr, keep := cStrArray(x)
	iupPlotAddStrSamples(uintptr(ih), int32(dsIndex), &arr[0], y, int32(len(x)))
	runtime.KeepAlive(keep)
}

func PlotGetSample(ih Ihandle, dsIndex, sampleIndex int) (x, y float64) {
	iupPlotGetSample(uintptr(ih), int32(dsIndex), int32(sampleIndex), &x, &y)
	return
}

func PlotGetSampleStr(ih Ihandle, dsIndex, sampleIndex int) (x string, y float64) {
	var xp uintptr
	iupPlotGetSampleStr(uintptr(ih), int32(dsIndex), int32(sampleIndex), &xp, &y)
	return goString(xp), y
}

func PlotGetSampleSelection(ih Ihandle, dsIndex, sampleIndex int) int {
	return int(iupPlotGetSampleSelection(uintptr(ih), int32(dsIndex), int32(sampleIndex)))
}

func PlotGetSampleExtra(ih Ihandle, dsIndex, sampleIndex int) float64 {
	return iupPlotGetSampleExtra(uintptr(ih), int32(dsIndex), int32(sampleIndex))
}

func PlotSetSample(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	iupPlotSetSample(uintptr(ih), int32(dsIndex), int32(sampleIndex), x, y)
}

func PlotSetSampleStr(ih Ihandle, dsIndex, sampleIndex int, x string, y float64) {
	iupPlotSetSampleStr(uintptr(ih), int32(dsIndex), int32(sampleIndex), x, y)
}

func PlotSetSampleSelection(ih Ihandle, dsIndex, sampleIndex int, selected int) {
	iupPlotSetSampleSelection(uintptr(ih), int32(dsIndex), int32(sampleIndex), int32(selected))
}

func PlotSetSampleExtra(ih Ihandle, dsIndex, sampleIndex int, extra float64) {
	iupPlotSetSampleExtra(uintptr(ih), int32(dsIndex), int32(sampleIndex), extra)
}

func PlotTransform(ih Ihandle, x, y float64) (cnvX, cnvY float64) {
	iupPlotTransform(uintptr(ih), x, y, &cnvX, &cnvY)
	return
}

func PlotTransformTo(ih Ihandle, cnvX, cnvY float64) (x, y float64) {
	iupPlotTransformTo(uintptr(ih), cnvX, cnvY, &x, &y)
	return
}

func PlotFindSample(ih Ihandle, cnvX, cnvY float64) (dsIndex, sampleIndex int, found bool) {
	var ds, s int32
	ret := iupPlotFindSample(uintptr(ih), cnvX, cnvY, &ds, &s)
	return int(ds), int(s), ret != 0
}

func PlotFindSegment(ih Ihandle, cnvX, cnvY float64) (dsIndex, sampleIndex1, sampleIndex2 int, found bool) {
	var ds, s1, s2 int32
	ret := iupPlotFindSegment(uintptr(ih), cnvX, cnvY, &ds, &s1, &s2)
	return int(ds), int(s1), int(s2), ret != 0
}

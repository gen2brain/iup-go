//go:build plot

package iup

import (
	"unsafe"

	"github.com/google/uuid"
)

/*
#include <stdlib.h>
#include "iup.h"
#include "iup_plot.h"
*/
import "C"

// PlotOpen initializes the IupPlot widget class.
// Must be called after IupOpen.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotOpen() {
	C.IupPlotOpen()
}

// Plot creates a plot widget instance.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func Plot() Ihandle {
	h := mkih(C.IupPlot())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// PlotBegin prepares a dataset for receiving samples.
// If strXdata is non-zero, the X axis data will be strings.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotBegin(ih Ihandle, strXdata int) {
	C.IupPlotBegin(ih.ptr(), C.int(strXdata))
}

// PlotAdd adds a sample to the current dataset with numeric X data.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotAdd(ih Ihandle, x, y float64) {
	C.IupPlotAdd(ih.ptr(), C.double(x), C.double(y))
}

// PlotAddStr adds a sample to the current dataset with string X data.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotAddStr(ih Ihandle, x string, y float64) {
	cX := C.CString(x)
	defer C.free(unsafe.Pointer(cX))

	C.IupPlotAddStr(ih.ptr(), cX, C.double(y))
}

// PlotAddSegment adds a sample that starts a new segment in the current dataset.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotAddSegment(ih Ihandle, x, y float64) {
	C.IupPlotAddSegment(ih.ptr(), C.double(x), C.double(y))
}

// PlotEnd ends the current dataset definition and returns its index.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotEnd(ih Ihandle) int {
	return int(C.IupPlotEnd(ih.ptr()))
}

// PlotLoadData loads data from a file for the current dataset.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotLoadData(ih Ihandle, filename string, strXdata int) int {
	cFilename := C.CString(filename)
	defer C.free(unsafe.Pointer(cFilename))

	return int(C.IupPlotLoadData(ih.ptr(), cFilename, C.int(strXdata)))
}

// PlotInsert inserts a sample at the given position in a dataset.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotInsert(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	C.IupPlotInsert(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), C.double(x), C.double(y))
}

// PlotInsertStr inserts a string X sample at the given position in a dataset.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotInsertStr(ih Ihandle, dsIndex, sampleIndex int, x string, y float64) {
	cX := C.CString(x)
	defer C.free(unsafe.Pointer(cX))

	C.IupPlotInsertStr(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), cX, C.double(y))
}

// PlotInsertSegment inserts a segment-starting sample at the given position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotInsertSegment(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	C.IupPlotInsertSegment(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), C.double(x), C.double(y))
}

// PlotInsertSamples inserts multiple numeric samples at the given position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotInsertSamples(ih Ihandle, dsIndex, sampleIndex int, x, y []float64) {
	count := len(x)
	if count == 0 {
		return
	}
	C.IupPlotInsertSamples(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), (*C.double)(&x[0]), (*C.double)(&y[0]), C.int(count))
}

// PlotInsertStrSamples inserts multiple string X samples at the given position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotInsertStrSamples(ih Ihandle, dsIndex, sampleIndex int, x []string, y []float64) {
	count := len(x)
	if count == 0 {
		return
	}
	cStrs := make([]*C.char, count)
	for i, s := range x {
		cStrs[i] = C.CString(s)
	}
	defer func() {
		for _, cs := range cStrs {
			C.free(unsafe.Pointer(cs))
		}
	}()

	C.IupPlotInsertStrSamples(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), &cStrs[0], (*C.double)(&y[0]), C.int(count))
}

// PlotAddSamples adds multiple numeric samples to a dataset.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotAddSamples(ih Ihandle, dsIndex int, x, y []float64) {
	count := len(x)
	if count == 0 {
		return
	}
	C.IupPlotAddSamples(ih.ptr(), C.int(dsIndex), (*C.double)(&x[0]), (*C.double)(&y[0]), C.int(count))
}

// PlotAddStrSamples adds multiple string X samples to a dataset.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotAddStrSamples(ih Ihandle, dsIndex int, x []string, y []float64) {
	count := len(x)
	if count == 0 {
		return
	}
	cStrs := make([]*C.char, count)
	for i, s := range x {
		cStrs[i] = C.CString(s)
	}
	defer func() {
		for _, cs := range cStrs {
			C.free(unsafe.Pointer(cs))
		}
	}()

	C.IupPlotAddStrSamples(ih.ptr(), C.int(dsIndex), &cStrs[0], (*C.double)(&y[0]), C.int(count))
}

// PlotGetSample returns the X and Y values of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotGetSample(ih Ihandle, dsIndex, sampleIndex int) (x, y float64) {
	var cx, cy C.double
	C.IupPlotGetSample(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), &cx, &cy)
	return float64(cx), float64(cy)
}

// PlotGetSampleStr returns the string X and numeric Y values of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotGetSampleStr(ih Ihandle, dsIndex, sampleIndex int) (x string, y float64) {
	var cx *C.char
	var cy C.double
	C.IupPlotGetSampleStr(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), &cx, &cy)
	return C.GoString(cx), float64(cy)
}

// PlotGetSampleSelection returns the selection state of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotGetSampleSelection(ih Ihandle, dsIndex, sampleIndex int) int {
	return int(C.IupPlotGetSampleSelection(ih.ptr(), C.int(dsIndex), C.int(sampleIndex)))
}

// PlotGetSampleExtra returns the extra value of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotGetSampleExtra(ih Ihandle, dsIndex, sampleIndex int) float64 {
	return float64(C.IupPlotGetSampleExtra(ih.ptr(), C.int(dsIndex), C.int(sampleIndex)))
}

// PlotSetSample sets the X and Y values of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotSetSample(ih Ihandle, dsIndex, sampleIndex int, x, y float64) {
	C.IupPlotSetSample(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), C.double(x), C.double(y))
}

// PlotSetSampleStr sets the string X and numeric Y values of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotSetSampleStr(ih Ihandle, dsIndex, sampleIndex int, x string, y float64) {
	cX := C.CString(x)
	defer C.free(unsafe.Pointer(cX))

	C.IupPlotSetSampleStr(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), cX, C.double(y))
}

// PlotSetSampleSelection sets the selection state of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotSetSampleSelection(ih Ihandle, dsIndex, sampleIndex int, selected int) {
	C.IupPlotSetSampleSelection(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), C.int(selected))
}

// PlotSetSampleExtra sets the extra value of a sample.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotSetSampleExtra(ih Ihandle, dsIndex, sampleIndex int, extra float64) {
	C.IupPlotSetSampleExtra(ih.ptr(), C.int(dsIndex), C.int(sampleIndex), C.double(extra))
}

// PlotTransform converts plot coordinates to canvas (pixel) coordinates.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotTransform(ih Ihandle, x, y float64) (cnvX, cnvY float64) {
	var cx, cy C.double
	C.IupPlotTransform(ih.ptr(), C.double(x), C.double(y), &cx, &cy)
	return float64(cx), float64(cy)
}

// PlotTransformTo converts canvas (pixel) coordinates to plot coordinates.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotTransformTo(ih Ihandle, cnvX, cnvY float64) (x, y float64) {
	var cx, cy C.double
	C.IupPlotTransformTo(ih.ptr(), C.double(cnvX), C.double(cnvY), &cx, &cy)
	return float64(cx), float64(cy)
}

// PlotFindSample finds the sample closest to the given canvas coordinates.
// Returns the dataset index, sample index, and whether a sample was found.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotFindSample(ih Ihandle, cnvX, cnvY float64) (dsIndex, sampleIndex int, found bool) {
	var cds, csample C.int
	ret := C.IupPlotFindSample(ih.ptr(), C.double(cnvX), C.double(cnvY), &cds, &csample)
	return int(cds), int(csample), ret != 0
}

// PlotFindSegment finds the segment closest to the given canvas coordinates.
// Returns the dataset index, segment start and end sample indices, and whether a segment was found.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_plot.md
func PlotFindSegment(ih Ihandle, cnvX, cnvY float64) (dsIndex, sampleIndex1, sampleIndex2 int, found bool) {
	var cds, csample1, csample2 C.int
	ret := C.IupPlotFindSegment(ih.ptr(), C.double(cnvX), C.double(cnvY), &cds, &csample1, &csample2)
	return int(cds), int(csample1), int(csample2), ret != 0
}

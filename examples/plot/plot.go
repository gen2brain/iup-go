//go:build plot

package main

import (
	"fmt"
	"math"
	"math/rand"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

const maxPlot = 10

var (
	plots [maxPlot]iup.Ihandle
	tabs  iup.Ihandle
	tgg1  iup.Ihandle // Y Autoscale
	tgg2  iup.Ihandle // X Autoscale
	tgg3  iup.Ihandle // Vertical Grid
	tgg4  iup.Ihandle // Horizontal Grid
	tgg5  iup.Ihandle // Legend
)

func tabsGetIndex() int {
	currTab := iup.GetHandle(iup.GetAttribute(tabs, "VALUE"))
	ss := iup.GetAttribute(currTab, "TABTITLE")
	ss = strings.TrimPrefix(ss, "Plot ")
	ii, _ := strconv.Atoi(ss)
	return ii
}

func initPlots() {
	var x, y float64
	mult := 1.0

	// PLOT 0 - AutoScale
	plots[0].SetAttribute("TITLE", "AutoScale")
	plots[0].SetAttribute("FONT", "Helvetica, 10")
	plots[0].SetAttribute("LEGENDSHOW", "YES")
	plots[0].SetAttribute("AXS_XLABEL", "gnu (Foo)")
	plots[0].SetAttribute("AXS_YLABEL", "Space (m^3)")
	plots[0].SetAttribute("AXS_XCROSSORIGIN", "YES")
	plots[0].SetAttribute("AXS_YCROSSORIGIN", "YES")
	plots[0].SetAttribute("AXS_XLABELCENTERED", "NO")
	plots[0].SetAttribute("AXS_YLABELCENTERED", "NO")
	plots[0].SetAttribute("HIGHLIGHTMODE", "CURVE")

	theFac := 1.0 / (100 * 100 * 100)
	iup.PlotBegin(plots[0], 0)
	for i := -100; i <= 100; i++ {
		x = float64(i + 50)
		y = mult * theFac * float64(i*i*i)
		iup.PlotAdd(plots[0], x, y)
	}
	iup.PlotEnd(plots[0])
	plots[0].SetAttribute("DS_LINEWIDTH", "3")
	plots[0].SetAttribute("DS_LEGEND", "Line")
	plots[0].SetAttribute("DS_MODE", "LINE")
	plots[0].SetAttribute("DS_SELECTED", "YES")

	theFac = 2.0 / 100
	iup.PlotBegin(plots[0], 0)
	for i := -100; i < 0; i++ {
		x = float64(i)
		y = -mult * theFac * float64(i)
		iup.PlotAdd(plots[0], x, y)
	}

	index := iup.PlotEnd(plots[0])
	px := make([]float64, 101)
	py := make([]float64, 101)
	for i := 0; i <= 100; i++ {
		px[i] = float64(i)
		py[i] = -mult * theFac * float64(i)
	}
	iup.PlotInsertSamples(plots[0], index, 100, px, py)

	plots[0].SetAttribute("DS_LEGEND", "Curve 1")

	iup.PlotBegin(plots[0], 0)
	for i := -100; i <= 100; i++ {
		x = 0.01*float64(i*i) - 30
		y = 0.01 * mult * float64(i)
		iup.PlotAdd(plots[0], x, y)
	}
	iup.PlotEnd(plots[0])
	plots[0].SetAttribute("DS_LEGEND", "Curve 2")
	plots[0].SetAttribute("DS_ORDEREDX", "NO")

	// PLOT 1 - No Autoscale+No CrossOrigin
	plots[1].SetAttribute("TITLE", "No Autoscale+No CrossOrigin")
	plots[1].SetAttribute("FONT", "Helvetica, 10")
	plots[1].SetAttribute("BGCOLOR", "0 192 192")
	plots[1].SetAttribute("AXS_XLABEL", "Tg (X)")
	plots[1].SetAttribute("AXS_YLABEL", "Tg (Y)")
	plots[1].SetAttribute("AXS_XAUTOMIN", "NO")
	plots[1].SetAttribute("AXS_XAUTOMAX", "NO")
	plots[1].SetAttribute("AXS_YAUTOMIN", "NO")
	plots[1].SetAttribute("AXS_YAUTOMAX", "NO")
	plots[1].SetAttribute("AXS_XMIN", "10")
	plots[1].SetAttribute("AXS_XMAX", "60")
	plots[1].SetAttribute("AXS_YMIN", "-0.5")
	plots[1].SetAttribute("AXS_YMAX", "0.5")
	plots[1].SetAttribute("AXS_XFONTSTYLE", "ITALIC")
	plots[1].SetAttribute("AXS_YFONTSTYLE", "BOLD")
	plots[1].SetAttribute("AXS_XREVERSE", "YES")
	plots[1].SetAttribute("GRIDCOLOR", "128 255 128")
	plots[1].SetAttribute("GRIDLINESTYLE", "DOTTED")
	plots[1].SetAttribute("GRID", "YES")
	plots[1].SetAttribute("LEGENDSHOW", "YES")
	plots[1].SetAttribute("AXS_XLABELCENTERED", "YES")
	plots[1].SetAttribute("AXS_YLABELCENTERED", "YES")
	plots[1].SetAttribute("GRAPHICSMODE", "IMAGERGB")
	plots[1].SetAttribute("HIGHLIGHTMODE", "CURVE")

	theFac = 1.0 / (100 * 100 * 100)
	iup.PlotBegin(plots[1], 0)
	for i := 0; i <= 100; i++ {
		x = float64(i)
		y = theFac * float64(i*i*i)
		iup.PlotAdd(plots[1], x, y)
	}
	iup.PlotEnd(plots[1])
	plots[1].SetAttribute("DS_ORDEREDX", "NO")

	theFac = 2.0 / 100
	iup.PlotBegin(plots[1], 0)
	for i := 0; i <= 100; i++ {
		x = float64(i)
		y = -theFac * float64(i)
		iup.PlotAdd(plots[1], x, y)
	}
	iup.PlotEnd(plots[1])
	plots[1].SetAttribute("DS_ORDEREDX", "NO")

	// PLOT 2 - Log Scale
	plots[2].SetAttribute("TITLE", "Log Scale")
	plots[2].SetAttribute("GRID", "YES")
	plots[2].SetAttribute("AXS_XSCALE", "LOG10")
	plots[2].SetAttribute("AXS_YSCALE", "LOG2")
	plots[2].SetAttribute("AXS_XLABEL", "Tg (X)")
	plots[2].SetAttribute("AXS_YLABEL", "Tg (Y)")
	plots[2].SetAttribute("AXS_XFONTSTYLE", "BOLD")
	plots[2].SetAttribute("AXS_YFONTSTYLE", "BOLD")
	plots[2].SetAttribute("HIGHLIGHTMODE", "CURVE")

	theFac = 100.0 / (100 * 100 * 100)
	iup.PlotBegin(plots[2], 0)
	for i := 0; i <= 100; i++ {
		x = 0.0001 + float64(i)*0.001
		y = 0.01 + theFac*float64(i*i*i)
		iup.PlotAdd(plots[2], x, y)
	}
	iup.PlotEnd(plots[2])
	plots[2].SetAttribute("DS_COLOR", "100 100 200")
	plots[2].SetAttribute("DS_LINESTYLE", "DOTTED")

	// PLOT 3 - Bar Mode
	plots[3].SetAttribute("TITLE", "Bar Mode")
	plots[3].SetAttribute("HIGHLIGHTMODE", "SAMPLE")

	labels := []string{"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"}
	barData := []float64{10, -20, 30, 40, 50, 60, 70, 80, 90, 0, 10, 20}
	iup.PlotBegin(plots[3], 1)
	for i := 0; i < 12; i++ {
		iup.PlotAddStr(plots[3], labels[i], barData[i])
	}
	iup.PlotEnd(plots[3])
	plots[3].SetAttribute("DS_COLOR", "180 180 250")
	plots[3].SetAttribute("DS_BAROUTLINE", "YES")
	plots[3].SetAttribute("DS_BAROUTLINECOLOR", "70 70 160")
	plots[3].SetAttribute("DS_BARSPACING", "0")
	plots[3].SetAttribute("DS_MODE", "BAR")

	// PLOT 4 - Marks Mode
	plots[4].SetAttribute("TITLE", "Marks Mode")
	plots[4].SetAttribute("AXS_XAUTOMIN", "NO")
	plots[4].SetAttribute("AXS_XAUTOMAX", "NO")
	plots[4].SetAttribute("AXS_YAUTOMIN", "NO")
	plots[4].SetAttribute("AXS_YAUTOMAX", "NO")
	plots[4].SetAttribute("AXS_XMIN", "0")
	plots[4].SetAttribute("AXS_XMAX", "0.011")
	plots[4].SetAttribute("AXS_YMIN", "0")
	plots[4].SetAttribute("AXS_YMAX", "0.22")
	plots[4].SetAttribute("AXS_XTICKFORMAT", "%1.3f")
	plots[4].SetAttribute("LEGENDSHOW", "YES")
	plots[4].SetAttribute("LEGENDPOS", "BOTTOMRIGHT")

	theFac = 100.0 / (100 * 100 * 100)
	iup.PlotBegin(plots[4], 0)
	for i := 0; i <= 10; i++ {
		x = 0.0001 + float64(i)*0.001
		y = 0.01 + theFac*float64(i*i)
		iup.PlotAdd(plots[4], x, y)
	}
	iup.PlotEnd(plots[4])
	plots[4].SetAttribute("DS_MODE", "MARKLINE")
	iup.SetCallback(plots[4], "CLICKSEGMENT_CB", iup.PlotClickSegmentFunc(func(ih iup.Ihandle, ds, sample1 int, rx1, ry1 float64, sample2 int, rx2, ry2 float64, button int) int {
		fmt.Printf("CLICKSEGMENT_CB(ds=%d, sample1=%d, rx1=%.4f, ry1=%.4f, sample2=%d, rx2=%.4f, ry2=%.4f, button=%d)\n", ds, sample1, rx1, ry1, sample2, rx2, ry2, button)
		return iup.DEFAULT
	}))
	iup.SetCallback(plots[4], "CLICKSAMPLE_CB", iup.PlotClickSampleFunc(func(ih iup.Ihandle, ds, sample int, rx, ry float64, button int) int {
		fmt.Printf("CLICKSAMPLE_CB(ds=%d, sample=%d, rx=%.4f, ry=%.4f, button=%d)\n", ds, sample, rx, ry, button)
		return iup.DEFAULT
	}))

	iup.PlotBegin(plots[4], 0)
	for i := 0; i <= 10; i++ {
		x = 0.0001 + float64(i)*0.001
		y = 0.2 - theFac*float64(i*i)
		iup.PlotAdd(plots[4], x, y)
	}
	iup.PlotEnd(plots[4])
	plots[4].SetAttribute("DS_MODE", "MARK")
	plots[4].SetAttribute("DS_MARKSTYLE", "HOLLOW_CIRCLE")
	plots[4].SetAttribute("HIGHLIGHTMODE", "BOTH")

	// PLOT 5 - Data Selection and Editing
	plots[5].SetAttribute("TITLE", "Data Selection and Editing")

	theFac = 100.0 / (100 * 100 * 100)
	iup.PlotBegin(plots[5], 0)
	for i := -10; i <= 10; i++ {
		x = 0.001 * float64(i)
		y = 0.01 + theFac*float64(i*i*i)
		iup.PlotAdd(plots[5], x, y)
	}
	iup.PlotEnd(plots[5])

	plots[5].SetAttribute("AXS_XCROSSORIGIN", "YES")
	plots[5].SetAttribute("AXS_YCROSSORIGIN", "YES")
	plots[5].SetAttribute("DS_COLOR", "100 100 200")
	plots[5].SetAttribute("READONLY", "NO")

	iup.SetCallback(plots[5], "DELETE_CB", iup.PlotDeleteFunc(func(ih iup.Ihandle, dsIndex, sampleIndex int, x, y float64) int {
		fmt.Printf("DELETE_CB(%d, %d, %g, %g)\n", dsIndex, sampleIndex, x, y)
		return iup.DEFAULT
	}))
	iup.SetCallback(plots[5], "SELECT_CB", iup.PlotSelectFunc(func(ih iup.Ihandle, dsIndex, sampleIndex int, x, y float64, selected int) int {
		fmt.Printf("SELECT_CB(%d, %d, %g, %g, %d)\n", dsIndex, sampleIndex, x, y, selected)
		return iup.DEFAULT
	}))
	iup.SetCallback(plots[5], "POSTDRAW_CB", iup.PlotDrawFunc(func(ih iup.Ihandle) int {
		ix, iy := iup.PlotTransform(ih, 0.003, 0.02)
		fmt.Printf("POSTDRAW_CB() - PlotTransform(0.003, 0.02) = (%.2f, %.2f)\n", ix, iy)
		return iup.DEFAULT
	}))
	iup.SetCallback(plots[5], "PREDRAW_CB", iup.PlotDrawFunc(func(ih iup.Ihandle) int {
		fmt.Printf("PREDRAW_CB()\n")
		return iup.DEFAULT
	}))

	// PLOT 6 - Step Merge
	plots[6].SetAttribute("PLOT_COUNT", "2")
	plots[6].SetAttribute("PLOT_NUMCOL", "2")
	plots[6].SetAttribute("MERGEVIEW", "YES")
	plots[6].SetAttribute("SYNCVIEW", "YES")

	plots[6].SetAttribute("PLOT_CURRENT", "0")

	iup.PlotBegin(plots[6], 0)
	for i := 0; i <= 360; i += 10 {
		x = float64(i)
		y = math.Sin(x * math.Pi / 180.0)
		iup.PlotAdd(plots[6], x, y)
	}
	iup.PlotEnd(plots[6])
	plots[6].SetAttribute("TITLE", "Step Merge curve")
	plots[6].SetAttribute("DS_LINEWIDTH", "3")
	plots[6].SetAttribute("DS_LEGEND", "Line")
	plots[6].SetAttribute("DS_MODE", "STEP")
	plots[6].SetAttribute("HIGHLIGHTMODE", "SAMPLE")
	plots[6].SetAttribute("DATASETCLIPPING", "NONE")
	plots[6].SetAttribute("AXS_YPOSITION", "END")

	plots[6].SetAttribute("PLOT_CURRENT", "1")

	iup.PlotBegin(plots[6], 0)
	for i := 0; i <= 360; i += 10 {
		x = float64(i)
		y = 50 * math.Sin(x*math.Pi/90.0)
		iup.PlotAdd(plots[6], x, y)
	}
	iup.PlotEnd(plots[6])

	plots[6].SetAttribute("DS_LINEWIDTH", "3")
	plots[6].SetAttribute("DS_LEGEND", "Merged")
	plots[6].SetAttribute("DS_COLOR", "180 180 250")
	plots[6].SetAttribute("HIGHLIGHTMODE", "CURVE")
	plots[6].SetAttribute("AXS_X", "NO")

	// PLOT 7 - Stem Mode
	plots[7].SetAttribute("TITLE", "Stem Mode")
	plots[7].SetAttribute("AXS_XAUTOMIN", "NO")
	plots[7].SetAttribute("AXS_XAUTOMAX", "NO")
	plots[7].SetAttribute("AXS_YAUTOMIN", "NO")
	plots[7].SetAttribute("AXS_YAUTOMAX", "NO")
	plots[7].SetAttribute("AXS_XMIN", "0")
	plots[7].SetAttribute("AXS_XMAX", "0.011")
	plots[7].SetAttribute("AXS_YMIN", "0")
	plots[7].SetAttribute("AXS_YMAX", "0.22")
	plots[7].SetAttribute("AXS_XTICKFORMAT", "%1.3f")
	plots[7].SetAttribute("LEGENDSHOW", "YES")
	plots[7].SetAttribute("LEGENDPOS", "BOTTOMRIGHT")

	iup.PlotBegin(plots[7], 0)
	for i := 0; i <= 10; i++ {
		x = 0.0001 + float64(i)*0.001
		y = 0.2 - theFac*float64(i*i)
		iup.PlotAdd(plots[7], x, y)
	}
	iup.PlotEnd(plots[7])
	plots[7].SetAttribute("DS_MODE", "MARKSTEM")
	plots[7].SetAttribute("DS_MARKSTYLE", "HOLLOW_CIRCLE")
	plots[7].SetAttribute("HIGHLIGHTMODE", "SAMPLE")

	// PLOT 8 - Horizontal Bar Mode
	plots[8].SetAttribute("TITLE", "Horizontal Bar Mode")

	numLabels := []float64{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}
	hbarData := []float64{10, 20, 30, 40, 50, 60, 70, 80, 90, 0, 10, 20}
	iup.PlotBegin(plots[8], 0)
	for i := 0; i < 12; i++ {
		iup.PlotAdd(plots[8], hbarData[i], numLabels[i])
	}
	iup.PlotEnd(plots[8])
	plots[8].SetAttribute("AXS_YAUTOMIN", "NO")
	plots[8].SetAttribute("AXS_YMIN", "0")
	plots[8].SetAttribute("DS_COLOR", "100 100 200")
	plots[8].SetAttribute("DS_MODE", "HORIZONTALBAR")
	plots[8].SetAttribute("DS_BAROUTLINE", "YES")
	plots[8].SetAttribute("DS_BAROUTLINECOLOR", "70 70 160")
	plots[8].SetAttribute("HIGHLIGHTMODE", "SAMPLE")

	// PLOT 9 - Multi-plot (4 sub-plots)
	plots[9].SetAttribute("PLOT_COUNT", "4")
	plots[9].SetAttribute("PLOT_NUMCOL", "2")

	// Sub-plot 0: Multi Bar
	plots[9].SetAttribute("PLOT_CURRENT", "0")
	plots[9].SetAttribute("TITLE", "Multi Bar Mode")

	multiBarData1 := []float64{10, 20, 30, 40, 50, 60, 70, 80, 90, 0, 10, 20}
	iup.PlotBegin(plots[9], 1)
	for i := 0; i < 12; i++ {
		iup.PlotAddStr(plots[9], labels[i], multiBarData1[i])
	}
	iup.PlotEnd(plots[9])
	plots[9].SetAttribute("DS_MODE", "MULTIBAR")
	plots[9].SetAttribute("DS_BAROUTLINE", "YES")
	plots[9].SetAttribute("DS_BAROUTLINECOLOR", "70 70 160")
	plots[9].SetAttribute("HIGHLIGHTMODE", "SAMPLE")

	multiBarData2 := []float64{5, 10, 15, 20, 25, 30, 35, 40, 45, 0, 5, 10}
	iup.PlotBegin(plots[9], 1)
	for i := 0; i < 12; i++ {
		iup.PlotAddStr(plots[9], labels[i], multiBarData2[i])
	}
	iup.PlotEnd(plots[9])
	plots[9].SetAttribute("DS_MODE", "MULTIBAR")
	plots[9].SetAttribute("DS_BAROUTLINE", "YES")
	plots[9].SetAttribute("DS_BAROUTLINECOLOR", "70 70 160")
	plots[9].SetAttribute("HIGHLIGHTMODE", "SAMPLE")

	multiBarData3 := []float64{15, 20, 15, 25, 45, 10, 5, 0, 45, 90, 115, 110}
	iup.PlotBegin(plots[9], 1)
	for i := 0; i < 12; i++ {
		iup.PlotAddStr(plots[9], labels[i], multiBarData3[i])
	}
	iup.PlotEnd(plots[9])
	plots[9].SetAttribute("DS_MODE", "MULTIBAR")
	plots[9].SetAttribute("DS_BAROUTLINE", "YES")
	plots[9].SetAttribute("DS_BAROUTLINECOLOR", "70 70 160")
	plots[9].SetAttribute("HIGHLIGHTMODE", "SAMPLE")

	// Sub-plot 1: Error Bar
	plots[9].SetAttribute("PLOT_CURRENT", "1")
	iup.PlotBegin(plots[9], 0)
	for i := 0; i <= 360; i += 10 {
		x = float64(i)
		y = math.Sin(x * math.Pi / 180.0)
		iup.PlotAdd(plots[9], x, y)
	}
	iup.PlotEnd(plots[9])
	count := 0
	for i := 0; i <= 360; i += 10 {
		iup.PlotSetSampleExtra(plots[9], 0, count, 0.1*rand.Float64())
		count++
	}
	plots[9].SetAttribute("TITLE", "Error Bar")
	plots[9].SetAttribute("DS_LEGEND", "Error Bar")
	plots[9].SetAttribute("DS_MODE", "ERRORBAR")
	plots[9].SetAttribute("HIGHLIGHTMODE", "BOTH")
	plots[9].SetAttribute("DATASETCLIPPING", "AREAOFFSET")

	// Sub-plot 2: Pie
	plots[9].SetAttribute("PLOT_CURRENT", "2")
	plots[9].SetAttribute("TITLE", "Pie")

	pieData := []float64{10, 20, 30, 40, 50, 60, 70, 80, 90, 0, 10, 20}
	iup.PlotBegin(plots[9], 1)
	for i := 0; i < 12; i++ {
		iup.PlotAddStr(plots[9], labels[i], pieData[i])
	}
	iup.PlotEnd(plots[9])
	plots[9].SetAttribute("DS_PIECONTOUR", "YES")
	plots[9].SetAttribute("HIGHLIGHTMODE", "SAMPLE")
	plots[9].SetAttribute("DS_PIESLICELABEL", "X")
	plots[9].SetAttribute("AXS_YFONTSIZE", "10")
	plots[9].SetAttribute("AXS_YCOLOR", "255 255 255")
	plots[9].SetAttribute("DS_PIEHOLE", "0.3")
	plots[9].SetAttribute("DS_COLOR", "255 255 255")
	plots[9].SetAttribute("DS_LINEWIDTH", "3")
	plots[9].SetAttribute("DS_MODE", "PIE")
	plots[9].SetAttribute("VIEWPORTSQUARE", "YES")

	// Sub-plot 3: Area
	plots[9].SetAttribute("PLOT_CURRENT", "3")
	plots[9].SetAttribute("TITLE", "Area")
	plots[9].SetAttribute("FONT", "Helvetica, 10")
	plots[9].SetAttribute("LEGENDSHOW", "YES")
	plots[9].SetAttribute("AXS_XLABEL", "Year")
	plots[9].SetAttribute("AXS_YLABEL", "Company Performance")
	plots[9].SetAttribute("AXS_XMIN", "2013")
	plots[9].SetAttribute("AXS_XMAX", "2016")
	plots[9].SetAttribute("AXS_YMIN", "0")
	plots[9].SetAttribute("AXS_YMAX", "1200")
	plots[9].SetAttribute("AXS_XAUTOMIN", "NO")
	plots[9].SetAttribute("AXS_XAUTOMAX", "NO")
	plots[9].SetAttribute("AXS_YAUTOMIN", "NO")
	plots[9].SetAttribute("AXS_YAUTOMAX", "NO")

	areaYears := []float64{2013, 2014, 2015, 2016}
	areaData1 := []float64{1000, 1170, 660, 1030}
	iup.PlotBegin(plots[9], 0)
	for i := 0; i < 4; i++ {
		iup.PlotAdd(plots[9], areaYears[i], areaData1[i])
	}
	iup.PlotEnd(plots[9])
	plots[9].SetAttribute("DS_MODE", "AREA")
	plots[9].SetAttribute("DS_AREATRANSPARENCY", "64")
	plots[9].SetAttribute("DS_LINEWIDTH", "3")
	plots[9].SetAttribute("HIGHLIGHTMODE", "BOTH")

	areaData2 := []float64{400, 460, 1120, 540}
	iup.PlotBegin(plots[9], 0)
	for i := 0; i < 4; i++ {
		iup.PlotAdd(plots[9], areaYears[i], areaData2[i])
	}
	iup.PlotEnd(plots[9])
	plots[9].SetAttribute("DS_MODE", "AREA")
	plots[9].SetAttribute("DS_AREATRANSPARENCY", "64")
	plots[9].SetAttribute("DS_LINEWIDTH", "3")
	plots[9].SetAttribute("HIGHLIGHTMODE", "BOTH")
}

func tabsTabChangeCB(self iup.Ihandle, newTab, oldTab iup.Ihandle) int {
	ss := newTab.GetAttribute("TABTITLE")
	ss = strings.TrimPrefix(ss, "Plot ")
	ii, _ := strconv.Atoi(ss)

	if iup.GetInt(plots[ii], "AXS_XAUTOMIN") != 0 && iup.GetInt(plots[ii], "AXS_XAUTOMAX") != 0 {
		tgg2.SetAttribute("VALUE", "ON")
	} else {
		tgg2.SetAttribute("VALUE", "OFF")
	}

	if iup.GetInt(plots[ii], "AXS_YAUTOMIN") != 0 && iup.GetInt(plots[ii], "AXS_YAUTOMAX") != 0 {
		tgg1.SetAttribute("VALUE", "ON")
	} else {
		tgg1.SetAttribute("VALUE", "OFF")
	}

	grid := iup.GetAttribute(plots[ii], "GRID")
	if iup.GetInt(plots[ii], "GRID") != 0 {
		tgg3.SetAttribute("VALUE", "ON")
		tgg4.SetAttribute("VALUE", "ON")
	} else {
		if len(grid) > 0 && grid[0] == 'V' {
			tgg3.SetAttribute("VALUE", "ON")
		} else {
			tgg3.SetAttribute("VALUE", "OFF")
		}
		if len(grid) > 0 && grid[0] == 'H' {
			tgg4.SetAttribute("VALUE", "ON")
		} else {
			tgg4.SetAttribute("VALUE", "OFF")
		}
	}

	if iup.GetInt(plots[ii], "LEGENDSHOW") != 0 {
		tgg5.SetAttribute("VALUE", "ON")
	} else {
		tgg5.SetAttribute("VALUE", "OFF")
	}

	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.PlotOpen()

	for i := 0; i < maxPlot; i++ {
		plots[i] = iup.Plot()
		plots[i].SetAttribute("MENUITEMPROPERTIES", "YES")
	}

	// Y Autoscale toggle
	tgg1 = iup.Toggle("Y Autoscale")
	iup.SetCallback(tgg1, "ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		ii := tabsGetIndex()
		if state != 0 {
			plots[ii].SetAttribute("AXS_YAUTOMIN", "YES")
			plots[ii].SetAttribute("AXS_YAUTOMAX", "YES")
		} else {
			plots[ii].SetAttribute("AXS_YAUTOMIN", "NO")
			plots[ii].SetAttribute("AXS_YAUTOMAX", "NO")
		}
		plots[ii].SetAttribute("REDRAW", "")
		return iup.DEFAULT
	}))
	tgg1.SetAttribute("VALUE", "ON")

	// X Autoscale toggle
	tgg2 = iup.Toggle("X Autoscale")
	iup.SetCallback(tgg2, "ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		ii := tabsGetIndex()
		if state != 0 {
			plots[ii].SetAttribute("AXS_XAUTOMIN", "YES")
			plots[ii].SetAttribute("AXS_XAUTOMAX", "YES")
		} else {
			plots[ii].SetAttribute("AXS_XAUTOMIN", "NO")
			plots[ii].SetAttribute("AXS_XAUTOMAX", "NO")
		}
		plots[ii].SetAttribute("REDRAW", "")
		return iup.DEFAULT
	}))

	// Vertical Grid toggle
	tgg3 = iup.Toggle("Vertical Grid")
	iup.SetCallback(tgg3, "ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		ii := tabsGetIndex()
		if state != 0 {
			if iup.GetInt(tgg4, "VALUE") != 0 {
				plots[ii].SetAttribute("GRID", "YES")
			} else {
				plots[ii].SetAttribute("GRID", "VERTICAL")
			}
		} else {
			if iup.GetInt(tgg4, "VALUE") == 0 {
				plots[ii].SetAttribute("GRID", "NO")
			} else {
				plots[ii].SetAttribute("GRID", "HORIZONTAL")
			}
		}
		plots[ii].SetAttribute("REDRAW", "")
		return iup.DEFAULT
	}))

	// Horizontal Grid toggle
	tgg4 = iup.Toggle("Horizontal Grid")
	iup.SetCallback(tgg4, "ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		ii := tabsGetIndex()
		if state != 0 {
			if iup.GetInt(tgg3, "VALUE") != 0 {
				plots[ii].SetAttribute("GRID", "YES")
			} else {
				plots[ii].SetAttribute("GRID", "HORIZONTAL")
			}
		} else {
			if iup.GetInt(tgg3, "VALUE") == 0 {
				plots[ii].SetAttribute("GRID", "NO")
			} else {
				plots[ii].SetAttribute("GRID", "VERTICAL")
			}
		}
		plots[ii].SetAttribute("REDRAW", "")
		return iup.DEFAULT
	}))

	// Legend toggle
	tgg5 = iup.Toggle("Legend")
	iup.SetCallback(tgg5, "ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		ii := tabsGetIndex()
		if state != 0 {
			plots[ii].SetAttribute("LEGENDSHOW", "YES")
		} else {
			plots[ii].SetAttribute("LEGENDSHOW", "NO")
		}
		plots[ii].SetAttribute("REDRAW", "")
		return iup.DEFAULT
	}))

	// Clear button
	bt1 := iup.Button("Clear")
	iup.SetCallback(bt1, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		ii := tabsGetIndex()
		plots[ii].SetAttribute("CLEAR", "YES")
		plots[ii].SetAttribute("REDRAW", "")
		return iup.DEFAULT
	}))

	scaleFrame := iup.Frame(iup.Vbox(tgg1, tgg2))
	scaleFrame.SetAttribute("TITLE", "Autoscale")

	gridFrame := iup.Frame(iup.Vbox(tgg3, tgg4))
	gridFrame.SetAttribute("TITLE", "Grid")

	optionsFrame := iup.Frame(iup.Vbox(tgg5, bt1))
	optionsFrame.SetAttribute("TITLE", "Options")

	vboxl := iup.Vbox(scaleFrame, gridFrame, optionsFrame)
	vboxl.SetAttribute("GAP", "4")
	vboxl.SetAttribute("EXPAND", "NO")

	vboxr := make([]iup.Ihandle, maxPlot)
	for i := 0; i < maxPlot; i++ {
		vboxr[i] = iup.Vbox(plots[i])
		vboxr[i].SetAttribute("TABTITLE", fmt.Sprintf("Plot %d", i))
		iup.SetHandle(vboxr[i].GetAttribute("TABTITLE"), vboxr[i])
	}

	tabs = iup.Tabs(vboxr...)
	iup.SetCallback(tabs, "TABCHANGE_CB", iup.TabChangeFunc(tabsTabChangeCB))

	hbox := iup.Hbox(vboxl, tabs)
	hbox.SetAttribute("MARGIN", "0x0")
	hbox.SetAttribute("GAP", "10")

	dlg := iup.Dialog(hbox)
	dlg.SetAttribute("TITLE", "IupPlot Example")

	initPlots()

	tabsTabChangeCB(tabs, vboxr[0], 0)

	dlg.SetAttribute("SIZE", "x400")
	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	iup.MainLoop()
}

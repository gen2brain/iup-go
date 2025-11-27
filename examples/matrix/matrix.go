//go:build ctl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

const TestImageSize = 20

var imageData32 = []byte{
	0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
	0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255,
}

func mouseMoveCb(ih iup.Ihandle, lin, col int) int {
	//fmt.Printf("mouseMoveCb(%d, %d)\n", lin, col)
	return iup.DEFAULT
}

func dropCb(ih, drop iup.Ihandle, lin, col int) int {
	fmt.Printf("dropCb(%d, %d)\n", lin, col)
	if lin == 3 && col == 1 {
		drop.SetAttribute("1", "A - Test of Very Big String for Dropdown!")
		drop.SetAttribute("2", "B")
		drop.SetAttribute("3", "C")
		drop.SetAttribute("4", "XXX")
		drop.SetAttribute("5", "5")
		drop.SetAttribute("6", "6")
		drop.SetAttribute("7", "7")
		drop.SetAttribute("8", "")
		drop.SetAttribute("VALUE", "4")
		return iup.DEFAULT
	}
	return iup.IGNORE
}

func dropCheckCb(ih iup.Ihandle, lin, col int) int {
	if lin == 3 && col == 1 {
		return iup.DEFAULT
	}
	if lin == 4 && col == 4 {
		return iup.CONTINUE
	}
	return iup.IGNORE
}

func toggleValueCb(ih iup.Ihandle, lin, col, value int) int {
	fmt.Printf("toggleValueCb(%d, %d)=%d\n", lin, col, value)
	return iup.DEFAULT
}

func clickCb(ih iup.Ihandle, lin, col int, status string) int {
	fmt.Printf("clickCb(%d, %d)\n", lin, col)
	ih.SetAttribute("MARKED", "")
	iup.SetAttributeId2(ih, "MARK", lin, 0, "1")
	ih.SetAttribute("REDRAW", fmt.Sprintf("L%d", lin))
	return iup.DEFAULT
}

func enterItemCb(ih iup.Ihandle, lin, col int) int {
	ih.SetAttribute("MARKED", "")
	iup.SetAttributeId2(ih, "MARK", lin, 0, "1")
	ih.SetAttribute("REDRAW", fmt.Sprintf("L%d", lin))
	return iup.DEFAULT
}

func createMatrix() iup.Ihandle {
	//mat := iup.Matrix("")
	mat := iup.MatrixEx()

	mat.SetAttribute("NUMLIN", "4")
	mat.SetAttribute("NUMCOL", "8")

	// Set row and column headers (0:0, 1:0, 2:0, 3:0 are row headers; 0:1, 0:2, etc. are column headers)
	mat.SetAttribute("0:0", "Inflation")
	mat.SetAttribute("1:0", "Medicine\nPharma")
	mat.SetAttribute("2:0", "Food")
	mat.SetAttribute("3:0", "Energy")
	mat.SetAttribute("0:1", "January 2000")
	mat.SetAttribute("0:2", "February 2000")
	mat.SetAttribute("0:3", "March 2000")
	mat.SetAttribute("0:4", "April 2000")
	mat.SetAttribute("0:5", "May 2000")
	mat.SetAttribute("0:6", "June 2000")
	mat.SetAttribute("0:7", "July 2000")
	mat.SetAttribute("0:8", "August 2000")

	// Set inflation data (Medicine/Pharma row)
	mat.SetAttribute("1:1", "5.6\n3.33")
	mat.SetAttribute("1:2", "4.5")
	mat.SetAttribute("1:3", "5.1")
	mat.SetAttribute("1:4", "6.2")
	mat.SetAttribute("1:5", "4.8")
	mat.SetAttribute("1:6", "5.3")
	mat.SetAttribute("1:7", "6.1")
	mat.SetAttribute("1:8", "4.9")

	// Food row
	mat.SetAttribute("2:1", "2.2")
	mat.SetAttribute("2:2", "(çãõáóé)")
	mat.SetAttribute("2:3", "2.5")
	mat.SetAttribute("2:4", "2.1")
	mat.SetAttribute("2:5", "3.2")
	mat.SetAttribute("2:6", "2.9")
	mat.SetAttribute("2:7", "2.7")
	mat.SetAttribute("2:8", "2.4")

	// Energy row
	mat.SetAttribute("3:1", "3.4")
	mat.SetAttribute("3:2", "Very Very Very Very Very Large Text")
	mat.SetAttribute("3:3", "Font Test")
	mat.SetAttribute("3:4", "7.5")
	mat.SetAttribute("3:5", "8.1")
	mat.SetAttribute("3:6", "9.3")
	mat.SetAttribute("3:7", "7.9")
	mat.SetAttribute("3:8", "8.5")

	mat.SetAttribute("SORTSIGN2", "DOWN")
	mat.SetAttribute("RESIZEMATRIX", "YES")
	mat.SetAttribute("USETITLESIZE", "YES") // Auto-size rows/columns based on content

	// Set custom font for cell (3,3)
	mat.SetAttribute("FONT3:3", "Helvetica, 24")

	// Set mask for column 3
	mat.SetAttribute("MASK*:3", "[a-zA-Z][0-9a-zA-Z_]*")

	// Color type in cell (4,1)
	mat.SetAttribute("TYPE4:1", "COLOR")
	mat.SetAttribute("4:1", "255 0 128")

	// Fill type in cell (4,2)
	mat.SetAttribute("TYPE4:2", "FILL")
	mat.SetAttribute("4:2", "60")
	mat.SetAttribute("SHOWFILLVALUE", "Yes")

	// Image type in cell (4,3)
	image := iup.ImageRGBA(TestImageSize, TestImageSize, imageData32)
	mat.SetAttribute("TYPE4:3", "IMAGE")
	iup.SetAttributeHandle(mat, "4:3", image)

	// Toggle cell configuration
	mat.SetAttribute("TOGGLECENTERED", "Yes")

	// Mark settings
	mat.SetAttribute("MARKMODE", "CELL")
	mat.SetAttribute("MARKMULTIPLE", "YES")

	// Frame colors
	mat.SetAttribute("FRAMEVERTCOLOR1:2", "BGCOLOR")
	mat.SetAttribute("FRAMEHORIZCOLOR1:2", "0 0 255")
	mat.SetAttribute("FRAMEHORIZCOLOR1:3", "0 255 0")
	mat.SetAttribute("FRAMEVERTCOLOR2:2", "255 255 0")
	mat.SetAttribute("FRAMEVERTCOLOR*:4", "0 255 0")
	mat.SetAttribute("FRAMEVERTCOLOR*:5", "BGCOLOR")

	mat.SetAttribute("20:8", "The End")
	mat.SetAttribute("NUMCOL_VISIBLE", "3")
	mat.SetAttribute("NUMLIN_VISIBLE", "5")

	// Matrix Size Variants:
	// mat.SetAttribute("NUMLIN", "3")  // Smaller matrix
	// mat.SetAttribute("NUMCOL", "15") // Wider matrix
	//
	// Edit Behavior:
	// mat.SetAttribute("READONLY", "Yes")          // No editing allowed
	// mat.SetAttribute("EDITHIDEONFOCUS", "NO")    // Keep edit box visible
	// mat.SetAttribute("EDITALIGN", "Yes")         // Align edit box with cell
	// mat.SetAttribute("EDITFITVALUE", "Yes")      // Edit box fits cell value
	// mat.SetAttribute("EDITNEXT", "COLCR")        // Move to next cell after edit
	//
	// Sizing Options:
	// mat.SetAttribute("HEIGHT2", "30")            // Row 2 height
	// mat.SetAttribute("WIDTH2", "190")            // Column 2 width
	// mat.SetAttribute("WIDTHDEF", "34")           // Default column width
	// mat.SetAttribute("USETITLESIZE", "YES")      // Size titles to fit content
	// mat.SetAttribute("LIMITEXPAND", "Yes")       // Limit matrix expansion
	//
	// Multiline and Text Options:
	// mat.SetAttribute("MULTILINE", "YES")         // Allow multiline text
	// mat.SetAttribute("HIDDENTEXTMARKS", "YES")   // Show hidden text marks
	//
	// Scrollbar Options:
	// mat.SetAttribute("SCROLLBAR", "NO")          // Disable scrollbar
	// mat.SetAttribute("SCROLLBAR", "HORIZONTAL")  // Only horizontal scrollbar
	// mat.SetAttribute("XAUTOHIDE", "NO")          // Don't auto-hide X scrollbar
	// mat.SetAttribute("YAUTOHIDE", "NO")          // Don't auto-hide Y scrollbar
	// mat.SetAttribute("NUMCOL_NOSCROLL", "1")     // First column no scroll
	// mat.SetAttribute("FLATSCROLLBAR", "Yes")     // Flat scrollbar style
	//
	// Cell Colors (uncomment to test colorful cells):
	// mat.SetAttribute("BGCOLOR1:2", "255 92 255") // Cell (1,2) background
	// mat.SetAttribute("BGCOLOR2:*", "92 92 255")  // Row 2 all columns background
	// mat.SetAttribute("BGCOLOR*:3", "255 92 92")  // Column 3 all rows background
	// mat.SetAttribute("FGCOLOR1:2", "255 0 0")    // Cell (1,2) foreground (red)
	// mat.SetAttribute("FGCOLOR2:*", "0 128 0")    // Row 2 foreground (green)
	// mat.SetAttribute("FGCOLOR*:3", "0 0 255")    // Column 3 foreground (blue)
	//
	// Cell Fonts:
	// mat.SetAttribute("FONT2:*", "Courier, 14")   // Row 2 font
	// mat.SetAttribute("FONT*:3", "Times, Bold 14") // Column 3 font
	//
	// Alignment Options:
	// mat.SetAttribute("ALIGNMENT1", "ALEFT")      // Column 1 left-aligned
	// mat.SetAttribute("ALIGNMENT3", "ARIGHT")     // Column 3 right-aligned
	// mat.SetAttribute("ALIGN2:1", ":ARIGHT")      // Cell (2,1) right-aligned
	// mat.SetAttribute("LINEALIGNMENT1", "ATOP")   // Row 1 top-aligned
	//
	// Input Masks:
	// mat.SetAttribute("MASK1:3", iup.MASK_FLOAT)  // Float mask for cell (1,3)
	// mat.SetAttribute("MASKFLOAT1:3", "0.0:10.0") // Float range 0-10
	//
	// Toggle Cells:
	// mat.SetAttribute("TOGGLEVALUE4:4", "ON")     // Toggle cell (4,4) on
	// mat.SetAttribute("VALUE4:4", "1")            // Alternative: set value directly
	//
	// Mark Mode Options (selection behavior):
	// mat.SetAttribute("MARKMODE", "LIN")          // Select entire lines
	// mat.SetAttribute("MARKMODE", "LINCOL")       // Select lines and columns
	// mat.SetAttribute("MARKMULTIPLE", "NO")       // Only single selection
	// mat.SetAttribute("MARKAREA", "NOT_CONTINUOUS") // Non-continuous selection
	// mat.SetAttribute("MARK2:2", "YES")           // Pre-mark cell (2,2)
	// mat.SetAttribute("MARK2:3", "YES")           // Pre-mark cell (2,3)
	// mat.SetAttribute("MARK3:3", "YES")           // Pre-mark cell (3,3)
	//
	// Visibility Options:
	// mat.SetAttribute("NUMCOL_VISIBLE_LAST", "YES") // Show last column completely
	// mat.SetAttribute("NUMLIN_VISIBLE_LAST", "YES") // Show last line completely
	//
	// Additional Title Cells:
	// mat.SetAttribute("10:0", "Middle Line")      // Row title at line 10
	// mat.SetAttribute("15:0", "Middle Line")      // Row title at line 15
	// mat.SetAttribute("0:4", "Middle Column")     // Column title at col 4
	// mat.SetAttribute("20:0", "Line Title Test")  // Row title at line 20
	// mat.SetAttribute("0:8", "Column Title Test") // Column title at col 8
	//
	// Size and Layout:
	// mat.SetAttribute("RASTERSIZE", "x300")       // Fixed height 300 pixels
	// mat.SetAttribute("FITTOSIZE", "LINES")       // Fit lines to available size
	// mat.SetAttribute("EXPAND", "NO")             // Don't expand in dialog
	// mat.SetAttribute("FRAMEBORDER", "Yes")       // Draw frame border
	// mat.SetAttribute("SHOWFLOATING", "Yes")      // Show floating elements
	//
	// Special States:
	// mat.SetAttribute("ACTIVE", "NO")             // Disable entire matrix
	// mat.SetAttribute("TYPECOLORINACTIVE", "No")  // Don't gray out inactive colors

	// Set callbacks
	mat.SetCallback("DROPCHECK_CB", iup.DropCheckFunc(dropCheckCb))
	mat.SetCallback("DROP_CB", iup.MatrixDropFunc(dropCb))
	mat.SetCallback("TOGGLEVALUE_CB", iup.MatrixToggleValueFunc(toggleValueCb))
	mat.SetCallback("MOUSEMOVE_CB", iup.MatrixMouseMoveFunc(mouseMoveCb))
	mat.SetCallback("CLICK_CB", iup.ClickFunc(clickCb))
	mat.SetCallback("ENTERITEM_CB", iup.EnterItemFunc(enterItemCb))

	return mat
}

func main() {
	iup.Open()
	iup.ControlsOpen()
	defer iup.Close()

	mat := createMatrix()
	box := iup.Vbox(mat)
	box.SetAttribute("MARGIN", "10x10")

	dlg := iup.Dialog(box)
	dlg.SetAttribute("TITLE", "IupMatrix Simple Test")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	iup.MainLoop()
}

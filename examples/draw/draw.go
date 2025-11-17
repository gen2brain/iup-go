package main

import (
	"fmt"
	"github.com/gen2brain/iup-go/iup"
	"math"
)

func main() {
	iup.Open()
	defer iup.Close()

	createTestImage()

	canvas := iup.Canvas()
	canvas.SetAttribute("RASTERSIZE", "750x750")
	canvas.SetAttribute("BORDER", "NO")
	canvas.SetAttribute("BGCOLOR", "255 255 255")

	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))

	dlg := iup.Dialog(canvas)
	dlg.SetAttribute("TITLE", "IUP Drawing Showcase")

	iup.Show(dlg)
	iup.MainLoop()
}

func createTestImage() {
	width, height := 16, 16
	pixels := make([]byte, width*height*4)

	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			idx := (y*width + x) * 4
			// Create a colorful gradient pattern
			pixels[idx+0] = byte((x * 255) / width)                  // R
			pixels[idx+1] = byte((y * 255) / height)                 // G
			pixels[idx+2] = byte(((x + y) * 255) / (width + height)) // B
			pixels[idx+3] = 255                                      // A
		}
	}

	img := iup.ImageRGBA(width, height, pixels)
	iup.SetHandle("TEST_IMAGE", img)
}

func redraw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)

	// White background
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)

	// Title
	ih.SetAttribute("DRAWCOLOR", "30 30 30")
	ih.SetAttribute("DRAWFONT", "Helvetica, Bold 14")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ACENTER")
	iup.DrawText(ih, "IUP Drawing Functions Showcase", w/2, 10, -1, -1)

	margin := 15
	yPos := 40

	// Lines with different styles
	drawLinesSection(ih, margin, yPos, w)
	yPos += 105

	// Rectangles (filled and stroked)
	drawRectanglesSection(ih, margin, yPos, w)
	yPos += 120

	// Arcs and Circles
	drawArcsSection(ih, margin, yPos, w)
	yPos += 160

	// Polygons
	drawPolygonsSection(ih, margin, yPos, w)
	yPos += 115

	// Text rendering
	drawTextSection(ih, margin, yPos, w)
	yPos += 130

	// Images, Clipped Circle, and Special effects (bottom row)
	drawImagesSection(ih, margin, yPos)
	drawClippedCircle(ih, margin+150, yPos)
	drawSpecialEffects(ih, margin+320, yPos)

	return iup.DEFAULT
}

func drawLinesSection(ih iup.Ihandle, x, y, maxW int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Lines & Pixels", x, y, -1, -1)
	y += 30

	styles := []struct {
		style string
		name  string
	}{
		{"STROKE", "Solid"},
		{"STROKE_DASH", "Dashed"},
		{"STROKE_DOT", "Dotted"},
	}

	ih.SetAttribute("DRAWLINEWIDTH", "2")
	for i, s := range styles {
		yLine := y + i*16
		// Draw style name
		ih.SetAttribute("DRAWCOLOR", "80 80 80")
		iup.DrawText(ih, s.name, x, yLine-10, -1, -1)
		// Draw line with style
		ih.SetAttribute("DRAWCOLOR", "255 50 50")
		ih.SetAttribute("DRAWSTYLE", s.style)
		iup.DrawLine(ih, x+140, yLine, x+300, yLine)
	}

	// Draw lines with different widths
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWCOLOR", "80 80 80")
	iup.DrawText(ih, "Width: 1px", x+420, y-5, -1, -1)
	ih.SetAttribute("DRAWCOLOR", "50 150 255")
	ih.SetAttribute("DRAWLINEWIDTH", "1")
	iup.DrawLine(ih, x+520, y, x+620, y)

	ih.SetAttribute("DRAWCOLOR", "80 80 80")
	iup.DrawText(ih, "Width: 3px", x+420, y+15, -1, -1)
	ih.SetAttribute("DRAWCOLOR", "50 150 255")
	ih.SetAttribute("DRAWLINEWIDTH", "3")
	iup.DrawLine(ih, x+520, y+20, x+620, y+20)

	ih.SetAttribute("DRAWCOLOR", "80 80 80")
	iup.DrawText(ih, "Width: 6px", x+420, y+35, -1, -1)
	ih.SetAttribute("DRAWCOLOR", "50 150 255")
	ih.SetAttribute("DRAWLINEWIDTH", "6")
	iup.DrawLine(ih, x+520, y+40, x+620, y+40)

	// Pixel art demo (small checkerboard pattern)
	pixelY := y + 45
	ih.SetAttribute("DRAWCOLOR", "80 80 80")
	iup.DrawText(ih, "Pixel Art:", x, pixelY-5, -1, -1)

	// Draw a simple pixel pattern (8x8 checkerboard)
	pixelSize := 3
	for py := 0; py < 8; py++ {
		for px := 0; px < 8; px++ {
			// Checkerboard pattern with colors
			if (px+py)%2 == 0 {
				ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d %d", 255-px*20, 100+py*15, 150))
			} else {
				ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d %d", 100+px*15, 255-py*20, 200))
			}
			// Draw multiple pixels for visibility
			for dy := 0; dy < pixelSize; dy++ {
				for dx := 0; dx < pixelSize; dx++ {
					iup.DrawPixel(ih, x+140+px*pixelSize+dx, pixelY+py*pixelSize+dy)
				}
			}
		}
	}
}

func drawRectanglesSection(ih iup.Ihandle, x, y, maxW int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Rectangles & Rounded Rectangles", x, y, -1, -1)
	y += 25

	// Filled rectangles with different colors
	colors := []struct {
		r, g, b int
		name    string
	}{
		{255, 100, 100, "Red"},
		{100, 255, 100, "Green"},
		{100, 100, 255, "Blue"},
	}

	ih.SetAttribute("DRAWSTYLE", "FILL")
	for i, c := range colors {
		rectX := x + i*80
		// Filled rectangle
		ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d %d", c.r, c.g, c.b))
		iup.DrawRectangle(ih, rectX, y, rectX+60, y+40)
		// Label
		ih.SetAttribute("DRAWCOLOR", "80 80 80")
		ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
		iup.DrawText(ih, c.name, rectX+5, y+45, -1, -1)
	}

	// Rounded rectangles with different radii
	roundedY := y
	for i := 0; i < 3; i++ {
		rectX := x + 340 + i*100
		radius := (i + 1) * 5
		// Filled rounded rectangle
		ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d %d", 255-i*50, 150, 100+i*50))
		ih.SetAttribute("DRAWSTYLE", "FILL")
		iup.DrawRoundedRectangle(ih, rectX, roundedY, rectX+70, roundedY+40, radius)
		// Label
		ih.SetAttribute("DRAWCOLOR", "80 80 80")
		ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
		label := fmt.Sprintf("R=%d", radius)
		iup.DrawText(ih, label, rectX+5, roundedY+45, -1, -1)
	}

	// Stroked rectangles with different line widths
	y += 65
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWCOLOR", "100 50 150")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ACENTER")
	for i := 0; i < 3; i++ {
		rectX := x + i*90
		lineWidth := i + 1
		ih.SetAttribute("DRAWLINEWIDTH", fmt.Sprintf("%d", lineWidth))
		iup.DrawRectangle(ih, rectX, y, rectX+70, y+20)
		ih.SetAttribute("DRAWCOLOR", "80 80 80")
		label := fmt.Sprintf("%dpx", lineWidth)
		iup.DrawText(ih, label, rectX+35, y+3, -1, -1)
		ih.SetAttribute("DRAWCOLOR", "100 50 150")
	}

	// Stroked rounded rectangles
	for i := 0; i < 3; i++ {
		rectX := x + 340 + i*100
		lineWidth := (i + 1) * 2
		ih.SetAttribute("DRAWCOLOR", "50 150 200")
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", fmt.Sprintf("%d", lineWidth))
		iup.DrawRoundedRectangle(ih, rectX, y, rectX+70, y+20, 8)
		ih.SetAttribute("DRAWCOLOR", "80 80 80")
		ih.SetAttribute("DRAWTEXTALIGNMENT", "ACENTER")
		label := fmt.Sprintf("%dpx", lineWidth)
		iup.DrawText(ih, label, rectX+35, y+3, -1, -1)
	}
}

func drawArcsSection(ih iup.Ihandle, x, y, maxW int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Arcs and Circles", x, y+10, -1, -1)
	y += 25

	// Filled circles
	radius := 30
	ih.SetAttribute("DRAWSTYLE", "FILL")
	for i := 0; i < 3; i++ {
		cx := x + 50 + i*120
		cy := y + 40
		ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d 150", 255-i*60, 100+i*40))
		iup.DrawArc(ih, cx-radius, cy-radius, cx+radius, cy+radius, 0, 360)
	}

	// Ellipses (horizontal)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "2")
	for i := 0; i < 3; i++ {
		ex := x + 400 + i*120
		ey := y + 40
		ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("150 %d %d", 100+i*50, 200-i*30))
		iup.DrawArc(ih, ex-50, ey-25, ex+50, ey+25, 0, 360)
	}

	// Stroked circles
	y += 80
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	for i := 0; i < 3; i++ {
		cx := x + 50 + i*120
		cy := y + 20
		ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("100 %d 200", 100+i*50))
		ih.SetAttribute("DRAWLINEWIDTH", fmt.Sprintf("%d", (i+1)*2))
		iup.DrawArc(ih, cx-25, cy-25, cx+25, cy+25, 0, 360)
	}
}

func drawPolygonsSection(ih iup.Ihandle, x, y, maxW int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Polygons", x, y, -1, -1)
	y += 25

	// Triangle (filled)
	ih.SetAttribute("DRAWCOLOR", "255 150 50")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	triangle := []int{
		x + 60, y + 5, // top
		x + 20, y + 70, // bottom left
		x + 100, y + 70, // bottom right
	}
	iup.DrawPolygon(ih, triangle, len(triangle)/2)

	// Pentagon (stroked)
	cx, cy, r := x+180, y+40, 35
	pentagon := make([]int, 10)
	for i := 0; i < 5; i++ {
		angle := float64(i)*72.0*math.Pi/180.0 - math.Pi/2
		pentagon[i*2] = cx + int(float64(r)*math.Cos(angle))
		pentagon[i*2+1] = cy + int(float64(r)*math.Sin(angle))
	}
	ih.SetAttribute("DRAWCOLOR", "50 200 50")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "3")
	iup.DrawPolygon(ih, pentagon, len(pentagon)/2)

	// Star (filled)
	cx, cy, outerR, innerR := x+300, y+40, 40, 18
	star := make([]int, 20)
	for i := 0; i < 10; i++ {
		angle := float64(i)*36.0*math.Pi/180.0 - math.Pi/2
		radius := outerR
		if i%2 == 1 {
			radius = innerR
		}
		star[i*2] = cx + int(float64(radius)*math.Cos(angle))
		star[i*2+1] = cy + int(float64(radius)*math.Sin(angle))
	}
	ih.SetAttribute("DRAWCOLOR", "255 215 0")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawPolygon(ih, star, len(star)/2)
}

func drawTextSection(ih iup.Ihandle, x, y, maxW int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Text Rendering", x, y-10, -1, -1)
	y += 25

	// Left aligned
	ih.SetAttribute("DRAWCOLOR", "255 0 0")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Left Aligned", x, y-10, -1, -1)

	// Center aligned
	ih.SetAttribute("DRAWCOLOR", "0 150 0")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ACENTER")
	iup.DrawText(ih, "Center Aligned", x+320, y-10, -1, -1)

	// Right aligned
	ih.SetAttribute("DRAWCOLOR", "0 0 255")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ARIGHT")
	iup.DrawText(ih, "Right Aligned", x+625, y-10, -1, -1)

	y += 20

	// Multi-line text with wrapping (manual demonstration)
	ih.SetAttribute("DRAWCOLOR", "255 255 200")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, x, y, x+300, y+70)
	ih.SetAttribute("DRAWCOLOR", "200 200 150")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "1")
	iup.DrawRectangle(ih, x, y, x+300, y+70)

	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	ih.SetAttribute("DRAWFONT", "Helvetica, 10")
	iup.DrawText(ih, "This is a long text that demonstrates", x+5, y+5, -1, -1)
	iup.DrawText(ih, "text rendering. Multiple lines can be", x+5, y+20, -1, -1)
	iup.DrawText(ih, "drawn by calling DrawText multiple", x+5, y+35, -1, -1)
	iup.DrawText(ih, "times at different Y positions.", x+5, y+50, -1, -1)
}

func drawImagesSection(ih iup.Ihandle, x, y int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Images & Clipping", x, y, -1, -1)
	y += 20

	// Draw original image
	iup.DrawImage(ih, "TEST_IMAGE", x, y, -1, -1)

	// Draw scaled image
	iup.DrawImage(ih, "TEST_IMAGE", x+35, y, 32, 32)

	// Draw image with clipping
	iup.DrawSetClipRect(ih, x+85, y, x+125, y+20)
	iup.DrawImage(ih, "TEST_IMAGE", x+85, y, 48, 48)
	iup.DrawResetClip(ih)
	ih.SetAttribute("DRAWCOLOR", "255 0 0")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "1")
	iup.DrawRectangle(ih, x+85, y, x+125, y+20)
}

func drawClippedCircle(ih iup.Ihandle, x, y int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Clipped Circle", x, y, -1, -1)
	y += 20

	// Set clip region
	iup.DrawSetClipRect(ih, x, y, x+60, y+30)
	// Draw circle (will be clipped)
	ih.SetAttribute("DRAWCOLOR", "255 150 0")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawArc(ih, x+5, y-10, x+55, y+40, 0, 360)
	iup.DrawResetClip(ih)
	// Show clip boundary
	ih.SetAttribute("DRAWCOLOR", "255 0 0")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "2")
	iup.DrawRectangle(ih, x, y, x+60, y+30)
}

func drawSpecialEffects(ih iup.Ihandle, x, y int) {
	// Section title
	ih.SetAttribute("DRAWCOLOR", "50 50 50")
	ih.SetAttribute("DRAWFONT", "Helvetica, 12")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Special Effects", x, y, -1, -1)
	y += 20

	// Selection rectangle
	ih.SetAttribute("DRAWCOLOR", "80 80 80")
	ih.SetAttribute("DRAWFONT", "Helvetica, 10")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, "Selection:", x, y, -1, -1)
	iup.DrawSelectRect(ih, x, y+15, x+80, y+45)

	// Focus rectangle
	iup.DrawText(ih, "Focus:", x+120, y, -1, -1)
	ih.SetAttribute("DRAWCOLOR", "220 220 220")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, x+120, y+15, x+200, y+45)
	iup.DrawFocusRect(ih, x+125, y+20, x+195, y+40)

	// Alpha transparency demonstration (real alpha blending)
	ih.SetAttribute("DRAWCOLOR", "80 80 80")
	iup.DrawText(ih, "Transparency:", x+240, y, -1, -1)

	// Background
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, x+240, y+15, x+340, y+45)

	// Overlapping rectangles with real alpha transparency
	ih.SetAttribute("DRAWSTYLE", "FILL")
	for i := 0; i < 4; i++ {
		// Real alpha transparency - all modern drivers support this!
		r := 255 - i*30
		g := 100 + i*40
		b := 100 + i*20
		alpha := int(255 * (1.0 - float64(i)*0.2)) // Decreasing opacity: 255, 204, 153, 102
		ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d %d %d", r, g, b, alpha))
		iup.DrawRectangle(ih, x+250+i*15, y+20, x+290+i*15, y+40)
	}
}

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetHandle("activeimg", makeImage(40, 110, 220))
	iup.SetHandle("impressimg", makeImage(40, 200, 90))

	var testLeaves []iup.Ihandle
	var rows []iup.Ihandle

	add := func(name string, test, ref iup.Ihandle) {
		rows = append(rows, iup.Label(name), test, ref)
		testLeaves = append(testLeaves, test)
		ref.SetAttribute("ACTIVE", "NO")
	}

	simple := func(name string, ctor func() iup.Ihandle) {
		add(name, ctor(), ctor())
	}

	rows = append(rows,
		iup.Label("Control").SetAttributes(`FONTSTYLE=Bold`),
		iup.Label("Test (toggle)").SetAttributes(`FONTSTYLE=Bold`),
		iup.Label("Reference (inactive)").SetAttributes(`FONTSTYLE=Bold`),
	)

	simple("Button", func() iup.Ihandle { return iup.Button("Button") })
	simple("Button (image)", func() iup.Ihandle {
		return iup.Button("Go").SetAttributes(`IMAGE=activeimg`)
	})
	simple("Toggle", func() iup.Ihandle {
		return iup.Toggle("Check").SetAttribute("VALUE", "ON")
	})
	simple("Toggle (3-state)", func() iup.Ihandle {
		return iup.Toggle("Tri-state").SetAttributes(`3STATE=YES, VALUE=NOTDEF`)
	})
	simple("Toggle (right)", func() iup.Ihandle {
		return iup.Toggle("Right button").SetAttributes(`RIGHTBUTTON=YES, VALUE=ON`)
	})
	simple("Toggle (switch)", func() iup.Ihandle {
		return iup.Toggle("Switch").SetAttributes(`SWITCH=YES, VALUE=ON`)
	})
	simple("Toggle (image align)", func() iup.Ihandle {
		return iup.Toggle("").SetAttributes(`IMAGE=activeimg, RASTERSIZE=90x50, ALIGNMENT=ARIGHT:ABOTTOM`)
	})
	simple("Toggle (image flat)", func() iup.Ihandle {
		return iup.Toggle("").SetAttributes(`IMAGE=activeimg, FLAT=YES`)
	})
	simple("Toggle (image impress)", func() iup.Ihandle {
		return iup.Toggle("").SetAttributes(`IMAGE=activeimg, IMPRESS=impressimg, VALUE=ON`)
	})

	rt1 := iup.Toggle("Option A").SetAttribute("VALUE", "ON")
	rt2 := iup.Toggle("Option B")
	rr1 := iup.Toggle("Option A").SetAttribute("VALUE", "ON")
	rr2 := iup.Toggle("Option B")
	rows = append(rows, iup.Label("Radio").SetAttributes(`ALIGNMENT=ALEFT:ACENTER`),
		iup.Radio(iup.Vbox(rt1, rt2)), iup.Radio(iup.Vbox(rr1, rr2)))
	testLeaves = append(testLeaves, rt1, rt2)
	rr1.SetAttribute("ACTIVE", "NO")
	rr2.SetAttribute("ACTIVE", "NO")

	simple("Text", func() iup.Ihandle {
		return iup.Text().SetAttributes(`VALUE="Editable text", VISIBLECOLUMNS=10`)
	})
	simple("Text (multiline)", func() iup.Ihandle {
		t := iup.Text().SetAttributes(`MULTILINE=YES, VISIBLELINES=3, VISIBLECOLUMNS=8`)
		t.SetAttribute("VALUE", "Line 1\nLine 2\nLine 3")
		return t
	})
	simple("Text (spin)", func() iup.Ihandle {
		return iup.Text().SetAttributes(`SPIN=YES, SPINMIN=0, SPINMAX=100, VALUE=42, VISIBLECOLUMNS=8`)
	})
	simple("List (dropdown)", func() iup.Ihandle {
		return iup.List().SetAttributes(`DROPDOWN=YES, 1=Alpha, 2=Beta, 3=Gamma, VALUE=1`)
	})
	simple("List (listbox)", func() iup.Ihandle {
		return iup.List().SetAttributes(`1=One, 2=Two, 3=Three, 4=Four, VALUE=2, VISIBLELINES=3, VISIBLECOLUMNS=8`)
	})
	simple("List (editbox)", func() iup.Ihandle {
		return iup.List().SetAttributes(`EDITBOX=YES, DROPDOWN=YES, 1=Red, 2=Green, 3=Blue, VALUE=1`)
	})
	simple("Val (horizontal)", func() iup.Ihandle {
		return iup.Val("HORIZONTAL").SetAttribute("VALUE", "0.4")
	})
	simple("Dial (horizontal)", func() iup.Ihandle {
		return iup.Dial("HORIZONTAL").SetAttribute("VALUE", "0.3")
	})
	simple("ProgressBar", func() iup.Ihandle {
		return iup.ProgressBar().SetAttribute("VALUE", "0.6")
	})
	simple("Label", func() iup.Ihandle { return iup.Label("A label") })
	simple("Label (image)", func() iup.Ihandle {
		return iup.Label("").SetAttributes(`IMAGE=activeimg`)
	})
	simple("Link", func() iup.Ihandle {
		return iup.Link("https://www.tecgraf.puc-rio.br/iup/", "IUP site")
	})
	simple("DatePick", func() iup.Ihandle { return iup.DatePick() })
	simple("ColorBrowser", func() iup.Ihandle {
		return iup.ColorBrowser()
	})
	treeTest := iup.Tree().SetAttributes(`VISIBLECOLUMNS=8, VISIBLELINES=7`)
	treeRef := iup.Tree().SetAttributes(`VISIBLECOLUMNS=8, VISIBLELINES=7`)
	add("Tree", treeTest, treeRef)
	simple("Table", func() iup.Ihandle {
		t := iup.Table().SetAttributes(`NUMCOL=2, NUMLIN=3, VISIBLELINES=3`)
		t.SetAttribute("TITLE1", "Name")
		t.SetAttribute("TITLE2", "Age")
		data := [][2]string{{"Alice", "30"}, {"Bob", "42"}, {"Eve", "25"}}
		for r, row := range data {
			for c, v := range row {
				t.SetAttribute(fmt.Sprintf("%d:%d", r+1, c+1), v)
			}
		}
		return t
	})
	simple("Tabs", func() iup.Ihandle {
		return iup.Tabs(
			iup.Vbox(iup.Label("Page one")).SetAttribute("TABTITLE", "One"),
			iup.Vbox(iup.Label("Page two")).SetAttribute("TABTITLE", "Two"),
		)
	})
	simple("Frame", func() iup.Ihandle {
		return iup.Frame(iup.Vbox(iup.Label("Framed content"))).SetAttribute("TITLE", "Frame")
	})

	grid := iup.GridBox(rows...).SetAttributes(map[string]string{
		"NUMDIV":      "3",
		"ORIENTATION": "HORIZONTAL",
		"SIZECOL":     "-1",
		"SIZELIN":     "-1",
		"CGAPLIN":     "8",
		"CGAPCOL":     "12",
		"MARGIN":      "10x10",
	})

	scroll := iup.ScrollBox(grid).SetAttribute("EXPAND", "YES")

	stateLbl := iup.Label("Test column: ACTIVE")
	setState := func(on bool) {
		v, s := "YES", "ACTIVE"
		if !on {
			v, s = "NO", "INACTIVE"
		}
		for _, h := range testLeaves {
			h.SetAttribute("ACTIVE", v)
		}
		stateLbl.SetAttribute("TITLE", "Test column: "+s)
	}
	testActive := true

	toolbar := iup.Hbox(
		iup.Button("Toggle").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			testActive = !testActive
			setState(testActive)
			return iup.DEFAULT
		})),
		iup.Button("Set Active").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			testActive = true
			setState(true)
			return iup.DEFAULT
		})),
		iup.Button("Set Inactive").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			testActive = false
			setState(false)
			return iup.DEFAULT
		})),
		iup.Fill(),
		stateLbl,
	).SetAttributes(`MARGIN=10x6, GAP=6, ALIGNMENT=ACENTER`)

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Toggle the Test column and compare each control against the always-inactive Reference column."),
			toolbar,
			scroll,
		).SetAttributes(`MARGIN=8x8, GAP=6`),
	).SetAttributes(`TITLE="ACTIVE attribute", RASTERSIZE=650x550`)

	iup.Show(dlg)

	for _, t := range []iup.Ihandle{treeTest, treeRef} {
		t.SetAttribute("TITLE", "Figures")
		t.SetAttribute("ADDBRANCH0", "3D")
		t.SetAttribute("ADDLEAF1", "Cube")
		t.SetAttribute("ADDLEAF2", "Sphere")
		t.SetAttribute("INSERTBRANCH1", "2D")
		t.SetAttribute("ADDLEAF4", "Triangle")
		t.SetAttribute("ADDLEAF5", "Square")
	}

	iup.MainLoop()
}

func makeImage(r, g, b byte) iup.Ihandle {
	const n = 16
	pixels := make([]byte, n*n*4)
	for y := 0; y < n; y++ {
		for x := 0; x < n; x++ {
			i := (y*n + x) * 4
			dx, dy := float64(x-8)+0.5, float64(y-8)+0.5
			if dx*dx+dy*dy <= 49 {
				pixels[i+0], pixels[i+1], pixels[i+2], pixels[i+3] = r, g, b, 255
			}
		}
	}
	return iup.ImageRGBA(n, n, pixels)
}

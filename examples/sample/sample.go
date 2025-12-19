package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("APPID", "com.example.Sample") // For Wayland/XDG desktop file
	iup.SetGlobal("APPNAME", "Sample")           // For taskbar/dock/WM

	iup.ImageRGBA(32, 32, imgTecgraf).SetHandle("img1")

	img := iup.Image(32, 32, imgBits).SetHandle("img2")
	iup.SetAttribute(img, "0", "0 0 0")
	iup.SetAttribute(img, "1", "0 255 0")
	iup.SetAttribute(img, "2", "BGCOLOR")
	iup.SetAttribute(img, "3", "255 0 0")

	iup.ImageRGBA(16, 16, imgLogoTecgraf).SetHandle("img3")
	iup.ImageRGBA(32, 32, imgGopher).SetHandle("img4")

	iup.Menu(
		iup.Submenu("Submenu 1", iup.Menu(
			iup.Item("Item 1 Checked").SetAttributes("VALUE=ON, AUTOTOGGLE=YES"),
			iup.Item("Item 2 Disabled").SetAttributes("ACTIVE=NO"),
		)),
		iup.Item("Item 3"),
		iup.Item("Item 4"),
	).SetHandle("menu")

	fr1 := iup.Frame(
		iup.Vbox(
			iup.Button("Button Text").SetAttributes("PADDING=5x5"),
			iup.Button("Text").SetAttributes("IMAGE=img1, PADDING=5x5").SetHandle("button2"),
			iup.Hbox(
				iup.Button("").SetAttributes("IMAGE=img1, IMPRESS=img2"),
				iup.Button("").SetAttributes("IMAGE=img1"),
			).SetAttribute("ALIGNMENT", "ACENTER"),
		).SetAttribute("ALIGNMENT", "ACENTER"),
	).SetAttribute("TITLE", "Button")

	iup.GetHandle("button2").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.ColorDlg().SetAttributes(map[string]interface{}{
			"TITLE":          "ColorDlg",
			"VALUE":          "128 0 255",
			"ALPHA":          "142",
			"SHOWHEX":        "YES",
			"SHOWCOLORTABLE": "YES",
		})
		defer dlg.Destroy()

		iup.Popup(dlg, iup.CENTER, iup.CENTER)

		return iup.DEFAULT
	}))

	fr2 := iup.Frame(
		iup.Vbox(
			iup.Label("Label text"),
			iup.Label("Bold Label").SetAttributes(`FONT="Sans, Bold 10"`),
			iup.Label("Colored Label").SetAttributes(`FGCOLOR="0 0 255"`),
			iup.Label("Monospace Font").SetAttributes(`FONT="Monospace, 10"`),
			iup.Label("Highlighted").SetAttributes(`FGCOLOR="50 50 50", BGCOLOR="255 235 200"`),
			iup.Label("").SetAttributes("SEPARATOR=HORIZONTAL"),
			iup.Label("").SetAttributes(`IMAGE=img4, TIP="Label with image"`),
		).SetAttribute("GAP", "5"),
	).SetAttribute("TITLE", "Label")

	fr3 := iup.Frame(
		iup.Vbox(
			iup.Toggle("Toggle Text").SetAttributes("VALUE=ON"),
			iup.Hbox(
				iup.Toggle("").SetAttributes("VALUE=ON, SWITCH=YES"),
				iup.Toggle("").SetAttributes("VALUE=ON, IMAGE=img1"),
			).SetAttribute("ALIGNMENT", "ACENTER"),
			iup.Radio(
				iup.Vbox(
					iup.Toggle("Toggle Text"),
					iup.Toggle("Toggle Text"),
				),
			),
		),
	).SetAttribute("TITLE", "Toggle")

	fr4 := iup.Frame(
		iup.Vbox(
			iup.Text().SetAttributes(`VALUE="Single Line Text", EXPAND=HORIZONTAL`),
			iup.Fill().SetAttributes("SIZE=1"),
			iup.MultiLine().SetAttributes(map[string]string{
				"EXPAND":       "HORIZONTAL",
				"AUTOHIDE":     "NO",
				"SCROLLBAR":    "VERTICAL",
				"VALUE":        "Multiline Text\nSecond Line\nThird Line\nFourth Line\nFifth Line",
				"VISIBLELINES": "4",
			}),
			iup.Vbox(
				iup.Text().SetAttributes(`SPIN=YES, SPINVALUE=32`),
			),
		),
	).SetAttribute("TITLE", "Text")

	list1 := iup.List()
	iup.SetAttribute(list1, "VALUE", "1")
	iup.SetAttribute(list1, "1", "Item 1 Text")
	iup.SetAttribute(list1, "2", "Item 2 Text")
	iup.SetAttribute(list1, "3", "Item 3 Text")
	iup.SetAttribute(list1, "4", "Item 4 Text")
	iup.SetAttribute(list1, "TIP", "List 1")
	iup.SetAttribute(list1, "VISIBLELINES", "3")

	list2 := iup.List()
	iup.SetAttribute(list2, "DROPDOWN", "YES")
	iup.SetAttribute(list2, "VALUE", "3")
	iup.SetAttribute(list2, "1", "Item 1 Text")
	iup.SetAttribute(list2, "2", "Item 2 Text")
	iup.SetAttribute(list2, "3", "Item 3 Text")
	iup.SetAttribute(list2, "4", "Item 4 Text")
	iup.SetAttribute(list2, "TIP", "List 2")
	iup.SetAttribute(list2, "VISIBLEITEMS", "3")

	list3 := iup.List()
	iup.SetAttribute(list3, "EDITBOX", "YES")
	iup.SetAttribute(list3, "VALUE", "Item 2 Text")
	iup.SetAttribute(list3, "1", "Item 1 Text")
	iup.SetAttribute(list3, "2", "Item 2 Text")
	iup.SetAttribute(list3, "3", "Item 3 Text")
	iup.SetAttribute(list3, "4", "Item 4 Text")
	iup.SetAttribute(list3, "TIP", "List 3")
	iup.SetAttribute(list3, "VISIBLELINES", "3")

	fr5 := iup.Frame(
		iup.Vbox(
			list1,
			list2,
			list3,
		),
	).SetAttribute("TITLE", "List")

	hbox1 := iup.Hbox(
		fr1,
		fr2,
		fr3,
		fr4,
		fr5,
	)

	val := iup.Val("").SetAttribute("VALUE", "0.3")

	pbar := iup.ProgressBar().SetAttribute("VALUE", "0.7")

	tabs := iup.Tabs(
		iup.Vbox(iup.Label("")),
		iup.Vbox(iup.Label("")),
		iup.Vbox(iup.Label("")),
	).SetAttributes(map[string]interface{}{
		"TABIMAGE1":    "img3",
		"TABTITLE0":    "Tab Title 0",
		"TABTITLE1":    "Tab Title 1",
		"TABTITLE2":    "Tab Title 2",
		"ALLOWREORDER": "YES",
	})

	table := iup.Table()
	table.SetAttribute("NUMCOL", "4")
	table.SetAttribute("NUMLIN", "10")
	table.SetAttribute("VISIBLELINES", "5")
	table.SetAttribute("FOCUSCELL", "4:2")
	table.SetAttribute("TITLE1", "ID")
	table.SetAttribute("TITLE2", "Name")
	table.SetAttribute("TITLE3", "Age")
	table.SetAttribute("TITLE4", "City")
	table.SetAttribute("ALIGNMENT1", "ACENTER")
	table.SetAttribute("ALIGNMENT2", "ALEFT")
	table.SetAttribute("ALIGNMENT3", "ACENTER")
	table.SetAttribute("ALIGNMENT4", "ALEFT")
	table.SetAttribute("RASTERWIDTH1", "40")
	table.SetAttribute("RASTERWIDTH2", "80")
	table.SetAttribute("RASTERWIDTH3", "50")
	table.SetAttribute("ALTERNATECOLOR", "YES")

	if iup.GetGlobal("DARKMODE") == "YES" && iup.GetGlobal("DRIVER") != "Win32" {
		table.SetAttribute("EVENROWCOLOR", "#3A3A3A")
		table.SetAttribute("ODDROWCOLOR", "#2D2D2D")
	} else {
		table.SetAttribute("EVENROWCOLOR", "#F0F0F0")
		table.SetAttribute("ODDROWCOLOR", "#FFFFFF")
	}

	sampleData := [][]string{
		{"1", "Alice", "25", "NYC"},
		{"2", "Bob", "30", "LA"},
		{"3", "Charlie", "35", "Chicago"},
		{"4", "Diana", "28", "Boston"},
		{"5", "Eve", "32", "Seattle"},
		{"6", "Frank", "41", "Miami"},
		{"7", "Grace", "27", "Denver"},
		{"8", "Henry", "33", "Austin"},
		{"9", "Iris", "29", "Portland"},
		{"10", "Jack", "38", "Phoenix"},
	}

	for lin, row := range sampleData {
		for col, value := range row {
			iup.SetAttributeId2(table, "", lin+1, col+1, value)
		}
	}

	canvas := iup.Canvas().SetAttributes(`BGCOLOR="255 255 255", BORDER=NO, XMIN=0, XMAX=99, POSX=0, DX=10`)
	canvas.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.DrawBegin(ih)
		defer iup.DrawEnd(ih)

		w, h := iup.DrawGetSize(ih)
		cx, cy := w/2, h/2

		ih.SetAttributes(`DRAWCOLOR="204 229 255", DRAWSTYLE=FILL`)
		iup.DrawRectangle(ih, 0, 0, w, h)

		const (
			gopherBlue = "125 211 253"
			colorWhite = "255 255 255"
			colorBlack = "0 0 0"
			textColor  = "0 0 50"
		)

		ih.SetAttributes(`DRAWCOLOR="` + gopherBlue + `", DRAWSTYLE=FILL`)
		iup.DrawArc(ih, cx-60, cy-80, cx-20, cy-40, 0, 360)
		iup.DrawArc(ih, cx+20, cy-80, cx+60, cy-40, 0, 360)

		ih.SetAttributes(`DRAWCOLOR="` + gopherBlue + `", DRAWSTYLE=FILL`)
		iup.DrawArc(ih, cx-80, cy-50, cx+80, cy+70, 0, 360)

		ih.SetAttributes(`DRAWCOLOR="` + colorWhite + `", DRAWSTYLE=FILL`)
		iup.DrawArc(ih, cx-45, cy-30, cx-15, cy+10, 0, 360)
		iup.DrawArc(ih, cx+15, cy-30, cx+45, cy+10, 0, 360)

		ih.SetAttributes(`DRAWCOLOR="` + colorBlack + `", DRAWSTYLE=FILL`)
		iup.DrawArc(ih, cx-33, cy-15, cx-23, cy-5, 0, 360)
		iup.DrawArc(ih, cx+23, cy-15, cx+33, cy-5, 0, 360)

		ih.SetAttributes(`DRAWCOLOR="` + colorBlack + `", DRAWSTYLE=FILL`)
		iup.DrawArc(ih, cx-10, cy+15, cx+10, cy+25, 0, 360)

		ih.SetAttributes(`DRAWCOLOR="` + colorWhite + `", DRAWSTYLE=FILL`)
		iup.DrawRectangle(ih, cx-15, cy+25, cx-1, cy+40)
		iup.DrawRectangle(ih, cx+1, cy+25, cx+15, cy+40)

		ih.SetAttributes(`DRAWCOLOR="` + textColor + `", DRAWFONT="Helvetica, Bold 16"`)
		iup.DrawText(ih, "Hello from IUP-Go!", w/2-80, h/2+40, -1, -1)

		return iup.DEFAULT
	}))

	initTree()

	tree := iup.GetHandle("tree")

	vbox1 := iup.Vbox(
		hbox1,
		iup.Hbox(
			iup.Frame(iup.Hbox(table)).SetAttributes(`TITLE=Table`),
			iup.Vbox(
				iup.Frame(iup.Hbox(tabs)).SetAttributes(`TITLE=Tabs`),
				iup.Hbox(
					iup.Frame(iup.Hbox(val)).SetAttributes(`TITLE=Val`),
					iup.Fill(),
					iup.Frame(iup.Hbox(pbar)).SetAttributes(`TITLE=ProgressBar`),
				).SetAttribute("GAP", "5"),
			).SetAttribute("GAP", "5"),
		).SetAttribute("GAP", "5"),
		iup.Hbox(
			iup.Frame(iup.Hbox(canvas)).SetAttributes(`TITLE=Canvas`),
			iup.Frame(iup.Hbox(tree)).SetAttributes(`TITLE=Tree`),
		).SetAttribute("GAP", "5"),
	).SetAttributes("MARGIN=5x5, GAP=5").SetHandle("vbox1")

	dlg := iup.Dialog(vbox1).SetAttributes(`TITLE="Sample", MENU=menu, ICON=img1`)
	dlg.SetHandle("dlg")

	iup.Show(dlg)

	initTreeAttributes()

	iup.MainLoop()
}

func initTree() {
	tree := iup.Tree().SetHandle("tree")
	iup.SetAttribute(tree, "RASTERSIZE", "x150")
	iup.SetAttribute(tree, "SHOWRENAME", "YES")
}

func initTreeAttributes() {
	tree := iup.GetHandle("tree")

	tree.SetAttributes(map[string]string{
		"TITLE0":     "Figures",
		"ADDLEAF0":   "Other",
		"ADDBRANCH1": "triangle",
		"ADDLEAF2":   "equilateral",
		"ADDLEAF3":   "isoceles",
		"ADDLEAF4":   "scalenus",
	})
}

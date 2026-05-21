package main

import (
	"fmt"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	setupImages()
	setupMenu()
	setupDrawer()
	requestNotifyPermission()

	iup.Notify().SetAttributes(`STYLE=TOAST, TIMEOUT=2000`).SetHandle("status")

	tab1 := iup.Vbox(
		buildButtons(),
		iup.Hbox(buildLabels(), buildToggles()).SetAttribute("GAP", "8"),
		buildText(),
		iup.Hbox(buildLists(), buildTree()).SetAttribute("GAP", "8"),
	).SetAttributes(`TABTITLE="Controls", NMARGIN=10x10, NGAP=10`)

	tab2 := iup.Vbox(
		buildTable(),
		buildValProgress(),
		buildDialogButtons(),
		buildCanvas(),
		buildClipboardNotify(),
	).SetAttributes(`TABTITLE="Data", NMARGIN=10x10, NGAP=10`)

	dlg := iup.Dialog(
		iup.Tabs(tab1, tab2),
	).SetAttributes(`TITLE="Mobile Sample", MENU=menu, DRAWER=drawer, ICON=img1`)

	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(ih iup.Ihandle, darkMode int) int {
		iup.Update(iup.GetHandle("canvas"))
		return iup.DEFAULT
	}))

	iup.Show(dlg)

	tree := iup.GetHandle("tree")
	iup.SetAttributeId(tree, "ADDBRANCH", -1, "2D")
	iup.SetAttribute(tree, "INSERTBRANCH0", "3D")
	iup.SetAttribute(tree, "ADDLEAF0", "triangle")
	iup.SetAttribute(tree, "ADDLEAF1", "square")
	iup.SetAttribute(tree, "ADDLEAF3", "sphere")
	iup.SetAttribute(tree, "ADDLEAF4", "cube")
	iup.SetAttribute(tree, "STATE0", "COLLAPSED")

	iup.MainLoop()
}

func setStatus(msg string) {
	status := iup.GetHandle("status")
	status.SetAttribute("CLOSE", "YES")
	status.SetAttribute("BODY", msg)
	status.SetAttribute("SHOW", "YES")
}

// Images --------------------------------------------------------------

func setupImages() {
	iup.Image(16, 16, imgDiamond).SetAttributes(map[string]string{
		"0": "BGCOLOR",
		"1": "40 40 120",
		"2": "80 100 200",
		"3": "180 210 255",
	}).SetHandle("img1")

	iup.Image(16, 16, imgDiamond).SetAttributes(map[string]string{
		"0": "BGCOLOR",
		"1": "40 110 40",
		"2": "80 180 80",
		"3": "210 255 180",
	}).SetHandle("img2")
}

var imgDiamond = []byte{
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 2, 3, 3, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, 0, 0,
	0, 0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 1, 0, 0,
	0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 0,
	1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1,
	1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1,
	0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 0,
	0, 0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 2, 3, 3, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
}

// Menu ----------------------------------------------------------------

func setupMenu() {
	fileMenu := iup.Menu(
		iup.MenuItem("New").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Menu: New"); return iup.DEFAULT })),
		iup.MenuItem("Open").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Menu: Open"); return iup.DEFAULT })),
		iup.MenuItem("Save").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Menu: Save"); return iup.DEFAULT })),
		iup.Separator(),
		iup.MenuItem("Exit").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { return iup.CLOSE })),
	)

	editMenu := iup.Menu(
		iup.MenuItem("Cut").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Menu: Cut"); return iup.DEFAULT })),
		iup.MenuItem("Copy").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Menu: Copy"); return iup.DEFAULT })),
		iup.MenuItem("Paste").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Menu: Paste"); return iup.DEFAULT })),
	)

	iup.Menu(
		iup.Submenu("File", fileMenu),
		iup.Submenu("Edit", editMenu),
	).SetHandle("menu")
}

// Drawer --------------------------------------------------------------

func setupDrawer() {
	navItem := func(title, image string) iup.Ihandle {
		mi := iup.MenuItem(title).SetCallback("ACTION",
			iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Drawer: " + title); return iup.DEFAULT }))
		if image != "" {
			mi.SetAttribute("IMAGE", image)
		}
		return mi
	}

	controls := iup.Menu(
		navItem("Buttons", "img1"),
		navItem("Labels", ""),
		navItem("Toggles", "img2"),
		navItem("Text", ""),
		navItem("Lists", "img1"),
		navItem("Tree", ""),
		navItem("Canvas", "img2"),
	)

	settings := iup.Menu(
		navItem("Preferences", "img1"),
		navItem("Theme", "img2"),
		navItem("Language", ""),
	)

	iup.Menu(
		navItem("Home", "img1"),
		iup.Submenu("Controls", controls).SetAttributes(`IMAGE=img2, EXPANDABLE=YES, STATE=OPEN`),
		iup.Submenu("Settings", settings).SetAttributes(`IMAGE=img1, EXPANDABLE=YES`),
		navItem("About", "img2"),
	).SetHandle("drawer")
}

func requestNotifyPermission() {
	sys := iup.GetGlobal("SYSTEM")
	if !strings.HasPrefix(sys, "iOS") && !strings.HasPrefix(sys, "macOS") {
		return
	}
	n := iup.Notify()
	defer iup.Destroy(n)
	if n.GetAttribute("PERMISSION") == "NOTDETERMINED" {
		n.SetAttribute("REQUESTPERMISSION", "YES")
	}
}

// Buttons -------------------------------------------------------------

func buildButtons() iup.Ihandle {
	btnText := iup.Button("Text").SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Button Text pressed"); return iup.DEFAULT }))

	btnImg := iup.Button("").SetAttributes(`IMAGE=img1`).SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Image button pressed"); return iup.DEFAULT }))

	btnImgText := iup.Button("Go").SetAttributes(`IMAGE=img2`).SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Image+Text pressed"); return iup.DEFAULT }))

	btnBorderless := iup.Button("").SetAttributes(`IMAGE=img1, IMPRESS=img2`).SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Borderless pressed"); return iup.DEFAULT }))

	return iup.Frame(
		iup.Hbox(btnText, btnImg, btnBorderless, btnImgText).SetAttributes(`ALIGNMENT=ACENTER, GAP=8, MARGIN=8x8`),
	).SetAttributes(`TITLE=Buttons, EXPAND=HORIZONTAL`)
}

// Labels --------------------------------------------------------------

func buildLabels() iup.Ihandle {
	return iup.Frame(
		iup.Vbox(
			iup.Label("Plain label").SetAttribute("SELECTABLE", "YES"),
			iup.Label("Bold label").SetAttribute("FONTSTYLE", "Bold"),
			iup.Label("Colored label").SetAttribute("FGCOLOR", "40 80 200"),
			iup.Label("Monospace label").SetAttribute("FONTFACE", "Courier"),
			iup.Label("Highlighted").SetAttributes(`BGCOLOR="255 230 150", FGCOLOR="150 60 20"`),
			iup.Label("").SetAttribute("SEPARATOR", "HORIZONTAL"),
			iup.Label("Separator above."),
		).SetAttributes(`GAP=4, MARGIN=8x8`),
	).SetAttribute("EXPAND", "HORIZONTAL").SetAttribute("TITLE", "Labels")
}

// Toggles -------------------------------------------------------------

func buildToggles() iup.Ihandle {
	chk := iup.Toggle("Check").SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int {
			setStatus(fmt.Sprintf("Checkbox=%s", ih.GetAttribute("VALUE")))
			return iup.DEFAULT
		}))

	sw := iup.Toggle("").SetAttributes(`SWITCH=YES, VALUE=ON`)
	sw.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		setStatus(fmt.Sprintf("Switch=%s", ih.GetAttribute("VALUE")))
		return iup.DEFAULT
	}))

	imgToggle := iup.Toggle("").SetAttributes(`IMAGE=img1, VALUE=ON`)
	imgToggle.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		setStatus(fmt.Sprintf("ImageToggle=%s", ih.GetAttribute("VALUE")))
		return iup.DEFAULT
	}))

	radio := iup.Radio(
		iup.Vbox(
			iup.Toggle("Red").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Radio: Red"); return iup.DEFAULT })),
			iup.Toggle("Green").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Radio: Green"); return iup.DEFAULT })),
			iup.Toggle("Blue").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { setStatus("Radio: Blue"); return iup.DEFAULT })),
		).SetAttributes(`GAP=2`),
	)

	return iup.Frame(
		iup.Vbox(
			iup.Hbox(chk, sw).SetAttributes(`GAP=12, ALIGNMENT=ACENTER`),
			iup.Hbox(radio, imgToggle).SetAttributes(`GAP=12, ALIGNMENT=ACENTER`),
		).SetAttributes(`GAP=8, NMARGIN=8x8`),
	).SetAttribute("TITLE", "Toggles")
}

// Text ----------------------------------------------------------------

func buildText() iup.Ihandle {
	single := iup.Text().SetAttributes(`EXPAND=HORIZONTAL, VALUE="Single line"`)
	pwd := iup.Text().SetAttributes(`EXPAND=HORIZONTAL, PASSWORD=YES, VALUE="secret"`)

	spin := iup.Text().SetAttributes(`SPIN=YES, SPINMIN=0, SPINMAX=100, VALUE=42, EXPAND=HORIZONTAL`)
	spin.SetCallback("VALUECHANGED_CB",
		iup.ValueChangedFunc(func(ih iup.Ihandle) int {
			setStatus("Spin=" + ih.GetAttribute("VALUE"))
			return iup.DEFAULT
		}))

	multi := iup.Text().SetAttributes(`MULTILINE=YES, VISIBLELINES=3, EXPAND=HORIZONTAL`)
	multi.SetAttribute("VALUE", "Line 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6")

	return iup.Frame(
		iup.Vbox(single, pwd, spin, multi).SetAttributes(`GAP=6, MARGIN=8x8`),
	).SetAttributes(`TITLE=Text, EXPAND=HORIZONTAL`)
}

// Lists ---------------------------------------------------------------

func buildLists() iup.Ihandle {
	simple := iup.List().SetAttributes(`1=Alpha, 2=Beta, 3=Gamma, 4=Delta, 5=Epsilon, 6=Zeta, 7=Eta, 8=Theta, VISIBLELINES=3, EXPAND=HORIZONTAL`)
	simple.SetCallback("VALUECHANGED_CB",
		iup.ValueChangedFunc(func(ih iup.Ihandle) int {
			setStatus("List=" + ih.GetAttribute("VALUE"))
			return iup.DEFAULT
		}))

	drop := iup.List().SetAttributes(`DROPDOWN=YES, 1=One, 2=Two, 3=Three, VALUE=2, EXPAND=HORIZONTAL`)
	drop.SetCallback("VALUECHANGED_CB",
		iup.ValueChangedFunc(func(ih iup.Ihandle) int {
			setStatus("Dropdown=" + ih.GetAttribute("VALUE"))
			return iup.DEFAULT
		}))

	return iup.Frame(
		iup.Vbox(simple, drop).SetAttributes(`GAP=6, MARGIN=8x8`),
	).SetAttributes(`TITLE=Lists, EXPAND=HORIZONTAL`)
}

// Tree ----------------------------------------------------------------

func buildTree() iup.Ihandle {
	tree := iup.Tree().SetAttributes(`RASTERSIZE=150x150, EXPAND=YES, ADDROOT=NO`).SetHandle("tree")
	tree.SetCallback("SELECTION_CB",
		iup.SelectionFunc(func(ih iup.Ihandle, id, status int) int {
			if status == 1 {
				setStatus(fmt.Sprintf("Tree: %s", ih.GetAttribute(fmt.Sprintf("TITLE%d", id))))
			}
			return iup.DEFAULT
		}))

	return iup.Frame(
		iup.Vbox(tree).SetAttributes(`MARGIN=8x8`),
	).SetAttributes(`TITLE=Tree, EXPAND=HORIZONTAL`)
}

// Table ---------------------------------------------------------------

func buildTable() iup.Ihandle {
	table := iup.Table().SetAttributes(`EXPAND=HORIZONTAL, NUMCOL=3, NUMLIN=10, VISIBLELINES=3, FOCUSCELL=2:1`)

	headers := []string{"Name", "Age", "City"}
	for i, h := range headers {
		table.SetAttribute(fmt.Sprintf("TITLE%d", i+1), h)
	}

	rows := [][3]string{
		{"Alice", "30", "NYC"},
		{"Bob", "42", "LA"},
		{"Charlie", "35", "Chicago"},
		{"Diana", "28", "Boston"},
		{"Eve", "50", "Seattle"},
		{"Frank", "37", "Denver"},
		{"Grace", "25", "Dallas"},
		{"Henry", "45", "Miami"},
		{"Ivy", "33", "Austin"},
		{"Jack", "29", "Portland"},
	}
	for r, row := range rows {
		for c, val := range row {
			table.SetAttribute(fmt.Sprintf("%d:%d", r+1, c+1), val)
		}
	}

	table.SetCallback("VALUECHANGED_CB", iup.TableValueChangedFunc(func(ih iup.Ihandle, lin, col int) int {
		setStatus(fmt.Sprintf("Table changed: %d,%d", lin, col))
		return iup.DEFAULT
	}))

	return iup.Frame(
		iup.Vbox(table).SetAttributes(`MARGIN=8x8`),
	).SetAttributes(`TITLE=Table, EXPAND=HORIZONTAL`)
}

// Val + ProgressBar ---------------------------------------------------

func buildValProgress() iup.Ihandle {
	progress := iup.ProgressBar().SetAttributes(`EXPAND=HORIZONTAL, MIN=0, MAX=100, VALUE=30`)

	val := iup.Val("HORIZONTAL").SetAttributes(`MIN=0, MAX=100, VALUE=30, EXPAND=HORIZONTAL`)
	val.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		v := fmt.Sprintf("%.1f", ih.GetFloat("VALUE"))
		progress.SetAttribute("VALUE", v)
		setStatus("Val=" + v)
		return iup.DEFAULT
	}))

	return iup.Frame(
		iup.Vbox(
			iup.Label("Slider:"), val,
			iup.Label("Progress:"), progress,
		).SetAttributes(`GAP=6, MARGIN=8x8`),
	).SetAttributes(`TITLE="Val + Progress", EXPAND=HORIZONTAL`)
}

// Canvas --------------------------------------------------------------

func buildCanvas() iup.Ihandle {
	canvas := iup.Canvas().SetAttributes(`RASTERSIZE=220x84, BORDER=NO`)
	iup.SetHandle("canvas", canvas)
	canvas.SetCallback("ACTION", iup.ActionFunc(canvasDraw))
	canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(func(ih iup.Ihandle, button, pressed, x, y int, status string) int {
		if pressed != 0 {
			setStatus(fmt.Sprintf("Canvas tap @ %d,%d", x, y))
		}
		return iup.DEFAULT
	}))
	return iup.Frame(
		iup.Vbox(canvas).SetAttributes(`MARGIN=8x8`),
	).SetAttributes(`TITLE=Canvas, EXPAND=HORIZONTAL`)
}

func canvasDraw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)

	dark := iup.GetGlobal("DARKMODE") == "YES"
	base, baseline, title := "230 230 235", "120 120 140", "60 60 100"
	if dark {
		base, baseline, title = "60 60 70", "180 180 200", "220 220 240"
	}

	_ = base
	colors := []string{"230 60 60", "230 150 60", "230 220 60", "60 180 80", "60 140 220"}
	vals := []float64{0.35, 0.55, 0.75, 0.90, 0.65}
	barW := (w - 30) / len(vals)
	baseY := h - 20
	for i, v := range vals {
		x1 := 15 + i*barW
		x2 := x1 + barW - 6
		y1 := baseY - int(float64(baseY-20)*v)
		ih.SetAttributes(`DRAWSTYLE=FILL, DRAWCOLOR="` + colors[i] + `"`)
		iup.DrawRectangle(ih, x1, y1, x2, baseY)
	}

	ih.SetAttributes(`DRAWCOLOR="` + baseline + `", DRAWSTYLE=STROKE, LINEWIDTH=2`)
	iup.DrawLine(ih, 10, baseY, w-10, baseY)

	ih.SetAttributes(`DRAWCOLOR="` + title + `", DRAWFONT="Arial, Bold 11"`)
	iup.DrawText(ih, "IupDraw", 8, 6, w/2, 18)
	return iup.DEFAULT
}

// Dialog buttons ------------------------------------------------------

func buildDialogButtons() iup.Ihandle {
	btn := func(title string, fn func()) iup.Ihandle {
		return iup.Button(title).SetCallback("ACTION",
			iup.ActionFunc(func(ih iup.Ihandle) int { fn(); return iup.DEFAULT }))
	}

	return iup.Frame(
		iup.Vbox(
			iup.Hbox(
				btn("File", func() {
					d := iup.FileDlg().SetAttributes(`DIALOGTYPE=OPEN, TITLE="Open"`)
					defer d.Destroy()
					iup.Popup(d, iup.CENTER, iup.CENTER)
					setStatus("File: " + d.GetAttribute("VALUE"))
				}),
				btn("Message", func() {
					d := iup.MessageDlg().SetAttributes(`VALUE="Hello from IUP!", BUTTONS=OKCANCEL`)
					defer d.Destroy()
					iup.Popup(d, iup.CENTER, iup.CENTER)
					setStatus("MessageDlg: " + d.GetAttribute("BUTTONRESPONSE"))
				}),
				btn("Color", func() {
					d := iup.ColorDlg().SetAttributes(map[string]interface{}{
						"TITLE":          "ColorDlg",
						"VALUE":          "128 0 255",
						"ALPHA":          "142",
						"SHOWALPHA":      "YES",
						"SHOWCOLORTABLE": "YES",
						"SHOWCOMPACT":    "YES",
					})
					defer d.Destroy()
					iup.Popup(d, iup.CENTER, iup.CENTER)
					setStatus("Color: " + d.GetAttribute("VALUE"))
				}),
			).SetAttributes(`GAP=6, ALIGNMENT=ACENTER`),
			iup.Hbox(
				btn("Font", func() {
					d := iup.FontDlg()
					defer d.Destroy()
					iup.Popup(d, iup.CENTER, iup.CENTER)
					setStatus("Font: " + d.GetAttribute("VALUE"))
				}),
				btn("Date", func() {
					dp := iup.DatePick().SetAttribute("VALUE", "2026/04/22")
					dlg := iup.Dialog(iup.Vbox(iup.Hbox(dp, iup.Fill())).SetAttribute("NMARGIN", "20x20")).SetAttribute("TITLE", "Pick Date")
					defer dlg.Destroy()
					iup.Popup(dlg, iup.CENTER, iup.CENTER)
					setStatus("Date: " + dp.GetAttribute("VALUE"))
				}),
				btn("Help", func() {
					iup.Help("https://github.com/gen2brain/iup-go")
					setStatus("Help opened")
				}),
			).SetAttributes(`GAP=6, ALIGNMENT=ACENTER`),
		).SetAttributes(`GAP=6, NMARGIN=8x8, ALIGNMENT=ACENTER`),
	).SetAttributes(`TITLE=Dialogs, EXPAND=HORIZONTAL`)
}

// Clipboard + Notify --------------------------------------------------

func buildClipboardNotify() iup.Ihandle {
	entry := iup.Text().SetAttributes(`EXPAND=HORIZONTAL, VALUE="Edit me and copy/paste"`)

	btnCopy := iup.Button("Copy").SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int {
			clip := iup.Clipboard()
			defer clip.Destroy()
			clip.SetAttribute("TEXT", entry.GetAttribute("VALUE"))
			setStatus("Copied to clipboard")
			return iup.DEFAULT
		}))

	btnPaste := iup.Button("Paste").SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int {
			clip := iup.Clipboard()
			defer clip.Destroy()
			entry.SetAttribute("VALUE", clip.GetAttribute("TEXT"))
			setStatus("Pasted from clipboard")
			return iup.DEFAULT
		}))

	btnNotify := iup.Button("Notify").SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int {
			n := iup.Notify()
			n.SetAttributes(`TITLE="IUP-Go", BODY="Notification from Mobile Sample"`)
			n.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, msg string) int {
				setStatus("Notify error: " + msg)
				return iup.DEFAULT
			}))
			n.SetAttribute("SHOW", "YES")
			setStatus("Notification posted")
			return iup.DEFAULT
		}))

	btnToast := iup.Button("Toast").SetCallback("ACTION",
		iup.ActionFunc(func(ih iup.Ihandle) int {
			n := iup.Notify()
			n.SetAttributes(`STYLE=TOAST, TITLE="IUP-Go", BODY="Toast from Mobile Sample", ICON=img2`)
			n.SetAttribute("SHOW", "YES")
			return iup.DEFAULT
		}))

	return iup.Frame(
		iup.Vbox(
			entry,
			iup.Hbox(btnCopy, btnPaste, btnNotify, btnToast).SetAttributes(`GAP=8, ALIGNMENT=ACENTER`),
		).SetAttributes(`GAP=6, NMARGIN=8x8`),
	).SetAttributes(`TITLE="Clipboard + Notify", EXPAND=HORIZONTAL`)
}

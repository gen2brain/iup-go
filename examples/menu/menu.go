package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	setupImages()

	iup.Menu(
		iup.Submenu("File",
			iup.Menu(
				iup.MenuItem("New").SetAttribute("IMAGE", "imgNew").SetCallback("ACTION", iup.ActionFunc(itemCb)).SetCallback("HIGHLIGHT_CB", iup.HighlightFunc(highlightCb)),
				iup.MenuItem("Open").SetAttribute("IMAGE", "imgOpen").SetCallback("ACTION", iup.ActionFunc(itemCb)).SetCallback("HIGHLIGHT_CB", iup.HighlightFunc(highlightCb)),
				iup.MenuItem("Close").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuSeparator(),
				iup.MenuItem("Page Setup").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Print").SetAttribute("IMAGE", "imgPrint").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuSeparator(),
				iup.MenuItem("Exit").SetCallback("ACTION", iup.ActionFunc(itemExitCb)),
			).SetCallback("OPEN_CB", iup.MenuOpenFunc(menuOpenCb)).
				SetCallback("MENUCLOSE_CB", iup.MenuCloseFunc(menuCloseCb)),
		),
		iup.Submenu("Edit",
			iup.Menu(
				iup.MenuItem("Copy").SetAttribute("IMAGE", "imgCopy").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Paste").SetAttribute("IMAGE", "imgPaste").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuSeparator(),
				iup.MenuItem("Bookmark").
					SetAttribute("IMAGE", "imgBookmarkOff").
					SetAttribute("IMPRESS", "imgBookmarkOn").
					SetAttribute("HIDEMARK", "YES").
					SetCallback("ACTION", iup.ActionFunc(itemBookmarkCb)),
				iup.MenuSeparator(),
				iup.Submenu("Create",
					iup.Menu(
						iup.MenuItem("Line").SetCallback("ACTION", iup.ActionFunc(itemCb)),
						iup.MenuItem("Circle").SetCallback("ACTION", iup.ActionFunc(itemCb)),
						iup.Submenu("Triangle",
							iup.Menu(
								iup.MenuItem("Equilateral").SetCallback("ACTION", iup.ActionFunc(itemCb)),
								iup.MenuItem("Isosceles").SetCallback("ACTION", iup.ActionFunc(itemCb)),
								iup.MenuItem("Scalene").SetCallback("ACTION", iup.ActionFunc(itemCb)),
							),
						),
					),
				).SetAttribute("IMAGE", "imgNew"),
			),
		),
		iup.Submenu("View",
			iup.Menu(
				iup.MenuItem("Small").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Medium").SetAttribute("VALUE", "ON").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Large").SetCallback("ACTION", iup.ActionFunc(itemCb)),
			).SetAttribute("RADIO", "YES"),
		),
		iup.Submenu("Help",
			iup.Menu(
				iup.MenuItem("Help").SetAttribute("IMAGE", "imgHelp").SetCallback("ACTION", iup.ActionFunc(itemHelpCb)).SetCallback("HIGHLIGHT_CB", iup.HighlightFunc(highlightCb)),
			),
		),
	).SetHandle("menu")

	dlg := iup.Dialog(
		iup.Canvas(),
	).SetAttributes(map[string]string{
		"TITLE": "Menu",
		"SIZE":  "QUARTERxQUARTER",
		"MENU":  "menu",
	})

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func itemCb(ih iup.Ihandle) int {
	fmt.Printf("Item %q selected\n", ih.GetAttribute("TITLE"))
	return iup.DEFAULT
}

func itemHelpCb(ih iup.Ihandle) int {
	iup.Message("Help", "Only Help and Exit items perform an operation")
	return iup.DEFAULT
}

func itemExitCb(ih iup.Ihandle) int {
	return iup.CLOSE
}

func itemBookmarkCb(ih iup.Ihandle) int {
	if ih.GetAttribute("VALUE") == "ON" {
		ih.SetAttribute("VALUE", "OFF")
	} else {
		ih.SetAttribute("VALUE", "ON")
	}
	fmt.Printf("Bookmark: %s\n", ih.GetAttribute("VALUE"))
	return iup.DEFAULT
}

func menuOpenCb(ih iup.Ihandle) int {
	fmt.Println("Menu OPEN_CB")
	return iup.DEFAULT
}

func menuCloseCb(ih iup.Ihandle) int {
	fmt.Println("Menu MENUCLOSE_CB")
	return iup.DEFAULT
}

func highlightCb(ih iup.Ihandle) int {
	fmt.Printf("Highlight %q\n", ih.GetAttribute("TITLE"))
	return iup.DEFAULT
}

// 16x16 two-color icons (pixels use palette indices 0..3; 0 = BGCOLOR).
func setupImages() {
	mk := func(name string, px []byte, c1, c2, c3 string) {
		iup.Image(16, 16, px).SetAttributes(map[string]string{
			"0": "BGCOLOR",
			"1": c1,
			"2": c2,
			"3": c3,
		}).SetHandle(name)
	}
	mk("imgNew", imgPage, "80 80 80", "200 200 200", "255 255 255")
	mk("imgOpen", imgFolder, "120 100 40", "220 190 100", "255 240 180")
	mk("imgPrint", imgPage, "40 40 80", "90 120 200", "200 220 255")
	mk("imgCopy", imgPage, "60 120 60", "120 200 120", "210 255 210")
	mk("imgPaste", imgFolder, "120 60 60", "210 120 120", "255 220 220")
	mk("imgHelp", imgPage, "80 60 160", "180 140 220", "230 210 255")
	mk("imgBookmarkOff", imgPage, "150 150 150", "200 200 200", "240 240 240")
	mk("imgBookmarkOn", imgPage, "180 130 30", "240 200 80", "255 230 130")
}

var imgPage = []byte{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
	0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0, 0, 0,
	0, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0,
	0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
}

var imgFolder = []byte{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0,
	0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 0,
	0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 3, 2, 3, 3, 3, 2, 3, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 3, 2, 3, 3, 3, 2, 3, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 1, 0, 0,
	0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0, 0,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
}

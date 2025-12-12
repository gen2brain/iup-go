package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

const maxRecent = 10

func recentMenuCallback(ih iup.Ihandle) int {
	filename := ih.GetAttribute("RECENTFILENAME")
	fmt.Printf("Recent file selected (menu): %s\n", filename)
	iup.GetHandle("statusLabel").SetAttribute("TITLE", "Opened: "+filename)
	return iup.DEFAULT
}

func recentListCallback(ih iup.Ihandle) int {
	filename := ih.GetAttribute("RECENTFILENAME")
	fmt.Printf("Recent file selected (list): %s\n", filename)
	iup.GetHandle("statusLabel").SetAttribute("TITLE", "Opened: "+filename)
	return iup.DEFAULT
}

func openFile(ih iup.Ihandle) int {
	dlg := iup.FileDlg()
	dlg.SetAttribute("DIALOGTYPE", "OPEN")
	dlg.SetAttribute("TITLE", "Open File")

	iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)

	if dlg.GetInt("STATUS") >= 0 {
		filename := dlg.GetAttribute("VALUE")
		fmt.Printf("Opened: %s\n", filename)

		config := iup.GetHandle("config")
		iup.ConfigRecentUpdate(config, filename)
		iup.ConfigSave(config)

		iup.GetHandle("statusLabel").SetAttribute("TITLE", "Opened: "+filename)
	}

	dlg.Destroy()
	return iup.DEFAULT
}

func clearRecent(ih iup.Ihandle) int {
	config := iup.GetHandle("config")
	for i := 1; i <= maxRecent; i++ {
		iup.ConfigSetVariableStrId(config, "Recent", "File", i, "")
	}
	iup.ConfigSave(config)

	iup.ConfigRecentInit(config, iup.GetHandle("recentMenu"), recentMenuCallback, maxRecent)
	iup.ConfigRecentInit(config, iup.GetHandle("recentList"), recentListCallback, maxRecent)

	iup.GetHandle("statusLabel").SetAttribute("TITLE", "Recent files cleared")
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	config := iup.Config()
	config.SetAttribute("APP_NAME", "RecentFilesTest")
	config.SetHandle("config")
	iup.ConfigLoad(config)

	recentMenu := iup.Menu()
	recentMenu.SetHandle("recentMenu")

	itemOpen := iup.Item("Open...")
	itemOpen.SetCallback("ACTION", iup.ActionFunc(openFile))

	itemClearRecent := iup.Item("Clear Recent Files")
	itemClearRecent.SetCallback("ACTION", iup.ActionFunc(clearRecent))

	itemExit := iup.Item("Exit")
	itemExit.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		return iup.CLOSE
	}))

	fileMenu := iup.Menu(
		itemOpen,
		iup.Submenu("Recent Files", recentMenu),
		itemClearRecent,
		iup.Separator(),
		itemExit,
	)

	menu := iup.Menu(iup.Submenu("File", fileMenu))
	menu.SetHandle("mainmenu")

	iup.ConfigRecentInit(config, recentMenu, recentMenuCallback, maxRecent)

	recentList := iup.List()
	recentList.SetAttributes(map[string]string{
		"EXPAND":       "YES",
		"VISIBLELINES": "5",
	})
	recentList.SetHandle("recentList")
	iup.ConfigRecentInit(config, recentList, recentListCallback, maxRecent)

	statusLabel := iup.Label("Open files to add them to the Recent Files menu and list")
	statusLabel.SetAttribute("EXPAND", "HORIZONTAL")
	statusLabel.SetHandle("statusLabel")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Recent Files (List):"),
			recentList,
			statusLabel,
		).SetAttribute("MARGIN", "10x10").SetAttribute("GAP", "5"),
	).SetAttributes(map[string]string{
		"TITLE": "Recent Files",
		"MENU":  "mainmenu",
		"SIZE":  "300x200",
	})

	iup.Show(dlg)
	iup.MainLoop()

	dlg.Destroy()
	config.Destroy()
}

//go:build web

package main

import (
	"fmt"
	"runtime"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

// historyCallback displays browser history (non-Windows only)
func historyCallback(ih iup.Ihandle) int {
	back := ih.GetInt("BACKCOUNT")
	fwrd := ih.GetInt("FORWARDCOUNT")

	fmt.Println("HISTORY ITEMS")
	for i := -back; i < 0; i++ {
		attr := fmt.Sprintf("ITEMHISTORY%d", i)
		fmt.Printf("Backward %02d: %s\n", i, ih.GetAttribute(attr))
	}

	fmt.Printf("Current  %02d: %s\n", 0, ih.GetAttribute("ITEMHISTORY0"))

	for i := 1; i <= fwrd; i++ {
		attr := fmt.Sprintf("ITEMHISTORY%d", i)
		fmt.Printf("Forward  %02d: %s\n", i, ih.GetAttribute(attr))
	}

	return iup.DEFAULT
}

// navigateCallback is called when navigating to a URL
func navigateCallback(ih iup.Ihandle, url string) int {
	fmt.Printf("NAVIGATE_CB: %s\n", url)
	if strings.Contains(url, "download") {
		return iup.IGNORE
	}
	return iup.DEFAULT
}

// errorCallback is called when an error occurs
func errorCallback(ih iup.Ihandle, url string) int {
	fmt.Printf("ERROR_CB: %s\n", url)
	return iup.DEFAULT
}

// completedCallback is called when a page load completes
func completedCallback(ih iup.Ihandle, url string) int {
	fmt.Printf("COMPLETED_CB: %s\n", url)
	return iup.DEFAULT
}

// newWindowCallback is called when a new window is requested
func newWindowCallback(ih iup.Ihandle, url string) int {
	fmt.Printf("NEWWINDOW_CB: %s\n", url)
	ih.SetAttribute("VALUE", url)
	return iup.DEFAULT
}

// backCallback handles the Back button
func backCallback(ih iup.Ihandle) int {
	web := ih.GetAttribute("MY_WEB")
	webHandle := iup.GetHandle(web)
	webHandle.SetAttribute("BACKFORWARD", "-1")
	// fmt.Printf("zoom=%s\n", webHandle.GetAttribute("ZOOM"))
	return iup.DEFAULT
}

// forwardCallback handles the Forward button
func forwardCallback(ih iup.Ihandle) int {
	web := ih.GetAttribute("MY_WEB")
	webHandle := iup.GetHandle(web)
	webHandle.SetAttribute("BACKFORWARD", "1")
	// webHandle.SetAttribute("ZOOM", "200")
	return iup.DEFAULT
}

// stopCallback handles the Stop button
func stopCallback(ih iup.Ihandle) int {
	web := ih.GetAttribute("MY_WEB")
	webHandle := iup.GetHandle(web)
	webHandle.SetAttribute("STOP", "")
	return iup.DEFAULT
}

// reloadCallback handles the Reload button
func reloadCallback(ih iup.Ihandle) int {
	web := ih.GetAttribute("MY_WEB")
	webHandle := iup.GetHandle(web)
	webHandle.SetAttribute("RELOAD", "")

	// fmt.Printf("STATUS=%s\n", webHandle.GetAttribute("STATUS"))
	return iup.DEFAULT
}

// loadCallback handles the Load button
func loadCallback(ih iup.Ihandle) int {
	txt := ih.GetAttribute("MY_TEXT")
	web := ih.GetAttribute("MY_WEB")
	txtHandle := iup.GetHandle(txt)
	webHandle := iup.GetHandle(web)
	webHandle.SetAttribute("VALUE", txtHandle.GetAttribute("VALUE"))

	// txtHandle.SetAttribute("VALUE", webHandle.GetAttribute("VALUE"))
	// webHandle.SetAttribute("HTML", "<html><body><b>Hello</b>, World!</body></html>")
	// webHandle.SetAttribute("VALUE", "http://www.microsoft.com")

	return iup.DEFAULT
}

func webBrowserTest() {
	iup.WebBrowserOpen()

	if iup.GetGlobal("IUP_WEBBROWSER_MISSING_LIB") != "" {
		fmt.Println("cannot find", iup.GetGlobal("IUP_WEBBROWSER_MISSING_LIB"))
	}

	// Create web browser control
	web := iup.WebBrowser()

	// Create UI elements
	btBack := iup.Button("Back")
	btForward := iup.Button("Forward")
	txt := iup.Text()
	btLoad := iup.Button("Load").SetHandle("btLoad")
	btReload := iup.Button("Reload")
	btStop := iup.Button("Stop")

	// Create horizontal box with controls
	var hbox iup.Ihandle
	if runtime.GOOS != "windows" {
		history := iup.Button("History")
		hbox = iup.Hbox(btBack, btForward, txt, btLoad, btReload, btStop, history)
		history.SetCallback("ACTION", historyCallback)
	} else {
		hbox = iup.Hbox(btBack, btForward, txt, btLoad, btReload, btStop)
	}

	// Create dialog with vertical layout
	vbox := iup.Vbox(hbox, web)
	dlg := iup.Dialog(vbox)

	// Set dialog attributes
	dlg.SetAttribute("TITLE", "WebBrowser")
	dlg.SetAttribute("RASTERSIZE", "800x600")
	dlg.SetAttribute("MARGIN", "10x10")
	dlg.SetAttribute("GAP", "10")

	// Store references to web and text controls
	iup.SetHandle("MY_WEB_HANDLE", web)
	iup.SetHandle("MY_TEXT_HANDLE", txt)
	dlg.SetAttribute("MY_WEB", "MY_WEB_HANDLE")
	dlg.SetAttribute("MY_TEXT", "MY_TEXT_HANDLE")
	btBack.SetAttribute("MY_WEB", "MY_WEB_HANDLE")
	btForward.SetAttribute("MY_WEB", "MY_WEB_HANDLE")
	btStop.SetAttribute("MY_WEB", "MY_WEB_HANDLE")
	btReload.SetAttribute("MY_WEB", "MY_WEB_HANDLE")
	btLoad.SetAttribute("MY_TEXT", "MY_TEXT_HANDLE")
	btLoad.SetAttribute("MY_WEB", "MY_WEB_HANDLE")

	// Set initial URL
	//web.SetAttribute("HTML", "<html><body><b>Hello</b>World!</body></html>")
	txt.SetAttribute("VALUE", "http://www.tecgraf.puc-rio.br/iup")
	web.SetAttribute("VALUE", txt.GetAttribute("VALUE"))
	dlg.SetAttribute("DEFAULTENTER", "btLoad")

	// Configure text field
	txt.SetAttribute("EXPAND", "HORIZONTAL")

	// Set button callbacks
	btLoad.SetCallback("ACTION", iup.ActionFunc(loadCallback))
	btReload.SetCallback("ACTION", iup.ActionFunc(reloadCallback))
	btBack.SetCallback("ACTION", iup.ActionFunc(backCallback))
	btForward.SetCallback("ACTION", iup.ActionFunc(forwardCallback))
	btStop.SetCallback("ACTION", iup.ActionFunc(stopCallback))

	// Set web browser callbacks
	web.SetCallback("NEWWINDOW_CB", iup.NewWindowFunc(newWindowCallback))
	web.SetCallback("NAVIGATE_CB", iup.NavigateFunc(navigateCallback))
	web.SetCallback("ERROR_CB", iup.ErrorFunc(errorCallback))
	web.SetCallback("COMPLETED_CB", iup.CompletedFunc(completedCallback))

	// Show dialog
	iup.Show(dlg)
}

func main() {
	iup.Open()
	defer iup.Close()

	webBrowserTest()

	iup.MainLoop()
}

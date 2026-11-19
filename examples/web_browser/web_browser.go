//go:build web

package main

import (
	"fmt"
	"net/url"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

const homePage = "https://en.wikipedia.org/wiki/Main_Page"

var dlg, tabs, txt iup.Ihandle

func currentWeb() iup.Ihandle {
	return iup.Ihandle(tabs.GetPtr("VALUE_HANDLE"))
}

func pageTitle(web iup.Ihandle, addr string) string {
	web.SetAttribute("JAVASCRIPT", "document.title")
	if title := strings.TrimSpace(web.GetAttribute("JAVASCRIPT")); title != "" {
		return title
	}
	if u, err := url.Parse(addr); err == nil && u.Host != "" {
		return u.Host
	}
	return "New Tab"
}

func shortTitle(title string) string {
	r := []rune(title)
	if len(r) > 24 {
		return string(r[:23]) + "…"
	}
	return title
}

func updateBar(web iup.Ihandle) {
	txt.SetAttribute("VALUE", web.GetAttribute("VALUE"))
	dlg.SetAttribute("TITLE", web.GetAttribute("PAGETITLE")+" - WebBrowser")
}

func historyCallback(ih iup.Ihandle) int {
	web := currentWeb()
	back := web.GetInt("BACKCOUNT")
	fwrd := web.GetInt("FORWARDCOUNT")

	fmt.Println("HISTORY ITEMS")
	for i := -back; i < 0; i++ {
		fmt.Printf("Backward %02d: %s\n", i, web.GetAttribute(fmt.Sprintf("ITEMHISTORY%d", i)))
	}
	fmt.Printf("Current  %02d: %s\n", 0, web.GetAttribute("ITEMHISTORY0"))
	for i := 1; i <= fwrd; i++ {
		fmt.Printf("Forward  %02d: %s\n", i, web.GetAttribute(fmt.Sprintf("ITEMHISTORY%d", i)))
	}
	return iup.DEFAULT
}

func navigateCallback(ih iup.Ihandle, addr string) int {
	fmt.Printf("NAVIGATE_CB: %s\n", addr)
	if strings.Contains(addr, "download") {
		return iup.IGNORE
	}
	return iup.DEFAULT
}

func errorCallback(ih iup.Ihandle, addr string) int {
	fmt.Printf("ERROR_CB: %s\n", addr)
	return iup.DEFAULT
}

func completedCallback(ih iup.Ihandle, addr string) int {
	fmt.Printf("COMPLETED_CB: %s\n", addr)
	title := pageTitle(ih, addr)
	ih.SetAttribute("PAGETITLE", title)
	pos := iup.GetChildPos(tabs, ih)
	tabs.SetAttribute(fmt.Sprintf("TABTITLE%d", pos), shortTitle(title))
	tabs.SetAttribute(fmt.Sprintf("TABTIP%d", pos), title)
	if ih == currentWeb() {
		updateBar(ih)
	}
	return iup.DEFAULT
}

func newWindowCallback(ih iup.Ihandle, addr string) int {
	fmt.Printf("NEWWINDOW_CB: %s\n", addr)
	openTab(addr)
	return iup.DEFAULT
}

func newWeb(addr string) iup.Ihandle {
	web := iup.WebBrowser()
	web.SetAttribute("TABTITLE", "New Tab")
	web.SetAttribute("PAGETITLE", "New Tab")
	web.SetAttribute("VALUE", addr)

	iup.On(web, iup.NavigateCB, navigateCallback)
	iup.On(web, iup.ErrorCB, errorCallback)
	iup.On(web, iup.CompletedCB, completedCallback)
	iup.On(web, iup.NewWindowCB, newWindowCallback)
	return web
}

func openTab(addr string) {
	web := newWeb(addr)
	iup.Append(tabs, web)
	iup.Map(web)
	iup.Refresh(tabs)
	tabs.SetAttribute("VALUE_HANDLE", web)
	updateBar(web)
}

func webBrowserTest() {
	iup.WebBrowserOpen()

	if iup.GetGlobal("IUP_WEBBROWSER_MISSING_LIB") != "" {
		fmt.Println("cannot find", iup.GetGlobal("IUP_WEBBROWSER_MISSING_LIB"))
	}

	btBack := iup.Button("Back")
	btForward := iup.Button("Forward")
	btReload := iup.Button("Reload")
	btStop := iup.Button("Stop")
	btNewTab := iup.Button("New Tab")
	btLoad := iup.Button("Load").SetHandle("btLoad")
	txt = iup.Text().SetAttributes(`EXPAND=HORIZONTAL, VALUE="` + homePage + `"`)

	tabs = iup.Tabs(newWeb(homePage)).SetAttributes("SHOWCLOSE=YES, ALLOWREORDER=YES")

	var rows iup.Ihandle
	switch iup.GetGlobal("DRIVER") {
	case "Android", "CocoaTouch":
		rows = iup.Vbox(
			iup.Hbox(btBack, btForward, btReload),
			iup.Hbox(btStop, btNewTab),
			iup.Hbox(txt, btLoad),
		)
	case "WASM", "Haiku":
		rows = iup.Vbox(
			iup.Hbox(btBack, btForward, btReload, btStop),
			iup.Hbox(txt, btLoad, btNewTab),
		)
	default:
		history := iup.Button("History")
		iup.On(history, iup.ActionCB, historyCallback)
		rows = iup.Vbox(
			iup.Hbox(btBack, btForward, btReload, btStop, history),
			iup.Hbox(txt, btLoad, btNewTab),
		)
	}

	rows.SetAttribute("MARGIN", "0x0")

	dlg = iup.Dialog(iup.Vbox(rows, tabs))
	dlg.SetAttributes(map[string]string{
		"TITLE":        "WebBrowser",
		"RASTERSIZE":   "800x600",
		"MARGIN":       "10x10",
		"GAP":          "10",
		"DEFAULTENTER": "btLoad",
	})

	iup.On(btBack, iup.ActionCB, func(ih iup.Ihandle) int {
		currentWeb().SetAttribute("BACKFORWARD", "-1")
		return iup.DEFAULT
	})
	iup.On(btForward, iup.ActionCB, func(ih iup.Ihandle) int {
		currentWeb().SetAttribute("BACKFORWARD", "1")
		return iup.DEFAULT
	})
	iup.On(btReload, iup.ActionCB, func(ih iup.Ihandle) int {
		currentWeb().SetAttribute("RELOAD", "")
		return iup.DEFAULT
	})
	iup.On(btStop, iup.ActionCB, func(ih iup.Ihandle) int {
		currentWeb().SetAttribute("STOP", "")
		return iup.DEFAULT
	})
	iup.On(btLoad, iup.ActionCB, func(ih iup.Ihandle) int {
		currentWeb().SetAttribute("VALUE", txt.GetAttribute("VALUE"))
		return iup.DEFAULT
	})
	iup.On(btNewTab, iup.ActionCB, func(ih iup.Ihandle) int {
		openTab(homePage)
		return iup.DEFAULT
	})

	iup.On(tabs, iup.TabChangeCB, func(ih, newTab, oldTab iup.Ihandle) int {
		updateBar(newTab)
		return iup.DEFAULT
	})
	iup.On(tabs, iup.TabCloseCB, func(ih iup.Ihandle, pos int) int {
		if ih.GetInt("COUNT") == 1 {
			iup.ExitLoop()
			return iup.IGNORE
		}
		iup.Destroy(iup.GetChild(ih, pos))
		iup.Refresh(ih)
		updateBar(currentWeb())
		return iup.IGNORE
	})

	iup.Show(dlg)
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	webBrowserTest()

	iup.MainLoop()
}

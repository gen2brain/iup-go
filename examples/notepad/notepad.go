// Features: file open/save with recent files, dirty tracking, find/replace,
// go to line, font selection, toggleable toolbar and status bar, drag and drop,
// and persistent window/placement preferences via IupConfig.
package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

const (
	appName   = "iup-notepad"
	maxRecent = 10
	appTitle  = "Notepad"
)

var (
	config    iup.Ihandle
	dlg       iup.Ihandle
	multitext iup.Ihandle
	toolbar   iup.Ihandle
	statusbar iup.Ihandle
	findDlg   iup.Ihandle

	filename string
)

func init() { iup.EntryPoint(main) }

func main() {
	if len(os.Args) > 1 && (os.Args[1] == "-h" || os.Args[1] == "--help") {
		fmt.Println("Usage: notepad [file]")
		return
	}

	iup.Open()
	defer iup.Close()

	iup.SetGlobal("APPNAME", appTitle)

	config = iup.Config()
	config.SetAttribute("APP_NAME", appName)
	iup.ConfigLoad(config)

	buildMainDialog()

	iup.ConfigDialogShow(config, dlg, "MainWindow")

	if len(os.Args) > 1 {
		loadFile(os.Args[1])
	} else {
		newFile()
	}

	iup.MainLoop()
}

// buildMainDialog constructs the main window and wires all callbacks.
func buildMainDialog() {
	multitext = iup.Text()
	multitext.SetAttributes(map[string]string{
		"MULTILINE":       "YES",
		"EXPAND":          "YES",
		"SCROLLBAR":       "YES",
		"WORDWRAP":        "NO",
		"DROPFILESTARGET": "YES",
	})
	multitext.SetCallback("CARET_CB", iup.CaretFunc(caretCb))
	multitext.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(valueChangedCb))
	multitext.SetCallback("DROPFILES_CB", iup.DropFilesFunc(dropFilesCb))

	if font := iup.ConfigGetVariableStr(config, "MainWindow", "Font"); font != "" {
		multitext.SetAttribute("FONT", font)
	}

	statusbar = iup.Label("Lin 1, Col 1")
	statusbar.SetAttributes("EXPAND=HORIZONTAL, PADDING=10x5")

	toolbar = buildToolbar()

	mainMenu := buildMenu()

	vbox := iup.Vbox(toolbar, multitext, statusbar)

	dlg = iup.Dialog(vbox).SetAttributes(map[string]string{
		"SIZE":   "HALFxHALF",
		"SHRINK": "YES",
	})
	iup.SetAttributeHandle(dlg, "MENU", mainMenu)
	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(closeCb))
	dlg.SetCallback("DROPFILES_CB", iup.DropFilesFunc(dropFilesCb))
	dlg.SetCallback("K_cN", iup.ActionFunc(newActionCb))
	dlg.SetCallback("K_cO", iup.ActionFunc(openActionCb))
	dlg.SetCallback("K_cS", iup.ActionFunc(saveActionCb))
	dlg.SetCallback("K_cF", iup.ActionFunc(findActionCb))
	dlg.SetCallback("K_cH", iup.ActionFunc(replaceActionCb))
	dlg.SetCallback("K_cG", iup.ActionFunc(gotoActionCb))
	dlg.SetCallback("K_F3", iup.ActionFunc(findNextCb))

	iup.SetAttributeHandle(iup.Ihandle(0), "PARENTDIALOG", dlg)

	if !configBool("Toolbar", true) {
		toolbar.SetAttributes("FLOATING=YES, VISIBLE=NO")
	}
	if !configBool("Statusbar", true) {
		statusbar.SetAttributes("FLOATING=YES, VISIBLE=NO")
	}
}

// buildMenu creates the complete menu bar with all items.
func buildMenu() iup.Ihandle {
	itemNew := iup.MenuItem("&New\tCtrl+N").SetCallback("ACTION", iup.ActionFunc(newActionCb))
	itemOpen := iup.MenuItem("&Open...\tCtrl+O").SetCallback("ACTION", iup.ActionFunc(openActionCb))

	itemSave := iup.MenuItem("&Save\tCtrl+S").SetCallback("ACTION", iup.ActionFunc(saveActionCb))
	itemSave.SetHandle("itemSave")

	itemSaveAs := iup.MenuItem("Save &As...").SetCallback("ACTION", iup.ActionFunc(saveAsActionCb))

	itemRevert := iup.MenuItem("&Revert").SetCallback("ACTION", iup.ActionFunc(revertActionCb))
	itemRevert.SetHandle("itemRevert")

	itemExit := iup.MenuItem("E&xit").SetCallback("ACTION", iup.ActionFunc(exitActionCb))

	recentMenu := iup.Menu().SetHandle("recentMenu")
	iup.ConfigRecentInit(config, recentMenu, recentCb, maxRecent)

	fileMenu := iup.Menu(
		itemNew,
		itemOpen,
		itemSave,
		itemSaveAs,
		itemRevert,
		iup.MenuSeparator(),
		iup.Submenu("Recent &Files", recentMenu),
		iup.MenuSeparator(),
		itemExit,
	).SetCallback("MENUOPEN_CB", iup.MenuOpenFunc(fileMenuOpenCb))

	itemCut := iup.MenuItem("Cu&t\tCtrl+X").SetCallback("ACTION", iup.ActionFunc(cutActionCb))
	itemCut.SetHandle("itemCut")
	itemCopy := iup.MenuItem("&Copy\tCtrl+C").SetCallback("ACTION", iup.ActionFunc(copyActionCb))
	itemCopy.SetHandle("itemCopy")
	itemPaste := iup.MenuItem("&Paste\tCtrl+V").SetCallback("ACTION", iup.ActionFunc(pasteActionCb))
	itemPaste.SetHandle("itemPaste")
	itemDelete := iup.MenuItem("&Delete\tDel").SetCallback("ACTION", iup.ActionFunc(deleteActionCb))
	itemDelete.SetHandle("itemDelete")
	itemSelectAll := iup.MenuItem("Select &All\tCtrl+A").SetCallback("ACTION", iup.ActionFunc(selectAllActionCb))
	itemFind := iup.MenuItem("&Find...\tCtrl+F").SetCallback("ACTION", iup.ActionFunc(findActionCb))
	itemFindNext := iup.MenuItem("Find &Next\tF3").SetCallback("ACTION", iup.ActionFunc(findNextCb))
	itemReplace := iup.MenuItem("&Replace...\tCtrl+H").SetCallback("ACTION", iup.ActionFunc(replaceActionCb))
	itemGoTo := iup.MenuItem("&Go To Line...\tCtrl+G").SetCallback("ACTION", iup.ActionFunc(gotoActionCb))

	editMenu := iup.Menu(
		itemCut,
		itemCopy,
		itemPaste,
		itemDelete,
		iup.MenuSeparator(),
		itemFind,
		itemFindNext,
		itemReplace,
		itemGoTo,
		iup.MenuSeparator(),
		itemSelectAll,
	).SetCallback("MENUOPEN_CB", iup.MenuOpenFunc(editMenuOpenCb))

	itemFont := iup.MenuItem("&Font...").SetCallback("ACTION", iup.ActionFunc(fontActionCb))
	formatMenu := iup.Menu(itemFont)

	itemToolbar := iup.MenuItem("&Toolbar").SetCallback("ACTION", iup.ActionFunc(toolbarActionCb))
	itemToolbar.SetAttribute("AUTOTOGGLE", "YES")
	if configBool("Toolbar", true) {
		itemToolbar.SetAttribute("VALUE", "ON")
	}

	itemStatusbar := iup.MenuItem("&Status Bar").SetCallback("ACTION", iup.ActionFunc(statusbarActionCb))
	itemStatusbar.SetAttribute("AUTOTOGGLE", "YES")
	if configBool("Statusbar", true) {
		itemStatusbar.SetAttribute("VALUE", "ON")
	}

	viewMenu := iup.Menu(itemToolbar, itemStatusbar)

	itemAbout := iup.MenuItem("&About...").SetCallback("ACTION", iup.ActionFunc(aboutActionCb))
	helpMenu := iup.Menu(itemAbout)

	return iup.Menu(
		iup.Submenu("&File", fileMenu),
		iup.Submenu("&Edit", editMenu),
		iup.Submenu("F&ormat", formatMenu),
		iup.Submenu("&View", viewMenu),
		iup.Submenu("&Help", helpMenu),
	)
}

// buildToolbar creates a horizontal button bar above the editor.
func buildToolbar() iup.Ihandle {
	btn := func(text, tip string, cb iup.ActionFunc) iup.Ihandle {
		b := iup.Button(text)
		b.SetAttribute("TIP", tip)
		b.SetAttribute("PADDING", "6x3")
		b.SetAttribute("CANFOCUS", "NO")
		b.SetCallback("ACTION", cb)
		return b
	}

	sep := func() iup.Ihandle {
		return iup.Label("").SetAttribute("SEPARATOR", "VERTICAL")
	}

	hb := iup.Hbox(
		btn("New", "New file (Ctrl+N)", newActionCb),
		btn("Open", "Open file (Ctrl+O)", openActionCb),
		btn("Save", "Save file (Ctrl+S)", saveActionCb),
		sep(),
		btn("Cut", "Cut (Ctrl+X)", cutActionCb),
		btn("Copy", "Copy (Ctrl+C)", copyActionCb),
		btn("Paste", "Paste (Ctrl+V)", pasteActionCb),
		sep(),
		btn("Find", "Find (Ctrl+F)", findActionCb),
		iup.Fill(),
	)
	hb.SetAttributes("MARGIN=5x3, GAP=3, ALIGNMENT=ACENTER")
	return hb
}

// ---------- File operations ----------

// newFile resets the editor to an untitled empty buffer.
func newFile() {
	filename = ""
	multitext.SetAttribute("VALUE", "")
	multitext.SetAttribute("DIRTY", "NO")
	updateTitle()
}

// loadFile reads a file from the disk and replaces the buffer contents.
func loadFile(path string) {
	data, err := os.ReadFile(path)
	if err != nil {
		iup.Message("Error", fmt.Sprintf("Can't open file: %s\n%v", path, err))
		return
	}

	filename = path
	multitext.SetAttribute("VALUE", string(data))
	multitext.SetAttribute("DIRTY", "NO")
	updateTitle()

	iup.ConfigRecentUpdate(config, filename)
	iup.ConfigSetVariableStr(config, "MainWindow", "LastDirectory", filepath.Dir(filename))
}

// saveFile writes the buffer contents to the given path.
func saveFile(path string) bool {
	if err := os.WriteFile(path, []byte(multitext.GetAttribute("VALUE")), 0644); err != nil {
		iup.Message("Error", fmt.Sprintf("Can't save file: %s\n%v", path, err))
		return false
	}
	filename = path
	multitext.SetAttribute("DIRTY", "NO")
	updateTitle()
	iup.ConfigRecentUpdate(config, filename)
	iup.ConfigSetVariableStr(config, "MainWindow", "LastDirectory", filepath.Dir(filename))
	return true
}

// saveCheck asks the user what to do about unsaved changes. Returns false to abort.
func saveCheck() bool {
	if multitext.GetInt("DIRTY") == 0 {
		return true
	}
	switch iup.Alarm(appTitle, "File not saved. Save changes now?", "Yes", "No", "Cancel") {
	case 1:
		return saveCurrent()
	case 2:
		return true
	default:
		return false
	}
}

// saveCurrent saves the current filename or prompts for one. Returns false on cancel/failure.
func saveCurrent() bool {
	if filename == "" {
		return promptSaveAs()
	}
	return saveFile(filename)
}

// promptSaveAs shows a Save As file dialog.
func promptSaveAs() bool {
	fdlg := iup.FileDlg()
	defer fdlg.Destroy()
	fdlg.SetAttributes(map[string]string{
		"DIALOGTYPE": "SAVE",
		"EXTFILTER":  "Text Files|*.txt|All Files|*.*|",
		"DIRECTORY":  iup.ConfigGetVariableStr(config, "MainWindow", "LastDirectory"),
		"FILE":       filename,
	})
	iup.SetAttributeHandle(fdlg, "PARENTDIALOG", dlg)
	iup.Popup(fdlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if fdlg.GetInt("STATUS") == -1 {
		return false
	}
	return saveFile(fdlg.GetAttribute("VALUE"))
}

// updateTitle refreshes the dialog title to reflect the current file.
func updateTitle() {
	name := "Untitled"
	if filename != "" {
		name = filepath.Base(filename)
	}
	dlg.SetAttribute("TITLE", name+" - "+appTitle)
}

// ---------- Menu callbacks ----------

func newActionCb(ih iup.Ihandle) int {
	if saveCheck() {
		newFile()
	}
	return iup.DEFAULT
}

func openActionCb(ih iup.Ihandle) int {
	if !saveCheck() {
		return iup.DEFAULT
	}
	fdlg := iup.FileDlg()
	defer fdlg.Destroy()
	fdlg.SetAttributes(map[string]string{
		"DIALOGTYPE": "OPEN",
		"EXTFILTER":  "Text Files|*.txt|All Files|*.*|",
		"DIRECTORY":  iup.ConfigGetVariableStr(config, "MainWindow", "LastDirectory"),
	})
	iup.SetAttributeHandle(fdlg, "PARENTDIALOG", dlg)
	iup.Popup(fdlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if fdlg.GetInt("STATUS") != -1 {
		loadFile(fdlg.GetAttribute("VALUE"))
	}
	return iup.DEFAULT
}

func saveActionCb(ih iup.Ihandle) int {
	if filename == "" {
		promptSaveAs()
	} else if multitext.GetInt("DIRTY") != 0 {
		saveFile(filename)
	}
	return iup.DEFAULT
}

func saveAsActionCb(ih iup.Ihandle) int {
	promptSaveAs()
	return iup.DEFAULT
}

func revertActionCb(ih iup.Ihandle) int {
	if filename == "" {
		return iup.DEFAULT
	}
	if iup.Alarm(appTitle, "Discard changes and revert to saved file?", "Yes", "No", "") == 1 {
		loadFile(filename)
	}
	return iup.DEFAULT
}

func exitActionCb(ih iup.Ihandle) int {
	if !saveCheck() {
		return iup.IGNORE
	}
	iup.ConfigDialogClosed(config, dlg, "MainWindow")
	iup.ConfigSave(config)
	config.Destroy()
	return iup.CLOSE
}

func closeCb(ih iup.Ihandle) int {
	return exitActionCb(ih)
}

func recentCb(ih iup.Ihandle) int {
	if saveCheck() {
		loadFile(ih.GetAttribute("RECENTFILENAME"))
	}
	return iup.DEFAULT
}

func cutActionCb(ih iup.Ihandle) int {
	cb := iup.Clipboard()
	defer cb.Destroy()
	cb.SetAttribute("TEXT", multitext.GetAttribute("SELECTEDTEXT"))
	multitext.SetAttribute("SELECTEDTEXT", "")
	return iup.DEFAULT
}

func copyActionCb(ih iup.Ihandle) int {
	cb := iup.Clipboard()
	defer cb.Destroy()
	cb.SetAttribute("TEXT", multitext.GetAttribute("SELECTEDTEXT"))
	return iup.DEFAULT
}

func pasteActionCb(ih iup.Ihandle) int {
	cb := iup.Clipboard()
	defer cb.Destroy()
	if cb.GetAttribute("TEXTAVAILABLE") == "YES" {
		multitext.SetAttribute("INSERT", cb.GetAttribute("TEXT"))
	}
	return iup.DEFAULT
}

func deleteActionCb(ih iup.Ihandle) int {
	multitext.SetAttribute("SELECTEDTEXT", "")
	return iup.DEFAULT
}

func selectAllActionCb(ih iup.Ihandle) int {
	iup.SetFocus(multitext)
	multitext.SetAttribute("SELECTION", "ALL")
	return iup.DEFAULT
}

func fontActionCb(ih iup.Ihandle) int {
	fontDlg := iup.FontDlg()
	defer fontDlg.Destroy()
	fontDlg.SetAttribute("VALUE", multitext.GetAttribute("FONT"))
	iup.SetAttributeHandle(fontDlg, "PARENTDIALOG", dlg)
	iup.Popup(fontDlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if fontDlg.GetInt("STATUS") == 1 {
		font := fontDlg.GetAttribute("VALUE")
		multitext.SetAttribute("FONT", font)
		iup.ConfigSetVariableStr(config, "MainWindow", "Font", font)
	}
	return iup.DEFAULT
}

func toolbarActionCb(ih iup.Ihandle) int {
	toggleBar(toolbar, ih, "Toolbar")
	return iup.DEFAULT
}

func statusbarActionCb(ih iup.Ihandle) int {
	toggleBar(statusbar, ih, "Statusbar")
	return iup.DEFAULT
}

// toggleBar hides/shows a bar and mirrors the state into the config.
func toggleBar(bar, item iup.Ihandle, key string) {
	on := item.GetInt("VALUE")
	if on != 0 {
		bar.SetAttributes("FLOATING=NO, VISIBLE=YES")
	} else {
		bar.SetAttributes("FLOATING=YES, VISIBLE=NO")
	}
	iup.Refresh(bar)
	iup.ConfigSetVariableInt(config, "MainWindow", key, on)
}

func aboutActionCb(ih iup.Ihandle) int {
	btnOk := iup.Button("OK").SetAttribute("PADDING", "15x3")
	btnOk.SetCallback("ACTION", iup.ActionFunc(func(b iup.Ihandle) int { return iup.CLOSE }))

	btnInfo := iup.Button("System Info...").SetAttribute("PADDING", "15x3")
	btnInfo.SetCallback("ACTION", iup.ActionFunc(func(b iup.Ihandle) int {
		iup.VersionShow()
		return iup.DEFAULT
	}))

	about := iup.Dialog(iup.Vbox(
		iup.Label(fmt.Sprintf("IUP-Go Notepad\n\nIUP version: %s\nSimple text editor example built with iup-go.", iup.Version())),
		iup.Hbox(iup.Fill(), btnInfo, btnOk).SetAttributes("NORMALIZESIZE=HORIZONTAL, GAP=5"),
	).SetAttributes("MARGIN=15x15, GAP=15"))
	about.SetAttributes(map[string]string{"TITLE": "About", "DIALOGFRAME": "YES"})
	iup.SetAttributeHandle(about, "PARENTDIALOG", dlg)
	iup.SetAttributeHandle(about, "DEFAULTENTER", btnOk)
	iup.SetAttributeHandle(about, "DEFAULTESC", btnOk)

	iup.Popup(about, iup.CENTERPARENT, iup.CENTERPARENT)
	about.Destroy()
	return iup.DEFAULT
}

// ---------- Find / Replace ----------

func findActionCb(ih iup.Ihandle) int {
	showFindDialog(false)
	return iup.DEFAULT
}

func replaceActionCb(ih iup.Ihandle) int {
	showFindDialog(true)
	return iup.IGNORE
}

// showFindDialog creates (once) and displays the find/replace dialog.
// showReplace controls whether the Replace row is visible.
func showFindDialog(showReplace bool) {
	if findDlg == 0 {
		findDlg = createFindDialog()
	}
	setReplaceVisible(showReplace)
	if sel := multitext.GetAttribute("SELECTEDTEXT"); sel != "" && !strings.Contains(sel, "\n") {
		iup.GetHandle("findText").SetAttribute("VALUE", sel)
	}
	iup.ConfigDialogShow(config, findDlg, "FindDialog")
}

// createFindDialog builds the modeless find/replace dialog.
func createFindDialog() iup.Ihandle {
	findText := iup.Text().SetAttributes("VISIBLECOLUMNS=24, EXPAND=HORIZONTAL")
	findText.SetHandle("findText")

	replaceText := iup.Text().SetAttributes("VISIBLECOLUMNS=24, EXPAND=HORIZONTAL")
	replaceText.SetHandle("replaceText")

	caseToggle := iup.Toggle("Case Sensitive")
	caseToggle.SetHandle("findCase")

	btnNext := iup.Button("Find Next").SetAttribute("PADDING", "10x2")
	btnNext.SetCallback("ACTION", iup.ActionFunc(findNextCb))

	btnReplace := iup.Button("Replace").SetAttribute("PADDING", "10x2")
	btnReplace.SetCallback("ACTION", iup.ActionFunc(replaceOnceCb))
	btnReplace.SetHandle("replaceBtn")

	btnReplaceAll := iup.Button("Replace All").SetAttribute("PADDING", "10x2")
	btnReplaceAll.SetCallback("ACTION", iup.ActionFunc(replaceAllCb))
	btnReplaceAll.SetHandle("replaceAllBtn")

	btnClose := iup.Button("Close").SetAttribute("PADDING", "10x2")
	btnClose.SetCallback("ACTION", iup.ActionFunc(findCloseCb))

	lblReplace := iup.Label("Replace with:")
	lblReplace.SetHandle("replaceLabel")

	box := iup.Vbox(
		iup.Label("Find what:"),
		findText,
		lblReplace,
		replaceText,
		caseToggle,
		iup.Hbox(
			iup.Fill(),
			btnNext,
			btnReplace,
			btnReplaceAll,
			btnClose,
		).SetAttributes("NORMALIZESIZE=HORIZONTAL, GAP=5"),
	).SetAttributes("MARGIN=10x10, GAP=5")

	d := iup.Dialog(box)
	d.SetAttributes("TITLE=Find, DIALOGFRAME=YES")
	iup.SetAttributeHandle(d, "DEFAULTENTER", btnNext)
	iup.SetAttributeHandle(d, "DEFAULTESC", btnClose)
	iup.SetAttributeHandle(d, "PARENTDIALOG", dlg)
	d.SetCallback("CLOSE_CB", iup.CloseFunc(findCloseCb))
	return d
}

// setReplaceVisible toggles the "Replace with" row for the combined dialog.
func setReplaceVisible(show bool) {
	replaceText := iup.GetHandle("replaceText")
	replaceLbl := iup.GetHandle("replaceLabel")
	replaceBtn := iup.GetHandle("replaceBtn")
	replaceAll := iup.GetHandle("replaceAllBtn")

	vis := "NO"
	floating := "YES"
	title := "Find"
	if show {
		vis = "YES"
		floating = "NO"
		title = "Find and Replace"
	}
	for _, w := range []iup.Ihandle{replaceText, replaceLbl, replaceBtn, replaceAll} {
		w.SetAttributes("FLOATING=" + floating + ", VISIBLE=" + vis)
	}
	findDlg.SetAttribute("TITLE", title)
	findDlg.SetAttribute("SIZE", "")
	iup.Refresh(findDlg)
}

func findNextCb(ih iup.Ihandle) int {
	if findDlg == 0 {
		showFindDialog(false)
		return iup.DEFAULT
	}
	needle := iup.GetHandle("findText").GetAttribute("VALUE")
	if needle == "" {
		return iup.DEFAULT
	}
	caseSensitive := iup.GetHandle("findCase").GetInt("VALUE") != 0
	haystack := multitext.GetAttribute("VALUE")

	start := multitext.GetInt("CARETPOS")
	pos := indexOf(haystack[start:], needle, caseSensitive)
	if pos >= 0 {
		pos += start
	} else if start > 0 {
		pos = indexOf(haystack, needle, caseSensitive)
	}

	if pos < 0 {
		iup.Message("Find", "Text not found.")
		return iup.DEFAULT
	}

	end := pos + len(needle)
	iup.SetFocus(multitext)
	multitext.SetAttribute("SELECTIONPOS", fmt.Sprintf("%d:%d", pos, end))
	multitext.SetAttribute("CARETPOS", end)
	lin, _ := iup.TextConvertPosToLinCol(multitext, pos)
	linePos := iup.TextConvertLinColToPos(multitext, lin, 0)
	multitext.SetAttribute("SCROLLTOPOS", linePos)
	return iup.DEFAULT
}

func replaceOnceCb(ih iup.Ihandle) int {
	sel := multitext.GetAttribute("SELECTEDTEXT")
	needle := iup.GetHandle("findText").GetAttribute("VALUE")
	caseSensitive := iup.GetHandle("findCase").GetInt("VALUE") != 0

	if sel != "" && equal(sel, needle, caseSensitive) {
		multitext.SetAttribute("SELECTEDTEXT", iup.GetHandle("replaceText").GetAttribute("VALUE"))
	}
	return findNextCb(ih)
}

func replaceAllCb(ih iup.Ihandle) int {
	needle := iup.GetHandle("findText").GetAttribute("VALUE")
	if needle == "" {
		return iup.DEFAULT
	}
	replacement := iup.GetHandle("replaceText").GetAttribute("VALUE")
	caseSensitive := iup.GetHandle("findCase").GetInt("VALUE") != 0

	src := multitext.GetAttribute("VALUE")
	var out strings.Builder
	count := 0
	for {
		pos := indexOf(src, needle, caseSensitive)
		if pos < 0 {
			out.WriteString(src)
			break
		}
		out.WriteString(src[:pos])
		out.WriteString(replacement)
		src = src[pos+len(needle):]
		count++
	}
	if count == 0 {
		iup.Message("Replace", "Text not found.")
		return iup.DEFAULT
	}
	multitext.SetAttribute("VALUE", out.String())
	multitext.SetAttribute("DIRTY", "YES")
	iup.Message("Replace", fmt.Sprintf("Replaced %d occurrence(s).", count))
	return iup.DEFAULT
}

func findCloseCb(ih iup.Ihandle) int {
	iup.ConfigDialogClosed(config, findDlg, "FindDialog")
	iup.Hide(findDlg)
	return iup.DEFAULT
}

// ---------- Go To Line ----------

func gotoActionCb(ih iup.Ihandle) int {
	lineCount := multitext.GetInt("LINECOUNT")

	txt := iup.Text().SetAttributes(map[string]string{
		"MASK":           iup.MASK_UINT,
		"VISIBLECOLUMNS": "10",
		"EXPAND":         "HORIZONTAL",
	})

	btnOk := iup.Button("OK").SetAttribute("PADDING", "10x2")
	btnCancel := iup.Button("Cancel").SetAttribute("PADDING", "10x2")

	gotoDlg := iup.Dialog(iup.Vbox(
		iup.Label(fmt.Sprintf("Line Number [1-%d]:", lineCount)),
		txt,
		iup.Hbox(iup.Fill(), btnOk, btnCancel).SetAttributes("NORMALIZESIZE=HORIZONTAL, GAP=5"),
	).SetAttributes("MARGIN=10x10, GAP=5"))
	gotoDlg.SetAttributes(map[string]string{
		"TITLE":       "Go To Line",
		"DIALOGFRAME": "YES",
	})
	iup.SetAttributeHandle(gotoDlg, "DEFAULTENTER", btnOk)
	iup.SetAttributeHandle(gotoDlg, "DEFAULTESC", btnCancel)
	iup.SetAttributeHandle(gotoDlg, "PARENTDIALOG", dlg)

	btnOk.SetCallback("ACTION", iup.ActionFunc(func(b iup.Ihandle) int {
		line := txt.GetInt("VALUE")
		if line < 1 || line > lineCount {
			iup.Message("Error", "Invalid line number.")
			return iup.DEFAULT
		}
		gotoDlg.SetAttribute("STATUS", "1")
		return iup.CLOSE
	}))
	btnCancel.SetCallback("ACTION", iup.ActionFunc(func(b iup.Ihandle) int {
		gotoDlg.SetAttribute("STATUS", "0")
		return iup.CLOSE
	}))

	iup.Popup(gotoDlg, iup.CENTERPARENT, iup.CENTERPARENT)

	if gotoDlg.GetInt("STATUS") == 1 {
		line := txt.GetInt("VALUE")
		pos := iup.TextConvertLinColToPos(multitext, line, 0)
		multitext.SetAttribute("CARETPOS", fmt.Sprintf("%d", pos))
		multitext.SetAttribute("SCROLLTOPOS", fmt.Sprintf("%d", pos))
		iup.SetFocus(multitext)
	}
	gotoDlg.Destroy()
	return iup.DEFAULT
}

// ---------- Editor callbacks ----------

func caretCb(ih iup.Ihandle, lin, col, pos int) int {
	statusbar.SetAttribute("TITLE", fmt.Sprintf("Lin %d, Col %d", lin, col))
	return iup.DEFAULT
}

func valueChangedCb(ih iup.Ihandle) int {
	multitext.SetAttribute("DIRTY", "YES")
	return iup.DEFAULT
}

func dropFilesCb(ih iup.Ihandle, path string, num, x, y int) int {
	if num == 0 && saveCheck() {
		loadFile(path)
	}
	return iup.DEFAULT
}

func fileMenuOpenCb(ih iup.Ihandle) int {
	dirty := multitext.GetBool("DIRTY")
	iup.GetHandle("itemSave").SetAttribute("ACTIVE", dirty)
	iup.GetHandle("itemRevert").SetAttribute("ACTIVE", dirty && filename != "")
	return iup.DEFAULT
}

func editMenuOpenCb(ih iup.Ihandle) int {
	hasSelection := multitext.GetAttribute("SELECTEDTEXT") != ""
	iup.GetHandle("itemCut").SetAttribute("ACTIVE", hasSelection)
	iup.GetHandle("itemCopy").SetAttribute("ACTIVE", hasSelection)
	iup.GetHandle("itemDelete").SetAttribute("ACTIVE", hasSelection)

	cb := iup.Clipboard()
	defer cb.Destroy()
	iup.GetHandle("itemPaste").SetAttribute("ACTIVE", cb.GetBool("TEXTAVAILABLE"))
	return iup.DEFAULT
}

// ---------- Helpers ----------

// indexOf returns the byte offset of needle in haystack, or -1 if missing.
func indexOf(haystack, needle string, caseSensitive bool) int {
	if caseSensitive {
		return strings.Index(haystack, needle)
	}
	return strings.Index(strings.ToLower(haystack), strings.ToLower(needle))
}

func equal(a, b string, caseSensitive bool) bool {
	if caseSensitive {
		return a == b
	}
	return strings.EqualFold(a, b)
}

func configBool(key string, def bool) bool {
	d := 0
	if def {
		d = 1
	}
	return iup.ConfigGetVariableIntDef(config, "MainWindow", key, d) != 0
}

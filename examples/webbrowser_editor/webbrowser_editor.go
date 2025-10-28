//go:build web

package main

import (
	"fmt"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func wbNavigateCB(ih iup.Ihandle, url string) int {
	fmt.Printf("NAVIGATE_CB: %s\n", url)
	if strings.Contains(url, "download") {
		return iup.IGNORE
	}
	return iup.DEFAULT
}

func wbErrorCB(ih iup.Ihandle, url string) int {
	fmt.Printf("ERROR_CB: %s\n", url)
	return iup.DEFAULT
}

func wbCompletedCB(ih iup.Ihandle, url string) int {
	fmt.Printf("COMPLETED_CB: %s\n", url)
	fmt.Printf("CANGOBACK: %s\n", ih.GetAttribute("CANGOBACK"))
	fmt.Printf("CANGOFORWARD: %s\n", ih.GetAttribute("CANGOFORWARD"))
	return iup.DEFAULT
}

func wbUpdateCB(ih iup.Ihandle) int {
	fmt.Println("UPDATE_CB")
	return iup.DEFAULT
}

func wbNewWindowCB(ih iup.Ihandle, url string) int {
	fmt.Printf("NEWWINDOW_CB: %s\n", url)
	ih.SetAttribute("VALUE", url)
	return iup.DEFAULT
}

func openCB(ih iup.Ihandle) int {
	filedlg := iup.FileDlg()
	filedlg.SetAttribute("DIALOGTYPE", "OPEN")
	filedlg.SetAttribute("EXTFILTER", "HTML Files|*.html;*.htm;*.mhtml|All Files|*.*|")
	iup.Popup(filedlg, iup.CENTERPARENT, iup.CENTERPARENT)

	if filedlg.GetInt("STATUS") != -1 {
		filename := filedlg.GetAttribute("VALUE")
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		web.SetAttribute("OPENFILE", filename)
	}

	iup.Destroy(filedlg)
	return iup.DEFAULT
}

func saveCB(ih iup.Ihandle) int {
	filedlg := iup.FileDlg()
	filedlg.SetAttribute("DIALOGTYPE", "SAVE")
	filedlg.SetAttribute("EXTFILTER", "HTML Files|*.html;*.htm;*.mhtml|All Files|*.*|")
	iup.Popup(filedlg, iup.CENTERPARENT, iup.CENTERPARENT)

	if filedlg.GetInt("STATUS") != -1 {
		filename := filedlg.GetAttribute("VALUE")
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		web.SetAttribute("SAVEFILE", filename)
	}

	iup.Destroy(filedlg)
	return iup.DEFAULT
}

func newCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
	web.SetAttribute("NEW", "")
	return iup.DEFAULT
}

func printCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
	web.SetAttribute("PRINT", "YES")
	return iup.DEFAULT
}

func execCmdCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
	cmd := ih.GetAttribute("CMD")
	web.SetAttribute("EXECCOMMAND", cmd)
	return iup.DEFAULT
}

func insertImageCB(ih iup.Ihandle) int {
	filedlg := iup.FileDlg()
	filedlg.SetAttribute("DIALOGTYPE", "OPEN")
	filedlg.SetAttribute("EXTFILTER", "Image Files|*.jpeg;*.jpg;*.png;*.gif;*.svg|All Files|*.*|")
	iup.Popup(filedlg, iup.CENTERPARENT, iup.CENTERPARENT)

	if filedlg.GetInt("STATUS") != -1 {
		filename := filedlg.GetAttribute("VALUE")
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		web.SetAttribute("INSERTIMAGEFILE", filename)
	}

	iup.Destroy(filedlg)
	return iup.DEFAULT
}

func fgColorCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))

	dlg := iup.ColorDlg()
	if v := web.GetAttribute("FORECOLOR"); v != "" {
		dlg.SetAttribute("VALUE", v)
	}
	iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if dlg.GetInt("STATUS") != -1 {
		web.SetAttribute("FORECOLOR", dlg.GetAttribute("VALUE"))
	}
	iup.Destroy(dlg)
	return iup.DEFAULT
}

func bgColorCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))

	dlg := iup.ColorDlg()
	if v := web.GetAttribute("BACKCOLOR"); v != "" {
		dlg.SetAttribute("VALUE", v)
	}
	iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if dlg.GetInt("STATUS") != -1 {
		web.SetAttribute("BACKCOLOR", dlg.GetAttribute("VALUE"))
	}
	iup.Destroy(dlg)
	return iup.DEFAULT
}

func insertTextCB(ih iup.Ihandle) int {
	text, _ := iup.GetText("Insert Text", "", 1024)
	if text != "" {
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		web.SetAttribute("INSERTTEXT", text)
	}
	return iup.DEFAULT
}

func insertHTMLCB(ih iup.Ihandle) int {
	text, _ := iup.GetText("Insert HTML", "", 1024)
	if text != "" {
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		web.SetAttribute("INSERTHTML", text)
	}
	return iup.DEFAULT
}

func fontNameCB(ih iup.Ihandle) int {
	text, _ := iup.GetText("Font name", "", 255)
	if text != "" {
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		web.SetAttribute("FONTNAME", text)
	}
	return iup.DEFAULT
}

func fontSizeCB(ih iup.Ihandle, text string, index int, state int) int {
	if state == 1 {
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		web.SetAttribute("FONTSIZE", index)
	}
	return iup.DEFAULT
}

func formatBlockCB(ih iup.Ihandle, text string, index int, state int) int {
	if state == 1 {
		formats := []string{"<H1>", "<H2>", "<H3>", "<H4>", "<H5>", "<H6>", "<P>", "<PRE>", "<BLOCKQUOTE>"}
		web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
		if index >= 1 && index <= len(formats) {
			web.SetAttribute("FORMATBLOCK", formats[index-1])
		}
	}
	return iup.DEFAULT
}

func createLinkCB(ih iup.Ihandle) int {
	url, _ := iup.GetText("Create Link", "", 1024)
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
	web.SetAttribute("CREATELINK", url)
	return iup.DEFAULT
}

func htmlCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
	html := web.GetAttribute("HTML")
	if html == "" {
		return iup.DEFAULT
	}
	if newHTML, ret := iup.GetText("HTML Text", html, 1024); ret != 0 {
		web.SetAttribute("HTML", newHTML)
	}
	return iup.DEFAULT
}

func editableCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
	if web.GetInt("EDITABLE") != 0 {
		web.SetAttribute("EDITABLE", "NO")
	} else {
		web.SetAttribute("EDITABLE", "YES")
	}
	return iup.DEFAULT
}

func findCB(ih iup.Ihandle) int {
	web := iup.GetHandle(iup.GetAttribute(ih, "MY_WEB"))
	web.SetAttribute("FIND", "")
	return iup.DEFAULT
}

func keypressCB(ih iup.Ihandle, c int, pressed int) int {
	if iup.IsPrint(c) {
		fmt.Printf("KEYPRESS_CB(%d = '%c' (%d))\n", c, rune(c), pressed)
	} else {
		fmt.Printf("KEYPRESS_CB(%d (%d))\n", c, pressed)
	}
	return iup.DEFAULT
}

func buttonCB(ih iup.Ihandle, but int, pressed int, x int, y int, status string) int {
	fmt.Printf("BUTTON_CB(but=%c (%d), x=%d, y=%d [%s])\n", rune(but), pressed, x, y, status)
	return iup.DEFAULT
}

func webBrowserEditorTest() {
	iup.WebBrowserOpen()

	// Web control
	web := iup.WebBrowser()
	web.SetCallback("KEYPRESS_CB", iup.KeyPressFunc(keypressCB))
	web.SetCallback("BUTTON_CB", iup.ButtonFunc(buttonCB))

	// Top row: file/print/edit commands
	btNew := iup.Button("New").SetAttribute("CMD", "NEW")
	btOpen := iup.Button("Open...")
	btSave := iup.Button("Save As...")
	btPrint := iup.Button("Print")
	btUndo := iup.Button("Undo").SetAttribute("CMD", "UNDO")
	btRedo := iup.Button("Redo").SetAttribute("CMD", "REDO")
	btCut := iup.Button("Cut").SetAttribute("CMD", "CUT")
	btCopy := iup.Button("Copy").SetAttribute("CMD", "COPY")
	btPaste := iup.Button("Paste").SetAttribute("CMD", "PASTE")
	btSelectAll := iup.Button("Select All").SetAttribute("CMD", "SELECTALL")
	btFind := iup.Button("Find")
	btEditable := iup.Button("Editable")

	// Second row: formatting
	btBold := iup.Button("Bold").SetAttribute("CMD", "BOLD")
	btItalic := iup.Button("Italic").SetAttribute("CMD", "ITALIC")
	btStrike := iup.Button("Strikethrough").SetAttribute("CMD", "STRIKETHROUGH")
	btUnderline := iup.Button("Underline").SetAttribute("CMD", "UNDERLINE")
	btLeft := iup.Button("Left").SetAttribute("CMD", "JUSTIFYLEFT")
	btCenter := iup.Button("Center").SetAttribute("CMD", "JUSTIFYCENTER")
	btRight := iup.Button("Right").SetAttribute("CMD", "JUSTIFYRIGHT")
	btJustified := iup.Button("Justified").SetAttribute("CMD", "JUSTIFYFULL")
	btIndent := iup.Button("Indent").SetAttribute("CMD", "INDENT")
	btOutdent := iup.Button("Outdent").SetAttribute("CMD", "OUTDENT")
	btSub := iup.Button("Subscript").SetAttribute("CMD", "SUBSCRIPT")
	btSup := iup.Button("Superscript").SetAttribute("CMD", "SUPERSCRIPT")

	// Third row: lists/colors
	btRemoveFmt := iup.Button("Remove Format").SetAttribute("CMD", "REMOVEFORMAT")
	btDelete := iup.Button("Delete").SetAttribute("CMD", "DELETE")
	btOL := iup.Button("Insert Ordered List").SetAttribute("CMD", "INSERTORDEREDLIST")
	btUL := iup.Button("Insert Unordered List").SetAttribute("CMD", "INSERTUNORDEREDLIST")
	btFgColor := iup.Button("Foreground Color...").SetAttribute("CMD2", "FORECOLOR")
	btBgColor := iup.Button("Background Color...").SetAttribute("CMD2", "BACKCOLOR")

	// Fourth row: insertions/links/source
	btInsertImage := iup.Button("Insert Image...")
	btInsertText := iup.Button("Insert Text...")
	btInsertHTML := iup.Button("Insert HTML...")
	btCreateLink := iup.Button("Create Link...")
	btRemoveLink := iup.Button("Remove Link").SetAttribute("CMD", "UNLINK")
	btEditSource := iup.Button("Edit Source...")

	// Fifth row: font name/size/format block
	btFontName := iup.Button("Font Name...").SetAttribute("CMD2", "FONTNAME")
	lblFontSize := iup.Label("Font Size:")
	listFontSize := iup.List()
	listFontSize.SetAttribute("VALUESTRING", "medium")
	listFontSize.SetAttribute("DROPDOWN", "YES")
	listFontSize.SetAttribute("1", "x-small")
	listFontSize.SetAttribute("2", "small")
	listFontSize.SetAttribute("3", "medium")
	listFontSize.SetAttribute("4", "large")
	listFontSize.SetAttribute("5", "x-large")
	listFontSize.SetAttribute("6", "xx-large")
	listFontSize.SetAttribute("7", "xx-large")

	lblFormatBlock := iup.Label("Format Block:")
	listFormatBlock := iup.List()
	listFormatBlock.SetAttribute("VALUESTRING", "Paragraph")
	listFormatBlock.SetAttribute("DROPDOWN", "YES")
	listFormatBlock.SetAttribute("1", "Heading 1")
	listFormatBlock.SetAttribute("2", "Heading 2")
	listFormatBlock.SetAttribute("3", "Heading 3")
	listFormatBlock.SetAttribute("4", "Heading 4")
	listFormatBlock.SetAttribute("5", "Heading 5")
	listFormatBlock.SetAttribute("6", "Heading 6")
	listFormatBlock.SetAttribute("7", "Paragraph")
	listFormatBlock.SetAttribute("8", "Preformatted")
	listFormatBlock.SetAttribute("9", "Block Quote")

	// Pack layout
	row1 := iup.Hbox(
		btNew,
		btOpen, btSave,
		btPrint,
		btUndo, btRedo,
		btCut, btCopy, btPaste, btSelectAll,
		btFind,
		btEditable,
	)

	row2 := iup.Hbox(
		btBold, btItalic, btStrike, btUnderline,
		btLeft, btCenter, btRight, btJustified,
		btIndent, btOutdent,
		btSub, btSup,
	)

	row3 := iup.Hbox(
		btRemoveFmt, btDelete, btOL, btUL,
		btFgColor, btBgColor,
	)

	row4 := iup.Hbox(
		btInsertImage, btInsertText, btInsertHTML,
		btCreateLink, btRemoveLink, btEditSource,
	)

	row5 := iup.Hbox(
		btFontName,
		lblFontSize, listFontSize,
		lblFormatBlock, listFormatBlock,
	).SetAttribute("ALIGNMENT", "ACENTER")

	dlg := iup.Dialog(iup.Vbox(
		row1, row2, row3, row4, row5,
		web,
	))

	// Attributes
	dlg.SetAttribute("TITLE", "WebBrowser Editor")
	dlg.SetAttribute("RASTERSIZE", "x600")
	dlg.SetAttribute("MARGIN", "5x5")
	dlg.SetAttribute("GAP", "5")

	// Handle names and cross-references
	iup.SetHandle("WEB_HANDLE", web)
	for _, h := range []iup.Ihandle{
		btNew, btOpen, btSave, btPrint, btUndo, btRedo,
		btCut, btCopy, btPaste, btSelectAll, btFind, btEditable,
		btBold, btItalic, btStrike, btUnderline,
		btLeft, btCenter, btRight, btJustified,
		btIndent, btOutdent, btSub, btSup,
		btRemoveFmt, btDelete, btOL, btUL,
		btFgColor, btBgColor,
		btInsertImage, btInsertText, btInsertHTML,
		btCreateLink, btRemoveLink, btEditSource,
		btFontName, listFontSize, listFormatBlock,
	} {
		h.SetAttribute("MY_WEB", "WEB_HANDLE")
	}

	// Initial content and behavior
	web.SetAttribute("HTML", "<html><body><p><b>Hello</b> World!</p></body></html>")
	web.SetAttribute("EDITABLE", "YES")

	// Web callbacks
	web.SetCallback("NEWWINDOW_CB", iup.NewWindowFunc(wbNewWindowCB))
	web.SetCallback("NAVIGATE_CB", iup.NavigateFunc(wbNavigateCB))
	web.SetCallback("ERROR_CB", iup.ErrorFunc(wbErrorCB))
	web.SetCallback("COMPLETED_CB", iup.CompletedFunc(wbCompletedCB))
	web.SetCallback("UPDATE_CB", iup.UpdateFunc(wbUpdateCB))

	// Button callbacks
	btNew.SetCallback("ACTION", iup.ActionFunc(newCB))
	btOpen.SetCallback("ACTION", iup.ActionFunc(openCB))
	btSave.SetCallback("ACTION", iup.ActionFunc(saveCB))
	btPrint.SetCallback("ACTION", iup.ActionFunc(printCB))

	for _, b := range []iup.Ihandle{
		btUndo, btRedo, btCut, btCopy, btPaste, btSelectAll,
		btBold, btItalic, btStrike, btUnderline,
		btLeft, btCenter, btRight, btJustified,
		btIndent, btOutdent, btSub, btSup,
		btRemoveFmt, btDelete, btOL, btUL, btRemoveLink,
	} {
		b.SetCallback("ACTION", iup.ActionFunc(execCmdCB))
	}

	btFgColor.SetCallback("ACTION", iup.ActionFunc(fgColorCB))
	btBgColor.SetCallback("ACTION", iup.ActionFunc(bgColorCB))

	btInsertImage.SetCallback("ACTION", iup.ActionFunc(insertImageCB))
	btInsertText.SetCallback("ACTION", iup.ActionFunc(insertTextCB))
	btInsertHTML.SetCallback("ACTION", iup.ActionFunc(insertHTMLCB))
	btCreateLink.SetCallback("ACTION", iup.ActionFunc(createLinkCB))
	btEditSource.SetCallback("ACTION", iup.ActionFunc(htmlCB))
	btFontName.SetCallback("ACTION", iup.ActionFunc(fontNameCB))
	btEditable.SetCallback("ACTION", iup.ActionFunc(editableCB))
	btFind.SetCallback("ACTION", iup.ActionFunc(findCB))

	listFontSize.SetCallback("ACTION", iup.ListActionFunc(fontSizeCB))
	listFormatBlock.SetCallback("ACTION", iup.ListActionFunc(formatBlockCB))

	iup.Show(dlg)
}

func main() {
	iup.Open()
	defer iup.Close()

	webBrowserEditorTest()

	iup.MainLoop()
}

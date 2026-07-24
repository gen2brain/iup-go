package main

import (
	"strconv"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const sampleText = `Rich Text Editor

Select some text and use the toolbar to make it bold, italic, colored, or aligned. You can also insert links, bullet lists, and import Markdown.

Type here to try it out.`

const sampleMarkdown = `

## Imported Markdown

This section was inserted with **APPENDMARKDOWN**, mixing *emphasis*, ` + "`code`" + `, and a list:

- first item
- second item
`

func editorOf(ih iup.Ihandle) iup.Ihandle {
	return iup.GetDialogChild(ih, "editor")
}

func applyTag(ih iup.Ihandle, set func(tag iup.Ihandle)) {
	editor := editorOf(ih)
	tag := iup.User()
	set(tag)
	if sel := editor.GetAttribute("_SAVEDSEL"); sel != "" {
		tag.SetAttribute("SELECTION", sel)
	}
	iup.SetAttributeHandle(editor, "ADDFORMATTAG", tag)
	iup.SetFocus(editor)
}

func styleButton(title string, set func(tag iup.Ihandle)) iup.Ihandle {
	b := iup.Button(title)
	b.SetAttribute("CANFOCUS", "NO")
	iup.SetCallback(b, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		applyTag(ih, set)
		return iup.DEFAULT
	}))
	return b
}

func colorButton(title, attr string) iup.Ihandle {
	b := iup.Button(title)
	b.SetAttribute("CANFOCUS", "NO")
	iup.SetCallback(b, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		cd := iup.ColorDlg()
		cd.SetAttribute("VALUE", "0 0 0")
		iup.Popup(cd, iup.CENTER, iup.CENTER)
		if iup.GetInt(cd, "STATUS") == 1 {
			color := cd.GetAttribute("VALUE")
			applyTag(ih, func(tag iup.Ihandle) {
				tag.SetAttribute(attr, color)
			})
		}
		cd.Destroy()
		return iup.DEFAULT
	}))
	return b
}

func sizeList() iup.Ihandle {
	list := iup.List()
	list.SetAttributes(`DROPDOWN=YES, TIP="Font size"`)
	for i, s := range []string{"8", "9", "10", "11", "12", "14", "16", "18", "24", "36"} {
		list.SetAttribute(strconv.Itoa(i+1), s)
	}
	list.SetAttribute("VALUE", "5")
	iup.SetCallback(list, "ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			applyTag(ih, func(tag iup.Ihandle) {
				tag.SetAttribute("FONTSIZE", text)
			})
		}
		return iup.DEFAULT
	}))
	return list
}

func linkButton() iup.Ihandle {
	b := iup.Button("Link")
	b.SetAttribute("CANFOCUS", "NO")
	iup.SetCallback(b, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		url, ok := iup.GetText("Insert Link", "https://", 256)
		if ok != 0 && url != "" {
			applyTag(ih, func(tag iup.Ihandle) {
				tag.SetAttribute("LINK", url)
			})
		}
		return iup.DEFAULT
	}))
	return b
}

func importButton() iup.Ihandle {
	b := iup.Button("Import MD")
	b.SetAttribute("CANFOCUS", "NO")
	iup.SetCallback(b, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		fd := iup.FileDlg()
		fd.SetAttributes(`DIALOGTYPE=OPEN, TITLE="Import Markdown", EXTFILTER="Markdown Files|*.md;*.markdown|All Files|*.*"`)
		iup.Popup(fd, iup.CENTER, iup.CENTER)
		if iup.GetInt(fd, "STATUS") != -1 {
			editorOf(ih).SetAttribute("LOADMARKDOWN", fd.GetAttribute("VALUE"))
		}
		fd.Destroy()
		return iup.DEFAULT
	}))
	return b
}

func actionButton(title string, cb func(editor iup.Ihandle)) iup.Ihandle {
	b := iup.Button(title)
	b.SetAttribute("CANFOCUS", "NO")
	iup.SetCallback(b, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		cb(editorOf(ih))
		return iup.DEFAULT
	}))
	return b
}

func main() {
	iup.Open()
	defer iup.Close()

	editor := iup.MultiLine()
	editor.SetAttributes(`NAME=editor, FORMATTING=YES, WORDWRAP=YES, EXPAND=YES, SIZE=420x240`)
	editor.SetAttribute("VALUE", sampleText)

	iup.SetCallback(editor, "CARET_CB", iup.CaretFunc(func(ih iup.Ihandle, lin, col, pos int) int {
		ih.SetAttribute("_SAVEDSEL", ih.GetAttribute("SELECTION"))
		return iup.DEFAULT
	}))

	clear := actionButton("Clear", func(editor iup.Ihandle) {
		if sel := editor.GetAttribute("_SAVEDSEL"); sel != "" {
			editor.SetAttribute("SELECTION", sel)
		}
		editor.SetAttribute("REMOVEFORMATTING", "YES")
		iup.SetFocus(editor)
	})
	clear.SetAttribute("TIP", "Remove formatting from selection")

	toolbar1 := iup.Hbox(
		styleButton("Bold", func(t iup.Ihandle) { t.SetAttribute("WEIGHT", "BOLD") }),
		styleButton("Italic", func(t iup.Ihandle) { t.SetAttribute("ITALIC", "YES") }),
		styleButton("Underline", func(t iup.Ihandle) { t.SetAttribute("UNDERLINE", "SINGLE") }),
		styleButton("Strike", func(t iup.Ihandle) { t.SetAttribute("STRIKEOUT", "YES") }),
		iup.Label("Size:"),
		sizeList(),
		colorButton("Color", "FGCOLOR"),
		colorButton("Highlight", "BGCOLOR"),
		clear,
	)
	toolbar1.SetAttributes("GAP=3, ALIGNMENT=ACENTER")

	toolbar2 := iup.Hbox(
		styleButton("Left", func(t iup.Ihandle) { t.SetAttribute("ALIGNMENT", "LEFT") }),
		styleButton("Center", func(t iup.Ihandle) { t.SetAttribute("ALIGNMENT", "CENTER") }),
		styleButton("Right", func(t iup.Ihandle) { t.SetAttribute("ALIGNMENT", "RIGHT") }),
		styleButton("Bullet", func(t iup.Ihandle) { t.SetAttribute("NUMBERING", "BULLET") }),
		styleButton("Numbered", func(t iup.Ihandle) { t.SetAttribute("NUMBERING", "ARABIC") }),
		linkButton(),
		importButton(),
		actionButton("Sample MD", func(editor iup.Ihandle) {
			editor.SetAttribute("APPENDMARKDOWN", sampleMarkdown)
		}),
	)
	toolbar2.SetAttribute("GAP", "3")

	vbox := iup.Vbox(toolbar1, toolbar2, editor)
	vbox.SetAttributes("NMARGIN=10x10, NGAP=6")

	dlg := iup.Dialog(vbox)
	dlg.SetAttribute("TITLE", "Rich Text Editor")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.SetFocus(editor)
	iup.MainLoop()
}

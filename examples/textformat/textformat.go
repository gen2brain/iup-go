package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func text2multiline(ih iup.Ihandle, attribute string) {
	mltline := iup.GetDialogChild(ih, "mltline")
	text := iup.GetDialogChild(ih, "text")
	iup.SetAttribute(mltline, attribute, iup.GetAttribute(text, "VALUE"))
}

func multiline2text(ih iup.Ihandle, attribute string) {
	mltline := iup.GetDialogChild(ih, "mltline")
	text := iup.GetDialogChild(ih, "text")
	iup.SetAttribute(text, "VALUE", iup.GetAttribute(mltline, attribute))
}

func btnAppendCB(ih iup.Ihandle) int {
	text2multiline(ih, "APPEND")
	return iup.DEFAULT
}

func btnInsertCB(ih iup.Ihandle) int {
	text2multiline(ih, "INSERT")
	return iup.DEFAULT
}

func btnClipCB(ih iup.Ihandle) int {
	text2multiline(ih, "CLIPBOARD")
	return iup.DEFAULT
}

func btnKeyCB(ih iup.Ihandle) int {
	mltline := iup.GetDialogChild(ih, "mltline")
	iup.SetFocus(mltline)
	iup.Flush()
	return iup.DEFAULT
}

func btnCaretCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "CARET")
	} else {
		multiline2text(ih, "CARET")
	}
	return iup.DEFAULT
}

func btnReadonlyCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "READONLY")
	} else {
		multiline2text(ih, "READONLY")
	}
	return iup.DEFAULT
}

func btnSelectionCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "SELECTION")
	} else {
		multiline2text(ih, "SELECTION")
	}
	return iup.DEFAULT
}

func btnSelectedTextCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "SELECTEDTEXT")
	} else {
		multiline2text(ih, "SELECTEDTEXT")
	}
	return iup.DEFAULT
}

func btnOverwriteCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "OVERWRITE")
	} else {
		multiline2text(ih, "OVERWRITE")
	}
	return iup.DEFAULT
}

func btnActiveCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "ACTIVE")
	} else {
		multiline2text(ih, "ACTIVE")
	}
	return iup.DEFAULT
}

func btnRemFormatCB(ih iup.Ihandle) int {
	text2multiline(ih, "REMOVEFORMATTING")
	return iup.DEFAULT
}

func btnNcCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "NC")
	} else {
		multiline2text(ih, "NC")
	}
	return iup.DEFAULT
}

func btnValueCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "VALUE")
	} else {
		multiline2text(ih, "VALUE")
	}
	return iup.DEFAULT
}

func btnTabSizeCB(ih iup.Ihandle) int {
	opt := iup.GetHandle("text2multi")
	if iup.GetInt(opt, "VALUE") != 0 {
		text2multiline(ih, "TABSIZE")
	} else {
		multiline2text(ih, "TABSIZE")
	}
	return iup.DEFAULT
}

func kAny(ih iup.Ihandle, c int) int {
	fmt.Printf("  CARET(%s)\n", iup.GetAttribute(ih, "CARET"))

	if c == iup.K_F2 {
		fmt.Println("K_F2")
		return iup.DEFAULT
	}
	if c == iup.K_A {
		return iup.IGNORE
	}
	return iup.CONTINUE
}

func caretCB(ih iup.Ihandle, lin, col, pos int) int {
	fmt.Printf("CARET_CB(%d, %d - %d)\n", lin, col, pos)
	fmt.Printf("  CARET(%s - %s)\n", iup.GetAttribute(ih, "CARET"), iup.GetAttribute(ih, "CARETPOS"))
	return iup.DEFAULT
}

func getFocusCB(ih iup.Ihandle) int {
	fmt.Println("GETFOCUS_CB()")
	return iup.DEFAULT
}

func helpCB(ih iup.Ihandle) int {
	fmt.Println("HELP_CB()")
	return iup.DEFAULT
}

func killFocusCB(ih iup.Ihandle) int {
	fmt.Println("KILLFOCUS_CB()")
	return iup.DEFAULT
}

func leaveWindowCB(ih iup.Ihandle) int {
	fmt.Println("LEAVEWINDOW_CB()")
	return iup.DEFAULT
}

func enterWindowCB(ih iup.Ihandle) int {
	fmt.Println("ENTERWINDOW_CB()")
	return iup.DEFAULT
}

func btnDefEscCB(ih iup.Ihandle) int {
	fmt.Println("DEFAULTESC")
	return iup.DEFAULT
}

func btnDefEnterCB(ih iup.Ihandle) int {
	fmt.Println("DEFAULTENTER")
	return iup.DEFAULT
}

func dropFilesCB(ih iup.Ihandle, filename string, num, x, y int) int {
	fmt.Printf("DROPFILES_CB(%s, %d, x=%d, y=%d)\n", filename, num, x, y)
	return iup.DEFAULT
}

func buttonCB(ih iup.Ihandle, button, pressed, x, y int, status string) int {
	fmt.Printf("BUTTON_CB(but=%c (%d), x=%d, y=%d [%s])\n", button, pressed, x, y, status)
	pos := iup.ConvertXYToPos(ih, x, y)
	lin, col := iup.TextConvertPosToLinCol(ih, pos)
	fmt.Printf("         (lin=%d, col=%d, pos=%d)\n", lin, col, pos)
	return iup.DEFAULT
}

func motionCB(ih iup.Ihandle, x, y int, status string) int {
	fmt.Printf("MOTION_CB(x=%d, y=%d [%s])\n", x, y, status)
	pos := iup.ConvertXYToPos(ih, x, y)
	lin, col := iup.TextConvertPosToLinCol(ih, pos)
	fmt.Printf("         (lin=%d, col=%d, pos=%d)\n", lin, col, pos)
	return iup.DEFAULT
}

func textTest() {
	formatting := false

	text := iup.Text()
	iup.SetAttribute(text, "EXPAND", "HORIZONTAL")
	iup.SetAttribute(text, "CUEBANNER", "Enter Attribute Value Here")
	iup.SetAttribute(text, "NAME", "text")
	iup.SetAttribute(text, "TIP", "Attribute Value")

	opt := iup.Toggle("Set/Get")
	iup.SetAttribute(opt, "VALUE", "ON")
	iup.SetHandle("text2multi", opt)

	mltline := iup.MultiLine()
	iup.SetAttribute(mltline, "NAME", "mltline")

	iup.SetCallback(mltline, "DROPFILES_CB", iup.DropFilesFunc(dropFilesCB))
	iup.SetCallback(mltline, "BUTTON_CB", iup.ButtonFunc(buttonCB))
	//iup.SetCallback(mltline, "MOTION_CB", iup.MotionFunc(motionCB))
	iup.SetCallback(mltline, "HELP_CB", iup.HelpFunc(helpCB))
	iup.SetCallback(mltline, "GETFOCUS_CB", iup.GetFocusFunc(getFocusCB))
	iup.SetCallback(mltline, "KILLFOCUS_CB", iup.KillFocusFunc(killFocusCB))
	iup.SetCallback(mltline, "ENTERWINDOW_CB", iup.EnterWindowFunc(enterWindowCB))
	iup.SetCallback(mltline, "LEAVEWINDOW_CB", iup.LeaveWindowFunc(leaveWindowCB))
	iup.SetCallback(mltline, "K_ANY", iup.KAnyFunc(kAny))
	iup.SetCallback(mltline, "CARET_CB", iup.CaretFunc(caretCB))

	//iup.SetAttribute(mltline, "WORDWRAP", "YES")
	//iup.SetAttribute(mltline, "BORDER", "NO")
	//iup.SetAttribute(mltline, "AUTOHIDE", "YES")
	//iup.SetAttribute(mltline, "BGCOLOR", "255 0 128")
	//iup.SetAttribute(mltline, "FGCOLOR", "0 128 192")
	//iup.SetAttribute(mltline, "PADDING", "15x15")
	iup.SetAttribute(mltline, "VALUE", "First Line\nSecond Line Big Big Big\nThird Line\nmore\nmore\nçãõáóé")
	iup.SetAttribute(mltline, "TIP", "First Line\nSecond Line\nThird Line")
	iup.SetAttribute(mltline, "SIZE", "80x40")
	iup.SetAttribute(mltline, "EXPAND", "YES")

	if formatting {
		iup.SetAttribute(mltline, "FORMATTING", "YES")

		formattag := iup.User()
		iup.SetAttribute(formattag, "ALIGNMENT", "CENTER")
		iup.SetAttribute(formattag, "SPACEAFTER", "10")
		iup.SetAttribute(formattag, "FONTSIZE", "24")
		iup.SetAttribute(formattag, "SELECTION", "3,1:3,50")
		iup.SetAttributeHandle(mltline, "ADDFORMATTAG_HANDLE", formattag)

		formattag = iup.User()
		iup.SetAttribute(formattag, "BGCOLOR", "255 128 64")
		iup.SetAttribute(formattag, "UNDERLINE", "SINGLE")
		iup.SetAttribute(formattag, "WEIGHT", "BOLD")
		iup.SetAttribute(formattag, "SELECTION", "3,7:3,11")
		iup.SetAttributeHandle(mltline, "ADDFORMATTAG_HANDLE", formattag)
	}

	btnAppend := iup.Button("&APPEND")
	btnInsert := iup.Button("INSERT")
	btnCaret := iup.Button("CARET")
	btnReadonly := iup.Button("READONLY")
	btnSelection := iup.Button("SELECTION")
	btnSelectedText := iup.Button("SELECTEDTEXT")
	btnNc := iup.Button("NC")
	btnValue := iup.Button("VALUE")
	btnTabSize := iup.Button("TABSIZE")
	btnClip := iup.Button("CLIPBOARD")
	btnKey := iup.Button("KEY")
	btnDefEnter := iup.Button("Default Enter")
	btnDefEsc := iup.Button("Default Esc")
	btnActive := iup.Button("ACTIVE")
	btnRemFormat := iup.Button("REMOVEFORMATTING")
	btnOverwrite := iup.Button("OVERWRITE")

	iup.SetAttribute(btnAppend, "TIP", "First Line\nSecond Line\nThird Line")

	iup.SetCallback(btnAppend, "ACTION", iup.ActionFunc(btnAppendCB))
	iup.SetCallback(btnInsert, "ACTION", iup.ActionFunc(btnInsertCB))
	iup.SetCallback(btnCaret, "ACTION", iup.ActionFunc(btnCaretCB))
	iup.SetCallback(btnReadonly, "ACTION", iup.ActionFunc(btnReadonlyCB))
	iup.SetCallback(btnSelection, "ACTION", iup.ActionFunc(btnSelectionCB))
	iup.SetCallback(btnSelectedText, "ACTION", iup.ActionFunc(btnSelectedTextCB))
	iup.SetCallback(btnNc, "ACTION", iup.ActionFunc(btnNcCB))
	iup.SetCallback(btnValue, "ACTION", iup.ActionFunc(btnValueCB))
	iup.SetCallback(btnTabSize, "ACTION", iup.ActionFunc(btnTabSizeCB))
	iup.SetCallback(btnClip, "ACTION", iup.ActionFunc(btnClipCB))
	iup.SetCallback(btnKey, "ACTION", iup.ActionFunc(btnKeyCB))
	iup.SetCallback(btnDefEnter, "ACTION", iup.ActionFunc(btnDefEnterCB))
	iup.SetCallback(btnDefEsc, "ACTION", iup.ActionFunc(btnDefEscCB))
	iup.SetCallback(btnActive, "ACTION", iup.ActionFunc(btnActiveCB))
	iup.SetCallback(btnRemFormat, "ACTION", iup.ActionFunc(btnRemFormatCB))
	iup.SetCallback(btnOverwrite, "ACTION", iup.ActionFunc(btnOverwriteCB))

	lbl := iup.Label("&Multiline:")
	iup.SetAttribute(lbl, "PADDING", "2x2")

	dlg := iup.Dialog(
		iup.Vbox(
			lbl,
			mltline,
			iup.Hbox(text, opt),
			iup.Hbox(btnAppend, btnInsert, btnCaret, btnReadonly, btnSelection),
			iup.Hbox(btnSelectedText, btnNc, btnValue, btnTabSize, btnClip, btnKey),
			iup.Hbox(btnDefEnter, btnDefEsc, btnActive, btnRemFormat, btnOverwrite),
		),
	)

	iup.SetAttribute(dlg, "TITLE", "IupText Test")
	iup.SetAttribute(dlg, "MARGIN", "10x10")
	iup.SetAttribute(dlg, "GAP", "5")
	iup.SetAttributeHandle(dlg, "DEFAULTENTER", btnDefEnter)
	iup.SetAttributeHandle(dlg, "DEFAULTESC", btnDefEsc)
	iup.SetAttribute(dlg, "SHRINK", "YES")

	if formatting {
		iup.Map(dlg)

		formattag := iup.User()
		iup.SetAttribute(formattag, "ITALIC", "YES")
		iup.SetAttribute(formattag, "STRIKEOUT", "YES")
		iup.SetAttribute(formattag, "SELECTION", "2,1:2,12")
		iup.SetAttributeHandle(mltline, "ADDFORMATTAG_HANDLE", formattag)
	}

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.SetFocus(mltline)
}

func main() {
	iup.Open()
	defer iup.Close()

	textTest()
	iup.MainLoop()
}

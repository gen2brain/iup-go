package main

import (
	"fmt"
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	mltline := iup.MultiLine()
	text := iup.Text()
	list := iup.List()

	iup.SetAttribute(mltline, "EXPAND", "YES")
	iup.SetAttribute(text, "EXPAND", "HORIZONTAL")

	iup.SetHandle("mltline", mltline)
	iup.SetHandle("text", text)
	iup.SetHandle("list", list)

	iup.SetAttributes(list, "1=SET, 2=GET, DROPDOWN=YES")

	btnAppend := iup.Button("Append")
	btnInsert := iup.Button("Insert")
	btnBorder := iup.Button("Border")
	btnCaret := iup.Button("Caret")
	btnReadonly := iup.Button("Read only")
	btnSelection := iup.Button("Selection")
	btnSelectedtext := iup.Button("Selected Text")
	btnNc := iup.Button("Number of characters")
	btnValue := iup.Button("Value")

	iup.SetCallback(btnAppend, "ACTION", iup.ActionFunc(btnAppendCb))
	iup.SetCallback(btnInsert, "ACTION", iup.ActionFunc(btnInsertCb))
	iup.SetCallback(btnBorder, "ACTION", iup.ActionFunc(btnBorderCb))
	iup.SetCallback(btnCaret, "ACTION", iup.ActionFunc(btnCaretCb))
	iup.SetCallback(btnReadonly, "ACTION", iup.ActionFunc(btnReadonlyCb))
	iup.SetCallback(btnSelection, "ACTION", iup.ActionFunc(btnSelectionCb))
	iup.SetCallback(btnSelectedtext, "ACTION", iup.ActionFunc(btnSelectedTextCb))
	iup.SetCallback(btnNc, "ACTION", iup.ActionFunc(btnNcCb))
	iup.SetCallback(btnValue, "ACTION", iup.ActionFunc(btnValueCb))

	dlg := iup.Dialog(
		iup.Vbox(
			mltline,
			iup.Hbox(text, list),
			iup.Hbox(btnAppend, btnInsert, btnBorder, btnCaret, btnReadonly, btnSelection),
			iup.Hbox(btnSelectedtext, btnNc, btnValue),
		),
	)

	iup.SetAttributes(dlg, "TITLE=\"MultiLine Example\", SIZE=HALFxQUARTER")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	iup.MainLoop()
}

// setAttribute sets an attribute with the value in the text
func setAttribute(attribute string) int {
	mltline := iup.GetHandle("mltline")
	text := iup.GetHandle("text")

	value := iup.GetAttribute(text, "VALUE")
	iup.SetAttribute(mltline, attribute, value)

	message := fmt.Sprintf("Attribute %s set with value %s", attribute, value)
	iup.Message("Set attribute", message)

	return iup.DEFAULT
}

// getAttribute gets an attribute of the multiline and shows it in the text
func getAttribute(attribute string) int {
	mltline := iup.GetHandle("mltline")
	text := iup.GetHandle("text")

	value := iup.GetAttribute(mltline, attribute)
	iup.SetAttribute(text, "VALUE", value)

	message := fmt.Sprintf("Attribute %s get with value %s", attribute, value)
	iup.Message("Get attribute", message)

	return iup.DEFAULT
}

// handleAttributeAction handles SET/GET for an attribute based on list selection
func handleAttributeAction(attribute string) func() int {
	return func() int {
		list := iup.GetHandle("list")

		if iup.GetInt(list, "VALUE") == 1 {
			return setAttribute(attribute)
		} else {
			return getAttribute(attribute)
		}
	}
}

// btnAppendCb appends text to the multiline
func btnAppendCb(ih iup.Ihandle) int {
	return handleAttributeAction("APPEND")()
}

// btnInsertCb inserts text in the multiline
func btnInsertCb(ih iup.Ihandle) int {
	return handleAttributeAction("INSERT")()
}

// btnBorderCb sets/gets border of the multiline
func btnBorderCb(ih iup.Ihandle) int {
	return handleAttributeAction("BORDER")()
}

// btnCaretCb sets/gets position of the caret
func btnCaretCb(ih iup.Ihandle) int {
	return handleAttributeAction("CARET")()
}

// btnReadonlyCb sets/gets readonly attribute
func btnReadonlyCb(ih iup.Ihandle) int {
	return handleAttributeAction("READONLY")()
}

// btnSelectionCb changes/gets the selection attribute
func btnSelectionCb(ih iup.Ihandle) int {
	return handleAttributeAction("SELECTION")()
}

// btnSelectedTextCb changes/gets the selected text attribute
func btnSelectedTextCb(ih iup.Ihandle) int {
	return handleAttributeAction("SELECTEDTEXT")()
}

// btnNcCb sets/gets limit number of characters in the multiline
func btnNcCb(ih iup.Ihandle) int {
	return handleAttributeAction("NC")()
}

// btnValueCb sets/gets text in the multiline
func btnValueCb(ih iup.Ihandle) int {
	return handleAttributeAction("VALUE")()
}

package iup

import (
	"bytes"
	"image/color"
	"unsafe"

	"github.com/google/uuid"
)

/*
#include <stdlib.h>
#include <string.h>
#include "iup.h"
*/
import "C"

// Dialog creates a dialog element. It manages user interaction with the interface elements.
// For any interface element to be shown, it must be encapsulated in a dialog.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupdialog.html
func Dialog(child Ihandle) Ihandle {
	h := mkih(C.IupDialog(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Popup shows a dialog or menu and restricts user interaction only to the specified element.
// It is equivalent of creating a Modal dialog is some toolkits.
//
// If another dialog is shown after Popup using Show, then its interaction will not be inhibited.
// Every Popup call creates a new popup level that inhibits all previous dialogs interactions, but does not disable new ones
// (even if they were disabled by the Popup, calling Show will re-enable the dialog because it will change its popup level).
// IMPORTANT: The popup levels must be closed in the reverse order they were created or unpredictable results will occur.
//
// For a dialog, this function will only return the control to the application
// after a callback returns CLOSE, ExitLoop is called, or when the popup dialog is hidden, for example, using Hide.
// For a menu it returns automatically after a menu item is selected.
// IMPORTANT: If a menu item callback returns CLOSE, it will also ends the current popup level dialog.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuppopup.html
func Popup(ih Ihandle, x, y int) int {
	return int(C.IupPopup(ih.ptr(), C.int(x), C.int(y)))
}

// Show displays a dialog in the current position, or changes a control VISIBLE attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupshow.html
func Show(ih Ihandle) int {
	return int(C.IupShow(ih.ptr()))
}

// ShowXY displays a dialog in a given position on the screen.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupshowxy.html
func ShowXY(ih Ihandle, x, y int) int {
	return int(C.IupShowXY(ih.ptr(), C.int(x), C.int(y)))
}

// Hide hides an interface element.
// This function has the same effect as attributing value "NO" to the interface element’s VISIBLE attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuphide.html
func Hide(ih Ihandle) int {
	return int(C.IupHide(ih.ptr()))
}

// FileDlg creates the File Dialog element. It is a predefined dialog for selecting files or a directory.
// The dialog can be shown with the Popup function only.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupfiledlg.html
func FileDlg() Ihandle {
	h := mkih(C.IupFileDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// MessageDlg creates the Message Dialog element. It is a predefined dialog for displaying a message.
// The dialog can be shown with the Popup function only.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupmessagedlg.html
func MessageDlg() Ihandle {
	h := mkih(C.IupMessageDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ColorDlg creates the Color Dialog element. It is a predefined dialog for selecting a color.
// The Windows and GTK dialogs can be shown only with the Popup function.
// The ColorBrowser based dialog is a Dialog that can be shown as any regular Dialog.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupcolordlg.html
func ColorDlg() Ihandle {
	h := mkih(C.IupColorDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FontDlg creates the Font Dialog element. It is a predefined dialog for selecting a font.
// The dialog can be shown with the Popup function only.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupfontdlg.html
func FontDlg() Ihandle {
	h := mkih(C.IupFontDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ProgressDlg creates a progress dialog element. It is a predefined dialog for displaying the progress of an operation.
// The dialog is meant to be shown with the show functions Show or ShowXY.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupprogressdlg.html
func ProgressDlg() Ihandle {
	h := mkih(C.IupProgressDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Alarm shows a modal dialog containing a message and up to three buttons.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupalarm.html
func Alarm(title, msg, b1, b2, b3 string) int {
	cTitle, cMsg, cB1, cB2, cB3 := C.CString(title), C.CString(msg), C.CString(b1), cStrOrNull(b2), cStrOrNull(b3)
	defer C.free(unsafe.Pointer(cTitle))
	defer C.free(unsafe.Pointer(cMsg))
	defer C.free(unsafe.Pointer(cB1))
	defer cStrFree(cB2)
	defer cStrFree(cB3)

	return int(C.IupAlarm(cTitle, cMsg, cB1, cB2, cB3))
}

// GetFile shows a modal dialog of the native interface system to select a filename. Uses the FileDlg element.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupgetfile.html
func GetFile(path string) (sel string, ret int) {
	if len(path) > 4095 {
		panic("path is too long (maximum is 4095)")
	}
	buf := bytes.NewBuffer([]byte(path))
	buf.Grow(4096 - len(path))
	byt := buf.Bytes()

	ret = int(C.IupGetFile((*C.char)((unsafe.Pointer)(&byt[0]))))

	sel = string(byt[:int(C.strlen((*C.char)((unsafe.Pointer)(&byt[0]))))])
	return
}

// GetColor shows a modal dialog which allows the user to select a color. Based on ColorDlg.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupgetcolor.html
func GetColor(x, y int) (col color.RGBA, ret int) {
	var r, g, b uint8
	ret = int(C.IupGetColor(C.int(x), C.int(y), (*C.uchar)(unsafe.Pointer(&r)), (*C.uchar)(unsafe.Pointer(&g)), (*C.uchar)(unsafe.Pointer(&b))))

	col.R = r
	col.G = g
	col.B = b
	col.A = 255
	return
}

// GetText shows a modal dialog to edit a multiline text.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupgettext.html
func GetText(title, text string, maxSize int) (string, int) {
	cTitle := C.CString(title)
	defer C.free(unsafe.Pointer(cTitle))

	bufSize := maxSize
	if maxSize == 0 {
		bufSize = len(text)
	} else if maxSize < 0 {
		bufSize = len(text)
	}
	if bufSize < 1 {
		bufSize = 1
	}

	buf := make([]byte, bufSize+1)
	copy(buf, text)

	ret := int(C.IupGetText(cTitle, (*C.char)(unsafe.Pointer(&buf[0])), C.int(maxSize)))

	resultLen := int(C.strlen((*C.char)(unsafe.Pointer(&buf[0]))))
	return string(buf[:resultLen]), ret
}

// ListDialog shows a modal dialog to select items from a simple or multiple selection list.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iuplistdialog.html
func ListDialog(_type int, title string, list []string, op, maxCol, maxLin int, marks *[]bool) (ret int) {
	if len(list) != len(*marks) {
		panic("bad parameter passed to ListDialog")
	}

	cTitle := C.CString(title)
	defer C.free(unsafe.Pointer(cTitle))

	pList := make([]*C.char, len(list))
	for i := 0; i < len(list); i++ {
		pList[i] = C.CString(list[i])
	}
	defer func() {
		for i := 0; i < len(list); i++ {
			C.free(unsafe.Pointer(pList[i]))
		}
	}()

	pMark := make([]C.int, len(list))
	for i := 0; i < len(list); i++ {
		if (*marks)[i] {
			pMark[i] = 1
		} else {
			pMark[i] = 0
		}
	}
	defer func() {
		for i := 0; i < len(list); i++ {
			(*marks)[i] = pMark[i] != C.int(0)
		}
	}()

	ret = int(C.IupListDialog(C.int(_type), cTitle, C.int(len(list)), (**C.char)(unsafe.Pointer(&(pList[0]))),
		C.int(op), C.int(maxCol), C.int(maxLin), (*C.int)(unsafe.Pointer(&pMark[0]))))
	return
}

// Message shows a modal dialog containing a message.
// It simply creates and popup a MessageDlg.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupmessage.html
func Message(title, msg string) {
	cTitle, cMsg := C.CString(title), C.CString(msg)
	defer C.free(unsafe.Pointer(cTitle))
	defer C.free(unsafe.Pointer(cMsg))

	C.IupMessage(cTitle, cMsg)
}

// MessageError shows a modal dialog containing an error message.
// It simply creates and popup a MessageDlg with DIALOGTYPE=ERROR.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupmessageerror.html
func MessageError(parent Ihandle, msg string) {
	cMsg := C.CString(msg)
	defer C.free(unsafe.Pointer(cMsg))

	C.IupMessageError(parent.ptr(), cMsg)
}

// MessageAlarm shows a modal dialog containing a question message, similar to Alarm.
// It simply creates and popup a MessageDlg with DIALOGTYPE=QUESTION.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupmessagealarm.html
func MessageAlarm(parent Ihandle, title, msg, buttons string) {
	cTitle, cMsg, cButtons := C.CString(title), C.CString(msg), C.CString(buttons)
	defer C.free(unsafe.Pointer(cTitle))
	defer C.free(unsafe.Pointer(cMsg))
	defer C.free(unsafe.Pointer(cButtons))

	C.IupMessageAlarm(parent.ptr(), cTitle, cMsg, cButtons)
}

// LayoutDialog creates a Layout Dialog. It is a predefined dialog to visually edit the layout of another dialog in run time.
// It is a standard Dialog constructed with other IUP elements.
// The dialog can be shown with any of the show functions Show, ShowXY or Popup.
//
// This is a dialog intended for developers, so they can see and inspect their dialogs in other ways.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iuplayoutdialog.html
func LayoutDialog(dialog Ihandle) Ihandle {
	h := mkih(C.IupLayoutDialog(dialog.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ElementPropertiesDialog creates an Element Properties Dialog.
// It is a predefined dialog to edit the properties of an element in run time.
// It is a standard Dialog constructed with other IUP elements.
// The dialog can be shown with any of the show functions Show, ShowXY or Popup.
//
// This is a dialog intended for developers, so they can see and inspect their elements in other ways.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupelementpropdialog.html
func ElementPropertiesDialog(parent, elem Ihandle) Ihandle {
	h := mkih(C.IupElementPropertiesDialog(parent.ptr(), elem.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// GlobalsDialog creates an Globals Dialog.
// It is a predefined dialog to check and edit global attributes, functions (read-only) and names (read-only) in run time.
// It is a standard Dialog constructed with other IUP elements.
// The dialog can be shown with any of the show functions Show, ShowXY or Popup.
//
// This is a dialog intended for developers, so they can see and inspect their globals in other ways.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupglobalsdialog.html
func GlobalsDialog() Ihandle {
	h := mkih(C.IupGlobalsDialog())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ClassInfoDialog creates an Iup Class Information dialog.
// It is a predefined dialog to show all registered classes, each class attributes and callbacks.
// It is a standard IupDialog constructed with other IUP elements.
// The dialog can be shown with any of the show functions Show, ShowXY or Popup.
//
// This is a dialog intended for developers, so they can see attributes and callbacks information of a class.
//
// https://www.tecgraf.puc-rio.br/iup/en/dlg/iupclassinfodialog.html
func ClassInfoDialog(dialog Ihandle) Ihandle {
	h := mkih(C.IupClassInfoDialog(dialog.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

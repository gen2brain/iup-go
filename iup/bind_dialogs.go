package iup

import (
	"bytes"
	"fmt"
	"image/color"
	"runtime/cgo"
	"strings"
	"unsafe"

	"github.com/google/uuid"
)

/*
#include <stdlib.h>
#include <string.h>
#include "iup.h"
#include "bind_callbacks.h"
*/
import "C"

// Dialog creates a dialog element. It manages user interaction with the interface elements.
// For any interface element to be shown, it must be encapsulated in a dialog.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_dialog.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_popup.md
func Popup(ih Ihandle, x, y int) int {
	return int(C.IupPopup(ih.ptr(), C.int(x), C.int(y)))
}

// Show displays a dialog in the current position, or changes a control VISIBLE attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_show.md
func Show(ih Ihandle) int {
	return int(C.IupShow(ih.ptr()))
}

// ShowXY displays a dialog in a given position on the screen.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_showxy.md
func ShowXY(ih Ihandle, x, y int) int {
	return int(C.IupShowXY(ih.ptr(), C.int(x), C.int(y)))
}

// Hide hides an interface element.
// This function has the same effect as attributing value "NO" to the interface element’s VISIBLE attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_hide.md
func Hide(ih Ihandle) int {
	return int(C.IupHide(ih.ptr()))
}

// FileDlg creates the File Dialog element. It is a predefined dialog for selecting files or a directory.
// The dialog can be shown with the Popup function only.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_filedlg.md
func FileDlg() Ihandle {
	h := mkih(C.IupFileDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// MessageDlg creates the Message Dialog element. It is a predefined dialog for displaying a message.
// The dialog can be shown with the Popup function only.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_messagedlg.md
func MessageDlg() Ihandle {
	h := mkih(C.IupMessageDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ColorDlg creates the Color Dialog element. It is a predefined dialog for selecting a color.
// The Windows and GTK dialogs can be shown only with the Popup function.
// The ColorBrowser based dialog is a Dialog that can be shown as any regular Dialog.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_colordlg.md
func ColorDlg() Ihandle {
	h := mkih(C.IupColorDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FontDlg creates the Font Dialog element. It is a predefined dialog for selecting a font.
// The dialog can be shown with the Popup function only.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_fontdlg.md
func FontDlg() Ihandle {
	h := mkih(C.IupFontDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ProgressDlg creates a progress dialog element. It is a predefined dialog for displaying the progress of an operation.
// The dialog is meant to be shown with the show functions Show or ShowXY.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_progressdlg.md
func ProgressDlg() Ihandle {
	h := mkih(C.IupProgressDlg())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Alarm shows a modal dialog containing a message and up to three buttons.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_alarm.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_getfile.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_getcolor.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_gettext.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_listdialog.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_message.md
func Message(title, msg string) {
	cTitle, cMsg := C.CString(title), C.CString(msg)
	defer C.free(unsafe.Pointer(cTitle))
	defer C.free(unsafe.Pointer(cMsg))

	C.IupMessage(cTitle, cMsg)
}

// MessageError shows a modal dialog containing an error message.
// It simply creates and popup a MessageDlg with DIALOGTYPE=ERROR.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_messageerror.md
func MessageError(parent Ihandle, msg string) {
	cMsg := C.CString(msg)
	defer C.free(unsafe.Pointer(cMsg))

	C.IupMessageError(parent.ptr(), cMsg)
}

// MessageAlarm shows a modal dialog containing a question message, similar to Alarm.
// It simply creates and popup a MessageDlg with DIALOGTYPE=QUESTION.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_messagealarm.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_layoutdialog.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_elementpropdialog.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_globalsdialog.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_classinfodialog.md
func ClassInfoDialog(dialog Ihandle) Ihandle {
	h := mkih(C.IupClassInfoDialog(dialog.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Param creates a Param element from a format string line.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_param.md
func Param(format string) Ihandle {
	cFormat := C.CString(format)
	defer C.free(unsafe.Pointer(cFormat))

	return mkih(C.IupParam(cFormat))
}

// ParamBox creates a ParamBox element from an array of Param elements.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_parambox.md
func ParamBox(params ...Ihandle) Ihandle {
	cParams := make([]*C.Ihandle, len(params)+1)
	for i, p := range params {
		cParams[i] = p.ptr()
	}
	cParams[len(params)] = nil

	return mkih(C.IupParamBoxv((**C.Ihandle)(unsafe.Pointer(&cParams[0]))))
}

// getParamInfo parses a GetParam format string and returns the parameter count,
// extra count (separators and button names), and the type letter for each data parameter.
func getParamInfo(format string) (paramCount, paramExtra int, types []byte) {
	lines := strings.Split(format, "\n")
	for _, line := range lines {
		if len(line) == 0 {
			continue
		}

		pctIdx := strings.LastIndex(line, "%")
		if pctIdx < 0 || pctIdx+1 >= len(line) {
			continue
		}
		typeLetter := line[pctIdx+1]

		switch typeLetter {
		case 't', 'u':
			paramExtra++
		case 'x':
			paramCount++
		default:
			paramCount++
			types = append(types, typeLetter)
		}
	}
	return
}

// GetParam shows a modal dialog with automatically created controls based on the format string.
// Each line in the format string describes a parameter. The data arguments must be pointers
// matching the format types:
//   - %b, %i, %l, %o → *int
//   - %r, %a → *float32
//   - %R, %A → *float64
//   - %s, %m, %c, %f, %n, %d → *string
//   - %h → Ihandle (not a pointer)
//   - %t, %u → no data argument
//
// Returns 1 if the user pressed OK, 0 if Cancel.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_getparam.md
func GetParam(title string, action GetParamFunc, format string, data ...interface{}) int {
	cTitle := C.CString(title)
	defer C.free(unsafe.Pointer(cTitle))
	cFormat := C.CString(format)
	defer C.free(unsafe.Pointer(cFormat))

	paramCount, paramExtra, types := getParamInfo(format)

	if len(types) != len(data) {
		panic(fmt.Sprintf("GetParam: format expects %d data arguments, got %d", len(types), len(data)))
	}

	paramData := make([]unsafe.Pointer, paramCount)

	type allocation struct {
		ptr unsafe.Pointer
	}
	var allocs []allocation

	p := 0
	dataIdx := 0
	for i := 0; i < paramCount; i++ {
		if p < len(types) && i == p {
			switch types[p] {
			case 'b', 'i', 'l', 'o':
				ip, ok := data[dataIdx].(*int)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *int for %%%c", dataIdx, types[p]))
				}
				cVal := (*C.int)(C.malloc(C.size_t(unsafe.Sizeof(C.int(0)))))
				*cVal = C.int(*ip)
				paramData[i] = unsafe.Pointer(cVal)
				allocs = append(allocs, allocation{unsafe.Pointer(cVal)})
			case 'r', 'a':
				fp, ok := data[dataIdx].(*float32)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *float32 for %%%c", dataIdx, types[p]))
				}
				cVal := (*C.float)(C.malloc(C.size_t(unsafe.Sizeof(C.float(0)))))
				*cVal = C.float(*fp)
				paramData[i] = unsafe.Pointer(cVal)
				allocs = append(allocs, allocation{unsafe.Pointer(cVal)})
			case 'R', 'A':
				dp, ok := data[dataIdx].(*float64)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *float64 for %%%c", dataIdx, types[p]))
				}
				cVal := (*C.double)(C.malloc(C.size_t(unsafe.Sizeof(C.double(0)))))
				*cVal = C.double(*dp)
				paramData[i] = unsafe.Pointer(cVal)
				allocs = append(allocs, allocation{unsafe.Pointer(cVal)})
			case 's', 'm', 'c', 'f', 'n', 'd':
				sp, ok := data[dataIdx].(*string)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *string for %%%c", dataIdx, types[p]))
				}
				var bufSize int
				switch types[p] {
				case 'm':
					bufSize = 10240
				case 'f':
					bufSize = 4096
				default:
					bufSize = 512
				}
				cBuf := (*C.char)(C.malloc(C.size_t(bufSize)))
				cStr := C.CString(*sp)
				C.strncpy(cBuf, cStr, C.size_t(bufSize-1))
				*(*C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(cBuf)) + uintptr(bufSize-1))) = 0
				C.free(unsafe.Pointer(cStr))
				paramData[i] = unsafe.Pointer(cBuf)
				allocs = append(allocs, allocation{unsafe.Pointer(cBuf)})
			case 'h':
				ih, ok := data[dataIdx].(Ihandle)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be Ihandle for %%h", dataIdx))
				}
				paramData[i] = unsafe.Pointer(ih.ptr())
			default:
				panic(fmt.Sprintf("GetParam: unsupported format type %%%c", types[p]))
			}
			p++
			dataIdx++
		} else {
			paramData[i] = nil
		}
	}

	defer func() {
		for _, a := range allocs {
			C.free(a.ptr)
		}
	}()

	var paramDataPtr *unsafe.Pointer
	if len(paramData) > 0 {
		paramDataPtr = &paramData[0]
	}

	var ret C.int
	if action != nil {
		ch := cgo.NewHandle(action)
		defer ch.Delete()
		ret = C.goIupCallGetParamv(cTitle, C.uintptr_t(ch), cFormat,
			C.int(paramCount), C.int(paramExtra), (*unsafe.Pointer)(unsafe.Pointer(paramDataPtr)))
	} else {
		ret = C.goIupCallGetParamvNoAction(cTitle, cFormat,
			C.int(paramCount), C.int(paramExtra), (*unsafe.Pointer)(unsafe.Pointer(paramDataPtr)))
	}

	if ret == 1 {
		dataIdx = 0
		for i := 0; i < len(types); i++ {
			switch types[i] {
			case 'b', 'i', 'l', 'o':
				ip := data[dataIdx].(*int)
				*ip = int(*(*C.int)(paramData[i]))
			case 'r', 'a':
				fp := data[dataIdx].(*float32)
				*fp = float32(*(*C.float)(paramData[i]))
			case 'R', 'A':
				dp := data[dataIdx].(*float64)
				*dp = float64(*(*C.double)(paramData[i]))
			case 's', 'm', 'c', 'f', 'n', 'd':
				sp := data[dataIdx].(*string)
				*sp = C.GoString((*C.char)(paramData[i]))
			}
			dataIdx++
		}
	}

	return int(ret)
}

//go:build js && wasm

package iup

import (
	"image/color"
	"strings"
)

// FileDlg creates the File Dialog element. It is a predefined dialog for selecting files or a directory.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_filedlg.md
func FileDlg() Ihandle {
	return ccallHandle("IupFileDlg", nil, nil)
}

// MessageDlg creates the Message Dialog element. It is a predefined dialog for displaying a message.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_messagedlg.md
func MessageDlg() Ihandle {
	return ccallHandle("IupMessageDlg", nil, nil)
}

// ColorDlg creates the Color Dialog element. It is a predefined dialog for selecting a color.
// Under WebAssembly it is the platform-independent ColorBrowser-based dialog and can be shown as any regular Dialog.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_colordlg.md
func ColorDlg() Ihandle {
	return ccallHandle("IupColorDlg", nil, nil)
}

// ColorBrowser creates a color browser control.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_colorbrowser.md
func ColorBrowser() Ihandle {
	return ccallHandle("IupColorBrowser", nil, nil)
}

// ProgressDlg creates a progress dialog element. It is a predefined dialog for displaying the progress of an operation.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_progressdlg.md
func ProgressDlg() Ihandle {
	return ccallHandle("IupProgressDlg", nil, nil)
}

// GetFile shows a modal dialog to select a filename. Uses the FileDlg element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_getfile.md
func GetFile(path string) (sel string, ret int) {
	if len(path) > 4095 {
		panic("path is too long (maximum is 4095)")
	}
	ptr := wasmMalloc(4096)
	defer wasmFree(ptr)
	wasmBufWrite(ptr, path, 4096)

	ret = ccall("IupGetFile", "number", []interface{}{"number"}, []interface{}{ptr}).Int()
	sel = wasmPtrToStr(ptr)
	return
}

// GetColor shows a modal dialog which allows the user to select a color. Based on ColorDlg.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_getcolor.md
func GetColor(x, y int) (col color.RGBA, ret int) {
	pr := wasmMalloc(3)
	defer wasmFree(pr)
	pg, pb := pr+1, pr+2
	module().Call("setValue", pr, 255, "i8")
	module().Call("setValue", pg, 255, "i8")
	module().Call("setValue", pb, 255, "i8")

	ret = ccall("IupGetColor", "number",
		[]interface{}{"number", "number", "number", "number", "number"},
		[]interface{}{x, y, pr, pg, pb}).Int()
	return color.RGBA{wasmGetU8(pr), wasmGetU8(pg), wasmGetU8(pb), 255}, ret
}

// GetText shows a modal dialog to edit a multiline text.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_gettext.md
func GetText(title, text string, maxSize int) (string, int) {
	if maxSize <= 0 {
		maxSize = 10240
	}
	buf := wasmMalloc(maxSize)
	defer wasmFree(buf)
	wasmBufWrite(buf, text, maxSize)

	ret := ccall("IupGetText", "number", []interface{}{"string", "number", "number"}, []interface{}{title, buf, maxSize}).Int()
	return wasmPtrToStr(buf), ret
}

// ListDialog shows a modal dialog to select items from a simple or multiple selection list.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_listdialog.md
func ListDialog(_type int, title string, list []string, op, maxCol, maxLin int, marks *[]bool) (ret int) {
	if len(list) != len(*marks) {
		panic("bad parameter passed to ListDialog")
	}

	cTitle := wasmStrToPtr(title)
	defer wasmFree(cTitle)

	listArr := wasmMalloc(len(list) * 4)
	defer wasmFree(listArr)
	for i, s := range list {
		p := wasmStrToPtr(s)
		defer wasmFree(p)
		wasmSetI32(listArr+i*4, p)
	}

	marksArr := wasmMalloc(len(list) * 4)
	defer wasmFree(marksArr)
	for i, m := range *marks {
		v := 0
		if m {
			v = 1
		}
		wasmSetI32(marksArr+i*4, v)
	}

	ret = ccall("IupListDialog", "number",
		[]interface{}{"number", "number", "number", "number", "number", "number", "number", "number"},
		[]interface{}{_type, cTitle, len(list), listArr, op, maxCol, maxLin, marksArr}).Int()

	for i := range *marks {
		(*marks)[i] = wasmGetI32(marksArr+i*4) != 0
	}
	return
}

func getParamInfo(format string) (paramCount, paramExtra int, types []byte) {
	for _, line := range strings.Split(format, "\n") {
		if len(line) == 0 {
			continue
		}
		pct := strings.LastIndex(line, "%")
		if pct < 0 || pct+1 >= len(line) {
			continue
		}
		switch t := line[pct+1]; t {
		case 't', 'u':
			paramExtra++
		case 'x':
			paramCount++
		default:
			paramCount++
			types = append(types, t)
		}
	}
	return
}

var curGetParamAction GetParamFunc

// GetParam shows a modal dialog with controls built from the format string.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_getparam.md
func GetParam(title string, action GetParamFunc, format string, data ...interface{}) int {
	paramCount, paramExtra, types := getParamInfo(format)
	if len(types) != len(data) {
		panic("GetParam: format/data argument count mismatch")
	}

	paramData := wasmMalloc(paramCount * 4)
	defer wasmFree(paramData)

	var frees []int
	defer func() {
		for _, p := range frees {
			wasmFree(p)
		}
	}()

	for i := 0; i < paramCount; i++ {
		if i >= len(types) {
			wasmSetI32(paramData+i*4, 0)
			continue
		}
		switch types[i] {
		case 'b', 'i', 'l', 'o':
			p := wasmMalloc(4)
			wasmSetI32(p, *data[i].(*int))
			frees = append(frees, p)
			wasmSetI32(paramData+i*4, p)
		case 'r', 'a':
			p := wasmMalloc(4)
			wasmSetF32(p, *data[i].(*float32))
			frees = append(frees, p)
			wasmSetI32(paramData+i*4, p)
		case 'R', 'A':
			p := wasmMalloc(8)
			wasmSetF64(p, *data[i].(*float64))
			frees = append(frees, p)
			wasmSetI32(paramData+i*4, p)
		case 's', 'm', 'c', 'f', 'n', 'd':
			n := 512
			if types[i] == 'm' {
				n = 10240
			} else if types[i] == 'f' {
				n = 4096
			}
			p := wasmMalloc(n)
			wasmBufWrite(p, *data[i].(*string), n)
			frees = append(frees, p)
			wasmSetI32(paramData+i*4, p)
		case 'h':
			wasmSetI32(paramData+i*4, int(data[i].(Ihandle)))
		default:
			panic("GetParam: unsupported format type")
		}
	}

	curGetParamAction = action
	hasAction := 0
	if action != nil {
		hasAction = 1
	}
	ret := ccall("iupwasmGetParamv", "number",
		[]interface{}{"string", "string", "number", "number", "number", "number"},
		[]interface{}{title, format, hasAction, paramCount, paramExtra, paramData}).Int()
	curGetParamAction = nil

	if ret == 1 {
		for i := 0; i < len(types); i++ {
			p := wasmGetI32(paramData + i*4)
			switch types[i] {
			case 'b', 'i', 'l', 'o':
				*data[i].(*int) = wasmGetI32(p)
			case 'r', 'a':
				*data[i].(*float32) = wasmGetF32(p)
			case 'R', 'A':
				*data[i].(*float64) = wasmGetF64(p)
			case 's', 'm', 'c', 'f', 'n', 'd':
				*data[i].(*string) = wasmPtrToStr(p)
			}
		}
	}
	return ret
}

func ClassInfoDialog(dialog Ihandle) Ihandle {
	return ccallHandle("IupClassInfoDialog", []interface{}{"number"}, []interface{}{int(dialog)})
}

func ElementPropertiesDialog(parent, elem Ihandle) Ihandle {
	return ccallHandle("IupElementPropertiesDialog", []interface{}{"number", "number"}, []interface{}{int(parent), int(elem)})
}

func GlobalsDialog() Ihandle {
	return ccallHandle("IupGlobalsDialog", nil, nil)
}

func Hide(ih Ihandle) int {
	return ccall("IupHide", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
}

func Param(format string) Ihandle {
	return ccallHandle("IupParam", []interface{}{"string"}, []interface{}{format})
}

func VersionShow() {
	ccall("IupVersionShow", "", nil, nil)
}

func ParamBox(params ...Ihandle) Ihandle { return newContainer("parambox", params) }

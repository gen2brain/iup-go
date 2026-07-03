//go:build !cgo && !js

package iup

import (
	"fmt"
	"image/color"
	"runtime"
	"strings"
	"unsafe"
)

func Param(format string) Ihandle {
	return mkih(iupParam(format))
}

func ParamBox(params ...Ihandle) Ihandle {
	return mkih(iupParamBoxv(childrenArray(params)))
}

func ListDialog(_type int, title string, list []string, op, maxCol, maxLin int, marks *[]bool) int {
	if len(list) != len(*marks) {
		panic("bad parameter passed to ListDialog")
	}
	bufs := make([][]byte, len(list))
	pList := make([]uintptr, len(list))
	for i, s := range list {
		bufs[i] = append([]byte(s), 0)
		pList[i] = uintptr(unsafe.Pointer(&bufs[i][0]))
	}
	pMark := make([]int32, len(list))
	for i := range list {
		if (*marks)[i] {
			pMark[i] = 1
		}
	}
	ret := int(iupListDialog(int32(_type), optCStr(title), int32(len(list)), &pList[0],
		int32(op), int32(maxCol), int32(maxLin), &pMark[0]))
	for i := range list {
		(*marks)[i] = pMark[i] != 0
	}
	runtime.KeepAlive(bufs)
	return ret
}

func Dialog(child Ihandle) Ihandle {
	return mkih(iupDialog(uintptr(child)))
}

func Popup(ih Ihandle, x, y int) int {
	return int(iupPopup(uintptr(ih), int32(x), int32(y)))
}

func Hide(ih Ihandle) int {
	return int(iupHide(uintptr(ih)))
}

func FileDlg() Ihandle {
	return mkih(iupFileDlg())
}

func MessageDlg() Ihandle {
	return mkih(iupMessageDlg())
}

func ColorDlg() Ihandle {
	return mkih(iupColorDlg())
}

func FontDlg() Ihandle {
	return mkih(iupFontDlg())
}

func ProgressDlg() Ihandle {
	return mkih(iupProgressDlg())
}

func Alarm(title, msg, b1, b2, b3 string) int {
	return int(iupAlarm(optCStr(title), optCStr(msg), optCStr(b1), optCStr(b2), optCStr(b3)))
}

func GetColor(x, y int) (col color.RGBA, ret int) {
	var r, g, b uint8
	ret = int(iupGetColor(int32(x), int32(y), &r, &g, &b))
	col = color.RGBA{R: r, G: g, B: b, A: 255}
	return
}

func Message(title, msg string) {
	iupMessage(optCStr(title), optCStr(msg))
}

func MessageError(parent Ihandle, msg string) {
	iupMessageError(uintptr(parent), optCStr(msg))
}

func MessageAlarm(parent Ihandle, title, msg, buttons string) {
	iupMessageAlarm(uintptr(parent), optCStr(title), optCStr(msg), optCStr(buttons))
}

func VersionShow() {
	iupVersionShow()
}

func GetText(title, text string, maxSize int) (string, int) {
	bufSize := maxSize
	if maxSize <= 0 {
		bufSize = len(text)
	}
	if bufSize < 1 {
		bufSize = 1
	}
	buf := make([]byte, bufSize+1)
	copy(buf, text)
	ret := int(iupGetText(optCStr(title), &buf[0], int32(maxSize)))
	return goString(uintptr(unsafe.Pointer(&buf[0]))), ret
}

func GetFile(path string) (sel string, ret int) {
	if len(path) > 4095 {
		panic("path is too long (maximum is 4095)")
	}
	buf := make([]byte, 4096)
	copy(buf, path)
	ret = int(iupGetFile(&buf[0]))
	sel = goString(uintptr(unsafe.Pointer(&buf[0])))
	return
}

func ClassInfoDialog(dialog Ihandle) Ihandle {
	return mkih(iupClassInfoDialog(uintptr(dialog)))
}

func ElementPropertiesDialog(parent, elem Ihandle) Ihandle {
	return mkih(iupElementPropertiesDialog(uintptr(parent), uintptr(elem)))
}

func GlobalsDialog() Ihandle {
	return mkih(iupGlobalsDialog())
}

func getParamInfo(format string) (paramCount, paramExtra int, types []byte) {
	for _, line := range strings.Split(format, "\n") {
		if len(line) == 0 {
			continue
		}
		pctIdx := strings.LastIndex(line, "%")
		if pctIdx < 0 || pctIdx+1 >= len(line) {
			continue
		}
		switch typeLetter := line[pctIdx+1]; typeLetter {
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

func GetParam(title string, action GetParamFunc, format string, data ...interface{}) int {
	paramCount, paramExtra, types := getParamInfo(format)
	if len(types) != len(data) {
		panic(fmt.Sprintf("GetParam: format expects %d data arguments, got %d", len(types), len(data)))
	}

	paramData := make([]uintptr, paramCount)
	// IUP reads/writes these buffers during the modal IupGetParamv call; Go
	// memory stays valid (non-moving GC) as long as keep references it.
	keep := make([][]byte, 0, paramCount)
	buf := func(n int) []byte {
		b := make([]byte, n)
		keep = append(keep, b)
		return b
	}

	p, dataIdx := 0, 0
	for i := 0; i < paramCount; i++ {
		if p < len(types) && i == p {
			switch types[p] {
			case 'b', 'i', 'l', 'o':
				ip, ok := data[dataIdx].(*int)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *int for %%%c", dataIdx, types[p]))
				}
				b := buf(4)
				*(*int32)(unsafe.Pointer(&b[0])) = int32(*ip)
				paramData[i] = uintptr(unsafe.Pointer(&b[0]))
			case 'r', 'a':
				fp, ok := data[dataIdx].(*float32)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *float32 for %%%c", dataIdx, types[p]))
				}
				b := buf(4)
				*(*float32)(unsafe.Pointer(&b[0])) = *fp
				paramData[i] = uintptr(unsafe.Pointer(&b[0]))
			case 'R', 'A':
				dp, ok := data[dataIdx].(*float64)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *float64 for %%%c", dataIdx, types[p]))
				}
				b := buf(8)
				*(*float64)(unsafe.Pointer(&b[0])) = *dp
				paramData[i] = uintptr(unsafe.Pointer(&b[0]))
			case 's', 'm', 'c', 'f', 'n', 'd':
				sp, ok := data[dataIdx].(*string)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be *string for %%%c", dataIdx, types[p]))
				}
				bufSize := 512
				switch types[p] {
				case 'm':
					bufSize = 10240
				case 'f':
					bufSize = 4096
				}
				b := buf(bufSize)
				copy(b[:bufSize-1], *sp)
				paramData[i] = uintptr(unsafe.Pointer(&b[0]))
			case 'h':
				ih, ok := data[dataIdx].(Ihandle)
				if !ok {
					panic(fmt.Sprintf("GetParam: data[%d] must be Ihandle for %%h", dataIdx))
				}
				paramData[i] = uintptr(ih)
			default:
				panic(fmt.Sprintf("GetParam: unsupported format type %%%c", types[p]))
			}
			p++
			dataIdx++
		}
	}

	var pd *uintptr
	if len(paramData) > 0 {
		pd = &paramData[0]
	}

	var ret int32
	if action != nil {
		id := cbStore(action)
		ret = iupGetParamv(optCStr(title), getParamCB, uintptr(id), format, int32(paramCount), int32(paramExtra), pd)
		cbDelete(id)
	} else {
		ret = iupGetParamv(optCStr(title), 0, 0, format, int32(paramCount), int32(paramExtra), pd)
	}

	if ret == 1 {
		dataIdx = 0
		for i := 0; i < len(types); i++ {
			switch types[i] {
			case 'b', 'i', 'l', 'o':
				*data[dataIdx].(*int) = int(*(*int32)(unsafe.Pointer(paramData[i])))
			case 'r', 'a':
				*data[dataIdx].(*float32) = *(*float32)(unsafe.Pointer(paramData[i]))
			case 'R', 'A':
				*data[dataIdx].(*float64) = *(*float64)(unsafe.Pointer(paramData[i]))
			case 's', 'm', 'c', 'f', 'n', 'd':
				*data[dataIdx].(*string) = goString(paramData[i])
			}
			dataIdx++
		}
	}

	runtime.KeepAlive(keep)
	return int(ret)
}

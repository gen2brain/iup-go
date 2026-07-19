//go:build js && wasm

package iup

import (
	"image"
	"image/draw"
)

func Execute(fileName, parameters string) int {
	return ccall("IupExecute", "number", []interface{}{"string", "string"}, []interface{}{fileName, parameters}).Int()
}

func ExecuteWait(fileName, parameters string) int {
	return ccall("IupExecuteWait", "number", []interface{}{"string", "string"}, []interface{}{fileName, parameters}).Int()
}

func GetAllDialogs() []string { return wasmNames("IupGetAllDialogs", nil, nil) }

func GetAllNames() []string { return wasmNames("IupGetAllNames", nil, nil) }

func GetFocus() Ihandle {
	return ccallHandle("IupGetFocus", nil, nil)
}

func GetLanguage() string {
	return ccall("IupGetLanguage", "string", nil, nil).String()
}

func GetLanguageString(name string) string {
	return ccall("IupGetLanguageString", "string", []interface{}{"string"}, []interface{}{name}).String()
}

func ImageFromImage(i image.Image) Ihandle {
	img, ok := i.(*image.RGBA)
	if !ok {
		b := i.Bounds()
		dst := image.NewRGBA(image.Rect(0, 0, b.Dx(), b.Dy()))
		draw.Draw(dst, dst.Bounds(), i, b.Min, draw.Src)
		img = dst
	}
	return ImageRGBA(img.Bounds().Dx(), img.Bounds().Dy(), img.Pix)
}

func ImageGetHandle(name string) Ihandle {
	return ccallHandle("IupImageGetHandle", []interface{}{"string"}, []interface{}{name})
}

func ImageFromHandle(handle uintptr) Ihandle {
	return ccallHandle("IupImageFromHandle", []interface{}{"number"}, []interface{}{int(handle)})
}

func ImageSave(ih Ihandle, filename, format string) int {
	return ccall("IupImageSave", "number", []interface{}{"number", "string", "string"}, []interface{}{int(ih), filename, format}).Int()
}

func ImageSaveToBuffer(ih Ihandle, format string) []byte {
	psize := wasmMalloc(4)
	defer wasmFree(psize)
	ptr := ccall("IupImageSaveToBuffer", "number", []interface{}{"number", "string", "number"}, []interface{}{int(ih), format, psize}).Int()
	if ptr == 0 {
		return nil
	}
	size := wasmGetI32(psize)
	out := make([]byte, size)
	h8 := module().Get("HEAPU8")
	for i := 0; i < size; i++ {
		out[i] = byte(h8.Index(ptr + i).Int())
	}
	return out
}

func ImageToImage(ih Ihandle) *image.RGBA {
	width := GetInt(ih, "WIDTH")
	height := GetInt(ih, "HEIGHT")
	bpp := GetInt(ih, "BPP")
	if width == 0 || height == 0 || (bpp != 24 && bpp != 32) {
		return nil
	}
	channels := bpp / 8
	size := width * height * channels
	ptr := ccall("IupGetAttribute", "number", []interface{}{"number", "string"}, []interface{}{int(ih), "WID"}).Int()
	if ptr == 0 {
		return nil
	}
	src := make([]byte, size)
	h8 := module().Get("HEAPU8")
	for i := 0; i < size; i++ {
		src[i] = byte(h8.Index(ptr + i).Int())
	}
	img := image.NewRGBA(image.Rect(0, 0, width, height))
	if bpp == 32 {
		copy(img.Pix, src)
	} else {
		for i, j := 0, 0; i < size; i, j = i+3, j+4 {
			img.Pix[j] = src[i]
			img.Pix[j+1] = src[i+1]
			img.Pix[j+2] = src[i+2]
			img.Pix[j+3] = 255
		}
	}
	return img
}

func Log(_type, str string) {
	ccall("IupLog", "", []interface{}{"string", "string"}, []interface{}{_type, str})
}

func NextField(ih Ihandle) Ihandle {
	return ccallHandle("IupNextField", []interface{}{"number"}, []interface{}{int(ih)})
}

func PreviousField(ih Ihandle) Ihandle {
	return ccallHandle("IupPreviousField", []interface{}{"number"}, []interface{}{int(ih)})
}

func SetFocus(ih Ihandle) Ihandle {
	return ccallHandle("IupSetFocus", []interface{}{"number"}, []interface{}{int(ih)})
}

func SetLanguage(lng string) {
	ccall("IupSetLanguage", "", []interface{}{"string"}, []interface{}{lng})
}

func SetLanguagePack(ih Ihandle) {
	ccall("IupSetLanguagePack", "", []interface{}{"number"}, []interface{}{int(ih)})
}

func SetLanguageString(name, str string) {
	ccall("IupStoreLanguageString", "", []interface{}{"string", "string"}, []interface{}{name, str})
}

func Thread() Ihandle {
	ih := ccallHandle("IupThread", nil, nil)
	wasmRegisterThread(ih)
	return ih
}

func Tray() Ihandle {
	return ccallHandle("IupTray", nil, nil)
}

func User() Ihandle {
	return ccallHandle("IupUser", nil, nil)
}

//go:build !cgo && !js

package iup

import (
	"image"
	"image/draw"
	"unsafe"
)

func ImageFromImage(i image.Image) Ihandle {
	if img, ok := i.(*image.RGBA); ok {
		return mkih(iupImageRGBA(int32(img.Bounds().Dx()), int32(img.Bounds().Dy()), img.Pix))
	}
	b := i.Bounds()
	dst := image.NewRGBA(image.Rect(0, 0, b.Dx(), b.Dy()))
	draw.Draw(dst, dst.Bounds(), i, b.Min, draw.Src)
	return mkih(iupImageRGBA(int32(dst.Bounds().Dx()), int32(dst.Bounds().Dy()), dst.Pix))
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
	ptr := iupGetAttributeRaw(uintptr(ih), "WID")
	if ptr == 0 {
		return nil
	}
	src := unsafe.Slice((*byte)(goPtr(ptr)), size)
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

func ImageSaveToBuffer(ih Ihandle, format string) []byte {
	var size int32
	buf := iupImageSaveToBuffer(uintptr(ih), format, &size)
	if buf == 0 {
		return nil
	}
	out := make([]byte, size)
	copy(out, unsafe.Slice((*byte)(goPtr(buf)), int(size)))
	if cfree != nil {
		cfree(buf)
	}
	return out
}

func Image(width, height int, pixMap []byte) Ihandle {
	return mkih(iupImage(int32(width), int32(height), pixMap))
}

func ImageRGB(width, height int, pixMap []byte) Ihandle {
	return mkih(iupImageRGB(int32(width), int32(height), pixMap))
}

func ImageRGBA(width, height int, pixMap []byte) Ihandle {
	return mkih(iupImageRGBA(int32(width), int32(height), pixMap))
}

func ImageGetHandle(name string) Ihandle {
	return mkih(iupImageGetHandle(name))
}

// ImageFromHandle creates an IUP image from a native image handle (e.g. the IupClipboard NATIVEIMAGE).
func ImageFromHandle(handle uintptr) Ihandle {
	return mkih(iupImageFromHandle(handle))
}

func Menu(children ...Ihandle) Ihandle {
	return mkih(iupMenuv(childrenArray(children)))
}

func MenuItem(title string) Ihandle {
	return mkih(iupMenuItem(optCStr(title)))
}

func MenuSeparator() Ihandle {
	return mkih(iupMenuSeparator())
}

func Submenu(title string, child Ihandle) Ihandle {
	return mkih(iupSubmenu(optCStr(title), uintptr(child)))
}

func NextField(ih Ihandle) Ihandle {
	return mkih(iupNextField(uintptr(ih)))
}

func PreviousField(ih Ihandle) Ihandle {
	return mkih(iupPreviousField(uintptr(ih)))
}

func GetFocus() Ihandle {
	return mkih(iupGetFocus())
}

func SetFocus(ih Ihandle) Ihandle {
	iupSetFocus(uintptr(ih))
	return ih
}

func GetName(ih Ihandle) string {
	return iupGetName(uintptr(ih))
}

func Clipboard() Ihandle {
	return mkih(iupClipboard())
}

func Thread() Ihandle {
	return mkih(iupThread())
}

func Tray() Ihandle {
	return mkih(iupTray())
}

func User() Ihandle { return mkih(iupUser()) }

func Notify() Ihandle { return mkih(iupNotify()) }

func SetLanguage(lng string) { iupSetLanguage(lng) }

func GetLanguage() string { return iupGetLanguage() }

func SetLanguageString(name, str string) { iupStoreLanguageString(name, str) }

func GetLanguageString(name string) string { return iupGetLanguageString(name) }

func SetLanguagePack(ih Ihandle) { iupSetLanguagePack(uintptr(ih)) }

func Log(_type, str string) { iupLog(_type, "%s", str) }

func ImageSave(ih Ihandle, filename, format string) int {
	return int(iupImageSave(uintptr(ih), filename, format))
}

//go:build !cgo && !js && media

package iup

import "github.com/ebitengine/purego"

var (
	iupMediaOpen func()
	iupAudio     func() uintptr
	iupCamera    func() uintptr
)

func init() {
	ensureBase()
	lib := openLib("iupmedia")
	if lib == 0 {
		panic("iup: cannot load libiupmedia")
	}
	purego.RegisterLibFunc(&iupMediaOpen, lib, "IupMediaOpen")
	purego.RegisterLibFunc(&iupAudio, lib, "IupAudio")
	purego.RegisterLibFunc(&iupCamera, lib, "IupCamera")
}

// MediaOpen must be called after Open, so that the media elements can be used.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_audio.md
func MediaOpen() {
	iupMediaOpen()
}

// Audio creates an audio player.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_audio.md
func Audio() Ihandle {
	return mkih(iupAudio())
}

// Camera creates a canvas that shows the live picture of a camera.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_camera.md
func Camera() Ihandle {
	return mkih(iupCamera())
}

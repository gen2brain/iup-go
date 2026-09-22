//go:build !cgo && !js && media

package iup

import "github.com/ebitengine/purego"

var (
	iupMediaOpen  func()
	iupAudio      func() uintptr
	iupCamera     func() uintptr
	iupMicrophone func() uintptr
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
	purego.RegisterLibFunc(&iupMicrophone, lib, "IupMicrophone")
}

// MediaOpen must be called after Open, so that the media elements can be used.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_audio.html
func MediaOpen() {
	iupMediaOpen()
}

// Audio creates an audio player.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_audio.html
func Audio() Ihandle {
	return mkih(iupAudio())
}

// Camera creates a canvas that shows the live picture of a camera.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_camera.html
func Camera() Ihandle {
	return mkih(iupCamera())
}

// Microphone creates an audio capture source.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_microphone.html
func Microphone() Ihandle {
	return mkih(iupMicrophone())
}

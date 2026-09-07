//go:build media

package iup

/*
#include "iup.h"
#include "iupmedia.h"
*/
import "C"

// MediaOpen must be called after Open, so that the media elements can be used.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_audio.md
func MediaOpen() {
	C.IupMediaOpen()
}

// Audio creates an audio player.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_audio.md
func Audio() Ihandle {
	return mkih(C.IupAudio())
}

// Camera creates a canvas that shows the live picture of a camera.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_camera.md
func Camera() Ihandle {
	return mkih(C.IupCamera())
}

// Microphone creates an audio capture source.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_microphone.md
func Microphone() Ihandle {
	return mkih(C.IupMicrophone())
}

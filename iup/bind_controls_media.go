//go:build media

package iup

/*
#include "iup.h"
#include "iupmedia.h"
*/
import "C"

// MediaOpen must be called after Open, so that the media elements can be used.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_audio.html
func MediaOpen() {
	C.IupMediaOpen()
}

// Audio creates an audio player.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_audio.html
func Audio() Ihandle {
	return mkih(C.IupAudio())
}

// Camera creates a canvas that shows the live picture of a camera.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_camera.html
func Camera() Ihandle {
	return mkih(C.IupCamera())
}

// Microphone creates an audio capture source.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_microphone.html
func Microphone() Ihandle {
	return mkih(C.IupMicrophone())
}
